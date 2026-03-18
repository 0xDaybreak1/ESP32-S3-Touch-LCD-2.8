# LED_Driver 模块说明

## 模块概述

可配置 WS2812B LED 灯带驱动，支持动态灯珠数量与随机排版的空间映射。

---

## 核心设计：映射表机制

```
视觉索引 (0, 1, 2, 3, ...)   ← 动画颜色计算基于此
       ↓  led_map[]
物理索引 (3, 1, 0, 2, ...)   ← 实际写入 leds[] 数组的位置
```

`led_map[视觉索引] = 物理FastLED索引`

灯珠物理接线乱序时，只需提供正确的映射表，动画效果（如彩虹流水）在视觉上始终保持连续平滑。

---

## 接口说明

| 函数 | 说明 |
|------|------|
| `LED_Init()` | 初始化，创建 FreeRTOS 任务（Core 1，优先级 2） |
| `LED_SetConfig(count, map_array)` | 运行时设置灯珠数量和映射表，`map_array=nullptr` 使用默认顺序 |
| `LED_SetEffect(effect)` | 切换灯效 |
| `LED_SetColor(r, g, b)` | 设置静态纯色 |
| `LED_SetBrightness(brightness)` | 设置全局亮度（0~255） |

## 灯效类型

| 枚举值 | 效果 |
|--------|------|
| `LED_OFF` | 关闭 |
| `LED_FLOW` | 流水彩虹（基于视觉映射，随机排版下视觉连续） |
| `LED_BREATH` | 呼吸灯（蓝紫色） |
| `LED_STATIC` | 静态纯色 |

## 线程安全

配置参数（`active_led_count`、`led_map`、`current_effect`）均受 FreeRTOS Mutex 保护，`LED_SetConfig` 等接口可在任意任务中安全调用。

## 使用示例

```cpp
// 8 颗灯珠，物理接线顺序为 5,3,1,7,6,4,2,0
uint16_t my_map[] = {5, 3, 1, 7, 6, 4, 2, 0};
LED_SetConfig(8, my_map);
LED_SetEffect(LED_FLOW);
```

---

*创建时间：2026-03-18，作者：Kiro*
*内容：新建 LED_Driver 模块（LED_Driver.h / LED_Driver.cpp），支持动态灯珠数量、空间映射表与 FreeRTOS 互斥锁保护*

*更新时间：2026-03-18，作者：Kiro*
*内容：修复两个运行时 bug：1) `active_led_count` 默认为 0 导致任务跳过渲染，现改为默认 16；2) `LED_SetColor` 不自动切换模式导致颜色不生效，现在调用 `LED_SetColor` 时自动切换到 `LED_STATIC`*

*更新时间：2026-03-18，作者：Kiro*
*内容：修正 LED_DATA_PIN 为 GPIO44（19号 UART 接口 RXD）。18号 I2C 接口 GPIO6/7 虽然物理上未接线，但代码中已被 PWR_Key 模块占用，不可用于 LED 输出*
