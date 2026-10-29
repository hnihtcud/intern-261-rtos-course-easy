/* Week 1 / Ex4 — print the live RAM picture over USART1.
 *
 * This is your measurement instrument. It must be honest: read the values from
 * FreeRTOS, never from a constant you typed in.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "main.h"
#include "params.h"

#include "stdio.h"
#include "string.h"

extern UART_HandleTypeDef huart1;

extern TaskHandle_t dyn_handle;
extern TaskHandle_t stat_handle;
/* TODO 1: a blocking UART print helper. It is called from a task, so it may
 *         block — but say in REPORT.md what would break if you called it from
 *         the stack overflow hook instead. */
static void uart_print(const char *text){
    HAL_UART_Transmit(&huart1, (uint8_t *)text,(uint16_t)strlen(text), 100);
}

static void print_stack(const char *name, TaskHandle_t task)
{
    char text[96];
    unsigned long words = uxTaskGetStackHighWaterMark(task);
    snprintf(text, sizeof(text),"%s: %lu words = %lu bytes\r\n", name, words, words * (unsigned long)sizeof(StackType_t));
    uart_print(text);
}

void ram_report_task(void *argument)
{
    (void)argument;
    configASSERT(dyn_handle != NULL);
    configASSERT(stat_handle != NULL);
    char text[128];
    for (;;) {
        /* TODO 2: print, every 2 seconds:
         *   - xPortGetFreeHeapSize()
         *   - xPortGetMinimumEverFreeHeapSize()
         *   - uxTaskGetStackHighWaterMark() for: dyn_task, stat_task,
         *     ram_report_task, the idle task (xTaskGetIdleTaskHandle()),
         *     and the timer task (xTimerGetTimerDaemonTaskHandle()).
         *
         * High water mark is returned in WORDS. Print both words and bytes so
         * your report cannot silently mix the two units. */

        
        size_t free_heap = xPortGetFreeHeapSize();
        size_t min_heap = xPortGetMinimumEverFreeHeapSize();
        snprintf(text, sizeof(text),
                 "\r\n--- RAM REPORT ---\r\n"
                 "Free heap: %lu bytes\r\n"
                 "Minimum-ever free: %lu bytes\r\n",
                 (unsigned long)free_heap,
                 (unsigned long)min_heap);
        uart_print(text);
        print_stack("dyn_task", dyn_handle);
        print_stack("stat_task", stat_handle);
        print_stack("ram_report", NULL); /* NULL = task hien tai */
        print_stack("Idle", xTaskGetIdleTaskHandle());
        print_stack("Timer", xTimerGetTimerDaemonTaskHandle());

        uart_print(free_heap >= TARGET_FREE_HEAP_BYTES
                   ? "Current heap target: PASS\r\n"
                   : "Current heap target: FAIL\r\n");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
