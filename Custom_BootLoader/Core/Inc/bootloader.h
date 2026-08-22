#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stdint.h>

#define APP_START_ADDRESS    0x08008100U
#define APP_END_ADDRESS      0x08020000U // change

#define SRAM_START_ADDRESS   0x20000000U
#define SRAM_END_ADDRESS     0x20008000U


#define APP_HEADER_ADDRESS 0x08008000UL
#define APP_HEADER_SIZE 0x100UL

#define APP_MAGIC_VALUE 0x50505050U


bool Boot_IsAddressValid(void);
void Boot_JumpToApplication(void);
bool Boot_IsMagicValid(void);
bool Boot_IsCrcValid(void);

#endif
