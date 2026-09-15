#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"

static const uint8_t ONE_WIRE_PIN = 4;
static const uint8_t MAX_PROBES = 4;
static const unsigned long READ_INTERVAL_MS = 2000;

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

WiFiClient net;
PubSubClient mqtt(net);

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

static void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("wifi: connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf(" ok, ip %s\n", WiFi.localIP().toString().c_str());
}

static bool ensureMqtt() {
  if (mqtt.connected()) {
    return true;
  }

  if (mqtt.connect(MQTT_CLIENT_ID)) {
    Serial.println("mqtt: connected");
    return true;
  }

  Serial.printf("mqtt: connect failed, state %d\n", mqtt.state());
  return false;
}

static void printAddress(const DeviceAddress addr) {
  for (uint8_t i = 0; i < 8; i++) {
    Serial.printf("%02X", addr[i]);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  connectWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);

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
  if (!ensureMqtt()) {
    delay(READ_INTERVAL_MS);
    return;
  }
  mqtt.loop();

  sensors.requestTemperatures();

  for (uint8_t i = 0; i < probeCount; i++) {
    const float celsius = sensors.getTempC(probeAddress[i]);

    char topic[48];
    char payload[16];
    snprintf(topic, sizeof(topic), "coffee/sensor/%u/temperature", i);
    snprintf(payload, sizeof(payload), "%.2f", celsius);

    mqtt.publish(topic, payload);
    Serial.printf("probe %u: %s C\n", i, payload);
  }

  delay(READ_INTERVAL_MS);
}
