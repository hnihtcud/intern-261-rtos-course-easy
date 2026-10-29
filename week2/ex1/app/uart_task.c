/* Week 2 / Ex1 — prints filtered samples to USART1 at UART_BAUD.
 * queue_filtered -> UART.
 *
 * This is also the task you deliberately starve in step 7 (temporarily set
 * UART_BAUD-equivalent hardware baud to 9600) to force queue_filtered to fill.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

#include "main.h"
#include "params.h"
#include "pipeline.h"
extern UART_HandleTypeDef huart1;
volatile uint32_t uart_error_count = 0;
void uart_task(void *argument)
{
    (void)argument;
    char buffer[64];
    for (;;) {
        filtered_sample_t in;
        /* TODO 1: xQueueReceive(queue_filtered, &in, portMAX_DELAY). */
        if (xQueueReceive(queue_filtered, &in, portMAX_DELAY) == pdPASS){
            //int length = snprintf(buffer,  sizeof(buffer), "%lu,%d,%d,%d\r\n", (unsigned long)in.seq, (int)in.ax, (int)in.ay,(int)in.az);
            int length = snprintf(buffer, sizeof(buffer), "%lu,%d,%d\r\n", (unsigned long)in.seq, (int)in.raw_ax, (int)in.ax);
            if (length > 0 && (size_t)length < sizeof(buffer)) {
                HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t *)buffer, (uint16_t)length, 100);
                if (status != HAL_OK) {
                    uart_error_count++;
                }
            }
            else {
                uart_error_count++;
            }
        }
        /* TODO 2: format "seq,ax,ay,az\r\n" and HAL_UART_Transmit on USART1.
         *         Transmit time at UART_BAUD is the thing that makes this
         *         task slow relative to sensor_task/filter_task — say so in
         *         REPORT.md with the actual byte time you computed. */
    }
}
