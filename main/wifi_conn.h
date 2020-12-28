#ifndef __WIFI_CONN_H
#define __WIFI_CONN_H

#include "esp_netif.h"

typedef struct {
  uint8_t flag_connected;
  esp_netif_ip_info_t ip_info;
  esp_netif_ip6_info_t ip6_info;
} WIFI_CONN;

extern WIFI_CONN wifi_conn;

#ifdef __cplusplus
extern "C" {
#endif

void wifi_init(void);

void wifi_connect(const char* ssid, const char* password);

void wifi_info_print();

#ifdef __cplusplus
}
#endif

#endif // __WIFI_CONN_H
