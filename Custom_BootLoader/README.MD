# Custom Bootloader - STM32G431

## Overview
`Custom_BootLoader` starts at the STM32G431CBUX Flash base address and validates the application before transferring control to it. It also provides shared LED functions that remain available to the application after the jump.

## Flash Layout

- **Bootloader Flash**: `0x08000000`, 32 KB
- **Application Flash**: `0x08008000`, 68 KB
- **Application header**: `0x08008200`
- **Shared API region**: `0x08019000`, 28 KB
- **End of device Flash**: `0x08020000`

The bootloader linker script places the shared API table in `.api_shared` at `0x08019000` and the API implementation functions in `.my_mem_text` in the same shared memory region.

## Application Validation

Before jumping, `main()` performs these checks:

1. Reads the application initial stack pointer from `0x08008000` and confirms it is inside SRAM (`0x20000000` to `0x20008000`).
2. Reads the application reset handler from `0x08008004` and confirms it is inside the application Flash range and has the Thumb bit set.
3. Checks the application header magic at `0x08008200` against `0x50505050`.
4. Checks the application header CRC against the currently configured value `0x12345`.

If any check fails, the bootloader remains in an infinite loop. The CRC is currently a fixed comparison value, not a calculated image CRC.

## Application Jump

`Boot_JumpToApplication()`:

- Disables interrupts and stops the SysTick timer.
- Deinitializes the HAL and RCC.
- Disables and clears NVIC interrupts.
- Sets `SCB->VTOR` to `0x08008000`.
- Loads the application's initial MSP.
- Enables interrupts and calls the application reset handler.

## Shared LED API

The bootloader exports this function table at `0x08019000`:

- `Blink`
- `TurnOn`
- `TurnOff`

The functions control LED pin PB9. The application reads this table using the same fixed address and calls `TurnOn` and `TurnOff` from its FreeRTOS tasks.

## Device Information

- **MCU**: STM32G431CBUX
- **Total Flash**: 128 KB
- **RAM**: 32 KB
- **Bootloader source**: `Core/Src/main.c` and `Core/Src/bootloader.c`
- **Linker script**: `STM32G431CBUX_FLASH.ld`