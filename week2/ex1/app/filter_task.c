/* Week 2 / Ex1 — moving-average filter, window = FILTER_WINDOW samples.
 * queue_raw -> queue_filtered.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"
volatile uint32_t filter_queue_drops = 0;
void filter_task(void *argument)
{
    (void)argument;
    int16_t buffer_ax[FILTER_WINDOW] = {0};
    int16_t buffer_ay[FILTER_WINDOW] = {0};
    int16_t buffer_az[FILTER_WINDOW] = {0};
    int32_t sum_ax = 0, sum_ay = 0, sum_az = 0, index = 0, count = 0;
     configASSERT(FILTER_WINDOW > 0);
    /* TODO 1: ring buffers for ax/ay/az, size FILTER_WINDOW, plus running sums.
     *         Do NOT recompute the full sum every sample — that is the whole
     *         point of a moving-average window. */

    for (;;) {
        raw_sample_t in;
        if (xQueueReceive(queue_raw, &in, portMAX_DELAY) == pdPASS) {
            sum_ax -= buffer_ax[index];
            sum_ay -= buffer_ay[index];
            sum_az -= buffer_az[index];

            buffer_ax[index] = in.ax;
            buffer_ay[index] = in.ay;
            buffer_az[index] = in.az;

            sum_ax += in.ax;
            sum_ay += in.ay;
            sum_az += in.az;
            if (count < FILTER_WINDOW) {
                count++;
            }
            
            
        /* TODO 2: xQueueReceive(queue_raw, &in, portMAX_DELAY). */

        /* TODO 3: push into the ring buffer, update running sums, compute the
         *         averaged ax/ay/az. */

            filtered_sample_t out;
            out.seq = in.seq;
            out.raw_ax = in.ax;
            out.ax = (int16_t)(sum_ax / (int32_t)count);
            out.ay = (int16_t)(sum_ay / (int32_t)count);
            out.az = (int16_t)(sum_az / (int32_t)count);
        /* TODO 4: fill out.ax/ay/az from the running average. */
            index++;
            if (index >= FILTER_WINDOW) {
                index = 0;
            }

            if (xQueueSend(queue_filtered, &out, portMAX_DELAY) != pdPASS) {
                filter_queue_drops++;
            }
        /* TODO 5: xQueueSend(queue_filtered, &out, <timeout>) — same question
         *         as sensor_task: what should happen here if uart_task is
         *         slow and queue_filtered is full? */
        }
    }
}
