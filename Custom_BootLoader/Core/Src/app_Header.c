#include "main.h"
#include "app_Header.h"

#define MOVE_FUNC __attribute__((section(".my_mem_text")))
#define SHARED_API __attribute__((section(".api_shared")))

static void MOVE_FUNC Led_Toggle(void)
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
}

static void MOVE_FUNC Led_On(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
}

static void MOVE_FUNC Led_Off(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
}

SHARED_API const BootApi_t BootApi =
{
    .Blink = Led_Toggle,
    .TurnOn = Led_On,
    .TurnOff = Led_Off
};

__attribute__((section(".metadata")))
const metadata_t metadata =
{
    .ptrToBootApi = &BootApi
};