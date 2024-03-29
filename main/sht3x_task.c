#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "sht3x.h"
#include "sensor_task.h"
#include <string.h>

#define SLOW_SENSOR_FETCH 1

#define SENSOR_RESET_PIN 4

static const char TAG[] = "sht3x-task";

#define I2C_MASTER_SCL_IO GPIO_NUM_22
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_NUM    I2C_NUM_0
#define I2C_MASTER_FREQ_HZ I2C_FREQ_400K
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

#if SUPPORT_SENSOR_RESET
void sht3x_reset()
{
  printf("GPIO %d low\n", SENSOR_RESET_PIN);
  gpio_set_level(SENSOR_RESET_PIN, 0);
  vTaskDelay(5 / portTICK_PERIOD_MS);
  printf("high\n");
  gpio_set_level(SENSOR_RESET_PIN, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  while ((sht3x_sensor = sht3x_init_sensor (I2C_MASTER_NUM, SHT3x_ADDR_1)) == NULL)
  {
    ESP_LOGE(TAG, "sht3x_init_sensor failed\n");
    vTaskDelay(3600000 / portTICK_PERIOD_MS);
  }
  printf("sht3x_init_sensor ok\n");
  sht3x_start_measurement (sht3x_sensor, sht3x_periodic_1mps, sht3x_high);
  vTaskDelay(100 / portTICK_PERIOD_MS);
}
#endif

void sht3x_task_run(void *arg)
{
  float temperature, humidity;
  char report_buf[128];

  ESP_LOGI(TAG, "sht3x_task_run started");
  while ((sht3x_sensor = sht3x_init_sensor (I2C_MASTER_NUM, SHT3x_ADDR_1)) == NULL)
  {
    ESP_LOGE(TAG, "sht3x_init_sensor failed\n");
    vTaskDelay(3600000 / portTICK_PERIOD_MS);
  }

  uint64_t last_measure_ms, diff_ms, sleep_ms;
#if SLOW_SENSOR_FETCH
  sht3x_start_measurement (sht3x_sensor, sht3x_periodic_2mps, sht3x_high);
  sleep_ms = 60000;
#else
  // Start periodic measurements with 1 measurement per second.
  sht3x_start_measurement (sht3x_sensor, sht3x_periodic_1mps, sht3x_high);
  sleep_ms = 5000;
#endif

  // Wait until first measurement is ready (constant time of at least 30 ms
  // or the duration returned from *sht3x_get_measurement_duration*).
  vTaskDelay (sht3x_get_measurement_duration(sht3x_high));

  while (1) 
  {
    last_measure_ms = esp_timer_get_time() / 1000;
    // Get the values and do something with them.
    if (sht3x_get_results (sht3x_sensor, &temperature, &humidity))
    {
      printf("%.3f SHT3x Sensor: %.2f °C, %.2f %%\n", 
       (double)get_uptime_ms()*1e-3, temperature, humidity);
      sprintf(report_buf, "{"
        "\"type\": \"temp/humi\","
        "\"temp\": %0.2f,"
        "\"humi\": %0.2f,"
        "\"t\": %u"
        "}",
        temperature, humidity, (uint32_t)(esp_timer_get_time() / 1000000));
      sht3x_reporter(sht3x_device_id, report_buf);
    }
    else
    {
      ESP_LOGE(TAG, "failed to fetch temperature\n");
#if SUPPORT_SENSOR_RESET
      sht3x_reset();
#endif
    }

    diff_ms = esp_timer_get_time() / 1000 - last_measure_ms;
    if (diff_ms < sleep_ms)
    {
      vTaskDelay((sleep_ms - diff_ms) / portTICK_PERIOD_MS);
    }
  }
}

void sht3x_task(const char* device_id, sensor_reporter_t reporter)
{
#if SUPPORT_SENSOR_RESET
  gpio_reset_pin(SENSOR_RESET_PIN);
  gpio_set_direction(SENSOR_RESET_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(SENSOR_RESET_PIN, 1);
#endif
  strncpy(sht3x_device_id, device_id, sizeof(sht3x_device_id) - 1);
  sht3x_reporter = reporter;
  ESP_ERROR_CHECK(i2c_master_init());
  xTaskCreate(sht3x_task_run, "sht3x_task_run", 1024 * 7, (void *)0, 10, NULL);
}

