#include "main.h"
#include "app_Header.h"
#include "app_ota.h"
#include "flash_Operations.h"

#define OTA_FLAG_SET 1U

//NOTE : System should not go to reset in middle of this function execution, it will never recover since the meta data is erased!


void enable_ota_request(void)
{
    uint32_t flash_buffer[APP_HEADER_WORDS];

    flash_read_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);

    /* Update only first word of the header */
    flash_buffer[4] = OTA_FLAG_SET;

    flash_erase_page(FLASH_APP_HEADER_START);
    flash_write_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);

    NVIC_SystemReset();
}


/**
 * @brief Checks whether an OTA request is pending in the app header.
 *
 * Here we cant use pointers to the flash memory since the flash memory is erased in the middle of this * function execution, so we need to read the flash memory into a buffer and then check the OTA flag.
 * 
 * just checking one byte is still file but u cant modify it since its there in flash,  to modify it we * need to read the flash memory into a buffer and then modify the buffer and write it back to flash.
 */
uint32_t check_ota_flag(void)
{

    uint32_t flash_buffer[APP_HEADER_WORDS];

    flash_read_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);

    if (flash_buffer[4] == OTA_FLAG_SET)
    {
        /* Clear the OTA flag */
        flash_buffer[4] = 0U;

        flash_erase_page(FLASH_APP_HEADER_START);
        flash_write_page((uint32_t *)FLASH_APP_HEADER_START, flash_buffer);

        return OTA_FLAG_SET;
    }
    return 0;
}
