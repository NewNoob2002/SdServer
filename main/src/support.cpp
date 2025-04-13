#include "support.h"

int systemAvailable()
{
    // if (printEndpoint == PRINT_ENDPOINT_BLUETOOTH || printEndpoint == PRINT_ENDPOINT_ALL)
    // return (bluetoothRxDataAvailable());
    return (Serial.available());
}

void systemFlush()
{
    Serial.flush();
}

int systemRead()
{
    return Serial.read();
}
void systemWrite(const uint8_t *buffer, uint16_t length)
{
    Serial.write(buffer, length);
}

void systemWrite(uint8_t value)
{
    systemWrite(&value, 1);
}
void systemPrint(const char *string)
{
    systemWrite((const uint8_t *)string, strlen(string));
}

void systemPrintln(const char *value)
{
    systemPrint(value);
    systemPrintln();
}

void systemPrintln()
{
    systemPrint("\r\n");
}

// void systemPrintln(const char value[])
// {
//     systemPrint(value);
//     systemPrintln();
// }

void systemPrintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    va_list args2;
    va_copy(args2, args);
    char buf[vsnprintf(nullptr, 0, format, args) + 1];

    vsnprintf(buf, sizeof buf, format, args2);

    systemPrint(buf);

    va_end(args);
    va_end(args2);
}

void systemPrint(int value)
{
    char temp[20];
    snprintf(temp, sizeof(temp), "%d", value);
    systemPrint(temp);
}

void systemPrintln(int value)
{
    systemPrint(value);
    systemPrintln();
}

void systemPrint(int value, uint8_t printType)
{
    char temp[100];

    if (printType == HEX)
        snprintf(temp, sizeof(temp), "%08X", value);
    else if (printType == DEC)
        snprintf(temp, sizeof(temp), "%d", value);

    systemPrint(temp);
}

void systemPrintln(int value, uint8_t printType)
{
    systemPrint(value, printType);
    systemPrintln();
}

void systemPrint(uint8_t value, uint8_t printType)
{
    char temp[20];

    if (printType == HEX)
        snprintf(temp, sizeof(temp), "%02X", value);
    else if (printType == DEC)
        snprintf(temp, sizeof(temp), "%d", value);

    systemPrint(temp);
}

void systemPrintln(uint8_t value, uint8_t printType)
{
    systemPrint(value, printType);
    systemPrintln();
}

// Print a 16-bit value as HEX or decimal
void systemPrint(uint16_t value, uint8_t printType)
{
    char temp[20];

    if (printType == HEX)
        snprintf(temp, sizeof(temp), "%04X", value);
    else if (printType == DEC)
        snprintf(temp, sizeof(temp), "%d", value);

    systemPrint(temp);
}

// Print a 16-bit value as HEX or decimal with a carriage return and linefeed
void systemPrintln(uint16_t value, uint8_t printType)
{
    systemPrint(value, printType);
    systemPrintln();
}

// Print a floating point value with a specified number of decimal places
void systemPrint(float value, uint8_t decimals)
{
    char temp[20];
    snprintf(temp, sizeof(temp), "%.*f", decimals, value);
    systemPrint(temp);
}

// Print a floating point value with a specified number of decimal places and a
// carriage return and linefeed
void systemPrintln(float value, uint8_t decimals)
{
    systemPrint(value, decimals);
    systemPrintln();
}

void systemPrint(double value, uint8_t decimals)
{
    char temp[30];
    snprintf(temp, sizeof(temp), "%.*f", decimals, value);
    systemPrint(temp);
}

// Print a double precision floating point value with a specified number of decimal
// places and a carriage return and linefeed
void systemPrintln(double value, uint8_t decimals)
{
    systemPrint(value, decimals);
    systemPrintln();
}

// Print a string
void systemPrint(String myString)
{
    systemPrint(myString.c_str());
}
void systemPrintln(String myString)
{
    systemPrint(myString);
    systemPrintln();
}

void clearBuffer()
{
    systemFlush();
    delay(20); // Wait for any incoming chars to hit buffer
    while (systemAvailable() > 0)
        systemRead(); // Clear buffer
}

void geodeticToEcef(double lat, double lon, double alt, double *x, double *y, double *z)
{
    double clat = cos(lat * DEG_TO_RAD);
    double slat = sin(lat * DEG_TO_RAD);
    double clon = cos(lon * DEG_TO_RAD);
    double slon = sin(lon * DEG_TO_RAD);

    double N = WGS84_A / sqrt(1.0 - WGS84_E * WGS84_E * slat * slat);

    *x = (N + alt) * clat * clon;
    *y = (N + alt) * clat * slon;
    *z = (N * (1.0 - WGS84_E * WGS84_E) + alt) * slat;
}

// Convert ECEF to LLH (geodetic)
// From: https://danceswithcode.net/engineeringnotes/geodetic_to_ecef/geodetic_to_ecef.html
void ecefToGeodetic(double x, double y, double z, double *lat, double *lon, double *alt)
{
    double a = 6378137.0;              // WGS-84 semi-major axis
    double e2 = 6.6943799901377997e-3; // WGS-84 first eccentricity squared
    double a1 = 4.2697672707157535e+4; // a1 = a*e2
    double a2 = 1.8230912546075455e+9; // a2 = a1*a1
    double a3 = 1.4291722289812413e+2; // a3 = a1*e2/2
    double a4 = 4.5577281365188637e+9; // a4 = 2.5*a2
    double a5 = 4.2840589930055659e+4; // a5 = a1+a3
    double a6 = 9.9330562000986220e-1; // a6 = 1-e2

    double zp, w2, w, r2, r, s2, c2, s, c, ss;
    double g, rg, rf, u, v, m, f, p;

    zp = abs(z);
    w2 = x * x + y * y;
    w = sqrt(w2);
    r2 = w2 + z * z;
    r = sqrt(r2);
    *lon = atan2(y, x); // Lon (final)

    s2 = z * z / r2;
    c2 = w2 / r2;
    u = a2 / r;
    v = a3 - a4 / r;
    if (c2 > 0.3)
    {
        s = (zp / r) * (1.0 + c2 * (a1 + u + s2 * v) / r);
        *lat = asin(s); // Lat
        ss = s * s;
        c = sqrt(1.0 - ss);
    }
    else
    {
        c = (w / r) * (1.0 - s2 * (a5 - u - c2 * v) / r);
        *lat = acos(c); // Lat
        ss = 1.0 - c * c;
        s = sqrt(ss);
    }

    g = 1.0 - e2 * ss;
    rg = a / sqrt(g);
    rf = a6 * rg;
    u = w - rg * c;
    v = zp - rf * s;
    f = c * u + s * v;
    m = c * v - s * u;
    p = m / (rf / g + f);
    *lat = *lat + p;        // Lat
    *alt = f + m * p / 2.0; // Altitude
    if (z < 0.0)
    {
        *lat *= -1.0; // Lat
    }

    *lat *= RAD_TO_DEG; // Convert to degrees
    *lon *= RAD_TO_DEG;
}

// Make size of files human readable
void stringHumanReadableSize(String &returnText, uint64_t bytes)
{
    char suffix[5] = {'\0'};
    char readableSize[50] = {'\0'};
    float cardSize = 0.0;

    if (bytes < 1024)
        strcpy(suffix, "B");
    else if (bytes < (1024 * 1024))
        strcpy(suffix, "KB");
    else if (bytes < (1024 * 1024 * 1024))
        strcpy(suffix, "MB");
    else
        strcpy(suffix, "GB");

    if (bytes < (1024))
        cardSize = bytes; // B
    else if (bytes < (1024 * 1024))
        cardSize = bytes / 1024.0; // KB
    else if (bytes < (1024 * 1024 * 1024))
        cardSize = bytes / 1024.0 / 1024.0; // MB
    else
        cardSize = bytes / 1024.0 / 1024.0 / 1024.0; // GB

    if (strcmp(suffix, "GB") == 0)
        snprintf(readableSize, sizeof(readableSize), "%0.1f %s", cardSize, suffix); // Print decimal portion
    else if (strcmp(suffix, "MB") == 0)
        snprintf(readableSize, sizeof(readableSize), "%0.1f %s", cardSize, suffix); // Print decimal portion
    else if (strcmp(suffix, "KB") == 0)
        snprintf(readableSize, sizeof(readableSize), "%0.1f %s", cardSize, suffix); // Print decimal portion
    else
        snprintf(readableSize, sizeof(readableSize), "%.0f %s", cardSize, suffix); // Don't print decimal portion

    returnText = String(readableSize);
}

// void verifyTable()
// {
//     // Verify the product name table
//     if (productDisplayNamesEntries != (RTK_UNKNOWN + 1))
//         reportFatalError("Fix productDisplayNames to match ProductVariant");
//     if (platformFilePrefixTableEntries != (RTK_UNKNOWN + 1))
//         reportFatalError("Fix platformFilePrefixTable to match ProductVariant");
//     if (platformPrefixTableEntries != (RTK_UNKNOWN + 1))
//         reportFatalError("Fix platformPrefixTable to match ProductVariant");
//     if (platformProvisionTableEntries != (RTK_UNKNOWN + 1))
//         reportFatalError("Fix platformProvisionTable to match ProductVariant");

//     // Verify the measurement scales
//     if (measurementScaleEntries != MEASUREMENT_UNITS_MAX)
//         reportFatalError("Fix measurementScaleTable to match measurementUnits");

//     tasksValidateTables();

//     if (CORR_NUM >= (int)('h' - 'a'))
//         reportFatalError("Too many correction sources");
// }

// void checkGNSSArrayDefaults()
// {
//     bool defaultsApplied = false;

//     if (present.gnss_um980)
//     {
//         if (settings.dynamicModel == 254)
//             settings.dynamicModel = UM980_DYN_MODEL_SURVEY;

//         if (settings.um980Constellations[0] == 254)
//         {
//             defaultsApplied = true;
//             ESP_LOGI("checkGNSSArrayDefaults", "UM980 Constellations");
//             // Reset constellations to defaults
//             for (int x = 0; x < MAX_UM980_CONSTELLATIONS; x++)
//                 settings.um980Constellations[x] = 1;
//         }

//         if (settings.um980MessageRatesNMEA[0] == 254)
//         {
//             defaultsApplied = true;
//             ESP_LOGI("checkGNSSArrayDefaults", "UM980 um980MessageRatesNMEA");
//             // Reset rates to defaults
//             for (int x = 0; x < MAX_UM980_NMEA_MSG; x++)
//                 settings.um980MessageRatesNMEA[x] = umMessagesNMEA[x].msgDefaultRate;
//         }

//         if (settings.um980MessageRatesRTCMRover[0] == 254)
//         {
//             defaultsApplied = true;
//             ESP_LOGI("checkGNSSArrayDefaults", "UM980 um980MessageRatesRTCMRover");
//             // For rovers, RTCM should be zero by default.
//             for (int x = 0; x < MAX_UM980_RTCM_MSG; x++)
//                 settings.um980MessageRatesRTCMRover[x] = 0;
//         }

//         if (settings.um980MessageRatesRTCMBase[0] == 254)
//         {
//             defaultsApplied = true;
//             ESP_LOGI("checkGNSSArrayDefaults", "UM980 um980MessageRatesRTCMBase");
//             // Reset RTCM rates to defaults
//             for (int x = 0; x < MAX_UM980_RTCM_MSG; x++)
//                 settings.um980MessageRatesRTCMBase[x] = umMessagesRTCM[x].msgDefaultRate;
//         }
//     }

//     if (defaultsApplied == true)
//     {
//         if (present.gnss_um980)
//         {
//             ESP_LOGI("checkGNSSArrayDefaults", "UM980 Defaults Applied");
//             settings.minCNO = 10;                    // Default 10 degrees
//             settings.surveyInStartingAccuracy = 2.0; // Default 2m
//             settings.measurementRateMs = 500;        // Default 2Hz.
//         }
//     }
// }
void printPartitionTable(void)
{
    systemPrintln("ESP32 Partition table:\n");

    systemPrintln("| Type | Sub |  Offset  |   Size   |       Label      |");
    systemPrintln("| ---- | --- | -------- | -------- | ---------------- |");

    esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    if (pi != NULL)
    {
        do
        {
            const esp_partition_t *p = esp_partition_get(pi);
            systemPrintf("|  %02x  | %02x  | 0x%06X | 0x%06X | %-16s |\r\n", p->type, p->subtype, p->address, p->size,
                         p->label);
        } while ((pi = (esp_partition_next(pi))));
    }
}

bool findSpiffsPartition(void)
{
    esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    if (pi != NULL)
    {
        do
        {
            const esp_partition_t *p = esp_partition_get(pi);
            if (strcmp(p->label, "spiffs") == 0)
                return true;
        } while ((pi = (esp_partition_next(pi))));
    }
    return false;
}

// const char *correctionGetName(uint8_t id)
// {
//     if (id == CORR_NUM)
//         return "None";

//     // Validate the id value
//     if (id > CORR_NUM)
//     {
//         systemPrintf("ERROR: correctionGetName invalid correction id value %d, valid range (0 - %d)!\r\n", id,
//                      CORR_NUM - 1);
//         return nullptr;
//     }

//     // Return the name of the correction source
//     return correctionsSourceNames[id];
// }

// Print the timestamp
void printTimeStamp()
{
    // uint32_t currentMilliseconds;
    // static uint32_t previousMilliseconds;

    // // Timestamp the messages
    // currentMilliseconds = millis();
    // if ((currentMilliseconds - previousMilliseconds) >= 1000)
    // {
    //     //         1         2         3
    //     // 123456789012345678901234567890
    //     // YYYY-mm-dd HH:MM:SS.xxxrn0
    //     struct tm timeinfo = rtc.getTimeStruct();
    //     char timestamp[30];
    //     strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
    //     systemPrintf("%s.%03ld\r\n", timestamp, rtc.getMillis());

    //     // Select the next time to display the timestamp
    //     previousMilliseconds = currentMilliseconds;
    // }
}

// void checkTaskStatus()
// {
//     char InfoBuffer[512] = {0};
//     memset(InfoBuffer, 0, 512); // 信息缓冲区清零
//     vTaskList((char *)&InfoBuffer);
//     systemPrintf("任务名  任务状态  优先级  剩余栈  任务序号  cpu核\r\n");
//     systemPrintf("\r\n%s\r\n", InfoBuffer);

// #if 1
//     char CPU_RunInfo[400] = {0}; // 保存任务运行时间信息
//     memset(CPU_RunInfo, 0, 400); // 信息缓冲区清零
//     vTaskGetRunTimeStats((char *)&CPU_RunInfo);
//     systemPrintf("任务名       运行计数         使用率\r\n");
//     systemPrintf("%s", CPU_RunInfo);
//     systemPrintf("---------------------------------------------\r\n\n");
// #endif

//     // 打印剩余ram和堆容量
//     systemPrintf("IDLE: ****free internal ram %d  all heap size: %d Bytes****\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_free_size(MALLOC_CAP_8BIT));
//     systemPrintf("IDLE: ****free SPIRAM size: %d Bytes****\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
// }