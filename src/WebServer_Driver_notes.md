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
- [WIFI_QUICK_START.md](./WIFI_QUICK_START.md) - 快速开始指南

---

**文档版本**: v1.1  
**创建时间**: 2026-02-23  
**创建者**: Kiro
