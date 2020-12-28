#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sensor_task.h"
#include "cli.h"
#include "cli_proc.h"
#include "wifi_conn.h"
#include "cfg.h"

#define SUPPORT_SHT3x 1
#define SUPPORT_SPS30 1

static const char TAG[] = "main";
static CFG app_cfg;

void sht3x_task(void);
void sps30_task(void);

static void show_uptime()
{
  uint32_t secs = (uint32_t)(esp_timer_get_time() / 1000000);
  uint32_t day, hh, mm, ss;

  hh = secs / 3600;
  mm = (secs % 3600) / 60;
  ss = secs % 60;
  day = hh / 24;
  hh = hh % 24;
  printf("uptime: %d day %02d:%02d:%02d\n", day, hh, mm, ss);
}

static void show_version()
{
  esp_chip_info_t info;
  esp_chip_info(&info);
  printf("IDF Version:%s\n", esp_get_idf_version());
  printf("Chip info:\n");
  printf(" model:%s\n", info.model == CHIP_ESP32 ? "ESP32" : "Unknow");
  printf(" cores:%d\n", info.cores);
  printf(" feature:%s%s%s%s%d%s\n",
         info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
         info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
         info.features & CHIP_FEATURE_BT ? "/BT" : "",
         info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
         spi_flash_get_chip_size() / (1024 * 1024), " MB");
  printf(" revision number:%d\n", info.revision);
}

static void _sys(uint32_t ac, char *av[])
{
  const char* cmd = av[1];
  
  printf("\n");
  if (ac < 2)
  {
    printf("%s [info|stat|show_mem|reboot]\n", av[0]);
    return;
  }

  if (!strcmp(cmd, "info"))
  {
    show_version();
  }
  else if (!strcmp(cmd, "stat"))
  {
    show_uptime();
  }
  else if (!strcmp(cmd, "show_mem"))
  {
    printf("heap: %d\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf("free: %d\n", esp_get_free_heap_size());
  }
  else if (!strcmp(cmd, "reboot"))
  {
    esp_restart();
  }
  else
  {
    printf("unknown '%s'\n", cmd);
  }
}

static void _cfg(uint32_t ac, char *av[])
{
  const char* cmd = av[1];
  
  printf("\n");
  if (ac < 2)
  {
    printf("%s [show|set_wifi|erase]\n", av[0]);
    return;
  }

  if (!strcmp(cmd, "show"))
  {
    printf(
      "wifi.ssid: %s\n"
      "wifi.password: %s\n",
      app_cfg.wifi_ssid,
      app_cfg.wifi_password
      );
    wifi_info_print();
  }
  else if (!strcmp(cmd, "set_wifi"))
  {
    if (ac < 3)
    {
      printf("%s set_wifi <ssid> [passwod]\n", av[0]);
    }
    else
    {
      const char *password = ac >= 4 ? av[3] : "";
      strncpy(app_cfg.wifi_ssid, av[2], sizeof(app_cfg.wifi_ssid)-1);
      strncpy(app_cfg.wifi_password, password, sizeof(app_cfg.wifi_password)-1);
      cfg_write(&app_cfg);
    }
  }
  else if (!strcmp(cmd, "erase"))
  {
    cfg_erase();
    cfg_get_default(&app_cfg);
    printf("cfg erased\n");
  }
  else
  {
    printf("unknown '%s'\n", cmd);
  }
}

static void register_commands(void)
{
  static cli_record_t sys;
  static cli_record_t cfg;

  cli_mkcmd("sys", _sys, NULL, &sys);
  cli_mkcmd("cfg", _cfg, NULL, &cfg);
}

static void cmd_line_init()
{
  static const cli_init_data_t init_data = { "" };

  cli_init(&init_data);
  register_commands();
  cliproc_init();
}

static void cmd_output(const char* data, uint16_t len)
{
  fwrite(data, 1, len, stdout);
}

static void cmd_init(int is_nonblock) {
  int flags = fcntl(STDIN_FILENO, F_GETFL);
  if (is_nonblock)
  {
    // set console non block mode
    flags |= O_NONBLOCK;
  }
  else
  {
    flags &= ~O_NONBLOCK;
  }
  int res = fcntl(STDIN_FILENO, F_SETFL, flags);
  if (res != 0) {
    ESP_LOGE(TAG, "fcntl error");
  } else {
    ESP_LOGI(TAG, "console non block mode set");
  }

  cmd_line_init();
}

static void initialize_nvs(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "nvs_flash_init error: %d", err);
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
}

static void main_task_run(void* pvParameter)
{
  char c;
  int cb;
  cmd_init(0);

  while (1)
  {
    cb = fread(&c, 1, 1, stdin);
    if (cb > 0) {
      cliproc_push_key(c, cmd_output);
    }
    else
    {
      //ESP_LOGI(TAG, "fread error");
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

static void wifi_setup(void)
{
  wifi_init();
  if (app_cfg.wifi_ssid[0] != 0)
  {
    wifi_connect(app_cfg.wifi_ssid, app_cfg.wifi_password);
  }
  else
  {
    ESP_LOGW(TAG, "not start wifi: ssid not configured");
  }
}

static void app_init(void)
{
  initialize_nvs();
  cfg_init();
  if (cfg_read(&app_cfg) < 0)
    cfg_get_default(&app_cfg);
}

void app_main(void)
{
  app_init();
  wifi_setup();

#if SUPPORT_SHT3x
  sht3x_task();
#endif
#if SUPPORT_SPS30
  sps30_task();
#endif
  xTaskCreate(main_task_run, "main_task", 1024 * 8, (void *)0, 10, NULL);
}

