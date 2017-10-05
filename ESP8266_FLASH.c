/*************************************************
* ESP8266 FLASH LIBRARY
*
* NOVEMBER 2 2016
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
*
* REFERENCES
*   (1) ESP8266 SDK GETTING STARTED GUIDE v2.8
*       SECTION 4.1.1
*       http://espressif.com/sites/default/files/documentation/2a-esp8266-sdk_getting_started_guide_en.pdf
*   (2) ESP8266 FLASH RW OPERATIONS
*       http://www.espressif.com/sites/default/files/99a-sdk-espressif_iot_flash_rw_operation_en_v1.0_0.pdf
*
* MOST OF THE 12F MODULES USE FLASH CHIP ID : 0x1640E0 (BERG MICRO 32MBIT / 4MB FLASH)
*
* FLASH IS A BLOCK DEVICE AND WORKS ON SECTORS (4KB). ESP8266 FLASH API (READ/WRITE FUNCTIONS) REQUIRE THAT THE 
* DATA TO BE WRITTEN / DATA BUFFER TO BE READ INTO BE WORD (4 BYTE) ALIGNED. THIS IS ESP8266 FLASH API RESTRICTION
* AND NOT A FLASH RESTRICTION.
*
* ALSO, THE SIZE IN BYTES NEEDS TO BE MULTIPLE OF 4 BYTES. ESP8622 FLASH API CRASHES IF THAT IS NOT THE CASE
*
* ALSO THE FLASH ADDRESS NEEDS TO BE BYTE ALIGNED (ALTHOUGH DURING TESTING I FOUND IT NEEDS TO BE SECTOR = 4KB = 4096
* BYTES ALIGNED)
*
* THIS LIBRARY STORES AT THE END OF DATA WRITTEN A SINGLE BYTE OF CRC8 TO VERIFIY DATA READING IS VALID
*
* IN GENERAL, FLASH WRITE CAN ONLY SET THE BITS TO 0, NOT TO 1. TO SET TO 1 WE NEED TO ERASE THE BIT. SINCE FLASH
* IS A BLOCK DEVICE, ERASE DOES NOT HAPPEN BYTE WISE. IT HAPPENS SECTOR WISE. SO IF YOU NEED TO UPDATE AN ALREADY
* WRITTEN LOCATION, YOU NEED TO ERASE IT FIRST. IF YOU ARE WRITING TO A LOCATION THAT IS NOT PREVIOUSLY WRITTEN TO,
* NO NEED TO ERASE IT FIRST.
*
* SO THIS LIBRARY FIRST ERASES ALL THE SECTORS IN WHICH THE USER SUPPLIED DATA WILL FALL IN THE FLASH. THEN IT WRITES
* THE USER DATA
*
*
* AS PER THE DOCUMENTS, MEMORY MAP IS AS FOLLOWING (NON FOTA)
*                                                                                                     0x400000
* +------------+------------------------+-------------------------------------------+----------+------+
* + FLASH      | IROM0TEXT              |       THIS AREA IS VARIABLE (DEPNDING     | ESP INIT | BLANK|
* +  .BIN      |      .BIN              |        ON THE SIZE OF IROM0TEXT.BIN &     |     DATA |  .BIN|
* + (MAX 64KB) |(MAX 76KB)              |        CAN BE USED TO STORE USER DATA     |    (8KB) | (8KB)|
* +------------+------------------------+-------------------------------------------+----------+------+
* 0x00000      0x10000                                                              0x3FC000   0x3FE000
************************************************/

#include "ESP8266_FLASH.h"

//LOCAL LIBRARY VARIABLES////////////////////////////////
//DEBUG RELATED
static uint8_t _esp8266_flash_debug;
//END LOCAL LIBRARY VARIABLES/////////////////////////////

void ICACHE_FLASH_ATTR ESP8266_FLASH_SetDebug(uint8_t debug_on)
{
    //SET DEBUG PRINTF ON(1) OR OFF(0)

    _esp8266_flash_debug = debug_on;
}

SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_EraseSector(uint16_t sector_number)
{
    //ERASE THE SPECIFIED SECTOR NUMBER

    //CHECK FOR SECTOR VALIDITY
    if(_esp8266_flash_check_address_validity(sector_number * ESP8266_FLASH_SECTOR_SIZE_BYTES))
    {
        //VALID
        if(_esp8266_flash_debug)
        {
            os_printf("ESP8266 : FLASH : Sector #%u erased\n", sector_number);
        }
        return spi_flash_erase_sector(sector_number);
    }
    //INVALID
    if(_esp8266_flash_debug)
    {
        os_printf("ESP8266 : FLASH : Sector #%u invalid. Not erased\n", sector_number);
    }
    return SPI_FLASH_RESULT_ERR;
}

SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_EraseBlock(uint16_t sector_number, uint16_t num_blocks)
{
    //ERASE A CHAIN OF SECTORS STARTING AT THE SPECIFIED SECTOR
    //VALIDITY OF A SECTOR ADDRESS WOULD BE DONE IN THE ERASE SECTOR SINGLE FUNCTION

    uint8_t i;
    uint8_t errors = 0;

    for(i = 0; i < num_blocks; i++)
    {
        if(ESP8266_FLASH_EraseSector(sector_number + i) == SPI_FLASH_RESULT_ERR)
        {
            errors = 1;
        }
    }

    if(_esp8266_flash_debug)
    {
        os_printf("ESP8266 : FLASH : Sector chain erase of length %u starting at sector #%u done. Error = %u\n", num_blocks, sector_number, errors);
    }

    if(errors)
    {
        return SPI_FLASH_RESULT_ERR;
    }
    return SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_WriteAddress(uint32_t flash_address, uint32_t* source_address, uint32_t size_bytes)
{
    //COPY SPECIFIED NUMBER OF BYTES FROM THE SOURCE ADDRESS INTO THE FLASH ADDRESS
    //NOTE FLASH NEEDS TO BE ERASED BEFORE IT CAN BE WRITTEN INTO, AND ERASE HAPPENS ONLY
    //AT THE BLOCK LEVEL (4KB)
    //FLASH ADDRESS NEEDS TO BE WORD 4 BYTE ALIGNED

    //VERIFY FLASH ADDRESS
    if(!_esp8266_flash_check_address_validity(flash_address))
    {
        if(_esp8266_flash_debug)
        {
            os_printf("ESP8266 : FLASH : Flash write. Starting flash address %x invalid\n", flash_address);
        }
        return SPI_FLASH_RESULT_ERR;
    }

    os_printf("a\n");

    //VERIFY SOURCE ADDRESS
    if(!_esp8266_flash_check_address_word_alignment(source_address))
    {
        if(_esp8266_flash_debug)
        {
            os_printf("ESP8266 : FLASH : Flash write. Source address %x not word aligned\n", flash_address);
        }
        return SPI_FLASH_RESULT_ERR;
    }

    os_printf("b\n");

    uint16_t starting_sector = (flash_address / ESP8266_FLASH_SECTOR_SIZE_BYTES);
    uint16_t num_sectors = (size_bytes / ESP8266_FLASH_SECTOR_SIZE_BYTES) + ((size_bytes % ESP8266_FLASH_SECTOR_SIZE_BYTES) != 0);
    uint8_t err_2;
    uint8_t crc;

    uint32_t final_size_bytes = size_bytes + 1;
    uint32_t _32_bit_blocks_required = (final_size_bytes / 4) + ((final_size_bytes % 4) != 0);

    //COPY DATA INTO TEMPORARY BUFFER + CRC8 BYTE (1 WORD)
    uint32_t* buffer = (uint32_t*)os_zalloc(_32_bit_blocks_required);
    os_printf("c\n");
    os_memcpy((uint8_t*)buffer, (uint8_t*)source_address, size_bytes);
    os_printf("d\n");
    
    //ADD CRC8 TO THE LAST BYYE OF UINT32_T BLOCK CHAIN
    crc = _esp8266_flash_calculate_crc8((uint8_t*)buffer, size_bytes);
    ((uint8_t*)(buffer + _32_bit_blocks_required - 1))[3] = crc;

    //ERASE SECTORS
    if(ESP8266_FLASH_EraseBlock(starting_sector, num_sectors) == SPI_FLASH_RESULT_ERR)
    {
        os_free(buffer);
        return SPI_FLASH_RESULT_ERR;
    }
    os_printf("r\n");

    //WRITE DATA
    if(spi_flash_write(flash_address, buffer, _32_bit_blocks_required * 4) == SPI_FLASH_RESULT_ERR)
    {
        os_free(buffer);
        return SPI_FLASH_RESULT_ERR;
    }
    os_printf("f\n");
    if(_esp8266_flash_debug)
    {
        os_printf("ESP8266 : FLASH : Flash write of %u bytes into address %x done with CRC8 %02X\n", size_bytes, flash_address, crc);
    }
    os_free(buffer);
    return SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_ReadAddress(uint32_t flash_address, uint32_t* destination_address, uint32_t size_bytes)
{
    //READ THE SPECIFIED NUMBER OF BYTES FROM FLASH INTO THE DESTINATION ADDRESS

    //VERIFY FLASH ADDRESS
    if(!_esp8266_flash_check_address_validity(flash_address))
    {
        if(_esp8266_flash_debug)
        {
            os_printf("ESP8266 : FLASH : Flash read. Starting flash address %x invalid\n", flash_address);
        }
        return SPI_FLASH_RESULT_ERR;
    }

    //VERIFY DESTINATION ADDRESS
    if(!_esp8266_flash_check_address_word_alignment(destination_address))
    {
        if(_esp8266_flash_debug)
        {
            os_printf("ESP8266 : FLASH : Flash write. Destination address %x not word aligned\n", flash_address);
        }
        return SPI_FLASH_RESULT_ERR;
    }

    uint32_t final_size_bytes = size_bytes + 1;
    uint32_t _32_bit_blocks_required = (final_size_bytes / 4) + ((final_size_bytes % 4) != 0);
    uint8_t crc_read, crc_calc;

    //ALLOCATE READ BUFFER
    uint32_t* buffer = (uint32_t*)os_zalloc(_32_bit_blocks_required);
    
    //READ DATA + CRC8
    SpiFlashOpResult val = spi_flash_read(flash_address, buffer, _32_bit_blocks_required * 4);

    crc_calc = _esp8266_flash_calculate_crc8((uint8_t*)buffer, size_bytes);
    crc_read = ((uint8_t*)(buffer))[_32_bit_blocks_required * 4 - 1];

    if(crc_calc != crc_read)
    {
        if(_esp8266_flash_debug)
        {
            os_printf("ESP8266 : FLASH : Flash read of %u bytes from address %x failed. CRC8 mismatch %02x vs %02x\n", size_bytes, flash_address, crc_calc, crc_read);
        }
        os_free(buffer);
        return SPI_FLASH_RESULT_ERR;
    }

    if(_esp8266_flash_debug)
    {
        os_printf("ESP8266 : FLASH : Flash read of %u bytes from address %x done result = %u\n", size_bytes, flash_address, val);
    }
    
    os_memcpy((uint8_t*)destination_address, (uint8_t*)buffer, size_bytes);
    os_free(buffer);
    return SPI_FLASH_RESULT_OK;
}

static uint8_t ICACHE_FLASH_ATTR _esp8266_flash_check_address_validity(uint32_t flash_address)
{
    //ENSURE THAT THE SPECIFIED FLASH ADDRESS IS WITHIN THE VALID RANGE
    //AND DOES NOT LIE WITHIN PROTECTED / INVALID AREAS AS PER THE FLASH MAP ABOVE

    if(flash_address > ESP8266_FLASH_MAX_ADDRESS)
    {
        //OUT OF BOUND
        return 0;
    }

    if(flash_address >= ESP8266_FLASH_ESP_INIT_BIN_ADDRESS)
    {
        //IN PROTECTED AREA RESERVED FOR SYSTEM FILES
        return 0;
    }

    if(flash_address <= ESP8266_FLASH_ESP_IROM0TEX_BIN_ADDRESS)
    {
        //FLASH ADDRESS WITHIN THE FIRST 64KNB AREA WHERE FLASH.BIN
        //RESIDES. TECHNICALLY WE COULD USE IT DEPENDING ON SIDE OF FLASH.BIN
        //BUT WE WILL DISALLOW IT
        return 0;
    }

    //LAST CHECK THAT THE ADDRESS IS WORD ALIGNED (4 BYTE ALIGNED)
    if((flash_address & 0x03) != 0)
    {
    	//NOT WORD ALIGNED
    	return 0;
    }

    //FLASH ADDRESS VALID
    return 1;
}

static uint8_t ICACHE_FLASH_ATTR _esp8266_flash_check_address_word_alignment(uint32_t* address)
{
    //CHECK WORD (4 BYTE ALIGNMENT) OF SPECIFIED ADDRESS LOCATION
    
    uint32_t pointer_address = (uint32_t)address;
    if((pointer_address & 0x03) != 0)
    {
        return 0;
    }
    return 1;
}

static uint8_t ICACHE_FLASH_ATTR _esp8266_flash_calculate_crc8(uint8_t* data, uint16_t len)
{
	//CALCULATE CRC8 FOR A BLOCK OF DATA OF SPECIFIED length

	char crc = 0x00;
    char extract, sum;
		uint16_t i;
		uint8_t j;

		for(i = 0; i < len; i++)
    {
       extract = *data;
       for (j = 8; j > 0; j--)
       {
          sum = (crc ^ extract) & 0x01;
          crc >>= 1;
          if(sum)
             crc ^= 0x8C;
          extract >>= 1;
       }
       data++;
    }
    return crc;
}
