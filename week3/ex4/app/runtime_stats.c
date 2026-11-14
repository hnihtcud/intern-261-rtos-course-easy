/* Week 3 / Ex4 -- run-time stats on DWT CYCCNT.
 * Native FreeRTOS API only.
 */
#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "main.h"
#include "params.h"
#include "runtime_stats.h"
extern UART_HandleTypeDef huart1;
void runtime_stats_timer_init(void)
{
    /* TODO 1: same DWT enable sequence as week1/ex2 dwt_init() (CoreDebug
     *         DEMCR TRCENA bit, zero DWT->CYCCNT, DWT->CTRL CYCCNTENA bit).
     *         If you already have week1/ex2's dwt_init(), the sequence is
     *         identical -- do not invent a second mechanism. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t runtime_stats_timer_value(void)
{
    /* TODO 2: return DWT->CYCCNT. This is portGET_RUN_TIME_COUNTER_VALUE()'s
     *         implementation -- wire it up in FreeRTOSConfig.deltas. */
    return DWT->CYCCNT;
}

void runtime_stats_task(void *argument)
{
    (void)argument;
    static char buf[512];

    for (;;) {
        /* TODO 3: vTaskGetRunTimeStats(buf) and print buf over UART.
         *         Note: this walks every task's TCB -- it is itself not free,
         *         factor that into why STATS_WINDOW_MS should not be too
         *         short. */
        vTaskDelay(pdMS_TO_TICKS(STATS_WINDOW_MS));
        buf[0] = '\0';
        vTaskGetRunTimeStats(buf);

        static const char header[] = "\r\nTask\tCounter\tCPU%\r\n";

        HAL_UART_Transmit(&huart1, (uint8_t *)header, sizeof(header) - 1U, 100);

        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)strlen(buf), 100);
        TaskStatus_t tasks[8];
        UBaseType_t count;
        uint32_t total_run_time;

        count = uxTaskGetSystemState(tasks, sizeof(tasks) / sizeof(tasks[0]), &total_run_time);

        for (UBaseType_t i = 0; i < count; i++) {
            const char *state;
            switch (tasks[i].eCurrentState) {
                case eRunning:   state = "eRunning";   break;
                case eReady:     state = "eReady";     break;
                case eBlocked:   state = "eBlocked";   break;
                case eSuspended: state = "eSuspended"; break;
                case eDeleted:   state = "eDeleted";   break;
                default:         state = "Unknown";   break;
            }

            char line[96];

            snprintf(line, sizeof(line), "%s\t%s\r\n", tasks[i].pcTaskName, state);

            HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)strlen(line), 100);
        }
    }
}
