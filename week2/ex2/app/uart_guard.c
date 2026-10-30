/* Week 2 / Ex2 — four ways to serialize access to USART1 from WRITER_COUNT
 * tasks. Only one UART_GUARD_MODE is compiled at a time.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "uart_guard.h"


volatile uint32_t max_release_to_run_cycles = 0;
volatile uint32_t max_prepare_cycles = 0;

extern UART_HandleTypeDef huart1;
extern volatile uint32_t writer_tx_end_cycles[WRITER_COUNT];
extern volatile uint32_t writer_tx_done[WRITER_COUNT];
static void writer_tx_finished(uint32_t id, uint32_t end)
{
    writer_tx_end_cycles[id] = end;
    writer_tx_done[id] = 1U;
}
extern uint32_t writer_request_cycles[WRITER_COUNT];
#define UART_TIMEOUT_MS          100U
#define GATEKEEPER_QUEUE_LENGTH  WRITER_COUNT
#define GATEKEEPER_STACK_WORDS   256
#define LATENCY_SAMPLE_COUNT 400U
volatile uint32_t latency_count = 0;
volatile uint64_t latency_sum_cycles = 0;
volatile uint32_t latency_max_cycles = 0;

volatile float latency_avg_us = 0;
volatile float latency_max_us = 0;
volatile uint32_t latency_done = 0;
volatile uint32_t latency_error_count = 0;

#if UART_GUARD_MODE == 1
static SemaphoreHandle_t s_uart_mutex;
#elif UART_GUARD_MODE == 3
static QueueHandle_t s_gatekeeper_queue;
void gatekeeper_task(void *argument);
#endif


static void record_latency(uint32_t elapsed, HAL_StatusTypeDef status)
{
    taskENTER_CRITICAL();

    if (latency_done == 0U) {
        if (status != HAL_OK) {
            latency_error_count++;
        } else {
            latency_sum_cycles += elapsed;

            if (elapsed > latency_max_cycles) {
                latency_max_cycles = elapsed;
            }

            latency_count++;

            if (latency_count == LATENCY_SAMPLE_COUNT) {
                latency_done = 1U;
            }
        }
    }

    taskEXIT_CRITICAL();
}
void latency_calculate(void)
{
    if (latency_done != 0U) {
        double cycles_per_us = (double)SystemCoreClock / 1000000.0;

        latency_avg_us = (float)(
            (double)latency_sum_cycles / LATENCY_SAMPLE_COUNT / cycles_per_us
        );

        latency_max_us = (float)((double)latency_max_cycles / cycles_per_us);
    }
}

static HAL_StatusTypeDef uart_transmit(const char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t *)&buf[i], 1, UART_TIMEOUT_MS);
        if (status != HAL_OK) 
            return status;
        #if UART_GUARD_MODE == 0
            if (i + 1U < len) {
                taskYIELD();
            }
        #endif
    }
    return HAL_OK;
}
void uart_guard_init(void)
{
#if UART_GUARD_MODE == 1
    /* TODO 1: xSemaphoreCreateMutex(). Check for NULL. */
    s_uart_mutex = xSemaphoreCreateMutex();
    configASSERT(s_uart_mutex != NULL);

#elif UART_GUARD_MODE == 3
    /* TODO 2: create s_gatekeeper_queue (length: pick and justify in
     *         REPORT.md), item size = MSG_LEN bytes.
     *         xTaskCreate(gatekeeper_task, ..., GATEKEEPER_PRIO). */
    s_gatekeeper_queue = xQueueCreate(GATEKEEPER_QUEUE_LENGTH, MSG_LEN);
    configASSERT(s_gatekeeper_queue != NULL);
    BaseType_t created = xTaskCreate(gatekeeper_task, "Gatekeeper", GATEKEEPER_STACK_WORDS, NULL, GATEKEEPER_PRIO, NULL);
    configASSERT(created == pdTRUE);
#endif
}

#if UART_GUARD_MODE == 3

/* Đủ chỗ cho bản tin trong queue, đang truyền và đang chờ gửi. */
#define TIMESTAMP_SLOTS (GATEKEEPER_QUEUE_LENGTH + 2U)

typedef struct {
    uint32_t seq;
    uint32_t start_cycles;
} timestamp_t;

static timestamp_t timestamps[WRITER_COUNT][TIMESTAMP_SLOTS];

static void get_message_key(const char *buf, size_t len,
                            uint32_t *id, uint32_t *seq)
{
    configASSERT(len >= 4U);
    configASSERT(buf[0] >= '0' && buf[0] <= '9');
    configASSERT(buf[1] >= '0' && buf[1] <= '9');
    configASSERT(buf[2] == ':');

    *id = (uint32_t)(buf[0] - '0') * 10U
        + (uint32_t)(buf[1] - '0');

    configASSERT(*id < WRITER_COUNT);

    *seq = 0;
    size_t i = 3U;

    while (i < len && buf[i] >= '0' && buf[i] <= '9') {
        *seq = *seq * 10U + (uint32_t)(buf[i] - '0');
        i++;
    }

    configASSERT(i > 3U);
}

#endif

volatile uint32_t uart_busy_count[WRITER_COUNT] = {0};
void uart_guard_send(const char *buf, size_t len)
{
    configASSERT(buf != NULL);
    configASSERT(len == MSG_LEN);

#if UART_GUARD_MODE != 0
    configASSERT(len >= 3U);
    configASSERT(buf[0] >= '0' && buf[0] <= '9');
    configASSERT(buf[1] >= '0' && buf[1] <= '9');
    configASSERT(buf[2] == ':');

    uint32_t writer_id =(uint32_t)(buf[0] - '0') * 10U + (uint32_t)(buf[1] - '0');

    configASSERT(writer_id < WRITER_COUNT);

    uint32_t start = writer_request_cycles[writer_id];
#endif

#if UART_GUARD_MODE == 0
    /* The bug: no serialization at all. */
    // HAL_StatusTypeDef status = uart_transmit(buf, len);
    // (void)status;
    HAL_StatusTypeDef status = uart_transmit(buf, len);

    /* Bản tin bắt đầu bằng ID hai chữ số, ví dụ "02:". */
    uint32_t id = (uint32_t)(buf[0] - '0') * 10U
                + (uint32_t)(buf[1] - '0');
    if (id < WRITER_COUNT && status == HAL_BUSY) {
        uart_busy_count[id]++;
    }
    /* TODO 3: HAL_UART_Transmit directly. */
#elif UART_GUARD_MODE == 1
    /* TODO 4: xSemaphoreTake(s_uart_mutex, portMAX_DELAY), transmit, Give.
     *         Every return path must Give — no early return between Take and
     *         Give. */
    BaseType_t taken =  xSemaphoreTake(s_uart_mutex, portMAX_DELAY);
    configASSERT(taken == pdTRUE);
    uint32_t acquired = DWT->CYCCNT;
    HAL_StatusTypeDef status = uart_transmit(buf, len);
    uint32_t end = DWT->CYCCNT;
    uint32_t elapsed = (uint32_t)(end - start);
    writer_tx_finished(writer_id, end);
    BaseType_t given = xSemaphoreGive(s_uart_mutex);
    configASSERT(given == pdTRUE);
    record_latency(elapsed, status);
    (void)status;

#elif UART_GUARD_MODE == 2
    /* TODO 5: taskENTER_CRITICAL(), transmit, taskEXIT_CRITICAL().
     *         Measure in REPORT.md how long the critical section holds IRQs
     *         off, as a function of MSG_LEN and UART_BAUD. */
    taskENTER_CRITICAL();
    HAL_StatusTypeDef status = uart_transmit(buf, len);
    uint32_t end = DWT->CYCCNT;
    uint32_t elapsed = (uint32_t)(end - start);
    writer_tx_finished(writer_id, end);
    taskEXIT_CRITICAL();
    record_latency(elapsed, status);
    (void)status;


#elif UART_GUARD_MODE == 3
    /* TODO 6: xQueueSend(s_gatekeeper_queue, buf, portMAX_DELAY).
     *         The writer never touches UART directly in this mode. */
    uint32_t id;
    uint32_t seq;
    get_message_key(buf, len, &id, &seq);

    uint32_t slot = seq % TIMESTAMP_SLOTS;

    timestamps[id][slot].seq = seq;
    timestamps[id][slot].start_cycles = start;
    BaseType_t sent = xQueueSend(s_gatekeeper_queue, buf, portMAX_DELAY);
    configASSERT(sent == pdTRUE);
#endif
}

#if UART_GUARD_MODE == 3
void gatekeeper_task(void *argument)
{
    (void)argument;
    for (;;) {
        char msg[MSG_LEN];
        /* TODO 7: xQueueReceive(s_gatekeeper_queue, msg, portMAX_DELAY), then
         *         HAL_UART_Transmit. This is the only task allowed to call
         *         HAL_UART_Transmit in this mode. */
        BaseType_t received = xQueueReceive(s_gatekeeper_queue, msg, portMAX_DELAY);
        configASSERT(received == pdTRUE);

        uint32_t id;
        uint32_t seq;

        get_message_key(msg, sizeof(msg), &id, &seq);
        uint32_t slot = seq % TIMESTAMP_SLOTS;
        configASSERT(timestamps[id][slot].seq == seq);
        uint32_t start = timestamps[id][slot].start_cycles;

        
        HAL_StatusTypeDef status = uart_transmit(msg, MSG_LEN);
        uint32_t end = DWT->CYCCNT;
        uint32_t elapsed = (uint32_t)(end - start);
        writer_tx_finished(id, end);
        record_latency(elapsed, status);
        (void)status;
    }
}
#endif