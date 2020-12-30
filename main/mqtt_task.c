#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "sps30.h"
#include "sensirion_arch_config.h"
#include "sensirion_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "sensor_task.h"

static const char TAG[] = "mqtt-task";

static esp_mqtt_client_handle_t mqtt_client;
static uint8_t mqtt_connected = 0;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
  //esp_mqtt_client_handle_t client = event->client;
  //int msg_id;
  // your_context_t *context = event->context;
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
#if 0
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
#endif
        mqtt_connected = 1;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = 0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
#if 0
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
#endif
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        mqtt_connected = 0;
        ESP_LOGW(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
  }
  return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
  mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(const char* uri, const char* client_id, const char* username, const char* password)
{
  esp_mqtt_client_config_t mqtt_cfg = {
    .uri = uri,
    .client_id = client_id,
    .username = username,
    .password = password
  };

  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
  esp_mqtt_client_start(mqtt_client);
}

void mqtt_task_status_print(void)
{
  if (mqtt_connected)
  {
    printf("[MQTT connected]\n");
  }
  else
  {
    printf("[MQTT disconnected]\n");
  }
}

void mqtt_task_publish(const char* device_id, const char* data)
{
  if (mqtt_client != NULL)
  {
    char topic[64];
    sprintf(topic, "/sensors/%s/report", device_id);
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, 0, 1, 0);
    ESP_LOGI(TAG, "esp_mqtt_client_publish: topic=%s data=%s result=%d", topic, data, msg_id);
  }
  else
  {
    ESP_LOGW(TAG, "mqtt_task_publish: MQTT not connected");
  }
}

void mqtt_task(const char* broker_uri, const char* client_id, const char* username, const char* password)
{
  mqtt_app_start(broker_uri, client_id, username, password);
}

