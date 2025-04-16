#include <Arduino.h>
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "esp_netif.h"
#include "esp_wifi.h"
// #include "esp_netif.h"
#include <ESPAsyncWebServer.h>
#include <settings.h>
#include "support.h"
#include "global.h"
#include "state.h"
#include "Network_.h"
#include "WebServer.h"

#include "WiFiManager.h"
// Start timeout
static uint32_t wifiStartTimeout;
// WiFi interface status
static bool wifiApRunning;
static bool wifiStationRunning;

// static esp_netif_t *ap_netif = NULL;
// static char ip_address[16] = "192.168.4.1"; // Default AP IP

void initWiFiAP(void)
{
    const esp_netif_ip_info_t netif_soft_ap_ip = {
        .ip = {.addr = ESP_IP4TOADDR(192, 168, 10, 12)},
        .netmask = {.addr = ESP_IP4TOADDR(255, 255, 255, 0)},
        .gw = {.addr = ESP_IP4TOADDR(192, 168, 10, 12)}

    };
    const esp_netif_inherent_config_t netif_base_cfg =
        {
            .flags = (esp_netif_flags_t)(ESP_NETIF_IPV4_ONLY_FLAGS(ESP_NETIF_DHCP_SERVER) | ESP_NETIF_FLAG_AUTOUP),
            ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(mac)
                .ip_info = &netif_soft_ap_ip,
            .get_ip_event = 0,
            .lost_ip_event = 0,
            .if_key = "WIFI_AP_DEF",
            .if_desc = "ap",
            .route_prio = 10,
            .bridge_info = NULL

        };
    const esp_netif_config_t wifi_netif_ap = {
        .base = &netif_base_cfg,
        .driver = NULL,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_AP,
    };

    esp_netif_t *netif = esp_netif_new(&wifi_netif_ap);
    assert(netif);
    ESP_ERROR_CHECK(esp_netif_attach_wifi_ap(netif));
    ESP_ERROR_CHECK(esp_wifi_set_default_wifi_ap_handlers());

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32",
            .password = "12345678",
            .ssid_len = strlen("ESP32"),
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 2,

        },
    };

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    esp_err_t ret = esp_wifi_start();
    if (ret == ESP_OK)
    {
        ESP_LOGI("wifi_start", "wifi name: %s, wifi password: %s", "ESP32", "12345678");
    }
}

// const char *getIPAddress(void)
// {
//     return ip_address;
// }

bool wifiStart()
{
    int wifiStatus;

    // Determine which parts of WiFi need to be started
    bool startWiFiStation = false;
    bool startWiFiAp = false;

    uint16_t consumerTypes = networkGetConsumerTypes();

    // The consumers need station
    if (consumerTypes & (1 << NETIF_WIFI_STA))
        startWiFiStation = true;

    // The consumers need AP
    if (consumerTypes & (1 << NETIF_WIFI_AP))
        startWiFiAp = true;

    if (startWiFiStation == false && startWiFiAp == false)
    {
        systemPrintln("wifiStart() requested without any NETCONSUMER combination");
        WIFI_STOP();
        return (false);
    }

    // Determine if WiFi is already running
    if (startWiFiStation == wifiStationRunning && startWiFiAp == wifiApRunning)
    {
        if (settings.debugWifiState == true)
            systemPrintln("WiFi is already running with requested setup");
        return (true);
    }

    // Handle special cases if no networks have been entered
    // if (wifiNetworkCount() == 0)
    // {
    //     if (startWiFiStation == true && startWiFiAp == false)
    //     {
    //         systemPrintln("Error: Please enter at least one SSID before using WiFi");
    //         displayNoSSIDs(2000);
    //         WIFI_STOP();
    //         return false;
    //     }
    //     else if (startWiFiStation == true && startWiFiAp == true)
    //     {
    //         systemPrintln("Error: No SSID available to start WiFi Station during AP");
    //         // Allow the system to continue in AP only mode
    //         startWiFiStation = false;
    //     }
    // }

    // Start WiFi

    wifiConnect(startWiFiStation, startWiFiAp, settings.wifiConnectTimeoutMs);

    // If we are in AP only mode, as long as the AP is started, return true
    if (WiFi.getMode() == WIFI_MODE_AP)
        return wifiApIsRunning();

    // If we are in STA or AP+STA mode, return if the station connected successfully
    wifiStatus = WiFi.status();
    return (wifiStatus == WL_CONNECTED);
}

//*********************************************************************
// Stop WiFi and release all resources
void wifiStop()
{
    // Stop the web server
    stopWebServer();

    // Stop the DNS server if we were using the captive portal
    if (((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() == WIFI_AP_STA)) && settings.enableCaptivePortal)
        dnsServer.stop();

    wifiFailedConnectionAttempts = 0; // Reset the counter

    // If ESP-Now is active, change protocol to only Long Range
    // if (espnowGetState() > ESPNOW_OFF)
    // {
    //     if (WiFi.getMode() != WIFI_STA)
    //         WiFi.mode(WIFI_STA);

    //     // Enable long range, PHY rate of ESP32 will be 512Kbps or 256Kbps
    //     // esp_wifi_set_protocol requires WiFi to be started
    //     esp_err_t response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    //     if (response != ESP_OK)
    //         systemPrintf("wifiShutdown: Error setting ESP-Now lone protocol: %s\r\n", esp_err_to_name(response));
    //     else
    //     {
    //         if (settings.debugWifiState == true)
    //             systemPrintln("WiFi disabled, ESP-Now left in place");
    //     }
    // }
    // else
    {
        WiFi.mode(WIFI_OFF);
        if (settings.debugWifiState == true)
            systemPrintln("WiFi Stopped");
    }

    // Take the network offline
    // networkInterfaceEventInternetLost(NETWORK_WIFI);

    // if (wifiMulti != nullptr)
    //     wifiMulti = nullptr;

    // Display the heap state
    reportHeapNow(settings.debugWifiState);
    wifiStationRunning = false;
    wifiApRunning = false;
}

//----------------------------------------
// Starts WiFi in STA, AP, or STA_AP mode depending on bools
// Returns true if STA connects, or if AP is started
//----------------------------------------
bool wifiConnect(bool startWiFiStation, bool startWiFiAP, unsigned long timeout)
{
    // Is a change needed?
    if (startWiFiStation && startWiFiAP && WiFi.getMode() == WIFI_AP_STA && WiFi.status() == WL_CONNECTED)
        return (true); // There is nothing needing to be changed

    if (startWiFiStation && WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED)
        return (true); // There is nothing needing to be changed

    if (startWiFiAP && WiFi.getMode() == WIFI_AP)
        return (true); // There is nothing needing to be changed

    wifiStationRunning = false; // Mark it as offline while we mess about
    wifiApRunning = false;      // Mark it as offline while we mess about

    wifi_mode_t wifiMode = WIFI_OFF;
    wifi_interface_t wifiInterface = WIFI_IF_STA;

    // Establish what WiFi mode we need to be in
    if (startWiFiStation && startWiFiAP)
    {
        systemPrintln("Starting WiFi AP+Station");
        wifiMode = WIFI_AP_STA;
        wifiInterface = WIFI_IF_AP; // There is no WIFI_IF_AP_STA
    }
    else if (startWiFiStation)
    {
        systemPrintln("Starting WiFi Station");
        wifiMode = WIFI_STA;
        wifiInterface = WIFI_IF_STA;
    }
    else if (startWiFiAP)
    {
        systemPrintln("Starting WiFi AP");
        wifiMode = WIFI_AP;
        wifiInterface = WIFI_IF_AP;
    }

    // displayWiFiConnect();

    if (WiFi.mode(wifiMode) == false) // Start WiFi in the appropriate mode
    {
        systemPrintln("WiFi failed to set mode");
        return (false);
    }

    // Verify that the necessary protocols are set
    uint8_t protocols = 0;
    esp_err_t response = esp_wifi_get_protocol(wifiInterface, &protocols);
    if (response != ESP_OK)
        systemPrintf("Failed to get protocols: %s\r\n", esp_err_to_name(response));

    // If ESP-NOW is running, blend in ESP-NOW protocol.
    // if (espnowGetState() > ESPNOW_OFF)
    // {
    //     if (protocols != (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR))
    //     {
    //         esp_err_t response =
    //             esp_wifi_set_protocol(wifiInterface, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N |
    //                                                      WIFI_PROTOCOL_LR); // Enable WiFi + ESP-Now.
    //         if (response != ESP_OK)
    //             systemPrintf("Error setting WiFi + ESP-NOW protocols: %s\r\n", esp_err_to_name(response));
    //     }
    // }
    // else
    {
        // Make sure default WiFi protocols are in place
        if (protocols != (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N))
        {
            esp_err_t response = esp_wifi_set_protocol(wifiInterface, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                                                                          WIFI_PROTOCOL_11N); // Enable WiFi.
            if (response != ESP_OK)
                systemPrintf("Error setting WiFi protocols: %s\r\n", esp_err_to_name(response));
        }
    }

    // Start AP with fixed IP
    if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA)
    {
        IPAddress local_IP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);

        WiFi.softAPConfig(local_IP, gateway, subnet);

        // Determine the AP name
        // If in web config mode then 'RTK Config'
        // otherwise 'RTK Caster'

        char softApSsid[strlen("RTK_Config")]; // Must be short enough to fit OLED Width

        if (inWebConfigMode())
            strncpy(softApSsid, "RTK_Config", sizeof(softApSsid));
        // snprintf("%s", sizeof(softApSsid), softApSsid, (const char)"RTK Config"); // TODO use
        // settings.webconfigApName
        else
            strncpy(softApSsid, "RTK_Caster", sizeof(softApSsid));
        // snprintf("%s", sizeof(softApSsid), softApSsid, (const char*)"RTK Caster");
        if (WiFi.softAP("ESP32Server") == false)
        {
            systemPrintln("WiFi AP failed to start");
            return (false);
        }
        WiFi.begin();
        systemPrintf("WiFi AP %s started with IP: ", "ESP32Server");
        systemPrintln(WiFi.softAPIP());

        // Start DNS Server
        if (dnsServer.start(53, "*", WiFi.softAPIP()) == false)
        {
            systemPrintln("WiFi DNS Server failed to start");
            return (false);
        }
        else
        {
            if (settings.debugWifiState == true)
                systemPrintln("DNS Server started");
        }

        wifiApRunning = true;

        // If we're only here to start the AP, then we're done
        if (wifiMode == WIFI_AP)
            return true;
    }

    systemPrintln("Connecting to WiFi... ");

    // if (wifiMulti == nullptr)
    //     wifiMulti = new WiFiMulti;

    // // Load SSIDs
    // wifiMulti->APlistClean();
    // for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    // {
    //     if (strlen(settings.wifiNetworks[x].ssid) > 0)
    //         wifiMulti->addAP((const char *)&settings.wifiNetworks[x].ssid,
    //                          (const char *)&settings.wifiNetworks[x].password);
    // }

    // int wifiStatus = wifiMulti->run(timeout);
    // if (wifiStatus == WL_CONNECTED)
    // {
    //     wifiResetTimeout(); // If we successfully connected then reset the throttling timeout
    //     wifiStationRunning = true;
    //     return true;
    // }
    // if (wifiStatus == WL_DISCONNECTED)
    //     systemPrint("No friendly WiFi networks detected.\r\n");
    // else
    // {
    //     systemPrintf("WiFi failed to connect: error #%d - %s\r\n", wifiStatus, wifiPrintState((wl_status_t)wifiStatus));
    // }
    // wifiStationRunning = false;
    return false;
}

//----------------------------------------
// Determine if WIFI is connected
//----------------------------------------
bool wifiIsConnected()
{
    bool connected;

    connected = (WiFi.status() == WL_CONNECTED);
    return connected;
}

// Determine if WiFi Station is running
//----------------------------------------
bool wifiStationIsRunning()
{
    return wifiStationRunning;
}

//----------------------------------------
// Determine if WiFi AP is running
//----------------------------------------
bool wifiApIsRunning()
{
    return wifiApRunning;
}

//----------------------------------------
// Determine if either Station or AP is running
//----------------------------------------
bool wifiIsRunning()
{
    if (wifiStationRunning || wifiApRunning)
        return true;
    return false;
}

//----------------------------------------
//----------------------------------------
uint32_t wifiGetStartTimeout()
{
    return (wifiStartTimeout);
}