# Custom Bootloader - STM32G431

## Overview

`Custom_BootLoader` is the first-stage firmware that runs immediately after reset. Its job is to validate the application image, handle a simple OTA request flag, and then jump into the application if the image is valid.

This is a good bootloader architecture for a small MCU project because the validation logic stays isolated from the application runtime. The bootloader is intentionally minimal and deterministic.

## Responsibilities

The bootloader is responsible for:

- reading the application vector table at `APP_START_ADDRESS`,
- validating the application stack pointer and reset handler,
- checking the app header magic value,
- computing and comparing the CRC over the application image,
- handling a pending OTA request in the application header,
- transferring control into the application with a clean CPU state.

## Actual memory layout

The code uses the following addresses from the shared flash layout definitions:

```text
0x08000000  Bootloader image start
0x08007800  Reserved boot metadata area
0x08008000  Application header start
0x08008800  Application image start
0x08020000  Flash end
```

The important implementation constants are:

- `FLASH_APPLICATION_START = 0x08008800`
- `FLASH_APP_HEADER_START = 0x08008000`
- `APP_MAGIC_VALUE = 0x50505050`
- `SRAM_START_ADDRESS = 0x20000000`
- `SRAM_END_ADDRESS = 0x20008000`

## Validation sequence

`main.c` calls the following checks in sequence before jumping:

1. `Boot_IsUpdateAvailable()`
   - checks whether the OTA flag in the app header was requested.
2. `Boot_IsAddressValid()`
   - verifies the MSP is in SRAM,
   - verifies the reset handler is in application flash,
   - verifies the reset handler Thumb bit is set.
3. `Boot_IsMagicValid()`
   - ensures the application header magic equals `0x50505050`.
4. `Boot_IsCrcValid()`
   - validates the application size,
   - rejects placeholder CRC values,
   - calculates a CRC32 over the application region and compares it to the header.

If any check fails, the bootloader enters a permanent loop. This is intentionally strict and safe.

## Jump operation

`Boot_JumpToApplication()` performs the final transfer:

- disables interrupts,
- suspends SysTick,
- deinitializes HAL and RCC,
- clears NVIC interrupts,
- sets `SCB->VTOR = APP_START_ADDRESS`,
- loads the new MSP,
- calls the application reset handler.

This is the correct approach for a bare-metal jump from one image to another on STM32 devices.

## Shared API contract

The bootloader exports a shared API table using a fixed memory section (`.api_shared`) and function section (`.my_mem_text`). The application reads the function pointers via `metadata_t` and `BootApi_t`:

- `Blink()`
- `TurnOn()`
- `TurnOff()`

This provides a minimal runtime API while keeping the bootloader code in a protected flash region.

## Design strengths

- Very clear reset-time responsibilities.
- Small code footprint.
- Uses a real CRC-32 validation step instead of trusting an unverified image.
- Provides a simple and stable app-to-bootloader contract.

## Important limitations

This implementation is a strong prototype, but it is not a production-grade secure boot design:

- only one application image is considered valid at a time,
- the header is manually updated with the correct size and CRC,
- no signed image verification or anti-rollback protection is present,
- the memory layout is static and must stay consistent across the build.

## Relevant files

- `Core/Src/main.c`
- `Core/Src/bootloader.c`
- `Core/Src/app_Header.c`
- `Core/Inc/bootloader.h`
- `STM32G431CBUX_FLASH.ld`
- `../Common/Inc/flash_layout.h`
- `../Common/Inc/app_Header.h`

The bootloader design is good for small embedded systems and firmware update experiments, but it would need more safety measures for large-scale field deployment.