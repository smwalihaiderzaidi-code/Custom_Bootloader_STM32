#include "main.h"
#include "app_Header.h"
#include "app_ota.h"
#include "flash_Operations.h"

#define OTA_FLAG_OFFSET 4U
#define OTA_MAGIC_VALUE 0x50505050U

// NOTE: System should not go to reset in middle of this function execution,
// otherwise the OTA metadata may be erased and the device may never recover.

static void ota_read_header_buffer(OtaHeader_t *header)
{
    uint32_t flash_buffer[APP_HEADER_WORDS];
    uint32_t *word_ptr = (uint32_t *)header;

    flash_read_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);

    for (uint32_t i = 0U; i < APP_HEADER_WORDS; i++)
    {
        word_ptr[i] = flash_buffer[i];
    }
}

static void ota_write_header_buffer(const OtaHeader_t *header)
{
    uint32_t flash_buffer[APP_HEADER_WORDS];
    const uint32_t *word_ptr = (const uint32_t *)header;

    for (uint32_t i = 0U; i < APP_HEADER_WORDS; i++)
    {
        flash_buffer[i] = word_ptr[i];
    }

    flash_erase_page(FLASH_APP_HEADER_START);
    flash_write_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);
}

void enable_ota_request(void)
{
    uint32_t flash_buffer[APP_HEADER_WORDS];

    flash_read_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);
    flash_buffer[OTA_FLAG_OFFSET] = OTA_FLAG_SET;

    flash_erase_page(FLASH_APP_HEADER_START);
    flash_write_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);

    NVIC_SystemReset();
}

bool ota_request_pending(void)
{
    uint32_t flash_buffer[APP_HEADER_WORDS];

    flash_read_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);
    return (flash_buffer[OTA_FLAG_OFFSET] == OTA_FLAG_SET);
}

void ota_clear_request_flag(void)
{
    uint32_t flash_buffer[APP_HEADER_WORDS];

    flash_read_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);
    flash_buffer[OTA_FLAG_OFFSET] = OTA_FLAG_CLEAR;

    flash_erase_page(FLASH_APP_HEADER_START);
    flash_write_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);
}

void ota_set_request_flag(void)
{
    uint32_t flash_buffer[APP_HEADER_WORDS];

    flash_read_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);
    flash_buffer[OTA_FLAG_OFFSET] = OTA_FLAG_SET;

    flash_erase_page(FLASH_APP_HEADER_START);
    flash_write_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);
}

bool ota_write_firmware_image(uint32_t destination, const uint8_t *image, uint32_t image_size)
{
    if ((image == NULL) || (image_size == 0U))
    {
        return false;
    }

    flash_erase_pages(destination, destination + image_size);
    flash_write_buffer(destination, image, image_size);

    return true;
}