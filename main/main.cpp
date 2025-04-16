#include "Arduino.h"
#include "SdFat.h"
#include "LittleFS.h"

#include "state.h"
#include "global.h"
#include "WebServer.h"
#include "WiFiManager.h"
#include "Bluetooth.h"
#include "settings.h"

settings_t settings;
task_t task;

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
volatile SystemState systemState = STATE_NOT_SET;
volatile SystemState requestedSystemState = STATE_NOT_SET;
uint8_t sectorBuffer[512];
//------------------------------------------------------------------------------
// SdCardFactory constructs and initializes the appropriate card.
uint32_t cardSectorCount = 0;
SdCardFactory cardFactory;
// Pointer to generic SD card.
SdCard *m_card = nullptr;
uint32_t const ERASE_SIZE = 262144L;

uint32_t lastSystemStateUpdate;
bool forceSystemStateUpdate; // Set true to avoid update wait
bool newSystemStateRequested = false;

online_struct online;
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
uint8_t buffer[1024 * 8];
int bytesRead = 0;
int bytesWritten = 0;
void readSerial1(void *pvParameters)
{
    while (1)
    {
        if (bluetoothGetState() == BT_CONNECTED)
        {
            bluetoothWrite((uint8_t *)"hello", sizeof("hello"));
            while (bluetoothRxDataAvailable())
            {
                bytesRead = bluetoothRead(buffer, sizeof(buffer));
                Serial.write(buffer, bytesRead);
                delay(10);
            }
        }
        delay(1);
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
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (!LittleFS.begin(true, "/fs", 8, "littlefs"))
    {
        Serial.println("Failed to mount LittleFS");
    }
    Serial.printf("Total bytes: %d, Used bytes: %d\n", LittleFS.totalBytes(), LittleFS.usedBytes());
}

IPAddress local_ip(192, 168, 4, 1); // 设置静态 IP 地址
IPAddress gateway(192, 168, 4, 1);  // 设置网关地址
IPAddress subnet(255, 255, 255, 0);  // 设置子网掩码

extern "C" void app_main()
{
    initArduino();

    Serial.begin(115200);
    beginFS();
    beginButton();
    // initWiFiAP();
    // initWebServer();
    // beginSD();
    beginSystemState();
    // sd.remove("test.txt");
    // xTaskCreate(sdTest, "sdTest", 3072, NULL, 5, NULL);
    xTaskCreate(readSerial1, "readSerial1", 2048, NULL, 5, NULL);
    xTaskCreate(stateUpdate, "stateUpdate", 4096, nullptr, 6, nullptr);
    // initWiFiAP();
    while (1)
    {
        webServerUpdate();
        reportHeap();
        delay(10);
    }
}