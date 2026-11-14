/* Week 3 / Ex2 -- debounce via one-shot timer + long-press via auto-reload
 * timer. Native FreeRTOS API only.
 */
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "main.h"
#include "params.h"
#include "debounce.h"
extern UART_HandleTypeDef huart1;
static TimerHandle_t s_debounce_timer;
static TimerHandle_t s_longpress_timer;
static uint8_t s_stable_pin_is_high;

static void debounce_callback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* TODO 1: printf/HAL_UART "PRESS" + xTaskGetTickCount(). This is the
     *         stable-press event, DEBOUNCE_MS after the last edge.
     *
     * TODO 2 (temporary, for the README step-4 experiment ONLY): add
     *         vTaskDelay(pdMS_TO_TICKS(50)) here, capture what happens to
     *         longpress_timer's callback timing in your REPORT.md, then
     *         DELETE this line again before you submit. A callback that
     *         blocks here is blocking the ONE daemon task every other
     *         software timer in the system also depends on. */

    uint8_t pin_is_high = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, pin_is_high ? GPIO_PIN_SET : GPIO_PIN_RESET);

    if (pin_is_high && !s_stable_pin_is_high)
    {
        char message[32];
        int length = snprintf(message, sizeof(message), "PRESS %lu\r\n",
                      (unsigned long)xTaskGetTickCount());
        if (length > 0 && (size_t)length < sizeof(message)){
            HAL_UART_Transmit(&huart1, (uint8_t *)message, (uint16_t)length, 100);
        }
    }
    
    s_stable_pin_is_high = pin_is_high;
}

static void longpress_callback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* TODO 3: printf/HAL_UART "HELD" + xTaskGetTickCount(). Fires once per
     *         LONGPRESS_MS while the button stays down. */

    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET)
    {
        char message[32];
        int length = snprintf(message, sizeof(message), "HELD %lu\r\n",
                      (unsigned long)xTaskGetTickCount());

        if (length > 0 && (size_t)length < sizeof(message)){
            HAL_UART_Transmit(&huart1, (uint8_t *)message, (uint16_t)length, 100);
        }
    }
}

void debounce_init(void)
{
    /* TODO 4: s_debounce_timer = xTimerCreate(..., pdMS_TO_TICKS(DEBOUNCE_MS),
     *         pdFALSE, NULL, debounce_callback); -- pdFALSE = one-shot.
     *         s_longpress_timer = xTimerCreate(..., pdMS_TO_TICKS(LONGPRESS_MS),
     *         pdTRUE, NULL, longpress_callback); -- pdTRUE = auto-reload.
     *         Check both handles non-NULL. Do NOT xTimerStart() either here --
     *         they are driven entirely from debounce_on_edge(). */

    configASSERT(pdMS_TO_TICKS(DEBOUNCE_MS) > 0);
    configASSERT(pdMS_TO_TICKS(LONGPRESS_MS) > 0);

    s_stable_pin_is_high = 0;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

    s_debounce_timer = xTimerCreate("debounce", pdMS_TO_TICKS(DEBOUNCE_MS), pdFALSE, NULL, debounce_callback);

    s_longpress_timer = xTimerCreate("longpress", pdMS_TO_TICKS(LONGPRESS_MS), pdTRUE, NULL, longpress_callback);

    configASSERT(s_debounce_timer != NULL);
    configASSERT(s_longpress_timer != NULL);
}

void debounce_on_edge(uint8_t pin_is_high)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result;

    /* TODO 5: xTimerResetFromISR(s_debounce_timer, &xHigherPriorityTaskWoken)
     *         on every edge -- this is what swallows bounce: each new edge
     *         restarts the DEBOUNCE_MS countdown, so the callback only fires
     *         once the line has been quiet for DEBOUNCE_MS. */

    result = xTimerResetFromISR(s_debounce_timer, &xHigherPriorityTaskWoken);
    configASSERT(result == pdPASS);

    /* TODO 6: if (pin_is_high) xTimerStartFromISR(s_longpress_timer, ...);
     *         else xTimerStopFromISR(s_longpress_timer, ...); */

    if (pin_is_high){
        result = xTimerStartFromISR(s_longpress_timer, &xHigherPriorityTaskWoken);
    }
    else{
        result = xTimerStopFromISR(s_longpress_timer, &xHigherPriorityTaskWoken);
    }

    configASSERT(result == pdPASS);

    /* TODO 7: portYIELD_FROM_ISR(xHigherPriorityTaskWoken). */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}