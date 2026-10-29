/* Week 1 / Ex4 — FreeRTOS hooks.
 *
 * Both hooks must be observable from outside the chip. A breakpoint is not a
 * deliverable: you need a UART line or a distinct LED pattern you can photograph.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"

extern UART_HandleTypeDef huart1;

static void hook_uart_send(const char *text, uint16_t length)
{
    if (text != NULL && length > 0U)
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)text, length, HAL_MAX_DELAY);
}

static void hook_uart_puts(const char *text)
{
    if (text == NULL) {
        return;
    }
    uint16_t length = 0;
    while (text[length] != '\0') {
        length++;
    }
    hook_uart_send(text, length);
}

static void hook_uart_task_name(const char *name)
{
    uint16_t length = 0;
    while (length < configMAX_TASK_NAME_LEN && name[length] != '\0') {
        length++;
    }
    hook_uart_send(name, length);
}
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* TODO 1: report xTask / pcTaskName out of the chip, then stop.
     *
     * Careful: the stack of the offending task is already corrupt. Think about
     * what you are allowed to call here, and write the reasoning in REPORT.md.
     * Hint: this hook runs from the scheduler's context, not the task's.
     */
    (void)xTask;
    hook_uart_puts("Stack overflow in task: ");
    hook_uart_task_name(pcTaskName);
    hook_uart_puts("\r\n");
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    /* TODO 2: report, then stop. */
    hook_uart_puts("Malloc failed\r\n");

    for (;;) {
    }
}

/* configSUPPORT_STATIC_ALLOCATION = 1 makes these two callbacks mandatory.
 * Provide the memory for the idle task and the timer task here. */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    /* TODO 3: static buffers. Note the size is in WORDS, not bytes —
     *         getting this wrong is a silent overflow. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack; 
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    /* TODO 4: same, for the software timer task. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configMINIMAL_STACK_SIZE];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configMINIMAL_STACK_SIZE;
}
