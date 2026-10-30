/* Week 2 / Ex2 — WRITER_COUNT identical writer tasks. Each prints an MSG_LEN
 * byte string with its own id + sequence number, every WRITER_PERIOD_MS.
 */
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "params.h"
#include "uart_guard.h"

/* TODO 1: one writer_task(void *argument) taking the writer index via the
 *         task parameter (argument), NOT WRITER_COUNT copy-pasted bodies.
 *         Format a MSG_LEN-byte string containing the writer id and a
 *         per-writer sequence number, then uart_guard_send() it, then
 *         vTaskDelay(pdMS_TO_TICKS(WRITER_PERIOD_MS)). */
uint32_t writer_request_cycles[WRITER_COUNT];
volatile uint32_t writer_tx_end_cycles[WRITER_COUNT];
volatile uint32_t writer_tx_done[WRITER_COUNT];
extern volatile uint32_t latency_done;
void writer_task(void *argument)
{
    uint32_t id = (uint32_t)(uintptr_t)argument;
    uint32_t seq = 0;
    char msg[MSG_LEN + 1U];

    configASSERT(id < WRITER_COUNT);
    configASSERT(MSG_LEN >= 3U);

    const uint32_t wait_cycles = (SystemCoreClock / 1000U) * WRITER_PERIOD_MS;
    uint32_t request_time = DWT->CYCCNT;
    uint32_t stats_calculated = 0U; 
    for (;;) {
        /* TODO 2 */
        while ((int32_t)(DWT->CYCCNT - request_time) < 0) {
            taskYIELD();
        }
        writer_request_cycles[id] = request_time;  
        

        int length = snprintf(msg, sizeof(msg), "%02lu:%04lu ", (unsigned long)id, (unsigned long)seq);
        configASSERT(length >= 0);
        configASSERT((size_t)length <= MSG_LEN - 2U);
        memset(&msg[length], (int)('A' + id), MSG_LEN - 2U - (size_t)length);
        msg[MSG_LEN - 2U] = '\r';
        msg[MSG_LEN - 1U] = '\n';
        msg[MSG_LEN] = '\0';
        
#if UART_GUARD_MODE == 0
    /* Mode tái hiện lỗi: giữ cách chạy cũ. */
    uart_guard_send(msg, MSG_LEN);
    request_time = DWT->CYCCNT + wait_cycles;
#else
    writer_tx_done[id] = 0U;

    uart_guard_send(msg, MSG_LEN);

    while (writer_tx_done[id] == 0U) {
        taskYIELD();
    }

    
    request_time = writer_tx_end_cycles[id] + wait_cycles;
#endif

        if (id == 0U && stats_calculated == 0U && latency_done != 0U) {
                latency_calculate();
                stats_calculated = 1U;
    }
        seq++;
    }
}