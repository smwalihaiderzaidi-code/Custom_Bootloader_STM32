#include "app_Header.h"

static const metadata_t *const meta =
    (const metadata_t *)METADATA_ADDRESS;

const BootApi_t *gBootApi;

void App_Init(void)
{
    gBootApi = meta->ptrToBootApi;
}


/**
	@brief  - This is a temp meta data, python script "patch_crc.py" will a generate metadata and patch is to same bin file.
	updated parameters:
					version
					size
					crc

*/
__attribute__((section(".app_header"), used))
const AppHeader_t appheader =
{
    .version = 0,
    .size = 0,
    .magic = 0x50505050U,
    .crc = 0,
    .ota_update = 0U,
    .reserved = 0xFFFFFFFFU
};
