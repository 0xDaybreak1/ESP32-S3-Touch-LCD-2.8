# WebServer_Driver 模块说明

## 📋 模块概述

**文件**: `WebServer_Driver.h` / `WebServer_Driver.cpp`  
**功能**: ESP32-S3 WiFi Web 服务器，提供图片上传、管理和显示控制  
**创建时间**: 2026-02-22  
**最后更新**: 2026-02-23  

---

## 🎯 核心功能

1. **WiFi 连接**
   - AP 模式：创建热点 `ESP32-ImageDisplay`
   - STA 模式：连接现有 WiFi 网络
   - **动态配网**：Web 界面配置 WiFi，保存到 NVS
   - mDNS 服务：域名 `vision.local` 自动解析

2. **Web 服务器**
   - 基于 `ESPAsyncWebServer` 异步架构
   - 嵌入式 HTML 界面（响应式设计）
   - RESTful API 接口

3. **图片管理**
   - 上传：分块接收，流式写入 SD 卡
   - 列表：扫描 `/uploaded` 目录
   - 显示：控制屏幕显示指定图片
   - 删除：文件锁保护，防止删除正在显示的图片

4. **播放列表**
   - 自定义播放列表：选择特定图片循环播放
   - 全局轮播：自动播放所有图片

5. **RGB 灯珠控制**（预留接口）
   - 常亮、流水灯、呼吸灯模式

---

## 🔧 最新修改记录

### 2026-02-23：Web 控制台动态显示局域网 IP

**修改人**: Kiro  

**问题**:
- 用户不知道开发板在局域网中的 IP 地址
- 需要查看路由器 DHCP 列表或串口日志才能获取 IP

**解决方案**:
在 Web 前端页面动态展示 ESP32-S3 当前的局域网 IP 地址

**实现细节**:

1. **后端新增 `/status` API** (`WebServer_Driver.cpp`)
   - 检查 STA 模式连接状态
   - 返回 JSON 数据：`sta_ip`, `connected`, `ap_mode`, `ap_ip`
   - 使用 `WiFi.localIP().toString()` 获取 IP
   - 未连接时返回 "未连接"

2. **前端 UI 修改** (`index_html`)
   - 在 `.header` 区域添加 IP 显示元素
   - `<p id="ipDisplay">` 用于动态更新 IP 信息
   - 根据连接状态显示不同颜色（绿色=已连接，红色=AP模式）

3. **前端逻辑修改** (`index_html` JavaScript)
   - 新增 `fetchSystemStatus()` 异步函数
   - 使用 `fetch('/status')` 请求后端接口
   - 动态更新 `ipDisplay` 元素文本
   - 页面加载时自动调用一次
   - 每 10 秒自动刷新一次状态

**技术优势**:
- ✅ 用户友好：无需查看路由器或串口即可获取 IP
- ✅ 实时更新：每 10 秒自动刷新状态
- ✅ 状态可视化：不同颜色区分连接状态
- ✅ 多模式支持：显示 STA IP 或 AP IP

**显示效果**:
- STA 已连接：`🌍 局域网 IP: 192.168.1.105` (绿色)
- AP 模式：`📡 AP 模式 IP: 192.168.4.1 (未连接局域网)` (红色)
- 未连接：`🌍 局域网 IP: 未连接` (红色)

---

### 2026-02-23：商业级 Web 动态配网功能

**修改人**: Kiro  

**问题**:
- 硬编码 WiFi 密码体验差，不适合商业产品
- 每次更换 WiFi 需要重新编译上传固件

**解决方案**:
引入基于 `Preferences` (NVS) 的 Web 动态配网

**实现细节**:

1. **引入 NVS 存储**
   - 包含 `<Preferences.h>` 头文件
   - 实例化全局对象 `Preferences preferences`
   - 使用命名空间 `"wifi"` 存储 `ssid` 和 `password`

2. **重构启动逻辑** (`WebServer_Init()`)
   - 开机时从 NVS 读取保存的 WiFi 配置
   - 如果有配置：尝试连接 STA 模式（10 秒超时）
   - 连接成功：正常启动 WebServer，不开启 AP
   - 连接失败或无配置：启动 AP 配网模式

3. **新增配网 Web 界面**
   - 新增 `wifi_html` 页面：包含 SSID 和 Password 输入框
   - 路由 `/wifi`：返回配网页面
   - 路由 `/setwifi`：接收配置，保存到 NVS，延迟 2 秒后重启

4. **新增配网辅助函数**
   - `loadWiFiConfig()`: 从 NVS 读取配置
   - `saveWiFiConfig()`: 保存配置到 NVS
   - `connectToWiFi()`: 连接到指定 WiFi（带超时）
   - `clearWiFiConfig()`: 清除 NVS 配置

5. **主页面添加配网入口**
   - 新增 "WiFi 配置" 区域
   - 按钮跳转到 `/wifi` 配网页面

**技术优势**:
- ✅ 商业级用户体验：无需编程知识即可配网
- ✅ 配置持久化：断电重启后自动连接
- ✅ 智能降级：连接失败自动进入 AP 配网模式
- ✅ 安全重启：配置保存后自动重启生效

**使用流程**:
1. 首次启动：自动进入 AP 模式（`ESP32-ImageDisplay`）
2. 连接热点，访问 `http://192.168.4.1`
3. 点击 "WiFi 配网"，输入家庭 WiFi 信息
4. 保存后设备自动重启，连接到家庭 WiFi
5. 后续访问：`http://vision.local`

---

### 2026-02-23：前端 Canvas 预处理重构

**修改人**: Kiro  

**问题**:
- ESP32 解析复杂格式图片（Progressive JPEG、大尺寸 PNG）导致内存溢出
- 解码失败率高，系统不稳定

**解决方案**:
将图像处理压力转移到客户端浏览器

**实现细节**:

1. **新增 `preprocessImage()` 函数**（JavaScript）
   - 使用 `FileReader` 读取用户上传的图片
   - 创建离屏 `Canvas` 和 `Image` 对象
   - 强制缩放到 `240x320` 分辨率
   - 采用 `cover` 模式：填满画布，超出部分裁切
   - 使用 `canvas.toBlob('image/jpeg', 0.85)` 转换为 Baseline JPEG
   - 文件名自动替换为 `.jpg` 后缀

2. **修改 `handleFiles()` 函数**
   - 拦截用户上传的文件
   - 调用 `preprocessImage()` 进行预处理
   - 将处理后的 JPEG 传递给 `uploadFile()`

3. **更新 Web 界面提示**
   - 上传区域：`"支持任意图片格式 (自动转换为 240x320 JPEG)"`
   - 文件选择器：`accept="image/*"`

**技术优势**:
- ✅ 无论用户上传任意尺寸、任意格式，ESP32 只收到 240x320 的 Baseline JPEG
- ✅ 大幅降低 ESP32 内存占用和解码复杂度
- ✅ 提升系统稳定性，避免内存溢出
- ✅ 减少网络传输流量（预处理后文件更小）

**测试建议**:
- 上传 4K 图片，验证自动缩放
- 上传 PNG/BMP 格式，验证自动转换
- 上传 Progressive JPEG，验证转换为 Baseline JPEG
- 检查浏览器控制台日志

---

## 📡 API 接口

| 路径 | 方法 | 功能 | 参数 | 返回 |
|------|------|------|------|------|
| `/` | GET | 主页面 | - | HTML |
| `/wifi` | GET | WiFi 配网页面 | - | HTML |
| `/status` | GET | 系统状态查询 | - | JSON (sta_ip, connected, ap_mode, ap_ip) |
| `/setwifi` | POST | 保存 WiFi 配置 | ssid, password (JSON) | JSON |
| `/upload` | POST | 上传图片 | file (multipart) | JSON |
| `/list` | GET | 图片列表 | - | JSON |
| `/display` | GET | 显示图片 | file (query) | JSON |
| `/delete` | GET | 删除图片 | file (query) | JSON |
| `/playlist` | POST | 设置播放列表 | playlist (JSON) | JSON |
| `/led` | POST | RGB 控制 | mode, color, brightness | JSON |

---

## 🔒 线程安全

- **SD 卡互斥锁** (`sdCardMutex`)：保护 SD 卡 SPI 访问
- **文件锁机制** (`currentDisplayFile`)：防止删除正在显示的图片
- **双核任务分离**：Core 0 处理网络，Core 1 处理显示

---

## 📚 相关文档

- [WIFI_IMAGE_TRANSFER_README.md](./WIFI_IMAGE_TRANSFER_README.md) - WiFi 功能完整文档
- [WebServer_Canvas_Preprocessing.md](./WebServer_Canvas_Preprocessing.md) - Canvas 预处理技术详解
- [WebServer_WiFi_Provisioning.md](./WebServer_WiFi_Provisioning.md) - WiFi 配网技术详解
- [WIFI_QUICK_START.md](./WIFI_QUICK_START.md) - 快速开始指南

---

**文档版本**: v1.3  
**创建时间**: 2026-02-23  
**创建者**: Kiro  
**最后更新**: 2026-02-23
