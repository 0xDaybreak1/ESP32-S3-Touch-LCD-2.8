# 源代码说明文档

## 最近修改记录

### 2026-02-22 - 修复编译错误 (Kiro)

**修改内容:**
1. 在 `Display_ST7789.h` 中添加了 `LCD_WriteData_nbyte` 函数声明
2. 修正了 `Image_Decoder.cpp` 中 PNG 回调函数的 PNGDRAW 结构体成员访问
   - 将 `pDraw->iX` 和 `pDraw->iY` 改为 `pDraw->y`
   - 使用 `pDraw->iWidth` 获取宽度
3. 修正了 `Image_Decoder.h` 中 JPEG 回调函数的签名

**问题原因:**
- `LCD_WriteData_nbyte` 函数在 `.cpp` 中实现但未在 `.h` 中声明,导致链接错误
- PNGDRAW 结构体实际成员名为 `y` 和 `iWidth`,而非 `iX`/`iY`/`x`

**编译结果:**
- ✅ 编译成功
- RAM 使用: 38.3% (125368/327680 字节)
- Flash 使用: 62.8% (1973962/3145728 字节)

## 模块说明

### Image_Decoder (图片解码器)
- 支持 JPEG、PNG、BMP 三种格式
- JPEG: 使用 TJpg_Decoder 库
- PNG: 使用 PNGdec 库
- BMP: 手动解析文件头和像素数据

### Display_ST7789 (显示驱动)
- ST7789 LCD 驱动程序
- 分辨率: 240×320
- 支持 RGB565 颜色格式
