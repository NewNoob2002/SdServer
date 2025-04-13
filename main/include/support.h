#pragma once

#include <stdio.h>
#include <cstdarg>
#include <Arduino.h>

// #include "GlobalDef.h"
// #include "Task.h"
#include "global.h"
#include "settings.h"

const double WGS84_A = 6378137;           // https://geographiclib.sourceforge.io/html/Constants_8hpp_source.html
const double WGS84_E = 0.081819190842622; // http://docs.ros.org/en/hydro/api/gps_common/html/namespacegps__common.html
                                          // and https://gist.github.com/uhho/63750c4b54c7f90f37f958cc8af0c71
int systemAvailable();
void systemFlush();
int systemRead();

void systemWrite(const uint8_t *buffer, uint16_t length);
void systemWrite(uint8_t value);
void systemPrintf(const char *format, ...);
void systemPrint(int value);
void systemPrintln(int value);
void systemPrint(int value, uint8_t printType);
void systemPrintln(int value, uint8_t printType);
void systemPrint(uint8_t value, uint8_t printType);
void systemPrintln(uint8_t value, uint8_t printType);
void systemPrint(uint16_t value, uint8_t printType);
void systemPrintln(uint16_t value, uint8_t printType);
void systemPrint(const char *value);
void systemPrintln(const char *value);
void systemPrintln();
void systemPrint(float value, uint8_t decimals);
void systemPrintln(float value, uint8_t decimals);
void systemPrint(double value, uint8_t decimals);
void systemPrintln(double value, uint8_t decimals);
void systemPrint(String myString);
void systemPrintln(String myString);
void clearBuffer();

void geodeticToEcef(double lat, double lon, double alt, double *x, double *y, double *z);
void ecefToGeodetic(double x, double y, double z, double *lat, double *lon, double *alt);
void stringHumanReadableSize(String &returnText, uint64_t bytes);

void verifyTable();
void checkGNSSArrayDefaults();
void reportFatalError(const char*error);
void reportHeapNow(bool alwaysPrint);
void reportHeap();

void printPartitionTable(void);
bool findSpiffsPartition(void);

const char *correctionGetName(uint8_t id);
void printTimeStamp();
void checkTaskStatus();