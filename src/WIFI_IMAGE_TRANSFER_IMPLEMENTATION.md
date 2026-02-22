# WiFi 无线图片传输功能实现方案

## 📋 项目概述

本文档详细说明如何在 ESP32-S3 开发板上实现 WiFi 无线图片传输功能，支持通过 Web 界面上传图片到 SD 卡并显示。

---

## 🎯 核心技术选型

### 1. 网络架构
- **ESPAsyncWebServer** (异步 Web 服务器)
  - ✅ 非阻塞架构，不会卡顿主循环和屏幕刷新
  - ✅ 支持大文件分块上传 (Chunked Transfer)
  - ✅ 事件驱动，CPU 占用低
  - ❌ 避免使用同步的 WebServer 库（会导致看门狗重启）

- **AsyncTCP** (异步 TCP 底层库)
  - ESPAsyncWebServer 的依赖库

### 2. WiFi 工作模式
- **AP + STA 双模共存** (WIFI_AP_STA)
  - AP 模式：创建热点 "ESP32-ImageDisplay"，密码 "12345678"
  - STA 模式：可连接到现有路由器
  - 用户可通过任一方式访问 Web 界面

- **mDNS 服务**
  - 域名：`http://vision.local`
  - 无需记忆 IP 地址，自动发现设备

### 3. 双核任务分离
- **Core 0 (PRO_CPU)**
  - WiFi 驱动
  - AsyncWebServer 处理
  - 文件上传接收和 SD 卡写入

- **Core 1 (APP_CPU)**
  - 主循环 (loop)
  - 图片解码和屏幕刷新
  - RGB 灯珠控制
  - 串口 AT 指令处理

### 4. 文件冲突管理
- **临时文件机制**
  - 上传的图片先保存为 `/uploaded/temp.jpg`
  - 上传完成后重命名为目标文件名
  - 避免正在显示的图片被覆盖导致解码崩溃

- **文件锁机制**
  - 使用 Mutex 保护 SD 卡访问
  - 显示任务和上传任务互斥访问 SD 卡

---

## 📦 依赖库清单

需要在 `platformio.ini` 中添加以下库：

```ini
lib_deps = 
    ; 现有库...
    moononournation/GFX Library for Arduino @ ^1.4.6
    bodmer/TJpg_Decoder @ ^1.1.0
    bitbank2/PNGdec @ ^1.0.1
    fastled/FastLED @ ^3.6.0
    
    ; 新增：异步 Web 服务器
    me-no-dev/ESPAsyncWebServer @ ^1.2.3
    me-no-dev/AsyncTCP @ ^1.1.1
```

---

## 🏗️ 实现步骤

### 步骤 1: 创建 Web 服务器驱动模块

**文件：** `src/WebServer_Driver.h` 和 `src/WebServer_Driver.cpp`

**功能：**
- 初始化 WiFi (AP + STA 双模)
- 启动 mDNS 服务
- 配置 ESPAsyncWebServer
- 处理文件上传请求
- 提供 Web 控制界面 (HTML/CSS/JavaScript)

**关键 API：**
```cpp
void WebServer_Init();           // 初始化 Web 服务器
void WebServer_Loop();           // 主循环处理 (可选)
String getLocalIP();             // 获取 IP 地址
bool isClientConnected();        // 检查是否有客户端连接
```

---

### 步骤 2: 实现文件上传处理

**核心逻辑：**

1. **接收分块数据**
   ```cpp
   server.on("/upload", HTTP_POST, 
       [](AsyncWebServerRequest *request) {
           // 上传完成回调
       },
       [](AsyncWebServerRequest *request, String filename, 
          size_t index, uint8_t *data, size_t len, bool final) {
           // 分块数据处理
       }
   );
   ```

2. **流式写入 SD 卡**
   - 使用 `File.write()` 逐块写入
   - 避免一次性加载整个文件到内存

3. **上传完成后处理**
   - 关闭临时文件
   - 重命名为目标文件名
   - 通知主任务刷新图片列表

---

### 步骤 3: 创建 Web 控制界面

**文件：** 嵌入在 `WebServer_Driver.cpp` 中的 HTML 字符串

**界面功能：**
- 📤 图片上传区域 (支持拖拽)
- 🖼️ 图片列表显示 (从 SD 卡读取)
- 🎨 RGB 灯珠控制面板
  - 颜色选择器
  - 亮度滑块
  - 模式切换 (常亮/流水灯/呼吸灯)
- 📺 图片显示控制
  - 选择图片显示
  - 切换图片
  - 删除图片

**技术栈：**
- HTML5 + CSS3 (响应式设计)
- JavaScript (Fetch API 异步通信)
- 无需外部框架，纯原生实现

---

### 步骤 4: 实现 SD 卡文件管理 API

**新增接口：**

```cpp
// 列出指定目录下的所有图片文件
bool listImageFiles(const char* directory, 
                    char fileList[][100], 
                    uint16_t maxFiles);

// 删除指定文件
bool deleteFile(const char* filepath);

// 检查文件是否正在被使用
bool isFileInUse(const char* filepath);

// 标记文件为使用中
void lockFile(const char* filepath);

// 解锁文件
void unlockFile(const char* filepath);
```

---

### 步骤 5: 集成到主程序

**修改 `main.cpp`：**

1. **包含头文件**
   ```cpp
   #include "WebServer_Driver.h"
   ```

2. **在 `setup()` 中初始化**
   ```cpp
   void setup() {
       // 现有初始化代码...
       
       // 初始化 Web 服务器
       WebServer_Init();
       
       Serial.println("Web 服务器已启动");
       Serial.print("访问地址: http://vision.local 或 http://");
       Serial.println(getLocalIP());
   }
   ```

3. **在 `loop()` 中处理**
   ```cpp
   void loop() {
       // 现有图片显示逻辑...
       
       // Web 服务器处理 (可选，异步库不需要)
       // WebServer_Loop();
       
       vTaskDelay(pdMS_TO_TICKS(10));
   }
   ```

---

### 步骤 6: RGB 灯珠控制 (可选扩展)

**如果使用 WS2812 幻彩灯带：**
- 使用 ESP32-S3 的 RMT 外设
- FastLED 库已支持 RMT 驱动

**如果使用普通 PWM RGB 灯珠：**
- 使用 LEDC 外设 (硬件 PWM)
- 零 CPU 占用，不阻塞主循环

**实现文件：** `src/RGB_LED_Driver.h` 和 `src/RGB_LED_Driver.cpp`

---

## 🔒 安全与稳定性考虑

### 1. 内存管理
- ✅ 使用 PSRAM 存储大文件缓冲区
- ✅ 及时释放不再使用的内存
- ✅ 监控堆内存使用情况

### 2. 看门狗保护
- ✅ 使用异步库避免长时间阻塞
- ✅ 在耗时操作中调用 `vTaskDelay()` 喂狗
- ✅ 合理设置任务优先级

### 3. 文件系统保护
- ✅ 使用 Mutex 保护 SD 卡访问
- ✅ 文件锁机制防止冲突
- ✅ 上传失败时清理临时文件

### 4. 网络安全
- ⚠️ AP 模式设置密码
- ⚠️ 限制上传文件大小 (建议 10MB 以内)
- ⚠️ 验证文件格式 (仅允许 JPEG/PNG/BMP)

---

## 📊 性能指标

### 预期性能
- **上传速度：** 500KB/s ~ 1MB/s (取决于 WiFi 信号)
- **图片切换延迟：** < 500ms (JPEG 解码)
- **屏幕刷新率：** 不受上传影响，保持流畅
- **内存占用：** 
  - 静态：约 200KB (Web 服务器 + 缓冲区)
  - 动态：上传时额外 4KB ~ 8KB (分块缓冲)

---

## 🧪 测试计划

### 功能测试
1. ✅ WiFi AP 模式连接测试
2. ✅ WiFi STA 模式连接测试
3. ✅ mDNS 域名解析测试
4. ✅ Web 界面访问测试
5. ✅ 小文件上传测试 (< 1MB)
6. ✅ 大文件上传测试 (3MB ~ 5MB)
7. ✅ 多文件连续上传测试
8. ✅ 上传过程中图片显示测试 (验证无卡顿)
9. ✅ 文件删除测试
10. ✅ RGB 灯珠控制测试

### 压力测试
1. ⚠️ 长时间运行稳定性测试 (24 小时)
2. ⚠️ 多客户端并发访问测试
3. ⚠️ 内存泄漏检测
4. ⚠️ 看门狗触发测试

---

## 📝 后续扩展

### 短期扩展
- [ ] 串口 AT 指令控制
- [ ] 图片幻灯片播放
- [ ] 图片缩放和旋转

### 长期扩展
- [ ] OTA 固件升级
- [ ] MQTT 远程控制
- [ ] 图片云端同步
- [ ] 视频播放支持

---

## 🔧 故障排查

### 常见问题

**Q1: 上传大文件时开发板重启**
- A: 检查是否使用了同步 WebServer 库，必须使用 ESPAsyncWebServer

**Q2: 上传后图片无法显示**
- A: 检查文件格式是否正确，SD 卡是否有足够空间

**Q3: 无法访问 http://vision.local**
- A: 检查设备和电脑是否在同一网络，尝试使用 IP 地址访问

**Q4: 屏幕刷新卡顿**
- A: 检查是否在主循环中使用了 `delay()`，改用 `vTaskDelay()`

**Q5: WiFi 连接不稳定**
- A: 检查天线连接，调整 WiFi 功率设置

---

## 📚 参考资料

- [ESPAsyncWebServer 官方文档](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ESP32-S3 技术参考手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [Arduino_GFX 库文档](https://github.com/moononournation/Arduino_GFX)
- [FastLED 库文档](https://github.com/FastLED/FastLED)

---

**文档版本：** v1.0  
**创建时间：** 2026-02-22  
**创建者：** Kiro  
**最后更新：** 2026-02-22


---

## 🐛 关键问题修复记录

### 问题 1: 串口波特率不匹配（已确认无问题）
- **现象**: 无法在串口监视器中看到 WiFi 热点信息
- **原因**: PlatformIO 串口监视器波特率设置为 9600，而 ESP32-S3 默认使用 115200
- **状态**: ✅ `platformio.ini` 中已正确配置 `monitor_speed = 115200`

### 问题 2: 双核 WiFi 驱动竞态条件（已修复）
- **修复时间**: 2026-02-22
- **修复人**: Kiro
- **现象**: WiFi AP 热点无法被手机搜索到
- **根本原因**: 
  - `DriverTask` (Core 0) 在启动时调用 `Wireless_Test2()`
  - `Wireless_Test2()` 会将 WiFi 设置为 **STA 模式**并扫描网络，最后执行 `WiFi.mode(WIFI_OFF)`
  - 与 `WebServer_Init()` (Core 1) 建立 AP 热点的操作产生**跨核并发冲突**
  - ESP32 WiFi 驱动在没有互斥锁保护的情况下无法承受跨核同时初始化
- **修复方案**: 
  - 在 `main.cpp` 的 `DriverTask()` 中注释掉 `Wireless_Test2()` 调用
  - 确保全系统只有 `WebServer_Init()` 一个入口初始化 WiFi
- **代码变更**:
  ```cpp
  void DriverTask(void *parameter) {
    // ⛔ 注释掉以避免与 AP 模式冲突
    // Wireless_Test2();
    while(1){
      PWR_Loop();
      // ... 其他循环代码
    }
  }
  ```

### 问题 3: SD 卡路径拼接错误（已修复 -> 已回退）
- **修复时间**: 2026-02-22
- **修复人**: Kiro
- **现象**: Web 上传的图片无法在屏幕上显示，图片列表显示"暂无图片"
- **根本原因**:
  - **VFS 路径挂载错位问题**
  - `SD_MMC.begin("/sdcard", true, true)` 指定了挂载点为 `/sdcard`
  - SD_MMC 库会自动将相对路径转换为绝对路径
  - 错误地手动添加 `/sdcard` 前缀，导致路径重复（如 `/sdcard/sdcard/uploaded/xxx.jpg`）
- **修复方案**:
  - **统一使用相对路径**（不带 `/sdcard` 前缀）
  - 让 SD_MMC 库自动处理挂载点转换
  - 所有接口都使用 `UPLOAD_DIR` 宏定义
- **代码变更**:
  ```cpp
  // /display 接口（正确）
  String filepath = String(UPLOAD_DIR) + "/" + filename;
  
  // /list 接口（正确）
  if (listImageFiles(UPLOAD_DIR, jsonList)) {
  ```

### 验证清单
完成上述修复后，请按以下步骤验证：

1. **重新编译并上传固件**
   ```bash
   pio run -t upload
   ```

2. **打开串口监视器（115200 波特率）**
   ```bash
   pio device monitor
   ```

3. **检查启动日志**
   - 应该看到 `WiFi AP 已启动` 的消息
   - 应该看到 `SSID: ESP32-ImageDisplay`
   - 应该看到 `IP: 192.168.4.1`
   - 应该看到 `Web 控制台: http://vision.local 或 http://192.168.4.1`

4. **手机搜索 WiFi**
   - 打开手机 WiFi 设置
   - 搜索名为 `ESP32-ImageDisplay` 的热点
   - 密码: `12345678`

5. **访问 Web 控制台**
   - 连接热点后，浏览器访问 `http://192.168.4.1`
   - 或使用 mDNS: `http://vision.local`

### 硬件检查（如果仍无热点）
如果完成上述修复后仍然搜索不到热点，请检查：

1. **ESP32-S3 模组型号**
   - 如果是 **WROOM-1U** 或带有 **U.FL (IPEX) 天线座**的版本
   - 必须外接天线，否则射频信号无法发射
   - 板载天线版本（如 WROOM-1）无需外接天线

2. **供电检查**
   - 虽然图片轮播正常说明供电充足
   - 但 WiFi 发射时瞬时电流可达 500mA
   - 建议使用至少 2A 的 USB 电源适配器

3. **RF 屏蔽**
   - 确保 ESP32-S3 模组周围没有金属外壳完全包裹
   - 金属外壳会严重衰减 2.4GHz 信号


### 问题 4: 图片列表无法显示（已修复）
- **修复时间**: 2026-02-22
- **修复人**: Kiro
- **现象**: 文件已上传到 SD 卡，但 Web 端图片库显示"暂无图片"
- **根本原因**:
  - `SD_MMC.openNextFile()` 返回的 `file.name()` 可能包含完整路径
  - 例如返回 `/sdcard/uploaded/123.jpg` 而不是 `123.jpg`
  - 导致文件名处理失败或前端无法正确显示
- **修复方案**:
  - 在 `listImageFiles` 函数中添加路径提取逻辑
  - 使用 `lastIndexOf('/')` 提取纯文件名
  - 添加详细的调试日志输出
- **代码变更**:
  ```cpp
  // 提取文件名（去除路径前缀）
  int lastSlash = filename.lastIndexOf('/');
  if (lastSlash >= 0) {
      filename = filename.substring(lastSlash + 1);
  }
  ```


---

## 🎬 动态图库轮播功能

### 实现时间
2026-02-22

### 功能描述
将硬编码的图片数组改为动态读取 SD 卡 `/uploaded` 目录，实现自动轮播所有上传的图片。

### 核心实现

#### 1. 全局变量
```cpp
static File uploadedDir;           // 上传目录句柄
static bool dirInitialized = false; // 目录是否已初始化
```

#### 2. `getNextImageFile()` 函数
- 动态遍历 `/uploaded` 目录
- 自动过滤隐藏文件（`.` 或 `._` 开头）
- 只返回图片格式（`.jpg`, `.jpeg`, `.png`, `.bmp`）
- 到达目录末尾时自动循环（`rewindDirectory()`）
- 返回完整路径（如 `/uploaded/123.jpg`）

#### 3. 修改后的 `loop()` 函数
- 移除硬编码的 `imageFiles` 数组
- 每 5 秒调用 `getNextImageFile()` 获取下一张图片
- 所有 SD 卡操作都在互斥锁保护下进行
- Web 请求优先级高于自动轮播

### 功能特性
- ✅ 动态目录遍历，无需预知图片数量
- ✅ 自动过滤隐藏文件和非图片格式
- ✅ 循环播放所有图片
- ✅ 线程安全（互斥锁保护）
- ✅ 路径兼容性（自动处理完整路径）

### 使用方法
1. 通过 Web 控制台上传图片
2. 系统自动每 5 秒切换一次图片
3. 可通过 Web 控制台手动选择图片

### 调试日志示例
```
✓ 已打开 /uploaded 目录
→ 找到图片: /uploaded/123.jpg
--- 自动轮播: /uploaded/123.jpg ---
✓ 渲染成功！
→ 目录遍历完成，重新开始轮播
```


---

## 🎬 自定义播放列表功能

### 实现时间
2026-02-22

### 功能描述
允许用户在 Web 控制台中勾选多个图片，创建自定义播放列表，ESP32-S3 将循环播放选中的图片。

### 核心实现

#### 1. 依赖库
- 添加 `ArduinoJson @ ^7.2.1` 用于 JSON 解析

#### 2. 全局变量（WebServer_Driver）
```cpp
std::vector<String> customPlaylist;  // 自定义播放列表
bool useCustomPlaylist = false;      // 是否使用自定义播放列表
```

#### 3. 前端修改
- 每个图片卡片添加复选框
- 添加"▶️ 播放选中图片"按钮
- 添加"⏹️ 停止播放列表"按钮
- 实现 `playSelectedImages()` 和 `stopPlaylist()` 函数

#### 4. 后端 API
- 新增 `/playlist` 接口（HTTP POST）
- 解析 JSON 格式的播放列表
- 空数组恢复全局轮播模式

#### 5. 主循环修改（main.cpp）
- 添加 `playlistIndex` 索引变量
- 判断 `useCustomPlaylist` 标志
- 自定义播放列表模式：从 `customPlaylist` 数组中获取文件名
- 全局轮播模式：调用 `getNextImageFile()` 遍历目录

### 功能特性
- ✅ 多选支持（复选框）
- ✅ 播放模式切换（自定义/全局）
- ✅ 循环播放（自动重置索引）
- ✅ 线程安全（互斥锁保护）
- ✅ 实时切换（无需重启）

### 使用方法
1. 在图片库中勾选要播放的图片
2. 点击"▶️ 播放选中图片"按钮
3. 系统开始循环播放选中的图片
4. 点击"⏹️ 停止播放列表"恢复全局轮播

### 调试日志示例
```
  添加到播放列表: 123.jpg
  添加到播放列表: 456.jpg
✓ 播放列表已设置 (2 张图片)

--- 播放列表轮播 [1/2]: /uploaded/123.jpg ---
✓ 渲染成功！

--- 播放列表轮播 [2/2]: /uploaded/456.jpg ---
✓ 渲染成功！
```

### JSON 格式
```json
{
  "playlist": ["123.jpg", "456.jpg"]
}
```

空播放列表（恢复全局轮播）：
```json
{
  "playlist": []
}
```


---

## 🖼️ 图片格式识别和 PNG 内存问题修复

### 修复时间
2026-02-22

### 问题分析

#### 问题 1：格式识别
- **现象**: 只能识别 .jpeg，.jpg 和 .png 无法显示
- **分析**: `getImageFormat()` 函数已正确使用 `strcasecmp()` 不区分大小写比较
- **结论**: 格式识别逻辑本身没有问题

#### 问题 2：PNG 内存墙（真正的根源）
- **现象**: PNG 图片解码失败或系统崩溃
- **根本原因**:
  - PNG 使用 Deflate 无损压缩，解码需要大量内存（32KB - 64KB+）
  - 原代码使用 `malloc()` 从内部 SRAM 分配内存
  - ESP32-S3 内部 SRAM 有限（约 512KB），容易堆栈溢出

### 修复方案

#### 核心修复：使用 PSRAM 分配内存
ESP32-S3 配备了 8MB PSRAM，应该优先使用 PSRAM 分配大块内存。

#### 修改内容

1. **引入 PSRAM API**
```cpp
#include <esp_heap_caps.h>
```

2. **修改内存分配策略**
```cpp
// 优先使用 PSRAM
uint8_t* buffer = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
if (buffer == nullptr) {
    // 降级到内部 RAM
    buffer = (uint8_t*)malloc(fileSize);
}
```

3. **修复的函数**
- `initImageDecoder()`: 图片缓冲区使用 PSRAM
- `displayJPEG()`: JPEG 文件缓冲区使用 PSRAM
- `displayPNG()`: PNG 文件缓冲区使用 PSRAM

### 内存使用对比

| 组件 | 修复前（SRAM） | 修复后（PSRAM） | 风险降低 |
|------|---------------|----------------|---------|
| 图片缓冲区 | 150KB | 150KB | ✅ |
| JPEG 文件 | 50KB | 50KB | ✅ |
| PNG 文件 | 100KB | 100KB | ✅ |
| **总计** | **300KB SRAM** | **300KB PSRAM** | **避免溢出** |

### 修复效果

#### 支持的格式
- ✅ .jpg / .JPG（JPEG 格式）
- ✅ .jpeg / .JPEG（JPEG 格式）
- ✅ .png / .PNG（PNG 格式）
- ✅ .bmp / .BMP（BMP 格式）

#### 调试日志
```
✓ 图片缓冲区已分配到 PSRAM
✓ 图片解码器初始化完成

正在加载 PNG 图片: /uploaded/test.png
PNG 文件大小: 85432 字节
✓ PNG 文件已完整读入内存,SD 卡总线已释放
PNG 信息 - 宽: 240, 高: 320
✓ PNG 图片显示完成
```

### 技术细节

#### PSRAM 分配 API
```cpp
void* heap_caps_malloc(size_t size, uint32_t caps)
```
- `MALLOC_CAP_SPIRAM`: 使用 SPI RAM (PSRAM)
- `MALLOC_CAP_8BIT`: 8 位可访问内存

#### 内存释放
```cpp
free(buffer);  // 自动识别内存类型
```

### 注意事项
1. PSRAM 访问速度比内部 SRAM 慢（约 40MHz vs 240MHz）
2. 适合存储大块数据，不适合频繁访问的小数据
3. 某些 DMA 操作不支持 PSRAM
