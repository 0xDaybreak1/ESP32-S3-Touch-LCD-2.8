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
  Serial.println("准备显示图片: /sdcard/test1.png");
}

// 主循环
void loop()
{
  // 刷新 LVGL 显示
  Lvgl_Loop();
  
  // 静态变量用于控制图片显示时机
  static bool imageDisplayed = false;
  static unsigned long lastDisplayTime = 0;
  
  // 仅在启动后 2 秒显示一次图片
  if (!imageDisplayed && (millis() - lastDisplayTime > 2000)) {
    Serial.println("\n========== 开始显示图片 ==========");
    
    // 尝试显示 PNG 图片
    if (loadAndDisplayImage("/sdcard/test1.png")) {
      Serial.println("✓ PNG 图片显示成功");
    } else {
      Serial.println("✗ PNG 图片显示失败，尝试其他格式...");
      
      // 尝试 JPEG 格式
      if (loadAndDisplayImage("/sdcard/test1.jpg")) {
        Serial.println("✓ JPEG 图片显示成功");
      } else {
        // 尝试 BMP 格式
        if (loadAndDisplayImage("/sdcard/test1.bmp")) {
          Serial.println("✓ BMP 图片显示成功");
        } else {
          Serial.println("✗ 所有格式都失败，请检查文件是否存在");
        }
      }
    }
    
    Serial.println("========== 图片显示完成 ==========\n");
    imageDisplayed = true;
  }
  
  // 延迟以避免过度占用 CPU
  vTaskDelay(pdMS_TO_TICKS(5));
}
