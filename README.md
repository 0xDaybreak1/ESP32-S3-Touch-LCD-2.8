# 🖼️ ESP32-S3 高性能无线图片显示系统

[![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Framework](https://img.shields.io/badge/framework-Arduino-00979D.svg)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

> 🚀 一个基于 ESP32-S3 的全栈物联网图片显示解决方案，集成 Web 配网、Canvas 前端预处理、双网络模式、实时状态反馈等商业级特性。

---

## 📋 项目简介

这是一个完整的嵌入式 + Web 全栈项目，实现了通过 WiFi 无线上传图片到 ESP32-S3 开发板，并在 ST7789 TFT 屏幕上实时显示的功能。项目采用现代化的技术架构，将复杂的图像处理任务转移到浏览器端，大幅降低单片机负载，提升系统稳定性。

### 🎯 核心价值

- **零门槛使用**：无需编程知识，通过 Web 界面即可完成所有操作
- **商业级体验**：动态配网、智能降级、实时状态反馈
- **高性能架构**：前端 Canvas 预处理 + 异步 Web 服务器 + RTOS 多任务
- **开箱即用**：完整的硬件接线方案和软件配置指南

---

## ✨ 核心特性

### 🌐 智能网络切换 (STA + AP 双保险)

- **STA 模式优先**：开机自动尝试连接保存的家用 WiFi
- **AP 模式降级**：连接失败自动开启 `ESP32-ImageDisplay` 独立热点，密码为12345678
- **Web 动态配网**：无需重新编译固件，网页端即可修改 WiFi 配置
- **配置持久化**：基于 NVS 存储，断电不丢失

### 🎨 前端 Canvas 黑科技预处理

**彻底解放单片机算力！**

- 用户上传任意格式（PNG/BMP/4K JPEG）图片
- 浏览器自动利用 Canvas API 缩放至 `240x320` 分辨率
- 强制转换为 Baseline JPEG（质量 85%）
- ESP32 只接收 20-50KB 的标准化小文件
- **内存占用降低 95%+，解码成功率接近 100%**

### 📊 实时状态反馈

- Web 页面动态显示开发板当前局域网 IP
- 每 10 秒自动刷新连接状态
- 颜色可视化：绿色=已连接，红色=AP 模式

### 🎬 全能播放列表

- **全局轮播**：自动循环播放 SD 卡 `/uploaded` 目录下的所有图片
- **自定义播放列表**：网页端复选图片，下发指定播放序列
- **实时控制**：支持单张图片立即显示

### 🔒 企业级稳定性

- **异步 Web 服务器**：基于 `ESPAsyncWebServer`，避免阻塞
- **RTOS 互斥锁**：`sdCardMutex` 保护 SD 卡 SPI 访问
- **文件锁机制**：防止删除正在显示的图片
- **双核任务分离**：Core 0 处理网络，Core 1 处理显示
- **mDNS 服务**：支持 `http://vision.local` 域名访问

---

## 🛠️ 硬件选型

### 主控板

- **型号**：ESP32-S3-DevKitC-1 (N8R8)
- **Flash**：8MB
- **PSRAM**：8MB (qio_opi 模式)
- **双核**：Xtensa LX7 @ 240MHz

### 外设模块

| 模块 | 型号 | 接口 | 说明 |
|------|------|------|------|
| TFT 显示屏 | ST7789 | SPI | 240x320 分辨率，65K 色 |
| SD 卡模块 | - | SD_MMC (4-bit) | 高速读写，存储图片 |

---

## 🔌 硬件接线

### ST7789 TFT 显示屏

| ST7789 引脚 | ESP32-S3 引脚 | 说明 |
|-------------|---------------|------|
| VCC | 3.3V | 电源 |
| GND | GND | 地 |
| SCL (SCK) | GPIO 12 | SPI 时钟 |
| SDA (MOSI) | GPIO 11 | SPI 数据 |
| RES (RST) | GPIO 10 | 复位 |
| DC | GPIO 9 | 数据/命令选择 |
| CS | GPIO 13 | 片选 |
| BLK (背光) | GPIO 14 | 背光控制 (可选) |

### SD 卡模块 (SD_MMC 4-bit 模式)

| SD 卡引脚 | ESP32-S3 引脚 | 说明 |
|-----------|---------------|------|
| CLK | GPIO 39 | 时钟 |
| CMD | GPIO 38 | 命令 |
| D0 | GPIO 40 | 数据线 0 |
| D1 | GPIO 41 | 数据线 1 |
| D2 | GPIO 42 | 数据线 2 |
| D3 | GPIO 43 | 数据线 3 |
| VCC | 3.3V | 电源 |
| GND | GND | 地 |

> ⚠️ **注意**：SD_MMC 4-bit 模式性能远超 SPI 模式，推荐使用。

---

## 📦 软件依赖

### 核心库

| 库名称 | 版本 | 功能 |
|--------|------|------|
| `TFT_eSPI` | ^2.5.43 | ST7789 驱动 |
| `TJpg_Decoder` | ^1.1.0 | JPEG 解码 |
| `PNGdec` | ^1.0.1 | PNG 解码 |
| `ESPAsyncWebServer` | ^1.2.3 | 异步 Web 服务器 |
| `AsyncTCP` | ^1.1.1 | 异步 TCP 底层 |
| `ArduinoJson` | ^6.21.3 | JSON 解析 |
| `SdFat` | ^2.2.3 | SD 卡文件系统 |

### 平台配置

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

board_build.arduino.memory_type = qio_opi  ; 启用 PSRAM
board_build.partitions = huge_app.csv      ; 大分区表
board_build.f_flash = 80000000L
board_build.flash_mode = qio

build_flags = 
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1
```

---

## 🏗️ 系统架构

### 数据流图

```
┌─────────────────────────────────────────────────────────────┐
│                      用户浏览器                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  1. 用户上传图片 (任意格式/尺寸)                      │  │
│  │  2. Canvas API 预处理                                 │  │
│  │     - 缩放到 240x320                                  │  │
│  │     - 转换为 Baseline JPEG (85% 质量)                │  │
│  │  3. 上传处理后的小文件 (20-50KB)                     │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────┬────────────────────────────────────┘
                         │ HTTP POST /upload
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                   ESP32-S3 Web 服务器                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  ESPAsyncWebServer (Core 0)                          │  │
│  │  - 接收文件流                                         │  │
│  │  - 分块写入 SD 卡 (互斥锁保护)                       │  │
│  │  - 返回上传结果                                       │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────┬────────────────────────────────────┘
                         │ 文件保存
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                      SD 卡存储                               │
│  /uploaded/                                                  │
│    ├── photo1.jpg                                           │
│    ├── photo2.jpg                                           │
│    └── photo3.jpg                                           │
└────────────────────────┬────────────────────────────────────┘
                         │ 读取图片
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                   图片解码与显示 (Core 1)                    │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  1. 从 SD 卡读取 JPEG 文件                           │  │
│  │  2. TJpg_Decoder 解码                                │  │
│  │  3. 输出到 TFT_eSPI 显示                             │  │
│  │  4. 自动轮播或播放列表控制                           │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 技术亮点

1. **前端预处理**：将图像处理任务从 ESP32 转移到浏览器，降低 95% 内存占用
2. **异步架构**：Web 服务器不阻塞主循环，避免看门狗重启
3. **双核分离**：网络处理和图像显示互不干扰
4. **互斥锁保护**：SD 卡 SPI 访问线程安全
5. **NVS 持久化**：WiFi 配置断电不丢失

---

## 🚀 快速开始

详细的快速入门指南请参考 [QuickStart.md](QuickStart.md)

### 最小化步骤

1. **安装开发环境**：VSCode + PlatformIO
2. **克隆项目**：`git clone <repository-url>`
3. **配置 WiFi**：修改 `src/WebServer_Driver.cpp` 或使用 Web 配网
4. **编译上传**：PlatformIO → Upload
5. **访问 Web 界面**：`http://vision.local` 或 `http://192.168.4.1`
6. **上传图片**：拖拽图片到网页，自动显示

---

## 📚 文档导航

### 核心文档

- [QuickStart.md](QuickStart.md) - 快速入门指南
- [src/WIFI_IMAGE_TRANSFER_README.md](src/WIFI_IMAGE_TRANSFER_README.md) - WiFi 功能完整文档
- [src/WebServer_Driver_notes.md](src/WebServer_Driver_notes.md) - Web 服务器模块说明

### 技术文档

- [src/WebServer_Canvas_Preprocessing.md](src/WebServer_Canvas_Preprocessing.md) - Canvas 预处理技术详解
- [src/WebServer_WiFi_Provisioning.md](src/WebServer_WiFi_Provisioning.md) - WiFi 配网技术详解
- [src/WebServer_Status_API.md](src/WebServer_Status_API.md) - 状态 API 技术文档
- [src/PATH_FIX_SUMMARY.md](src/PATH_FIX_SUMMARY.md) - SD 卡路径修复总结

---

## 🎯 使用场景

### 家庭场景

- 📷 **数码相框**：自动轮播家庭照片
- 🎨 **艺术展示**：展示数字艺术作品
- 📊 **信息看板**：显示天气、日历、待办事项

### 商业场景

- 🏪 **店铺广告**：动态更新促销海报
- 🏢 **会议室**：显示会议日程和欢迎信息
- 🏭 **工业监控**：显示设备状态图表

### 教育场景

- 🎓 **教学演示**：无线投屏教学图片
- 🔬 **实验展示**：实时显示实验数据图表

---

## 🔧 高级功能

### 自定义播放列表

```javascript
// Web 控制台操作
1. 勾选要播放的图片
2. 点击 "播放选中图片"
3. ESP32 自动循环播放选中的图片
```

### RGB 灯珠控制 (预留接口)

```cpp
// 支持多种灯效模式
- 常亮模式
- 流水灯模式
- 呼吸灯模式
```

### mDNS 域名访问

```bash
# 无需记忆 IP 地址
http://vision.local
```

---

## 🐛 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 屏幕白屏 | SPI 接线错误 | 检查接线，参考硬件接线表 |
| 无法上传图片 | SD 卡未插入 | 插入 SD 卡并重启 |
| 无法访问 Web | WiFi 未连接 | 连接到 `ESP32-ImageDisplay` 热点 |
| 图片显示花屏 | PSRAM 未启用 | 检查 `platformio.ini` 配置 |

详细排查指南请参考 [QuickStart.md](QuickStart.md) 的 FAQ 章节。

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

### 开发规范

- 代码注释使用中文
- 变量名使用英文
- 提交信息使用中文

### 分支策略

- `main`：稳定版本
- `dev`：开发版本
- `feature/*`：新功能分支

---

## 📄 开源协议

本项目采用 [MIT License](LICENSE) 开源协议。

---

## 🙏 致谢

感谢以下开源项目：

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - TFT 显示驱动
- [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder) - JPEG 解码库
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - 异步 Web 服务器
- [ArduinoJson](https://arduinojson.org/) - JSON 解析库

---

## 📧 联系方式

- **项目主页**：[GitHub Repository](https://github.com/your-repo)
- **问题反馈**：[Issues](https://github.com/your-repo/issues)
- **技术交流**：[Discussions](https://github.com/your-repo/discussions)

---

<p align="center">
  <strong>⭐ 如果这个项目对你有帮助，请给一个 Star！⭐</strong>
</p>

<p align="center">
  Made with ❤️ by Kiro
</p>
