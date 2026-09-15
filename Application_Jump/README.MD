# Application Jump - STM32G431

## Overview
`Application_Jump` is the FreeRTOS application loaded by `Custom_BootLoader` at `0x08008000`. It uses the bootloader's shared LED API and runs the application logic in three statically allocated FreeRTOS threads.

## Flash Layout

- **Application vector table and code**: starts at `0x08008000`
- **Application header**: fixed at `0x08008200`
- **Shared bootloader API**: `0x08019000`
- **Application flash region**: 68 KB, from `0x08008000` through `0x08018FFF`
- **Shared API region**: 28 KB, from `0x08019000` through `0x0801FFFF`

The application linker script places the `.app_header` section at `0x08008200`. The header currently contains version `1`, magic value `0x50505050`, and CRC value `0x12345`.

## Bootloader API

The application reads a `BootApi_t` table from `0x08019000`:

- `Blink`
- `TurnOn`
- `TurnOff`

These functions are implemented by the bootloader in its `.my_mem_text` section and exposed through its `.api_shared` section. The application uses `TurnOn` and `TurnOff` for LED control on PB9.

## FreeRTOS Tasks

All task buffers and control blocks are statically allocated.

- `StartDefaultTask`: normal priority; turns the LED on and off with 300 ms delays and increments `default_counter`.
- `T_100msTask`: high priority; increments `counter_100ms` every 100 ms.
- `T_500msTask`: realtime priority; increments `counter_500ms` every 500 ms.

The tasks are created before `osKernelStart()` and use `osDelay()` for periodic scheduling.

## Startup Behavior

After HAL, clock, and GPIO initialization, `main()` calls the bootloader `TurnOn` and `TurnOff` API functions 25 times with 50 ms delays, waits 2 seconds, creates the FreeRTOS tasks, and starts the scheduler.

## Device Information

- **MCU**: STM32G431CBUX
- **Total Flash**: 128 KB
- **RAM**: 32 KB
- **Application project**: `Core/Src/main.c`
- **Linker script**: `STM32G431CBUX_FLASH.ld`