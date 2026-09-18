# STM32G431CBUX Bootloader and Application Architecture

## Architecture assessment

This project follows a sound embedded pattern for a small bootloader-based firmware system: a minimal first-stage bootloader owns reset-time validation and control transfer, while the application project contains the user logic and the runtime services it needs.

In other words, the architecture is good for a prototype, educational project, or a constrained production update flow with a single application slot. It is not a hardened production-safe bootloader design yet because it relies on static addresses, a single image slot, and a manual post-build CRC patching workflow.

## System structure

The workspace contains three meaningful layers:

- `Custom_BootLoader`: first-stage reset handler, flash validation, OTA request handling, and jump into the app.
- `Application_Jump`: runtime firmware built as an independent STM32 project and placed at a known flash offset.
- `Common`: shared constants and data definitions that define the contract between the bootloader and the application.

This split keeps responsibilities clean and easy to reason about:

- the bootloader validates the application before execution,
- the application owns task logic and runtime behaviour,
- the shared header and flash layout guarantee a consistent memory contract.

## Actual flash layout

The implementation uses the following layout defined by the common header and linker scripts:

```text
0x08000000 ┌───────────────────────────────┐
           │ Bootloader code + vectors     │  30 KB
           │ Custom_BootLoader             │
0x08007800 ├───────────────────────────────┤
           │ Reserved metadata area        │   2 KB
           │ .metadata / boot metadata     │
0x08008000 ├───────────────────────────────┤
           │ Application header            │   2 KB
           │ version, size, magic, crc     │
0x08008800 ├───────────────────────────────┤
           │ Application image             │  ~96 KB
           │ vector table + .text + .data  │
0x08020000 └───────────────────────────────┘
```

| Region | Start | End | Size | Purpose |
|--------|-------|-----|------|---------|
| Bootloader | 0x08000000 | 0x080077FF | 30 KB | Reset logic, validation, jump |
| Boot metadata | 0x08007800 | 0x08007FFF | 2 KB | Reserved for shared boot metadata |
| App header | 0x08008000 | 0x080087FF | 2 KB | Header with version, size, magic, CRC, OTA flag |
| Application | 0x08008800 | 0x0801FFFF | ~96 KB | Application firmware image |
| SRAM | 0x20000000 | 0x20007FFF | 32 KB | Runtime data, stack, heap |

## Boot flow

The real boot sequence is:

1. MCU power-up or reset loads the bootloader vector table from 0x08000000.
2. `Custom_BootLoader/main.c` runs and calls validation helpers.
3. `Boot_IsAddressValid()` verifies that the application MSP is in SRAM and that the reset handler is within the application flash range.
4. `Boot_IsMagicValid()` checks the application header magic against `0x50505050`.
5. `Boot_IsCrcValid()` verifies the header size is valid and computes the CRC over the application flash image.
6. If the OTA flag is set in the header, the bootloader handles the pending update request.
7. `Boot_JumpToApplication()` disables interrupts, resets system state, sets `SCB->VTOR`, loads the app MSP, and calls the app reset handler.

This is a clean and understandable boot procedure for a single-stage application update flow.

## Why the architecture is good

- Clear separation of concerns between reset-time logic and runtime logic.
- The bootloader is small and deterministic, which is ideal for embedded systems.
- The application header creates a simple verification contract between images.
- The shared API table lets a bootloader keep a small fixed function set available after the jump.
- The CRC check blocks invalid firmware from being executed.

## What is weak or missing for production use

The current implementation is good as a proof-of-concept, but several trade-offs remain:

- There is only one application slot; there is no rollback or banked firmware strategy.
- The header size and CRC are patched manually, which increases the chance of human error.
- The OTA flow is simple but not fully transactional; a power loss during a flash write could leave the header in an uncertain state.
- The shared API is fixed and not versioned, so compatibility must be managed carefully.
- There is no cryptographic signature or anti-rollback scheme.

For production firmware, this would typically evolve toward a dual-bank or secure update model with signed images, version negotiation, and safer rollback logic.

## Project-specific documentation

- [Custom_BootLoader/README.MD](Custom_BootLoader/README.MD)
- [Application_Jump/README.MD](Application_Jump/README.MD)

## Build and deployment flow

1. Build the application project.
2. Generate the firmware binary.
3. Patch the header values with the actual image size and CRC.
4. Program the bootloader image.
5. Program the application image at the application region.
6. Reset the board and allow the bootloader to validate and jump.

This design is easy to maintain in STM32CubeIDE and easy to debug interactively.

---

This repository is best viewed as a compact bootloader system for learning and field-level prototype firmware updates, with a clean architecture that is close to production but still intentionally minimal in its update safety features.
