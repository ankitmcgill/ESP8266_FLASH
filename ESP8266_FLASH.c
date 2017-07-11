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
* FLASH IS A BLOCK DEVICE AND WORKS ON SECTORS (4KB)
* BEFORE WRITING TO AN ADDRESS YOU NEED TO ERASE IT. ERASING CAN ONLY BE DONE AT THE LEAST OF A 4 KB BLOCK
*
* NOTE : FOR FLASH OPERATIONS, ALL READ/WRITE OPERATIONS NEED TO BE 4 BYTE ALIGNED
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
    if(_esp8266_flash_check_address_validity(sector_number * 4096))
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
        os_printf("ESP8266 : FLASH : Sector chain erase of length %u starting at sector #%u done\n", num_blocks, sector_number);
    }

    if(errors)
    {
        return SPI_FLASH_RESULT_ERR;
    }
    return SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_WriteAddress(uint32_t flash_address, uint32_t* source_address, uint32_t size)
{
    //COPY SPECIFIED NUMBER OF BYTES FROM THE SOURCE ADDRESS INTO THE FLASH ADDRESS
    //NOTE FLASH NEEDS TO BE ERASED BEFORE IT CAN BE WRITTEN INTO, AND ERASE HAPPENS ONLY
    //AT THE BLOCK LEVEL (4KB)
    
    //VERIFY FLASH ADDRESS
    if(!_esp8266_flash_check_address_validity(flash_address))
    {
        if(_esp8266_flash_debug)
        {
            os_printf("ESP8266 : FLASH : Flash write. Starting flash address %x invalid\n", flash_address);
        }
        return SPI_FLASH_RESULT_ERR;
    }
    
    uint16_t starting_sector_num = (flash_address / 4096);
    uint16_t num_blocks = (size / 4096);
    uint8_t err_1, err_2;

    //ERASE BLOCKS
    err_1 = ESP8266_FLASH_EraseBlock(starting_sector_num, num_blocks);
    
    //WRITE NOW
    err_2 = spi_flash_write(flash_address, source_address, size);
    
    if(_esp8266_flash_debug)
    {
        os_printf("ESP8266 : FLASH : Flash write of %u bytes into address %x done\n", size, flash_address);
    }

    if((err_1 == SPI_FLASH_RESULT_ERR) || (err_2 == SPI_FLASH_RESULT_ERR))
    {
        return SPI_FLASH_RESULT_ERR;
    }
    return SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_ReadAddress(uint32_t flash_address, uint32_t* destination_address, uint32_t size)
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
    
    //READ
    SpiFlashOpResult val = spi_flash_read(flash_address, destination_address, size);

    if(_esp8266_flash_debug)
    {
        os_printf("ESP8266 : FLASH : Flash read of %u bytes from address %x done result = %u\n", size, flash_address, val);
    }
    
    return val;
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
    if((flash_address & 1) != 0)
    {
    	//NOT WORD ALIGNED
    	return 0;
    }

    //FLASH ADDRESS VALID
    return 1;
}
