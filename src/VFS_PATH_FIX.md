# VFS 路径挂载错位问题修复

## 🎯 问题本质

这是一个经典的**"虚拟文件系统（VFS）路径挂载错位"**问题，导致"写得进，读不出"。

## 🔍 根本原因

### SD_MMC 库的路径处理机制
```cpp
// SD_Card.cpp 第 25 行
SD_MMC.begin("/sdcard", true, true);
```

当 `SD_MMC.begin()` 指定挂载点为 `/sdcard` 后，SD_MMC 库会**自动处理路径转换**：
- 代码中写：`/uploaded/xxx.jpg`
- 库自动转换为：`/sdcard/uploaded/xxx.jpg`

### 错误的路径处理

我之前错误地在代码中手动添加了 `/sdcard` 前缀，导致路径重复：

| 接口 | 错误路径 | 实际访问路径 | 结果 |
|------|---------|-------------|------|
| `/upload` | `/uploaded/xxx.jpg` | `/sdcard/uploaded/xxx.jpg` | ✅ 成功（库自动转换） |
| `/list` | `/sdcard/uploaded` | `/sdcard/sdcard/uploaded` | ❌ 目录不存在 |
| `/display` | `/sdcard/uploaded/xxx.jpg` | `/sdcard/sdcard/uploaded/xxx.jpg` | ❌ 文件不存在 |

## 🔧 修复方案

### 统一使用相对路径
所有接口都使用 `UPLOAD_DIR`（即 `/uploaded`），让 SD_MMC 库自动处理挂载点转换。

### 修复内容

#### 1. `/list` 接口
```cpp
// 修复前（错误）
String fullPath = "/sdcard" + String(UPLOAD_DIR);
if (listImageFiles(fullPath.c_str(), jsonList)) {

// 修复后（正确）
if (listImageFiles(UPLOAD_DIR, jsonList)) {
```

#### 2. `/display` 接口
```cpp
// 修复前（错误）
String filepath = "/sdcard" + String(UPLOAD_DIR) + "/" + filename;

// 修复后（正确）
String filepath = String(UPLOAD_DIR) + "/" + filename;
```

## 📋 路径标准总结

### 正确的路径使用规范
| 场景 | 代码中使用的路径 | SD_MMC 库实际访问 |
|------|----------------|------------------|
| 上传文件 | `/uploaded/temp_xxx.jpg` | `/sdcard/uploaded/temp_xxx.jpg` |
| 列出文件 | `/uploaded` | `/sdcard/uploaded` |
| 显示文件 | `/uploaded/xxx.jpg` | `/sdcard/uploaded/xxx.jpg` |
| 删除文件 | `/uploaded/xxx.jpg` | `/sdcard/uploaded/xxx.jpg` |

### 关键原则
1. **代码中永远不要手动添加 `/sdcard` 前缀**
2. **所有路径都使用 `UPLOAD_DIR` 宏定义**
3. **让 SD_MMC 库自动处理挂载点转换**

## 📝 验证方法

### 1. 重新编译上传
```bash
pio run -t upload
pio device monitor
```

### 2. 检查串口日志
点击"刷新列表"，应该看到：
```
--- 开始列出图片文件 ---
目录路径: /uploaded
✓ 目录打开成功，开始遍历文件...
  发现文件: 123.jpg (目录: 否)
    处理后的文件名: 123.jpg
    ✓ 添加到列表: 123.jpg
✓ 列表生成完成，共 7 个图片文件
```

### 3. 测试完整流程
1. 上传图片 -> 应该成功
2. 刷新列表 -> 应该显示所有图片
3. 点击显示 -> 屏幕应该立即显示图片

## ✅ 预期结果

- 图片列表正常显示所有已上传的图片
- 点击"显示"按钮后，屏幕立即切换到对应图片
- 串口日志显示正确的相对路径（如 `/uploaded/xxx.jpg`）
- 不再出现"目录不存在"或"文件不存在"错误

---
**修复时间**: 2026-02-22  
**修复人**: Kiro  
**影响文件**: `src/WebServer_Driver.cpp`  
**关键教训**: 当使用 VFS 挂载点时，应该使用相对路径，让库自动处理路径转换
