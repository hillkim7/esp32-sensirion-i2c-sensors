#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cli_proc.h"
#include "cli_linebuf.h"
#include "cli.h"

#define CLI_HISTORY_SIZE 5
#define CLI_INPUT_BUF_SIZE  256
#define ESC_KEY  0x1B

static const char ERASE_CHARS[3] = {'\b', ' ', '\b'};

static char cli_input_buf[CLI_INPUT_BUF_SIZE + 1];
static LineBuffer_t linebuf;

static char cli_history[CLI_HISTORY_SIZE][CLI_INPUT_BUF_SIZE + 1];
static int cli_history_index = -1;

static void add_history(const char* input_line)
{
  size_t i;

  // don't put duplicated input line in command history
  if (input_line[0] == 0 || !strcmp(input_line, cli_history[0]))
    return;

  // shift history
  for (i = 0; i < CLI_HISTORY_SIZE - 1; ++i)
  {
    strcpy(cli_history[i + 1], cli_history[i]);
  }

  strcpy(cli_history[0], input_line);
}

static char* select_history(int is_up_key)
{
  if (is_up_key)
  {
    cli_history_index++;
    if (cli_history_index == CLI_HISTORY_SIZE)
      cli_history_index = CLI_HISTORY_SIZE - 1;
  }
  else
  {
    cli_history_index--;
    if (cli_history_index < 0)
      cli_history_index = 0;
  }

  return cli_history[cli_history_index];
}

static void reset_hisotry_cursor(void)
{
  cli_history_index = -1;
}

static void handle_up_down_key(CLI_WriteFunc_t write_func, int is_up_key)
{
  char* cmd = select_history(is_up_key);

  // erase current input buffer
  while (linebuf.count > 0)
  {
    write_func(ERASE_CHARS, sizeof(ERASE_CHARS));
    linebuf_pop(&linebuf);
  }

  while(*cmd)
  {
    write_func(cmd, 1);
    linebuf_append(&linebuf, *cmd++);
  }
}

void cliproc_init(void)
{
  linebuf_init(&linebuf, cli_input_buf, CLI_INPUT_BUF_SIZE);
}

void cliproc_push_key(char key, CLI_WriteFunc_t write_func)
{
  static uint8_t escape_seq = 0;
  
  if (key == '\t')
    key = ' ';

  if (escape_seq)
  {
    //printf("escape_seq=%d key=%0X %c\n", escape_seq, key, key);
    if (key == 0x5B)
    {
      escape_seq++;
      return;
    }

    if (escape_seq == 2)
    {
      // up arrow key sequence: <ESC><0x5B>A
      // down arrow key sequence: <ESC><0x5B>B
      // right arrow key sequence: <ESC><0x5B>C
      // left arrow key sequence: <ESC><0x5B>D
      if (key == 'A')
      {
        handle_up_down_key(write_func, 1);
      }
      else if (key == 'B')
      {
        handle_up_down_key(write_func, 0);
      }
    }
    escape_seq = 0;
    return;
  }
  else if (key == ESC_KEY)
  {
    escape_seq = 1;
    return;
  }
  else if (key == '\r')
  {
    write_func(&key, 1);
    key = '\n';
  }

  if (key == '\n')
  {
    //if (linebuf.count > 0)
      write_func(&key, 1);
    add_history(linebuf.buf);   
    reset_hisotry_cursor();
    linebuf_append(&linebuf, '\n');
    cli_engine(linebuf.buf);
    linebuf_clear(&linebuf);
  }
  else if (key == ('c' & 0x1f))  // Ctrl+C
  {
    linebuf_append(&linebuf, '\n');
    linebuf_clear(&linebuf);
    cli_engine(linebuf.buf);
  }
  else if (key == '\b' || key == '\x7f')  // backspace or del
  {
    if (linebuf.count > 0)
    {
      write_func(ERASE_CHARS, sizeof(ERASE_CHARS));
      linebuf_pop(&linebuf);
    }
  }
  else if (key == ('u' & 0x1f))  // Ctrl+u
  {
    while (linebuf.count > 0)
    {
      write_func(ERASE_CHARS, sizeof(ERASE_CHARS));
      linebuf_pop(&linebuf);
    }
  }
  else if ((key >= ' ' && key <= '\x7e'))
  {
    write_func(&key, 1);
    linebuf_append(&linebuf, key);
  }
}

