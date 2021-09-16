#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi_conn.h"

static EventGroupHandle_t wifi_event_group;

static const int IPV4_CONNECTED_BIT = BIT0;
static const int IPV6_CONNECTED_BIT = BIT1;

WIFI_CONN wifi_conn;

/* types of ipv6 addresses to be displayed on ipv6 events */
static const char *s_ipv6_addr_types[] = {
    "ESP_IP6_ADDR_IS_UNKNOWN",
    "ESP_IP6_ADDR_IS_GLOBAL",
    "ESP_IP6_ADDR_IS_LINK_LOCAL",
    "ESP_IP6_ADDR_IS_SITE_LOCAL",
    "ESP_IP6_ADDR_IS_UNIQUE_LOCAL",
    "ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6"
};

static const char TAG[] = "wifi_conn";

static void sta_start(void *esp_netif, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
  esp_wifi_connect();
}

static void sta_disconnected(void *esp_netif, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
  ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
  esp_wifi_connect();
  xEventGroupClearBits(wifi_event_group, IPV4_CONNECTED_BIT);
  xEventGroupClearBits(wifi_event_group, IPV6_CONNECTED_BIT);
  wifi_conn.flag_connected = 0;
}

static void sta_connected(void *esp_netif, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
  ESP_LOGI(TAG, "Wi-Fi connected");
  esp_netif_create_ip6_linklocal(esp_netif);
  wifi_conn.flag_connected = 1;
}

static void got_ip(void *esp_netif, esp_event_base_t event_base,
                   int32_t event_id, void *event_data)
{
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  xEventGroupSetBits(wifi_event_group, IPV4_CONNECTED_BIT);
  ESP_LOGI(TAG, "Got IPv4 event: address: " IPSTR, IP2STR(&event->ip_info.ip));
  wifi_conn.ip_info = event->ip_info;
}

static void got_ip6(void *esp_netif, esp_event_base_t event_base,
                    int32_t event_id, void *event_data)
{
  ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
  esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
  ESP_LOGI(TAG, "Got IPv6 event: address: " IPV6STR ", type: %s",
           IPV62STR(event->ip6_info.ip), s_ipv6_addr_types[ipv6_type]);
  xEventGroupSetBits(wifi_event_group, IPV6_CONNECTED_BIT);
  if (ipv6_type == ESP_IP6_ADDR_IS_LINK_LOCAL) {
    //memcpy(&s_ipv6_addr, &event->ip6_info.ip, sizeof(s_ipv6_addr));
    wifi_conn.ip6_info = event->ip6_info;
  }
}

void wifi_init(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  char *desc;
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
  // Prefix the interface description with the module TAG
  // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
  asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
  esp_netif_config.if_desc = desc;
  esp_netif_config.route_prio = 128;
  esp_netif_t *netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
  free(desc);
  ESP_ERROR_CHECK(esp_wifi_set_default_wifi_sta_handlers());

  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, sta_disconnected, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, sta_start, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, sta_connected, netif));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, got_ip6, NULL));
}

void wifi_connect(const char* ssid, const char* password)
{
  wifi_config_t wifi_config = {0, };

  strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)-1);
  strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password)-1);
  ESP_LOGI(TAG, "WIFI connect: '%s', '%s'", wifi_config.sta.ssid, wifi_config.sta.password);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_info_print()
{
  if (wifi_conn.flag_connected)
  {
    printf("[WIFI connected]\n");
    printf("IPv4: IP " IPSTR ", GW " IPSTR "\n", IP2STR(&wifi_conn.ip_info.ip), IP2STR(&wifi_conn.ip_info.gw));
    printf("IPv6: address " IPV6STR " %s\n",
             IPV62STR(wifi_conn.ip6_info.ip), s_ipv6_addr_types[ESP_IP6_ADDR_IS_LINK_LOCAL]);
  }
  else
  {
    printf("[WIFI disconnected]\n");
  }
}

