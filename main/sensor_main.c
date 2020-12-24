#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "sensor_task.h"

#define SUPPORT_SHT3x 1
#define SUPPORT_SPS30 1

void sht3x_task(void);
void sps30_task(void);

void app_main(void)
{
#if SUPPORT_SHT3x
  sht3x_task();
#endif
#if SUPPORT_SPS30
  sps30_task();
#endif
}

