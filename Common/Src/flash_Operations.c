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

void flash_erase_page(uint32_t page_address)
{
    uint32_t page_index = page_address;
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error = 0U;
    HAL_StatusTypeDef status;
    HAL_StatusTypeDef lockstatus;

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

    lockstatus = HAL_FLASH_Unlock();
    status = HAL_FLASHEx_Erase(&erase, &page_error);
    lockstatus = HAL_FLASH_Lock();

    (void)status;
    (void)page_error;
}

void flash_write_page(uint32_t *page_start_addr, uint32_t *buffer)
{
    volatile uint32_t *address = (volatile uint32_t *)page_start_addr;
    uint32_t *source = buffer;
    HAL_StatusTypeDef status = HAL_OK;
    HAL_StatusTypeDef lockstatus;
    lockstatus = HAL_FLASH_Unlock();

    for (uint32_t i = 0U; i < APP_HEADER_WORDS; i += 2U)
    {
        uint64_t data = ((uint64_t)source[i + 1U] << 32U) | ((uint64_t)source[i]);

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)(address + i), data);

        if (status != HAL_OK)
        {
            break;
        }
    }

    lockstatus = HAL_FLASH_Lock();
    (void)status;
}
