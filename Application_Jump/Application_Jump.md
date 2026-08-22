# Application_Jump Project Configuration

## Overview

Application_Jump is an STM32CubeIDE project that implements a FreeRTOS-based application with:

1. Metadata header at `0x08008000` (version, size, magic, CRC)
2. Vector table at `0x08008100`
3. Application code and FreeRTOS tasks
4. Shared API calls to bootloader LED functions

The application is linked starting at `0x08008000` and occupies up to  96KB of Flash.

---

## STM32CubeIDE Project Structure

```
Application_Jump/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── stm32g4xx_hal_conf.h
│   │   └── FreeRTOSConfig.h
│   ├── Src/
│   │   ├── main.c                (Application entry, FreeRTOS tasks)
│   │   ├── app_freertos.c        (FreeRTOS configuration)
│   │   ├── system_stm32g4xx.c    (SystemInit, VECT_TAB_OFFSET)
│   │   └── stm32g4xx_hal_msp.c
│   └── Startup/
│       └── startup_stm32g431cbux.s
├── Middlewares/
│   └── Third_Party/
│       └── FreeRTOS/
├── Drivers/
│   ├── CMSIS/
│   └── STM32G4xx_HAL_Driver/
├── STM32G431CBUX_FLASH.ld        (Linker script)
├── patch_crc.py                  (Post-build CRC patching)
└── Application_Jump.ioc          (CubeIDE configuration)
```

---

## Application Linker Script Configuration

### Complete Linker Script

File: `STM32G431CBUX_FLASH.ld`

```ld
/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

/* Memories definition */
MEMORY
{
    RAM        (xrw) : ORIGIN = 0x20000000, LENGTH = 32K
    APP_HEADER (r)   : ORIGIN = 0x08008000, LENGTH = 0x100
    FLASH      (rx)  : ORIGIN = 0x08008100, LENGTH = 96K - 0x100
}

/* Sections */
SECTIONS
{
    /* Application header section */
    .app_header :
    {
        __app_header_start__ = .;
        . = ALIGN(4);
        KEEP(*(.app_header))
        . = ALIGN(4);
    } >APP_HEADER

    /* Vector table section */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
        __isr_vector_end__ = .;
    } >FLASH
}
    
```

### Memory Region Explanation

#### MEMORY Block

```ld
MEMORY
{
    RAM        (xrw) : ORIGIN = 0x20000000, LENGTH = 32K
    APP_HEADER (r)   : ORIGIN = 0x08008000, LENGTH = 0x100
    FLASH      (rx)  : ORIGIN = 0x08008100, LENGTH = 96K - 0x100
}
```

| Region | Start | Length | Size | Access | Purpose |
|--------|-------|--------|------|--------|---------|
| RAM | 0x20000000 | 32K | 32 KB | Read/Write/Execute | Stack, heap, FreeRTOS data |
| APP_HEADER | 0x08008000 | 0x100 | 256 B | Read-only | Metadata (version, size, magic, CRC) |
| FLASH | 0x08008100 | 0x17F00 | 98,048 B | Read/Execute | Vector table, code, constants |

#### Length Calculation for FLASH

```
Total application Flash available:  0x08020000 - 0x08008000 = 0x18000 bytes
Less header region:                 0x18000 - 0x100 = 0x17F00 bytes
                                    = 98,048 decimal bytes
                                    ≈ 96 KB - 0x100
```

---

## Application Header Configuration

### Header Definition

File: `Core/Src/main.c` (Application_Jump)

```c
typedef struct
{
    uint32_t version;
    uint32_t size;
    uint32_t magic;
    uint32_t crc;
} AppHeader_t;
```

### Memory Layout of Header

```
Offset  Field       Value           Size      Notes
──────────────────────────────────────────────────
0x00    version     0x00000001      4 bytes   Version number
0x04    size        0x11111111      4 bytes   Placeholder (patched post-build)
0x08    magic       0x50505050      4 bytes   Magic number (never changed)
0x0C    crc         0xAAAAAAAA      4 bytes   Placeholder (patched post-build)
```

**Total: 16 bytes** (but linker reserves 256-byte APP_HEADER region)

### Defining the Header in Code

```c
__attribute__((section(".app_header"), used))
const AppHeader_t appheader =
{
    .version = 1U,
    .size    = 0x11111111U,     /* Placeholder; patch_crc.py replaces this */
    .magic   = 0x50505050U,     /* Bootloader expects this value */
    .crc     = 0xAAAAAAAAU      /* Placeholder; patch_crc.py replaces this */
};
```

### Attributes Explained

| Attribute | Purpose |
|-----------|---------|
| `__attribute__((section(".app_header")))` | Place this variable in .app_header section (at 0x08008000) |
| `, used` | Prevent linker from removing unused variable |
| `const` | Store in Flash (not RAM), placed in .rodata or .app_header section |

---

## VECT_TAB_OFFSET Configuration (Critical)

### What is VECT_TAB_OFFSET?

`VECT_TAB_OFFSET` is a compile-time configuration that tells the CPU where to find interrupt handlers (SysTick, SVC, PendSV, etc.) after booting.

### Placed it an offset of **0x8100** 

### Why It Matters

- **Reset Handler** (`0x08008104`): Called directly by bootloader, not fetched through VTOR
- **Interrupt Handlers** (SysTick, SVC, etc.): Fetched from vector table pointed to by VTOR

If `VECT_TAB_OFFSET` is misconfigured:
- Reset handler executes correctly (direct call)
- Interrupts fail (wrong vector table)
- `HAL_Delay()` hangs (depends on SysTick)
- FreeRTOS hangs (depends on SysTick and SVC)

### Correct Value for Application_Jump

File: `Core/Src/system_stm32g4xx.c`

```c
#if !defined(VECT_TAB_OFFSET)
#define VECT_TAB_OFFSET         0x00008100U
#endif
```

**Why `0x00008100`?**
- Application vector table starts at `0x08008100`
- VECT_TAB_OFFSET is the offset from FLASH_BASE (0x08000000)
- Offset = 0x08008100 - 0x08000000 = 0x00008100 ✓

### How VECT_TAB_OFFSET is Used

In `SystemInit()`:

```c
void SystemInit(void)
{
    #if defined(USER_VECT_TAB_ADDRESS)
        SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET;
    #endif
}
```

Which expands to:

```c
SCB->VTOR = FLASH_BASE | 0x00008100;
SCB->VTOR = 0x08000000 | 0x00008100;
SCB->VTOR = 0x08008100;  /* Application vector table */
```

---

## Post-Build CRC Patching Configuration

### Overview

The bootloader validates the application using a CRC-32 checksum. However, the CRC cannot be calculated until the application binary is fully linked. Therefore:

1. **Compile and link** → Application_Jump.elf (contains placeholder CRC)
2. **Extract binary** → Application_Jump.bin (contains placeholder CRC)
3. **Calculate CRC** → Scan application code (skip header)
4. **Patch binary** → Replace placeholder with actual CRC
5. **Result** → Application_Jump.bin ready for deployment

### STM32CubeIDE Post-Build Configuration

#### Step 1: Disable Automatic BIN Generation

Right-click Application_Jump project:

```
Properties
  → C/C++ Build
    → Settings
      → Tool Settings
        → MCU Post build outputs
          ✗ Disable: "Convert to binary file"
```

This prevents CubeIDE from automatically regenerating the BIN after the Python script patches it.

#### Step 2: Add Combined Post-Build Command

Right-click Application_Jump project:

```
Properties
  → C/C++ Build
    → Settings
      → Build Steps
        → Post-build steps
```

In the post-build command field, enter:

```bash
arm-none-eabi-objcopy -O binary "${BuildArtifactFileName}" "${BuildArtifactFileBaseName}.bin" && python "${ProjDirPath}/patch_crc.py" "${BuildArtifactFileBaseName}.bin"

OR

python "${ProjDirPath}/patch_crc.py" "${BuildArtifactFileBaseName}.bin"


```

**Purpose**: Calculate CRC and patch the binary

- Python script path: `${ProjDirPath}/patch_crc.py`
- Target file: `${BuildArtifactFileBaseName}.bin`

#### Execution Order (Critical)

```
1. Compile C files → .o object files
2. Link → Application_Jump.elf (contains placeholder CRC)
3. objcopy → Application_Jump.bin (contains placeholder CRC)
4. patch_crc.py → Patch Application_Jump.bin (insert actual CRC)
5. Build complete
```

This order ensures the patched BIN is the final output.

---

## Post-Build Python Script Configuration

### Script Purpose

The `patch_crc.py` script:
1. Reads the generated binary
2. Calculates CRC-32 of application data (skipping header)
3. Calculates application image size
4. Patches the binary with actual values

### Script Location

File: `Application_Jump/patch_crc.py`

### CRC Calculation

The CRC-32 is calculated over the **application data only**, excluding the header:

```python
application_data = image[HEADER_SIZE:]  # Skip first 256 bytes
image_crc = zlib.crc32(application_data) & 0xFFFFFFFF
```

---

## Verifying the Patched Binary

### Method 1: Python Inspection Script

After building, run this command from `Application_Jump/Debug` directory:

```bash
python -c "import struct; d=open('Application_Jump.bin','rb').read(16); print([f'0x{x:08X}' for x in struct.unpack('<4I',d)])"
```

**Expected output:**

```
['0x00000001', '0x<actual_size>', '0x50505050', '0x<actual_crc>']
```

**Examples (not 0x11111111 or 0xAAAAAAAA):**

```
['0x00000001', '0x0000F8B0', '0x50505050', '0x12A4F5E2']
```

---

## ELF vs BIN: Critical Distinction

### Problem Statement

The application header contains compile-time placeholders:

```c
const AppHeader_t appheader =
{
    .version = 1,
    .size    = 0x11111111U,     /* PLACEHOLDER */
    .magic   = 0x50505050U,     /* Real value */
    .crc     = 0xAAAAAAAAU      /* PLACEHOLDER */
};
```

When compiled and linked:

- **Application_Jump.elf** contains: `size = 0x11111111, crc = 0xAAAAAAAA`
- **Application_Jump.bin** contains: Same initially

After the post-build Python script:

- **Application_Jump.elf** contains: Still `size = 0x11111111, crc = 0xAAAAAAAA` (ELF unchanged)
- **Application_Jump.bin** contains: `size = <actual>, crc = <actual>` (BIN patched)

### Data Flow Diagram

```
Application Source Code
    ↓
Compiler
    ↓
Object Files (.o)
    ├─ Contains placeholder: size=0x11111111, crc=0xAAAAAAAA
    │
Linker
    ↓
Application_Jump.elf
    ├─ Contains placeholder: size=0x11111111, crc=0xAAAAAAAA
    │
objcopy (Post-build Step 1)
    ├─ Input: Application_Jump.elf
    └─ Output: Application_Jump.bin
        └─ Contains placeholder: size=0x11111111, crc=0xAAAAAAAA
            │
patch_crc.py (Post-build Step 2)
    ├─ Input: Application_Jump.bin
    ├─ Calculate CRC of application data (0x100 bytes onward)
    ├─ Write actual size at offset 0x04
    └─ Write actual CRC at offset 0x0C
        │
Final Application_Jump.bin
    └─ Contains ACTUAL values: size=<real>, crc=<real>
```

---

## Combined STM32CubeIDE Debug Configuration

### Setting Up the Debug Configuration

#### Step 1: Navigate to Debug Configurations

1. Open: **Run** → **Debug Configurations...**
2. Create new configuration: **STM32 C/C++ Application**
3. Name it: **Custom_BootLoader**

#### Step 2: Configure Main Tab

| Setting | Value |
|---------|-------|
| Project | Custom_BootLoader |
| C/C++ Application | `Debug/Custom_BootLoader.elf` |
| Build configuration | Debug |

#### Step 3: Configure Startup Tab

Click the **Startup** tab and add three images:

##### Image 1: Bootloader ELF

1. Click **Browse** or **New** to add Custom_BootLoader.elf
2. Set the following for Custom_BootLoader.elf:

| Setting | Value |
|---------|-------|
| Path | `Debug/Custom_BootLoader.elf` |
| Build | Before launch (as configured in Main tab) |
| Download | ✓ Enabled |
| Load symbols | ✓ Enabled |

##### Image 2: Application ELF (Symbols Only)

1. Click **Add** to add Application_Jump.elf
2. Set the following for Application_Jump.elf:

| Setting | Value |
|---------|-------|
| Path | `../Application_Jump/Debug/Application_Jump.elf` |
| Build | ✓ Enabled (if required) |
| Download | ✗ **Disabled** |
| Load symbols | ✓ Enabled |

**Why disable download?** The ELF contains placeholder values. We download the patched BIN instead.

##### Image 3: Application BIN (Optional in CubeIDE)

If STM32CubeIDE supports adding raw binaries:

1. Click **Add** to add Application_Jump.bin
2. Set the following:

| Setting | Value |
|---------|-------|
| Path | `../Application_Jump/Application_Jump.bin` |
| Format | Binary |
| Load address | 0x08008000 |
| Download | ✓ Enabled |
| Load symbols | ✗ Disabled |

**Note:** If CubeIDE doesn't support BIN directly, use the GDB restore method (see next section).

#### Step 4: Apply Configuration

1. Click **Apply**
2. Click **Close**

### GDB Restore Method (Alternative)

If CubeIDE does not support adding the BIN in the Startup tab, use the GDB restore command:

#### Procedure

1. Start debugging with Custom_BootLoader configuration
2. Debugger halts at bootloader main()
3. **Window** → **Show View** → **GDB Console**
4. In GDB console, type:

```gdb
restore "C:/ST/workspace/GitHub/Application_Jump/Debug/Application_Jump.bin" binary 0x08008000
```

5. **GDB response:**

```
Restore completed successfully.
```

6. Resume bootloader execution:
   - Press **F8** or click **Resume**

---

## Memory Browser Verification

### Inspecting the Application Header

After loading the patched BIN:

1. **Open Memory Browser**
   - **Window** → **Show View** → **Memory Browser**

2. **Navigate to 0x08008000**
   - Enter `0x08008000` in the address field
   - Set display format to **Hex (32-bit)**

3. **Expected values**

| Address | Expected Value | Meaning |
|---------|----------------|---------|
| 0x08008000 | 0x00000001 | Version |
| 0x08008004 | 0x0000F8B0 (example) | Patched application size |
| 0x08008008 | 0x50505050 | Magic (always this value) |
| 0x0800800C | 0x12A4F5E2 (example) | Patched CRC-32 |

### Inspecting the Vector Table

1. Navigate to **0x08008100**
2. Set display format to **Hex (32-bit)**

3. **Expected values**

| Address | Typical Value | Meaning |
|---------|---------------|---------|
| 0x08008100 | 0x20007FE0 | Initial MSP (top of SRAM) |
| 0x08008104 | 0x0800xxx1 | Reset handler (with Thumb bit) |
| 0x08008108 | 0x0800xxx1 | NMI handler |
| 0x0800810C | 0x0800xxx1 | Hard fault handler |

**Note:** Values should be in application Flash range (0x08008000–0x08020000) with LSB = 1 (Thumb mode).

---

## Complete Build and Debug Workflow

### Pre-Debug Steps

1. **Build Application_Jump**
   ```
   Right-click Application_Jump → Build Project
   ```

2. **Verify patched BIN**
   ```bash
   python -c "import struct; d=open('Application_Jump/Debug/Application_Jump.bin','rb').read(16); print([f'0x{x:08X}' for x in struct.unpack('<4I',d)])"
   ```

3. **Build Custom_BootLoader**
   ```
   Right-click Custom_BootLoader → Build Project
   ```

### Debug Steps

1. **Start Debug Session**
   ```
   Run → Debug Configurations → Custom_BootLoader → Debug
   ```

2. **Halt at bootloader main()**

3. **Load Application BIN** (if not auto-loaded)
   - **GDB Console**: `restore "C:/ST/workspace/GitHub/Application_Jump/Debug/Application_Jump.bin" binary 0x08008000`

4. **Resume bootloader execution**
   - Press **F8** (Resume)

5. **Debugger hits application breakpoints**

---

## Summary

**Application_Jump configuration:**
- Linker: APP_HEADER (0x08008000, 256 B) + FLASH (0x08008100, 68 KB)
- Header: version, size, magic (0x50505050), CRC
- Build: objcopy ELF → BIN, then patch_crc.py → patch size and CRC
- Debug: Load Application_Jump.elf symbols, download patched BIN
- VTOR: 0x08008100 (set by SystemInit)

**Key rule:**
- ELF is used for source symbols
- Patched BIN is used for programming
