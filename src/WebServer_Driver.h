#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <vector>
#include "SD_Card.h"

// WiFi 配置
#define WIFI_AP_SSID        "ESP32-ImageDisplay"
#define WIFI_AP_PASSWORD    "12345678"
#define MDNS_HOSTNAME       "vision"

// 上传目录
#define UPLOAD_DIR          "/uploaded"

// 全局变量
extern AsyncWebServer server;
extern SemaphoreHandle_t sdCardMutex;  // SD 卡访问互斥锁
extern char currentDisplayFile[100];   // 当前正在显示的文件

// 播放列表相关
extern std::vector<String> customPlaylist;  // 自定义播放列表
extern bool useCustomPlaylist;              // 是否使用自定义播放列表

// 函数声明
void WebServer_Init();
void WebServer_Loop();
String getLocalIP();
String getAPIP();
bool isClientConnected();

// 文件管理
bool listImageFiles(const char* directory, String& jsonList);
bool deleteImageFile(const char* filepath);
bool isFileInUse(const char* filepath);
void lockFile(const char* filepath);
void unlockFile(const char* filepath);
