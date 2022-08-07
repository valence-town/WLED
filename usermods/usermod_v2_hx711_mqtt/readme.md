# HX711 to MQTT

This usermod will publish values of HX711 load sensors via MQTT.

It reuses the MQTT connection specified in the WLED web user interface.

## Maintainer

me@khimaros.com

## Features

- Reads HX711 raw load sensor values.
- Publishes load sensor readings via MQTT.

## Example mqtt topics:

`$mqttDeviceTopic` is set in webui of WLED!

```
load: $mqttDeviceTopic/load
```

## Installation

1. Copy `platformio_override.ini.sample` to the root directory of your WLED checkout and name the file `platformio_override.ini`.
2. Edit as needed to match your controller settings.
3. Change pin settings at the top of `usermod_v2_hx711_mqtt.h` as needed.
3. Build and upload to your controller.

## Hardware

### Requirements

1. An HX711 based load amplifier.
2. A microcontroller which can talk I2C, e.g. ESP32.

### Wiring

Attach the load amplifier to the I2C interface.

# Credits

- The `sensors_to_mqtt` module author, whose code was borrowed liberally.
