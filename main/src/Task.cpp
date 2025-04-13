#include "Arduino.h"
#include "esp_mac.h"
#include "Ticker.h"
Ticker bluetoothLedTask;
Ticker gnssLedTask;
Ticker powerLedTask;
Ticker chargerLedTask;
Ticker functionKeyLedTask;
#include "support.h"
// #include "settings.h"
#include "global.h"
#include "Bluetooth.h"
// #include "log.h"
#include "state.h"
#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

SEMP_PARSE_STATE *Btparse = nullptr;

/// @brief Bluetooth parser
SEMP_PARSE_ROUTINE const btParserTable[] = {
    sempBluetoothPreamble,
    sempRtcmPreamble,
};
const int btParserCount = sizeof(btParserTable) / sizeof(btParserTable[0]);

// void btReadTask(void *e)
// {
//     int rxBytes = 0;
//     Btparse = sempBeginParser(btParserTable,
//                               btParserCount,
//                               btParserNames,
//                               btParserNameCount,
//                               0,
//                               1024 * 2,
//                               btDataProcess,
//                               "BluetoothDebug");
//     if (!Btparse)
//         systemPrintf("Failed to initialize the Bt parser");
//     taskdisplay.btReadTaskRunning = true;
//     if (settings.printTaskStartStop)
//         systemPrintln("Task btReadTask started");
//     // Run task until a request is raised
//     taskdisplay.btReadTaskStopRequest = false;
//     while (taskdisplay.btReadTaskStopRequest == false)
//     {
//         if (bluetoothGetState() == BT_CONNECTED)
//         {
//             if (bluetoothRxDataAvailable() > 0)
//             {
//                 rxBytes = bluetoothRead(bluetoothRxBuffer, sizeof(bluetoothRxBuffer));
//                 for (int x = 0; x < rxBytes; x++)
//                 {
//                     sempParseNextByte(Btparse, bluetoothRxBuffer[x]);
//                 }
//             }
//             if ((settings.enableTaskReports == true))
//                 systemPrintf("BTReadTask High watermark: %d\r\n", uxTaskGetStackHighWaterMark(nullptr));
//         }
//         delay(1);
//         yield();
//     }

//     sempStopParser(&Btparse);
//     taskdisplay.btReadTaskRunning = false;
//     if (settings.printTaskStartStop)
//         systemPrintln("Task btReadTask stopped");
//     vTaskDelete(NULL);
// }