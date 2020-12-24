# ESP32 Application Working With Sensirion I2C Sensors.

## Overview

This source code is to fetch data from Sensirion I2C sensors on ESP32 platform.

## Hardware

### Sensors pin map:

|                  | SDA    | SCL    |
| ---------------- | ------ | ------ |
| SHT30_DIS        | GPIO21 | GPIO22 |

## Reference source code

This SHT3x related source code based on 
[sht3x-esp-idf](https://github.com/gschorcht/sht3x-esp-idf).

## Testing H/W working

A I2C sensor can be scaned with an example application in `esp-idf/examples/peripherals/i2c/i2c_tools `.

```bash
i2c-tools> i2cconfig --port=0 --sda=21 --scl=22  --freq=100000
i2c-tools> i2cdetect
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00: 00 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- 44 -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
```
