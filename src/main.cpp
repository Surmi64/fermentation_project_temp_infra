#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

static const uint8_t ONE_WIRE_PIN = 4;
static const uint8_t MAX_PROBES = 4;
static const unsigned long READ_INTERVAL_MS = 2000;

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

DeviceAddress probeAddress[MAX_PROBES];
uint8_t probeCount = 0;

static void sortProbes(uint8_t count) {
  for (uint8_t i = 1; i < count; i++) {
    DeviceAddress key;
    memcpy(key, probeAddress[i], sizeof(DeviceAddress));

    int8_t j = i - 1;
    while (j >= 0 && memcmp(probeAddress[j], key, sizeof(DeviceAddress)) > 0) {
      memcpy(probeAddress[j + 1], probeAddress[j], sizeof(DeviceAddress));
      j--;
    }
    memcpy(probeAddress[j + 1], key, sizeof(DeviceAddress));
  }
}

static void printAddress(const DeviceAddress addr) {
  for (uint8_t i = 0; i < 8; i++) {
    Serial.printf("%02X", addr[i]);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  sensors.begin();
  probeCount = sensors.getDeviceCount();
  if (probeCount > MAX_PROBES) {
    probeCount = MAX_PROBES;
  }

  for (uint8_t i = 0; i < probeCount; i++) {
    sensors.getAddress(probeAddress[i], i);
  }
  sortProbes(probeCount);

  Serial.println();
  Serial.printf("found %u probe(s) on GPIO %u\n", probeCount, ONE_WIRE_PIN);
  for (uint8_t i = 0; i < probeCount; i++) {
    Serial.printf("  probe %u -> ", i);
    printAddress(probeAddress[i]);
    Serial.println();
  }
}

void loop() {
  sensors.requestTemperatures();

  for (uint8_t i = 0; i < probeCount; i++) {
    Serial.printf("probe %u: %.2f C\n", i, sensors.getTempC(probeAddress[i]));
  }

  delay(READ_INTERVAL_MS);
}
