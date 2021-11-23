#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_console.h"
#include "esp_event.h"
#include "esp_vfs_dev.h"
#include "esp_task_wdt.h"
#include "esp_http_client.h"
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
#define SUPPORT_MQTT 1
#define SUPPORT_WATCHDOG 1

static const char TAG[] = "main";
static CFG app_cfg;
static char device_mac_str[13];
static char device_ver_str[] = { __DATE__ " (1.0)" };
static volatile uint32_t last_publish_sec = 0;
static size_t http_num_error = 0;

#define TEST_PIN 11

#define TWDT_TIMEOUT_S          3
#define CHECK_ERROR_CODE(returned, expected) ({                        \
            if(returned != expected){                                  \
                printf("TWDT ERROR\n");                                \
                abort();                                               \
            }                                                          \
})

#if 0
static void test_gpio_init(void)
{
  // set GPIO output
  gpio_config_t io_conf;
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = (1ULL<<TEST_PIN);
  //disable pull-down mode
  io_conf.pull_down_en = 0;
  //disable pull-up mode
  io_conf.pull_up_en = 0;
  //configure GPIO with the given settings
  printf("TEST gpio init.. %d\n", TEST_PIN);
  gpio_config(&io_conf);
  printf("TEST gpio init ok: %d\n", TEST_PIN);
  //gpio_set_level(TEST_PIN, 1);
}
#endif

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
    printf("%s [stat|reboot|sht3x_reset]\n", av[0]);
    return;
  }

  if (!strcmp(cmd, "stat"))
  {
    printf("Firmware built at " __DATE__ " " __TIME__ "\n");
    show_version();
    show_uptime();
    printf("free: %d (min heap: %d)\n", esp_get_free_heap_size(), heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf("WIFI MAC: %s\n", device_mac_str);
#if SUPPORT_WATCHDOG
    printf("WATCHDOG: supported\n");
#endif
    wifi_info_print();
    mqtt_task_status_print();
  }
  else if (!strcmp(cmd, "reboot"))
  {
    esp_restart();
  }
  else if (!strcmp(cmd, "sht3x_reset"))
  {
#if SUPPORT_SENSOR_RESET
    sht3x_reset();
#else
    printf("command not supported\n");
#endif
  }
  /*
  else if (!strcmp(cmd, "test"))
  {
    if (atoi(av[2]))
    {
      printf("GPIO(%d) high\n", TEST_PIN);
      gpio_set_level(TEST_PIN, 1);
      printf("GPIO ok\n");
    }
    else
    {
      printf("GPIO(%d) low\n", TEST_PIN);
      gpio_set_level(TEST_PIN, 0);
      printf("GPIO ok\n");
    }
  }
  */
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
    printf("%s [show|set_wifi|push_type|set_http|set_mqtt|erase]\n", av[0]);
    return;
  }

  if (!strcmp(cmd, "show"))
  {
    printf("Firmware version: %s\n", device_ver_str);
    printf(
      "wifi.ssid: '%s'\n"
      "wifi.password: '%s'\n",
      app_cfg.wifi_ssid,
      app_cfg.wifi_password
      );
    printf(
      "puth.type: '%s'\n",
      app_cfg.push_type == 'h' ? "http" : "mqtt"
      );
    printf(
      "http.uri: '%s'\n",
      app_cfg.http_cfg.uri
      );
    printf(
      "mqtt.uri: '%s'\n"
      "mqtt.client_id: '%s'\n"
      "mqtt.username: '%s'\n"
      "mqtt.password: '%s'\n",
      app_cfg.mqtt_cfg.uri,
      app_cfg.mqtt_cfg.client_id,
      app_cfg.mqtt_cfg.username,
      app_cfg.mqtt_cfg.password
      );
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
  else if (!strcmp(cmd, "push_type"))
  {
    if (ac < 3)
    {
      printf("%s push_type <h|m>\n\t'h': http\n\t'm': mqtt\n", av[0]);
    }
    else
    {
      app_cfg.push_type = av[2][0];
      cfg_write(&app_cfg);
    }
  }
  else if (!strcmp(cmd, "set_http"))
  {
    if (ac < 3)
    {
      printf("%s set_http <uri>\n", av[0]);
    }
    else
    {
      strncpy(app_cfg.http_cfg.uri, av[2], sizeof(app_cfg.http_cfg.uri)-1);
      cfg_write(&app_cfg);
    }
  }
  else if (!strcmp(cmd, "set_mqtt"))
  {
    if (ac < 3)
    {
      printf(
        "%s set_mqtt <uri> [username] [password] [client_id]\n"
        " uri ex> mqtt://mqtt.eclipse.org\n", av[0]);
    }
    else
    {
      const char *username = ac >= 4 ? av[3] : "";
      const char *password = ac >= 5 ? av[4] : "";
      const char *client_id = ac >= 6 ? av[5] : device_mac_str;
      strncpy(app_cfg.mqtt_cfg.uri, av[2], sizeof(app_cfg.mqtt_cfg.uri)-1);
      strncpy(app_cfg.mqtt_cfg.client_id, client_id, sizeof(app_cfg.mqtt_cfg.client_id)-1);
      strncpy(app_cfg.mqtt_cfg.username, username, sizeof(app_cfg.mqtt_cfg.username)-1);
      strncpy(app_cfg.mqtt_cfg.password, password, sizeof(app_cfg.mqtt_cfg.password)-1);
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

static void _mqt(uint32_t ac, char *av[])
{
  const char* cmd = av[1];
  
  printf("\n");
  if (ac < 2)
  {
    printf("%s [publish]\n", av[0]);
    return;
  }

  if (!strcmp(cmd, "publish"))
  {
    if (ac < 3)
    {
      printf("%s set_wifi <data>\n", av[0]);
    }
    else
    {
      char buf[256] = {0, };
      uint32_t i;

      for (i = 2; i < ac; ++i)
      {
        strcat(buf, av[i]);
      }
      mqtt_task_publish(device_mac_str, buf);
    }
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
  static cli_record_t mqt;

  cli_mkcmd("sys", _sys, NULL, &sys);
  cli_mkcmd("cfg", _cfg, NULL, &cfg);
  cli_mkcmd("mqt", _mqt, NULL, &mqt);
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
  uint32_t curent_sec;

  cmd_init(0);

#if SUPPORT_WATCHDOG
    uint32_t watchdog_check_sec = (uint32_t)(esp_timer_get_time() / 1000000);
    //Subscribe this task to TWDT, then check if it is subscribed
    CHECK_ERROR_CODE(esp_task_wdt_add(NULL), ESP_OK);
    CHECK_ERROR_CODE(esp_task_wdt_status(NULL), ESP_OK);
#endif

  last_publish_sec = (uint32_t)(esp_timer_get_time() / 1000000);

  if (app_cfg.push_type == 'h')
  {
    if (app_cfg.http_cfg.uri[0] == 0)
    {
      ESP_LOGW(TAG, "push uri not configured");
    }
  }
  else
  {
#if SUPPORT_MQTT
    if (app_cfg.mqtt_cfg.uri[0] != 0)
    {
      // delay until WIFI is UP
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      mqtt_task(app_cfg.mqtt_cfg.uri, app_cfg.mqtt_cfg.client_id, app_cfg.mqtt_cfg.username, app_cfg.mqtt_cfg.password);
    }
    else
    {
      ESP_LOGW(TAG, "MQTT not started: mqtt not configured");
    }
#endif
  }
  while (1)
  {
    cb = fread(&c, 1, 1, stdin);
    if (cb > 0)
    {
      cliproc_push_key(c, cmd_output);
    }
    else
    {
      //ESP_LOGI(TAG, "fread error");
      vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    if (http_num_error >= 20)
    {
      ESP_LOGE(TAG, "restart at http failure.");
      vTaskDelay(50 / portTICK_PERIOD_MS);
      esp_restart();
    }

    if (mqtt_num_error >= 30)
    {
      ESP_LOGE(TAG, "restart at network failure.");
      vTaskDelay(50 / portTICK_PERIOD_MS);
      esp_restart();
    }

    curent_sec = (uint32_t)(esp_timer_get_time() / 1000000);
    if (curent_sec - last_publish_sec >= 120)
    {
      ESP_LOGE(TAG, "restart at sensor not working.");
      esp_restart();
    }

#if SUPPORT_WATCHDOG
    if (watchdog_check_sec != curent_sec) {
      watchdog_check_sec = curent_sec;
      //printf("watchdog_check_sec=%d\n", watchdog_check_sec);
      CHECK_ERROR_CODE(esp_task_wdt_reset(), ESP_OK);
    }
#endif
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
    ESP_LOGW(TAG, "WIFI not connected: ssid not configured");
  }
}

static void app_init(void)
{
  uint8_t device_mac[6];
  initialize_nvs();
  esp_read_mac(device_mac, ESP_MAC_WIFI_STA);
  sprintf(device_mac_str, "%02x%02x%02x%02x%02x%02x",
    device_mac[0],
    device_mac[1],
    device_mac[2],
    device_mac[3],
    device_mac[4],
    device_mac[5]
    );
  ESP_LOGI(TAG, "esp MAC: %s", device_mac_str);
  cfg_init();
  if (cfg_read(&app_cfg) < 0)
    cfg_get_default(&app_cfg);
}

static void http_publish(const char* device_id, const char* data)
{
  if (app_cfg.http_cfg.uri[0] == 0)
  {
    ESP_LOGW(TAG, "can't push sensor data, uri not configured");
    return;
  }

  char output_buffer[512];
  esp_err_t err;
  esp_http_client_config_t config = {
    .url = app_cfg.http_cfg.uri
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL)
  {
    ESP_LOGW(TAG, "esp_http_client_init failed");
    return;
  }
  size_t data_len = strlen(data);
  //esp_http_client_set_url(client, app_cfg.http_cfg.uri);
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  err = esp_http_client_open(client, data_len);
  if (err != ESP_OK)
  {
      ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
      http_num_error++;
  }
  else
  {
    int wlen = esp_http_client_write(client, data, data_len);
    if (wlen < 0) {
      ESP_LOGE(TAG, "Write failed");
      http_num_error++;
    }
    ESP_LOGI(TAG, "POST '%s'", app_cfg.http_cfg.uri);
    int data_read = esp_http_client_read_response(client, output_buffer, sizeof(output_buffer));
    if (data_read >= 0)
    {
      http_num_error = 0;
      //ESP_LOGI(TAG, "HTTP GET Status = %d", esp_http_client_get_status_code(client));
    }
    else
    {
      ESP_LOGE(TAG, "Failed to read response");
      http_num_error++;
    }
  }
  esp_http_client_cleanup(client);
}

static void on_sensor_publish(const char* device_id, const char* data)
{
  last_publish_sec = (uint32_t)(esp_timer_get_time() / 1000000);
  if (app_cfg.push_type == 'h')
  {
    http_publish(device_id, data);
  }
  else
  {
    mqtt_task_publish(device_id, data);
  }
}

void app_main(void)
{
  app_init();
  //test_gpio_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_setup();

#if SUPPORT_WATCHDOG
  CHECK_ERROR_CODE(esp_task_wdt_init(TWDT_TIMEOUT_S, true), ESP_OK);  // set true to reboot device when task is hang.
#endif

#if SUPPORT_SHT3x
  sht3x_task(device_mac_str, on_sensor_publish);
#endif
#if SUPPORT_SPS30
  sps30_task(device_mac_str, on_sensor_publish);
#endif
  xTaskCreate(main_task_run, "main_task", 1024 * 8, (void *)0, 10, NULL);
}

