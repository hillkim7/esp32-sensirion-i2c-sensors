# ESP32 Application Working With Sensirion I2C Sensors.

## Overview

This source code is to fetch data from Sensirion I2C sensors on ESP32 platform.

## Sensor Report

This sensor module reports using MQTT protocol.

### SHT30_DIS Temperature/Humidify Sensor:
- MQTT topic  
  topic format: /sensors/<MAC address>/report
```
/sensors/bcddc2ddde9c/report
```
- MQTT payload  
  payload format: JSON
```
{ type: 'temp/humi', temp: 27.73, humi: 10.03 }
```

### SPS30 Particulate Matter Sensor:
- MQTT topic  
  topic format: /sensors/<MAC address>/report
```
/sensors/bcddc2ddde9c/report
```
- MQTT payload  
  payload format: JSON
```
{
  type: 'PM',
  'pm1.0': 5.2,
  'pm2.5': 6.38,
  'pm4.0': 7.09,
  'pm10.0': 7.23,
  'nc0.5': 33.97,
  'nc1.0': 40.41,
  'nc2.5': 41.48,
  'nc4.5': 41.69,
  'nc10.0': 41.73
}
```

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
