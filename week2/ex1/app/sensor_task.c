/* Week 2 / Ex1 — reads MPU6050 over I2C1 every SAMPLE_PERIOD_MS, pushes a
 * raw_sample_t into queue_raw.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"
extern I2C_HandleTypeDef hi2c1;

#define MPU6050_ADDR       (0x68U << 1)

#define REG_WHO_AM_I       0x75U
#define REG_PWR_MGMT_1     0x6BU
#define REG_ACCEL_XOUT_H   0x3BU

volatile uint32_t sensor_init_failed = 0;
volatile uint32_t sensor_i2c_errors = 0;
volatile uint32_t sensor_queue_drops = 0;
void sensor_task(void *argument){
    (void)argument;
    uint32_t seq = 0;
    uint8_t id = 0;
    uint8_t power = 0; 

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, REG_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &id, 1, 100);

    if (status != HAL_OK || id != 0x68U) {
        sensor_init_failed = 1;
        vTaskDelete(NULL);
        return;
    }

    /* I2C1 is initialized by main; clear SLEEP to wake the sensor. */
    status = HAL_I2C_Mem_Write( &hi2c1,  MPU6050_ADDR, REG_PWR_MGMT_1, I2C_MEMADD_SIZE_8BIT, &power, 1, 100);
    if (status != HAL_OK) {
        sensor_init_failed = 1;
        vTaskDelete(NULL);
        return;
    }

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        raw_sample_t s;
        s.seq = seq++;
        uint8_t data[6];
        /* TODO 2: HAL_I2C_Mem_Read the accel registers into s.ax/ay/az. */

        /* TODO 3: xQueueSend(queue_raw, &s, <timeout>) — pick and justify the
         *         timeout in PREDICTION.md question 3/4. What happens to this
         *         sample if the queue is full and the timeout expires? */
        status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR,  REG_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, data, sizeof(data), 10);
        if (status == HAL_OK) {
            s.ax = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
            s.ay = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
            s.az = (int16_t)(((uint16_t)data[4] << 8) | data[5]);

            if (xQueueSend(queue_raw, &s, portMAX_DELAY) != pdPASS) {
                sensor_queue_drops++;
            }
        } else {
            sensor_i2c_errors++;
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
