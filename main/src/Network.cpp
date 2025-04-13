#include "settings.h"
#include "global.h"
#include "support.h"
#include "WebServer.h"

#include "Network_.h"

// Return the bitfield containing the type of consumers currently using the network
uint16_t networkGetConsumerTypes()
{
    uint16_t consumerTypes = 0;

    networkConsumers(&consumerTypes);

    return (consumerTypes);
}

// Return the count of consumers (TCP, NTRIP Client, etc) that are enabled
// and set consumerTypes bitfield
// From this number we can decide if the network (WiFi, ethernet, cellular, etc) needs to be started
// ESP-NOW is not considered a network consumer and is blended during wifiConnect()
uint8_t networkConsumers()
{
    uint16_t consumerTypes = 0;

    return (networkConsumers(&consumerTypes));
}

uint8_t networkConsumers(uint16_t *consumerTypes)
{
    uint8_t consumerCount = 0;
    uint16_t consumerId = 0; // Used to debug print who is asking for access

    *consumerTypes = NETIF_NONE; // Clear bitfield

    // If a consumer needs the network or is currently consuming the network (is online) then increment
    // consumer count

    // Network needed for NTRIP Client
    // if (ntripClientNeedsNetwork() || online.ntripClient)
    // {
    //     consumerCount++;
    //     consumerId |= (1 << NETCONSUMER_NTRIP_CLIENT);

    //     *consumerTypes = NETWORK_EWC; // Ask for eth/wifi/cellular
    // }

    // // Network needed for NTRIP Server
    // bool ntripServerOnline = false;
    // for (int index = 0; index < NTRIP_SERVER_MAX; index++)
    // {
    //     if (online.ntripServer[index])
    //     {
    //         ntripServerOnline = true;
    //         break;
    //     }
    // }

    // if (ntripServerNeedsNetwork() || ntripServerOnline)
    // {
    //     consumerCount++;
    //     consumerId |= (1 << NETCONSUMER_NTRIP_SERVER);

    //     *consumerTypes = NETWORK_EWC; // Ask for eth/wifi/cellular
    // }

    // // Network needed for TCP Client
    // if (tcpClientNeedsNetwork() || online.tcpClient)
    // {
    //     consumerCount++;
    //     consumerId |= (1 << NETCONSUMER_TCP_CLIENT);
    //     *consumerTypes = NETWORK_EWC; // Ask for eth/wifi/cellular
    // }

    // // Network needed for TCP Server
    // if (tcpServerNeedsNetwork() || online.tcpServer)
    // {
    //     consumerCount++;
    //     consumerId |= (1 << NETCONSUMER_TCP_SERVER);

    //     *consumerTypes = NETWORK_EWC; // Ask for eth/wifi/cellular

    //     // If NTRIP Caster is enabled then add AP mode
    //     // Caster is available over ethernet, WiFi AP, WiFi STA, and cellular
    //     // Caster is available in all mode: Rover, and Base
    //     if (settings.enableNtripCaster == true || settings.baseCasterOverride == true)
    //         *consumerTypes |= (1 << NETIF_WIFI_AP);
    // }

    // // Network needed for UDP Server
    // if (udpServerNeedsNetwork() || online.udpServer)
    // {
    //     consumerCount++;
    //     consumerId |= (1 << NETCONSUMER_UDP_SERVER);
    //     *consumerTypes = NETWORK_EWC; // Ask for eth/wifi/cellular
    // }

    // // Network needed for PointPerfect ZTP or key update requested by scheduler, from menu, or display menu
    // if (provisioningNeedsNetwork() || online.httpClient)
    // {
    //     consumerCount++;
    //     consumerId |= (1 << NETCONSUMER_PPL_KEY_UPDATE);
    //     *consumerTypes = NETWORK_EWC; // Ask for eth/wifi/cellular
    // }

    // // Network needed for PointPerfect Corrections MQTT client
    // if (mqttClientNeedsNetwork() || online.mqttClient)
    // {
    //     // PointPerfect is enabled, allow MQTT to begin
    //     consumerCount++;
    //     consumerId |= (1 << NETCONSUMER_PPL_MQTT_CLIENT);
    //     *consumerTypes = NETWORK_EWC; // Ask for eth/wifi/cellular
    // }

    // // Network needed to obtain the latest firmware version or do a firmware update
    // if (otaNeedsNetwork() || online.otaClient)
    // {
    //     consumerCount++;
    //     consumerId |= (1 << NETCONSUMER_OTA_CLIENT);
    //     *consumerTypes = (1 << NETIF_WIFI_STA); // OTA Pull library only supports WiFi
    // }

    // Network needed for Web Config
    if (webServerNeedsNetwork() || online.webServer)
    {
        consumerCount++;
        consumerId |= (1 << NETCONSUMER_WEB_CONFIG);

        *consumerTypes = NETWORK_EWC; // Ask for eth/wifi/cellular

        if (settings.wifiConfigOverAP == true)
            *consumerTypes |= (1 << NETIF_WIFI_AP); // WebConfig requires both AP and STA (for firmware check)

        // // A good number of RTK products have only WiFi
        // // If WiFi STA has failed or we have no WiFi SSIDs, fall back to WiFi AP, but allow STA to keep hunting
        // if (networkIsPresent(NETWORK_ETHERNET) == false && networkIsPresent(NETWORK_CELLULAR) == false &&
        //     settings.wifiConfigOverAP == false && (wifiGetStartTimeout() > 0 || wifiNetworkCount() == 0))
        // {
        //     *consumerTypes |= (1 << NETIF_WIFI_AP); // Re-allow Webconfig over AP
        // }
    }

    // Debug
    // if (settings.debugNetworkLayer)
    // {
    //     static unsigned long lastPrint = 0;

    //     if (millis() - lastPrint > 2000)
    //     {
    //         lastPrint = millis();
    //         systemPrintf("Network consumer count: %d ", consumerCount);
    //         if (consumerCount > 0)
    //         {
    //             systemPrintf("- Consumers: ", consumerCount);

    //             if (consumerId & (1 << NETCONSUMER_NTRIP_CLIENT))
    //                 systemPrint("Rover NTRIP Client, ");
    //             if (consumerId & (1 << NETCONSUMER_NTRIP_SERVER))
    //                 systemPrint("Base NTRIP Server, ");
    //             if (consumerId & (1 << NETCONSUMER_TCP_CLIENT))
    //                 systemPrint("TCP Client, ");
    //             if (consumerId & (1 << NETCONSUMER_TCP_SERVER))
    //                 systemPrint("TCP Server, ");
    //             if (consumerId & (1 << NETCONSUMER_UDP_SERVER))
    //                 systemPrint("UDP Server, ");
    //             if (consumerId & (1 << NETCONSUMER_PPL_KEY_UPDATE))
    //                 systemPrint("PPL Key Update Request, ");
    //             if (consumerId & (1 << NETCONSUMER_PPL_MQTT_CLIENT))
    //                 systemPrint("PPL MQTT Client, ");
    //             if (consumerId & (1 << NETCONSUMER_OTA_CLIENT))
    //                 systemPrint("OTA Version Check or Update, ");
    //             if (consumerId & (1 << NETCONSUMER_WEB_CONFIG))
    //                 systemPrint("Web Config, ");
    //         }
    //         systemPrintln();
    //     }
    // }

    return (consumerCount);
}


