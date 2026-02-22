# 动态图库轮播功能实现

## 📋 功能概述

将 `main.cpp` 中的硬编码图片数组改为动态读取 SD 卡 `/uploaded` 目录，实现自动轮播所有上传的图片。

## 🔧 实现细节

### 1. 全局变量
```cpp
static File uploadedDir;           // 上传目录句柄
static bool dirInitialized = false; // 目录是否已初始化
```

### 2. 核心函数：`getNextImageFile()`

#### 功能
- 从 `/uploaded` 目录中获取下一张图片文件
- 自动过滤隐藏文件和非图片格式
- 到达目录末尾时自动循环

#### 实现逻辑
1. **首次调用或重新打开目录**
   - 使用 `SD_MMC.open(UPLOAD_DIR)` 打开目录
   - 验证目录是否有效

2. **遍历目录**
   - 使用 `openNextFile()` 获取下一个文件
   - 如果返回 NULL，调用 `rewindDirectory()` 重新开始

3. **文件过滤**
   - 跳过目录
   - 跳过隐藏文件（以 `.` 或 `._` 开头）
   - 只保留图片格式（`.jpg`, `.jpeg`, `.png`, `.bmp`）

4. **路径处理**
   - 提取纯文件名（去除路径前缀）
   - 拼接完整路径：`/uploaded/xxx.jpg`

### 3. 修改后的 `loop()` 函数

#### 自动轮播逻辑
```cpp
// 每 5 秒切换一次
if (millis() - lastSwitchTime > displayInterval) {
    // 获取 SD 卡锁
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        String nextImage = getNextImageFile();
        
        if (nextImage.length() > 0) {
            loadAndDisplayImage(nextImage.c_str());
        }
        
        xSemaphoreGive(sdCardMutex);
    }
}
```

#### Web 请求优先级
- Web 请求显示图片时，会重置自动切换计时器
- 确保用户手动选择的图片能完整显示 5 秒

## ✅ 功能特性

### 1. 动态目录遍历
- 不需要预先知道图片数量
- 自动适应目录内容变化
- 循环播放所有图片

### 2. 格式过滤
- 支持：`.jpg`, `.jpeg`, `.png`, `.bmp`（大小写不敏感）
- 自动跳过隐藏文件（`.DS_Store`, `._xxx` 等）
- 自动跳过子目录

### 3. 线程安全
- 所有 SD 卡操作都在互斥锁保护下进行
- 避免与 Web 上传/列表操作冲突

### 4. 路径兼容性
- 自动处理 `file.name()` 返回的完整路径
- 统一使用相对路径（`/uploaded/xxx.jpg`）
- 兼容 SD_MMC 库的 VFS 挂载机制

## 📝 使用说明

### 1. 上传图片
通过 Web 控制台上传图片到 `/uploaded` 目录

### 2. 自动轮播
- 系统会自动每 5 秒切换一次图片
- 按照文件系统顺序轮播
- 到达末尾后自动循环

### 3. 手动控制
- 通过 Web 控制台点击"显示"按钮
- 手动选择的图片会立即显示
- 自动轮播计时器会重置

## 🔍 调试日志

### 正常运行
```
✓ 已打开 /uploaded 目录
→ 找到图片: /uploaded/123.jpg

--- 自动轮播: /uploaded/123.jpg ---
正在加载 JPEG 图片: /uploaded/123.jpg
✓ 渲染成功！

→ 找到图片: /uploaded/456.jpg

--- 自动轮播: /uploaded/456.jpg ---
✓ 渲染成功！

→ 目录遍历完成，重新开始轮播
```

### 目录为空
```
✗ /uploaded 目录为空
✗ 没有可轮播的图片
```

### 目录打开失败
```
✗ 无法打开 /uploaded 目录
✗ 没有可轮播的图片
```

## ⚠️ 注意事项

### 1. 目录必须存在
- 确保 SD 卡上存在 `/uploaded` 目录
- 如果目录不存在，轮播功能会失败

### 2. 文件命名规范
- 避免使用特殊字符
- 建议使用英文和数字
- 避免文件名过长（建议 < 50 字符）

### 3. 性能考虑
- 目录遍历在主循环中进行，速度很快
- 不会阻塞其他任务
- SD 卡锁超时设置为 1 秒，避免死锁

## 🚀 未来优化方向

1. **缓存文件列表**
   - 在 `setup()` 中预先扫描目录
   - 将文件列表存储在数组中
   - 减少 SD 卡访问次数

2. **随机播放模式**
   - 添加随机播放选项
   - 使用 `random()` 函数选择图片

3. **播放速度控制**
   - 通过 Web 控制台调整 `displayInterval`
   - 支持快速/慢速轮播

4. **播放状态指示**
   - 在屏幕上显示当前图片序号
   - 显示总图片数量

---
**实现时间**: 2026-02-22  
**实现人**: Kiro  
**影响文件**: `src/main.cpp`
