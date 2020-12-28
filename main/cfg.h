#pragma once

#include <stdint.h>

typedef struct _CFG
{
  char wifi_ssid[32];
  char wifi_password[32];
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
