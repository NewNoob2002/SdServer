#include "Arduino.h"
#include "time.h"
// static struct timeval time_new, time_old;
// static long data_num = 0;
#include "global.h"
#include "support.h"
uint8_t btFadeLevel = 0;
#include "Bluetooth.h"

static BluetoothRadioType_e bluetoothRadioType = BLUETOOTH_RADIO_BLE;
static volatile BTState bluetoothState = BT_OFF;

BTSerialInterface *bluetoothSerialSpp = nullptr;
BTSerialInterface *bluetoothSerialBle = nullptr;
TaskHandle_t bluetoothUpdateHandle = nullptr;

uint16_t *rbOffsetArray_BT = nullptr;
uint16_t rbOffsetEntries_BT = 0;

uint8_t *ringBuffer_BT = nullptr;

char deviceName[32] = {'\0'};
// static void print_speed(void)
// {
//     float time_old_s = time_old.tv_sec + time_old.tv_usec / 1000000.0;
//     float time_new_s = time_new.tv_sec + time_new.tv_usec / 1000000.0;
//     float time_interval = time_new_s - time_old_s;
//     float speed = data_num * 8 / time_interval / 1000.0;
//     ESP_LOGI("SPP", "speed(%fs ~ %fs): %f kbit/s", time_old_s, time_new_s, speed);
//     data_num = 0;
//     time_old.tv_sec = time_new.tv_sec;
//     time_old.tv_usec = time_new.tv_usec;
// }

BTState bluetoothGetState()
{
    return bluetoothState;
}

int bluetoothRead(uint8_t *buffer, int length)
{
    if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
    {
        int bytesRead = 0;

        // Give incoming BLE the priority
        bytesRead = bluetoothSerialBle->readBytes(buffer, length);

        if (bytesRead > 0)
            return (bytesRead);

        bytesRead = bluetoothSerialSpp->readBytes(buffer, length);

        return (bytesRead);
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
        return bluetoothSerialSpp->readBytes(buffer, length);
    else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        return bluetoothSerialBle->readBytes(buffer, length);

    return 0;
}

// Determine if data is available
int bluetoothRxDataAvailable()
{
    if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
    {
        // Give incoming BLE the priority
        if (bluetoothSerialBle->available())
            return (bluetoothSerialBle->available());

        return (bluetoothSerialSpp->available());
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
        return bluetoothSerialSpp->available();
    else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        return bluetoothSerialBle->available();

    return (0);
}

// Write data to the Bluetooth device
int bluetoothWrite(const uint8_t *buffer, int length)
{
    if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
    {
        // Write to both interfaces
        int bleWrite = bluetoothSerialBle->write(buffer, length);
        int sppWrite = bluetoothSerialSpp->write(buffer, length);

        // We hope and assume both interfaces pass the same byte count
        // through their respective stacks
        // If not, report the larger number
        if (bleWrite >= sppWrite)
            return (bleWrite);
        return (sppWrite);
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
    {
        return bluetoothSerialSpp->write(buffer, length);
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
    {
        // BLE write does not handle 0 length requests correctly
        if (length > 0)
            return bluetoothSerialBle->write(buffer, length);
        return length;
    }

    return 0;
}

// Write data to the Bluetooth device
int bluetoothWrite(uint8_t value)
{
    if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
    {
        // Write to both interfaces
        int bleWrite = bluetoothSerialBle->write(value);
        int sppWrite = bluetoothSerialSpp->write(value);

        // We hope and assume both interfaces pass the same byte count
        // through their respective stacks
        // If not, report the larger number
        if (bleWrite >= sppWrite)
            return (bleWrite);
        return (sppWrite);
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
    {
        return bluetoothSerialSpp->write(value);
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
    {
        return bluetoothSerialBle->write(value);
    }
    return 0;
}

// Flush Bluetooth device
void bluetoothFlush()
{
    if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
    {
        bluetoothSerialBle->flush();
        bluetoothSerialSpp->flush();
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
        bluetoothSerialSpp->flush();
    else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        bluetoothSerialBle->flush();
}
// Update the Bluetooth LED
// void tickerBluetoothLedUpdate()
// {
//     if (pin_dataStatusLED != PIN_UNDEFINED)
//     {
//         // 如果电台没有工作，才让蓝牙接管数据灯
//         //  if(isCharging() && poweroffCharger)
//         //      return;
//         if (dataLed.currentRate > 0)
//         {
//             const uint32_t now = millis();
//             if (now - dataLed.lastToggleTime >= (uint32_t)dataLed.currentRate)
//             {
//                 dataLed.isOn = !dataLed.isOn;
//                 ledcWrite(pin_dataStatusLED, dataLed.isOn ? 255 : 0);
//                 dataLed.lastToggleTime = now;
//             }
//         }
//         // Blink on/off while we wait for BT connection
//         if (bluetoothGetState() == BT_NOTCONNECTED)
//         {
//             dataLedBlink(300);
//         }
//         // Solid LED if BT Connected
//         else if (bluetoothGetState() == BT_CONNECTED)
//         {
//             dataLedBlink(0);
//             dataLedOff();
//         }
//         else
//             dataLedBlink(0);
//     }
// }

// Test each interface to see if there is a connection
// Return true if one is
bool bluetoothIsConnected()
{
    if (bluetoothGetState() == BT_OFF)
        return (false);

    if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
    {
        if (bluetoothSerialSpp->connected() == true || bluetoothSerialBle->connected() == true)
            return (true);
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
    {
        if (bluetoothSerialSpp->connected() == true)
            return (true);
    }
    else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
    {
        if (bluetoothSerialBle->connected() == true)
            return (true);
    }

    return (false);
}
// Check if Bluetooth is connected
void bluetoothUpdate(void *e)
{
    while (1)
    {
        static uint32_t lastCheck = millis(); // Check if connected every 100ms
        if (millis() > (lastCheck + 100))
        {
            lastCheck = millis();

            // If bluetoothState == BT_OFF, don't call bluetoothIsConnected()

            if ((bluetoothState == BT_NOTCONNECTED) && (bluetoothIsConnected()))
            {
                systemPrintf("BT connected\r\n");
                bluetoothState = BT_CONNECTED;
            }
            else if ((bluetoothState == BT_CONNECTED) && (!bluetoothIsConnected()))
            {
                systemPrintf("BT disconnected\r\n");
                bluetoothState = BT_NOTCONNECTED;
            }
        }
        delay(10);
        taskYIELD();
    }
}
void bluetoothStart()
{
    if (bluetoothRadioType == BLUETOOTH_RADIO_OFF)
        return;

    if (!online.bluetooth)
    {
        bluetoothState = BT_OFF;
        char productName[12] = {0};
        strncpy(productName, "ble-esp32", sizeof(productName));

        // BLE is limited to ~28 characters in the device name. Shorten platformPrefix if needed.
        if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE ||
            bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        {
            strncpy(productName, "E1-Lite*Ble", sizeof(productName));
        }

        snprintf(deviceName, sizeof(deviceName), "%s\n", productName);

        if (strlen(deviceName) > 28)
        {
            systemPrintf(
                "Warning! The Bluetooth device name '%s' is %d characters long. It may not work in BLE mode.\r\n",
                deviceName, strlen(deviceName));
        }

        // Select Bluetooth setup
        if (bluetoothRadioType == BLUETOOTH_RADIO_OFF)
            return;
        else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
        {
            bluetoothSerialSpp = new BTClassicSerial();
            bluetoothSerialBle = new BTLESerial();
        }
        else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
            bluetoothSerialSpp = new BTClassicSerial();
        else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        {
            bluetoothSerialBle = new BTLESerial();
        }

        // while (bluetoothPinned == false) // Wait for task to run once
        //     delay(1);

        bool beginSuccess = true;
        if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
        {
            beginSuccess &= bluetoothSerialSpp->begin(
                deviceName, false, false, 1024, 1024, 0, 0,
                0); // localName, isMaster, disableBLE, rxBufferSize, txBufferSize, serviceID, rxID, txID

            beginSuccess &= bluetoothSerialBle->begin(
                deviceName, false, false, 1024, 1024, BLE_SERVICE_UUID,
                BLE_RX_UUID,
                BLE_TX_UUID); // localName, isMaster, disableBLE, rxBufferSize, txBufferSize, serviceID, rxID, txID
        }
        else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
        {
            // Disable BLE
            beginSuccess &= bluetoothSerialSpp->begin(
                deviceName, false, true, 1024, 1024, 0, 0,
                0); // localName, isMaster, disableBLE, rxBufferSize, txBufferSize, serviceID, rxID, txID
        }
        else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        {
            // Don't disable BLE
            beginSuccess &= bluetoothSerialBle->begin(
                deviceName, false, false, 1024, 1024, BLE_SERVICE_UUID,
                BLE_RX_UUID,
                BLE_TX_UUID); // localName, isMaster, disableBLE, rxBufferSize, txBufferSize, serviceID, rxID, txID
        }
        if (beginSuccess == false)
        {

            printf("An error occurred initializing Bluetooth");
            return;
        }
        // Set PIN to 1234 so we can connect to older BT devices, but not require a PIN for modern device pairing
        // See issue: https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/5
        // https://github.com/espressif/esp-idf/issues/1541
        //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        // Note: Since version 3.0.0 this library does not support legacy pairing (using fixed PIN consisting of 4
        // digits). esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;

        // esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE; // Requires pin 1234 on old BT dongle, No prompt on new BT dongle
        // // esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_OUT; //Works but prompts for either pin (old) or 'Does this 6 pin
        // // appear on the device?' (new)

        // esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
        // esp_bt_pin_code_t pin_code;
        // pin_code[0] = '1';
        // pin_code[1] = '2';
        // pin_code[2] = '3';
        // pin_code[3] = '4';
        // esp_bt_gap_set_pin(pin_type, 4, pin_code);

        //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
        {
            // Bluetooth callbacks are handled by bluetoothUpdate()
            bluetoothSerialSpp->setTimeout(250);
            bluetoothSerialBle->setTimeout(10); // Using 10 from BleSerial example
        }
        else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
        {
            // Bluetooth callbacks are handled by bluetoothUpdate()
            bluetoothSerialSpp->setTimeout(250);
        }
        else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        {
            // Bluetooth callbacks are handled by bluetoothUpdate()
            bluetoothSerialBle->setTimeout(10);
        }

        if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
            printf("Bluetooth SPP and BLE broadcasting as: ");
        else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
            printf("Bluetooth SPP broadcasting as: ");
        else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
            printf("Bluetooth Low-Energy broadcasting as: ");

        printf(deviceName);

        // Start BLE Command Task if BLE is enabled
        if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE ||
            bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        {
        }
        xTaskCreatePinnedToCore(bluetoothUpdate,
                                "bluetoothUpdate",
                                2048,
                                NULL,
                                10,
                                &bluetoothUpdateHandle,
                                1);
        bluetoothState = BT_NOTCONNECTED;
        online.bluetooth = true;
        // Create the ring buffer
        // size_t length;
        // // Determine the length of data to be retained in the ring buffer
        // // after discarding the oldest data
        // length = settings.ringBufferSizeofBT;
        // rbOffsetEntries_BT = (length >> 1) / AVERAGE_LENGTH_IN_BYTES;
        // length = settings.ringBufferSizeofBT + (rbOffsetEntries_BT * sizeof(uint16_t));

        // if (rbOffsetArray_BT == nullptr)
        //     rbOffsetArray_BT = (uint16_t *)malloc(length);

        // if (!rbOffsetArray_BT)
        // {
        //     rbOffsetEntries_BT = 0;
        //     systemPrintln("ERROR: Failed to allocate the BT ring buffer!");
        // }
        // else
        // {
        //     ringBuffer_BT = (uint8_t *)&rbOffsetArray_BT[rbOffsetEntries_BT];
        //     rbOffsetArray_BT[0] = 0;
        // }
    }
}
void bluetoothStop()
{
    if (online.bluetooth)
    {
        if (bluetoothRadioType == BLUETOOTH_RADIO_SPP_AND_BLE)
        {
            bluetoothSerialBle->flush();      // Complete any transfers
            bluetoothSerialBle->disconnect(); // Drop any clients
            bluetoothSerialBle->end();        // Release resources

            bluetoothSerialSpp->flush();      // Complete any transfers
            bluetoothSerialSpp->disconnect(); // Drop any clients
            bluetoothSerialSpp->end();        // Release resources
        }
        else if (bluetoothRadioType == BLUETOOTH_RADIO_SPP)
        {
            bluetoothSerialSpp->flush();      // Complete any transfers
            bluetoothSerialSpp->disconnect(); // Drop any clients
            bluetoothSerialSpp->end();        // Release resources
        }
        else if (bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        {
            bluetoothSerialBle->flush();      // Complete any transfers
            bluetoothSerialBle->disconnect(); // Drop any clients
            bluetoothSerialBle->end();        // Release resources
        }

        log_d("Bluetooth turned off");
        if (rbOffsetArray_BT != nullptr)
        {
            free(rbOffsetArray_BT);
            rbOffsetArray_BT = nullptr;
            ringBuffer_BT = nullptr;
        }
        bluetoothState = BT_OFF;
    }
    // bluetoothIncomingRTCM = false;
}