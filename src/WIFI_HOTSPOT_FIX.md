# WiFi 热点问题修复说明

## 🎯 问题诊断

你的 ESP32-S3 图片轮播正常，说明硬件、供电、SD 卡、SPI 总线都没问题。WiFi 热点搜索不到是**软件层的双核竞态条件**导致的。

## 🔧 已修复的问题

### 问题：双核 WiFi 驱动冲突
- **现象**: 手机搜索不到 `ESP32-ImageDisplay` 热点
- **根本原因**: 
  - `DriverTask` (Core 0) 启动时调用 `Wireless_Test2()`
  - `Wireless_Test2()` 将 WiFi 设为 STA 模式并扫描网络，最后关闭 WiFi
  - 与 `WebServer_Init()` (Core 1) 的 AP 模式产生跨核冲突
  - ESP32 WiFi 驱动无法承受跨核同时初始化

### 修复方案
已在 `main.cpp` 的 `DriverTask()` 中注释掉 `Wireless_Test2()` 调用，确保只有 `WebServer_Init()` 初始化 WiFi。

## 📝 验证步骤

### 1. 重新编译上传
```bash
pio run -t upload
pio device monitor
```

### 2. 检查串口日志（115200 波特率）
应该看到：
```
WiFi AP 已启动
SSID: ESP32-ImageDisplay
密码: 12345678
IP: 192.168.4.1
Web 控制台: http://vision.local 或 http://192.168.4.1
```

### 3. 手机连接
- 打开 WiFi 设置
- 搜索 `ESP32-ImageDisplay`
- 密码: `12345678`
- 浏览器访问: `http://192.168.4.1`

## ⚠️ 如果仍无热点

### 检查硬件
1. **天线问题**: 如果你的 ESP32-S3 是 **WROOM-1U** 或带 **U.FL 天线座**的版本，必须外接天线
2. **供电**: WiFi 发射时瞬时电流可达 500mA，建议使用 2A 以上电源
3. **RF 屏蔽**: 确保模组周围没有金属外壳完全包裹

### 查看完整日志
如果串口日志中有 `WiFi 初始化失败` 或卡住不动，请将完整日志发给我分析。

---
**修复时间**: 2026-02-22  
**修复人**: Kiro


---

## 🐛 路径拼接问题修复

### 问题：Web 上传的图片无法显示
- **修复时间**: 2026-02-22
- **修复人**: Kiro
- **现象**: 图片成功上传到 `/sdcard/uploaded/` 目录，但点击"显示"按钮后屏幕无反应
- **根本原因**: 
  - SD 卡挂载在 `/sdcard`
  - Web 服务器传递的路径是 `/uploaded/123.jpg`（缺少 `/sdcard` 前缀）
  - 图片解码器使用 `SD_MMC.exists()` 和 `SD_MMC.open()` 时找不到文件
  - 导致解码器返回"文件不存在"错误

### 修复方案
在 `WebServer_Driver.cpp` 中修复了两个接口：

1. **`/display` 接口**（第 555 行）
   ```cpp
   // 修复前
   String filepath = String(UPLOAD_DIR) + "/" + filename;
   
   // 修复后
   String filepath = "/sdcard" + String(UPLOAD_DIR) + "/" + filename;
   ```

2. **`/list` 接口**（第 545 行）
   ```cpp
   // 修复前
   if (listImageFiles(UPLOAD_DIR, jsonList)) {
   
   // 修复后
   String fullPath = "/sdcard" + String(UPLOAD_DIR);
   if (listImageFiles(fullPath.c_str(), jsonList)) {
   ```

### 验证方法
重新编译上传后，串口日志应该显示：
```
--- Web 请求显示: /sdcard/uploaded/123.jpg ---
正在加载 JPEG 图片: /sdcard/uploaded/123.jpg
✓ JPEG 文件已完整读入内存
✓ JPEG 图片显示完成
```

而不是：
```
--- Web 请求显示: /uploaded/123.jpg ---
错误: 文件不存在 - /uploaded/123.jpg
```
