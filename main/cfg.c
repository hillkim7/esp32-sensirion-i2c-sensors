#include <string.h>
#include <stdio.h>
#include "cfg.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char cfg_namespace[] = "appcfg";
static const char cfg_key[] = "cfg";
static const char TAG[] = "cfg";

int cfg_init()
{
  return 0;
}

void cfg_get_default(CFG* cfg)
{
  memset(cfg, 0, sizeof(CFG));
}

static int cfg_read_(const char* name_space, const char* key, uint8_t* data, size_t data_size)
{
  nvs_handle_t handle;
  esp_err_t err;

  err = nvs_open(name_space, NVS_READWRITE, &handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_open failed: '%s' 0x%x", name_space, err);
    return -1;
  }

  // Read run time blob
  size_t required_size = 0;  // value will default to 0, if not set yet in NVS
  // obtain required memory space to store blob being read from NVS
  err = nvs_get_blob(handle, key, NULL, &required_size);
  if (err != ESP_OK)
  {
    if (err != ESP_ERR_NVS_NOT_FOUND)
    {
      ESP_LOGE(TAG, "nvs_get_blob failed: 0x%x", err);
    }
    else
    {
      ESP_LOGE(TAG, "nvs_get_blob not found: %s", key);
    }
    nvs_close(handle);
    return -1;
  }

  if (data_size != required_size)
  {
    ESP_LOGE(TAG, "size miss match: cfg_key=%s expected=%d actual=%d", key, data_size, required_size);
    nvs_close(handle);
    return -1;
  }

  nvs_get_blob(handle, key, (uint8_t*)data, &required_size);
  nvs_close(handle);
  ESP_LOGD(TAG, "oc_storage_read: '%s' %d", key, required_size);

  return 0;
}

int cfg_write_(const char* name_space, const char* key, const uint8_t* data, size_t data_size)
{
  nvs_handle_t handle;
  esp_err_t err;

  err = nvs_open(name_space, NVS_READWRITE, &handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_open failed: '%s' 0x%x", name_space, err);
    return -1;
  }

  err = nvs_set_blob(handle, key, data, data_size);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_set_blob failed: 0x%x", err);
    nvs_close(handle);
    return -1;
  }

  nvs_close(handle);
  ESP_LOGD(TAG, "oc_storage_write: '%s' %d", key, data_size);

  return 0;
}

int cfg_erase_(const char* name_space)
{
  nvs_handle_t handle;
  esp_err_t err;
  err = nvs_open(name_space, NVS_READWRITE, &handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_open failed: '%s' 0x%x", name_space, err);
    return -1;
  }
  nvs_erase_all(handle);
  nvs_close(handle);
  return 0;
}

int cfg_read(CFG* cfg)
{
  return cfg_read_(cfg_namespace, cfg_key, (uint8_t*)cfg, sizeof(CFG));
}

int cfg_write(const CFG* cfg)
{
  return cfg_write_(cfg_namespace, cfg_key, (const uint8_t*)cfg, sizeof(CFG));
}

int cfg_erase()
{
  return cfg_erase_(cfg_namespace);
}
