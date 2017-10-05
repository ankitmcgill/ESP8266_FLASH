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

#ifndef _ESP8266_FLASH_H_
#define _ESP8266_FLASH_H_

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "spi_flash.h"
#include "mem.h"

#define ESP8266_FLASH_SECTOR_SIZE_BYTES         4096

#define ESP8266_FLASH_MAX_ADDRESS               0x400000
#define ESP8266_FLASH_ESP_BLANK_BIN_ADDRESS     0x3FE000
#define ESP8266_FLASH_ESP_INIT_BIN_ADDRESS      0x3FC000
#define ESP8266_FLASH_ESP_IROM0TEX_BIN_ADDRESS  0x010000

#define ESP8266_FLASH_ALIGN_WORD                __attribute__((aligned(4)))

//FUNCTION PROTOTYPES/////////////////////////////////////////////
//CONFIGURATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_FLASH_SetDebug(uint8_t debug);

//OPERATION FUNCTIONS
SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_EraseSector(uint16_t sector_number);
SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_EraseBlock(uint16_t sector_number, uint16_t num_blocks);
SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_WriteAddress(uint32_t flash_address, uint32_t* source_address, uint32_t size_bytes);
SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_ReadAddress(uint32_t flash_address, uint32_t* destination_address, uint32_t size_bytes);

//INTERNAL FUNCTIONS
static uint8_t ICACHE_FLASH_ATTR _esp8266_flash_check_address_validity(uint32_t flash_address);
static uint8_t ICACHE_FLASH_ATTR _esp8266_flash_check_address_word_alignment(uint32_t* address);
static uint8_t ICACHE_FLASH_ATTR _esp8266_flash_calculate_crc8(uint8_t* data, uint16_t len);
#endif
