# Brain Lamp

ESP8266 firmware for a hacked [Philips LivingColors Micro](https://www.philips.com) desk lamp (`7001830PH`). A Wemos D1 Mini replaces the original controller so the lamp can be driven over HTTP, MQTT, and (via a broker + IFTTT / Google Home) voice.

The hardware story, teardown, and smart-home wiring live in the companion post:

**[A brain for the light!](https://medium.com/p/c5b290c2e31a)**

This repo is the 2018 proof of concept that post points at. It is not a polished product: credentials are hardcoded, the original touch strip was abandoned, and the web colour picker is served from your laptop, not the ESP.

## Hardware

| Item | Notes |
| --- | --- |
| Lamp | Philips LivingColors Micro (`7001830PH`) |
| Board | Wemos D1 Mini (ESP8266) |
| LED | Common-anode RGB module from the lamp |
| Power | Original lamp supply, with a hole melted in the shell for USB reflashing |

Wiring used in firmware:

| LED | Wemos pin | GPIO |
| --- | --- | --- |
| Red | D1 | 5 |
| Green | D2 | 4 |
| Blue | D3 | 0 |

The original plan was to keep the capacitive touch strip and switch between it and the ESP with a PNP transistor. After a breadboard full of spaghetti and a dead transistor, the strip came out and the D1 Mini stayed in.

## Firmware

PlatformIO project, Arduino framework, board `d1_mini`.

On boot the lamp is used as a status LED:

1. Flashes red for a few seconds
2. Stays red while joining Wi-Fi
3. Goes orange once Wi-Fi is up
4. Turns off when the MQTT client is initialised, then reconnects in `loop()`

Edit these before flashing (`src/main.cpp`):

```cpp
const char *ssid = "...";
const char *password = "...";
const char *mqtt_server = "192.168.0.200";
```

The MQTT topic is `lightBrain` (the post says `brainLamp`; the firmware won).

```bash
pio run -t upload
```

## Control

### HTTP

```
http://<lamp-ip>/set?r=255&g=128&b=0
```

Values are 0–255. The ESP maps them onto 10-bit PWM (`value * 4`).

### MQTT

Subscribe/publish on `lightBrain` (broker port **1883**). Payload is either a CSV RGB triple or a colour name:

| Payload | Effect |
| --- | --- |
| `255,0,128` | Set RGB |
| `red` / `green` / `blue` | Named colour (substring match) |
| `random` | Random RGB |

Voice control in the post is: Google Home → IFTTT webhook → a Raspberry Pi MQTT bridge → this topic. That bridge is not in this repo. Home Assistant or OpenHAB work the same way if you would rather not write one.

### Colour picker

`web/` is a tiny jscolor page that publishes RGB to `lightBrain` over MQTT WebSockets (port **1884** in `web/index.html`). Point the broker host at yours, then:

```bash
cd web
python3 -m http.server
```

## Layout

```
src/main.cpp              firmware
lib/mqtt_client/          bundled PubSubClient
web/                      colour picker (MQTT over WebSockets)
platformio.ini            Wemos D1 Mini / ESP8266
```

## License

[MIT](LICENSE)
