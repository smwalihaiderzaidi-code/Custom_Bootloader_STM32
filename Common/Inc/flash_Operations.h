#ifndef FLASH_OPERATIONS_H
#define FLASH_OPERATIONS_H

#include <stdint.h>

void flash_read_page(uint32_t *page_start_addr, uint32_t *buffer);
void flash_erase_page(uint32_t page_address);
void flash_erase_pages(uint32_t start_address, uint32_t end_address);
void flash_erase_application_region(void);
void flash_erase_header_region(void);
void flash_write_page(uint32_t *page_start_addr, uint32_t *buffer);
void flash_write_buffer(uint32_t destination, const uint8_t *source, uint32_t length);

#endif
