#include "ColorTemp_Filter.h"

// 全局变量定义
int8_t currentColorTemp = COLOR_TEMP_DEFAULT;
bool colorTempChanged = false;
uint8_t lut_R[LUT_SIZE];
uint8_t lut_B[LUT_SIZE];

// 初始化色温滤镜模块
void ColorTemp_Init() {
    Serial.println("\n========== 色温滤镜初始化 ==========");
    
    // 初始化 LUT 为线性映射（无调整）
    updateColorTempLUT(COLOR_TEMP_DEFAULT);
    
    Serial.println("✓ 色温滤镜初始化成功");
    Serial.printf("  默认色温: %d\n", COLOR_TEMP_DEFAULT);
    Serial.println("==================================\n");
}

// 设置色温偏移量
void ColorTemp_SetOffset(int8_t tempOffset) {
    // 边界保护
    tempOffset = constrain(tempOffset, COLOR_TEMP_MIN, COLOR_TEMP_MAX);
    
    if (currentColorTemp != tempOffset) {
        currentColorTemp = tempOffset;
        colorTempChanged = true;
        
        // 更新 LUT
        updateColorTempLUT(tempOffset);
        
        Serial.printf("色温设置: %d (%s)\n", 
            tempOffset, 
            tempOffset > 0 ? "暖色调" : (tempOffset < 0 ? "冷色调" : "中性"));
    }
}

// 更新色温查找表 (LUT)
void updateColorTempLUT(int8_t tempOffset) {
    // 计算调整系数
    // tempOffset 范围: -100 到 100
    // 映射到调整因子: 0.5 到 1.5
    float factor = 1.0f + (tempOffset / 100.0f);
    
    // 为了避免浮点运算，使用定点数
    // factor * 256 转换为整数运算
    int16_t factorInt = (int16_t)(factor * 256.0f);
    
    for (uint8_t i = 0; i < LUT_SIZE; i++) {
        if (tempOffset > 0) {
            // 暖色调：增强红色，减弱蓝色
            // R 通道增强
            int16_t newR = (i * factorInt) >> 8;  // 除以 256
            lut_R[i] = constrain(newR, 0, 31);
            
            // B 通道减弱
            int16_t newB = (i * 256) / factorInt;  // 反向调整
            lut_B[i] = constrain(newB, 0, 31);
            
        } else if (tempOffset < 0) {
            // 冷色调：减弱红色，增强蓝色
            // R 通道减弱
            int16_t newR = (i * 256) / (-factorInt + 512);  // 反向调整
            lut_R[i] = constrain(newR, 0, 31);
            
            // B 通道增强
            int16_t newB = (i * (-factorInt + 512)) >> 8;
            lut_B[i] = constrain(newB, 0, 31);
            
        } else {
            // 无调整：线性映射
            lut_R[i] = i;
            lut_B[i] = i;
        }
    }
    
    // 调试输出（可选）
    #ifdef DEBUG_COLOR_TEMP
    Serial.println("LUT 更新:");
    Serial.print("R: ");
    for (uint8_t i = 0; i < LUT_SIZE; i += 4) {
        Serial.printf("%2d ", lut_R[i]);
    }
    Serial.println();
    Serial.print("B: ");
    for (uint8_t i = 0; i < LUT_SIZE; i += 4) {
        Serial.printf("%2d ", lut_B[i]);
    }
    Serial.println();
    #endif
}

// 应用色温滤镜到 RGB565 图像缓冲区
void applyColorTemperature(uint16_t* buffer, uint32_t len) {
    // 如果色温为默认值，跳过处理
    if (currentColorTemp == COLOR_TEMP_DEFAULT) {
        return;
    }
    
    // 性能计时（可选）
    #ifdef DEBUG_COLOR_TEMP
    unsigned long startTime = micros();
    #endif
    
    // 遍历缓冲区，应用 LUT
    for (uint32_t i = 0; i < len; i++) {
        uint16_t pixel = buffer[i];
        
        // 分离 RGB565 分量（使用位运算）
        // R: 5-bit (位 15-11)
        // G: 6-bit (位 10-5)
        // B: 5-bit (位 4-0)
        uint8_t r = (pixel >> 11) & 0x1F;  // 提取 R (5-bit)
        uint8_t g = (pixel >> 5) & 0x3F;   // 提取 G (6-bit)
        uint8_t b = pixel & 0x1F;          // 提取 B (5-bit)
        
        // 通过 LUT 查表替换 R 和 B
        r = lut_R[r];
        b = lut_B[b];
        // G 保持不变，避免画面偏绿
        
        // 重新组合 RGB565（使用位运算）
        buffer[i] = (r << 11) | (g << 5) | b;
    }
    
    // 性能统计（可选）
    #ifdef DEBUG_COLOR_TEMP
    unsigned long elapsed = micros() - startTime;
    Serial.printf("色温处理耗时: %lu us (%lu 像素)\n", elapsed, len);
    #endif
}

// 获取当前色温偏移量
int8_t ColorTemp_GetOffset() {
    return currentColorTemp;
}
