#include <stdlib.h>
#include <string.h>
#include "cli_linebuf.h"

void linebuf_init(LineBuffer_t* linebuf, char* buf, uint16_t buf_size)
{
    linebuf->buf = buf;
    linebuf->buf_len = buf_size - 1;
    linebuf->count = 0;
    memset(linebuf->buf, 0, buf_size);
}

void linebuf_append(LineBuffer_t* linebuf, char c)
{
  if (linebuf->count < linebuf->buf_len)
    linebuf->buf[linebuf->count++] = c;
}

char linebuf_pop(LineBuffer_t* linebuf)
{
  char c;
  if (linebuf->count > 0)
  {
    c = linebuf->buf[--linebuf->count];
    linebuf->buf[linebuf->count] = 0;
  }
  else
    c = 0;
  return c;
}

void linebuf_clear(LineBuffer_t* linebuf)
{
  linebuf->count = 0;
  memset(linebuf->buf, 0, linebuf->buf_len);
}
