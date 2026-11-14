/* Week 3 / Ex4 -- task_alpha and task_beta share two resources, protected by
 * two mutexes, mimicking a sensor register + a shared UART buffer that both
 * tasks occasionally need at once. This file is provided complete, not a
 * skeleton: build and run it exactly as given first and use
 * uxTaskGetSystemState() to find out why it stops making progress, before
 * you change anything in here. Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "main.h"
#include "params.h"
#include "deadlock_demo.h"
#include "supervisor.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

static void deadlock_monitor_task(void *argument);
static SemaphoreHandle_t s_mutex_a;
static SemaphoreHandle_t s_mutex_b;

volatile uint32_t alpha_step = 0;
volatile uint32_t beta_step = 0;
volatile uint32_t alpha_cycles = 0;
volatile uint32_t beta_cycles = 0;

void deadlock_demo_init(void)
{
    s_mutex_a = xSemaphoreCreateMutex();
    s_mutex_b = xSemaphoreCreateMutex();

    configASSERT(s_mutex_a != NULL);
    configASSERT(s_mutex_b != NULL);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

    BaseType_t result = xTaskCreate(
        deadlock_monitor_task,
        "dl_monitor",
        384,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    configASSERT(result == pdPASS);

    if (result != pdPASS) {
        Error_Handler();
    }
}

void task_alpha(void *argument)
{
    (void)argument;

    for (;;) {
        alpha_step = 1;
        xSemaphoreTake(s_mutex_a, portMAX_DELAY);
        alpha_step = 2;

        vTaskDelay(pdMS_TO_TICKS(5));
        alpha_step = 3;
        xSemaphoreTake(s_mutex_b, portMAX_DELAY);
        alpha_step = 4;

        /* ... shared work using both resources ... */

        xSemaphoreGive(s_mutex_b);
        xSemaphoreGive(s_mutex_a);

        alpha_step = 5;
        alpha_cycles++;

        /* TODO (step 4 of README, after the deadlock is fixed): call
         *       supervisor_checkin(SUPERVISOR_BIT_ALPHA) here, AFTER both
         *       mutexes are released -- that is what makes the check-in mean
         *       "this task completed a full cycle", not just "this task is
         *       still being scheduled". */
        supervisor_checkin(SUPERVISOR_BIT_ALPHA);

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void task_beta(void *argument)
{
    (void)argument;

    for (;;) {
        beta_step = 1;
        xSemaphoreTake(s_mutex_b, portMAX_DELAY);
        beta_step = 2;

        vTaskDelay(pdMS_TO_TICKS(5));

        beta_step = 3;
        xSemaphoreTake(s_mutex_a, portMAX_DELAY);
        beta_step = 4;

        /* ... shared work using both resources ... */

        xSemaphoreGive(s_mutex_a);
        xSemaphoreGive(s_mutex_b);

        beta_step = 5;
        beta_cycles++;

        /* TODO (step 4 of README, after the deadlock is fixed): call
         *       supervisor_checkin(SUPERVISOR_BIT_BETA) here, AFTER both
         *       mutexes are released. */
        supervisor_checkin(SUPERVISOR_BIT_BETA);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void deadlock_monitor_task(void *argument)
{
    (void)argument;

    static TaskStatus_t task_states[8];
    char message[96];

    TickType_t poll_ticks = pdMS_TO_TICKS(10);
    if (poll_ticks == 0) {
        poll_ticks = 1;
    }

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

    for (;;) {
        vTaskDelay(poll_ticks);

        if ((alpha_step != 3U) || (beta_step != 3U)) {
            continue;
        }

        UBaseType_t count = uxTaskGetSystemState(
            task_states,
            sizeof(task_states) / sizeof(task_states[0]),
            NULL
        );

        BaseType_t alpha_blocked = pdFALSE;
        BaseType_t beta_blocked = pdFALSE;

        for (UBaseType_t i = 0; i < count; i++) {
            if (strcmp(task_states[i].pcTaskName, "alpha") == 0) {
                alpha_blocked =
                    (task_states[i].eCurrentState == eBlocked);
            }

            if (strcmp(task_states[i].pcTaskName, "beta") == 0) {
                beta_blocked =
                    (task_states[i].eCurrentState == eBlocked);
            }
        }

        if ((alpha_blocked == pdTRUE) &&
            (beta_blocked == pdTRUE) &&
            (alpha_step == 3U) &&
            (beta_step == 3U)) {

            /*
             * GPIO MARKER:
             * HIGH = Deadlock detected
             * LOW  = MCU reset by IWDG (external pull-down required)
             */
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);

            snprintf(
                message,
                sizeof(message),
                "DEADLOCK DETECTED\r\n"
            );

            HAL_UART_Transmit(
                &huart1,
                (uint8_t *)message,
                (uint16_t)strlen(message),
                100
            );

            for (;;) {
                vTaskDelay(poll_ticks);
            }
        }
    }
}