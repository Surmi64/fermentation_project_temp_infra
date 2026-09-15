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

---

## Quickstart

### 1. Flash the ESP32
1. Open the project in **Arduino IDE** (or PlatformIO).
2. Install required libraries: `OneWire`, `DallasTemperature`, `PubSubClient`.
3. Update Wi-Fi and MQTT IP configuration in `src/main.cpp`.
4. Upload code to ESP32 (`/dev/ttyUSB0` on Ubuntu).

### 2. Sensor Identification
Open Serial Monitor at `115200` baud. Hold one probe in your hand to see which sensor index (`0-3`) spikes in temperature, then label that physical probe.

### 3. Spin Up Services
```bash
docker compose up -d