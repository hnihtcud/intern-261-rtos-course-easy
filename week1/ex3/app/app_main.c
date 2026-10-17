/* Week 1 / Ex3 — entry point. */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"

void periodic_task(void *argument);
void dwt_init(void);
/*volatile uint32_t loop_count = 0;
volatile uint32_t measure_done = 0;

static void overhead_task(void *argument)
{
    (void)argument;

    vTaskDelay(pdMS_TO_TICKS(100));

    uint32_t start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < SystemCoreClock) {
        loop_count++;
    }

    measure_done = 1;

    for (;;) {
        vTaskDelay(portMAX_DELAY);  
    }
}*/
void app_main(void)
{
    dwt_init();

    /* TODO: create periodic_task. Its priority must be the highest in the system
     *       — justify why in REPORT.md, and say what the measurement would look
     *       like if it were not. */
    //xTaskCreate(overhead_task, "Overhead", 256, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(periodic_task, "Periodic", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();

    for (;;) {
    }
}
