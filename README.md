# esp32-coffee-thermometer

Real-time temperature monitoring setup for comparing 4 coffee cups simultaneously during a cupping session. Built to log and visualize cooling curves using an ESP32, 4x DS18B20 probes, MQTT, and Grafana.

Temperature drives what you actually taste: as a cup cools, perceived acidity and sweetness climb while bitterness recedes, so two coffees only compare fairly when they are tasted at the same point on their cooling curve. Logging that curve turns "taste it when it's ready" into something repeatable. Designed for James Hoffmann & Lucia Solis's [*The Fermentation Project*](https://www.thefermentationproject.com/) tasting experiment.

---

## Hardware

- **MCU:** ESP32 DevKit (WiFi)
- **Sensors:** 4x DS18B20 waterproof probes (1m cable)
- **Adapter:** DS18B20 screw-terminal adapter module (includes 4.7kΩ pull-up resistor)
- **Connections:** 3x Female-Female DuPont wires

---

## Wiring

All 4 probes are connected in **parallel** into the adapter's screw terminals.

### 1. Probes to Adapter Terminal
- **Red wires** (VCC) -> `VCC` / `+`
- **Black wires** (GND) -> `GND` / `-`
- **Yellow/Green wires** (Data) -> `DAT` / `A`

### 2. Adapter to ESP32
- `VCC` -> `3V3` (or `VIN`)
- `GND` -> `GND`
- `DAT` -> `GPIO 4` (`D4`)

---

## Data Flow & MQTT Topics

The ESP32 connects to Wi-Fi and reads all 4 sensors on 1-Wire GPIO 4 every 2 seconds.

Metrics are published to the local MQTT broker:

- `coffee/sensor/0/temperature`
- `coffee/sensor/1/temperature`
- `coffee/sensor/2/temperature`
- `coffee/sensor/3/temperature`

Alongside the readings:

- `coffee/status` - retained `online` / `offline`, backed by an MQTT last
  will, so a stalled graph can be told apart from a board that dropped off
- `coffee/session/elapsed` - seconds since t=0, published each cycle

Publishing anything to `coffee/session/start` resets t=0. Marking the pour
this way lets several cups be lined up on the same axis afterwards:

```bash
mosquitto_pub -h localhost -t coffee/session/start -n
```

---

## Quickstart

### 1. Flash the ESP32
1. Open the project in **Arduino IDE** (or PlatformIO).
2. Install the ESP32 core: Boards Manager -> *esp32* by Espressif Systems.
3. Install required libraries: `OneWire`, `DallasTemperature`, `PubSubClient`.
4. Copy `src/config.example.h` to `src/config.h` and fill in the Wi-Fi and
   broker values. `config.h` is git-ignored and stays on your machine.
5. Select the board and settings below, then upload (`/dev/ttyUSB0` on Ubuntu).

Board: **ESP32 Dev Module** (`esp32:esp32:esp32`). The named presets such as
*DOIT ESP32 DEVKIT V1* target 30-pin clones and map pins differently.

| Setting | Value |
| --- | --- |
| Flash Size | 4MB (32Mb) |
| PSRAM | Disabled |
| Flash Mode | QIO |
| Partition Scheme | Default 4MB with spiffs |
| CPU Frequency | 240MHz |
| Upload Speed | 921600 |

PSRAM must stay disabled - the WROOM-32 module has none, and enabling it boot
loops. Drop the upload speed to `115200` if flashing fails partway.

### 2. Sensor Identification
Open Serial Monitor at `115200` baud. Hold one probe in your hand to see which sensor index (`0-3`) spikes in temperature, then label that physical probe.

### 3. Spin Up Services
```bash
docker compose up -d
```

This starts two containers:

- **`coffee-mqtt`** - Mosquitto broker on port `1883`
- **`coffee-grafana`** - Grafana on port `3000`

Broker settings live in `mosquitto/config/mosquitto.conf`. The listener is
anonymous, which is fine on a trusted LAN and not fine on anything exposed to
the internet.

### 4. Verify the Data
With the ESP32 powered on, subscribe to every sensor topic at once:

```bash
mosquitto_sub -h localhost -t 'coffee/sensor/#' -v
```

Readings should appear every 2 seconds, one line per probe.

---

## Repository Layout

```
.
├── docker-compose.yml
└── mosquitto/
    └── config/
        └── mosquitto.conf
```

