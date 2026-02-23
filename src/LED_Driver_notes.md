# LED_Driver 模块说明

## 📋 模块概述

**文件**: `LED_Driver.h` / `LED_Driver.cpp`  
**功能**: WS2812B RGB 氛围灯控制，基于 FastLED 和 FreeRTOS  
**创建时间**: 2026-02-23  
**创建者**: Kiro  

---

## 🎯 核心功能

1. **多种灯效模式**
   - 常亮模式 (Solid)
   - 流水灯模式 (Flow)
   - 呼吸灯模式 (Breathe)
   - 关闭模式 (Off)

2. **Web 控制接口**
   - 接收 JSON 格式的控制指令
   - 支持颜色、亮度、模式动态调整

3. **非阻塞架构**
   - 基于 FreeRTOS 独立任务
   - 固定在 Core 1，与图像解码错开
   - 避免阻塞主循环和看门狗超时

---

## 🔧 硬件配置

### WS2812B 连接

| 参数 | 值 | 说明 |
|------|-----|------|
| LED 引脚 | GPIO 43 | 数据引脚 |
| LED 数量 | 16 | 4x4 矩阵排布 |
| LED 类型 | WS2812B | 可寻址 RGB LED |
| 颜色顺序 | GRB | FastLED 配置 |

### 接线方式

```
WS2812B          ESP32-S3
---------        ---------
VCC       -----> 5V
GND       -----> GND
DIN       -----> GPIO 43
```

> ⚠️ **注意**：WS2812B 需要 5V 供电，但数据引脚可以接 3.3V GPIO。

---

## 💻 技术实现

### 1. 数据结构

```cpp
// LED 数组
CRGB leds[NUM_LEDS];

// 灯效模式枚举
enum LEDMode {
    LED_OFF,        // 关闭
    LED_SOLID,      // 常亮
    LED_FLOW,       // 流水灯
    LED_BREATHE     // 呼吸灯
};

// 全局状态变量
LEDMode currentMode;         // 当前模式
CRGB targetColor;            // 目标颜色
uint8_t targetBrightness;    // 目标亮度 (0-255)
```

### 2. 初始化流程

```cpp
void LED_Init() {
    // 1. 初始化 FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(targetBrightness);
    FastLED.clear();
    FastLED.show();
    
    // 2. 创建 FreeRTOS 任务
    xTaskCreatePinnedToCore(
        LED_Task,           // 任务函数
        "LED_Task",         // 任务名称
        4096,               // 栈大小
        NULL,               // 参数
        1,                  // 优先级 (低于图像解码)
        &ledTaskHandle,     // 任务句柄
        1                   // 固定在 Core 1
    );
}
```

### 3. 灯效实现

#### 常亮模式 (Solid)

```cpp
case LED_SOLID:
    fill_solid(leds, NUM_LEDS, targetColor);
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(100));
    break;
```

#### 流水灯模式 (Flow)

```cpp
case LED_FLOW:
    // 彩虹流水效果
    fill_rainbow(leds, NUM_LEDS, flowOffset, 256 / NUM_LEDS);
    FastLED.show();
    
    // 更新偏移量
    flowOffset += 2;  // 流动速度
    
    vTaskDelay(pdMS_TO_TICKS(30));  // 30ms 刷新
    break;
```

#### 呼吸灯模式 (Breathe)

```cpp
case LED_BREATHE:
    // 基于时间的正弦波渐变
    uint8_t phase = beatsin8(60000 / breathePeriod, 0, 255);
    
    // 应用亮度到目标颜色
    CRGB breatheColor = targetColor;
    breatheColor.nscale8(phase);
    
    fill_solid(leds, NUM_LEDS, breatheColor);
    FastLED.show();
    
    vTaskDelay(pdMS_TO_TICKS(20));  // 20ms 刷新
    break;
```

### 4. Web 控制接口

#### API 路由

**路径**: `POST /led`

**请求格式**:

```json
{
  "mode": "flow",
  "color": "#ff0000",
  "brightness": "50"
}
```

**参数说明**:

| 参数 | 类型 | 值域 | 说明 |
|------|------|------|------|
| mode | String | solid, flow, breathe, off | 灯效模式 |
| color | String | #RRGGBB | 16 进制颜色码 |
| brightness | Integer | 0-100 | 亮度百分比 |

**响应格式**:

```json
{
  "success": true
}
```

#### 实现代码

```cpp
server.on("/led", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // 解析 JSON
        JsonDocument doc;
        deserializeJson(doc, data, len);
        
        String mode = doc["mode"].as<String>();
        String color = doc["color"].as<String>();
        int brightness = doc["brightness"].as<int>();
        
        // 设置模式
        if (mode == "solid") LED_SetMode(LED_SOLID);
        else if (mode == "flow") LED_SetMode(LED_FLOW);
        else if (mode == "breathe") LED_SetMode(LED_BREATHE);
        else if (mode == "off") LED_SetMode(LED_OFF);
        
        // 设置颜色
        CRGB rgbColor = hexToRGB(color);
        LED_SetColor(rgbColor);
        
        // 设置亮度 (0-100 映射到 0-255)
        uint8_t ledBrightness = map(brightness, 0, 100, 0, 255);
        LED_SetBrightness(ledBrightness);
        
        request->send(200, "application/json", "{\"success\":true}");
    }
);
```

### 5. 颜色转换

```cpp
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
```

---

## 🔒 线程安全

### FreeRTOS 任务架构

```
Core 0                          Core 1
------                          ------
- WiFi 处理                     - 图像解码 (优先级高)
- Web 服务器                    - LED 控制 (优先级低)
- 网络请求                      - 灯效动画
```

### 任务优先级

| 任务 | 核心 | 优先级 | 说明 |
|------|------|--------|------|
| 图像解码 | Core 1 | 高 | 保证显示流畅 |
| LED 控制 | Core 1 | 低 | 避免影响解码 |
| Web 服务器 | Core 0 | 中 | 网络处理 |

### 看门狗保护

所有灯效动画都包含 `vTaskDelay()`，避免饿死看门狗：

```cpp
vTaskDelay(pdMS_TO_TICKS(20));  // 最小 20ms 延迟
```

---

## 🎨 使用示例

### 示例 1：设置红色常亮

```bash
curl -X POST http://192.168.1.105/led \
  -H "Content-Type: application/json" \
  -d '{"mode":"solid","color":"#ff0000","brightness":"80"}'
```

### 示例 2：启动流水灯

```bash
curl -X POST http://192.168.1.105/led \
  -H "Content-Type: application/json" \
  -d '{"mode":"flow","color":"#00ff00","brightness":"50"}'
```

### 示例 3：呼吸灯效果

```bash
curl -X POST http://192.168.1.105/led \
  -H "Content-Type: application/json" \
  -d '{"mode":"breathe","color":"#0000ff","brightness":"60"}'
```

### 示例 4：关闭灯珠

```bash
curl -X POST http://192.168.1.105/led \
  -H "Content-Type: application/json" \
  -d '{"mode":"off","color":"#000000","brightness":"0"}'
```

---

## 🧪 测试用例

### 测试 1：基本功能

1. 上传固件
2. 访问 Web 控制台
3. 点击 "💡 常亮" 按钮
4. 观察 LED 是否点亮

**预期结果**: LED 显示选定的颜色

### 测试 2：流水灯效果

1. 点击 "🌊 流水灯" 按钮
2. 观察 LED 是否流动

**预期结果**: LED 显示彩虹流水效果

### 测试 3：呼吸灯效果

1. 点击 "💨 呼吸灯" 按钮
2. 观察 LED 是否渐变

**预期结果**: LED 显示呼吸渐变效果

### 测试 4：亮度调节

1. 调整亮度滑块
2. 点击任意模式按钮
3. 观察亮度变化

**预期结果**: LED 亮度随滑块变化

---

## 🐛 故障排查

### 问题 1：LED 不亮

**原因**:
- 接线错误
- 电源不足
- 引脚配置错误

**解决方案**:
1. 检查接线（VCC、GND、DIN）
2. 使用外部 5V 电源供电
3. 确认 `LED_PIN` 配置正确

### 问题 2：灯效卡顿

**原因**:
- 任务优先级过高
- 刷新频率过快
- CPU 负载过高

**解决方案**:
1. 降低 LED 任务优先级
2. 增加 `vTaskDelay` 延迟
3. 减少 LED 数量

### 问题 3：颜色不正确

**原因**:
- 颜色顺序配置错误
- 16 进制转换错误

**解决方案**:
1. 修改 `COLOR_ORDER` 为 RGB 或 GRB
2. 检查 `hexToRGB()` 函数

### 问题 4：系统重启

**原因**:
- 看门狗超时
- 内存溢出

**解决方案**:
1. 确保所有循环包含 `vTaskDelay()`
2. 减少任务栈大小
3. 检查串口日志

---

## 🚀 扩展功能

### 1. 更多灯效模式

```cpp
case LED_RAINBOW:
    // 彩虹渐变
    fill_rainbow(leds, NUM_LEDS, hue, 7);
    hue++;
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(20));
    break;

case LED_SPARKLE:
    // 闪烁效果
    fadeToBlackBy(leds, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[pos] = targetColor;
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(50));
    break;
```

### 2. 音乐律动

```cpp
// 根据音频输入调整灯效
uint8_t audioLevel = getAudioLevel();
uint8_t brightness = map(audioLevel, 0, 255, 0, 255);
FastLED.setBrightness(brightness);
```

### 3. 温度指示

```cpp
// 根据温度改变颜色
float temp = getTemperature();
CRGB color = temp > 30 ? CRGB::Red : CRGB::Blue;
fill_solid(leds, NUM_LEDS, color);
```

---

## 📚 相关文档

- [WebServer_Driver_notes.md](./WebServer_Driver_notes.md) - Web 服务器模块说明
- [README.md](../README.md) - 项目主页
- [QuickStart.md](../QuickStart.md) - 快速入门指南

---

## 📦 依赖库

- **FastLED** (^3.7.0): 高性能 LED 控制库
- **FreeRTOS**: ESP32 内置实时操作系统
- **ArduinoJson** (^7.2.1): JSON 解析库

---

**文档版本**: v1.0  
**创建时间**: 2026-02-23  
**创建者**: Kiro  
**最后更新**: 2026-02-23
