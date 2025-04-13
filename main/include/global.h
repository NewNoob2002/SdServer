#pragma once

#include <stdint.h>
typedef enum
{
    STATE_ROVER_NOT_STARTED = 0,
    // STATE_ROVER_NO_FIX,
    // STATE_ROVER_FIX,
    // STATE_ROVER_RTK_FLOAT,
    // STATE_ROVER_RTK_FIX,
    // STATE_BASE_CASTER_NOT_STARTED, //Set override flag
    // STATE_BASE_NOT_STARTED,
    // STATE_BASE_TEMP_SETTLE, // User has indicated base, but current pos accuracy is too low
    // STATE_BASE_TEMP_SURVEY_STARTED,
    // STATE_BASE_TEMP_TRANSMITTING,
    // STATE_BASE_FIXED_NOT_STARTED,
    // STATE_BASE_FIXED_TRANSMITTING,

    // STATE_DISPLAY_SETUP,
    STATE_WEB_CONFIG_NOT_STARTED,
    STATE_WEB_CONFIG_WAIT_FOR_NETWORK,
    STATE_WEB_CONFIG,
    STATE_PROFILE,
    // STATE_KEYS_REQUESTED,
    // STATE_ESPNOW_PAIRING_NOT_STARTED,
    // STATE_ESPNOW_PAIRING,
    // STATE_NTPSERVER_NOT_STARTED,
    // STATE_NTPSERVER_NO_SYNC,
    // STATE_NTPSERVER_SYNC,
    STATE_SHUTDOWN,
    STATE_NOT_SET, // Must be last on list
} SystemState;
extern volatile SystemState systemState;
extern volatile SystemState requestedSystemState;

extern uint32_t lastSystemStateUpdate;
extern bool forceSystemStateUpdate; // Set true to avoid update wait
extern bool newSystemStateRequested;

typedef struct _onlineStruct
{
    bool bluetooth = false;
    bool webServer = false;
}online_struct;

extern online_struct online;

const char *const productDisplayNames[] =
    {
        // Keep short - these are displayed on the OLED
        "E1-Lite",
        // Add new values just above this line
        "Unknown"};

void beginButton();