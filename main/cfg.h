#pragma once

#include <stdint.h>

typedef struct _MQTT_CFG
{
  char uri[64];
  char client_id[32];
  char username[16];
  char password[16];
} MQTT_CFG;

typedef struct _CFG
{
  char wifi_ssid[32];
  char wifi_password[32];
  MQTT_CFG mqtt_cfg;
} CFG;

#ifdef __cplusplus
extern "C" {
#endif

int cfg_init();

void cfg_get_default(CFG* cfg);

int cfg_read(CFG* cfg);

int cfg_write(const CFG* cfg);

int cfg_erase();

#ifdef __cplusplus
}
#endif
