#include "LED_Driver.h"

// 全局状态变量定义
CRGB leds[NUM_LEDS];
LEDMode currentMode = LED_OFF;
CRGB targetColor = CRGB::Red;
uint8_t targetBrightness = 128;  // 默认 50% 亮度

// FreeRTOS 任务句柄
TaskHandle_t ledTaskHandle = NULL;

// 初始化 LED
void LED_Init() {
    Serial.println("\n========== LED 初始化 ==========");
    
    // 初始化 FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(targetBrightness);
    FastLED.clear();
    FastLED.show();
    
    Serial.printf("✓ FastLED 初始化成功\n");
    Serial.printf("  LED 引脚: GPIO %d\n", LED_PIN);
    Serial.printf("  LED 数量: %d\n", NUM_LEDS);
    
    // 创建 LED 控制任务 (固定在 Core 1，与图像解码错开)
    xTaskCreatePinnedToCore(
        LED_Task,           // 任务函数
        "LED_Task",         // 任务名称
        4096,               // 栈大小
        NULL,               // 参数
        1,                  // 优先级 (低于图像解码)
        &ledTaskHandle,     // 任务句柄
        1                   // 固定在 Core 1
    );
    
    Serial.println("✓ LED 控制任务已启动 (Core 1)");
    Serial.println("==================================\n");
}

// 设置模式
void LED_SetMode(LEDMode mode) {
    currentMode = mode;
    Serial.printf("LED 模式切换: %d\n", mode);
}

// 设置颜色
void LED_SetColor(CRGB color) {
    targetColor = color;
    Serial.printf("LED 颜色设置: R=%d, G=%d, B=%d\n", color.r, color.g, color.b);
}

// 设置亮度
void LED_SetBrightness(uint8_t brightness) {
    targetBrightness = brightness;
    FastLED.setBrightness(brightness);
    Serial.printf("LED 亮度设置: %d\n", brightness);
}

// 16 进制颜色字符串转 CRGB
CRGB hexToRGB(const String& hexColor) {
    // 移除 # 符号
    String hex = hexColor;
    if (hex.startsWith("#")) {
        hex = hex.substring(1);
    }
    
    // 转换为整数
    long number = strtol(hex.c_str(), NULL, 16);
    
    // 提取 RGB 分量
    uint8_t r = (number >> 16) & 0xFF;
    uint8_t g = (number >> 8) & 0xFF;
    uint8_t b = number & 0xFF;
    
    return CRGB(r, g, b);
}

// LED 控制任务 (FreeRTOS)
void LED_Task(void *parameter) {
    // 流水灯状态变量
    static uint8_t flowOffset = 0;
    
    // 呼吸灯状态变量
    static unsigned long breatheStartTime = 0;
    static const uint16_t breathePeriod = 3000;  // 呼吸周期 3 秒
    
    Serial.println("LED_Task 已启动");
    
    while (true) {
        switch (currentMode) {
            case LED_OFF:
                // 关闭所有 LED
                FastLED.clear();
                FastLED.show();
                vTaskDelay(pdMS_TO_TICKS(100));  // 避免饿死看门狗
                break;
                
            case LED_SOLID:
                // 常亮模式：填充目标颜色
                fill_solid(leds, NUM_LEDS, targetColor);
                FastLED.show();
                vTaskDelay(pdMS_TO_TICKS(100));  // 避免饿死看门狗
                break;
                
            case LED_FLOW:
                // 流水灯模式：彩虹流水效果
                fill_rainbow(leds, NUM_LEDS, flowOffset, 256 / NUM_LEDS);
                FastLED.show();
                
                // 更新偏移量
                flowOffset += 2;  // 流动速度
                
                vTaskDelay(pdMS_TO_TICKS(30));  // 30ms 刷新一次，流畅动画
                break;
                
            case LED_BREATHE:
                // 呼吸灯模式：基于时间的正弦波渐变
                {
                    unsigned long currentTime = millis();
                    if (breatheStartTime == 0) {
                        breatheStartTime = currentTime;
                    }
                    
                    // 计算呼吸周期内的位置 (0-255)
                    uint8_t phase = beatsin8(60000 / breathePeriod, 0, 255);
                    
                    // 应用亮度到目标颜色
                    CRGB breatheColor = targetColor;
                    breatheColor.nscale8(phase);
                    
                    // 填充所有 LED
                    fill_solid(leds, NUM_LEDS, breatheColor);
                    FastLED.show();
                    
                    vTaskDelay(pdMS_TO_TICKS(20));  // 20ms 刷新一次，流畅呼吸
                }
                break;
                
            default:
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}
