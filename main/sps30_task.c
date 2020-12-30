#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "sps30.h"
#include "sensirion_arch_config.h"
#include "sensirion_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor_task.h"
#include <string.h>

static const char TAG[] = "sps30-task";

static char sps30_device_id[32];
static char sps30_buffer[256];
static sensor_reporter_t sps30_reporter;

void sps30_task_run(void *arg)
{
  struct sps30_measurement m;
  char serial[SPS30_MAX_SERIAL_LEN];
  const uint8_t AUTO_CLEAN_DAYS = 4;
  int16_t ret;

  ESP_LOGD(TAG, "sps30_task_run started");
  while (sensirion_uart_open() != 0) {
    ESP_LOGI(TAG, "UART init failed");
    sensirion_sleep_usec(600000000); /* sleep for 10min */
  }

  /* Busy loop for initialization, because the main loop does not work without
   * a sensor.
   */
  while (sps30_probe() != 0) {
    ESP_LOGW(TAG, "SPS30 sensor probing failed");
    vTaskDelay(3600000 / portTICK_PERIOD_MS);
  }
  ESP_LOGI(TAG, "SPS30 sensor probing successful");

  struct sps30_version_information version_information;
  ret = sps30_read_version(&version_information);
  if (ret) {
      ESP_LOGI(TAG, "error %d reading version information\n", ret);
  } else {
      ESP_LOGI(TAG, "FW: %u.%u HW: %u, SHDLC: %u.%u\n",
             version_information.firmware_major,
             version_information.firmware_minor,
             version_information.hardware_revision,
             version_information.shdlc_major,
             version_information.shdlc_minor);
  }

  ret = sps30_get_serial(serial);
  if (ret)
      ESP_LOGI(TAG, "error %d reading serial\n", ret);
  else
      ESP_LOGI(TAG, "SPS30 Serial: %s\n", serial);

  ret = sps30_set_fan_auto_cleaning_interval_days(AUTO_CLEAN_DAYS);
  if (ret)
      ESP_LOGE(TAG, "error %d setting the auto-clean interval\n", ret);

  while (1) {
    ret = sps30_start_measurement();
    if (ret < 0) {
        ESP_LOGE(TAG, "error starting measurement");
    }

    ESP_LOGI(TAG, "measurements started");
    sensirion_sleep_usec(5000000); /* sleep for 5s */

    for (int i = 0; i < 10; ++i) {

        ret = sps30_read_measurement(&m);
        if (ret < 0) {
            ESP_LOGI(TAG, "error reading measurement");
        } else {
            if (SPS30_IS_ERR_STATE(ret)) {
                ESP_LOGI(TAG, 
                    "Chip state: %u - measurements may not be accurate\n",
                    SPS30_GET_ERR_STATE(ret));
            }

            ESP_LOGI(TAG, "measured values:\n"
                   "\t%0.2f pm1.0\n"
                   "\t%0.2f pm2.5\n"
                   "\t%0.2f pm4.0\n"
                   "\t%0.2f pm10.0\n"
                   "\t%0.2f nc0.5\n"
                   "\t%0.2f nc1.0\n"
                   "\t%0.2f nc2.5\n"
                   "\t%0.2f nc4.5\n"
                   "\t%0.2f nc10.0\n"
                   "\t%0.2f typical particle size\n\n",
                   m.mc_1p0, m.mc_2p5, m.mc_4p0, m.mc_10p0, m.nc_0p5,
                   m.nc_1p0, m.nc_2p5, m.nc_4p0, m.nc_10p0,
                   m.typical_particle_size);
            if (i >= 2) // take second value
            {
              // print data with json format
              sprintf(sps30_buffer, "{"
                "\"type\": \"PM\","
                "\"pm1.0\": %0.2f,"
                "\"pm2.5\": %0.2f,"
                "\"pm4.0\": %0.2f,"
                "\"pm10.0\": %0.2f,"
                "\"nc0.5\": %0.2f,"
                "\"nc1.0\": %0.2f,"
                "\"nc2.5\": %0.2f,"
                "\"nc4.5\": %0.2f,"
                "\"nc10.0\": %0.2f"
                "}",
                m.mc_1p0, m.mc_2p5, m.mc_4p0, m.mc_10p0, m.nc_0p5,
                m.nc_1p0, m.nc_2p5, m.nc_4p0, m.nc_10p0
                );
              sps30_reporter(sps30_device_id, sps30_buffer);
              break;
            }
        }
        sensirion_sleep_usec(2000000); /* sleep for 5s */
    }

    /* Stop measurement for 1min to preserve power. Also enter sleep mode
     * if the firmware version is >=2.0.
     */
    ret = sps30_stop_measurement();
    if (ret) {
        ESP_LOGE(TAG, "Stopping measurement failed");
    }

    if (version_information.firmware_major >= 2) {
        ret = sps30_sleep();
        if (ret) {
            ESP_LOGE(TAG, "Entering sleep failed");
        }
    }

    ESP_LOGI(TAG, "No measurements for 1 minute");
    sensirion_sleep_usec(1000000 * 60);

    if (version_information.firmware_major >= 2) {
        ret = sps30_wake_up();
        if (ret) {
            ESP_LOGE(TAG, "Error %i waking up sensor\n", ret);
        }
    }
  }

  if (sensirion_uart_close() != 0)
    ESP_LOGE(TAG, "failed to close UART");
}

void sps30_task(const char* device_id, sensor_reporter_t reporter)
{
  strncpy(sps30_device_id, device_id, sizeof(sps30_device_id) - 1);
  sps30_reporter = reporter;
  xTaskCreate(sps30_task_run, "sps30_task_run", 1024 * 6, (void *)0, 10, NULL);
}

