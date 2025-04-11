#include "WiFiManager.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <ESPAsyncWebServer.h>
#include "Update.h"
#include "LittleFS.h"

#include "nvs.h"
#include "nvs_flash.h"
// static const char *TAG = "WiFiManager";
static esp_netif_t *ap_netif = NULL;
static char ip_address[16] = "192.168.4.1"; // Default AP IP

void initWiFiAP(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default AP netif
    ap_netif = esp_netif_create_default_wifi_ap();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure WiFi in AP mode
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .password = WIFI_AP_PASSWORD,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_AP_CHANNEL,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = WIFI_AP_MAX_CONN,

        }};

    // If no password is set, use open auth mode
    if (strlen(WIFI_AP_PASSWORD) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // Set WiFi mode and config
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Get the IP info of the AP netif
    esp_netif_ip_info_t ip_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(ap_netif, &ip_info));

    // Convert IP to string
    snprintf(ip_address, sizeof(ip_address), IPSTR, IP2STR(&ip_info.ip));

    // ESP_LOGI(TAG, "WiFi AP started with SSID: %s", WIFI_AP_SSID);
    // ESP_LOGI(TAG, "IP address: %s", ip_address);
}

const char *getIPAddress(void)
{
    return ip_address;
}