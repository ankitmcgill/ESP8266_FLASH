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

SpiFlashOpResult ESP8266_FLASH_erase(uint16_t sector_address);
SpiFlashOpResult ESP8266_FLASH_write(uint16_t sector_address, uint32_t* source_address, uint32_t size);
SpiFlashOpResult ESP8266_FLASH_read(uint16_t sector_address, uint32_t* destination_address, uint32_t size);

#endif
