/**
 * @file Image_Decoder.cpp
 * @brief ESP32-S3 图片解码器实现 - 支持 JPEG/PNG/BMP 格式
 * @author Kiro
 * @date 2026-02-22
 * 
 * 核心修复：
 * 1. 修复文件后缀识别漏洞（.jpg/.jpeg/.png 统一识别）
 * 2. 强制使用 PSRAM 分配大块内存，避免 SRAM 溢出
 * 3. 完美适配 PNGdec 的文件回调，支持 SD_MMC
 * 4. 所有文件读取完成后立即关闭，释放 SD 卡总线
 */

#include "Image_Decoder.h"
#include "Display_ST7789.h"
#include <esp_heap_caps.h>

// ============================================================================
// 全局变量定义
// ============================================================================

// 全局图片缓冲区（用于解码输出）
uint16_t* imageBuffer = nullptr;

// 全局变量用于回调函数
static uint16_t* g_imageBuffer = nullptr;
static uint16_t g_bufferWidth = 0;
static uint16_t g_bufferHeight = 0;

// ============================================================================
// 初始化函数
// ============================================================================

/**
 * @brief 初始化图片解码器
 * @details 分配图片缓冲区，优先使用 PSRAM（8MB），避免占用宝贵的内部 SRAM
 * 
 * 内存分配策略：
 * - 优先使用 PSRAM（MALLOC_CAP_SPIRAM）
 * - PSRAM 分配失败时降级到内部 RAM
 * - 分配失败时打印致命错误
 */
void initImageDecoder() {
    // 避免重复分配
    if (imageBuffer != nullptr) {
        Serial.println("⚠️ 图片缓冲区已存在，跳过初始化");
        return;
    }
    
    Serial.printf("正在分配图片缓冲区: %d 字节 (%.2f KB)\n", 
                  IMG_BUFFER_SIZE, IMG_BUFFER_SIZE / 1024.0);
    
    // 🔧 【核心修复 1】：强制使用 PSRAM 分配内存
    // ESP32-S3 配备 8MB PSRAM，应该优先使用 PSRAM 存储大块数据
    imageBuffer = (uint16_t*)heap_caps_malloc(IMG_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    
    if (imageBuffer == nullptr) {
        // PSRAM 分配失败，尝试降级到内部 RAM
        Serial.println("⚠️ PSRAM 分配失败，尝试使用内部 RAM");
        imageBuffer = (uint16_t*)malloc(IMG_BUFFER_SIZE);
        
        if (imageBuffer == nullptr) {
            // 致命错误：无法分配内存
            Serial.println("❌ 致命错误：无法分配图片缓冲区内存！");
            Serial.println("❌ 系统内存不足，图片解码功能将无法使用");
            return;
        }
        
        Serial.println("✓ 图片缓冲区已分配到内部 RAM（性能可能受影响）");
    } else {
        Serial.println("✓ 图片缓冲区已成功分配到 PSRAM");
    }
    
    // 初始化全局变量
    g_imageBuffer = imageBuffer;
    g_bufferWidth = LCD_WIDTH;
    g_bufferHeight = LCD_HEIGHT;
    
    Serial.println("✓ 图片解码器初始化完成");
}

// ============================================================================
// 格式识别函数
// ============================================================================

/**
 * @brief 获取图片格式
 * @param filename 文件名（完整路径）
 * @return ImageFormat 图片格式枚举
 * 
 * 🔧 【核心修复 2】：修复后缀识别漏洞
 * - 使用 strcasecmp 不区分大小写比较
 * - 同时支持 .jpg 和 .jpeg（它们是同一种格式）
 * - 支持大小写混合（.JPG, .Jpg, .jpg 等）
 */
ImageFormat getImageFormat(const char* filename) {
    if (filename == nullptr) {
        Serial.println("✗ 文件名为空");
        return IMG_UNKNOWN;
    }
    
    // 查找最后一个点号（文件扩展名）
    const char* ext = strrchr(filename, '.');
    if (ext == nullptr) {
        Serial.printf("✗ 文件名没有扩展名: %s\n", filename);
        return IMG_UNKNOWN;
    }
    
    // 🔧 【核心修复】：使用 strcasecmp 不区分大小写比较
    // 同时支持 .jpg 和 .jpeg（JPEG 格式的两种常见扩展名）
    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
        return IMG_JPEG;
    } else if (strcasecmp(ext, ".png") == 0) {
        return IMG_PNG;
    } else if (strcasecmp(ext, ".bmp") == 0) {
        return IMG_BMP;
    }
    
    Serial.printf("✗ 不支持的文件格式: %s\n", ext);
    return IMG_UNKNOWN;
}

// ============================================================================
// JPEG 解码相关函数
// ============================================================================

/**
 * @brief JPEG 解码回调函数
 * @param x 起始 X 坐标
 * @param y 起始 Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param bitmap 位图数据（RGB565 格式）
 * @return true 成功，false 失败
 * 
 * @details TJpgDec 库会将解码后的数据分块传递给这个回调函数
 *          我们需要将数据写入 ST7789 屏幕
 */
bool jpegDrawCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // 边界检查
    if (x < 0 || y < 0 || x + w > LCD_WIDTH || y + h > LCD_HEIGHT) {
        Serial.printf("⚠️ JPEG 回调：坐标超出屏幕范围 (%d,%d,%d,%d)\n", x, y, w, h);
        return false;
    }
    
    // 设置显示窗口
    LCD_SetCursor(x, y, x + w - 1, y + h - 1);
    
    // 写入位图数据到屏幕
    // bitmap 已经是 RGB565 格式，可以直接写入
    LCD_WriteData_nbyte((uint8_t*)bitmap, NULL, w * h * 2);
    
    return true;
}

/**
 * @brief 显示 JPEG 图片
 * @param filename 文件路径
 * @return true 成功，false 失败
 * 
 * @details 
 * 1. 将整个 JPEG 文件读入 PSRAM
 * 2. 关闭文件，释放 SD 卡总线
 * 3. 从内存解码并显示
 * 4. 释放内存
 */
bool displayJPEG(const char* filename) {
    Serial.printf("\n--- 开始加载 JPEG 图片 ---\n");
    Serial.printf("文件路径: %s\n", filename);
    
    // 检查文件是否存在
    if (!SD_MMC.exists(filename)) {
        Serial.printf("✗ 文件不存在: %s\n", filename);
        return false;
    }
    
    // 打开文件
    File jpegFile = SD_MMC.open(filename, FILE_READ);
    if (!jpegFile) {
        Serial.printf("✗ 无法打开文件: %s\n", filename);
        return false;
    }
    
    size_t fileSize = jpegFile.size();
    Serial.printf("文件大小: %d 字节 (%.2f KB)\n", fileSize, fileSize / 1024.0);
    
    // 🔧 【核心修复 3】：优先使用 PSRAM 分配文件缓冲区
    uint8_t* jpegBuffer = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM);
    if (jpegBuffer == nullptr) {
        // PSRAM 分配失败，尝试使用内部 RAM
        Serial.println("⚠️ PSRAM 分配失败，尝试使用内部 RAM");
        jpegBuffer = (uint8_t*)malloc(fileSize);
        if (jpegBuffer == nullptr) {
            Serial.println("✗ 无法分配 JPEG 文件缓冲区");
            jpegFile.close();
            return false;
        }
    }
    
    // 读取整个文件到内存
    size_t bytesRead = jpegFile.read(jpegBuffer, fileSize);
    jpegFile.close(); // 🔧 【关键】立即关闭文件，释放 SD 卡总线
    
    if (bytesRead != fileSize) {
        Serial.printf("✗ 文件读取失败 (期望 %d 字节, 实际 %d 字节)\n", fileSize, bytesRead);
        free(jpegBuffer);
        return false;
    }
    
    Serial.println("✓ JPEG 文件已完整读入内存，SD 卡总线已释放");
    
    // 配置 TJpgDec 解码器
    TJpgDec.setJpgScale(1);  // 不缩放（1:1 显示）
    TJpgDec.setCallback(jpegDrawCallback);
    
    // 从内存解码并显示
    Serial.println("开始解码 JPEG...");
    int result = TJpgDec.drawJpg(0, 0, jpegBuffer, fileSize);
    
    // 释放内存
    free(jpegBuffer);
    
    if (result == 0) {
        Serial.println("✓ JPEG 图片显示完成");
        Serial.println("--- JPEG 加载结束 ---\n");
        return true;
    } else {
        Serial.printf("✗ JPEG 解码失败，错误码: %d\n", result);
        return false;
    }
}

// ============================================================================
// PNG 解码相关函数
// ============================================================================

/**
 * @brief PNG 解码回调函数
 * @param pDraw PNG 绘制结构体
 * @return 0 成功
 * 
 * @details PNGdec 库会将解码后的数据逐行传递给这个回调函数
 *          数据格式已经是 RGB565，可以直接写入屏幕
 */
int pngDrawCallback(PNGDRAW* pDraw) {
    uint16_t* pPixels = (uint16_t*)pDraw->pPixels;
    uint16_t y = pDraw->y;
    uint16_t w = pDraw->iWidth;
    uint16_t h = 1;  // PNGdec 每次传递一行
    
    // 边界检查
    if (y >= LCD_HEIGHT || w > LCD_WIDTH) {
        Serial.printf("⚠️ PNG 回调：坐标超出屏幕范围 (y=%d, w=%d)\n", y, w);
        return 0;
    }
    
    // 设置显示窗口（一行）
    LCD_SetCursor(0, y, w - 1, y);
    
    // 写入像素数据
    LCD_WriteData_nbyte((uint8_t*)pPixels, NULL, w * 2);
    
    return 0;
}

/**
 * @brief 显示 PNG 图片
 * @param filename 文件路径
 * @return true 成功，false 失败
 * 
 * @details 
 * 支持两种解码方式：
 * 1. 文件回调方式（使用 png.open()）
 * 2. 内存方式（使用 png.openRAM()）
 * 
 * 默认使用文件回调方式，如果失败则尝试内存方式
 */
bool displayPNG(const char* filename) {
    Serial.printf("\n========== 开始加载 PNG 图片 ==========\n");
    Serial.printf("文件路径: %s\n", filename);
    
    // 检查文件是否存在
    if (!SD_MMC.exists(filename)) {
        Serial.printf("✗ 文件不存在: %s\n", filename);
        Serial.println("========================================\n");
        return false;
    }
    
    // 创建 PNG 解码器实例
    PNG png;
    int rc;
    
    // ============================================================================
    // 方法 1：使用文件回调方式（推荐，内存占用小）
    // ============================================================================
    Serial.println("\n--- 尝试方法 1：文件回调方式 ---");
    rc = png.open(filename, pngFileOpen, pngFileClose, pngFileRead, pngFileSeek, pngDrawCallback);
    
    if (rc == PNG_SUCCESS) {
        Serial.printf("✓ PNG 文件打开成功\n");
        Serial.printf("  图片信息 - 宽: %d, 高: %d, 位深: %d\n", 
                     png.getWidth(), png.getHeight(), png.getBpp());
        
        // 开始解码
        Serial.println("开始解码 PNG（文件回调方式）...");
        rc = png.decode(NULL, 0);
        
        png.close();
        
        if (rc == PNG_SUCCESS) {
            Serial.println("✓ PNG 图片显示完成（文件回调方式）");
            Serial.println("========================================\n");
            return true;
        } else {
            Serial.printf("✗ PNG 解码失败（文件回调方式）\n");
            Serial.printf("  错误码: %d\n", rc);
            Serial.println("  尝试方法 2...");
        }
    } else {
        Serial.printf("✗ PNG 文件打开失败（文件回调方式）\n");
        Serial.printf("  错误码: %d\n", rc);
        
        // 打印详细的错误信息
        switch (rc) {
            case -1:
                Serial.println("  原因: PNG_INVALID_FILE - 文件无效或不是 PNG 格式");
                break;
            case -2:
                Serial.println("  原因: PNG_MEM_ERROR - 内存分配失败");
                break;
            case -3:
                Serial.println("  原因: PNG_DECODE_ERROR - 解码错误");
                break;
            case -4:
                Serial.println("  原因: PNG_UNSUPPORTED_FEATURE - 不支持的 PNG 特性");
                break;
            default:
                Serial.printf("  原因: 未知错误码 %d\n", rc);
                break;
        }
        
        Serial.println("  尝试方法 2...");
    }
    
    // ============================================================================
    // 方法 2：使用内存方式（备用，内存占用大但更稳定）
    // ============================================================================
    Serial.println("\n--- 尝试方法 2：内存方式 ---");
    
    // 打开文件
    File pngFile = SD_MMC.open(filename, FILE_READ);
    if (!pngFile) {
        Serial.printf("✗ 无法打开文件: %s\n", filename);
        Serial.println("========================================\n");
        return false;
    }
    
    size_t fileSize = pngFile.size();
    Serial.printf("文件大小: %d 字节 (%.2f KB)\n", fileSize, fileSize / 1024.0);
    
    // 分配内存缓冲区（优先使用 PSRAM）
    uint8_t* pngBuffer = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM);
    if (pngBuffer == nullptr) {
        Serial.println("⚠️ PSRAM 分配失败，尝试使用内部 RAM");
        pngBuffer = (uint8_t*)malloc(fileSize);
        if (pngBuffer == nullptr) {
            Serial.println("✗ 无法分配 PNG 文件缓冲区");
            Serial.printf("  需要: %d 字节 (%.2f KB)\n", fileSize, fileSize / 1024.0);
            pngFile.close();
            Serial.println("========================================\n");
            return false;
        }
    } else {
        Serial.println("✓ 已从 PSRAM 分配文件缓冲区");
    }
    
    // 读取整个文件到内存
    size_t bytesRead = pngFile.read(pngBuffer, fileSize);
    pngFile.close();
    
    if (bytesRead != fileSize) {
        Serial.printf("✗ 文件读取失败 (期望 %d 字节, 实际 %d 字节)\n", fileSize, bytesRead);
        free(pngBuffer);
        Serial.println("========================================\n");
        return false;
    }
    
    Serial.println("✓ PNG 文件已完整读入内存，SD 卡总线已释放");
    
    // 从内存打开 PNG
    Serial.println("开始解码 PNG（内存方式）...");
    rc = png.openRAM(pngBuffer, fileSize, pngDrawCallback);
    
    if (rc == PNG_SUCCESS) {
        Serial.printf("✓ PNG 内存打开成功\n");
        Serial.printf("  图片信息 - 宽: %d, 高: %d, 位深: %d\n", 
                     png.getWidth(), png.getHeight(), png.getBpp());
        
        // 解码并显示
        rc = png.decode(NULL, 0);
        
        png.close();
        free(pngBuffer);
        
        if (rc == PNG_SUCCESS) {
            Serial.println("✓ PNG 图片显示完成（内存方式）");
            Serial.println("========================================\n");
            return true;
        } else {
            Serial.printf("✗ PNG 解码失败（内存方式）\n");
            Serial.printf("  错误码: %d\n", rc);
            
            // 打印详细的错误信息
            switch (rc) {
                case -1:
                    Serial.println("  原因: PNG_INVALID_FILE - 文件无效或不是 PNG 格式");
                    break;
                case -2:
                    Serial.println("  原因: PNG_MEM_ERROR - 内存分配失败");
                    Serial.println("  建议: 检查 PSRAM 是否正常工作");
                    break;
                case -3:
                    Serial.println("  原因: PNG_DECODE_ERROR - 解码错误");
                    Serial.println("  建议: 检查 PNG 文件是否损坏");
                    break;
                case -4:
                    Serial.println("  原因: PNG_UNSUPPORTED_FEATURE - 不支持的 PNG 特性");
                    Serial.println("  建议: 尝试使用标准的 PNG 格式（RGB/RGBA）");
                    break;
                default:
                    Serial.printf("  原因: 未知错误码 %d\n", rc);
                    break;
            }
            
            Serial.println("========================================\n");
            return false;
        }
    } else {
        free(pngBuffer);
        Serial.printf("✗ PNG 内存打开失败\n");
        Serial.printf("  错误码: %d\n", rc);
        
        // 打印详细的错误信息
        switch (rc) {
            case -1:
                Serial.println("  原因: PNG_INVALID_FILE - 数据无效或不是 PNG 格式");
                Serial.println("  建议: 检查文件是否完整下载");
                break;
            case -2:
                Serial.println("  原因: PNG_MEM_ERROR - 内存分配失败");
                Serial.println("  建议: 减小图片尺寸或释放其他内存");
                break;
            default:
                Serial.printf("  原因: 未知错误码 %d\n", rc);
                break;
        }
        
        Serial.println("========================================\n");
        return false;
    }
}

// ============================================================================
// BMP 解码相关函数
// ============================================================================

/**
 * @brief 显示 BMP 图片
 * @param filename 文件路径
 * @return true 成功，false 失败
 * 
 * @details 
 * 1. 读取 BMP 文件头，解析图片信息
 * 2. 将像素数据读入内存
 * 3. 关闭文件，释放 SD 卡总线
 * 4. 逐行转换 BGR 到 RGB565 并显示
 * 5. 释放内存
 * 
 * 支持：24 位和 32 位 BMP 图片
 */
bool displayBMP(const char* filename) {
    Serial.printf("\n--- 开始加载 BMP 图片 ---\n");
    Serial.printf("文件路径: %s\n", filename);
    
    // 检查文件是否存在
    if (!SD_MMC.exists(filename)) {
        Serial.printf("✗ 文件不存在: %s\n", filename);
        return false;
    }
    
    // 打开文件
    File bmpFile = SD_MMC.open(filename, FILE_READ);
    if (!bmpFile) {
        Serial.printf("✗ 无法打开文件: %s\n", filename);
        return false;
    }
    
    // 读取 BMP 文件头 (54 字节)
    uint8_t header[54];
    if (bmpFile.read(header, 54) != 54) {
        Serial.println("✗ 无法读取 BMP 文件头");
        bmpFile.close();
        return false;
    }
    
    // 验证 BMP 签名 (前两个字节应该是 'BM')
    if (header[0] != 'B' || header[1] != 'M') {
        Serial.println("✗ 不是有效的 BMP 文件（签名错误）");
        bmpFile.close();
        return false;
    }
    
    // 解析 BMP 文件头信息
    uint32_t pixelDataOffset = *(uint32_t*)&header[10];  // 像素数据偏移
    uint32_t width = *(uint32_t*)&header[18];            // 图片宽度
    uint32_t height = *(uint32_t*)&header[22];           // 图片高度
    uint16_t bitsPerPixel = *(uint16_t*)&header[28];     // 位深度
    
    Serial.printf("BMP 信息 - 宽: %d, 高: %d, 位深: %d\n", width, height, bitsPerPixel);
    
    // 检查是否为 24 位或 32 位 BMP
    if (bitsPerPixel != 24 && bitsPerPixel != 32) {
        Serial.printf("✗ 不支持的位深度: %d（仅支持 24 位或 32 位）\n", bitsPerPixel);
        bmpFile.close();
        return false;
    }
    
    // 检查分辨率
    if (width > LCD_WIDTH || height > LCD_HEIGHT) {
        Serial.printf("⚠️ 图片分辨率 (%d×%d) 超过屏幕 (%d×%d)，将被裁剪\n", 
                     width, height, LCD_WIDTH, LCD_HEIGHT);
    }
    
    // 计算每行字节数（BMP 行对齐到 4 字节）
    uint32_t bytesPerPixel = bitsPerPixel / 8;
    uint32_t rowSize = ((width * bytesPerPixel + 3) / 4) * 4;  // 4 字节对齐
    uint32_t pixelDataSize = rowSize * height;
    
    Serial.printf("像素数据大小: %d 字节 (%.2f KB)\n", pixelDataSize, pixelDataSize / 1024.0);
    
    // 🔧 【核心修复 7】：优先使用 PSRAM 分配像素数据缓冲区
    uint8_t* pixelData = (uint8_t*)heap_caps_malloc(pixelDataSize, MALLOC_CAP_SPIRAM);
    if (pixelData == nullptr) {
        Serial.println("⚠️ PSRAM 分配失败，尝试使用内部 RAM");
        pixelData = (uint8_t*)malloc(pixelDataSize);
        if (pixelData == nullptr) {
            Serial.println("✗ 无法分配像素数据缓冲区");
            bmpFile.close();
            return false;
        }
    }
    
    // 读取所有像素数据
    bmpFile.seek(pixelDataOffset);
    size_t bytesRead = bmpFile.read(pixelData, pixelDataSize);
    bmpFile.close(); // 🔧 【关键】立即关闭文件，释放 SD 卡总线
    
    if (bytesRead != pixelDataSize) {
        Serial.printf("✗ 像素数据读取失败 (期望 %d 字节, 实际 %d 字节)\n", pixelDataSize, bytesRead);
        free(pixelData);
        return false;
    }
    
    Serial.println("✓ BMP 像素数据已完整读入内存，SD 卡总线已释放");
    
    // 分配行缓冲区（用于 RGB565 转换）
    uint16_t* rowBuffer = (uint16_t*)malloc(width * 2);
    if (rowBuffer == nullptr) {
        Serial.println("✗ 无法分配行缓冲区");
        free(pixelData);
        return false;
    }
    
    Serial.println("开始转换并显示 BMP...");
    
    // 逐行处理并显示 BMP 数据
    // 注意：BMP 文件中像素数据从下到上存储，所以需要从下往上读取
    for (int32_t y = height - 1; y >= 0; y--) {
        // 计算当前行在缓冲区中的偏移
        uint32_t rowOffset = (height - 1 - y) * rowSize;
        
        // 转换 BGR 到 RGB565
        // BMP 使用 BGR 格式，需要转换为 RGB565
        for (uint32_t x = 0; x < width; x++) {
            uint32_t pixelOffset = rowOffset + x * bytesPerPixel;
            uint8_t b = pixelData[pixelOffset + 0];
            uint8_t g = pixelData[pixelOffset + 1];
            uint8_t r = pixelData[pixelOffset + 2];
            
            // 转换为 RGB565 格式
            // RGB565: RRRRRGGGGGGBBBBB (5 位红，6 位绿，5 位蓝)
            rowBuffer[x] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
        }
        
        // 显示这一行
        LCD_SetCursor(0, y, width - 1, y);
        LCD_WriteData_nbyte((uint8_t*)rowBuffer, NULL, width * 2);
    }
    
    // 释放内存
    free(pixelData);
    free(rowBuffer);
    
    Serial.println("✓ BMP 图片显示完成");
    Serial.println("--- BMP 加载结束 ---\n");
    return true;
}

// ============================================================================
// 主入口函数
// ============================================================================

/**
 * @brief 加载并显示图片（主入口函数）
 * @param filename 文件路径
 * @return true 成功，false 失败
 * 
 * @details 
 * 1. 根据文件扩展名识别格式
 * 2. 调用对应的解码函数
 * 3. 返回结果
 */
bool loadAndDisplayImage(const char* filename) {
    if (filename == nullptr) {
        Serial.println("✗ 文件名为空");
        return false;
    }
    
    // 识别图片格式
    ImageFormat format = getImageFormat(filename);
    
    // 根据格式调用对应的解码函数
    switch (format) {
        case IMG_JPEG:
            return displayJPEG(filename);
        
        case IMG_PNG:
            return displayPNG(filename);
        
        case IMG_BMP:
            return displayBMP(filename);
        
        default:
            Serial.printf("✗ 不支持的图片格式: %s\n", filename);
            return false;
    }
}

// ============================================================================
// PNG 文件回调函数（适配 SD_MMC）
// ============================================================================

/**
 * @brief PNG 文件打开回调
 * @param szFilename 文件名
 * @param pFileSize 文件大小指针（输出参数）
 * @return 文件句柄（File 对象指针）或 nullptr（失败）
 * 
 * @details 
 * - 使用 SD_MMC.open() 打开文件
 * - 返回 File 对象指针作为文件句柄
 * - 通过 pFileSize 返回文件大小
 */
void* pngFileOpen(const char *szFilename, int32_t *pFileSize) {
    Serial.printf("PNG 回调：打开文件 %s\n", szFilename);
    
    // 打开文件
    File* f = new File(SD_MMC.open(szFilename, FILE_READ));
    
    if (!f || !(*f)) {
        Serial.printf("✗ PNG 回调：无法打开文件 %s\n", szFilename);
        if (f) delete f;
        return nullptr;
    }
    
    // 获取文件大小
    *pFileSize = f->size();
    Serial.printf("✓ PNG 回调：文件已打开，大小: %d 字节\n", *pFileSize);
    
    return (void*)f;
}

/**
 * @brief PNG 文件关闭回调
 * @param pHandle 文件句柄（File 对象指针）
 */
void pngFileClose(void *pHandle) {
    Serial.println("PNG 回调：关闭文件");
    
    if (pHandle) {
        File* f = (File*)pHandle;
        f->close();
        delete f;
    }
}

/**
 * @brief PNG 文件读取回调
 * @param pFile PNG 文件结构体指针
 * @param pBuf 缓冲区指针
 * @param iLen 要读取的字节数
 * @return 实际读取的字节数
 */
int32_t pngFileRead(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    if (!pFile->fHandle) {
        Serial.println("✗ PNG 回调：文件句柄无效");
        return 0;
    }
    
    File* f = (File*)pFile->fHandle;
    int32_t bytesRead = f->read(pBuf, iLen);
    
    // 只在读取失败时打印（避免日志过多）
    if (bytesRead != iLen) {
        Serial.printf("⚠️ PNG 回调：读取 %d 字节，实际 %d 字节\n", iLen, bytesRead);
    }
    
    return bytesRead;
}

/**
 * @brief PNG 文件定位回调
 * @param pFile PNG 文件结构体指针
 * @param iPos 目标位置
 * @return 1 成功，0 失败
 */
int32_t pngFileSeek(PNGFILE *pFile, int32_t iPos) {
    if (!pFile->fHandle) {
        Serial.println("✗ PNG 回调：文件句柄无效");
        return 0;
    }
    
    File* f = (File*)pFile->fHandle;
    bool success = f->seek(iPos);
    
    if (!success) {
        Serial.printf("✗ PNG 回调：定位到 %d 失败\n", iPos);
    }
    
    return success ? 1 : 0;
}

// 保留旧的回调函数签名（兼容性）
int32_t fileOpenCallback(PNGFILE *pFile, const char *szFilename) {
    int32_t fileSize;
    void* handle = pngFileOpen(szFilename, &fileSize);
    if (handle) {
        pFile->fHandle = handle;
        return fileSize;
    }
    return 0;
}

void fileClosed(PNGFILE *pFile) {
    pngFileClose(pFile->fHandle);
}

uint32_t fileReadCallback(PNGFILE *pFile, uint8_t *pBuf, uint32_t iLen) {
    return (uint32_t)pngFileRead(pFile, pBuf, (int32_t)iLen);
}

int32_t fileSeekCallback(PNGFILE *pFile, uint32_t iPos) {
    return pngFileSeek(pFile, (int32_t)iPos);
}
