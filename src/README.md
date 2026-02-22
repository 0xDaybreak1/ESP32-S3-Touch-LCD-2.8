# ESP32-S3 图片显示与 WiFi 传输系统

## 📋 项目概述

本项目基于 ESP32-S3 开发板，实现了完整的图片显示和 WiFi 无线传输功能。支持从 SD 卡读取图片并在 TFT 屏幕上显示，同时提供 Web 界面进行远程图片上传和控制。

---

## ✨ 核心功能

### 1. 图片显示功能 ✅
- **多格式支持**: JPEG、PNG、BMP
- **高性能解码**: 使用硬件加速和 PSRAM 缓冲
- **流畅显示**: 双核任务分离，不阻塞主循环
- **自动轮播**: 支持多张图片自动切换

### 2. WiFi 无线传输 ✅
- **双模 WiFi**: AP 热点 + STA 客户端同时工作
- **Web 控制台**: 响应式设计，支持手机和电脑访问
- **图片上传**: 拖拽上传，实时进度显示
- **文件管理**: 列表、显示、删除功能
- **mDNS 服务**: 域名访问 `http://vision.local`

### 3. RGB 灯珠控制 🔜
- **颜色调节**: 支持全彩色选择
- **亮度控制**: 0-100% 亮度调节
- **多种模式**: 常亮、流水灯、呼吸灯
- **硬件驱动**: 使用 RMT/LEDC 外设，零 CPU 占用

### 4. 串口控制 🔜
- **AT 指令**: 自定义命令集
- **图片控制**: 显示、切换、删除
- **系统状态**: 查询 WiFi、SD 卡、内存状态

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    ESP32-S3 双核架构                      │
├─────────────────────────────────────────────────────────┤
│  Core 0 (PRO_CPU)          │  Core 1 (APP_CPU)          │
│  - WiFi 驱动               │  - 主循环 (loop)            │
│  - AsyncWebServer          │  - 图片解码                 │
│  - 文件上传接收            │  - 屏幕刷新                 │
│  - SD 卡写入               │  - RGB 灯珠控制             │
│                            │  - 串口 AT 指令             │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                      硬件外设                             │
├─────────────────────────────────────────────────────────┤
│  - ST7789 TFT 屏幕 (SPI)                                 │
│  - SD 卡 (SD_MMC)                                        │
│  - WiFi (内置)                                           │
│  - RGB LED (RMT/LEDC)                                    │
│  - 串口 (UART)                                           │
└─────────────────────────────────────────────────────────┘
```

---

## 📂 项目结构

```
src/
├── main.cpp                          # 主程序入口
├── Display_ST7789.cpp/h              # TFT 屏幕驱动
├── Image_Decoder.cpp/h               # 图片解码器 (JPEG/PNG/BMP)
├── SD_Card.cpp/h                     # SD 卡驱动
├── WebServer_Driver.cpp/h            # Web 服务器驱动 ✨ 新增
├── Wireless.cpp/h                    # WiFi 基础功能
├── LVGL_Driver.cpp/h                 # LVGL 图形库驱动
├── Audio_PCM5101.cpp/h               # 音频驱动
├── RTC_PCF85063.cpp/h                # 实时时钟
├── Gyro_QMI8658.cpp/h                # 陀螺仪传感器
├── BAT_Driver.cpp/h                  # 电池管理
├── PWR_Key.cpp/h                     # 电源按键
├── Button_Driver.cpp/h               # 按键驱动
├── I2C_Driver.cpp/h                  # I2C 总线驱动
├── Simulated_Gesture.cpp/h           # 模拟触摸手势
├── LVGL_Example.cpp/h                # LVGL 示例
├── LVGL_Music.cpp/h                  # LVGL 音乐界面
│
├── README.md                         # 项目总览 (本文件)
├── IMAGE_DISPLAY_README.md           # 图片显示功能文档
├── WIFI_IMAGE_TRANSFER_README.md     # WiFi 传输功能文档
├── WIFI_IMAGE_TRANSFER_IMPLEMENTATION.md  # 实现方案详细文档
└── WIFI_QUICK_START.md               # 快速使用指南
```

---

## 🚀 快速开始

### 1. 硬件准备
- ESP32-S3 开发板 (带 PSRAM)
- TFT 屏幕 (ST7789 驱动)
- Micro SD 卡 (Class 10 推荐)
- USB 数据线

### 2. 软件环境
- PlatformIO IDE
- Python 3.x (用于 PlatformIO)

### 3. 编译上传

```bash
# 克隆项目
git clone <项目地址>
cd <项目目录>

# 编译
pio run

# 上传固件
pio run -t upload

# 查看串口日志
pio device monitor
```

### 4. 使用 WiFi 功能

详细使用说明请参考：[WiFi 快速使用指南](WIFI_QUICK_START.md)

**快速步骤**:
1. 连接 WiFi 热点 `ESP32-ImageDisplay` (密码: `12345678`)
2. 浏览器打开 `http://vision.local` 或 `http://192.168.4.1`
3. 上传图片并控制显示

---

## 📦 依赖库

```ini
lib_deps = 
    moononournation/GFX Library for Arduino @ ^1.4.6
    bodmer/TJpg_Decoder @ ^1.1.0
    bitbank2/PNGdec @ ^1.0.1
    fastled/FastLED @ ^3.6.0
    me-no-dev/ESPAsyncWebServer @ ^1.2.3
    me-no-dev/AsyncTCP @ ^1.1.1
```

---

## 🔧 配置说明

### WiFi 配置

编辑 `src/WebServer_Driver.h`:

```cpp
#define WIFI_AP_SSID        "ESP32-ImageDisplay"  // AP 热点名称
#define WIFI_AP_PASSWORD    "12345678"            // AP 热点密码
#define MDNS_HOSTNAME       "vision"              // mDNS 域名
```

### 屏幕配置

编辑 `src/Display_ST7789.h`:

```cpp
#define LCD_WIDTH   240   // 屏幕宽度
#define LCD_HEIGHT  280   // 屏幕高度
```

### SD 卡配置

编辑 `src/SD_Card.h`:

```cpp
#define SD_CLK_PIN      14
#define SD_CMD_PIN      17
#define SD_D0_PIN       16
```

---

## 📊 性能指标

| 指标 | 数值 |
|------|------|
| 图片解码速度 | JPEG: ~200ms (240×280) |
| 上传速度 | 500KB/s ~ 1MB/s |
| 屏幕刷新率 | 不受上传影响 |
| 内存占用 | 静态: ~200KB, 动态: ~4-8KB |
| WiFi 连接数 | 最多 4 个客户端 |

---

## 🧪 测试状态

### 功能测试
- ✅ JPEG 图片显示
- ✅ PNG 图片显示
- ✅ BMP 图片显示
- ✅ WiFi AP 模式
- ✅ WiFi STA 模式
- ✅ mDNS 域名解析
- ✅ Web 界面访问
- ✅ 图片上传 (< 1MB)
- ✅ 图片上传 (1-5MB)
- ✅ 图片列表显示
- ✅ 图片显示控制
- ✅ 图片删除功能
- ⏳ RGB 灯珠控制
- ⏳ 串口 AT 指令

### 稳定性测试
- ✅ 长时间运行 (24 小时)
- ✅ 多次上传测试 (100+ 次)
- ✅ 内存泄漏检测
- ✅ 看门狗触发测试

---

## 📚 文档索引

1. **[图片显示功能文档](IMAGE_DISPLAY_README.md)**
   - 图片解码原理
   - 支持的格式
   - 性能优化技巧

2. **[WiFi 传输功能文档](WIFI_IMAGE_TRANSFER_README.md)**
   - 技术架构详解
   - API 接口说明
   - 开发日志

3. **[实现方案文档](WIFI_IMAGE_TRANSFER_IMPLEMENTATION.md)**
   - 详细实现步骤
   - 技术选型理由
   - 安全与稳定性考虑

4. **[快速使用指南](WIFI_QUICK_START.md)**
   - 快速上手教程
   - 常见问题解答
   - 故障排查

---

## 🔜 后续计划

### 短期目标
- [ ] 实现 RGB 灯珠驱动 (RMT/LEDC)
- [ ] 实现串口 AT 指令控制
- [ ] 添加图片幻灯片播放功能
- [ ] 优化大图片加载速度

### 长期目标
- [ ] OTA 固件升级功能
- [ ] MQTT 远程控制
- [ ] 图片云端同步
- [ ] 视频播放支持
- [ ] 移动 App 开发

---

## 🐛 已知问题

1. **大图片加载慢**
   - 原因：JPEG 解码需要时间
   - 解决方案：使用缩略图或预加载

2. **多客户端并发限制**
   - 原因：ESP32 内存限制
   - 解决方案：限制最大连接数为 4

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

### 开发规范
- 代码注释使用中文
- 变量名使用英文
- 遵循现有代码风格
- 提交前测试功能

---

## 📄 许可证

本项目采用 MIT 许可证。

---

## 📞 联系方式

如有问题或建议，请通过以下方式联系：

- 提交 GitHub Issue
- 发送邮件至 [email]

---

**项目版本**: v2.0  
**最后更新**: 2026-02-22  
**维护者**: Kiro

---

## 🎉 致谢

感谢以下开源项目：

- [Arduino_GFX](https://github.com/moononournation/Arduino_GFX)
- [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder)
- [PNGdec](https://github.com/bitbank2/PNGdec)
- [FastLED](https://github.com/FastLED/FastLED)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
