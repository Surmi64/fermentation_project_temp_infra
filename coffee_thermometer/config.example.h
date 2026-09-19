#pragma once

#define WIFI_SSID "your-network"
#define WIFI_PASSWORD "your-password"

#define MQTT_HOST "192.168.1.10"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "coffee-thermometer"

// true: publish synthetic readings around 60 C, no probes needed
// false: read the real DS18B20 probes on the 1-wire bus
#define TEST_MODE false
