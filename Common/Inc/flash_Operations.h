#ifndef FLASH_OPERATIONS_H
#define FLASH_OPERATIONS_H

#include <stdint.h>

void flash_read_page(uint32_t *page_start_addr, uint32_t *buffer);
void flash_erase_page(uint32_t page_address);
void flash_write_page(uint32_t *page_start_addr, uint32_t *buffer);

#endif
