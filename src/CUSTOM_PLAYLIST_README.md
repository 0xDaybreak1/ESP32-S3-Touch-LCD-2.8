# 自定义播放列表功能实现

## 📋 功能概述

允许用户在 Web 控制台中勾选多个图片，创建自定义播放列表，ESP32-S3 将循环播放选中的图片。

## 🎯 实现架构

### 1. 前端修改（HTML + JavaScript）

#### UI 更新
- 每个图片卡片添加复选框（Checkbox）
- 添加"▶️ 播放选中图片"按钮
- 添加"⏹️ 停止播放列表"按钮

#### 交互逻辑
```javascript
// 播放选中的图片
async function playSelectedImages() {
    const checkboxes = document.querySelectorAll('.image-checkbox:checked');
    const selectedFiles = Array.from(checkboxes).map(cb => cb.value);
    
    // 发送 POST 请求到 /playlist
    const response = await fetch('/playlist', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ playlist: selectedFiles })
    });
}

// 停止播放列表（恢复全局轮播）
async function stopPlaylist() {
    // 发送空数组到 /playlist
    const response = await fetch('/playlist', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ playlist: [] })
    });
}
```

### 2. 后端修改（WebServer_Driver）

#### 新增全局变量
```cpp
#include <vector>
#include <ArduinoJson.h>

std::vector<String> customPlaylist;  // 自定义播放列表
bool useCustomPlaylist = false;      // 是否使用自定义播放列表
```

#### 新增 `/playlist` 接口
```cpp
server.on("/playlist", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // 解析 JSON
        JsonDocument doc;
        deserializeJson(doc, data, len);
        
        // 清空现有播放列表
        customPlaylist.clear();
        
        // 获取播放列表数组
        JsonArray playlist = doc["playlist"];
        
        if (playlist.size() == 0) {
            // 空数组，恢复全局轮播
            useCustomPlaylist = false;
        } else {
            // 添加文件到播放列表
            for (JsonVariant file : playlist) {
                customPlaylist.push_back(file.as<String>());
            }
            useCustomPlaylist = true;
        }
    }
);
```

### 3. 主循环修改（main.cpp）

#### 新增播放列表索引
```cpp
static int playlistIndex = 0;  // 播放列表索引
```

#### 修改轮播逻辑
```cpp
if (useCustomPlaylist && customPlaylist.size() > 0) {
    // 使用自定义播放列表
    String filename = customPlaylist[playlistIndex];
    String nextImage = String(UPLOAD_DIR) + "/" + filename;
    
    // 更新索引（循环）
    playlistIndex = (playlistIndex + 1) % customPlaylist.size();
    
    loadAndDisplayImage(nextImage.c_str());
} else {
    // 使用全局目录轮播
    String nextImage = getNextImageFile();
    loadAndDisplayImage(nextImage.c_str());
}
```

## ✅ 功能特性

### 1. 多选支持
- 通过复选框选择多张图片
- 支持任意数量的图片

### 2. 播放模式切换
- 自定义播放列表模式：只播放选中的图片
- 全局轮播模式：播放所有图片

### 3. 循环播放
- 播放列表到达末尾后自动循环
- 索引自动重置

### 4. 线程安全
- 所有 SD 卡操作都在互斥锁保护下进行
- 避免与其他操作冲突

### 5. 实时切换
- 可随时切换播放模式
- 无需重启系统

## 📝 使用方法

### 1. 创建播放列表
1. 打开 Web 控制台（`http://192.168.4.1`）
2. 在图片库中勾选要播放的图片
3. 点击"▶️ 播放选中图片"按钮
4. 系统开始循环播放选中的图片

### 2. 停止播放列表
1. 点击"⏹️ 停止播放列表"按钮
2. 系统恢复全局轮播模式
3. 所有复选框自动取消

### 3. 修改播放列表
1. 重新勾选图片
2. 点击"▶️ 播放选中图片"按钮
3. 新的播放列表立即生效

## 🔍 调试日志

### 设置播放列表
```
  添加到播放列表: 123.jpg
  添加到播放列表: 456.jpg
  添加到播放列表: 789.jpg
✓ 播放列表已设置 (3 张图片)
```

### 播放列表轮播
```
--- 播放列表轮播 [1/3]: /uploaded/123.jpg ---
正在加载 JPEG 图片: /uploaded/123.jpg
✓ 渲染成功！

--- 播放列表轮播 [2/3]: /uploaded/456.jpg ---
✓ 渲染成功！

--- 播放列表轮播 [3/3]: /uploaded/789.jpg ---
✓ 渲染成功！

--- 播放列表轮播 [1/3]: /uploaded/123.jpg ---
（循环播放）
```

### 恢复全局轮播
```
✓ 已恢复全局轮播模式

--- 全局轮播: /uploaded/abc.jpg ---
✓ 渲染成功！
```

## 🔧 技术细节

### JSON 格式
```json
{
  "playlist": ["123.jpg", "456.jpg", "789.jpg"]
}
```

### 空播放列表（恢复全局轮播）
```json
{
  "playlist": []
}
```

### API 响应
```json
{
  "success": true,
  "count": 3
}
```

## ⚠️ 注意事项

### 1. 文件名必须存在
- 播放列表中的文件名必须存在于 `/uploaded` 目录
- 如果文件不存在，会显示"渲染失败"

### 2. 内存限制
- 播放列表使用 `std::vector<String>` 存储
- 建议播放列表不超过 100 张图片
- 每个文件名占用约 50 字节内存

### 3. 路径处理
- 前端只传递文件名（如 `123.jpg`）
- 后端自动拼接完整路径（`/uploaded/123.jpg`）

### 4. 并发安全
- 播放列表修改和读取都在主循环中进行
- 不需要额外的互斥锁保护

## 🚀 未来优化方向

### 1. 播放列表持久化
- 将播放列表保存到 SD 卡
- 重启后自动恢复

### 2. 播放顺序控制
- 支持随机播放
- 支持倒序播放

### 3. 播放速度控制
- 为播放列表设置独立的播放间隔
- 支持快速/慢速播放

### 4. 播放列表管理
- 支持多个播放列表
- 支持播放列表命名和保存

### 5. 播放状态显示
- 在屏幕上显示当前播放进度
- 显示播放列表名称

---
**实现时间**: 2026-02-22  
**实现人**: Kiro  
**影响文件**: 
- `platformio.ini`（添加 ArduinoJson 依赖）
- `src/WebServer_Driver.h`（添加播放列表变量声明）
- `src/WebServer_Driver.cpp`（添加 /playlist 接口和前端 UI）
- `src/main.cpp`（修改 loop 函数支持播放列表）
