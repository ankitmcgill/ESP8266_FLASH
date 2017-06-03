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

#ifndef _ESP8266_FLASH_H_
#define _ESP8266_FLASH_H_

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "spi_flash.h"

#define ESP8266_FLASH_MAX_ADDRESS               0x400000
#define ESP8266_FLASH_ESP_BLANK_BIN_ADDRESS     0x3FE000
#define ESP8266_FLASH_ESP_INIT_BIN_ADDRESS      0x3FC000
#define ESP8266_FLASH_ESP_IROM0TEX_BIN_ADDRESS  0x10000

//FUNCTION PROTOTYPES/////////////////////////////////////////////
//CONFIGURATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_FLASH_SetDebug(uint8_t debug);

//OPERATION FUNCTIONS
SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_EraseSector(uint16_t sector_number);
SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_EraseBlock(uint16_t sector_number, uint16_t num_blocks);
SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_WriteAddress(uint32_t flash_address, uint32_t* source_address, uint32_t size);
SpiFlashOpResult ICACHE_FLASH_ATTR ESP8266_FLASH_ReadAddress(uint32_t flash_address, uint32_t* destination_address, uint32_t size);

//INTERNAL FUNCTIONS
static uint8_t ICACHE_FLASH_ATTR _esp8266_flash_check_address_validity(uint32_t flash_address);
#endif
