/* Week 3 / Ex4 -- supervisor task: waits for all monitored tasks to check in
 * via an event group every cycle; refreshes IWDG only if they all did.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "main.h"
#include "params.h"
#include "supervisor.h"

#include <stdint.h>

extern IWDG_HandleTypeDef hiwdg;

EventGroupHandle_t supervisor_events;

#define ALL_BITS ((1u << SUPERVISOR_BITS) - 1u)

#define SUPERVISOR_BIT_SELF    (1u << 2)
#define SUPERVISOR_WAIT_MS     (IWDG_TIMEOUT_MS / 2U)
#define IWDG_LSI_NOMINAL_HZ    40000UL
#define IWDG_PRESCALER_VALUE   32UL

#if SUPERVISOR_BITS != 3
#error "This implementation requires SUPERVISOR_BITS == 3"
#endif

void supervisor_init(void)
{
    /* TODO 1: supervisor_events = xEventGroupCreate(); check non-NULL.
     *
     * TODO 2: HAL IWDG init here (IWDG_TIMEOUT_MS -> prescaler + reload per
     *         RM0008 IWDG chapter; IWDG runs off LSI, ~40 kHz, compute the
     *         reload value and show the computation in REPORT.md). */

    supervisor_events = xEventGroupCreate();
    configASSERT(supervisor_events != NULL);

    if (supervisor_events == NULL) {
        Error_Handler();
        return;
    }

    const uint32_t counter_ticks = (uint32_t)(((uint64_t)IWDG_TIMEOUT_MS * IWDG_LSI_NOMINAL_HZ)
                   / (1000UL * IWDG_PRESCALER_VALUE));

    configASSERT(counter_ticks >= 1U);
    configASSERT(counter_ticks <= 4096U);

    if ((counter_ticks < 1U) || (counter_ticks > 4096U)) {
        Error_Handler();
        return;
    }

    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = counter_ticks - 1U;

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
        return;
    }
}

void supervisor_task(void *argument)
{
    (void)argument;

    const TickType_t wait_ticks = pdMS_TO_TICKS(SUPERVISOR_WAIT_MS);

    configASSERT(supervisor_events != NULL);
    configASSERT(wait_ticks > 0);

    for (;;) {
        /* TODO 3: xEventGroupWaitBits(supervisor_events, ALL_BITS, pdTRUE,
         *         pdTRUE, <timeout>) -- clear-on-exit, wait-for-all. Pick a
         *         timeout tighter than IWDG_TIMEOUT_MS but loose enough that
         *         normal scheduling jitter does not cause a false trip;
         *         justify the number in REPORT.md.
         *
         * TODO 4: if the wait actually collected ALL_BITS (check the return
         *         value against ALL_BITS, do not assume success just because
         *         the call returned), HAL_IWDG_Refresh(&hiwdg). Otherwise do
         *         NOTHING -- that is what lets IWDG_TIMEOUT_MS expire and
         *         reset the board when a task stops checking in. */

        xEventGroupSetBits(supervisor_events, SUPERVISOR_BIT_SELF);

        EventBits_t bits = xEventGroupWaitBits(
            supervisor_events,
            ALL_BITS,
            pdTRUE,
            pdTRUE,
            wait_ticks
        );

        if ((bits & ALL_BITS) == ALL_BITS) {
            if (HAL_IWDG_Refresh(&hiwdg) != HAL_OK) {
                Error_Handler();
            }
        }
    }
}