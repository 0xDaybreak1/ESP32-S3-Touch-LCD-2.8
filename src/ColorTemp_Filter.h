#pragma once

#include <Arduino.h>

// 色温调节范围
#define COLOR_TEMP_MIN      -100    // 最冷（偏蓝）
#define COLOR_TEMP_MAX      100     // 最暖（偏红）
#define COLOR_TEMP_DEFAULT  0       // 默认（无调整）

// LUT 查找表大小（RGB565 的 R 和 B 分量都是 5-bit，范围 0-31）
#define LUT_SIZE            32

// 全局变量
extern int8_t currentColorTemp;     // 当前色温偏移量 (-100 到 100)
extern bool colorTempChanged;       // 色温变化标志位
extern uint8_t lut_R[LUT_SIZE];     // R 通道查找表
extern uint8_t lut_B[LUT_SIZE];     // B 通道查找表

// 函数声明

/**
 * @brief 初始化色温滤镜模块
 */
void ColorTemp_Init();

/**
 * @brief 设置色温偏移量
 * @param tempOffset 色温偏移量 (-100 到 100)
 */
void ColorTemp_SetOffset(int8_t tempOffset);

/**
 * @brief 更新色温查找表 (LUT)
 * @param tempOffset 色温偏移量 (-100 到 100)
 * 
 * 算法说明：
 * - tempOffset > 0: 增强红色，减弱蓝色（暖色调）
 * - tempOffset < 0: 减弱红色，增强蓝色（冷色调）
 * - tempOffset = 0: 不做调整（原始颜色）
 * - G (绿色) 保持不变，避免画面偏绿失真
 */
void updateColorTempLUT(int8_t tempOffset);

/**
 * @brief 应用色温滤镜到 RGB565 图像缓冲区
 * @param buffer RGB565 像素缓冲区指针（通常在 PSRAM 中）
 * @param len 缓冲区长度（像素数量）
 * 
 * 性能优化：
 * - 使用位运算分离和组合 RGB565
 * - 使用 LUT 查表替代实时计算
 * - 避免浮点运算和乘除法
 * 
 * RGB565 格式：
 * - R: 5-bit (位 15-11)
 * - G: 6-bit (位 10-5)
 * - B: 5-bit (位 4-0)
 */
void applyColorTemperature(uint16_t* buffer, uint32_t len);

/**
 * @brief 获取当前色温偏移量
 * @return 当前色温偏移量 (-100 到 100)
 */
int8_t ColorTemp_GetOffset();
