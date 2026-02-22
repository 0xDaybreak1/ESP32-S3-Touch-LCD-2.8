/*
 * ESP32-S3 图片显示示例
 * 支持格式: JPEG, PNG, BMP
 * 使用 LVGL 框架和多种图片解码库
 */

#include <Arduino.h>
#include "Display_ST7789.h"
#include "Audio_PCM5101.h"
#include "RTC_PCF85063.h"
#include "Gyro_QMI8658.h"
#include "LVGL_Driver.h"
#include "PWR_Key.h"
#include "SD_Card.h"
#include "LVGL_Example.h"
#include "BAT_Driver.h"
#include "Wireless.h"
#include "Simulated_Gesture.h"
#include "Image_Decoder.h"

// 后台驱动任务
void DriverTask(void *parameter) {
  Wireless_Test2();
  while(1){
    PWR_Loop();
    BAT_Get_Volts();
    PCF85063_Loop();
    QMI8658_Loop(); 
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// 创建后台驱动任务
void Driver_Loop() {
  xTaskCreatePinnedToCore(
    DriverTask,           
    "DriverTask",         
    4096,                 
    NULL,                 
    3,                    
    NULL,                 
    0                     
  );  
}

// 显示图片的 LVGL 界面
void createImageDisplayUI() {
  // 清空屏幕
  lv_obj_clean(lv_scr_act());
  
  // 创建标题标签
  lv_obj_t *title = lv_label_create(lv_scr_act());
  lv_label_set_text(title, "图片显示演示");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
  
  // 创建状态标签
  lv_obj_t *status = lv_label_create(lv_scr_act());
  lv_label_set_text(status, "正在加载图片...");
  lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// 初始化系统
void setup()
{
  // 初始化基础硬件
  Flash_test();
  Button_Init();
  PWR_Init();
  BAT_Init();
  I2C_Init();
  PCF85063_Init();
  QMI8658_Init();
  Backlight_Init();

  // 初始化存储和音频
  SD_Init();
  Audio_Init();
  
  // 初始化显示屏
  LCD_Init();
  Lvgl_Init();
  
  // 初始化图片解码器
  initImageDecoder();
  
  // 创建 UI 界面
  createImageDisplayUI();
  
  // 初始化模拟触摸
  Simulated_Touch_Init();
  
  // 启动后台驱动任务
  Driver_Loop();

  // 设置屏幕亮度
  Set_Backlight(80);
  
  // 延迟以确保所有初始化完成
  delay(1000);
  
  Serial.println("系统初始化完成");
  Serial.println("准备显示图片: /sdcard/test1.jpg");
}

// 主循环
void loop()
{
    // ⛔ 【核心修改】：彻底注释掉 LVGL 的刷新，防止其抢占 SPI
    // Lvgl_Loop(); 

    static unsigned long lastSwitchTime = 0;
    static int imageIndex = 0;
    const unsigned long displayInterval = 5000; 

    const char* imageFiles[] = {
        "/test1.jpg",
        "/test2.jpg"
    };
    const int totalImages = 2;

    if (millis() - lastSwitchTime > displayInterval) {
        lastSwitchTime = millis();
        
        Serial.printf("\n--- 正在独立解码: %s ---\n", imageFiles[imageIndex]);

        // 尝试显示图片
        if (loadAndDisplayImage(imageFiles[imageIndex])) {
            Serial.println("✓ 渲染成功！");
        } else {
            Serial.println("✗ 渲染失败！");
        }

        imageIndex = (imageIndex + 1) % totalImages;
    }

    vTaskDelay(pdMS_TO_TICKS(10)); 
}