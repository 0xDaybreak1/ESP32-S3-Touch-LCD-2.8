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
#include "WebServer_Driver.h"
#include "LED_Driver.h"

// 后台驱动任务
void DriverTask(void *parameter) {
  // ⛔ 【核心修复】：注释掉 Wireless_Test2() 以避免与 WebServer_Init() 的 AP 模式冲突
  // Wireless_Test2() 会将 WiFi 设置为 STA 模式并扫描网络，最后关闭 WiFi
  // 这会导致 AP 热点无法建立或被强制关闭
  // Wireless_Test2();
  
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
  
  // 初始化 Web 服务器
  WebServer_Init();
  
  // 初始化 RGB LED 灯珠
  LED_Init();
  
  // 延迟以确保所有初始化完成
  delay(1000);
  
  Serial.println("\n========== 系统初始化完成 ==========");
  Serial.println("准备显示图片: /sdcard/test1.jpg");
  Serial.printf("Web 控制台: http://vision.local 或 http://%s\n", getAPIP().c_str());
  Serial.println("====================================\n");
}

// 全局变量：用于动态轮播
static File uploadedDir;           // 上传目录句柄
static bool dirInitialized = false; // 目录是否已初始化
static int playlistIndex = 0;       // 播放列表索引

// 获取下一张图片文件
// 返回值：成功返回完整路径，失败返回空字符串
String getNextImageFile() {
    // 首次调用或需要重新打开目录
    if (!dirInitialized || !uploadedDir) {
        uploadedDir = SD_MMC.open(UPLOAD_DIR);
        if (!uploadedDir || !uploadedDir.isDirectory()) {
            Serial.println("✗ 无法打开 /uploaded 目录");
            return "";
        }
        dirInitialized = true;
        Serial.println("✓ 已打开 /uploaded 目录");
    }
    
    // 遍历目录，查找下一张图片
    while (true) {
        File file = uploadedDir.openNextFile();
        
        // 如果到达目录末尾，重新开始
        if (!file) {
            Serial.println("→ 目录遍历完成，重新开始轮播");
            uploadedDir.rewindDirectory();
            file = uploadedDir.openNextFile();
            
            // 如果目录为空
            if (!file) {
                Serial.println("✗ /uploaded 目录为空");
                return "";
            }
        }
        
        // 跳过目录
        if (file.isDirectory()) {
            continue;
        }
        
        String filename = String(file.name());
        
        // 提取纯文件名（去除路径前缀）
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
            filename = filename.substring(lastSlash + 1);
        }
        
        // 跳过隐藏文件（以 . 或 ._ 开头）
        if (filename.startsWith(".") || filename.startsWith("._")) {
            continue;
        }
        
        // 过滤图片格式
        if (filename.endsWith(".jpg") || filename.endsWith(".jpeg") || 
            filename.endsWith(".png") || filename.endsWith(".bmp") ||
            filename.endsWith(".JPG") || filename.endsWith(".JPEG") ||
            filename.endsWith(".PNG") || filename.endsWith(".BMP")) {
            
            // 拼接完整路径
            String fullPath = String(UPLOAD_DIR) + "/" + filename;
            Serial.printf("→ 找到图片: %s\n", fullPath.c_str());
            return fullPath;
        }
    }
}

// 主循环
void loop()
{
    // ⛔ 【核心修改】：彻底注释掉 LVGL 的刷新，防止其抢占 SPI
    // Lvgl_Loop(); 

    static unsigned long lastSwitchTime = 0;
    const unsigned long displayInterval = 5000; 

    // 检查是否有 Web 请求显示图片
    if (strlen(currentDisplayFile) > 0) {
        Serial.printf("\n--- Web 请求显示: %s ---\n", currentDisplayFile);
        
        // 获取 SD 卡锁
        if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            if (loadAndDisplayImage(currentDisplayFile)) {
                Serial.println("✓ Web 图片显示成功！");
            } else {
                Serial.println("✗ Web 图片显示失败！");
            }
            xSemaphoreGive(sdCardMutex);
        }
        
        // 清空请求
        currentDisplayFile[0] = '\0';
        lastSwitchTime = millis(); // 重置自动切换计时器
    }
    // 自动轮播图片
    else if (millis() - lastSwitchTime > displayInterval) {
        lastSwitchTime = millis();
        
        // 获取 SD 卡锁
        if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            String nextImage = "";
            
            // 判断使用自定义播放列表还是全局轮播
            if (useCustomPlaylist && customPlaylist.size() > 0) {
                // 使用自定义播放列表
                String filename = customPlaylist[playlistIndex];
                nextImage = String(UPLOAD_DIR) + "/" + filename;
                
                Serial.printf("\n--- 播放列表轮播 [%d/%d]: %s ---\n", 
                             playlistIndex + 1, customPlaylist.size(), nextImage.c_str());
                
                // 更新索引（循环）
                playlistIndex = (playlistIndex + 1) % customPlaylist.size();
            } else {
                // 使用全局目录轮播
                nextImage = getNextImageFile();
                
                if (nextImage.length() > 0) {
                    Serial.printf("\n--- 全局轮播: %s ---\n", nextImage.c_str());
                }
            }
            
            // 显示图片
            if (nextImage.length() > 0) {
                if (loadAndDisplayImage(nextImage.c_str())) {
                    Serial.println("✓ 渲染成功！");
                } else {
                    Serial.println("✗ 渲染失败！");
                }
            } else {
                Serial.println("✗ 没有可轮播的图片");
            }
            
            xSemaphoreGive(sdCardMutex);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(10)); 
}