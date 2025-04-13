#pragma once
#include <WiFi.h>
#include "nvs.h"
#include "nvs_flash.h"

// WiFi AP Configuration
#define WIFI_AP_SSID      "ESP32_WebServer"
#define WIFI_AP_PASSWORD  "password123"
#define WIFI_AP_CHANNEL   1
#define WIFI_AP_MAX_CONN  4

static int wifiFailedConnectionAttempts = 0;

#define WIFI_STOP()                                                                                                    \
    {                                                                                                                  \
        if (settings.debugWifiState)                                                                                   \
            systemPrintf("wifiStop called by %s %d\r\n", __FILE__, __LINE__);                                          \
        wifiStop();                                                                                                    \
    }
// Initialize WiFi in AP mode
// void initWiFiAP(void);

// Get IP address as string
const char* getIPAddress(void); 

bool wifiStart();

//*********************************************************************
// Stop WiFi and release all resources
void wifiStop();

//----------------------------------------
// Starts WiFi in STA, AP, or STA_AP mode depending on bools
// Returns true if STA connects, or if AP is started
//----------------------------------------
bool wifiConnect(bool startWiFiStation, bool startWiFiAP, unsigned long timeout);

//----------------------------------------
// Determine if WIFI is connected
//----------------------------------------
bool wifiIsConnected();

// Determine if WiFi Station is running
//----------------------------------------
bool wifiStationIsRunning();

//----------------------------------------
// Determine if WiFi AP is running
//----------------------------------------
bool wifiApIsRunning();

//----------------------------------------
// Determine if either Station or AP is running
//----------------------------------------
bool wifiIsRunning();

//----------------------------------------
//----------------------------------------
uint32_t wifiGetStartTimeout();