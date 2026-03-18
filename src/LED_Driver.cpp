#include "LED_Driver.h"

// ============================================================
// FastLED 要求编译期已知大小的静态数组
// active_led_count 控制运行时实际操作的灯珠数量
// ============================================================
CRGB leds[MAX_LEDS];

// ============================================================
// 运行时配置（受 Mutex 保护）
// ============================================================

// 视觉索引 → 物理索引的映射表
// led_map[i] = j 表示：视觉上第 i 个位置，对应物理接线第 j 颗灯
static uint16_t led_map[MAX_LEDS];

// 当前实际生效的灯珠数量
static uint16_t active_led_count = 0;

// 当前灯效
static LED_Effect_t current_effect = LED_OFF;

// 静态纯色（LED_STATIC 模式使用）
static CRGB static_color = CRGB::White;

// 互斥锁：保护 active_led_count、led_map、current_effect 的并发读写
static SemaphoreHandle_t led_mutex = nullptr;

// ============================================================
// 内部辅助：用视觉索引安全地写入物理灯珠颜色
// 调用前须已持有 led_mutex
// ============================================================
static inline void set_visual(uint16_t visual_idx, CRGB color) {
    // 通过映射表将视觉索引转换为物理索引后赋色
    leds[led_map[visual_idx]] = color;
}

// ============================================================
// 灯效实现
// ============================================================

/**
 * 流水彩虹效果
 * 核心思路：
 *   - hue_offset 随时间递增，形成色带推移
 *   - 颜色计算完全基于"视觉索引 i"，保证视觉上相邻的灯珠颜色连续
 *   - 最终通过 set_visual(i, color) 写入物理灯珠，解耦视觉与物理顺序
 */
static void effect_flow(uint8_t& hue_offset) {
    for (uint16_t i = 0; i < active_led_count; i++) {
        // 视觉索引 i 均匀分布在色轮上，再叠加时间偏移
        uint8_t hue = hue_offset + (uint8_t)((i * 255) / active_led_count);
        set_visual(i, CHSV(hue, 255, 255));
    }
    hue_offset += 2; // 每帧推移速度，值越大流动越快
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(20));
}

/**
 * 呼吸灯效果
 * 全部灯珠同步亮度正弦变化
 */
static void effect_breath(uint8_t& breath_phase) {
    // sin8 返回 0~255，映射到亮度
    uint8_t brightness = sin8(breath_phase);
    for (uint16_t i = 0; i < active_led_count; i++) {
        set_visual(i, CHSV(160, 200, brightness)); // 蓝紫色呼吸
    }
    breath_phase += 3;
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(20));
}

/**
 * 静态纯色效果
 */
static void effect_static(void) {
    for (uint16_t i = 0; i < active_led_count; i++) {
        set_visual(i, static_color);
    }
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(100)); // 静态模式降低刷新频率节省 CPU
}

/**
 * 关闭所有灯珠
 */
static void effect_off(void) {
    // 直接操作物理数组全部清零，无需映射
    fill_solid(leds, MAX_LEDS, CRGB::Black);
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(100));
}

// ============================================================
// FreeRTOS 任务
// ============================================================
static void LED_Task(void* parameter) {
    uint8_t hue_offset   = 0;
    uint8_t breath_phase = 0;
    uint32_t frame_count = 0;

    printf("LED_Task: ✅ 任务启动，运行在 Core %d\r\n", xPortGetCoreID());

    while (1) {
        // 加锁读取当前配置快照，尽快释放锁，避免长时间持锁
        LED_Effect_t effect_snapshot;
        uint16_t     count_snapshot;

        if (xSemaphoreTake(led_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            effect_snapshot = current_effect;
            count_snapshot  = active_led_count;
            xSemaphoreGive(led_mutex);
        } else {
            // 获取锁超时，跳过本帧
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // 每 100 帧打印一次心跳，确认任务存活
        if (frame_count % 100 == 0) {
            printf("LED_Task: 心跳 frame=%lu effect=%d count=%d 堆栈剩余=%lu\r\n",
                   frame_count, (int)effect_snapshot, count_snapshot,
                   uxTaskGetStackHighWaterMark(nullptr));
        }
        frame_count++;

        // 无灯珠配置时跳过渲染
        if (count_snapshot == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // 根据快照执行对应灯效
        switch (effect_snapshot) {
            case LED_FLOW:
                effect_flow(hue_offset);
                break;
            case LED_BREATH:
                effect_breath(breath_phase);
                break;
            case LED_STATIC:
                effect_static();
                break;
            case LED_OFF:
            default:
                effect_off();
                break;
        }
    }
}

// ============================================================
// 对外接口实现
// ============================================================

void LED_Init(void) {
    // 创建互斥锁
    led_mutex = xSemaphoreCreateMutex();
    configASSERT(led_mutex != nullptr);

    // 初始化默认映射：视觉顺序 == 物理顺序
    for (uint16_t i = 0; i < MAX_LEDS; i++) {
        led_map[i] = i;
    }

    // 设置默认灯珠数量，确保任务启动后立即可以渲染
    // 如需修改数量或映射表，调用 LED_SetConfig() 覆盖即可
    active_led_count = 16;

    // FastLED 注册灯带（编译期静态数组，MAX_LEDS 固定）
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, MAX_LEDS);
    FastLED.setBrightness(50);
    FastLED.clear(true);

    // 创建 LED 任务，固定在 Core 1 运行
    // 堆栈设为 4096：FastLED.show() 内部有较深的调用链，2048 容易溢出导致任务崩溃
    BaseType_t ret = xTaskCreatePinnedToCore(
        LED_Task,
        "LED_Task",
        4096,
        nullptr,
        2,
        nullptr,
        1
    );

    if (ret != pdPASS) {
        printf("LED_Driver: ❌ 任务创建失败！ret=%d\r\n", ret);
    } else {
        printf("LED_Driver: ✅ 初始化完成，active_led_count=%d，MAX_LEDS=%d\r\n", active_led_count, MAX_LEDS);
    }
}

void LED_SetConfig(uint16_t count, const uint16_t* map_array) {
    if (count > MAX_LEDS) {
        printf("LED_Driver: count=%d 超过 MAX_LEDS=%d，已截断\r\n", count, MAX_LEDS);
        count = MAX_LEDS;
    }

    if (xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        active_led_count = count;

        if (map_array != nullptr) {
            // 使用调用方提供的映射表
            // map_array[视觉索引] = 物理FastLED索引
            memcpy(led_map, map_array, count * sizeof(uint16_t));
            printf("LED_Driver: 已设置自定义映射，count=%d\r\n", count);
        } else {
            // 未提供映射表，默认视觉顺序 == 物理顺序
            for (uint16_t i = 0; i < count; i++) {
                led_map[i] = i;
            }
            printf("LED_Driver: 使用默认顺序映射，count=%d\r\n", count);
        }

        xSemaphoreGive(led_mutex);
    } else {
        printf("LED_Driver: LED_SetConfig 获取锁超时\r\n");
    }
}

void LED_SetEffect(LED_Effect_t effect) {
    if (xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        current_effect = effect;
        printf("LED_Driver: SetEffect → %d\r\n", (int)effect);
        xSemaphoreGive(led_mutex);
    }
}

void LED_SetColor(uint8_t r, uint8_t g, uint8_t b) {
    if (xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        static_color = CRGB(r, g, b);
        // 注意：不在此处切换模式，模式由 LED_SetEffect 单独控制
        // WebServer 会先调 SetEffect 设置模式，再调 SetColor 设置颜色
        // 若在此处强制切换到 LED_STATIC，会覆盖掉流水灯/呼吸灯等模式
        printf("LED_Driver: SetColor → R=%d G=%d B=%d\r\n", r, g, b);
        xSemaphoreGive(led_mutex);
    }
}

void LED_SetBrightness(uint8_t brightness) {
    // FastLED.setBrightness 本身是线程安全的简单赋值，无需额外加锁
    FastLED.setBrightness(brightness);
}
