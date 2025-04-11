#include "Arduino.h"
#include "SdFat.h"
#include "LittleFS.h"
#include "WebServer.h"
#include "WiFiManager.h"

#define SD_FAT_TYPE 3

#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs *sd = nullptr;
FsFile *file = nullptr;
#else // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif // SD_FAT_TYPE

#define SD_MISO_PIN 4
#define SD_MOSI_PIN 5
#define SD_SCK_PIN 6
#define SD_CS_PIN 7
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(16))
// SdFat *sd = nullptr;
// SdFile *logFile = nullptr;         // File that all GNSS messages sentences are written to
// SdFile *managerTempFile = nullptr; // File used for uploading or downloading in the file manager section of AP config
bool managerFileOpen = false;
int startLogTime_minutes = 0; // Mark when we start any logging so we can stop logging after maxLogTime_minutes
int startCurrentLogTime_minutes = 0;
// Mark when we start this specific log file so we can close it after x minutes and start a new one
// System crashes if two tasks access a file at the same time
// So we use a semaphore to see if the file system is available
SemaphoreHandle_t sdCardSemaphore = nullptr;

char logFileName[32] = {'\0'};
// Display used/free space in menu and config page
uint64_t sdCardSize = 0;
uint64_t sdFreeSpace = 0;
uint32_t sdCardSize_MB = 0;
uint32_t sdFreeSpace_MB = 0;
bool outOfSDSpace = false;
unsigned long lastLogSyncTime = 0;

uint8_t sectorBuffer[512];
//------------------------------------------------------------------------------
// SdCardFactory constructs and initializes the appropriate card.
uint32_t cardSectorCount = 0;
SdCardFactory cardFactory;
// Pointer to generic SD card.
SdCard *m_card = nullptr;
uint32_t const ERASE_SIZE = 262144L;

void sdErrorHalt()
{
    if (!m_card)
    {
        Serial.printf("Invalid SD_CONFIG");
    }
    else if (m_card->errorCode())
    {
        if (m_card->errorCode() == SD_CARD_ERROR_CMD0)
        {
            Serial.printf("No card, wrong chip select pin, or wiring error?");
        }
        printSdErrorSymbol(&Serial, m_card->errorCode());
    }
    while (true)
    {
    }
}

void beginSD()
{
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (sd == nullptr)
    {
        sd = new SdFs();
    }
    if (!sd->begin(SD_CONFIG))
    {
        sd->initErrorHalt(&Serial);
        return;
    }

    csd_t csd;
    sd->card()->readCSD(&csd); // Card Specific Data
    sdCardSize = (uint64_t)512 * sd->card()->sectorCount();

    sd->volumeBegin();

    // Find available cluster/space
    sdFreeSpace = sd->vol()->freeClusterCount(); // This takes a few seconds to complete
    sdFreeSpace *= sd->vol()->sectorsPerCluster();
    sdFreeSpace *= 512L; // Bytes per sector

    // uint64_t sdUsedSpace = sdCardSize - sdFreeSpace; //Don't think of it as used, think of it as unusable
    sdFreeSpace_MB = sdFreeSpace / 1024 / 1024;
    sdCardSize_MB = sdCardSize / 1024 / 1024;
    Serial.printf("SD card size: %ld MB / Free space: %ld MB\r\n", sdCardSize_MB, sdFreeSpace_MB);

    if (sdCardSemaphore == nullptr)
    {
        sdCardSemaphore = xSemaphoreCreateMutex();
    }
}
// #define sdError(msg) {
// Serial.printf("error: %s", F(msg));
// sdErrorHalt();
// }

// void beginSD()
// {
//     if (present.microSd == false)
//         return;

//     bool gotSemaphore;

//     gotSemaphore = false;

//     while (settings.enableSD == true)
//     {
//         // Setup SD card access semaphore
//         if (sdCardSemaphore == nullptr)
//             sdCardSemaphore = xSemaphoreCreateMutex();

//         // If an SD card is present, allow SdFat to take over
//         // ESP_LOGI("beginSD", "SD card detected");

//         // Allocate the data structure that manages the microSD card
//         if (sd == nullptr)
//         {
//             sd = new SdFat();
//             if (sd == nullptr)
//             {
//                 ESP_LOGE("beginSD", "Failed to allocate the SdFat structure!");
//                 break;
//             }
//         }

//         if (settings.spiFrequency > 16)
//         {
//             systemPrintln("Error: SPI Frequency out of range. Default to 16MHz");
//             settings.spiFrequency = 16;
//         }

//         if (sd->begin(SdSpiConfig(-1, DEDICATED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == false)
//         {
//             int tries = 0;
//             int maxTries = 2;
//             for (; tries < maxTries; tries++)
//             {
//                 ESP_LOGE("beginSD", "SD init failed - using SPI and SdFat. Trying again %d out of %d", tries + 1, maxTries);

//                 delay(250); // Give SD more time to power up, then try again
//                 if (sd->begin(SdSpiConfig(-1, DEDICATED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == true)
//                     break;
//             }

//             if (tries == maxTries)
//             {
//                 ESP_LOGE("beginSD", "SD init failed - using SPI and SdFat. Is card formatted?");
//                 // Check reset count and prevent rolling reboot
//                 // if (settings.resetCount < 5)
//                 // {
//                 //     if (settings.forceResetOnSDFail == true)
//                 //         ESP.restart();
//                 // }
//                 break;
//             }
//         }

//         // Change to root directory. All new file creation will be in root.
//         if (sd->chdir() == false)
//         {
//             ESP_LOGE("beginSD", "SD change directory failed");
//             break;
//         }

//         if (createTestFile() == false)
//         {
//             ESP_LOGE("beginSD", "Failed to create test file. Format SD card with 'SD Card Formatter'.");
//             // displaySDFail(5000);
//             break;
//         }

//         csd_t csd;
//         sd->card()->readCSD(&csd); // Card Specific Data
//         sdCardSize = (uint64_t)512 * sd->card()->sectorCount();

//         sd->volumeBegin();

//         // Find available cluster/space
//         sdFreeSpace = sd->vol()->freeClusterCount(); // This takes a few seconds to complete
//         sdFreeSpace *= sd->vol()->sectorsPerCluster();
//         sdFreeSpace *= 512L; // Bytes per sector

//         // uint64_t sdUsedSpace = sdCardSize - sdFreeSpace; //Don't think of it as used, think of it as unusable
//         sdFreeSpace_MB = sdFreeSpace / 1024 / 1024;
//         sdCardSize_MB = sdCardSize / 1024 / 1024;
//         systemPrintf("SD card size: %d MB / Free space: %d MB\r\n", sdCardSize_MB, sdFreeSpace_MB);

//         if (sdFreeSpace_MB < 100)
//             outOfSDSpace = true;
//         else
//             outOfSDSpace = false;

//         systemPrintln("microSD: Online");
//         online.microSD = true;
//         break;
//     }
// }

// void formatSD()
//  {
//      uint32_t firstBlock = 0;
//      uint32_t lastBlock;
//      uint16_t n = 0;
//      // Select and initialize proper card driver.
//      m_card = cardFactory.newCard(SdSpiConfig(-1, DEDICATED_SPI, SD_SCK_MHZ(settings.spiFrequency)));
//      if (!m_card || m_card->errorCode())
//      {
//          sdError("card init failed.");
//          releaseSdLock();
//          return;
//      }

//      cardSectorCount = m_card->sectorCount();
//      if (!cardSectorCount)
//      {
//          sdError("Get sector count failed.");
//          releaseSdLock();
//          return;
//      }
//      auto cardSize = cardSectorCount * 512e-9;
//      systemPrintf("Card size: %f GB (GB = 1E9 bytes)\n", cardSize);
//      systemPrintf("Card will be formated ");
//      if (cardSectorCount > 67108864)
//      {
//          systemPrintf("exFAT\n");
//      }
//      else if (cardSectorCount > 4194304)
//      {
//          systemPrintf("FAT32\n");
//      }
//      else
//      {
//          systemPrintf("FAT16\n");
//      }
//      do
//      {
//          lastBlock = firstBlock + ERASE_SIZE - 1;
//          if (lastBlock >= cardSectorCount)
//          {
//              lastBlock = cardSectorCount - 1;
//          }
//          if (!m_card->erase(firstBlock, lastBlock))
//          {
//              sdError("erase failed");
//          }
//          systemPrintf(".");
//          if ((n++) % 64 == 63)
//          {
//              systemPrintf("\n");
//          }
//          firstBlock += ERASE_SIZE;
//      } while (firstBlock < cardSectorCount);

//      if (!m_card->readSector(0, sectorBuffer))
//      {
//          sdError("readBlock");
//      }
//      systemPrintf("All data set to %d\n", int(sectorBuffer[0]));
//      systemPrintf("Erase done\n");
//      ExFatFormatter exFatFormatter;
//      FatFormatter fatFormatter;
//      bool rtn = cardSectorCount > 67108864
//                     ? exFatFormatter.format(m_card, sectorBuffer, &Serial)
//                     : fatFormatter.format(m_card, sectorBuffer, &Serial);

//      if (!rtn)
//      {
//          sdErrorHalt();
//      }
//      return;
//  }
uint8_t buffer[1024 * 8];
int bytesRead = 0;
int bytesWritten = 0;
void readSerial1(void *pvParameters)
{
    while (1)
    {
        bytesRead = Serial1.readBytes(buffer, sizeof(buffer));
        delay(10);
    }
}
void sdTest(void *pvParameters)
{

    while (1)
    {
        if (bytesRead > 0)
        {
            long startTime = millis();
            file->open(logFileName, O_CREAT | O_WRITE | O_APPEND);
            bytesWritten = file->write(buffer, bytesRead);
            if (bytesWritten > 0)
            {
                Serial.printf("writing to file: %d bytes written, %d bytes read\r\n", bytesWritten, bytesRead);
            }
            file->sync();
            file->close();
            long endTime = millis();
            log_i("time taken: %d ms, write speed: %d bytes/s\r\n", endTime - startTime, bytesWritten * 1000 / (endTime - startTime));
            delay(1);
        }
        delay(1);
    }
}

bool beginLogging(const char *customFileName)
{
    if (customFileName == nullptr)
    {
        // Generate a standard log file name
        // We are not reusing the last log, so erase the global/original filename
        logFileName[0] = 0;

        if (strlen(logFileName) == 0)
        {
            // u-blox platforms use ubx file extension for logs, all others use TXT
            char fileExtension[4] = "xyz";

            snprintf(logFileName, sizeof(logFileName), "/test.%s", // SdFat library
                     fileExtension);
        }
    }
    else
    {
        strncpy(logFileName, customFileName,
                sizeof(logFileName) - 1); // customFileName already has the preceding slash added
    }

    // Allocate the log file
    if (!file)
    {
        file = new FsFile;
        if (!file)
        {
            Serial.printf("Failed to allocate logFile!");
            return (false);
        }
    }

    // Attempt to write to file system. This avoids collisions with file writing in gnssSerialReadTask()
    if (xSemaphoreTake(sdCardSemaphore, 100) == pdPASS)
    {
        // markSemaphore(FUNCTION_CREATEFILE);

        // O_CREAT - create the file if it does not exist
        // O_APPEND - seek to the end of the file prior to each write
        // O_WRITE - open for write
        if (file->open(logFileName, O_CREAT | O_APPEND | O_WRITE) == false)
        {
            Serial.printf("Failed to create GNSS log file: %s\r\n", logFileName);
            xSemaphoreGive(sdCardSemaphore);
            return (false);
        }
    }
    return true;
}

void beginFS()
{
    if (!LittleFS.begin(true, "/fs", 8, "littlefs"))
    {
        Serial.println("Failed to mount LittleFS");
    }
    Serial.printf("Total bytes: %d, Used bytes: %d\n", LittleFS.totalBytes(), LittleFS.usedBytes());
}

extern "C" void app_main()
{
    initArduino();

    Serial.begin(115200);
    Serial1.setRxBufferSize(2048);
    Serial1.setRxFIFOFull(128);
    Serial1.begin(115200, SERIAL_8N1, 18, 19);
    beginFS();
    initWiFiAP();
    initWebServer();
    beginSD();
    beginLogging(nullptr);
    // Do your own thing
    sd->ls("/", LS_A | LS_R | LS_DATE | LS_SIZE);
    // sd.remove("test.txt");
    xTaskCreate(sdTest, "sdTest", 3072, NULL, 5, NULL);
    xTaskCreate(readSerial1, "readSerial1", 2048, NULL, 5, NULL);
}