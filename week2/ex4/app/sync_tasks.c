/* Week 2 / Ex4 — event group sync across EVENT_BIT_COUNT producers, with
 * producer_0's signal optionally replaced by a task notification.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdio.h>
#include "main.h"
#include "params.h"
#include "sync_tasks.h"
#define ENABLE_SYNC_UART_LOG 0
#define MEASURE_FRESH_NOTIFY 0
static EventGroupHandle_t s_events;
extern UART_HandleTypeDef huart1;
void *sync_task_handle;

#define ALL_BITS_MASK ((1u << EVENT_BIT_COUNT) - 1u)
volatile size_t event_group_struct_bytes = 0U;
volatile size_t event_group_heap_bytes = 0U;

static volatile uint32_t notify_start_cycles = 0U;

static volatile uint32_t event_start_cycles = 0U;
static volatile uint32_t event_source_id = 0U;

static volatile uint32_t notify_wake_cycles = 0U;
static volatile uint32_t notify_wake_valid = 0U;
static volatile uint32_t sync_complete_cycles = 0U;

static volatile uint32_t event_wake_cycles = 0U;
static volatile uint32_t event_wake_valid = 0U;
static volatile uint32_t event_wake_source = 0U;
volatile uint32_t sync_uart_errors = 0U;

#define WAKE_SAMPLE_COUNT 100U

/* Notification */
volatile uint32_t notify_sample_count = 0U;
volatile uint64_t notify_sum_cycles = 0U;
volatile float notify_avg_cycles = 0.0f;
volatile float notify_avg_us = 0.0f;

/* Event group */
volatile uint32_t event_sample_count = 0U;
volatile uint64_t event_sum_cycles = 0U;
volatile float event_avg_cycles = 0.0f;
volatile float event_avg_us = 0.0f;

#define SYNC_PERIOD_SAMPLE_COUNT 100U

volatile uint32_t sync_period_sample_count = 0U;
volatile uint64_t sync_period_sum_cycles = 0U;
volatile float sync_period_avg_us = 0.0f;
volatile float sync_period_avg_ms = 0.0f;
volatile uint32_t sync_period_done = 0U;

/* 0: event group; 1: notification */
#define BROADCAST_USE_NOTIFY 1
#define READY_BIT ((EventBits_t)1U)

static TaskHandle_t listener_handles[2];
volatile uint32_t listener_received[2] = {0U, 0U};
volatile uint32_t listener_finished[2] = {0U, 0U};

static uint32_t random_next(uint32_t *state)
{
    uint32_t x = *state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    *state = x;
    return x;
}
void sync_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* TODO 1: xEventGroupCreate(). Check for NULL.
     *         Measure and record in REPORT.md the sizeof the structure this
     *         allocates (see event_groups.c in the kernel sources) to answer
     *         PREDICTION.md question 1. */
    void *warmup = pvPortMalloc(1U);
    configASSERT(warmup != NULL);
    vPortFree(warmup);

    size_t heap_before = xPortGetFreeHeapSize();

    s_events = xEventGroupCreate();
    configASSERT(s_events != NULL);

    size_t heap_after = xPortGetFreeHeapSize();

    event_group_heap_bytes = heap_before - heap_after;


    event_group_struct_bytes = sizeof(StaticEventGroup_t);
}

void producer_task(void *argument)
{
    uint32_t idx = (uint32_t)(uintptr_t)argument;
    configASSERT(idx < EVENT_BIT_COUNT);

    /* Seed riêng cho từng producer, phải khác 0. */
    uint32_t random_state = 0x12345678UL ^ (idx + 1U);
    for (;;) {
        /* TODO 2: random small period (reuse a simple xorshift like ex3's
         *         periodic task, or vTaskDelay with a bounded random jitter). */
        uint32_t delay_ms;
       /* if (idx == 3U) {
            // 200 ms > timeout 120 ms. 
            delay_ms = SYNC_TIMEOUT_MS + 80U;
        } else*/ 
        if (idx == 0U) {
            delay_ms = NOTIFY_PERIOD_MS;
        } else {
            delay_ms = 2U + random_next(&random_state) % 5U;
        }

        TickType_t delay_ticks = pdMS_TO_TICKS(delay_ms);
        configASSERT(delay_ticks > 0U);
        vTaskDelay(delay_ticks);

#if PRODUCER0_USE_NOTIFY
        if (idx == 0) {
            /* TODO 3: xTaskNotifyGive((TaskHandle_t)sync_task_handle).
             *         Toggle a DWT-timestamped GPIO edge right before this
             *         call — this is your t0 for the wake-up latency
             *         measurement. */
            configASSERT(sync_task_handle != NULL);

            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);


            notify_start_cycles = DWT->CYCCNT;

            xTaskNotifyGive((TaskHandle_t)sync_task_handle);
        } else
#endif
        {
            /* TODO 4: xEventGroupSetBits(s_events, 1u << idx).
             *         Same timestamp discipline as TODO 3 for producers that
             *         still use the event group. */

            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);

            event_source_id = idx;
            event_start_cycles = DWT->CYCCNT;

            xEventGroupSetBits(s_events,(EventBits_t)(1UL << idx));
        }
    }
}

void sync_task(void *argument)
{
    (void)argument;
    const TickType_t timeout = pdMS_TO_TICKS(SYNC_TIMEOUT_MS);
    configASSERT(timeout > 0U);
    char buffer[160];

    uint32_t previous_sync_cycles = 0U;
    BaseType_t have_previous_sync = pdFALSE;
    for (;;) {
        BaseType_t success = pdFALSE;
        EventBits_t bits = 0U;
        notify_wake_valid = 0U;
        event_wake_valid = 0U;
#if PRODUCER0_USE_NOTIFY
        /* TODO 5: ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS)) for
         *         producer_0's contribution, THEN xEventGroupWaitBits for the
         *         remaining producers' bits (mask without bit 0), clear-on-exit
         *         = pdTRUE, wait-for-all = pdTRUE, same timeout.
         *         Toggle a GPIO edge the moment both are satisfied — your t1
         *         for the latency measurement. */
        notify_wake_valid = 0U;
        #if MEASURE_FRESH_NOTIFY

        (void)ulTaskNotifyTake(pdTRUE, 0U);
        #endif
        uint32_t wait_start = DWT->CYCCNT;
        uint32_t notifications = ulTaskNotifyTake(pdTRUE, timeout);

        uint32_t notify_end = DWT->CYCCNT;

        if (notifications > 0U) {

            if ((int32_t)(notify_start_cycles - wait_start) > 0) {
                notify_wake_cycles = (uint32_t)(notify_end - notify_start_cycles);
                notify_wake_valid = 1U;
                if (notify_sample_count < WAKE_SAMPLE_COUNT) {
                    notify_sum_cycles += notify_wake_cycles;
                    notify_sample_count++;
                    if (notify_sample_count == WAKE_SAMPLE_COUNT) {
                        notify_avg_cycles = (float)((double)notify_sum_cycles / notify_sample_count);
                        notify_avg_us = (float)((double)notify_sum_cycles * 1000000.0 / SystemCoreClock / notify_sample_count);
                    }
                }
            }

            const EventBits_t remaining_mask = ALL_BITS_MASK & ~((EventBits_t)1U);
            bits = xEventGroupWaitBits(s_events, remaining_mask, pdTRUE, pdTRUE, timeout);
            if ((bits & remaining_mask) == remaining_mask) {
                sync_complete_cycles = DWT->CYCCNT;
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
                success = pdTRUE;
            }
        }
#else
        /* TODO 6: xEventGroupWaitBits(s_events, ALL_BITS_MASK, pdTRUE, pdTRUE,
         *         pdMS_TO_TICKS(SYNC_TIMEOUT_MS)).
         *         Check the return value against ALL_BITS_MASK to tell
         *         "all bits arrived" apart from "timed out with some bits
         *         missing" — do not assume a non-zero return means success. */
         /* Mode thuần event group: chờ đủ cả bốn producer. */
        event_wake_valid = 0U;

        uint32_t event_wait_start = DWT->CYCCNT;

        bits = xEventGroupWaitBits(s_events, ALL_BITS_MASK, pdTRUE, pdTRUE, timeout);


        uint32_t event_end = DWT->CYCCNT;

        if ((bits & ALL_BITS_MASK) == ALL_BITS_MASK) {
            sync_complete_cycles = event_end;

            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);

            success = pdTRUE;

            if ((int32_t)(event_start_cycles - event_wait_start) > 0) {
                event_wake_cycles = (uint32_t)(event_end - event_start_cycles);
                event_wake_source = event_source_id;
                event_wake_valid = 1U;
                if ((event_wake_source == 1U || event_wake_source == 2U) && event_sample_count < WAKE_SAMPLE_COUNT) {
                    event_sum_cycles += event_wake_cycles;
                    event_sample_count++;
                    if (event_sample_count == WAKE_SAMPLE_COUNT) {
                        event_avg_cycles = (float)((double)event_sum_cycles / event_sample_count);
                        event_avg_us = (float)( (double)event_sum_cycles * 1000000.0 / SystemCoreClock / event_sample_count);
                    }
                }
            }
        }
#endif

        /* TODO 7: print the sync instant and measured period to UART. */
        int length = 0;

        if (success == pdTRUE) {
            uint32_t current_sync_cycles = sync_complete_cycles;

            uint32_t period_us = 0U;

            if (have_previous_sync == pdTRUE) {
                uint32_t period_cycles =(uint32_t)(current_sync_cycles - previous_sync_cycles);
                period_us = (uint32_t)((uint64_t)period_cycles * 1000000ULL / SystemCoreClock);

                if (sync_period_sample_count < SYNC_PERIOD_SAMPLE_COUNT) {
                    sync_period_sum_cycles += period_cycles;
                    sync_period_sample_count++;
                    if (sync_period_sample_count == SYNC_PERIOD_SAMPLE_COUNT) {
                        double average_us = (double)sync_period_sum_cycles * 1000000.0 / SystemCoreClock / sync_period_sample_count;
                        sync_period_avg_us = (float)average_us;
                        sync_period_avg_ms = (float)(average_us / 1000.0);
                        sync_period_done = 1U;
                    }
                }
            }
            previous_sync_cycles = current_sync_cycles;
            have_previous_sync = pdTRUE;
#if ENABLE_SYNC_UART_LOG

            uint32_t time_ms = (uint32_t)((uint64_t)xTaskGetTickCount() * 1000ULL / configTICK_RATE_HZ);

            length = snprintf(buffer, sizeof(buffer), "SYNC time=%lu ms, period=%lu us\r\n", (unsigned long)time_ms, (unsigned long)period_us);
#endif
        } else {
#if PRODUCER0_USE_NOTIFY
            if (notifications == 0U) {
                length = snprintf(buffer, sizeof(buffer), "TIMEOUT notification\r\n");
            } else {
                const EventBits_t expected_mask = ALL_BITS_MASK & ~((EventBits_t)1U);
                length = snprintf(buffer, sizeof(buffer), "TIMEOUT event bits=0x%lx, expected=0x%lx\r\n", (unsigned long)bits, (unsigned long)expected_mask);
            }
#else
            length = snprintf(buffer, sizeof(buffer), "TIMEOUT event bits=0x%lx, expected=0x%lx\r\n", (unsigned long)bits, (unsigned long)ALL_BITS_MASK);
#endif
        }
#if ENABLE_SYNC_UART_LOG
        if (length > 0 && (size_t)length < sizeof(buffer)) {
            HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t *)buffer, (uint16_t)length, 100U);
            if (status != HAL_OK) {
                sync_uart_errors++;
            }
        } else 
            sync_uart_errors++;
#endif
    }

}
static void listener_task(void *argument)
{
    uint32_t id = (uint32_t)(uintptr_t)argument;
#if BROADCAST_USE_NOTIFY
    uint32_t received = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS));
    listener_received[id] = (received > 0U);
#else
    EventBits_t bits = xEventGroupWaitBits(s_events, READY_BIT, pdTRUE, pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS));
    listener_received[id] = ((bits & READY_BIT) == READY_BIT);
#endif

    listener_finished[id] = 1U;

    /* Chạy thử đúng một lượt. */
    for (;;) {
        vTaskSuspend(NULL);
    }
}
static void broadcaster_task(void *argument)
{
    (void)argument;

    /*
     * Hai listener ưu tiên cao hơn nên đã chạy và
     * vào chờ trước khi broadcaster được chạy.
     */
    vTaskDelay(pdMS_TO_TICKS(10));

#if BROADCAST_USE_NOTIFY
    xTaskNotifyGive(listener_handles[0]);
#else
    /* Chỉ một lần set bit dùng chung. */
    xEventGroupSetBits(s_events, READY_BIT);
#endif

    /* Chờ các listener hoàn tất, kể cả lượt timeout. */
    while (listener_finished[0] == 0U || listener_finished[1] == 0U) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    char buffer[100];

    int length = snprintf(buffer, sizeof(buffer), "TEST notify=%u, A=%lu, B=%lu\r\n", (unsigned)BROADCAST_USE_NOTIFY, (unsigned long)listener_received[0], (unsigned long)listener_received[1]);

    if (length > 0 && (size_t)length < sizeof(buffer)) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, (uint16_t)length, 100U);
    }

    for (;;) {
        vTaskSuspend(NULL);
    }
}

void broadcast_test_create(void)
{
    for (uint32_t i = 0U; i < 2U; i++) {
        BaseType_t created = xTaskCreate(listener_task, i == 0U ? "ListenerA" : "ListenerB", 128U, (void *)(uintptr_t)i, 3U, &listener_handles[i]);
        configASSERT(created == pdPASS);
    }

    BaseType_t created = xTaskCreate(broadcaster_task, "Broadcaster", 384U, NULL, 2U, NULL);
    configASSERT(created == pdPASS);
}