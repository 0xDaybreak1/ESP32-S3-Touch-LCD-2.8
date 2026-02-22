#include "Image_Decoder.h"
#include "Display_ST7789.h"

// 全局缓冲区
uint16_t* imageBuffer = nullptr;

// 全局变量用于回调函数
static uint16_t* g_imageBuffer = nullptr;
static uint16_t g_bufferWidth = 0;
static uint16_t g_bufferHeight = 0;

// 初始化图片解码器
void initImageDecoder() {
    // 分配图片缓冲区
    if (imageBuffer == nullptr) {
        imageBuffer = (uint16_t*)malloc(IMG_BUFFER_SIZE);
        if (imageBuffer == nullptr) {
            Serial.println("错误: 无法分配图片缓冲区内存");
            return;
        }
    }
    
    g_imageBuffer = imageBuffer;
    g_bufferWidth = LCD_WIDTH;
    g_bufferHeight = LCD_HEIGHT;
    
    Serial.println("图片解码器初始化完成");
}

// 获取图片格式
ImageFormat getImageFormat(const char* filename) {
    if (filename == nullptr) return IMG_UNKNOWN;
    
    const char* ext = strrchr(filename, '.');
    if (ext == nullptr) return IMG_UNKNOWN;
    
    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
        return IMG_JPEG;
    } else if (strcasecmp(ext, ".png") == 0) {
        return IMG_PNG;
    } else if (strcasecmp(ext, ".bmp") == 0) {
        return IMG_BMP;
    }
    
    return IMG_UNKNOWN;
}

// JPEG 解码回调函数 - 使用 TJpgDec 的标准回调格式
bool jpegDrawCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // 设置显示窗口并写入数据
    LCD_SetCursor(x, y, x + w - 1, y + h - 1);
    
    // 逐行写入数据
    for (uint16_t row = 0; row < h; row++) {
        LCD_WriteData_nbyte((uint8_t*)&bitmap[row * w], NULL, w * 2);
    }
    
    return true;
}

// PNG 文件打开回调
void* pngFileOpen(const char* szFilename, int32_t* pFileSize) {
    File* pFile = new File(SD_MMC.open(szFilename, FILE_READ));
    if (pFile && *pFile) {
        *pFileSize = pFile->size();
        return (void*)pFile;
    }
    delete pFile;
    return nullptr;
}

// PNG 文件关闭回调
void pngFileClose(void* pHandle) {
    File* pFile = (File*)pHandle;
    if (pFile) {
        pFile->close();
        delete pFile;
    }
}

// PNG 文件读取回调
int32_t pngFileRead(PNGFILE* pFile, uint8_t* pBuf, int32_t iLen) {
    File* f = (File*)pFile->fHandle;
    if (f) {
        return f->read(pBuf, iLen);
    }
    return 0;
}

// PNG 文件查找回调
int32_t pngFileSeek(PNGFILE* pFile, int32_t iPos) {
    File* f = (File*)pFile->fHandle;
    if (f) {
        return f->seek(iPos) ? 1 : 0;
    }
    return 0;
}

// PNG 解码回调函数
int pngDrawCallback(PNGDRAW* pDraw) {
    uint16_t* pPixels = (uint16_t*)pDraw->pPixels;
    uint16_t y = pDraw->y;
    uint16_t w = pDraw->iWidth;
    
    // 设置显示窗口并写入数据
    LCD_SetCursor(0, y, w - 1, y);
    LCD_WriteData_nbyte((uint8_t*)pPixels, NULL, w * 2);
    
    return 0;
}

// 显示 JPEG 图片
bool displayJPEG(const char* filename) {
    Serial.printf("正在加载 JPEG 图片: %s\n", filename);
    
    // 检查文件是否存在
    if (!SD_MMC.exists(filename)) {
        Serial.printf("错误: 文件不存在 - %s\n", filename);
        return false;
    }
    
    // 使用 TJpgDec 库解码并显示
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(jpegDrawCallback);
    TJpgDec.drawJpg(0, 0, filename);
    
    Serial.println("JPEG 图片显示完成");
    return true;
}

// 显示 PNG 图片
bool displayPNG(const char* filename) {
    Serial.printf("正在加载 PNG 图片: %s\n", filename);
    
    // 检查文件是否存在
    if (!SD_MMC.exists(filename)) {
        Serial.printf("错误: 文件不存在 - %s\n", filename);
        return false;
    }
    
    PNG png;
    int rc = png.open((const char*)filename, pngFileOpen, pngFileClose, pngFileRead, pngFileSeek, pngDrawCallback);
    
    if (rc == PNG_SUCCESS) {
        Serial.printf("PNG 信息 - 宽: %d, 高: %d\n", png.getWidth(), png.getHeight());
        
        // 解码并显示
        rc = png.decode(NULL, 0);
        
        png.close();
        
        if (rc == PNG_SUCCESS) {
            Serial.println("PNG 图片显示完成");
            return true;
        }
    }
    
    Serial.printf("PNG 解码失败，错误码: %d\n", rc);
    return false;
}

// 显示 BMP 图片 - 使用 Arduino_GFX 内置的 BMP 解析
bool displayBMP(const char* filename) {
    Serial.printf("正在加载 BMP 图片: %s\n", filename);
    
    // 检查文件是否存在
    if (!SD_MMC.exists(filename)) {
        Serial.printf("错误: 文件不存在 - %s\n", filename);
        return false;
    }
    
    File bmpFile = SD_MMC.open(filename, FILE_READ);
    if (!bmpFile) {
        Serial.printf("错误: 无法打开文件 - %s\n", filename);
        return false;
    }
    
    // 读取 BMP 文件头 (54 字节)
    uint8_t header[54];
    if (bmpFile.read(header, 54) != 54) {
        Serial.println("错误: 无法读取 BMP 文件头");
        bmpFile.close();
        return false;
    }
    
    // 验证 BMP 签名
    if (header[0] != 'B' || header[1] != 'M') {
        Serial.println("错误: 不是有效的 BMP 文件");
        bmpFile.close();
        return false;
    }
    
    // 解析 BMP 信息
    uint32_t pixelDataOffset = *(uint32_t*)&header[10];
    uint32_t width = *(uint32_t*)&header[18];
    uint32_t height = *(uint32_t*)&header[22];
    uint16_t bitsPerPixel = *(uint16_t*)&header[28];
    
    Serial.printf("BMP 信息 - 宽: %d, 高: %d, 位深: %d\n", width, height, bitsPerPixel);
    
    // 检查是否为 24 位或 32 位 BMP
    if (bitsPerPixel != 24 && bitsPerPixel != 32) {
        Serial.println("错误: 仅支持 24 位或 32 位 BMP 图片");
        bmpFile.close();
        return false;
    }
    
    // 检查分辨率是否超过屏幕
    if (width > LCD_WIDTH || height > LCD_HEIGHT) {
        Serial.printf("警告: 图片分辨率 (%d×%d) 超过屏幕 (%d×%d)\n", 
                     width, height, LCD_WIDTH, LCD_HEIGHT);
    }
    
    // 分配行缓冲区
    uint32_t bytesPerPixel = bitsPerPixel / 8;
    uint32_t rowSize = width * bytesPerPixel;
    uint8_t* rowBuffer = (uint8_t*)malloc(rowSize);
    
    if (rowBuffer == nullptr) {
        Serial.println("错误: 无法分配行缓冲区");
        bmpFile.close();
        return false;
    }
    
    // 分配输出缓冲区 (RGB565)
    uint16_t* outputBuffer = (uint16_t*)malloc(width * 2);
    if (outputBuffer == nullptr) {
        Serial.println("错误: 无法分配输出缓冲区");
        free(rowBuffer);
        bmpFile.close();
        return false;
    }
    
    // 逐行读取并显示 BMP 数据
    // BMP 文件中像素数据从下到上存储，所以需要从下往上读取
    bmpFile.seek(pixelDataOffset);
    
    for (int32_t y = height - 1; y >= 0; y--) {
        // 读取一行像素数据
        if (bmpFile.read(rowBuffer, rowSize) != rowSize) {
            Serial.printf("错误: 读取第 %d 行失败\n", y);
            break;
        }
        
        // 转换 BGR 到 RGB565 (BMP 使用 BGR 格式)
        for (uint32_t x = 0; x < width; x++) {
            uint8_t b = rowBuffer[x * bytesPerPixel + 0];
            uint8_t g = rowBuffer[x * bytesPerPixel + 1];
            uint8_t r = rowBuffer[x * bytesPerPixel + 2];
            
            // 转换为 RGB565 格式
            // RGB565: RRRRRGGGGGGBBBBB
            outputBuffer[x] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
        }
        
        // 显示这一行
        LCD_SetCursor(0, y, width - 1, y);
        LCD_WriteData_nbyte((uint8_t*)outputBuffer, NULL, width * 2);
    }
    
    free(rowBuffer);
    free(outputBuffer);
    bmpFile.close();
    
    Serial.println("BMP 图片显示完成");
    return true;
}

// 加载并显示图片
bool loadAndDisplayImage(const char* filename) {
    if (filename == nullptr) {
        Serial.println("错误: 文件名为空");
        return false;
    }
    
    ImageFormat format = getImageFormat(filename);
    
    switch (format) {
        case IMG_JPEG:
            return displayJPEG(filename);
        case IMG_PNG:
            return displayPNG(filename);
        case IMG_BMP:
            return displayBMP(filename);
        default:
            Serial.printf("错误: 不支持的图片格式 - %s\n", filename);
            return false;
    }
}
