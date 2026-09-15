#include "bootloader.h"
#include "app_Header.h"
#include "stm32g4xx.h"
#include "stm32g4xx_hal.h"

typedef void (*ApplicationEntry_t)(void);

static uint32_t Boot_CalculateCRC32(const uint8_t *data,
                                    uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFU;

    for (uint32_t i = 0U; i < length; i++)
    {
        crc ^= data[i];

        for (uint32_t bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return crc ^ 0xFFFFFFFFU;
}


bool Boot_IsAddressValid(void)
{
    uint32_t appMsp;
    uint32_t appResetHandler;

    appMsp = *(volatile uint32_t *)APP_START_ADDRESS;						//Reads the application MSP from 0x08008000.
    appResetHandler = *(volatile uint32_t *)(APP_START_ADDRESS + 4U);		//Reads the application Reset Handler from 0x08008004.

    /* Check that application MSP points inside SRAM */						//Checks whether these addresses are valid.
    if ((appMsp < SRAM_START_ADDRESS) ||
        (appMsp > SRAM_END_ADDRESS))
    {
        return false;
    }

    /* Check that Reset_Handler points inside application Flash */			//Checks whether these addresses are valid.
    if ((appResetHandler < APP_START_ADDRESS) ||
        (appResetHandler >= APP_END_ADDRESS))
    {
        return false;
    }

    /* Cortex-M function pointer must have Thumb bit set */					//I DONT KNOW
    if ((appResetHandler & 0x1U) == 0U)
    {
        return false;
    }

    return true;
}


bool Boot_IsMagicValid(void)
{
	const AppHeader_t *appHeader = (const AppHeader_t *)APP_HEADER_ADDRESS;
	return (appHeader->magic == APP_MAGIC_VALUE);
}


bool Boot_IsCrcValid(void)
{
    const AppHeader_t *appHeader =
        (const AppHeader_t *)APP_HEADER_ADDRESS;

    uint32_t maxAppSize;
    uint32_t calculatedCrc;


    maxAppSize = APP_END_ADDRESS - APP_START_ADDRESS;

    /* Validate header */
    if (appHeader->magic != APP_MAGIC_VALUE)
    {
        return false;
    }

    /* Validate patched application size */
    if ((appHeader->size == 0U) ||
        (appHeader->size == 0xFFFFFFFFU) ||
        (appHeader->size > maxAppSize))
    {
        return false;
    }

    /* Prevent address overflow */
    if ((APP_START_ADDRESS + appHeader->size) > APP_END_ADDRESS)
    {
        return false;
    }

    /* Reject placeholder CRC values */
    if ((appHeader->crc == 0xFFFFFFFFU) ||
        (appHeader->crc == 0xAAAAAAAAU))
    {
        return false;
    }
    calculatedCrc = Boot_CalculateCRC32(
                               (const uint8_t *)APP_START_ADDRESS,
                               appHeader->size);


    return (calculatedCrc == appHeader->crc);
}

__attribute__((noreturn))
void Boot_JumpToApplication(void)
{
    uint32_t appMsp;
    uint32_t appResetHandler;
    ApplicationEntry_t applicationEntry;

    appMsp = *(volatile uint32_t *)APP_START_ADDRESS;						//Reads the application MSP from 0x08008100.
    appResetHandler = *(volatile uint32_t *)(APP_START_ADDRESS + 4U);		//Reads the application Reset Handler from 0x08001004.


    applicationEntry = (ApplicationEntry_t)appResetHandler;					//point this function pointer to App's Reset handler

    __disable_irq();												//disable interrupts

    HAL_SuspendTick();												//Stop SysTick
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    HAL_RCC_DeInit();
    HAL_DeInit();

    /* Disable and clear all NVIC interrupts */
    for (uint32_t index = 0U; index < 8U; index++)
    {
        NVIC->ICER[index] = 0xFFFFFFFFU;
        NVIC->ICPR[index] = 0xFFFFFFFFU;
    }

    SCB->VTOR = APP_START_ADDRESS;									//Sets SCB->VTOR to 0x08008100.

    __DSB();
    __ISB();

    __set_CONTROL(0U);
    __set_MSP(appMsp);

    __DSB();
    __ISB();

    __enable_irq();

    applicationEntry();

    while (1)
    {
    }
}
