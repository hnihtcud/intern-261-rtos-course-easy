/* Week 3 / Ex4 -- CPU load, deadlock, watchdog.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "runtime_stats.h"
#include "deadlock_demo.h"
#include "supervisor.h"

void app_main(void)
{
    BaseType_t result;
    runtime_stats_timer_init();
    deadlock_demo_init();
    supervisor_init();

    /* TODO: xTaskCreate task_alpha, task_beta, supervisor_task,
     *       runtime_stats_task. Run deadlock_demo exactly as given first
     *       (step 2 of README) before touching task_alpha/task_beta. */

    result = xTaskCreate(
        task_alpha,
        "alpha",
        256,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
    configASSERT(result == pdPASS);

    result = xTaskCreate(
        task_beta,
        "beta",
        256,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
    configASSERT(result == pdPASS);

    result = xTaskCreate(
        runtime_stats_task,
        "stats",
        512,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(result == pdPASS);
    result = xTaskCreate(supervisor_task, "supervisor", 256, NULL, tskIDLE_PRIORITY + 3, NULL);
    configASSERT(result == pdPASS);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    vTaskStartScheduler();

    for (;;) {
    }
}
