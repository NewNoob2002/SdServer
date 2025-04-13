#pragma once

typedef struct Settings
{
    bool debugWebServer = true;
    bool enableCaptivePortal = true;
    bool wifiConfigOverAP = true;
    bool debugWifiState = true;

    bool printTaskStartStop = true;
    unsigned long wifiConnectTimeoutMs = 1000;
}settings_t;

typedef struct struct_tasks
{
    volatile bool gnssUartPinnedTaskRunning = false;
    volatile bool i2cPinnedTaskRunning = false;
    volatile bool bluetoothCommandTaskRunning = false;
    volatile bool btReadTaskRunning = false;
    volatile bool buttonCheckTaskRunning = false;
    volatile bool gnssReadTaskRunning = false;
    volatile bool handleGnssDataTaskRunning = false;
    volatile bool idleTask0Running = false;
    volatile bool idleTask1Running = false;
    volatile bool sdSizeCheckTaskRunning = false;
    volatile bool updatePplTaskRunning = false;
    volatile bool updateWebServerTaskRunning = false;

    bool bluetoothCommandTaskStopRequest = false;
    bool btReadTaskStopRequest = false;
    bool buttonCheckTaskStopRequest = false;
    bool gnssReadTaskStopRequest = false;
    bool handleGnssDataTaskStopRequest = false;
    bool sdSizeCheckTaskStopRequest = false;
    bool updatePplTaskStopRequest = false;
    bool updateWebServerTaskStopRequest = false;
} task_t;

typedef enum _NetworkType
{
    NETIF_NONE = 0, // No consumers 0b0000
    NETIF_WIFI_STA, // The consumer can use STA 0b0001
    NETIF_WIFI_AP, // The consumer can use AP 0b0010
    NETIF_CELLULAR, // The consumer can use Cellular 0b0011
    NETIF_ETHERNET, // The consumer can use Ethernet 0b0100
    NETIF_UNKNOWN
}NetworkType;

#define NETWORK_EWC ((1 << NETIF_ETHERNET) | (1 << NETIF_WIFI_STA) | (1 << NETIF_CELLULAR))

typedef enum _NETCONSUMER
{
    NETCONSUMER_NTRIP_CLIENT = 0,
    NETCONSUMER_NTRIP_SERVER,
    NETCONSUMER_TCP_CLIENT,
    NETCONSUMER_TCP_SERVER,
    NETCONSUMER_UDP_SERVER,
    NETCONSUMER_PPL_KEY_UPDATE,
    NETCONSUMER_PPL_MQTT_CLIENT,
    NETCONSUMER_OTA_CLIENT,
    NETCONSUMER_WEB_CONFIG,
}_NETCONSUMER;

extern settings_t settings;
extern task_t task;