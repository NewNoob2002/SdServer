#pragma once

#include "esp_wifi.h"
#include "esp_event.h"

// WiFi AP Configuration
#define WIFI_AP_SSID      "ESP32_WebServer"
#define WIFI_AP_PASSWORD  "password123"
#define WIFI_AP_CHANNEL   1
#define WIFI_AP_MAX_CONN  4

// Initialize WiFi in AP mode
void initWiFiAP(void);

// Get IP address as string
const char* getIPAddress(void); 