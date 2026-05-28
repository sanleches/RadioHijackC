# RadioHijackC

Raspberry Pi Pico 2 W firmware for controlling an RDA5807M FM radio module from a browser dashboard.

The project is a Pico SDK C++ rewrite of the original MicroPython version. It keeps the same main functionality: Wi-Fi dashboard, radio tune/seek/volume/mute/power, RDS station/text display, station scanning, and persistent presets.

## Hardware

- Board: Raspberry Pi Pico 2 W
- Radio module: RDA5807M FM receiver
- I2C bus: `i2c0`
- SDA: `GP4`
- SCL: `GP5`
- I2C speed: `100 kHz`

Connect the RDA5807M module to power, ground, `GP4` SDA, and `GP5` SCL. Use pull-ups if your module does not already include them.

## Network Behavior

The firmware first tries to join the configured Wi-Fi network from `include/config/app_config.hpp`.

If that fails, it starts its own fallback access point:

- SSID: `RadioHijack-AP`
- Password: `radio1234`
- Dashboard: `http://192.168.4.1`

When connected to your Wi-Fi, the dashboard address is the DHCP IP printed over USB serial.

## Status LED

The Pico W onboard LED shows network state:

- Booting / connecting: fast continuous blink
- Connected to configured Wi-Fi: one short blink, then a long pause
- Fallback AP mode: double blink, then a long pause

## USB Serial Status

The firmware prints the dashboard address at startup and every 60 seconds.

Send any of these characters over USB serial to print status immediately:

- `s`
- `S`
- `?`

Status includes network mode, dashboard URL, radio power/mute state, frequency, volume, RSSI, station, and RadioText.

## Web Dashboard

Open the reported IP address in a browser.

The dashboard supports:

- Manual tune
- Seek up/down
- Step by 0.1 MHz
- Volume slider
- Mute/unmute
- Station scan
- Preset save/list/tune
- RDS station name
- RDS RadioText/song text
- RSSI, stereo, and RDS-ready status

## HTTP API

Routes are compatible with the original MicroPython project.

- `GET /status`
- `GET /tune?f=99.7`
- `GET /seek?dir=up`
- `GET /seek?dir=down`
- `GET /vol?v=8`
- `GET /step?mhz=0.1`
- `GET /scan?start=87.0&stop=108.0&step=0.2&minrssi=10`
- `GET /scanhtml?step=0.2&minrssi=10`
- `GET /mute?on=1`
- `GET /power?on=1`
- `GET /presets?action=save&name=Station&freq=99.7`
- `GET /presets?action=delete&name=Station`

The `/api/*` aliases are also supported for the same core routes.

## Project Layout

```text
RadioHijackC/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── config/       Compile-time app settings
│   ├── net/          Wi-Fi, DHCP, HTTP server, API routing
│   ├── radio/        RDA5807M driver and RDS parser
│   ├── storage/      Flash-backed preset storage
│   ├── ui/           Web page, serial console, status LED
│   ├── util/         Logger and URL helpers
│   └── lwipopts.h    lwIP raw API configuration
└── src/
    ├── main.cpp      Firmware entry point
    ├── net/          Network and API implementations
    ├── radio/        Radio implementations
    ├── storage/      Preset storage implementation
    ├── ui/           Dashboard/status UI implementations
    └── util/         Utility implementations
```

## Module Summary

- `config/app_config.hpp`: Wi-Fi credentials, AP credentials, I2C pins, defaults, limits
- `net/wifi_manager.*`: Station Wi-Fi and fallback AP setup
- `net/dhcp_server.*`: Minimal DHCP responder for fallback AP mode
- `net/web_server.*`: Raw lwIP HTTP server
- `net/api_router.*`: HTTP route handling and JSON/HTML responses
- `radio/rda5807m.*`: RDA5807M I2C driver
- `radio/rds_parser.*`: RDS station name and RadioText parser
- `storage/preset_store.*`: Flash-backed preset persistence
- `ui/web_page.*`: Embedded dashboard HTML/CSS/JavaScript
- `ui/serial_console.*`: Periodic and command-triggered serial status
- `ui/status_led.*`: Onboard LED network status patterns
- `util/logger.*`: USB serial logging
- `util/url.*`: URL decoding and query parsing

## Build

From the project directory:

```sh
cmake --build build
```

The generated UF2 is:

```text
build/RadioHijackC.uf2
```

## Flashing

Put the Pico 2 W into BOOTSEL mode and copy `build/RadioHijackC.uf2` to the mounted drive.

After flashing, open USB serial or watch the LED pattern to determine whether it joined Wi-Fi or started fallback AP mode.

## Configuration

Edit `include/config/app_config.hpp` to change:

- Wi-Fi SSID/password
- Fallback AP SSID/password
- I2C pins
- Default frequency
- Default volume
- HTTP port
- Preset limits

Rebuild after editing configuration.
