#include "WebServer.h"
#include "Arduino.h"
#include "LittleFS.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "SD.h"
#include "Update.h"

#include "WiFiManager.h"

AsyncWebServer server(80);

// SD card details
#define SD_CS_PIN 5  // Define your SD card CS pin here
bool sdCardMounted = false;

void initWebServer() {
    // Initialize SD Card
    // sdCardMounted = SD.begin(SD_CS_PIN);
    // if (sdCardMounted) {
    //     Serial.println("SD Card mounted successfully");
    // } else {
    //     Serial.println("SD Card mount failed");
    // }

    // Define routes for serving static files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // Handle firmware update
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        // When the update is complete, send the result
        bool success = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", success ? "OK" : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response);
        delay(500);
        Serial.println("Restarting...");
        // Update.canRollBack();
        ESP.restart();
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        // Handle the actual OTA update process
        if (!index) {
            Serial.printf("Update start: %s\n", filename.c_str());
            if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                Update.printError(Serial);
            }
        }
        
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
        
        if (final) {
            if (Update.end(true)) {
                Serial.printf("Update Success: %uB\n", index + len);
            } else {
                Update.printError(Serial);
            }
        }
    });

    // Handle not found requests
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    // Start server
    server.begin();
    Serial.println("Web server started");
} 