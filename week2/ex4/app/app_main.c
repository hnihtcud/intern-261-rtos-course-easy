/* Week 2 / Ex4 — event group & task notification.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include "main.h"
#include "params.h"
#include "sync_tasks.h"
#define SYNC_STACK_WORDS       384U
#define PRODUCER_STACK_WORDS   128U

#define SYNC_PRIORITY          3U
#define PRODUCER_PRIORITY      2U
void app_main(void)
{
    sync_init();

    /* TODO 1: xTaskCreate sync_task first, store its handle into
     *         sync_task_handle (cast to void*) — producer_0 needs it when
     *         PRODUCER0_USE_NOTIFY is set. */
    TaskHandle_t sync_handle = NULL;

    BaseType_t created = xTaskCreate(sync_task, "Sync", SYNC_STACK_WORDS, NULL, SYNC_PRIORITY, &sync_handle);
    configASSERT(created == pdPASS);
    sync_task_handle = (void *)sync_handle;
    /* TODO 2: xTaskCreate EVENT_BIT_COUNT instances of producer_task, each
     *         given its own index (0..EVENT_BIT_COUNT-1) as the task
     *         parameter. */
    for (uint32_t i = 0U; i < EVENT_BIT_COUNT; i++) {
        created = xTaskCreate( producer_task,  "Producer", PRODUCER_STACK_WORDS, (void *)(uintptr_t)i, PRODUCER_PRIORITY, NULL);
        configASSERT(created == pdPASS);
    }
    //broadcast_test_create();
    vTaskStartScheduler();
    configASSERT(0);
    for (;;) {
    }
}
