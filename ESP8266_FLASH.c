/*************************************************
* ESP8266 FLASH LIBRARY
*
* NOVEMBER 2 2016
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
* ***********************************************/

#include "ESP8266_FLASH.h"

SpiFlashOpResult ESP8266_FLASH_erase(uint16_t sector_address)
{
	return spi_flash_erase_sector(sector_address);
}

SpiFlashOpResult ESP8266_FLASH_write(uint32_t flash_address, uint32_t* source_address, uint32_t size)
{
	return spi_flash_write(flash_address, source_address, size);
}

SpiFlashOpResult ESP8266_FLASH_read(uint32_t flash_address, uint32_t* destination_address, uint32_t size)
{
	return spi_flash_read(flash_address, destination_address, size);
}
