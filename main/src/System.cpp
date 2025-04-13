#include <Arduino.h>
#include <LittleFS.h>
// #include <ESP32Time.h>

#include "support.h"
#include "global.h"
// rtc time
uint32_t lastHeapReport = 0;

void reportFatalError(const char *error)
{
    systemPrint("HALTED: ");
    systemPrint(error);
    systemPrintln();
}

// If debug option is on, print available heap
void reportHeapNow(bool alwaysPrint)
{
    if (alwaysPrint)
    {
        lastHeapReport = millis();

        systemPrintf("FreeHeap: %d / HeapLowestPoint: %d / LargestBlock: %d\r\n", ESP.getFreeHeap(),
                     xPortGetMinimumEverFreeHeapSize(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    }
}

void reportHeap()
{

    if (millis() - lastHeapReport > 1000)
    {
        reportHeapNow(true);
    }
}

void factoryReset(bool sdSemaphore)
{
    // tasksStopGnssUart();
    // tiltSensorFactoryReset();

    systemPrintln("Formatting internal file system...");
    LittleFS.format();

    // if (online.gnss == true)
    // {
    //     systemPrintln("Resetting the GNSS to factory defaults. This could take a few seconds...");
    //     gnss->factoryReset();
    // }
    // else
    //     systemPrintln("GNSS not online. Unable to factoryReset...");

    systemPrintln("Settings erased successfully. Rebooting. Goodbye!");
    delay(2000);
    ESP.restart();
}

// void rtcUpdate()
// {
//     if (online.rtc == false) // Only do this if the rtc has not been sync'd previously
//     {
//         if (online.gnss == true) // Only do this if the GNSS is online
//         {
//             if (millis() - lastRTCAttempt > syncRTCInterval) // Only attempt this once per second
//             {
//                 lastRTCAttempt = millis();

//                 // // gnss->update() is called in loop() but rtcUpdate
//                 // // can also be called during begin. To be safe, check for fresh PVT data here.
//                 // gnss->update();

//                 bool timeValid = false;

//                 if (gnss->isValidTime() == true &&
//                     gnss->isValidDate() == true) // Will pass if ZED's RTC is reporting (regardless of GNSS fix)
//                     timeValid = true;

//                 if (gnss->isConfirmedTime() == true && gnss->isConfirmedDate() == true) // Requires GNSS fix
//                     timeValid = true;

//                 if (timeValid &&
//                     (gnss->getFixAgeMilliseconds() > 999)) // If the GNSS time is over a second old, don't use it
//                     timeValid = false;

//                 if (timeValid == true)
//                 {
//                     // To perform the time zone adjustment correctly, it's easiest if we convert the GNSS time and date
//                     // into Unix epoch first and then apply the timeZone offset
//                     uint32_t epochSecs;
//                     uint32_t epochMicros;
//                     convertGnssTimeToEpoch(&epochSecs, &epochMicros);
//                     epochSecs += settings.timeZoneSeconds;
//                     epochSecs += settings.timeZoneMinutes * 60;
//                     epochSecs += settings.timeZoneHours * 60 * 60;

//                     // Set the internal system time
//                     rtc.setTime(epochSecs, epochMicros);

//                     online.rtc = true;
//                     lastRTCSync = millis();

//                     systemPrint("System time set to: ");
//                     systemPrintln(rtc.getDateTime(true));

//                     recordSystemSettingsToFileSD(
//                         settingsFileName); // This will re-record the setting file with the current date/time.
//                 }
//                 else
//                 {
//                     // Reduce the output frequency
//                     static uint32_t lastErrorMsec = -1000 * 1000 * 1000;
//                     if ((millis() - lastErrorMsec) > (30 * 1000))
//                     {
//                         lastErrorMsec = millis();
//                         systemPrintln("No GNSS date/time available for system RTC.");
//                     }
//                 } // End timeValid
//             } // End lastRTCAttempt
//         } // End online.gnss
//     } // End online.rtc

//     // Print TP time sync information here. Trying to do it in the ISR would be a bad idea...
//     if (settings.enablePrintRtcSync == true)
//     {
//         if ((previousGnssSyncTv.tv_sec != gnssSyncTv.tv_sec) || (previousGnssSyncTv.tv_usec != gnssSyncTv.tv_usec))
//         {
//             time_t nowtime;
//             struct tm *nowtm;
//             char tmbuf[64];

//             nowtime = gnssSyncTv.tv_sec;
//             nowtm = localtime(&nowtime);
//             strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
//             systemPrintf("RTC resync took place at: %s.%03d\r\n", tmbuf, gnssSyncTv.tv_usec / 1000);

//             previousGnssSyncTv.tv_sec = gnssSyncTv.tv_sec;
//             previousGnssSyncTv.tv_usec = gnssSyncTv.tv_usec;
//         }
//     }
// }