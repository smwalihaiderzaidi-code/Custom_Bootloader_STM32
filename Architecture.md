# Firmware Architecture Flow Diagram

This version is structured as a layered system architecture. It separates the boot validation path, flash contract, runtime application, and API interface into clear blocks so the control flow and data flow are easier to read.

## 1. System-Level Architecture

```mermaid
flowchart TB
    classDef boot fill:#E3F2FD,stroke:#1565C0,stroke-width:1.5px,color:#0D47A1;
    classDef meta fill:#E8F5E9,stroke:#2E7D32,stroke-width:1.5px,color:#1B5E20;
    classDef app fill:#FFF3E0,stroke:#EF6C00,stroke-width:1.5px,color:#E65100;
    classDef api fill:#F3E5F5,stroke:#8E24AA,stroke-width:1.5px,color:#4A148C;
    classDef hw fill:#FCE4EC,stroke:#C2185B,stroke-width:1.5px,color:#880E4F;

    subgraph BOOT["Bootloader Layer - Validation and Control"]
        B1["main()\nInput: MCU reset\nOutput: boot decision"]
        B2["Boot_IsUpdateAvailable()\nInput: OTA flag\nOutput: update state"]
        B3["Boot_IsAddressValid()\nInput: app vector, SRAM bounds\nOutput: valid/invalid"]
        B4["Boot_IsMagicValid()\nInput: app header magic\nOutput: valid/invalid"]
        B5["Boot_IsCrcValid()\nInput: app image + size/crc\nOutput: valid/invalid"]
        B6["Boot_JumpToApplication()\nInput: MSP + Reset_Handler\nOutput: app execution"]
    end

    subgraph META["Flash Metadata / Contract Layer"]
        M1["AppHeader_t\nversion, size, magic, crc, ota_update, reserved"]
        M2["flash_read_page()\nInput: flash address\nOutput: uint32_t buffer[]"]
        M3["flash_erase_page()\nInput: flash page\nOutput: erased page"]
        M4["flash_write_page()\nInput: buffer[]\nOutput: programmed flash"]
        M5["check_ota_flag()\nInput: FLASH_APP_HEADER_START\nOutput: OTA flag status"]
        M6["enable_ota_request()\nInput: request trigger\nOutput: OTA flag set + reset"]
    end

    subgraph APP["Application Layer - Runtime Execution"]
        A1["App_Init()\nInput: metadata_t\nOutput: gBootApi resolved"]
        A2["main()\nInput: API pointer\nOutput: LED test + scheduler start"]
        A3["StartDefaultTask()\nInput: none\nOutput: LED toggle"]
        A4["T_100msTask()\nInput: timer tick\nOutput: applicationCounter++"]
        A5["T_500msTask()\nInput: timer tick\nOutput: applicationStatus++"]
    end

    subgraph API["Shared API Layer"]
        P1["Blink()\nInput: none\nOutput: LED blink"]
        P2["TurnOn()\nInput: none\nOutput: LED ON"]
        P3["TurnOff()\nInput: none\nOutput: LED OFF"]
    end

    subgraph HW["Hardware / Memory Interface"]
        H1["Flash Memory\nAPP_HEADER + APP_IMAGE"]
        H2["SRAM\nMSP + runtime data"]
        H3["GPIO / LED\nUser output"]
    end

    B1 --> B2
    B2 --> B3
    B3 --> B4
    B4 --> B5
    B5 --> B6

    M1 --> B2
    M1 --> B3
    M1 --> B4
    M1 --> B5

    M2 --> M5
    M3 --> M6
    M4 --> M6
    M5 -->|OTA flag read/clear| B2
    M6 -->|writes OTA flag| M1

    B6 --> A1
    A1 --> A2
    A2 --> A3
    A2 --> A4
    A2 --> A5

    A1 -->|ptrToBootApi| P1
    A1 -->|ptrToBootApi| P2
    A1 -->|ptrToBootApi| P3
    A3 -->|calls| P2
    A3 -->|calls| P3

    H1 --> M1
    H1 --> M2
    H1 --> M4
    H1 --> B5
    H2 --> B3
    H2 --> B6
    H3 --> P1
    H3 --> P2
    H3 --> P3

    class B1,B2,B3,B4,B5,B6 boot;
    class M1,M2,M3,M4,M5,M6 meta;
    class A1,A2,A3,A4,A5 app;
    class P1,P2,P3 api;
    class H1,H2,H3 hw;
```

## 2. Functional Input/Output View

### Bootloader functions

- main()
  - Input: reset event, HAL init, system config
  - Output: validation sequence and either jump or lock-up state

- Boot_IsUpdateAvailable()
  - Input: OTA flag stored in app header
  - Output: status flag, OTA cleared if requested, or continue to validation

- Boot_IsAddressValid()
  - Input: application stack pointer and reset handler address
  - Output: true/false, plus bootloader status code

- Boot_IsMagicValid()
  - Input: application header magic value
  - Output: true/false, plus status code if invalid

- Boot_IsCrcValid()
  - Input: application payload, image size, and CRC from header
  - Output: true/false, plus status code on mismatch

- Boot_JumpToApplication()
  - Input: MSP, reset handler pointer, vector table address
  - Output: application reset handler entry

### Flash metadata and OTA helpers

- AppHeader_t
  - Input: version, size, magic, crc, ota_update, reserved
  - Output: validated metadata contract between images

- flash_read_page()
  - Input: page_start_addr, uint32_t buffer[]
  - Output: flash content copied to RAM buffer

- flash_erase_page()
  - Input: page address
  - Output: flash page erased before data update

- flash_write_page()
  - Input: page_start_addr, buffer[]
  - Output: new flash content written back

- check_ota_flag()
  - Input: pointer to header location
  - Output: OTA request status and cleared OTA bit

- enable_ota_request()
  - Input: OTA trigger from software
  - Output: OTA flag set in flash and pending reset

### Application functions

- App_Init()
  - Input: metadata_t pointer at METADATA_ADDRESS
  - Output: boot API pointer resolved to `gBootApi`

- main()
  - Input: boot API pointer and runtime init state
  - Output: LED startup pattern + RTOS task creation

- StartDefaultTask()
  - Input: task start context
  - Output: toggles LED on/off periodically

- T_100msTask()
  - Input: periodic timer event
  - Output: increments `applicationCounter`

- T_500msTask()
  - Input: periodic timer event
  - Output: increments `applicationStatus`

### Shared API functions

- Blink()
  - Input: none
  - Output: LED pattern through bootloader-owned hardware function

- TurnOn()
  - Input: none
  - Output: GPIO LED ON state

- TurnOff()
  - Input: none
  - Output: GPIO LED OFF state

## 3. Control-Flow Narrative

1. System powers on and enters the bootloader `main()`.
2. Bootloader checks the OTA flag in the metadata header.
3. It validates the application stack pointer, reset handler, magic, and CRC.
4. If all checks pass, it jumps to the application reset handler.
5. Application `App_Init()` resolves the shared API pointer from flash metadata.
6. Runtime tasks execute and call LED functions owned by the bootloader.
7. OTA requests are stored in the flash header and applied through the metadata update routines.

## 4. Architecture Interpretation

This is a classic split-image firmware architecture:

- Bootloader owns trust, validation, and reset control.
- Flash metadata owns the contract between images.
- Application owns runtime behavior and scheduling.
- Shared API provides controlled communication to bootloader-owned hardware features.

This structure is well suited for embedded firmware where image safety and controlled transfer of execution are critical.
