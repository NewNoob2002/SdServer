#pragma once
#include "support.h"
#include "global.h"
#include "settings.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "Update.h"
#include "WiFiManager.h"
#include "form.h"
#include <DNSServer.h>

static DNSServer dnsServer;
// State machine to allow web server access to network layer
enum WebServerState
{
    WEBSERVER_STATE_OFF = 0,
    WEBSERVER_STATE_WAIT_FOR_NETWORK,
    WEBSERVER_STATE_NETWORK_CONNECTED,
    WEBSERVER_STATE_RUNNING,

    // Add new states here
    WEBSERVER_STATE_MAX
};

void stopWebServer();
// Initialize and start the web server
void webServerStart();

// Stop the web config state machine
void webServerStop();

void webServerReleaseResources();

// Return true if we are in a state that requires network access
bool webServerNeedsNetwork();

void webServerStopSockets();

// Set the next webconfig state
void webServerSetState(uint8_t newState);

// Get the webconfig state name
const char *webServerGetStateName(uint8_t state, char *string);

bool webServerIsRunning();

void webServerUpdate();

void notFound(AsyncWebServerRequest *request);

// 实现websocket服务器启动
bool websocketServerStart();

// WebSocket消息处理函数
void handleWebSocketMessage(char *message);

// 文件管理处理函数
void handleFileManager(AsyncWebServerRequest *request);

// 文件上传处理函数
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

// 固件上传处理函数
void handleFirmwareFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

// 获取文件列表
void getFileList(String &fileList);

// 创建消息列表
void createMessageList(String &messageList);

// 创建基础消息列表
void createMessageListBase(String &messageList);

// Create the web server and web sockets
bool webServerAssignResources(int httpPort = 80);