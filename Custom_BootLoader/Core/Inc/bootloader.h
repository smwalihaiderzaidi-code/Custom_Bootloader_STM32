#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stdint.h>
#include "flash_layout.h"

#define APP_START_ADDRESS    FLASH_APPLICATION_START
#define APP_END_ADDRESS      FLASH_APPLICATION_END

#define SRAM_START_ADDRESS   0x20000000U
#define SRAM_END_ADDRESS     0x20008000U


#define APP_HEADER_ADDRESS FLASH_APP_HEADER_START
#define APP_HEADER_SIZE FLASH_APP_HEADER_SIZE

#define APP_MAGIC_VALUE 0x50505050U

typedef enum
{
	BOOTLOADER_APP_VALID = 0,								//In C, if you don't explicitly assign values,
	BOOTLOADER_INVALID_MAGIC,								//The compiler assigns them automatically starting from 0 and increments by 1.
	BOOTLOADER_INVALID_STACK_POINTER,
	BOOTLOADER_INVALID_RESET_HANDLER,
	BOOTLOADER_INVALID_THUMB_BIT,
	BOOTLOADER_INVALID_APP_SIZE,
	BOOTLOADER_FOUND_PLACEHOLDER_CRC,
	BOOTLOADER_INVALID_APP_CRC,
	BOOTLOADER_ADDRESS_OVERFLOW
}BootloaderStatus_t;

extern BootloaderStatus_t blstatus;

bool Boot_IsAddressValid(void);
void Boot_JumpToApplication(void);
bool Boot_IsMagicValid(void);
bool Boot_IsCrcValid(void);
void Boot_IsUpdateAvailable(void);

#endif
