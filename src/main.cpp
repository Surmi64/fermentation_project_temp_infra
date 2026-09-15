#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

static const uint8_t ONE_WIRE_PIN = 4;
static const unsigned long READ_INTERVAL_MS = 2000;

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

uint8_t probeCount = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  sensors.begin();
  probeCount = sensors.getDeviceCount();

  Serial.println();
  Serial.printf("found %u probe(s) on GPIO %u\n", probeCount, ONE_WIRE_PIN);
}

void loop() {
  sensors.requestTemperatures();

  for (uint8_t i = 0; i < probeCount; i++) {
    Serial.printf("probe %u: %.2f C\n", i, sensors.getTempCByIndex(i));
  }

  delay(READ_INTERVAL_MS);
}
