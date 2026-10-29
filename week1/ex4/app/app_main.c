/* Week 1 / Ex4 — entry point. */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"

void ram_report_task(void *argument);
#ifndef ENABLE_STACK_HOG
#define ENABLE_STACK_HOG 0 /* 1 to enable the stack hogging task */
#endif

#ifndef DYN_RUN_STACK_WORDS
#define DYN_RUN_STACK_WORDS 160
#endif

TaskHandle_t dyn_handle = NULL;
TaskHandle_t stat_handle = NULL;
TaskHandle_t ram_report_handle = NULL;
/* TODO 1: static storage for stat_task — TCB and stack.
 *         The stack array type matters; it is not char[]. */
static StaticTask_t stat_tcb;
static StackType_t stat_stack[STATIC_TASK_STACK_WORDS];

static void shared_work(void *argument)
{
    (void)argument;
    volatile uint32_t cnt = 0;
    for (;;) {
        /* TODO 2: identical trivial work in both tasks, so that any difference
         *         in high water mark comes from how they were created, not from
         *         what they do. */
        cnt++;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
#if ENABLE_STACK_HOG == 1
static void hog_work(void *argument)
{
    (void)argument;

    volatile uint8_t hog[HOG_DEPTH_BYTES];

    for (;;) {
        for (uint32_t i = 0; i < HOG_DEPTH_BYTES; i++) {
            hog[i] = (uint8_t)i;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

#endif
void app_main(void)
{
    /* TODO 3: dyn_task  via xTaskCreate,       stack DYN_TASK_STACK_WORDS
     *         stat_task via xTaskCreateStatic, stack STATIC_TASK_STACK_WORDS
     *         ram_report_task via xTaskCreate
     *
     * Check every return value. xTaskCreateStatic returns a handle, not a
     * BaseType_t — explain the difference in REPORT.md. */
    BaseType_t res;

    TaskFunction_t dyn_function = shared_work;
    #if ENABLE_STACK_HOG == 1
        dyn_function = hog_work;
    #endif
    res = xTaskCreate(dyn_function, "dyn_task", DYN_RUN_STACK_WORDS, NULL, tskIDLE_PRIORITY + 1, &dyn_handle);
    configASSERT(res == pdPASS);
    stat_handle = xTaskCreateStatic(shared_work, "stat_task", STATIC_TASK_STACK_WORDS, NULL, tskIDLE_PRIORITY + 1, stat_stack, &stat_tcb);
    configASSERT(stat_handle != NULL);
    res = xTaskCreate(ram_report_task, "ram_report", 512, NULL, tskIDLE_PRIORITY + 1, &ram_report_handle);
    configASSERT(res == pdPASS);
    res = xTaskCreate(shared_work,"extra_task",128,NULL,tskIDLE_PRIORITY + 1,NULL);
    configASSERT(res == pdPASS);
    /* TODO 4 (step 6 of the README, enable only for that run):
     *         declare a HOG_DEPTH_BYTES local array inside dyn_task's work
     *         function and write to every element, so the overflow is real and
     *         not optimised away. Think about what -O2 does to a dead array. */

    vTaskStartScheduler();

    /* Scheduler returned => not enough heap for the idle/timer task. */
    for (;;) {
    }
}
