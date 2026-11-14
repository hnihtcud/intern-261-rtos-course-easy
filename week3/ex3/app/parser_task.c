/* Week 3 / Ex3 -- command parser: rx_stream -> change demo_task's
 * period/priority/suspend state at runtime.
 *
 * Commands (each terminated by '\r', max MAX_CMD_LEN bytes):
 *   P<ms>\r   -- set demo_task period to <ms>
 *   S<prio>\r -- vTaskPrioritySet(demo_task, <prio>)
 *   T<0|1>\r  -- 0 = vTaskSuspend(demo_task), 1 = vTaskResume(demo_task)
 */
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "main.h"
#include "params.h"
#include "uart_rx.h"
#include "parser_task.h"

static TaskHandle_t s_demo_task_handle;
static volatile uint32_t s_demo_period_ms = 200;
volatile UBaseType_t priority_before;
volatile UBaseType_t priority_after;

volatile uint32_t test_p200_ok = 0U;
volatile uint32_t test_last_seq = 0U;
volatile uint32_t test_missing_commands = 0U;
volatile uint32_t test_seq_errors = 0U;
static BaseType_t parse_u32(const char *text, size_t len, uint32_t *result)
{
    if (text == NULL || result == NULL || len == 0U) {
        return pdFALSE;
    }

    uint32_t value = 0U;

    for (size_t i = 0U; i < len; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return pdFALSE;
        }

        uint32_t digit = (uint32_t)(text[i] - '0');

        if (value > (UINT32_MAX - digit) / 10U) {
            return pdFALSE;
        }

        value = value * 10U + digit;
    }

    *result = value;
    return pdTRUE;
}
// static void apply_command(const char *cmd, size_t len)
// {
//     /* TODO 1: switch on cmd[0]: 'P' -> parse decimal into s_demo_period_ms
//      *         (demo_task reads this every loop, no restart needed);
//      *         'S' -> parse decimal, vTaskPrioritySet(s_demo_task_handle, n);
//      *         'T' -> cmd[1]=='0' ? vTaskSuspend(...) : vTaskResume(...).
//      *         Ignore anything malformed -- do not crash on bad input. */
//     if (cmd == NULL || len < 2U) {
//         return;
//     }

//     switch (cmd[0]) {
//     case 'P':
//     case 'S': {
//         uint32_t value = 0U;
//         for (size_t i = 1U; i < len; i++) {
//             if (cmd[i] < '0' || cmd[i] > '9') 
//                 return;
//             uint32_t digit = (uint32_t)(cmd[i] - '0');
//             if (value > (UINT32_MAX - digit) / 10U) 
//                 return;

//             value = value * 10U + digit;
//         }

//         if (cmd[0] == 'P') {
//             if (value == 0U || pdMS_TO_TICKS(value) == 0U) 
//                 return;

//             s_demo_period_ms = value;
//             if (len == 4U && memcmp(cmd, "P200", 4U) == 0) 
//                 test_p200_ok++;
//         } else {
//             if (value >= configMAX_PRIORITIES || s_demo_task_handle == NULL) 
//                 return;

//             priority_before = uxTaskPriorityGet(s_demo_task_handle);

//             vTaskPrioritySet(s_demo_task_handle, (UBaseType_t)value);

//             priority_after = uxTaskPriorityGet(s_demo_task_handle);
//         }
//         break;
//     }

//     case 'T':
//         if (len != 2U || s_demo_task_handle == NULL) 
//             return;

//         if (cmd[1] == '0') 
//             vTaskSuspend(s_demo_task_handle);
//         else if (cmd[1] == '1') 
//             vTaskResume(s_demo_task_handle);
//         break;

//     default:
//         break;
//     }
// }
static void apply_command(const char *cmd, size_t len)
{
    if (cmd == NULL || len < 4U) {
        return;
    }

    size_t hash_pos = 0U;

    while (hash_pos < len && cmd[hash_pos] != '#') {
        hash_pos++;
    }

    if (hash_pos < 2U || hash_pos >= len - 1U) {
        return;
    }

    uint32_t value;
    uint32_t seq;

    if (parse_u32(&cmd[1], hash_pos - 1U, &value) != pdTRUE) {
        return;
    }

    if (parse_u32(&cmd[hash_pos + 1U],
                  len - hash_pos - 1U, &seq) != pdTRUE) {
        return;
    }

    if (seq == 0U) {
        return;
    }

    switch (cmd[0]) {
    case 'P':
        if (value == 0U || pdMS_TO_TICKS(value) == 0U) {
            return;
        }

        s_demo_period_ms = value;
        break;

    case 'S':
        if (value >= configMAX_PRIORITIES || s_demo_task_handle == NULL) {
            return;
        }

        priority_before = uxTaskPriorityGet(s_demo_task_handle);

        vTaskPrioritySet(s_demo_task_handle, (UBaseType_t)value);

        priority_after = uxTaskPriorityGet(s_demo_task_handle);
        break;

    case 'T':
        if (hash_pos != 2U || value > 1U ||
            s_demo_task_handle == NULL) {
            return;
        }

        if (value == 0U) {
            vTaskSuspend(s_demo_task_handle);
        } else {
            vTaskResume(s_demo_task_handle);
        }
        break;

    default:
        return;
    }

    if (seq > test_last_seq) {
        test_missing_commands += seq - test_last_seq - 1U;
        test_last_seq = seq;
    } else {
        test_seq_errors++;
    }
}
void parser_task(void *argument)
{
    (void)argument;
    char cmd[MAX_CMD_LEN];
    size_t idx = 0;
    BaseType_t discard = pdFALSE;
    for (;;) {
        uint8_t byte;

        /* TODO 2: xStreamBufferReceive(rx_stream, &byte, 1, portMAX_DELAY). */

        /* TODO 3: if byte == '\r', call apply_command(cmd, idx), idx = 0.
         *         else if idx < MAX_CMD_LEN - 1, cmd[idx++] = byte.
         *         else (overflow): discard the whole partial command, idx = 0
         *         -- explain in REPORT.md why silently truncating instead
         *         would be worse. */
        if (xStreamBufferReceive(rx_stream, &byte, 1, portMAX_DELAY) != 1U) 
            continue;
        if (byte == '\r') {
            if (discard == pdFALSE && idx > 0U) {
                cmd[idx] = '\0';
                apply_command(cmd, idx);
            }

            idx = 0U;
            discard = pdFALSE;
        } else if (byte == '\n') {
            continue;
        } else if (discard == pdTRUE) {
            continue;
        } else if (idx < sizeof(cmd) - 1U) {
            cmd[idx++] = (char)byte;
        } else {
            idx = 0U;
            discard = pdTRUE;
        }
    }
}

void demo_task(void *argument)
{
    (void)argument;
    s_demo_task_handle = xTaskGetCurrentTaskHandle();

    for (;;) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
        vTaskDelay(pdMS_TO_TICKS(s_demo_period_ms));
    }
}
