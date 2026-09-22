#include "main.h"
#include "flash_Operations.h"
#include "app_Header.h"

void flash_read_page(uint32_t *page_start_addr, uint32_t *buffer)
{
    volatile uint32_t *flash_ptr = (volatile uint32_t *)page_start_addr;

    for (uint32_t i = 0U; i < APP_HEADER_WORDS; i++)
    {
        buffer[i] = flash_ptr[i];
    }
}

void flash_write_buffer(uint32_t destination, const uint8_t *source, uint32_t length)
{
    uint32_t offset = 0U;

    HAL_FLASH_Unlock();

    while (offset < length)
    {
        uint32_t remaining = length - offset;
        uint64_t value = 0U;
        uint32_t write_count = (remaining >= 8U) ? 8U : remaining;

        for (uint32_t i = 0U; i < write_count; i++)
        {
            ((uint8_t *)&value)[i] = source[offset + i];
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination + offset, value) != HAL_OK)
        {
            break;
        }

        offset += write_count;
    }

    HAL_FLASH_Lock();
}

void flash_erase_page(uint32_t page_address)
{
    uint32_t page_index = page_address;
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error = 0U;
    HAL_StatusTypeDef status;


    if ((page_address >= FLASH_BASE) && (page_address < (FLASH_BASE + FLASH_SIZE)))
    {
        page_index = (page_address - FLASH_BASE) / FLASH_PAGE_SIZE;
    }

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = page_index;
    erase.NbPages = 1U;

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_PGSERR |
                           FLASH_FLAG_MISERR | FLASH_FLAG_FASTERR | FLASH_FLAG_RDERR |
                           FLASH_FLAG_OPTVERR);

    HAL_FLASH_Unlock();
    status = HAL_FLASHEx_Erase(&erase, &page_error);
    HAL_FLASH_Lock();

    (void)status;
    (void)page_error;
}

void flash_erase_pages(uint32_t start_address, uint32_t end_address)
{
    uint32_t current_page = start_address;
    uint32_t aligned_end = end_address;

    if ((start_address % FLASH_PAGE_SIZE) != 0U)
    {
        current_page = (start_address / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
    }

    if ((aligned_end % FLASH_PAGE_SIZE) != 0U)
    {
        aligned_end = ((aligned_end / FLASH_PAGE_SIZE) + 1U) * FLASH_PAGE_SIZE;
    }

    while (current_page < aligned_end)
    {
        flash_erase_page(current_page);
        current_page += FLASH_PAGE_SIZE;
    }
}

void flash_erase_application_region(void)
{
    flash_erase_pages(FLASH_APPLICATION_START, FLASH_APPLICATION_END);
}

void flash_erase_header_region(void)
{
    flash_erase_pages(FLASH_APP_HEADER_START, FLASH_APP_HEADER_END);
}

void flash_write_page(uint32_t *page_start_addr, uint32_t *buffer)
{
    volatile uint32_t *address = (volatile uint32_t *)page_start_addr;
    uint32_t *source = buffer;
    HAL_StatusTypeDef status = HAL_OK;

    HAL_FLASH_Unlock();

    for (uint32_t i = 0U; i < APP_HEADER_WORDS; i += 2U)
    {
        uint64_t data = ((uint64_t)source[i + 1U] << 32U) | ((uint64_t)source[i]);

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)(address + i), data);

        if (status != HAL_OK)
        {
            break;
        }
    }

    HAL_FLASH_Lock();
    (void)status;
}
