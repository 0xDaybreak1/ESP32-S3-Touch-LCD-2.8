#pragma once

#include <Arduino.h>
#include <FastLED.h>

// WS2812B 硬件配置
#define LED_PIN         43      // WS2812B 数据引脚
#define NUM_LEDS        16      // LED 数量 (4x4 矩阵)
#define LED_TYPE        WS2812B // LED 类型
#define COLOR_ORDER     GRB     // 颜色顺序

// 灯效模式枚举
enum LEDMode {
    LED_OFF,        // 关闭
    LED_SOLID,      // 常亮
    LED_FLOW,       // 流水灯
    LED_BREATHE     // 呼吸灯
};

// 全局状态变量
extern CRGB leds[NUM_LEDS];         // LED 数组
extern LEDMode currentMode;         // 当前模式
extern CRGB targetColor;            // 目标颜色
extern uint8_t targetBrightness;    // 目标亮度 (0-255)

// 函数声明
void LED_Init();
void LED_SetMode(LEDMode mode);
void LED_SetColor(CRGB color);
void LED_SetBrightness(uint8_t brightness);
void LED_Task(void *parameter);

// 辅助函数：将 16 进制颜色字符串转换为 CRGB
CRGB hexToRGB(const String& hexColor);
