idf_component_register(SRCS
  "sensor_main.c"
  "sht3x_task.c"
  "sps30_task.c"
  "mqtt_task.c"
  "wifi_conn.c"
  "cfg.c"
  "sht3x/esp32_wrapper.c"
  "sht3x/sht3x.c"
  "embedded-uart-common/sensirion_shdlc.c"
  "embedded-uart-common/sensirion_uart_esp32.c"
  "sps30/sps30.c"
  "ecli/cli.c"
  "ecli/cli_linebuf.c"
  "ecli/cli_proc.c"
  INCLUDE_DIRS "." "./sht3x" "./embedded-uart-common" "./sps30" "./ecli")

target_compile_definitions (${COMPONENT_TARGET} PUBLIC SHT3x_DEBUG_LEVEL_1)

## Uncomment below line if you need I2C debugging.
# target_compile_definitions (${COMPONENT_TARGET} PUBLIC SHT3x_DEBUG_LEVEL_2)
