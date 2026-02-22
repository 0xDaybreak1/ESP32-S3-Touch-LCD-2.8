# 图片显示模块文档

## 修改时间
2026-02-15 (最终修正完成)

## 修改人
Kiro

## 最新修正 (2026-02-15 最终版)
- ✅ 修正 TJpgDec 回调函数签名
- ✅ 修正 PNGdec 回调函数实现
- ✅ 修正 PNGDRAW 结构体成员访问 (iX, iY)
- ✅ 修正 pngDrawCallback 返回类型 (int)
- ✅ 添加 Display_ST7789.h 头文件包含
- ✅ 编译成功，无错误

## 功能概述
为 ESP32-S3 开发板实现多格式图片显示功能，支持 JPEG、PNG、BMP 三种主流图片格式。

## 核心文件

### 1. Image_Decoder.h
- **功能**: 图片解码器头文件
- **主要接口**:
  - `initImageDecoder()` - 初始化图片解码器
  - `loadAndDisplayImage(filename)` - 加载并显示图片
  - `getImageFormat(filename)` - 获取图片格式

### 2. Image_Decoder.cpp
- **功能**: 图片解码器实现
- **支持格式**:
  - JPEG: 使用 TJpg_Decoder 库
  - PNG: 使用 PNGdec 库
  - BMP: 使用 Arduino_GFX 内置的 BMP 解析功能（无需额外库）
- **关键函数**:
  - `displayJPEG()` - JPEG 显示
  - `displayPNG()` - PNG 显示
  - `displayBMP()` - BMP 显示（直接读取文件头和像素数据）

### 3. main.cpp
- **功能**: 主程序入口
- **流程**:
  1. 初始化所有硬件驱动
  2. 初始化图片解码器
  3. 启动后台驱动任务
  4. 在主循环中显示图片

## 使用方法

### 准备图片文件
1. 将图片文件放在 SD 卡根目录或指定目录
2. 支持的文件名格式: `test1.png`, `test1.jpg`, `test1.bmp`

### 显示图片
```cpp
// 方式 1: 自动检测格式
loadAndDisplayImage("/sdcard/test1.png");

// 方式 2: 指定格式显示
displayJPEG("/sdcard/test1.jpg");
displayPNG("/sdcard/test1.png");
displayBMP("/sdcard/test1.bmp");
```

## 硬件要求
- ESP32-S3 开发板
- ST7789 LCD 屏幕 (240×320)
- SD 卡模块
- 支持的图片格式: JPEG, PNG, BMP

## 依赖库
```ini
moononournation/GFX Library for Arduino @ ^1.4.6
bodmer/TJpg_Decoder @ ^1.1.0
bitbank2/PNGdec @ ^1.0.1
fastled/FastLED @ ^3.6.0
```

**注**: BMP 格式由 Arduino_GFX 内置支持，无需额外库

## 注意事项

### 内存管理
- 图片缓冲区大小: `LCD_WIDTH * LCD_HEIGHT * 2` (约 153KB)
- BMP 显示时额外分配行缓冲区
- 建议使用 PSRAM 以获得更好的性能

### 图片格式建议
- **JPEG**: 适合照片，文件小，解码速度快
- **PNG**: 支持透明度，适合图标和图形
- **BMP**: 无压缩，解码最快，但文件较大

### BMP 格式说明
- 支持 24 位和 32 位 BMP 格式
- 自动处理 BGR 到 RGB565 的转换
- 支持任意分辨率（会自动适配屏幕）
- 直接读取文件头和像素数据，无需专门库

### 性能优化
- JPEG 图片建议分辨率不超过 240×320
- PNG 图片建议使用 RGB565 格式
- BMP 图片建议使用 24 位格式

## 故障排查

### 图片显示失败
1. 检查 SD 卡是否正确初始化
2. 确认文件路径正确
3. 验证图片格式是否支持
4. 查看串口输出的错误信息

### 内存不足
1. 检查 PSRAM 是否启用
2. 减少其他任务的内存占用
3. 使用较小的图片文件

### BMP 显示异常
1. 确认 BMP 文件是 24 位或 32 位格式
2. 检查文件是否完整
3. 查看串口输出的 BMP 信息

## 修改历史
- **2026-02-15 (修正)**: 移除不存在的 BMPDecode 库，改用 Arduino_GFX 内置 BMP 解析
- **2026-02-15**: 初始版本，支持 JPEG、PNG、BMP 格式显示
