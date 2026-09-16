#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"

static const uint8_t ONE_WIRE_PIN = 4;
static const uint8_t MAX_PROBES = 4;
static const unsigned long READ_INTERVAL_MS = 2000;
static const unsigned long MQTT_RETRY_MS = 5000;
static const unsigned long WIFI_RETRY_MS = 15000;
static const uint8_t SENSOR_RESOLUTION_BITS = 12;

static const char STATUS_TOPIC[] = "coffee/status";
static const char STATUS_ONLINE[] = "online";
static const char STATUS_OFFLINE[] = "offline";
static const char SESSION_START_TOPIC[] = "coffee/session/start";
static const char SESSION_ELAPSED_TOPIC[] = "coffee/session/elapsed";

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

WiFiClient net;
PubSubClient mqtt(net);

DeviceAddress probeAddress[MAX_PROBES];
uint8_t probeCount = 0;

unsigned long lastReadMs = 0;
unsigned long lastMqttAttemptMs = 0;
unsigned long lastWifiAttemptMs = 0;
bool wifiWasUp = false;
unsigned long sessionStartMs = 0;

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

static void beginWifi() {
  lastWifiAttemptMs = millis();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("wifi: connecting to %s\n", WIFI_SSID);
}

static bool wifiReady() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiWasUp) {
      wifiWasUp = true;
      Serial.printf("wifi: up, ip %s\n", WiFi.localIP().toString().c_str());
    }
    return true;
  }

  if (wifiWasUp) {
    wifiWasUp = false;
    Serial.println("wifi: link lost");
  }

  const unsigned long now = millis();
  if (now - lastWifiAttemptMs >= WIFI_RETRY_MS) {
    Serial.println("wifi: retrying");
    WiFi.disconnect();
    beginWifi();
  }
  return false;
}

static void onMessage(char *topic, uint8_t *payload, unsigned int length) {
  (void)payload;
  (void)length;

  if (strcmp(topic, SESSION_START_TOPIC) == 0) {
    sessionStartMs = millis();
    Serial.println("session: t=0 marked");
  }
}

static bool ensureMqtt() {
  if (mqtt.connected()) {
    return true;
  }

  const unsigned long now = millis();
  if (lastMqttAttemptMs != 0 && now - lastMqttAttemptMs < MQTT_RETRY_MS) {
    return false;
  }
  lastMqttAttemptMs = now;

  if (mqtt.connect(MQTT_CLIENT_ID, STATUS_TOPIC, 0, true, STATUS_OFFLINE)) {
    mqtt.publish(STATUS_TOPIC, STATUS_ONLINE, true);
    mqtt.subscribe(SESSION_START_TOPIC);
    Serial.println("mqtt: connected");
    return true;
  }

  Serial.printf("mqtt: connect failed, state %d, retrying\n", mqtt.state());
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

  beginWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);

  sensors.begin();
  sensors.setResolution(SENSOR_RESOLUTION_BITS);
  uint8_t found = sensors.getDeviceCount();
  if (found > MAX_PROBES) {
    found = MAX_PROBES;
  }

  probeCount = 0;
  for (uint8_t i = 0; i < found; i++) {
    if (sensors.getAddress(probeAddress[probeCount], i)) {
      probeCount++;
    }
  }
  sortProbes(probeCount);
  sessionStartMs = millis();

  Serial.println();
  Serial.printf("found %u probe(s) on GPIO %u\n", probeCount, ONE_WIRE_PIN);
  for (uint8_t i = 0; i < probeCount; i++) {
    Serial.printf("  probe %u -> ", i);
    printAddress(probeAddress[i]);
    Serial.println();
  }
}

void loop() {
  const bool online = wifiReady() && ensureMqtt();
  if (online) {
    mqtt.loop();
  }

  const unsigned long now = millis();
  if (now - lastReadMs < READ_INTERVAL_MS) {
    return;
  }
  lastReadMs = now;

  sensors.requestTemperatures();

  const unsigned long elapsedSeconds = (now - sessionStartMs) / 1000;
  if (online) {
    char elapsed[16];
    snprintf(elapsed, sizeof(elapsed), "%lu", elapsedSeconds);
    mqtt.publish(SESSION_ELAPSED_TOPIC, elapsed);
  }

  for (uint8_t i = 0; i < probeCount; i++) {
    const float celsius = sensors.getTempC(probeAddress[i]);
    if (celsius <= DEVICE_DISCONNECTED_C) {
      Serial.printf("probe %u: disconnected\n", i);
      continue;
    }

    char topic[48];
    char payload[16];
    snprintf(topic, sizeof(topic), "coffee/sensor/%u/temperature", i);
    snprintf(payload, sizeof(payload), "%.2f", celsius);

    if (online) {
      mqtt.publish(topic, payload);
    }
    Serial.printf("probe %u: %s C\n", i, payload);
  }
}
