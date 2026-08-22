# Custom_BootLoader Project Configuration

## Overview

The Custom_BootLoader is a minimal first-stage bootloader that:

1. Executes immediately after device reset
2. Validates the application image (version, magic, CRC, addresses)
3. Configures the CPU vector table to use the application's interrupts
4. Transfers control to the application's reset handler
5. Provides persistent LED control functions to the application via a shared API

The bootloader occupies the first 32 KB of Flash (0x08000000–0x08007FFF).

---

## STM32CubeIDE Project Structure

```
Custom_BootLoader/
├── Core/
│   ├── Inc/
│   │   ├── bootloader.h           (Validation function declarations)
│   │   ├── main.h
│   │   └── stm32g4xx_hal_conf.h
│   ├── Src/
│   │   ├── main.c                 (Boot validation and jump logic)
│   │   ├── bootloader.c           (CRC and address validation)
│   │   └── system_stm32g4xx.c
│   └── Startup/
│       └── startup_stm32g431cbux.s
├── Drivers/
│   ├── CMSIS/
│   └── STM32G4xx_HAL_Driver/
├── STM32G431CBUX_FLASH.ld         (Linker script)
└── Custom_BootLoader.ioc          (CubeIDE configuration)
```

---

## Linker Script Configuration

### Memory Regions

File: `STM32G431CBUX_FLASH.ld`

```ld
MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 32K
  FLASH  (rx)     : ORIGIN = 0x08000000,   LENGTH = 30K
  MY_MEM (rx)     : ORIGIN = 0x08007800,   LENGTH = 2K
}
```

### Explanation

| Region | Start | Size | Purpose |
|--------|-------|------|---------|
| RAM | 0x20000000 | 32 KB | Stack and bootloader data |
| FLASH | 0x08000000 | 30 KB (0x7800) | Bootloader code |
| MY_MEM | 0x08007800 | 2 KB | Shared API and LED functions |

The bootloader's shared API starts at `0x08007800`, which is part of the bootloader Flash region but reserved for function pointers and implementations that remain accessible to the application.

### Critical Sections in Linker Script

#### 1. Vector Table Placement

```ld
.isr_vector :
{
    . = ALIGN(4);
    KEEP(*(.isr_vector))  /* Bootloader interrupt vectors */
    . = ALIGN(4);
} >FLASH
```

The bootloader's own vector table is placed at `0x08000000`.

#### 2. Shared API Placement

```ld
.api_shared :
{
    . = ALIGN(4);
    KEEP(*(.api_shared))  /* Shared function pointers */
    . = ALIGN(4);
} >MY_MEM
```

The shared API structure (containing function pointers to `Led_On`, `Led_Off`, `Led_Toggle`) is placed at `0x08007800`.

#### 3. Implementation Functions

```ld
.my_mem_text :
{
    . = ALIGN(4);
    KEEP(*(.my_mem_text))  /* LED implementation functions */
    . = ALIGN(4);
} >MY_MEM
```

The actual LED control functions are placed immediately after the API table, still within the MY_MEM region.

---

## Bootloader Header Files

### bootloader.h

File: `Core/Inc/bootloader.h`

```c
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stdint.h>

/* Application location in Flash */
#define APP_START_ADDRESS    0x08008100U
#define APP_END_ADDRESS      0x08020000U

/* Application metadata */
#define APP_MAGIC_ADDRESS    0x08008000U
#define APP_MAGIC_VALUE      0x50505050U
#define CALCULATED_CRC_VALUE 0x12345U    /* Currently fixed; should be dynamic */

/* SRAM bounds for validation */
#define SRAM_START_ADDRESS   0x20000000U
#define SRAM_END_ADDRESS     0x20008000U

/* Function declarations */
bool Boot_IsAddressValid(void);
void Boot_JumpToApplication(void);
bool Boot_IsMagicValid(void);
bool Boot_IsCrcValid(void);

#endif
```

### Explanation of Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| APP_START_ADDRESS | 0x08008100 | Application vector table start (not the header) |
| APP_END_ADDRESS | 0x08020000 | Exclusive end of application Flash |
| APP_MAGIC_ADDRESS | 0x08008000 | Address of application header (256 bytes before code) |
| APP_MAGIC_VALUE | 0x50505050 | Expected magic number in header |
| SRAM_START_ADDRESS | 0x20000000 | SRAM lower bound |
| SRAM_END_ADDRESS | 0x20008000 | SRAM upper bound |

---

## Bootloader Main Logic

### main.c

File: `Core/Src/main.c` (relevant sections)

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* Validate application before jumping */
    bool addrValid = Boot_IsAddressValid();
    bool magicValid = Boot_IsMagicValid();
    bool crcValid = Boot_IsCrcValid();

    /* If any check fails, halt in infinite loop */
    if (!addrValid || !magicValid || !crcValid)
    {
        while (1);  /* Bootloader hangs; MCU is unresponsive */
    }

    /* All validations passed; transfer control to application */
    Boot_JumpToApplication();

    /* Should never reach here */
    while (1);
}
```

### Flow Diagram

```
Device Reset
    ↓
Bootloader Reset_Handler (0x08000000)
    ↓
main()
    ├─→ HAL_Init()
    ├─→ SystemClock_Config()
    ├─→ MX_GPIO_Init()
    │
    ├─→ Boot_IsAddressValid()
    │   ├─→ Check MSP in SRAM range
    │   ├─→ Check Reset_Handler in app Flash
    │   └─→ Check Thumb bit set
    │
    ├─→ Boot_IsMagicValid()
    │   └─→ Compare header magic vs 0x50505050
    │
    ├─→ Boot_IsCrcValid()
    │   └─→ Compare header CRC vs CALCULATED_CRC_VALUE
    │
    ├─→ if (all checks pass)
    │   └─→ Boot_JumpToApplication()
    │       ├─→ Disable interrupts
    │       ├─→ Deinitialize HAL/RCC
    │       ├─→ Clear all NVIC interrupts
    │       ├─→ Set SCB->VTOR = 0x08008100
    │       ├─→ Load application MSP
    │       └─→ Call application Reset_Handler
    │
    └─→ else
        └─→ while (1);  /* Infinite loop; validation failed */
```

---

## Validation Functions

### bootloader.c

File: `Core/Src/bootloader.c`

#### Address Validation

```c
bool Boot_IsAddressValid(void)
{
    uint32_t appMsp;
    uint32_t appResetHandler;

    /* Read initial MSP from application vector table */
    appMsp = *(volatile uint32_t *)APP_START_ADDRESS;

    /* Read reset handler from application vector table */
    appResetHandler = *(volatile uint32_t *)(APP_START_ADDRESS + 4U);

    /* Validate MSP is inside SRAM */
    if ((appMsp < SRAM_START_ADDRESS) || (appMsp > SRAM_END_ADDRESS))
    {
        return false;
    }

    /* Validate reset handler is inside application Flash */
    if ((appResetHandler < APP_START_ADDRESS) || 
        (appResetHandler >= APP_END_ADDRESS))
    {
        return false;
    }

    /* Cortex-M4 function pointers must have Thumb bit set (bit 0 = 1) */
    if ((appResetHandler & 0x1U) == 0U)
    {
        return false;
    }

    return true;
}
```

**What it validates:**
1. Initial MSP at `0x08008100` points into SRAM (0x20000000–0x20008000)
2. Reset handler at `0x08008104` points into application Flash (0x08008100–0x08020000)
3. Reset handler address has Thumb bit set (LSB = 1)

#### Magic Number Validation

```c
bool Boot_IsMagicValid(void)
{
    const AppHeader_t *appHeader = 
        (const AppHeader_t *)APP_MAGIC_ADDRESS;
    
    return (appHeader->magic == APP_MAGIC_VALUE);
}
```

**What it validates:**
- Header magic at `0x08008008` equals `0x50505050`

#### CRC Validation

```c
bool Boot_IsCrcValid(void)
{
    const AppHeader_t *appHeader = 
        (const AppHeader_t *)APP_MAGIC_ADDRESS;
    
    return (appHeader->crc == CALCULATED_CRC_VALUE);
}
```

**Current Status:**
- Compares header CRC at `0x0800800C` against a fixed constant
- The constant should be updated post-build to contain the actual calculated CRC

#### Application Jump

```c
__attribute__((noreturn))
void Boot_JumpToApplication(void)
{
    uint32_t appMsp;
    uint32_t appResetHandler;
    ApplicationEntry_t applicationEntry;

    /* Read application stack and reset handler */
    appMsp = *(volatile uint32_t *)APP_START_ADDRESS;
    appResetHandler = *(volatile uint32_t *)(APP_START_ADDRESS + 4U);

    /* Create function pointer to application reset handler */
    applicationEntry = (ApplicationEntry_t)appResetHandler;

    /* Disable interrupts before modifying CPU state */
    __disable_irq();

    /* Stop SysTick */
    HAL_SuspendTick();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    /* Deinitialize HAL and RCC */
    HAL_RCC_DeInit();
    HAL_DeInit();

    /* Disable and clear all NVIC interrupts */
    for (uint32_t index = 0U; index < 8U; index++)
    {
        NVIC->ICER[index] = 0xFFFFFFFFU;
        NVIC->ICPR[index] = 0xFFFFFFFFU;
    }

    /* Configure CPU to use application's vector table */
    SCB->VTOR = APP_START_ADDRESS;  /* = 0x08008100 */

    /* Memory barriers to ensure CPU state is consistent */
    __DSB();
    __ISB();

    /* Clear CONTROL register for privileged execution in thread mode */
    __set_CONTROL(0U);

    /* Load application's initial stack pointer */
    __set_MSP(appMsp);

    /* Memory barriers */
    __DSB();
    __ISB();

    /* Re-enable interrupts for application */
    __enable_irq();

    /* Call application reset handler (does not return) */
    applicationEntry();

    /* Should never reach here; if we do, halt */
    while (1);
}
```

**Sequence:**
1. Disable interrupts (CPU cannot be interrupted during state change)
2. Deinitialize HAL to leave clean state
3. Stop SysTick timer
4. Disable and clear all NVIC interrupt pending bits
5. Set SCB->VTOR to `0x08008100` (application vector table start)
6. Load application MSP from `0x08008100`
7. Call application reset handler as direct function pointer
8. Control transfers to application; bootloader code is no longer executed

---

## Shared API Configuration

### LED API Header

The bootloader provides persistent LED functions to the application via a shared API table.

```c
typedef struct
{
    void (*Blink)(void);
    void (*TurnOn)(void);
    void (*TurnOff)(void);
} BootApi_t;
```

### LED Implementation (bootloader)

File: `Core/Src/main.c` (bootloader)

```c
#define MOVE_FUNC __attribute__((section(".my_mem_text")))
#define SHARED_API __attribute__((section(".api_shared")))

/* LED functions placed in .my_mem_text section (0x08007800+) */
void MOVE_FUNC Led_Toggle(void)
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
}

void MOVE_FUNC Led_On(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
}

void MOVE_FUNC Led_Off(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
}

/* API table placed in .api_shared section (0x08007800) */
SHARED_API const BootApi_t BootApi =
{
    .Blink = Led_Toggle,
    .TurnOn = Led_On,
    .TurnOff = Led_Off
};
```

### LED API Usage (application)

File: `Core/Src/main.c` (Application_Jump)

```c
#define BOOT_API_ADDRESS 0x08007800

typedef struct
{
    void (*Blink)(void);
    void (*TurnOn)(void);
    void (*TurnOff)(void);
} BootApi_t;

const BootApi_t *api = (const BootApi_t *)BOOT_API_ADDRESS;

void main(void)
{
    /* Application can call bootloader LED functions */
    api->TurnOn();   /* LED on */
    osDelay(500);
    api->TurnOff();  /* LED off */
    osDelay(500);
}
```

---

## Building Custom_BootLoader in STM32CubeIDE

### Step-by-Step Build Process

1. **Right-click Custom_BootLoader project**
   - Select "Build Project" or press Ctrl+B

2. **Verify build output**
   - Console should show: "Finished building target: Custom_BootLoader.elf"
   - No errors or warnings

3. **Verify linker output**
   - Open `Debug/Custom_BootLoader.map`
   - Search for `.isr_vector`: should start at `0x08000000`
   - Search for `.api_shared`: should start at `0x08007800`
   - Search for `BootApi`: should be at `0x08007800`

---

## Memory Verification in STM32CubeIDE

After building, verify the linker placement:

### Using the Map File

1. Build the project
2. Open `Debug/Custom_BootLoader.map`
3. Search for the following entries:

**Expected results:**

```
.isr_vector
 0x08000000                 0x1d8
 0x08000000                 0x1d8 ./Core/Src/startup_stm32g431cbux.s.o
 0x08000000                g_pfnVectors

.api_shared
 0x08007800                 0xc
 0x08007800                 0xc ./Core/Src/main.o
 0x08007800                BootApi

.my_mem_text
 0x0800780c                0x48
 0x0800780c                 0x8 ./Core/Src/main.o (Led_Toggle)
 0x08007824                 0x8 ./Core/Src/main.o (Led_On)
 0x0800783c                 0x8 ./Core/Src/main.o (Led_Off)
```

---

## Flash Programming Verification

After programming the bootloader to the device:

1. **Disconnect and reconnect power**
2. **Attach debugger**
3. **Open Memory Browser**: Window → Show View → Memory Browser
4. **Navigate to 0x08000000**
5. **Verify bootloader code is present** (should not be all 0xFF or 0x00)
6. **Navigate to 0x08007800**
7. **Verify API table is present**:
   - Expected: Three function pointers (typically 0x08007825, 0x0800783D, 0x08007815, etc.)
   - Should not be 0xFFFFFFFF

---

## Troubleshooting

### Bootloader Does Not Execute

**Symptom**: Device does not respond; debugger cannot halt CPU.

**Causes**:
1. Bootloader not programmed to Flash
2. Bootloader vector table corrupted
3. Flash read protection enabled

**Fix**:
1. Erase entire Flash: `Reset and Erase All`
2. Program bootloader ELF via STM32CubeIDE

### Bootloader Hangs in Infinite Loop

**Symptom**: Debugger stops at `while (1);` in main().

**Causes**:
1. Application validation failed
2. Application not programmed to Flash
3. Application header missing or invalid
4. Application addresses invalid

**Fix**:
1. Check `addrValid`, `magicValid`, `crcValid` in debugger
2. Inspect memory at 0x08008000 (should be valid header)
3. Inspect memory at 0x08008100 (should be valid MSP and reset handler)
4. Program patched Application_Jump.bin

---

## Summary

**Custom_BootLoader delivers:**
- First-stage validation of application image
- Transfer of control to application reset handler
- Persistent LED functions available to application
- CPU vector table reconfiguration for application interrupts

**Critical bootloader configuration:**
- Linker script: FLASH (0x08000000, 30 KB) + MY_MEM (0x08007800, 2 KB)
- Validation: Address, magic, CRC
- Jump: Direct reset handler call + VTOR configuration
- Shared API: BootApi structure at 0x08007800
