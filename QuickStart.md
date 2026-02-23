# 🚀 ESP32-S3 无线图传显示系统 - 快速入门指南

> 📖 本指南将帮助你在 15 分钟内完成系统搭建并开始使用。

---

## 📋 目录

- [环境准备](#-环境准备)
- [硬件连接](#-硬件连接)
- [源码配置](#-源码配置)
- [编译与烧录](#-编译与烧录)
- [使用全流程](#-使用全流程)
- [常见问题 FAQ](#-常见问题-faq)

---

## 🛠️ 环境准备

### 1. 安装开发工具

#### 方式一：VSCode + PlatformIO (推荐)

1. **安装 VSCode**
   - 下载地址：https://code.visualstudio.com/
   - 安装完成后启动 VSCode

2. **安装 PlatformIO 插件**
   - 打开 VSCode
   - 点击左侧扩展图标 (Extensions)
   - 搜索 `PlatformIO IDE`
   - 点击 `Install` 安装
   - 等待安装完成（首次安装需要下载依赖，约 5-10 分钟）

3. **验证安装**
   - 重启 VSCode
   - 左侧应出现 PlatformIO 图标（外星人头像）

#### 方式二：Arduino IDE

> ⚠️ 不推荐，因为需要手动安装大量库文件

---

### 2. 克隆项目

```bash
# 使用 Git 克隆
git clone <repository-url>
cd esp32-s3-image-display

# 或直接下载 ZIP 并解压
```

---

### 3. 打开项目

1. 启动 VSCode
2. 点击 `File` → `Open Folder`
3. 选择项目根目录
4. PlatformIO 会自动识别项目并下载依赖库（首次打开需要等待）

##（这里建议将梯子的服务地址输入到电脑的环境变量中，下载速度会快点）
---

### 4. 检查 PSRAM 配置 ⚠️

**这是最重要的一步！**

打开 `platformio.ini` 文件，确认以下配置：

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

; 🔧 关键配置：启用 PSRAM
board_build.arduino.memory_type = qio_opi
board_build.partitions = huge_app.csv
board_build.f_flash = 80000000L
board_build.flash_mode = qio

build_flags = 
    -DBOARD_HAS_PSRAM          ; 必须启用
    -DARDUINO_USB_CDC_ON_BOOT=1
```

> 💡 **为什么重要？**  
> PSRAM 提供额外的 8MB 内存，用于图片解码缓冲区。没有 PSRAM，系统无法正常工作！

---

## 🔌 硬件连接

### 准备材料

- ✅ ESP32-S3-DevKitC-1 (N8R8) × 1
- ✅ ST7789 TFT 显示屏 (240x320) × 1
- ✅ SD 卡模块 × 1
- ✅ Micro SD 卡 (建议 Class 10，8GB-32GB) × 1
- ✅ 杜邦线若干
- ✅ USB-C 数据线 × 1

### 接线图

#### ST7789 TFT 显示屏

| ST7789 引脚 | ESP32-S3 引脚 | 线色建议 |
|-------------|---------------|----------|
| VCC | 3.3V | 红色 |
| GND | GND | 黑色 |
| SCL (SCK) | GPIO 12 | 黄色 |
| SDA (MOSI) | GPIO 11 | 绿色 |
| RES (RST) | GPIO 10 | 蓝色 |
| DC | GPIO 9 | 紫色 |
| CS | GPIO 13 | 橙色 |
| BLK (背光) | GPIO 14 | 白色 (可选) |

#### SD 卡模块 (SD_MMC 4-bit 模式)

| SD 卡引脚 | ESP32-S3 引脚 | 线色建议 |
|-----------|---------------|----------|
| CLK | GPIO 39 | 黄色 |
| CMD | GPIO 38 | 绿色 |
| D0 | GPIO 40 | 蓝色 |
| D1 | GPIO 41 | 紫色 |
| D2 | GPIO 42 | 橙色 |
| D3 | GPIO 43 | 白色 |
| VCC | 3.3V | 红色 |
| GND | GND | 黑色 |

### 接线检查清单

- [ ] 所有 VCC 连接到 3.3V（不是 5V！）
- [ ] 所有 GND 连接到 GND
- [ ] SPI 引脚连接正确（SCK、MOSI、CS、DC、RST）
- [ ] SD_MMC 引脚连接正确（CLK、CMD、D0-D3）
- [ ] 杜邦线连接牢固，无松动

---

## ⚙️ 源码配置

### 方式一：使用 Web 配网 (推荐)

**无需修改代码！** 首次启动后通过 Web 界面配置 WiFi。

跳过此步骤，直接进入 [编译与烧录](#-编译与烧录)。

---

### 方式二：硬编码 WiFi (可选)

如果你想预先配置 WiFi，可以修改以下文件：

#### 1. 打开配置文件

文件路径：`src/WebServer_Driver.h`

#### 2. 修改 WiFi 配置

找到以下代码：

```cpp
// WiFi 配置
#define WIFI_AP_SSID        "ESP32-ImageDisplay"
#define WIFI_AP_PASSWORD    "12345678"
#define MDNS_HOSTNAME       "vision"
```

**说明**：

- `WIFI_AP_SSID`：AP 热点名称（配网模式下使用）
- `WIFI_AP_PASSWORD`：AP 热点密码（至少 8 位）
- `MDNS_HOSTNAME`：mDNS 域名（访问 `http://vision.local`）

> 💡 **提示**：如果使用 Web 配网，这些配置只在首次启动或连接失败时生效。

---

## 🔨 编译与烧录

### 1. 连接开发板

1. 使用 USB-C 数据线连接 ESP32-S3 到电脑
2. 等待驱动安装完成
3. 检查设备管理器中是否出现 COM 口

### 2. 编译项目

**方式一：使用 PlatformIO 界面**

1. 点击底部状态栏的 `✓` 图标（Build）
2. 等待编译完成（首次编译需要 3-5 分钟）
3. 查看输出窗口，确认无错误

**方式二：使用命令行**

```bash
pio run
```

### 3. 上传固件

**方式一：使用 PlatformIO 界面**

1. 点击底部状态栏的 `→` 图标（Upload）
2. 等待上传完成
3. 查看输出窗口，确认 "SUCCESS"

**方式二：使用命令行**

```bash
pio run --target upload
```

### 4. 查看串口日志

**方式一：使用 PlatformIO 界面**

1. 点击底部状态栏的 `🔌` 图标（Serial Monitor）
2. 波特率选择 `115200`
3. 查看启动日志

**方式二：使用命令行**

```bash
pio device monitor
```

### 预期启动日志

```
========== WiFi 初始化 ==========
✓ SD 卡互斥锁创建成功
✓ 创建上传目录: /uploaded
✓ 检测到已保存的 WiFi 配置
  SSID: YourWiFiName
正在连接到 WiFi: YourWiFiName
.....
✓ WiFi 连接成功 (STA 模式)
  IP 地址: 192.168.1.105
✓ mDNS 服务已启动
  访问地址: http://vision.local
✓ Web 服务器已启动
==================================
```

---