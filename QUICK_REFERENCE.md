# 快速参考卡片

## 🚀 快速开始 (3 步)

### 1️⃣ 编译
```bash
pio run -e esp32-s3-devkitc-1
```

### 2️⃣ 上传
```bash
pio run -e esp32-s3-devkitc-1 -t upload
```

### 3️⃣ 显示图片
```cpp
loadAndDisplayImage("/sdcard/test1.png");
```

---

## 📦 支持的格式

| 格式 | 库 | 说明 |
|------|-----|------|
| JPEG | TJpg_Decoder | 快速、文件小 |
| PNG | PNGdec | 支持透明度 |
| BMP | Arduino_GFX | 无需额外库 |

---

## 💻 常用代码

### 显示图片
```cpp
// 自动检测格式
loadAndDisplayImage("/sdcard/test1.png");

// 指定格式
displayJPEG("/sdcard/test1.jpg");
displayPNG("/sdcard/test1.png");
displayBMP("/sdcard/test1.bmp");
```

### 循环显示
```cpp
void loop() {
    static int index = 0;
    static unsigned long lastTime = 0;
    
    if (millis() - lastTime > 5000) {
        char filename[50];
        sprintf(filename, "/sdcard/image%d.png", index);
        loadAndDisplayImage(filename);
        index = (index + 1) % 3;
        lastTime = millis();
    }
}
```

### 初始化
```cpp
void setup() {
    LCD_Init();
    SD_Init();
    initImageDecoder();
}
```

---

## 🔧 配置参数

### 屏幕配置
```cpp
#define LCD_WIDTH   240
#define LCD_HEIGHT  320
#define LCD_Backlight_PIN 5
```

### 图片路径
```cpp
#define IMAGE_PATH_PNG  "/sdcard/test1.png"
#define IMAGE_PATH_JPEG "/sdcard/test1.jpg"
#define IMAGE_PATH_BMP  "/sdcard/test1.bmp"
```

### 亮度调整
```cpp
Set_Backlight(80);  // 0-100
```

---

## 📊 性能指标

| 格式 | 分辨率 | 显示时间 |
|------|--------|---------|
| JPEG | 240×320 | ~500ms |
| PNG | 240×320 | ~800ms |
| BMP | 240×320 | ~1000ms |

---

## 🐛 故障排查

### 编译失败
```
检查清单:
1. platformio.ini 是否正确
2. 所有源文件是否在 src 文件夹
3. 是否有语法错误
```

### 图片不显示
```
检查清单:
1. SD 卡是否初始化
2. 文件是否存在
3. 文件格式是否正确
4. 查看串口输出
```

### 内存不足
```
解决方案:
1. 启用 PSRAM
2. 使用较小的图片
3. 减少其他任务
```

---

## 📚 文档导航

| 需求 | 文档 |
|------|------|
| 快速开始 | QUICK_START_IMAGE_DISPLAY.md |
| 完整指南 | IMPLEMENTATION_GUIDE.md |
| BMP 说明 | BMP_IMPLEMENTATION_FIX.md |
| 修正说明 | CORRECTION_SUMMARY.md |
| 文档索引 | INDEX.md |

---

## ✅ 检查清单

### 编译前
- [ ] platformio.ini 已更新
- [ ] 所有源文件都在 src 文件夹
- [ ] 没有语法错误

### 上传前
- [ ] 开发板已连接
- [ ] 串口驱动已安装
- [ ] 编译成功

### 运行前
- [ ] SD 卡已插入
- [ ] 图片文件已准备
- [ ] 串口监视器已打开

---

## 🎯 API 速查

### 初始化
```cpp
void initImageDecoder();
```

### 显示函数
```cpp
bool loadAndDisplayImage(const char* filename);
bool displayJPEG(const char* filename);
bool displayPNG(const char* filename);
bool displayBMP(const char* filename);
```

### 工具函数
```cpp
ImageFormat getImageFormat(const char* filename);
```

### 返回值
```cpp
true   // 成功
false  // 失败
```

---

## 🔑 关键信息

### BMP 格式
- 支持: 24 位、32 位无压缩
- 不支持: 8 位及以下、压缩格式
- 转换: BGR → RGB565

### 内存占用
- 图片缓冲区: 153KB
- 行缓冲区: 1.4KB
- 代码大小: ~50KB

### 依赖库
```ini
moononournation/GFX Library for Arduino @ ^1.4.6
bodmer/TJpg_Decoder @ ^1.1.0
bitbank2/PNGdec @ ^1.0.1
fastled/FastLED @ ^3.6.0
```

---

## 💡 提示

### 性能优化
- 使用 JPEG 格式获得最快速度
- 启用 PSRAM 获得更好性能
- 使用 240×320 分辨率

### 图片优化
- JPEG: 质量 85-90
- PNG: RGB565 格式
- BMP: 24 位格式

### 调试技巧
- 查看串口输出了解进度
- 使用 Serial.printf() 输出调试信息
- 检查内存使用情况

---

## 🎓 学习资源

### 官方文档
- [Arduino_GFX](https://github.com/moononournation/Arduino_GFX)
- [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder)
- [PNGdec](https://github.com/bitbank2/PNGdec)

### 相关资源
- [BMP 格式规范](https://en.wikipedia.org/wiki/BMP_file_format)
- [RGB565 颜色格式](https://en.wikipedia.org/wiki/List_of_color_spaces_and_their_uses)

---

## 📞 获取帮助

### 常见问题
1. **编译错误?** → 查看 QUICK_START_IMAGE_DISPLAY.md
2. **显示失败?** → 查看 IMPLEMENTATION_GUIDE.md
3. **BMP 问题?** → 查看 BMP_IMPLEMENTATION_FIX.md

### 获取支持
1. 查看相关文档
2. 查看代码注释
3. 查看串口输出
4. 参考示例代码

---

**最后更新**: 2026-02-15  
**作者**: Kiro  
**版本**: 1.0.0
