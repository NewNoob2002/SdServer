#include <Arduino.h>

#include "support.h"
#include "global.h"
#include "WiFiManager.h"
#include "WebServer.h"
#include "Bluetooth.h"

#include "state.h"

static uint32_t lastStateTime = 0;

extern bool websocketConnected;

static SystemState lastState = STATE_NOT_SET;

void requestChangeState(SystemState requestedState)
{
    newSystemStateRequested = true;
    requestedSystemState = requestedState;
    log_d("Requested System State: %d", requestedSystemState);
}

// Print the current state
const char *getState(SystemState state, char *buffer)
{
    switch (state)
    {
    case (STATE_ROVER_NOT_STARTED):
        return "STATE_ROVER_NOT_STARTED";
        // case (STATE_ROVER_NO_FIX):
        //     return "STATE_ROVER_NO_FIX";
        // case (STATE_ROVER_FIX):
        //     return "STATE_ROVER_FIX";
        // case (STATE_ROVER_RTK_FLOAT):
        //     return "STATE_ROVER_RTK_FLOAT";
        // case (STATE_ROVER_RTK_FIX):
        //     return "STATE_ROVER_RTK_FIX";
        // case (STATE_BASE_CASTER_NOT_STARTED):
        //     return "STATE_BASE_CASTER_NOT_STARTED";
        // case (STATE_BASE_NOT_STARTED):
        //     return "STATE_BASE_NOT_STARTED";
        // case (STATE_BASE_TEMP_SETTLE):
        //     return "STATE_BASE_TEMP_SETTLE";
        // case (STATE_BASE_TEMP_SURVEY_STARTED):
        //     return "STATE_BASE_TEMP_SURVEY_STARTED";
        // case (STATE_BASE_TEMP_TRANSMITTING):
        //     return "STATE_BASE_TEMP_TRANSMITTING";
        // case (STATE_BASE_FIXED_NOT_STARTED):
        //     return "STATE_BASE_FIXED_NOT_STARTED";
        // case (STATE_BASE_FIXED_TRANSMITTING):
        //     return "STATE_BASE_FIXED_TRANSMITTING";
        // case (STATE_DISPLAY_SETUP):
        return "STATE_DISPLAY_SETUP";
    case (STATE_WEB_CONFIG_NOT_STARTED):
        return "STATE_WEB_CONFIG_NOT_STARTED";
    case (STATE_WEB_CONFIG_WAIT_FOR_NETWORK):
        return "STATE_WEB_CONFIG_WAIT_FOR_NETWORK";
    case (STATE_WEB_CONFIG):
        return "STATE_WEB_CONFIG";
    // case (STATE_TEST):
    //     return "STATE_TEST";
    // case (STATE_TESTING):
    //     return "STATE_TESTING";
    case (STATE_PROFILE):
        return "STATE_PROFILE";

        // case (STATE_KEYS_REQUESTED):
        //     return "STATE_KEYS_REQUESTED";

        // case (STATE_ESPNOW_PAIRING_NOT_STARTED):
        //     return "STATE_ESPNOW_PAIRING_NOT_STARTED";
        // case (STATE_ESPNOW_PAIRING):
        //     return "STATE_ESPNOW_PAIRING";

        // case (STATE_NTPSERVER_NOT_STARTED):
        //     return "STATE_NTPSERVER_NOT_STARTED";
        // case (STATE_NTPSERVER_NO_SYNC):
        //     return "STATE_NTPSERVER_NO_SYNC";
        // case (STATE_NTPSERVER_SYNC):
        //     return "STATE_NTPSERVER_SYNC";

    case (STATE_SHUTDOWN):
        return "STATE_SHUTDOWN";
    case (STATE_NOT_SET):
        return "STATE_NOT_SET";
    }

    // Handle the unknown case
    sprintf(buffer, "Unknown: %d", state);
    return buffer;
}

void beginSystemState()
{
    if (systemState > STATE_NOT_SET)
    {
        systemPrintln("Unknown state - factory reset");
        // factoryReset(false); // We do not have the SD semaphore
    }
    // Set the default previous state
    if (lastState == STATE_NOT_SET) // Default
    {
        systemState = STATE_ROVER_NOT_STARTED;
        lastState = systemState;
    }

    // Return to either Base or Rover Not Started. The last state previous to power down.
    systemState = lastState;

    //   // If the setting is not set, override with default
    //   if (settings.antennaPhaseCenter_mm == 0.0)
    //     settings.antennaPhaseCenter_mm = present.antennaPhaseCenter_mm;
}

void stateUpdate(void *e)
{
    while (1)
    {
        if (millis() - lastSystemStateUpdate > 500 || forceSystemStateUpdate == true)
        {
            lastSystemStateUpdate = millis();
            forceSystemStateUpdate = false;

            // Check to see if any external sources need to change state
            if (newSystemStateRequested == true)
            {
                newSystemStateRequested = false;
                if (systemState != requestedSystemState)
                {
                    changeState(requestedSystemState);
                    lastStateTime = millis();
                }
            }

            if ((millis() - lastStateTime) > 15000)
            {
                changeState(systemState);
                lastStateTime = millis();
            }

            // Move between states as needed
            changeState(systemState);
            switch (systemState)
            {
            case (STATE_ROVER_NOT_STARTED):
            {
                // bluetoothStart();
                break;
            }

            case (STATE_WEB_CONFIG_NOT_STARTED):
            {
                bluetoothStop();
                if ((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() == WIFI_AP_STA))
                {
                    // if (strcmp(WiFi.softAPSSID().c_str(), "RTK Config") != 0)
                    // {
                    //     // The AP name cannot be changed while it is running. WiFi must be restarted.
                    //     restartWiFi = true; // Tell network layer to restart WiFi
                    // }
                }
                
                webServerStart(); // Start the webserver state machine for web config
                
                // RTK_MODE(RTK_MODE_WEB_CONFIG);
                changeState(STATE_WEB_CONFIG_WAIT_FOR_NETWORK);
                break;
            }
            case (STATE_WEB_CONFIG_WAIT_FOR_NETWORK):
            {
                wifiStart();
                changeState(STATE_WEB_CONFIG);
                break;
            }
            case (STATE_WEB_CONFIG):
            {

                break;
            }
            case (STATE_PROFILE):
            {
                break;
            }
            case (STATE_SHUTDOWN):
            {
                break;
            }
            case (STATE_NOT_SET):
            {
            }
            }
        }
        delay(10);
    }
}

// Change states and print the new state
void changeState(SystemState newState)
{
    char string1[30];
    char string2[30];
    const char *arrow = "";
    const char *asterisk = "";
    const char *initialState = "";
    const char *endingState = "";

    // Log the heap size at the state change
    reportHeapNow(false);

    // Debug print of new state, add leading asterisk for repeated states
    if ((!false) && (newState == systemState))
        return;

    if (true)
    {
        arrow = "";
        asterisk = "";
        initialState = "";
        if (newState == systemState)
            asterisk = "*";
        else
        {
            initialState = getState(systemState, string1);
            arrow = " --> ";
        }
    }

    // Set the new state
    systemState = newState;
    if (true)
    {
        endingState = getState(newState, string2);

        if (1)
            systemPrintf("%s%s%s%s\r\n", asterisk, initialState, arrow, endingState);
        // else
        // {
        //     // Timestamp the state change
        //     //          1         2
        //     // 12345678901234567890123456
        //     // YYYY-mm-dd HH:MM:SS.xxxrn0
        //     struct tm timeinfo = rtc.getTimeStruct();
        //     char s[30];
        //     strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &timeinfo);
        //     systemPrintf("%s%s%s%s, %s.%03ld\r\n", asterisk, initialState, arrow, endingState, s, rtc.getMillis());
        // }
    }
}

bool inWebConfigMode()
{
    if (systemState >= STATE_WEB_CONFIG_NOT_STARTED && systemState <= STATE_WEB_CONFIG)
        return (true);
    return (false);
}