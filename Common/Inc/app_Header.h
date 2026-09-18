#ifndef APP_HEADER_H
#define APP_HEADER_H

#include <stdint.h>
#include "flash_layout.h"

#define METADATA_ADDRESS FLASH_METADATA_START

typedef struct
{
    uint32_t version;
    uint32_t size;
    uint32_t magic;
    uint32_t crc;
    uint32_t ota_update;
    uint32_t reserved;
} AppHeader_t;

#define APP_HEADER_WORDS (sizeof(AppHeader_t) / sizeof(uint32_t))

typedef struct
{
    void (*Blink)(void);
    void (*TurnOn)(void);
    void (*TurnOff)(void);
} BootApi_t;

typedef struct
{
    const BootApi_t *ptrToBootApi;
} metadata_t;

extern const BootApi_t *gBootApi;
void App_Init(void);

#endif
