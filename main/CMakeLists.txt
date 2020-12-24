idf_component_register(SRCS
  "sensor_main.c"
  "sht3x/esp32_wrapper.c"
  "sht3x/sht3x.c"
  INCLUDE_DIRS "." "./sht3x")

target_compile_definitions (${COMPONENT_TARGET} PUBLIC SHT3x_DEBUG_LEVEL_1)

## Uncomment below line if you need I2C debugging.
# target_compile_definitions (${COMPONENT_TARGET} PUBLIC SHT3x_DEBUG_LEVEL_2)
