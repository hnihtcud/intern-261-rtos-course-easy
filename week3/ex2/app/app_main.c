/* Week 3 / Ex2 -- software timer debounce.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "debounce.h"

static void enable_button_interrupt(void *argument)
{
    (void)argument;

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);
    HAL_NVIC_ClearPendingIRQ(EXTI1_IRQn);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    vTaskDelete(NULL);
}

void app_main(void)
{
    GPIO_InitTypeDef button_gpio = {0};
    BaseType_t result;

    /* TODO: GPIO init for the button pin as EXTI, both edges
     *       (GPIO_MODE_IT_RISING_FALLING) -- you need every bounce edge, not
     *       just the first one. */

    HAL_NVIC_DisableIRQ(EXTI1_IRQn);
    __HAL_RCC_GPIOB_CLK_ENABLE();

    button_gpio.Pin = GPIO_PIN_1;
    button_gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    button_gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &button_gpio);

    HAL_NVIC_SetPriority(EXTI1_IRQn, 6, 0);

    debounce_init();

    result = xTaskCreate(enable_button_interrupt, "ButtonInit", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    configASSERT(result == pdPASS);

    vTaskStartScheduler();

    for (;;) {
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_1)
    {
        uint8_t pin_is_high = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET);
        debounce_on_edge(pin_is_high);
    }
}