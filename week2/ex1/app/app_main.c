/* Week 2 / Ex1 — sensor pipeline: sensor_task -> queue_raw -> filter_task ->
 * queue_filtered -> uart_task.
 *
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"

QueueHandle_t queue_raw;
QueueHandle_t queue_filtered;

#define DEPTH_SAMPLE_COUNT  1024U
#define DEPTH_PERIOD_MS     1U
typedef struct {
    TickType_t tick;
    uint8_t raw_depth;
    uint8_t filtered_depth;
} queue_depth_sample_t;
volatile queue_depth_sample_t depth_log[DEPTH_SAMPLE_COUNT];
volatile uint32_t depth_log_count = 0;
volatile uint32_t depth_log_done = 0;

void depth_logger_task(void *argument)
{
    (void)argument;

    const TickType_t period = pdMS_TO_TICKS(DEPTH_PERIOD_MS);
    configASSERT(period > 0);

    TickType_t last_wake = xTaskGetTickCount();

    for (uint32_t i = 0; i < DEPTH_SAMPLE_COUNT; i++) {
        depth_log[i].tick = xTaskGetTickCount();
        depth_log[i].raw_depth = (uint8_t)uxQueueMessagesWaiting(queue_raw);
        depth_log[i].filtered_depth = (uint8_t)uxQueueMessagesWaiting(queue_filtered);
        depth_log_count = i + 1U;
        vTaskDelayUntil(&last_wake, period);
    }
    depth_log_done = 1;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void app_main(void)
{
    /* TODO 1: xQueueCreate(QUEUE_LENGTH, sizeof(raw_sample_t)) for queue_raw,
     *         and the equivalent for queue_filtered with filtered_sample_t.
     *         Check both handles are non-NULL before continuing. */
     queue_raw = xQueueCreate(QUEUE_LENGTH,sizeof(raw_sample_t));
     queue_filtered = xQueueCreate(QUEUE_LENGTH,sizeof(filtered_sample_t));

    configASSERT(queue_raw != NULL);
    configASSERT(queue_filtered != NULL);
    /* TODO 2: xTaskCreate sensor_task, filter_task, uart_task.
     *         Pick priorities you can justify in REPORT.md: who must never be
     *         starved by whom? */
     BaseType_t result;

    result = xTaskCreate(sensor_task,"Sensor",256,NULL,3,NULL);
    configASSERT(result == pdPASS);

    result = xTaskCreate(filter_task,"Filter",256,NULL,2,NULL);
    configASSERT(result == pdPASS);

    result = xTaskCreate(uart_task,"UART",512,NULL,1,NULL);
    configASSERT(result == pdPASS);
    /* TODO 3: xTaskCreate depth_logger_task — must not perturb the pipeline it
     *         is measuring (think about its own priority and period). */

    result = xTaskCreate(depth_logger_task,"Depth Logger",256,NULL,1,NULL);
    configASSERT(result == pdPASS);

    vTaskStartScheduler();

    for (;;) {
    }
}
