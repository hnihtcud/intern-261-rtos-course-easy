/* Week 3 / Ex1 -- EXTI button wakes a task via task notification.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "exti_button.h"

static TaskHandle_t s_ack_task_handle;

void exti_button_init(void)
{
    /* TODO 1: GPIO init for PB<EXTI_PIN> as EXTI falling edge (GPIO_MODE_IT_FALLING,
     *         pull-up), and PA<ACK_PIN> as push-pull output.
     
     * TODO 2 (do this FIRST, on purpose): HAL_NVIC_SetPriority(EXTIx_IRQn, <n>, 0)
     *         with <n> deliberately SMALLER (= higher hardware priority) than
     *         configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY. Run step 4 of the
     *         README with this wrong value first and capture the configASSERT.
     *         Only after that, set it to ISR_NVIC_PRIO and HAL_NVIC_EnableIRQ. */
    HAL_NVIC_DisableIRQ(EXTI1_IRQn);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 7, 0);
}

void exti_button_isr_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* TODO 3: vTaskNotifyGiveFromISR(s_ack_task_handle, &xHigherPriorityTaskWoken).
     *
     * TODO 4: portYIELD_FROM_ISR(xHigherPriorityTaskWoken) -- the README asks
     *         you to temporarily remove exactly this line in step 6 to measure
     *         the latency penalty. Do NOT remove it in the version you submit. */
    vTaskNotifyGiveFromISR(s_ack_task_handle, &xHigherPriorityTaskWoken);
    //portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    (void)xHigherPriorityTaskWoken;
}

void ack_task(void *argument)
{
    (void)argument;
    s_ack_task_handle = xTaskGetCurrentTaskHandle();
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);
    HAL_NVIC_ClearPendingIRQ(EXTI1_IRQn);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    for (;;) {
        /* TODO 5: ulTaskNotifyTake(pdTRUE, portMAX_DELAY). */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1U << ACK_PIN));
        /* TODO 6: toggle PA<ACK_PIN> here, nothing before it in this loop body --
         *         any extra work here inflates the latency you are measuring. */
    }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_1) {
        exti_button_isr_handler();
    }
}