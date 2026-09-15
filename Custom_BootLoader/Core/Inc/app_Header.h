#ifndef APP_HEADER_H
#define APP_HEADER_H

#include <stdint.h>

typedef struct
{
    uint32_t version;
    uint32_t size;
    uint32_t magic;
    uint32_t crc;
} AppHeader_t;

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

#endif