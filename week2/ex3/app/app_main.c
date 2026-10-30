/* Week 2 / Ex3 — priority inversion.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "tasks_lmh.h"

TickType_t experiment_start_tick;
void app_main(void)
{
    resource_init();
    BaseType_t result;
    result = xTaskCreate(task_low, "Low", 256, NULL, 1, NULL);
    configASSERT(result == pdPASS);

    result = xTaskCreate(task_mid, "Mid", 256, NULL, 2, NULL);
    configASSERT(result == pdPASS);

    result = xTaskCreate(task_high, "High", 256, NULL, 3, NULL);
    configASSERT(result == pdPASS);

    experiment_start_tick = xTaskGetTickCount();

    /* TODO: xTaskCreate task_low, task_mid, task_high with three DISTINCT
     *       priorities, low < mid < high. Justify the exact numbers in
     *       REPORT.md — they must leave room for configMAX_PRIORITIES and not
     *       collide with any other task in the system. */

    vTaskStartScheduler();

    for (;;) {
    }
}
