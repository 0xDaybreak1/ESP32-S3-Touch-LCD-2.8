#pragma once

#include <Arduino.h>
#include <FastLED.h>

// ============================================================
// 物理硬件常量（编译期固定，FastLED 要求静态数组）
// ============================================================
#define LED_DATA_PIN    44          // 数据引脚：GPIO44（19号 UART 接口的 RXD 引脚）
                                    // 引脚冲突说明：
                                    //   18号 I2C 接口 GPIO6/7 → 已被 PWR_Key 模块占用
                                    //   19号 UART TXD GPIO43  → 与串口调试冲突
                                    //   19号 UART RXD GPIO44  → 空闲，推荐用此引脚
                                    // 物理接线：灯带数据线接 19号接口第3根线（RXD）
#define MAX_LEDS        100         // 物理显存上限，静态数组大小
#define LED_TYPE        WS2812B     // 灯珠型号
#define COLOR_ORDER     GRB         // 颜色通道顺序

// ============================================================
// 灯效类型枚举
// ============================================================
typedef enum {
    LED_OFF    = 0,   // 关闭
    LED_FLOW   = 1,   // 流水彩虹（基于视觉顺序映射）
    LED_BREATH = 2,   // 呼吸灯
    LED_STATIC = 3,   // 静态纯色
} LED_Effect_t;

// ============================================================
// 对外接口
// ============================================================

/**
 * @brief 初始化 LED 驱动，创建 FreeRTOS 任务
 */
void LED_Init(void);

/**
 * @brief 运行时配置灯珠数量与空间映射表
 *
 * 映射表说明：
 *   map_array[视觉索引] = 物理FastLED索引
 *   例如：灯珠物理接线顺序为 3→1→0→2，视觉上希望从左到右顺序点亮，
 *         则 map_array = {3, 1, 0, 2}
 *   若 map_array 传入 nullptr，则默认视觉顺序 == 物理顺序（0,1,2,...）
 *
 * @param count     实际生效的灯珠数量（不超过 MAX_LEDS）
 * @param map_array 映射数组指针，长度须 >= count，可传 nullptr 使用默认顺序
 */
void LED_SetConfig(uint16_t count, const uint16_t* map_array);

/**
 * @brief 设置当前灯效
 * @param effect 灯效枚举值
 */
void LED_SetEffect(LED_Effect_t effect);

/**
 * @brief 设置静态纯色（仅 LED_STATIC 模式下生效）
 * @param r 红
 * @param g 绿
 * @param b 蓝
 */
void LED_SetColor(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 设置全局亮度（0~255）
 */
void LED_SetBrightness(uint8_t brightness);

// ============================================================
// 兼容旧接口（供 WebServer_Driver.cpp 等旧代码使用，无需修改调用方）
// ============================================================

// 枚举别名：旧名 → 新名
#define LED_SOLID   LED_STATIC
#define LED_BREATHE LED_BREATH

// 函数别名：旧名 LED_SetMode → 新名 LED_SetEffect
#define LED_SetMode LED_SetEffect

/**
 * @brief 将 "#RRGGBB" 或 "RRGGBB" 十六进制字符串转换为 CRGB
 * 供旧代码中 hexToRGB(color) 调用
 */
inline CRGB hexToRGB(const String& hex) {
    String h = hex.startsWith("#") ? hex.substring(1) : hex;
    long val = strtol(h.c_str(), nullptr, 16);
    return CRGB((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
}

/**
 * @brief 兼容旧的单参数 LED_SetColor(CRGB) 调用
 * 旧代码：LED_SetColor(rgbColor)  →  自动转发到新接口 LED_SetColor(r, g, b)
 */
inline void LED_SetColor(CRGB c) { LED_SetColor(c.r, c.g, c.b); }
