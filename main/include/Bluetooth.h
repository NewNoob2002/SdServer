#pragma once

#include "bluetoothSelect.h"
// #include "settings.h"
#include "SparkFun_Extensible_Message_Parser.h"

typedef enum BTState
{
    BT_OFF = 0,
    BT_NOTCONNECTED,
    BT_CONNECTED,
} BTState;

typedef enum BluetoothRadioType_e
{
    BLUETOOTH_RADIO_SPP = 0,
    BLUETOOTH_RADIO_BLE,
    BLUETOOTH_RADIO_SPP_AND_BLE,
    BLUETOOTH_RADIO_OFF,
} BluetoothRadioType_e;

#define BLE_SERVICE_UUID "8A82EA9C-A25C-7960-4209-2FC5CB624273"
#define BLE_RX_UUID "1E495AA7-AF1B-E561-1E39-7F5F21B5661C"
#define BLE_TX_UUID "F9E6A225-8D54-975E-C899-82C461A46077"

#define BLE_COMMAND_SERVICE_UUID "7e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_COMMAND_RX_UUID "7e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_COMMAND_TX_UUID "7e400003-b5a3-f393-e0a9-e50e24dcca9e"

#define AMOUNT_OF_RING_BUFFER_TO_DISCARD (settings.ringBufferSizeofBT >> 2)
#define AVERAGE_LENGTH_IN_BYTES 64

BTState bluetoothGetState();

int bluetoothRead(uint8_t *buffer, int length);

// Determine if data is available
int bluetoothRxDataAvailable();

// Write data to the Bluetooth device
int bluetoothWrite(const uint8_t *buffer, int length);

// Write data to the Bluetooth device
int bluetoothWrite(uint8_t value);

// Flush Bluetooth device
void bluetoothFlush();
void tickerBluetoothLedUpdate();

// Test each interface to see if there is a connection
// Return true if one is
bool bluetoothIsConnected();
// Check if Bluetooth is connected
void bluetoothUpdate(void *e);
void bluetoothStart();
void bluetoothStop();

const char *const btParserNames[] = {
    "BluetoothAPP",
    "BluetoothRTCM"
};
const int btParserNameCount = sizeof(btParserNames) / sizeof(btParserNames[0]);

// void btDataProcess(SEMP_PARSE_STATE *parse, uint16_t type);

extern uint16_t *rbOffsetArray_BT;
extern uint16_t rbOffsetEntries_BT;

extern uint8_t *ringBuffer_BT;