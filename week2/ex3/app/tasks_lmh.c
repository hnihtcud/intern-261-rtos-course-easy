/* Week 2 / Ex3 — priority inversion vs priority inheritance.
 *
 * task_low  (lowest prio)  holds the shared resource for LOW_HOLD_MS.
 * task_mid  (middle prio)  never touches the resource; pure CPU-bound work,
 *                          its only job is to be ready and preemptible so it
 *                          can steal CPU from task_low while task_low holds
 *                          the resource task_high is waiting for.
 * task_high (highest prio) needs the resource every HIGH_PERIOD_MS and is
 *                          the one whose block time we measure.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "main.h"
#include "params.h"
#include "tasks_lmh.h"

extern TickType_t experiment_start_tick;

#define LOW_START_MS         20U
#define MID_START_MS         26U
#define SCENARIO_PERIOD_MS  100U

#define LOW_PIN   GPIO_PIN_1
#define MID_PIN   GPIO_PIN_2
#define HIGH_PIN  GPIO_PIN_3

static SemaphoreHandle_t s_resource;

static volatile uint32_t s_shared_resource = 0;
static uint32_t s_iterations_per_ms;

volatile uint32_t high_block_last_cycles = 0;
volatile uint32_t high_block_max_cycles = 0;
volatile uint32_t high_response_last_cycles = 0;
volatile uint32_t high_request_count = 0;
volatile uint32_t high_deadline_misses = 0;
volatile UBaseType_t low_priority_before_give = 0;

static __attribute__((noinline))
void burn_iterations(uint32_t count)
{
    for (volatile uint32_t i = 0; i < count; i++) {
        __NOP();
    }
}
static void cpu_work_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    const uint32_t test_iterations = 100000U;

    uint32_t start = DWT->CYCCNT;
    burn_iterations(test_iterations);
    uint32_t elapsed = (uint32_t)(DWT->CYCCNT - start);

    configASSERT(elapsed > 0U);

    /*
     * iterations/ms =
     * test_iterations × cycles_per_ms / measured_cycles
     */
    s_iterations_per_ms = (uint32_t)(
        ((uint64_t)test_iterations * (SystemCoreClock / 1000U))
        / elapsed
    );

    configASSERT(s_iterations_per_ms > 0U);
}

static void cpu_work_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        burn_iterations(s_iterations_per_ms);
    }
}

void resource_init(void)
{
    HAL_GPIO_WritePin(GPIOA, LOW_PIN | MID_PIN | HIGH_PIN, GPIO_PIN_RESET);
    cpu_work_init();

#if RESOURCE_MODE == 0
    /* TODO 1: xSemaphoreCreateBinary(), then give it once so it starts
     *         "available" (binary semaphores start empty). */
    s_resource = xSemaphoreCreateBinary();
    configASSERT(s_resource != NULL);

    BaseType_t result = xSemaphoreGive(s_resource);
    configASSERT(result == pdTRUE);
#else
    /* TODO 2: xSemaphoreCreateMutex(). */
    s_resource = xSemaphoreCreateMutex();
    configASSERT(s_resource != NULL);
#endif
}

void task_low(void *argument)
{
    (void)argument;
    TickType_t last_wake = experiment_start_tick;

    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LOW_START_MS));
    for (;;) {
        /* TODO 3: take s_resource (portMAX_DELAY), toggle a GPIO marking
         *         "resource held", busy-wait LOW_HOLD_MS using the ex2 DWT
         *         helper or a HAL tick-based wait, lower the GPIO, give
         *         s_resource back. Then yield/delay briefly before looping —
         *         this task must not monopolize the CPU outside the hold. */
        BaseType_t taken = xSemaphoreTake(s_resource, portMAX_DELAY);
        configASSERT(taken == pdTRUE);
        HAL_GPIO_WritePin(GPIOA, LOW_PIN, GPIO_PIN_SET);
        s_shared_resource++;
        cpu_work_ms(LOW_HOLD_MS);

        low_priority_before_give = uxTaskPriorityGet(NULL);
        HAL_GPIO_WritePin(GPIOA, LOW_PIN, GPIO_PIN_RESET);
        BaseType_t given = xSemaphoreGive(s_resource);
        configASSERT(given == pdTRUE);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SCENARIO_PERIOD_MS));
    }
}

void task_mid(void *argument)
{
    (void)argument;
    TickType_t last_wake = experiment_start_tick;
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MID_START_MS));
    for (;;) {
        /* TODO 4: toggle a GPIO, busy-wait MID_WORK_MS, lower the GPIO, then
         *         vTaskDelay a short period so it keeps becoming Ready
         *         periodically rather than running once and blocking
         *         forever. This task never touches s_resource. */
        HAL_GPIO_WritePin(GPIOA, MID_PIN, GPIO_PIN_SET);

        cpu_work_ms(MID_WORK_MS);

        HAL_GPIO_WritePin(GPIOA, MID_PIN, GPIO_PIN_RESET);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SCENARIO_PERIOD_MS));
    }
}

void task_high(void *argument)
{
    (void)argument;
    TickType_t last_wake = experiment_start_tick;

    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(HIGH_PERIOD_MS));
    for (;;) {
        /* TODO 5: toggle a GPIO high (marks "task_high wants the resource"),
         *         take s_resource (portMAX_DELAY), toggle a second GPIO
         *         transition (marks "task_high got it"), do a small amount
         *         of work, give s_resource back, lower the first GPIO.
         *         The gap between the two GPIO edges on the logic analyzer
         *         is your block time t1 - t0. */
        HAL_GPIO_WritePin(GPIOA, HIGH_PIN, GPIO_PIN_SET);
        uint32_t request_cycles = DWT->CYCCNT;
        BaseType_t taken = xSemaphoreTake(s_resource, portMAX_DELAY);
        configASSERT(taken == pdTRUE);
        uint32_t acquired_cycles = DWT->CYCCNT;
        HAL_GPIO_WritePin(GPIOA, HIGH_PIN, GPIO_PIN_RESET);
        
        s_shared_resource++;

        BaseType_t given = xSemaphoreGive(s_resource);
        configASSERT(given == pdTRUE);

        uint32_t finished_cycles = DWT->CYCCNT;

        uint32_t block_cycles = (uint32_t)(acquired_cycles - request_cycles);

        uint32_t response_cycles = (uint32_t)(finished_cycles - request_cycles);

        high_block_last_cycles = block_cycles;
        high_response_last_cycles = response_cycles;

        if (block_cycles > high_block_max_cycles) {
            high_block_max_cycles = block_cycles;
        }

        high_request_count++;

        uint32_t deadline_cycles = (SystemCoreClock / 1000U) * DEADLINE_MS;

        if (response_cycles > deadline_cycles) {
            high_deadline_misses++;
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(HIGH_PERIOD_MS));
    }
}
