# Application Jump - STM32G431

## Overview

`Application_Jump` is the runtime firmware image that the bootloader validates and then jumps to. It is a FreeRTOS-based application that exposes a small metadata header and uses a bootloader-provided API to control the LED.

This is a sensible architecture for a firmware image because the application remains independent from the bootloader. The bootloader owns reset-time safety, and the application owns task scheduling and user functionality.

## Runtime responsibilities

The application does three main things:

1. exposes a fixed metadata header so that the bootloader can validate it,
2. reads a shared function pointer table exported by the bootloader,
3. runs FreeRTOS tasks to demonstrate the runtime behavior after a successful jump.

## Actual flash contract

The linked application layout is:

```text
0x08008000  Start of app header
0x08008800  Start of application image
0x08020000  Flash end
```

The implementation defines these main interfaces:

- `AppHeader_t` in `Common/Inc/app_Header.h`
- `BootApi_t` in `Common/Inc/app_Header.h`
- `metadata_t` in `Common/Inc/app_Header.h`

The application header stores:

- version
- size
- magic
- crc
- ota_update
- reserved

The bootloader reads this table before jumping.

## Shared API usage

The application resolves the bootloader API at startup:

```c
const metadata_t *const meta = (const metadata_t *)METADATA_ADDRESS;
gBootApi = meta->ptrToBootApi;
```

This allows the application to call the LED functions that remain in the bootloader memory region:

- `Blink()`
- `TurnOn()`
- `TurnOff()`

This is a simple cross-image function table pattern and works well for a small embedded system.

## FreeRTOS tasks

The application creates and starts three static FreeRTOS tasks:

- `defaultTask`: normal priority, toggles the LED in a simple periodic pattern,
- `Task100ms`: high priority, increments a timer counter,
- `Task500ms`: realtime priority, increments a second counter.

The tasks are created after the bootloader API is used for a startup LED test sequence.

## Startup sequence

During `main()`, the application performs the following:

1. calls `App_Init()` to fetch the shared boot API,
2. initializes the HAL and system clock,
3. blinks the LED several times via the bootloader API,
4. waits briefly,
5. creates the FreeRTOS tasks,
6. starts the kernel.

## Design strengths

- The application remains independent from the bootloader flash layout.
- The metadata header provides a clear validation contract.
- The runtime uses a shared API instead of duplicating LED logic in the application.
- The project is easy to debug because the app and bootloader are separated cleanly.

## Design limitations

The current implementation is intentionally minimal, and therefore:

- the application header location and API addresses are fixed,
- the image size and CRC must be updated in the header after build,
- there is no secure signature check or version negotiation,
- the application assumes the bootloader API remains compatible with the application image.

## Relevant files

- `Core/Src/main.c`
- `Core/Src/app_Header.c`
- `STM32G431CBUX_FLASH.ld`
- `../Common/Inc/app_Header.h`
- `../Common/Inc/flash_layout.h`
- `../Common/Src/app_ota.c`

Overall, this is a strong boot-to-application architecture for a small embedded system, with the expected limitations of a fixed flash layout and a prototype-grade update process.