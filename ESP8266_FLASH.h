/*************************************************
* ESP8266 FLASH LIBRARY
*
* NOVEMBER 2 2016
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
* ***********************************************/

#ifndef _ESP8266_FLASH_H_
#define _ESP8266_FLASH_H_

#include "user_interface.h"
#include <ets_sys.h>
#include <spi_flash.h>

//////////////////////////////////
//DEFINES
//////////////////////////////////
//FLASH SECTOR (4KB) TO STORE PROGRAM DATA
//ESP_INIT_DATA STARTS AT 0X3FC AS PER
//http://www.espressif.com/en/support/explore/get-started/esp8266/getting-started-guide#
/*http://www.esp8266.com/viewtopic.php?f=34&t=2662*/
//STORE PROGRAM INFO 1 SECTOR BEFORE THAT
//DO NOT CHANGE THIS ADDRESS !
//VALID FOR 4M FLASH CHIP WITH 1024+1024 MAP
#define FLASH_STORE_SECTOR  0x401

SpiFlashOpResult ESP8266_FLASH_erase(uint16_t sector_address);
SpiFlashOpResult ESP8266_FLASH_write(uint32_t flash_address, uint32_t* source_address, uint32_t size);
SpiFlashOpResult ESP8266_FLASH_read(uint32_t flash_address, uint32_t* destination_address, uint32_t size);

#endif
