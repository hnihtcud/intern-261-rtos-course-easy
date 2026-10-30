/* Week 2 / Ex2 — race condition & mutex.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "uart_guard.h"
#include <stdio.h>  
#define WRITER_STACK_WORDS  256U
#define WRITER_PRIORITY     2U

void writer_task(void *argument);
volatile size_t guard_heap_before = 0;
volatile size_t guard_heap_after  = 0;
volatile size_t guard_ram_bytes   = 0;
volatile uint32_t guard_ram_done  = 0;
extern volatile uint32_t latency_done;
static void measurement_dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
volatile uint32_t probe_max_cycles = 0;
// Hàm đo ảnh hưởng task khác
static void probe_task(void *argument)
{
    (void)argument;

    configASSERT(configTICK_RATE_HZ == 1000);

    TickType_t last_wake = xTaskGetTickCount();

    vTaskDelayUntil(&last_wake, 1);
    uint32_t previous = DWT->CYCCNT;

    for (;;) {
        vTaskDelayUntil(&last_wake, 1);  

        uint32_t now = DWT->CYCCNT;
        uint32_t elapsed = (uint32_t)(now - previous);
        previous = now;

        if (latency_done == 0U && elapsed > probe_max_cycles) {
            probe_max_cycles = elapsed;
        }
    }
}

void app_main(void)
{
    measurement_dwt_init();
    void *warmup = pvPortMalloc(1);
    configASSERT(warmup != NULL);
    vPortFree(warmup);
    guard_heap_before = xPortGetFreeHeapSize();
    uart_guard_init();
    guard_heap_after = xPortGetFreeHeapSize();
    guard_ram_bytes = guard_heap_before - guard_heap_after;
    guard_ram_done = 1;
    for(int i = 0; i < WRITER_COUNT; i++) {
        char name[configMAX_TASK_NAME_LEN];
        snprintf(name, sizeof(name),"Writer%lu", (unsigned long)i);
        BaseType_t created = xTaskCreate(writer_task, name, WRITER_STACK_WORDS, (void *)(uintptr_t)i, WRITER_PRIORITY, NULL);
        configASSERT(created == pdTRUE);
    }
    /* TODO: xTaskCreate WRITER_COUNT instances of writer_task, each given its
     *       own index (0..WRITER_COUNT-1) as the task parameter.
     *       Do not create WRITER_COUNT separately-named functions. */
   // BaseType_t result = xTaskCreate(probe_task, "Probe", 128, NULL, 4, NULL);
    //configASSERT(result == pdPASS);
    
    vTaskStartScheduler();

    for (;;) {
    }
}