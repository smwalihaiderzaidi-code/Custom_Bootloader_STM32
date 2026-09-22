#ifndef APP_OTA_H
#define APP_OTA_H

#include <stdbool.h>
#include <stdint.h>
#include "app_Header.h"

#define OTA_FLAG_SET 1U
#define OTA_FLAG_CLEAR 0U

#define OtaHeader_t AppHeader_t

void enable_ota_request(void);

bool ota_request_pending(void);
void ota_clear_request_flag(void);
void ota_set_request_flag(void);

bool ota_write_firmware_image(uint32_t destination, const uint8_t *image, uint32_t image_size);

#endif
