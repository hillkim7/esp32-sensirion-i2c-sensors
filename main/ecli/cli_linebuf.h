#pragma once

#include <stdint.h>

typedef struct
{
  char* buf;
  uint16_t buf_len;
  uint16_t count;
} LineBuffer_t;

void linebuf_init(LineBuffer_t* linebuf, char* buf, uint16_t buf_size);

void linebuf_append(LineBuffer_t* linebuf, char c);

char linebuf_pop(LineBuffer_t* linebuf);

void linebuf_clear(LineBuffer_t* linebuf);

