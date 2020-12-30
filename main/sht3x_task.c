#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "sht3x.h"
#include "sensor_task.h"
#include <string.h>

#define SLOW_SENSOR_FETCH 1

static const char TAG[] = "sht3x-task";

#define I2C_MASTER_SCL_IO GPIO_NUM_22
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_NUM    I2C_NUM_0
#define I2C_MASTER_FREQ_HZ I2C_FREQ_500K
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

static sht3x_sensor_t* sht3x_sensor;
static char sht3x_device_id[32];
static sensor_reporter_t sht3x_reporter;

static uint64_t get_uptime_ms()
{
  return (uint64_t)esp_timer_get_time() / 1000;
}

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void sht3x_task_run(void *arg)
{
  float temperature, humidity;
  char report_buf[128];

  ESP_LOGD(TAG, "sht3x_task_run started");
  while ((sht3x_sensor = sht3x_init_sensor (I2C_MASTER_NUM, SHT3x_ADDR_1)) == NULL)
  {
    ESP_LOGE(TAG, "sht3x_init_sensor failed\n");
    vTaskDelay(3600000 / portTICK_PERIOD_MS);
  }

#if SLOW_SENSOR_FETCH
  sht3x_start_measurement (sht3x_sensor, sht3x_periodic_2mps, sht3x_high);
#else
  // Start periodic measurements with 1 measurement per second.
  sht3x_start_measurement (sht3x_sensor, sht3x_periodic_1mps, sht3x_high);
#endif

  // Wait until first measurement is ready (constant time of at least 30 ms
  // or the duration returned from *sht3x_get_measurement_duration*).
  vTaskDelay (sht3x_get_measurement_duration(sht3x_high));

  TickType_t last_wakeup = xTaskGetTickCount();
  
  while (1) 
  {
    // Get the values and do something with them.
    if (sht3x_get_results (sht3x_sensor, &temperature, &humidity))
    {
      printf("%.3f SHT3x Sensor: %.2f °C, %.2f %%\n", 
       (double)get_uptime_ms()*1e-3, temperature, humidity);
      sprintf(report_buf, "{"
        "\"type\": \"temp/humi\","
        "\"temp\": %0.2f,"
        "\"humi\": %0.2f"
        "}",
        temperature, humidity);
      sht3x_reporter(sht3x_device_id, report_buf);
    }
    else
    {
      ESP_LOGW(TAG, "failed to fetch temperature\n");
    }

#if SLOW_SENSOR_FETCH
    vTaskDelayUntil(&last_wakeup, 30000 / portTICK_PERIOD_MS);
#else
    // Wait until 2 seconds (cycle time) are over.
    vTaskDelayUntil(&last_wakeup, 2000 / portTICK_PERIOD_MS);
#endif
  }
}

void sht3x_task(const char* device_id, sensor_reporter_t reporter)
{
  strncpy(sht3x_device_id, device_id, sizeof(sht3x_device_id) - 1);
  sht3x_reporter = reporter;
  ESP_ERROR_CHECK(i2c_master_init());
  xTaskCreate(sht3x_task_run, "sht3x_task_run", 1024 * 6, (void *)0, 10, NULL);
}

