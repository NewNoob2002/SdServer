#include "WebServer.h"
#include "Arduino.h"
// #include "LittleFS.h"


#include "WiFiManager.h"

AsyncWebServer *webServer = nullptr;
AsyncWebServer *wsserver = nullptr;
// DNSServer *dnsserver = nullptr;

#define AP_CONFIG_SETTING_SIZE 20000 // 10000 isn't enough if the SD card contains many files
char *settingsCSV;                   // Push large array onto heap
char *incomingSettings;

WebServerState webServerState = WEBSERVER_STATE_OFF;

bool websocketConnected = false;

static const char *const webServerStateNames[] = {
    "WEBSERVER_STATE_OFF",
    "WEBSERVER_STATE_WAIT_FOR_NETWORK",
    "WEBSERVER_STATE_NETWORK_CONNECTED",
    "WEBSERVER_STATE_RUNNING",
};

// WebServer任务相关定义
#define updateWebServerTaskStackSize 4096
#define updateWebServerTaskPriority 1
static TaskHandle_t updateWebServerTaskHandle = NULL;
// static void updateWebServerTask(void *pvParameters)
// {
//     // 标记任务正在运行
//     task.updateWebServerTaskRunning = true;

//     while (task.updateWebServerTaskStopRequest == false)
//     {
//         // 任务逻辑
//         // 例如：处理异步请求，维护连接等

//         // 给其他任务一些时间运行
//         vTaskDelay(1 / portTICK_PERIOD_MS);
//     }

//     // 标记任务已停止
//     task.updateWebServerTaskRunning = false;
//     task.updateWebServerTaskStopRequest = false;

//     // 删除任务
//     vTaskDelete(NULL);
// }

// Start the Web Server state machine
void webServerStart()
{
    // Display the heap state
    reportHeapNow(settings.debugWebServer);

    systemPrintln("Web Server start");
    webServerSetState(WEBSERVER_STATE_WAIT_FOR_NETWORK);
}

// Stop the web config state machine
void webServerStop()
{
    online.webServer = false;

    if (settings.debugWebServer)
        systemPrintln("webServerStop called");

    if (webServerState != WEBSERVER_STATE_OFF)
    {
        webServerStopSockets();      // Release socket resources
        webServerReleaseResources(); // Release web server resources

        // Stop network
        systemPrintln("Web Server releasing network request");

        // Stop the machine
        webServerSetState(WEBSERVER_STATE_OFF);
    }
}

// Return true if we are in a state that requires network access
bool webServerNeedsNetwork()
{
    if (webServerState >= WEBSERVER_STATE_WAIT_FOR_NETWORK && webServerState <= WEBSERVER_STATE_RUNNING)
        return true;
    return false;
}

void webServerStopSockets()
{
    websocketConnected = false;

    if (wsserver != nullptr)
    {
        // Stop the httpd server
        wsserver->end();
        wsserver = nullptr;
    }
}

// Set the next webconfig state
void webServerSetState(uint8_t newState)
{
    char string1[40];
    char string2[40];
    const char *arrow = nullptr;
    const char *asterisk = nullptr;
    const char *initialState = nullptr;
    const char *endingState = nullptr;

    // Display the state transition
    if (settings.debugWebServer)
    {
        arrow = "";
        asterisk = "";
        initialState = "";
        if (newState == webServerState)
            asterisk = "*";
        else
        {
            initialState = webServerGetStateName(webServerState, string1);
            arrow = " --> ";
        }
    }

    // Set the new state
    webServerState = (WebServerState)newState;
    if (settings.debugWebServer)
    {
        // Display the new firmware update state
        endingState = webServerGetStateName(newState, string2);
        // if (!online.rtc)
        systemPrintf("%s%s%s%s\r\n", asterisk, initialState, arrow, endingState);
        // else
        // {
        //     // Timestamp the state change
        //     //          1         2
        //     // 12345678901234567890123456
        //     // YYYY-mm-dd HH:MM:SS.xxxrn0
        //     struct tm timeinfo = rtc.getTimeStruct();
        //     char s[30];
        //     strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &timeinfo);
        //     systemPrintf("%s%s%s%s, %s.%03ld\r\n", asterisk, initialState, arrow, endingState, s, rtc.getMillis());
        // }
    }

    // Validate the state
    if (newState >= WEBSERVER_STATE_MAX)
        reportFatalError("Invalid web config state");
}

// Get the webconfig state name
const char *webServerGetStateName(uint8_t state, char *string)
{
    if (state < WEBSERVER_STATE_MAX)
        return webServerStateNames[state];
    sprintf(string, "Unknown state (%d)", state);
    return string;
}

bool webServerIsRunning()
{
    if (webServerState == WEBSERVER_STATE_RUNNING)
        return (true);
    return (false);
}

void updateWebServerTask(void *e)
{
    // Start notification
    task.updateWebServerTaskRunning = true;
    if (settings.printTaskStartStop)
        systemPrintln("Task updateWebServerTask started");

    // Verify that the task is still running
    task.updateWebServerTaskStopRequest = false;
    while (task.updateWebServerTaskStopRequest == false)
    {
        // Display an alive message
        // if (PERIODIC_DISPLAY(PD_TASK_UPDATE_WEBSERVER))
        // {
        //     PERIODIC_CLEAR(PD_TASK_UPDATE_WEBSERVER);
        //     systemPrintln("updateWebServerTask running");
        // }


        dnsServer.processNextRequest();
        

        delay(10);
        taskYIELD();
    }

    // Stop notification
    if (settings.printTaskStartStop)
        systemPrintln("Task updateWebServerTask stopped");
    task.updateWebServerTaskRunning = false;
    vTaskDelete(updateWebServerTaskHandle);
}

void stopWebServer()
{
    if (task.updateWebServerTaskRunning)
        task.updateWebServerTaskStopRequest = true;

    // Wait for task to stop running
    do
        delay(10);
    while (task.updateWebServerTaskRunning);

    if (webServer != nullptr)
    {
        webServer->end();
        free(webServer);
        webServer = nullptr;
    }

    // Stop the multicast DNS server
    // networkMulticastDNSStop();

    if (settingsCSV != nullptr)
    {
        free(settingsCSV);
        settingsCSV = nullptr;
    }

    if (incomingSettings != nullptr)
    {
        free(incomingSettings);
        incomingSettings = nullptr;
    }
}

void webServerReleaseResources()
{
    if (task.updateWebServerTaskRunning)
        task.updateWebServerTaskStopRequest = true;

    // Wait for task to stop running
    do
        delay(10);
    while (task.updateWebServerTaskRunning);

    if (webServer != nullptr)
    {
        webServer->end();
        free(webServer);
        webServer = nullptr;
    }

    // Stop the multicast DNS server
    // networkMulticastDNSStop();

    if (settingsCSV != nullptr)
    {
        free(settingsCSV);
        settingsCSV = nullptr;
    }

    if (incomingSettings != nullptr)
    {
        free(incomingSettings);
        incomingSettings = nullptr;
    }
}

void webServerUpdate()
{
    // Walk the state machine
    switch (webServerState)
    {
    default:
        systemPrintf("ERROR: Unknown Web Server state (%d)\r\n", webServerState);

        // Stop the machine
        webServerStop();
        break;

    case WEBSERVER_STATE_OFF:
        // Wait until webServerStart() is called
        break;

    // Wait for connection to the network
    case WEBSERVER_STATE_WAIT_FOR_NETWORK:
        // Wait until the network is connected to the internet or has WiFi AP
        // if (networkHasInternet() || wifiApIsRunning())
        if (wifiApIsRunning())
        {
            if (settings.debugWebServer)
                systemPrintln("Web Server connected to network");

            webServerSetState(WEBSERVER_STATE_NETWORK_CONNECTED);
        }
        break;

    // Start the web server
    case WEBSERVER_STATE_NETWORK_CONNECTED:
    {
        // Determine if the network has failed
        // if (networkHasInternet() == false && wifiApIsRunning() == false)
        if (wifiApIsRunning() == false)
            webServerStop();
        if (settings.debugWebServer)
            systemPrintln("Assigning web server resources");

        if (webServerAssignResources(80) == true)
        {
            online.webServer = true;
            webServerSetState(WEBSERVER_STATE_RUNNING);
        }
    }
    break;

    // Allow web services
    case WEBSERVER_STATE_RUNNING:
        // Determine if the network has failed
        if (wifiApIsRunning() == false)
            webServerStop();

        // This state is exited when webServerStop() is called

        break;
    }
}

void notFound(AsyncWebServerRequest *request)
{
    String logmessage = "notFound: Client:" + request->client()->remoteIP().toString() + " " + request->url();
    systemPrintln(logmessage);
    request->send(404, "text/plain", "Not found");
}

// 捕获门户请求处理器
class CaptiveRequestHandler : public AsyncWebHandler
{
public:
    String urls[5] = {"/hotspot-detect.html", "/library/test/success.html", "/generate_204", "/ncsi.txt",
                      "/check_network_status.txt"};
    CaptiveRequestHandler()
    {
        if (settings.debugWebServer == true)
            systemPrintln("CaptiveRequestHandler is registered");
    }
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)
    {
        for (int i = 0; i < sizeof(urls); i++)
        {
            if (request->url() == urls[i])
                return true;
        }
        return false;
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
        String logmessage = "Captive Portal Client:" + request->client()->remoteIP().toString() + " " + request->url();
        if (settings.debugWebServer == true)
            systemPrintln(logmessage);
        String response = "<!DOCTYPE html><html><head><title>RTK Config</title></head><body>";
        response += "<div class='container'>";
        response += "<div align='center' class='col-sm-12'><img src='http://";
        response += WiFi.softAPIP().toString();
        response += "/src/rtk-setup.png' alt='SparkFun RTK WiFi Setup'></div>";
        response += "<div align='center'><h3>Configure your RTK receiver <a href='http://";
        response += WiFi.softAPIP().toString();
        response += "/'>here</a></h3></div>";
        response += "</div></body></html>";
        request->send(200, "text/html", response);
    }
};

// 实现websocket服务器启动
bool websocketServerStart()
{
    // 创建websocket服务器实例在端口81
    wsserver = new AsyncWebServer(81);
    if (!wsserver)
    {
        if (settings.debugWebServer == true)
            systemPrintln("WebSocket Server failed to start");
        return false;
    }

    // 创建websocket处理程序
    AsyncWebSocket *ws = new AsyncWebSocket("/ws");
    if (!ws)
    {
        if (settings.debugWebServer == true)
            systemPrintln("WebSocket failed to initialize");
        wsserver->end();
        free(wsserver);
        wsserver = nullptr;
        return false;
    }

    // 设置websocket事件处理程序
    ws->onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
                {
        if (type == WS_EVT_CONNECT)
        {
            websocketConnected = true;
            if (settings.debugWebServer == true)
                systemPrintln("WebSocket client connected");
        }
        else if (type == WS_EVT_DISCONNECT)
        {
            websocketConnected = false;
            if (settings.debugWebServer == true)
                systemPrintln("WebSocket client disconnected");
        }
        else if (type == WS_EVT_DATA)
        {
            // 处理收到的数据
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len)
            {
                // 完整的消息
                if (info->opcode == WS_TEXT)
                {
                    // 确保数据以null结尾
                    data[len] = 0;
                    // 处理命令
                    handleWebSocketMessage((char*)data);
                }
            }
        } });

    // 添加WebSocket处理程序
    wsserver->addHandler(ws);

    // 启动WebSocket服务器
    wsserver->begin();

    return true;
}

// WebSocket消息处理函数
void handleWebSocketMessage(char *message)
{
    // 处理收到的WebSocket消息
    if (settings.debugWebServer)
        systemPrintf("WebSocket message: %s\n", message);

    // 在这里实现消息处理逻辑
    // 解析命令, 例如: command,value,param
    // ...
}

// 文件管理处理函数
void handleFileManager(AsyncWebServerRequest *request)
{
    if (!request)
    {
        return;
    }

    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (settings.debugWebServer == true)
        systemPrintln(logmessage);

    if (request->hasParam("delete"))
    {
        // 处理删除文件请求
        const AsyncWebParameter *p = request->getParam("delete");
        if (p->value().length() > 0)
        {
            String fileToDelete = p->value();
            // 安全检查 - 防止删除系统文件
            if (fileToDelete.startsWith("/"))
            {
                request->send(400, "text/plain", "Cannot delete system files");
                return;
            }

            // 执行删除操作
            // 此处需要实现实际的文件删除逻辑
            // 例如: deleteFile(fileToDelete);

            request->send(200, "text/plain", "File deleted");
            return;
        }
    }

    if (request->hasParam("download"))
    {
        // 处理下载文件请求
        const AsyncWebParameter *p = request->getParam("download");
        if (p->value().length() > 0)
        {
            String fileToDownload = p->value();

            // 此处需要实现实际的文件下载逻辑
            // 例如:
            // if (SPIFFS.exists(fileToDownload))
            //    request->send(SPIFFS, fileToDownload, "application/octet-stream", true);
            // else
            //    request->send(404, "text/plain", "File not found");

            // 临时实现
            request->send(404, "text/plain", "File download not implemented");
            return;
        }
    }

    // 默认情况下显示文件列表
    request->send(200, "text/plain", "File Manager - use with ?delete=filename or ?download=filename");
}

// 文件上传处理函数
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        // 开始上传新文件
        String logmessage = "Upload Start: " + filename;
        if (settings.debugWebServer == true)
            systemPrintln(logmessage);

        // 此处需要实现文件创建逻辑
        // 例如: uploadFile = SPIFFS.open("/" + filename, "w");
    }

    // 写入文件数据
    if (len)
    {
        // 此处需要实现数据写入逻辑
        // 例如: uploadFile.write(data, len);
    }

    if (final)
    {
        // 完成上传
        String logmessage = "Upload Complete: " + filename + ", size: " + String(index + len);
        if (settings.debugWebServer == true)
            systemPrintln(logmessage);

        // 此处需要实现文件关闭逻辑
        // 例如: uploadFile.close();
    }
}

// 固件上传处理函数
void handleFirmwareFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        // 开始上传固件
        String logmessage = "Firmware Upload Start: " + filename;
        if (settings.debugWebServer == true)
            systemPrintln(logmessage);

        // 检查是否是有效的固件文件
        if (!filename.endsWith(".bin"))
        {
            request->send(400, "text/plain", "Invalid firmware file");
            return;
        }

        // 开始更新过程
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            if (settings.debugWebServer == true)
                Update.printError(Serial);
            return;
        }
    }

    // 写入固件数据
    if (len)
    {
        if (Update.write(data, len) != len)
        {
            if (settings.debugWebServer == true)
                Update.printError(Serial);
            return;
        }
    }

    if (final)
    {
        // 完成固件上传
        if (!Update.end(true))
        {
            if (settings.debugWebServer == true)
                Update.printError(Serial);
            request->send(400, "text/plain", "Firmware update failed");
        }
        else
        {
            request->send(200, "text/plain", "Firmware update successful. Rebooting...");
            // 延迟一段时间以确保响应发送完成
            delay(1000);
            ESP.restart();
        }
    }
}

// 获取文件列表
void getFileList(String &fileList)
{
    // 此处需要实现获取文件列表的逻辑
    // 例如:
    // File root = SPIFFS.open("/");
    // File file = root.openNextFile();
    // while (file) {
    //     fileList += file.name();
    //     fileList += ",";
    //     fileList += file.size();
    //     fileList += "\n";
    //     file = root.openNextFile();
    // }

    // 临时实现
    fileList = "No files available\n";
}

// 创建消息列表
void createMessageList(String &messageList)
{
    // 临时实现
    messageList = "Message list not implemented\n";
}

// 创建基础消息列表
void createMessageListBase(String &messageList)
{
    // 临时实现
    messageList = "Base message list not implemented\n";
}

// Create the web server and web sockets
bool webServerAssignResources(int httpPort)
{
    do
    {
        // Freed by webServerStop
        // if (online.psram == true)
        //     incomingSettings = (char *)ps_malloc(AP_CONFIG_SETTING_SIZE);
        // else
        incomingSettings = (char *)malloc(AP_CONFIG_SETTING_SIZE);

        if (!incomingSettings)
        {
            systemPrintln("ERROR: Failed to allocate incomingSettings");
            break;
        }
        memset(incomingSettings, 0, AP_CONFIG_SETTING_SIZE);

        // Pre-load settings CSV
        // Freed by webServerStop
        // if (online.psram == true)
        //     settingsCSV = (char *)ps_malloc(AP_CONFIG_SETTING_SIZE);
        // else
        // settingsCSV = (char *)malloc(AP_CONFIG_SETTING_SIZE);

        // if (!settingsCSV)
        // {
        //     systemPrintln("ERROR: Failed to allocate settingsCSV");
        //     break;
        // }
        // createSettingsString(settingsCSV);

    //
    // https: // github.com/espressif/arduino-esp32/blob/master/libraries/DNSServer/examples/CaptivePortal/CaptivePortal.ino
        if (settings.enableCaptivePortal == true)
        {
            dnsServer.start();
        }

        webServer = new AsyncWebServer(httpPort);
        if (!webServer)
        {
            systemPrintln("ERROR: Failed to allocate webServer");
            break;
        }

        if (settings.enableCaptivePortal == true)
        {
            webServer->addHandler(new CaptiveRequestHandler());
        }

        // * index.html (not gz'd)
        // * favicon.ico

        // * /src/bootstrap.bundle.min.js - Needed for popper
        // * /src/bootstrap.min.css
        // * /src/bootstrap.min.js
        // * /src/jquery-3.6.0.min.js
        // * /src/main.js (not gz'd)
        // * /src/rtk-setup.png
        // * /src/style.css

        // * /src/fonts/icomoon.eot
        // * /src/fonts/icomoon.svg
        // * /src/fonts/icomoon.ttf
        // * /src/fonts/icomoon.woof

        // * /listfiles responds with a CSV of files and sizes in root
        // * /listMessages responds with a CSV of messages supported by this platform
        // * /listMessagesBase responds with a CSV of RTCM Base messages supported by this platform
        // * /file allows the download or deletion of a file

        webServer->onNotFound([](AsyncWebServerRequest *request)
                              { notFound(request); });

        webServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
// 检查index_html是否已定义
#ifdef index_html_found
                          request->send(200, "text/html", (const uint8_t *)index_html, sizeof(index_html));
#else
                          request->send(200, "text/html", "Welcome to ESP32 Web Server");
#endif
                      });

        webServer->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
#ifdef favicon_ico_found
                          request->send(200, "image/x-icon", (const uint8_t *)favicon_ico, sizeof(favicon_ico));
#else
                          request->send(404);
#endif
                      });

        webServer->on("/src/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
#ifdef bootstrap_bundle_min_js
                          request->send(200, "text/javascript", (const uint8_t *)bootstrap_bundle_min_js, sizeof(bootstrap_bundle_min_js));
#else
                          request->send(404);
#endif
                      });

        webServer->on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
#ifdef bootstrap_min_css
                          request->send(200, "text/css", (const uint8_t *)bootstrap_min_css, sizeof(bootstrap_min_css));
#else
                          request->send(404);
#endif
                      });

        webServer->on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
#ifdef bootstrap_min_js
                          request->send(200, "text/javascript", (const uint8_t *)bootstrap_min_js, sizeof(bootstrap_min_js));
#else
                          request->send(404);
#endif
                      });

        webServer->on("/src/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
#ifdef jquery_js
                          request->send(200, "text/javascript", (const uint8_t *)jquery_js, sizeof(jquery_js));
#else
                          request->send(404);
#endif
                      });

        webServer->on("/src/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
#ifdef main_js
                          request->send(200, "text/javascript", (const uint8_t *)main_js, sizeof(main_js));
#else
                          request->send(404);
#endif
                      });

#if defined(productVariant) && defined(RTK_EVK) && defined(rtkSetup_png) && defined(rtkSetupWiFi_png)
        webServer->on("/src/rtk-setup.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
            if (productVariant == RTK_EVK)
                request->send(200, "image/png", (const uint8_t *)rtkSetup_png, sizeof(rtkSetup_png));
            else
                request->send(200, "image/png", (const uint8_t *)rtkSetupWiFi_png, sizeof(rtkSetupWiFi_png)); });
#endif

// Battery icons
#ifdef batteryBlank_png
        webServer->on("/src/BatteryBlank.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)batteryBlank_png, sizeof(batteryBlank_png)); });
#endif
#ifdef battery0_png
        webServer->on("/src/Battery0.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)battery0_png, sizeof(battery0_png)); });
#endif
#ifdef battery1_png
        webServer->on("/src/Battery1.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)battery1_png, sizeof(battery1_png)); });
#endif
#ifdef battery2_png
        webServer->on("/src/Battery2.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)battery2_png, sizeof(battery2_png)); });
#endif
#ifdef battery3_png
        webServer->on("/src/Battery3.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)battery3_png, sizeof(battery3_png)); });
#endif

#ifdef battery0_Charging_png
        webServer->on("/src/Battery0_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)battery0_Charging_png, sizeof(battery0_Charging_png)); });
#endif
#ifdef battery1_Charging_png
        webServer->on("/src/Battery1_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)battery1_Charging_png, sizeof(battery1_Charging_png)); });
#endif
#ifdef battery2_Charging_png
        webServer->on("/src/Battery2_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)battery2_Charging_png, sizeof(battery2_Charging_png)); });
#endif
#ifdef battery3_Charging_png
        webServer->on("/src/Battery3_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "image/png", (const uint8_t *)battery3_Charging_png, sizeof(battery3_Charging_png)); });
#endif

#ifdef style_css
        webServer->on("/src/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "text/css", (const uint8_t *)style_css, sizeof(style_css)); });
#endif

#ifdef icomoon_eot
        webServer->on("/src/fonts/icomoon.eot", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "text/plain", (const uint8_t *)icomoon_eot, sizeof(icomoon_eot)); });
#endif

#ifdef icomoon_svg
        webServer->on("/src/fonts/icomoon.svg", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "text/plain", (const uint8_t *)icomoon_svg, sizeof(icomoon_svg)); });
#endif

#ifdef icomoon_ttf
        webServer->on("/src/fonts/icomoon.ttf", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "text/plain", (const uint8_t *)icomoon_ttf, sizeof(icomoon_ttf)); });
#endif

#ifdef icomoon_woof
        webServer->on("/src/fonts/icomoon.woof", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "text/plain", (const uint8_t *)icomoon_woof, sizeof(icomoon_woof)); });
#endif

        // https://lemariva.com/blog/2017/11/white-hacking-wemos-captive-portal-using-micropython
        webServer->on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request)
                      { request->send(200, "text/plain", "Microsoft Connect Test"); });

#ifdef handleUpload
        // Handler for the /uploadFile form POST
        webServer->on(
            "/uploadFile", HTTP_POST,
            [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", ""); },
            handleUpload); // Run handleUpload function when file manager file is uploaded
#endif

#ifdef handleFirmwareFileUpload
        // Handler for the /uploadFirmware form POST
        webServer->on(
            "/uploadFirmware", HTTP_POST,
            [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", ""); },
            handleFirmwareFileUpload);
#endif

#ifdef getFileList
        // Handler for file manager
        webServer->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
            String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
            if (settings.debugWebServer == true)
                systemPrintln(logmessage);
            String files;
            getFileList(files);
            request->send(200, "text/plain", files); });
#endif

#ifdef createMessageList
        // Handler for supported messages list
        webServer->on("/listMessages", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
            String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
            if (settings.debugWebServer == true)
                systemPrintln(logmessage);
            String messages;
            createMessageList(messages);
            if (settings.debugWebServer == true)
                systemPrintln(messages);
            request->send(200, "text/plain", messages); });
#endif

#ifdef createMessageListBase
        // Handler for supported RTCM/Base messages list
        webServer->on("/listMessagesBase", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
            String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
            if (settings.debugWebServer == true)
                systemPrintln(logmessage);
            String messageList;
            createMessageListBase(messageList);
            if (settings.debugWebServer == true)
                systemPrintln(messageList);
            request->send(200, "text/plain", messageList); });
#endif

#ifdef handleFileManager
        // Handler for file manager
        webServer->on("/file", HTTP_GET, handleFileManager);
#endif

        webServer->begin();

        // Starts task for updating webServer with handleClient
        if (task.updateWebServerTaskRunning == false)
            xTaskCreate(
                updateWebServerTask,
                "UpdateWebServer",            // Just for humans
                updateWebServerTaskStackSize, // Stack Size - needs to be large enough to hold the file manager list
                nullptr,                      // Task input parameter
                updateWebServerTaskPriority,
                &updateWebServerTaskHandle); // Task handle

        if (settings.debugWebServer == true)
            systemPrintln("Web Server Started");
        reportHeapNow(false);

        // Start the web socket server on port 81 using <esp_http_server.h>
        if (websocketServerStart() == false)
        {
            if (settings.debugWebServer == true)
                systemPrintln("Web Sockets failed to start");

            webServerStopSockets();
            webServerReleaseResources();

            return (false);
        }

        if (settings.debugWebServer == true)
            systemPrintln("Web Socket Server Started");

        reportHeapNow(false);

        return true;
    } while (0);

    // Release the resources
    webServerStopSockets();
    webServerReleaseResources();
    return false;
}