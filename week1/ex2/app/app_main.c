/* Week 1 / Ex2 — priorities, preemption, starvation.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "tasks.h"

void app_main(void)
{
    dwt_init();

    /* TODO: create the three tasks below with the priorities from params.h.
     *       Keep the stacks as small as you can still justify — ex4 will make
     *       you prove the number. */
    BaseType_t res;
    res = xTaskCreate(task_a, "A", configMINIMAL_STACK_SIZE, NULL, TASK_A_PRIO, NULL);
    configASSERT(res == pdPASS);
    res = xTaskCreate(task_b, "B", configMINIMAL_STACK_SIZE, NULL, TASK_B_PRIO, NULL);
    configASSERT(res == pdPASS);
    res = xTaskCreate(task_c, "C", configMINIMAL_STACK_SIZE, NULL, TASK_C_PRIO, NULL);
    configASSERT(res == pdPASS);
    vTaskStartScheduler();

    for (;;) {
    }
}
