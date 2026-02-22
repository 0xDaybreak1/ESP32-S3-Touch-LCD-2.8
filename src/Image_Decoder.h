#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include "Display_ST7789.h"
#include <TJpg_Decoder.h>
#include <PNGdec.h>

// 图片格式枚举
enum ImageFormat {
    IMG_JPEG,
    IMG_PNG,
    IMG_BMP,
    IMG_UNKNOWN
};

// 图片信息结构体
typedef struct {
    uint16_t width;
    uint16_t height;
    ImageFormat format;
    const char* filename;
} ImageInfo;

// 全局缓冲区用于图片解码
#define IMG_BUFFER_SIZE (LCD_WIDTH * LCD_HEIGHT * 2)
extern uint16_t* imageBuffer;

// 函数声明
ImageFormat getImageFormat(const char* filename);
bool loadAndDisplayImage(const char* filename);
bool displayJPEG(const char* filename);
bool displayPNG(const char* filename);
bool displayBMP(const char* filename);
void initImageDecoder();

// JPEG 回调函数
bool jpegDrawCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

// PNG 回调函数
int pngDrawCallback(PNGDRAW* pDraw);

// PNG 文件操作回调（新版本 - 正确的签名）
void* pngFileOpen(const char *szFilename, int32_t *pFileSize);
void pngFileClose(void *pHandle);
int32_t pngFileRead(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen);
int32_t pngFileSeek(PNGFILE *pFile, int32_t iPos);

// PNG 文件操作回调（旧版本 - 兼容性包装器）
int32_t fileOpenCallback(PNGFILE *pFile, const char *szFilename);
uint32_t fileReadCallback(PNGFILE *pFile, uint8_t *pBuf, uint32_t iLen);
int32_t fileSeekCallback(PNGFILE *pFile, uint32_t iPos);
void fileClosed(PNGFILE *pFile);
