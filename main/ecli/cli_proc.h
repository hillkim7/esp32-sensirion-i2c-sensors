#pragma once

#include <stdint.h>

typedef void (*CLI_WriteFunc_t)(const char* data, uint16_t len);

void cliproc_init(void);
void cliproc_push_key(char key, CLI_WriteFunc_t write_func);

