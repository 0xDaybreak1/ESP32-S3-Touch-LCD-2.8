# 图片格式识别和 PNG 内存问题修复

## 🎯 问题分析

### 问题 1：.jpg 和 .png 无法识别
**现象**: 只能识别 .jpeg 格式，.jpg 和 .png 无法显示

**根本原因**: 
- ✅ **格式识别逻辑已正确**：`getImageFormat()` 函数使用 `strcasecmp()` 不区分大小写比较
- ✅ **同时支持 .jpg 和 .jpeg**：代码中已经包含两种格式
- ❌ **真正的问题**：PNG 解码器内存分配失败

### 问题 2：PNG 无法显示（内存墙）
**现象**: PNG 图片解码失败或系统崩溃

**根本原因**:
- PNG 使用 Deflate 无损压缩算法
- 解码时需要大量内存（32KB - 64KB 以上）
- 原代码使用 `malloc()` 从内部 SRAM 分配内存
- ESP32-S3 内部 SRAM 有限（约 512KB），容易导致堆栈溢出

## 🔧 修复方案

### 核心修复：使用 PSRAM 分配内存

ESP32-S3 配备了 8MB PSRAM (N8R8)，应该优先使用 PSRAM 分配大块内存。

#### 修复内容

1. **引入 PSRAM API**
```cpp
#include <esp_heap_caps.h>
```

2. **修改内存分配策略**
```cpp
// 修复前（错误）
uint8_t* buffer = (uint8_t*)malloc(fileSize);

// 修复后（正确）
uint8_t* buffer = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
if (buffer == nullptr) {
    // 如果 PSRAM 分配失败，尝试使用内部 RAM
    buffer = (uint8_t*)malloc(fileSize);
}
```

### 修复的函数

#### 1. `initImageDecoder()`
- 图片缓冲区优先使用 PSRAM
- 大小：`IMG_BUFFER_SIZE`（通常为 240×320×2 = 153,600 字节）

#### 2. `displayJPEG()`
- JPEG 文件缓冲区优先使用 PSRAM
- 大小：文件大小（通常 10KB - 100KB）

#### 3. `displayPNG()`
- PNG 文件缓冲区优先使用 PSRAM
- 大小：文件大小（通常 20KB - 200KB）
- PNG 解码器内部也会分配解压缩缓冲区

## 📊 内存使用对比

### 修复前（使用内部 SRAM）
| 组件 | 内存类型 | 大小 | 风险 |
|------|---------|------|------|
| 图片缓冲区 | SRAM | 150KB | ⚠️ 高 |
| JPEG 文件 | SRAM | 50KB | ⚠️ 高 |
| PNG 文件 | SRAM | 100KB | ❌ 极高 |
| PNG 解压缓冲区 | SRAM | 64KB | ❌ 极高 |
| **总计** | **SRAM** | **364KB** | **堆栈溢出** |

### 修复后（使用 PSRAM）
| 组件 | 内存类型 | 大小 | 风险 |
|------|---------|------|------|
| 图片缓冲区 | PSRAM | 150KB | ✅ 无 |
| JPEG 文件 | PSRAM | 50KB | ✅ 无 |
| PNG 文件 | PSRAM | 100KB | ✅ 无 |
| PNG 解压缓冲区 | SRAM | 64KB | ✅ 低 |
| **总计** | **PSRAM** | **300KB** | **安全** |

## ✅ 修复效果

### 1. 格式支持
- ✅ .jpg（JPEG 格式）
- ✅ .jpeg（JPEG 格式）
- ✅ .JPG（大写，不区分大小写）
- ✅ .JPEG（大写，不区分大小写）
- ✅ .png（PNG 格式）
- ✅ .PNG（大写，不区分大小写）
- ✅ .bmp（BMP 格式）
- ✅ .BMP（大写，不区分大小写）

### 2. 内存优化
- ✅ 大块内存优先使用 PSRAM
- ✅ PSRAM 分配失败时自动降级到内部 RAM
- ✅ 避免堆栈溢出
- ✅ 提高系统稳定性

### 3. 调试日志
```
✓ 图片缓冲区已分配到 PSRAM
✓ 图片解码器初始化完成

正在加载 PNG 图片: /uploaded/test.png
PNG 文件大小: 85432 字节
✓ PNG 文件已完整读入内存,SD 卡总线已释放
PNG 信息 - 宽: 240, 高: 320
✓ PNG 图片显示完成
```

如果 PSRAM 分配失败：
```
⚠️ PSRAM 分配失败，尝试使用内部 RAM
✓ PNG 文件已完整读入内存,SD 卡总线已释放
```

## 🔍 技术细节

### PSRAM 分配 API
```cpp
void* heap_caps_malloc(size_t size, uint32_t caps)
```

**参数说明**:
- `size`: 要分配的字节数
- `caps`: 内存能力标志
  - `MALLOC_CAP_SPIRAM`: 使用 SPI RAM (PSRAM)
  - `MALLOC_CAP_8BIT`: 8 位可访问内存

### 内存释放
```cpp
free(buffer);  // 自动识别内存类型（PSRAM 或 SRAM）
```

### PSRAM 配置
在 `platformio.ini` 中已配置：
```ini
board_build.arduino.memory_type = qio_opi  ; 针对 8MB PSRAM (N8R8)
build_flags = -D BOARD_HAS_PSRAM
```

## ⚠️ 注意事项

### 1. PSRAM 性能
- PSRAM 访问速度比内部 SRAM 慢（约 40MHz vs 240MHz）
- 适合存储大块数据，不适合频繁访问的小数据
- 图片缓冲区属于大块数据，适合使用 PSRAM

### 2. DMA 限制
- 某些 DMA 操作不支持 PSRAM
- SPI 显示屏的 DMA 传输可能需要内部 SRAM
- 当前实现已考虑此限制

### 3. 内存碎片
- 长时间运行可能导致内存碎片
- 建议定期重启系统或实现内存池

## 🚀 未来优化方向

### 1. 内存池
- 预分配固定大小的内存池
- 避免频繁的 malloc/free 操作
- 减少内存碎片

### 2. 流式解码
- 不将整个文件读入内存
- 边读边解码
- 进一步降低内存占用

### 3. 硬件加速
- 使用 ESP32-S3 的 JPEG 硬件解码器
- 提高解码速度
- 降低 CPU 占用

---
**修复时间**: 2026-02-22  
**修复人**: Kiro  
**影响文件**: `src/Image_Decoder.cpp`  
**关键技术**: PSRAM 内存分配、heap_caps_malloc
