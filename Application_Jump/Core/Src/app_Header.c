#include "app_Header.h"

static const metadata_t *const meta =
    (const metadata_t *)METADATA_ADDRESS;

const BootApi_t *gBootApi;

void App_Init(void)
{
    gBootApi = meta->ptrToBootApi;
}

__attribute__((section(".app_header"), used))
const AppHeader_t appheader =
{
    .version = 1U,
    .size = 0x11111111U,
    .magic = 0x50505050U,
    .crc = 0xAAAAAAAAU,
    .ota_update = 0U,
    .reserved = 0xFFFFFFFFU
};
