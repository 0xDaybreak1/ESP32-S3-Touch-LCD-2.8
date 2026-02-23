# ESP32-S3 Web 动态配网技术文档

## 📋 文档信息

**创建时间**: 2026-02-23  
**创建者**: Kiro  
**版本**: v1.0  

---

## 🎯 功能概述

本文档详细说明 ESP32-S3 基于 `Preferences` (NVS) 和 `ESPAsyncWebServer` 实现的商业级 Web 动态配网功能。

### 核心特性

- ✅ Web 界面配网：无需编程知识
- ✅ 配置持久化：断电重启后自动连接
- ✅ 智能降级：连接失败自动进入 AP 配网模式
- ✅ 安全重启：配置保存后自动重启生效
- ✅ 用户友好：响应式设计，支持移动端

---

## 🔧 技术架构

### 1. NVS 存储

**NVS (Non-Volatile Storage)** 是 ESP32 的非易失性存储系统，类似于 EEPROM。

```cpp
#include <Preferences.h>

Preferences preferences;

// 命名空间: "wifi"
// 键值对:
//   - "ssid": WiFi 名称
//   - "password": WiFi 密码
```

**优势**:
- 断电不丢失
- 读写速度快
- 支持命名空间隔离
- 自动磨损均衡

### 2. 启动流程

```
开机
  ↓
读取 NVS 配置
  ↓
有配置？
  ├─ 是 → 尝试连接 STA (10秒超时)
  │         ├─ 成功 → 正常运行 (STA 模式)
  │         └─ 失败 → 启动 AP 配网模式
  └─ 否 → 启动 AP 配网模式
```

### 3. 配网流程

```
用户连接 AP (ESP32-ImageDisplay)
  ↓
访问 http://192.168.4.1
  ↓
点击 "WiFi 配网"
  ↓
输入 SSID 和 Password
  ↓
点击 "保存并重启"
  ↓
配置保存到 NVS
  ↓
延迟 2 秒
  ↓
ESP32 重启
  ↓
自动连接到配置的 WiFi
  ↓
用户访问 http://vision.local
```

---

## 💻 代码实现

### 1. 头文件修改 (`WebServer_Driver.h`)

```cpp
#include <Preferences.h>

// 全局变量
extern Preferences preferences;
extern bool isAPMode;

// WiFi 配网相关函数
bool loadWiFiConfig(String& ssid, String& password);
bool saveWiFiConfig(const String& ssid, const String& password);
bool connectToWiFi(const String& ssid, const String& password, unsigned long timeout);
void clearWiFiConfig();
```

### 2. 配网辅助函数

#### 2.1 从 NVS 加载配置

```cpp
bool loadWiFiConfig(String& ssid, String& password) {
    preferences.begin("wifi", true);  // 只读模式
    
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    
    preferences.end();
    
    return (ssid.length() > 0);
}
```

#### 2.2 保存配置到 NVS

```cpp
bool saveWiFiConfig(const String& ssid, const String& password) {
    preferences.begin("wifi", false);  // 读写模式
    
    bool success = true;
    
    if (preferences.putString("ssid", ssid) == 0) {
        success = false;
    }
    
    if (preferences.putString("password", password) == 0) {
        success = false;
    }
    
    preferences.end();
    
    return success;
}
```

#### 2.3 连接到 WiFi

```cpp
bool connectToWiFi(const String& ssid, const String& password, unsigned long timeout) {
    Serial.printf("正在连接到 WiFi: %s\n", ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeout) {
            Serial.println("✗ WiFi 连接超时");
            return false;
        }
        
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    return true;
}
```

#### 2.4 清除配置

```cpp
void clearWiFiConfig() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    
    Serial.println("✓ WiFi 配置已清除");
}
```

### 3. 启动逻辑重构 (`WebServer_Init()`)

```cpp
void WebServer_Init() {
    // ... SD 卡初始化 ...
    
    // 🔧 尝试从 NVS 读取 WiFi 配置
    String savedSSID, savedPassword;
    bool hasConfig = loadWiFiConfig(savedSSID, savedPassword);
    
    if (hasConfig && savedSSID.length() > 0) {
        Serial.println("✓ 检测到已保存的 WiFi 配置");
        
        // 尝试连接到保存的 WiFi
        if (connectToWiFi(savedSSID, savedPassword, WIFI_CONNECT_TIMEOUT)) {
            // 连接成功，使用 STA 模式
            isAPMode = false;
            Serial.println("✓ WiFi 连接成功 (STA 模式)");
        } else {
            // 连接失败，启动 AP 模式
            Serial.println("✗ WiFi 连接失败，启动 AP 配网模式");
            WiFi.mode(WIFI_AP);
            WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
            isAPMode = true;
        }
    } else {
        // 没有保存的配置，直接启动 AP 模式
        Serial.println("✓ 未检测到 WiFi 配置，启动 AP 配网模式");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        isAPMode = true;
    }
    
    // ... mDNS 和 Web 服务器初始化 ...
}
```

### 4. Web 路由配置

#### 4.1 配网页面路由

```cpp
server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", wifi_html);
});
```

#### 4.2 配置保存路由

```cpp
server.on("/setwifi", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len != total) return;
        
        // 解析 JSON
        JsonDocument doc;
        deserializeJson(doc, data, len);
        
        String ssid = doc["ssid"].as<String>();
        String password = doc["password"].as<String>();
        
        if (ssid.length() == 0) {
            request->send(400, "application/json", 
                "{\"success\":false,\"message\":\"SSID 不能为空\"}");
            return;
        }
        
        // 保存到 NVS
        if (saveWiFiConfig(ssid, password)) {
            request->send(200, "application/json", 
                "{\"success\":true,\"message\":\"配置保存成功\"}");
            
            // 延迟 2 秒后重启
            delay(2000);
            ESP.restart();
        } else {
            request->send(500, "application/json", 
                "{\"success\":false,\"message\":\"配置保存失败\"}");
        }
    }
);
```

---

## 🎨 前端界面

### 配网页面 HTML

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>WiFi 配网</title>
    <style>
        /* 响应式设计样式 */
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📡 WiFi 配网</h1>
            <p>配置 ESP32 连接到您的 WiFi 网络</p>
        </div>
        
        <div class="content">
            <form id="wifiForm">
                <div class="form-group">
                    <label for="ssid">WiFi 名称 (SSID)</label>
                    <input type="text" id="ssid" required>
                </div>
                
                <div class="form-group">
                    <label for="password">WiFi 密码</label>
                    <input type="password" id="password" required>
                </div>
                
                <button type="submit">💾 保存并重启</button>
            </form>
        </div>
    </div>
    
    <script>
        // 表单提交逻辑
        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const response = await fetch('/setwifi', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ 
                    ssid: document.getElementById('ssid').value,
                    password: document.getElementById('password').value
                })
            });
            
            const data = await response.json();
            
            if (data.success) {
                // 显示成功提示，禁用表单
                // 3 秒后显示重启提示
            }
        });
    </script>
</body>
</html>
```

---

## 📊 使用场景

### 场景 1：首次使用

1. 用户收到设备，首次上电
2. 设备检测到 NVS 中无配置，启动 AP 模式
3. 用户手机连接到 `ESP32-ImageDisplay` 热点
4. 浏览器自动弹出配网页面（或手动访问 `192.168.4.1`）
5. 输入家庭 WiFi 信息，点击保存
6. 设备重启，自动连接到家庭 WiFi
7. 用户手机切换回家庭 WiFi，访问 `http://vision.local`

### 场景 2：更换 WiFi

1. 用户更换路由器或 WiFi 密码
2. 设备尝试连接旧配置，10 秒后超时失败
3. 自动降级到 AP 配网模式
4. 用户重新配网（同场景 1）

### 场景 3：恢复出厂设置

```cpp
// 在某个按钮或串口命令中调用
clearWiFiConfig();
ESP.restart();
```

---

## 🔒 安全考虑

### 1. 密码存储

- NVS 存储在 Flash 中，未加密
- 建议：生产环境可使用 ESP32 的 Flash 加密功能

### 2. AP 模式安全

- AP 模式使用密码保护（`WIFI_AP_PASSWORD`）
- 建议：生产环境使用强密码或动态密码

### 3. 配网超时

- 连接超时设置为 10 秒，避免长时间等待
- 可根据实际网络环境调整 `WIFI_CONNECT_TIMEOUT`

---

## 🧪 测试用例

### 测试 1：首次配网

**步骤**:
1. 清除 NVS：`clearWiFiConfig()` + 重启
2. 连接到 AP：`ESP32-ImageDisplay`
3. 访问：`http://192.168.4.1/wifi`
4. 输入正确的 WiFi 信息
5. 点击保存

**预期结果**:
- 配置保存成功
- 设备重启
- 自动连接到配置的 WiFi
- 可通过 `http://vision.local` 访问

### 测试 2：错误的 WiFi 密码

**步骤**:
1. 输入正确的 SSID，错误的密码
2. 保存并重启

**预期结果**:
- 设备尝试连接 10 秒后失败
- 自动降级到 AP 模式
- 可重新配网

### 测试 3：断电重启

**步骤**:
1. 配网成功后，断电
2. 重新上电

**预期结果**:
- 自动读取 NVS 配置
- 自动连接到 WiFi
- 无需重新配网

### 测试 4：清除配置

**步骤**:
1. 调用 `clearWiFiConfig()`
2. 重启设备

**预期结果**:
- NVS 配置被清除
- 设备进入 AP 配网模式

---

## 🚀 扩展功能

### 1. WiFi 扫描

在配网页面显示可用的 WiFi 列表，用户点击选择：

```cpp
server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN) + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
});
```

### 2. 配网状态查询

前端轮询查询连接状态：

```cpp
server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"mode\":\"" + String(isAPMode ? "AP" : "STA") + "\",";
    json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED) + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    request->send(200, "application/json", json);
});
```

### 3. 多 WiFi 配置

支持保存多个 WiFi 配置，自动尝试连接：

```cpp
// NVS 结构
// "wifi1_ssid", "wifi1_password"
// "wifi2_ssid", "wifi2_password"
// ...
```

### 4. 配网二维码

生成包含 WiFi 信息的二维码，用户扫码配网：

```
WIFI:T:WPA;S:MySSID;P:MyPassword;;
```

---

## 📚 相关技术文档

- [ESP32 Preferences 库文档](https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences)
- [ESP32 WiFi 库文档](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [ESPAsyncWebServer 文档](https://github.com/me-no-dev/ESPAsyncWebServer)
- [NVS 存储原理](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)

---

## 🐛 常见问题

### Q1: 配网后无法连接

**原因**:
- WiFi 密码错误
- 路由器 MAC 地址过滤
- 信号太弱

**解决**:
- 检查密码是否正确
- 设备会自动降级到 AP 模式，重新配网

### Q2: 重启后丢失配置

**原因**:
- NVS 分区损坏
- Flash 写入失败

**解决**:
- 调用 `clearWiFiConfig()` 清除配置
- 重新烧录固件

### Q3: 无法访问 vision.local

**原因**:
- mDNS 服务未启动
- 设备和手机不在同一网络

**解决**:
- 检查串口日志，确认 mDNS 启动成功
- 使用 IP 地址访问（查看路由器 DHCP 列表）

---

**文档版本**: v1.0  
**创建时间**: 2026-02-23  
**创建者**: Kiro  
**最后更新**: 2026-02-23
