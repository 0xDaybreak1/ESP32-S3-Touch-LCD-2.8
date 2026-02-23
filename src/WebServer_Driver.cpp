#include "WebServer_Driver.h"
#include "LED_Driver.h"
#include <ArduinoJson.h>

// 全局对象
AsyncWebServer server(80);
Preferences preferences;             // NVS 存储
SemaphoreHandle_t sdCardMutex = NULL;
char currentDisplayFile[100] = "";

// 播放列表相关
std::vector<String> customPlaylist;  // 自定义播放列表
bool useCustomPlaylist = false;      // 是否使用自定义播放列表

// WiFi 状态
bool isAPMode = false;               // 是否处于 AP 模式

// 上传状态
File uploadFile;
String uploadFilename = "";
size_t uploadedBytes = 0;

// WiFi 配网界面 HTML
const char wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi 配网</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            max-width: 500px;
            width: 100%;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 { font-size: 2em; margin-bottom: 10px; }
        .header p { opacity: 0.9; }
        .content { padding: 30px; }
        .form-group {
            margin-bottom: 20px;
        }
        .form-group label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
        }
        .form-group input {
            width: 100%;
            padding: 12px;
            border: 2px solid #e2e8f0;
            border-radius: 8px;
            font-size: 1em;
            transition: border-color 0.3s;
        }
        .form-group input:focus {
            outline: none;
            border-color: #667eea;
        }
        .btn {
            width: 100%;
            padding: 15px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1.1em;
            font-weight: 600;
            transition: all 0.3s;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-primary:hover {
            background: #5568d3;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        .btn-secondary {
            background: #e2e8f0;
            color: #333;
            margin-top: 10px;
        }
        .btn-secondary:hover {
            background: #cbd5e0;
        }
        .status {
            margin-top: 20px;
            padding: 15px;
            border-radius: 8px;
            display: none;
            text-align: center;
        }
        .status.success {
            background: #c6f6d5;
            color: #22543d;
            display: block;
        }
        .status.error {
            background: #fed7d7;
            color: #742a2a;
            display: block;
        }
        .info {
            background: #f0f4ff;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            color: #4c51bf;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📡 WiFi 配网</h1>
            <p>配置 ESP32 连接到您的 WiFi 网络</p>
        </div>
        
        <div class="content">
            <div class="info">
                💡 提示：配置成功后，设备将自动重启并连接到指定的 WiFi 网络。
            </div>
            
            <form id="wifiForm">
                <div class="form-group">
                    <label for="ssid">WiFi 名称 (SSID)</label>
                    <input type="text" id="ssid" name="ssid" placeholder="请输入 WiFi 名称" required>
                </div>
                
                <div class="form-group">
                    <label for="password">WiFi 密码</label>
                    <input type="password" id="password" name="password" placeholder="请输入 WiFi 密码" required>
                </div>
                
                <button type="submit" class="btn btn-primary">💾 保存并重启</button>
                <button type="button" class="btn btn-secondary" onclick="window.location.href='/'">🔙 返回主页</button>
            </form>
            
            <div class="status" id="status"></div>
        </div>
    </div>
    
    <script>
        const form = document.getElementById('wifiForm');
        const status = document.getElementById('status');
        
        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            
            if (!ssid) {
                showStatus('请输入 WiFi 名称', 'error');
                return;
            }
            
            try {
                const response = await fetch('/setwifi', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ ssid, password })
                });
                
                const data = await response.json();
                
                if (data.success) {
                    showStatus('✓ 配置保存成功！设备将在 2 秒后重启...', 'success');
                    
                    // 禁用表单
                    form.querySelectorAll('input, button').forEach(el => el.disabled = true);
                    
                    // 3 秒后跳转提示页面
                    setTimeout(() => {
                        document.body.innerHTML = `
                            <div style="display: flex; align-items: center; justify-content: center; min-height: 100vh; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);">
                                <div style="background: white; padding: 40px; border-radius: 20px; text-align: center; max-width: 500px;">
                                    <h2 style="color: #667eea; margin-bottom: 20px;">🎉 配置成功</h2>
                                    <p style="color: #666; margin-bottom: 20px;">设备正在重启并连接到 WiFi...</p>
                                    <p style="color: #999; font-size: 0.9em;">请稍后连接到相同的 WiFi 网络，然后访问 <strong>http://vision.local</strong></p>
                                </div>
                            </div>
                        `;
                    }, 2000);
                } else {
                    showStatus('✗ 配置失败: ' + data.message, 'error');
                }
            } catch (error) {
                showStatus('✗ 配置失败: ' + error.message, 'error');
            }
        });
        
        function showStatus(message, type) {
            status.textContent = message;
            status.className = 'status ' + type;
            status.style.display = 'block';
        }
    </script>
</body>
</html>
)rawliteral";

// Web 界面 HTML (嵌入式)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 图片显示控制台</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 { font-size: 2.5em; margin-bottom: 10px; }
        .header p { opacity: 0.9; font-size: 1.1em; }
        .content { padding: 30px; }
        .section {
            margin-bottom: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 10px;
        }
        .section h2 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.5em;
        }
        .upload-area {
            border: 3px dashed #667eea;
            border-radius: 10px;
            padding: 40px;
            text-align: center;
            cursor: pointer;
            transition: all 0.3s;
        }
        .upload-area:hover {
            background: #f0f4ff;
            border-color: #764ba2;
        }
        .upload-area.dragover {
            background: #e0e7ff;
            border-color: #4c51bf;
        }
        .btn {
            padding: 12px 30px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1em;
            transition: all 0.3s;
            margin: 5px;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-primary:hover {
            background: #5568d3;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        .btn-danger {
            background: #f56565;
            color: white;
        }
        .btn-danger:hover {
            background: #e53e3e;
        }
        .image-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
            gap: 20px;
            margin-top: 20px;
        }
        .image-card {
            background: white;
            border-radius: 10px;
            padding: 15px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            transition: all 0.3s;
        }
        .image-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 8px 15px rgba(0,0,0,0.2);
        }
        .image-card img {
            width: 100%;
            height: 150px;
            object-fit: cover;
            border-radius: 8px;
            margin-bottom: 10px;
        }
        .image-card .name {
            font-weight: bold;
            margin-bottom: 10px;
            word-break: break-all;
        }
        .progress-bar {
            width: 100%;
            height: 30px;
            background: #e2e8f0;
            border-radius: 15px;
            overflow: hidden;
            margin-top: 15px;
            display: none;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
            transition: width 0.3s;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: bold;
        }
        .status {
            margin-top: 15px;
            padding: 15px;
            border-radius: 8px;
            display: none;
        }
        .status.success {
            background: #c6f6d5;
            color: #22543d;
            display: block;
        }
        .status.error {
            background: #fed7d7;
            color: #742a2a;
            display: block;
        }
        input[type="file"] { display: none; }
        .color-picker {
            width: 100%;
            height: 50px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
        }
        .slider {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #e2e8f0;
            outline: none;
            margin: 15px 0;
        }
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🖼️ ESP32 图片显示控制台</h1>
            <p>WiFi 无线图片传输与显示控制</p>
            <p id="ipDisplay" style="margin-top: 10px; font-weight: bold; color: #e2e8f0;">🌍 局域网 IP: 获取中...</p>
        </div>
        
        <div class="content">
            <!-- 图片上传区域 -->
            <div class="section">
                <h2>📤 上传图片</h2>
                <div class="upload-area" id="uploadArea">
                    <p style="font-size: 3em; margin-bottom: 10px;">📁</p>
                    <p style="font-size: 1.2em; margin-bottom: 10px;">拖拽图片到此处或点击选择</p>
                    <p style="color: #718096;">支持任意图片格式 (自动转换为 240x320 JPEG)</p>
                    <input type="file" id="fileInput" accept="image/*" multiple>
                </div>
                <div class="progress-bar" id="progressBar">
                    <div class="progress-fill" id="progressFill">0%</div>
                </div>
                <div class="status" id="status"></div>
            </div>
            
            <!-- 图片列表 -->
            <div class="section">
                <h2>🖼️ 图片库</h2>
                <div style="margin-bottom: 15px;">
                    <button class="btn btn-primary" onclick="refreshImageList()">🔄 刷新列表</button>
                    <button class="btn btn-primary" onclick="playSelectedImages()">▶️ 播放选中图片</button>
                    <button class="btn btn-danger" onclick="stopPlaylist()">⏹️ 停止播放列表</button>
                </div>
                <div class="image-grid" id="imageGrid">
                    <p style="color: #718096;">加载中...</p>
                </div>
            </div>
            
            <!-- RGB 灯珠控制 (预留) -->
            <div class="section">
                <h2>🎨 RGB 灯珠控制</h2>
                <p style="color: #718096; margin-bottom: 15px;">选择颜色和亮度</p>
                <input type="color" class="color-picker" id="colorPicker" value="#ff0000">
                <p style="margin-top: 15px;">亮度: <span id="brightnessValue">50</span>%</p>
                <input type="range" class="slider" id="brightnessSlider" min="0" max="100" value="50">
                <div style="margin-top: 15px;">
                    <button class="btn btn-primary" onclick="setLED('solid')">💡 常亮</button>
                    <button class="btn btn-primary" onclick="setLED('flow')">🌊 流水灯</button>
                    <button class="btn btn-primary" onclick="setLED('breathe')">💨 呼吸灯</button>
                    <button class="btn btn-danger" onclick="setLED('off')">⚫ 关闭</button>
                </div>
            </div>
            
            <!-- WiFi 配网入口 -->
            <div class="section">
                <h2>📡 WiFi 配置</h2>
                <p style="color: #718096; margin-bottom: 15px;">配置设备连接到您的 WiFi 网络</p>
                <button class="btn btn-primary" onclick="window.location.href='/wifi'">⚙️ WiFi 配网</button>
            </div>
        </div>
    </div>
    
    <script>
        const uploadArea = document.getElementById('uploadArea');
        const fileInput = document.getElementById('fileInput');
        const progressBar = document.getElementById('progressBar');
        const progressFill = document.getElementById('progressFill');
        const status = document.getElementById('status');
        const imageGrid = document.getElementById('imageGrid');
        const brightnessSlider = document.getElementById('brightnessSlider');
        const brightnessValue = document.getElementById('brightnessValue');
        
        // 点击上传区域
        uploadArea.addEventListener('click', () => fileInput.click());
        
        // 文件选择
        fileInput.addEventListener('change', (e) => {
            handleFiles(e.target.files);
        });
        
        // 拖拽上传
        uploadArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            uploadArea.classList.add('dragover');
        });
        
        uploadArea.addEventListener('dragleave', () => {
            uploadArea.classList.remove('dragover');
        });
        
        uploadArea.addEventListener('drop', (e) => {
            e.preventDefault();
            uploadArea.classList.remove('dragover');
            handleFiles(e.dataTransfer.files);
        });
        
        // 处理文件上传 (Canvas 预处理版本)
        async function handleFiles(files) {
            for (let file of files) {
                if (!file.type.match('image/')) {
                    showStatus('仅支持图片格式', 'error');
                    continue;
                }
                
                // 在浏览器端预处理图片
                try {
                    const processedFile = await preprocessImage(file);
                    await uploadFile(processedFile);
                } catch (error) {
                    showStatus('图片处理失败: ' + error.message, 'error');
                }
            }
        }
        
        // 图片预处理：缩放到 240x320 并转换为 Baseline JPEG
        async function preprocessImage(file) {
            return new Promise((resolve, reject) => {
                const reader = new FileReader();
                
                reader.onload = (e) => {
                    const img = new Image();
                    
                    img.onload = () => {
                        // 创建离屏 Canvas
                        const canvas = document.createElement('canvas');
                        const ctx = canvas.getContext('2d');
                        
                        // 目标尺寸
                        const targetWidth = 240;
                        const targetHeight = 320;
                        
                        // 设置 Canvas 尺寸
                        canvas.width = targetWidth;
                        canvas.height = targetHeight;
                        
                        // 计算缩放比例 (cover 模式：填满整个画布，超出部分裁切)
                        const imgRatio = img.width / img.height;
                        const targetRatio = targetWidth / targetHeight;
                        
                        let drawWidth, drawHeight, offsetX, offsetY;
                        
                        if (imgRatio > targetRatio) {
                            // 图片更宽，以高度为准
                            drawHeight = targetHeight;
                            drawWidth = img.width * (targetHeight / img.height);
                            offsetX = (targetWidth - drawWidth) / 2;
                            offsetY = 0;
                        } else {
                            // 图片更高，以宽度为准
                            drawWidth = targetWidth;
                            drawHeight = img.height * (targetWidth / img.width);
                            offsetX = 0;
                            offsetY = (targetHeight - drawHeight) / 2;
                        }
                        
                        // 填充黑色背景
                        ctx.fillStyle = '#000000';
                        ctx.fillRect(0, 0, targetWidth, targetHeight);
                        
                        // 绘制图片
                        ctx.drawImage(img, offsetX, offsetY, drawWidth, drawHeight);
                        
                        // 转换为 Baseline JPEG (质量 0.85)
                        canvas.toBlob((blob) => {
                            if (!blob) {
                                reject(new Error('Canvas 转换失败'));
                                return;
                            }
                            
                            // 生成新文件名 (强制 .jpg 后缀)
                            let newFilename = file.name.replace(/\.[^.]+$/, '.jpg');
                            
                            // 创建新的 File 对象
                            const processedFile = new File([blob], newFilename, {
                                type: 'image/jpeg',
                                lastModified: Date.now()
                            });
                            
                            console.log(`图片预处理完成: ${file.name} -> ${newFilename}`);
                            console.log(`原始大小: ${(file.size / 1024).toFixed(2)} KB`);
                            console.log(`处理后大小: ${(processedFile.size / 1024).toFixed(2)} KB`);
                            
                            resolve(processedFile);
                        }, 'image/jpeg', 0.85);
                    };
                    
                    img.onerror = () => {
                        reject(new Error('图片加载失败'));
                    };
                    
                    img.src = e.target.result;
                };
                
                reader.onerror = () => {
                    reject(new Error('文件读取失败'));
                };
                
                reader.readAsDataURL(file);
            });
        }
        
        // 上传文件
        async function uploadFile(file) {
            const formData = new FormData();
            formData.append('file', file);
            
            progressBar.style.display = 'block';
            status.style.display = 'none';
            
            try {
                const xhr = new XMLHttpRequest();
                
                xhr.upload.addEventListener('progress', (e) => {
                    if (e.lengthComputable) {
                        const percent = Math.round((e.loaded / e.total) * 100);
                        progressFill.style.width = percent + '%';
                        progressFill.textContent = percent + '%';
                    }
                });
                
                xhr.addEventListener('load', () => {
                    if (xhr.status === 200) {
                        showStatus('上传成功: ' + file.name, 'success');
                        refreshImageList();
                    } else {
                        showStatus('上传失败: ' + xhr.statusText, 'error');
                    }
                    progressBar.style.display = 'none';
                });
                
                xhr.addEventListener('error', () => {
                    showStatus('上传失败: 网络错误', 'error');
                    progressBar.style.display = 'none';
                });
                
                xhr.open('POST', '/upload');
                xhr.send(formData);
                
            } catch (error) {
                showStatus('上传失败: ' + error.message, 'error');
                progressBar.style.display = 'none';
            }
        }
        
        // 显示状态消息
        function showStatus(message, type) {
            status.textContent = message;
            status.className = 'status ' + type;
            status.style.display = 'block';
            
            if (type === 'success') {
                setTimeout(() => {
                    status.style.display = 'none';
                }, 3000);
            }
        }
        
        // 刷新图片列表
        async function refreshImageList() {
            try {
                const response = await fetch('/list');
                const data = await response.json();
                
                if (data.files && data.files.length > 0) {
                    imageGrid.innerHTML = data.files.map(file => `
                        <div class="image-card">
                            <input type="checkbox" class="image-checkbox" value="${file}" style="margin-right: 8px;">
                            <div class="name">${file}</div>
                            <button class="btn btn-primary" onclick="displayImage('${file}')">📺 显示</button>
                            <button class="btn btn-danger" onclick="deleteImage('${file}')">🗑️ 删除</button>
                        </div>
                    `).join('');
                } else {
                    imageGrid.innerHTML = '<p style="color: #718096;">暂无图片</p>';
                }
            } catch (error) {
                imageGrid.innerHTML = '<p style="color: #f56565;">加载失败</p>';
            }
        }
        
        // 显示图片
        async function displayImage(filename) {
            try {
                const response = await fetch('/display?file=' + encodeURIComponent(filename));
                const data = await response.json();
                
                if (data.success) {
                    showStatus('正在显示: ' + filename, 'success');
                } else {
                    showStatus('显示失败: ' + data.message, 'error');
                }
            } catch (error) {
                showStatus('显示失败: ' + error.message, 'error');
            }
        }
        
        // 删除图片
        async function deleteImage(filename) {
            if (!confirm('确定要删除 ' + filename + ' 吗？')) return;
            
            try {
                const response = await fetch('/delete?file=' + encodeURIComponent(filename));
                const data = await response.json();
                
                if (data.success) {
                    showStatus('删除成功: ' + filename, 'success');
                    refreshImageList();
                } else {
                    showStatus('删除失败: ' + data.message, 'error');
                }
            } catch (error) {
                showStatus('删除失败: ' + error.message, 'error');
            }
        }
        
        // 播放选中的图片
        async function playSelectedImages() {
            const checkboxes = document.querySelectorAll('.image-checkbox:checked');
            const selectedFiles = Array.from(checkboxes).map(cb => cb.value);
            
            if (selectedFiles.length === 0) {
                showStatus('请先选择要播放的图片', 'error');
                return;
            }
            
            try {
                const response = await fetch('/playlist', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ playlist: selectedFiles })
                });
                
                const data = await response.json();
                
                if (data.success) {
                    showStatus(`已设置播放列表 (${selectedFiles.length} 张图片)`, 'success');
                } else {
                    showStatus('设置播放列表失败: ' + data.message, 'error');
                }
            } catch (error) {
                showStatus('设置播放列表失败: ' + error.message, 'error');
            }
        }
        
        // 停止播放列表（恢复全局轮播）
        async function stopPlaylist() {
            try {
                const response = await fetch('/playlist', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ playlist: [] })
                });
                
                const data = await response.json();
                
                if (data.success) {
                    showStatus('已恢复全局轮播', 'success');
                    // 取消所有复选框
                    document.querySelectorAll('.image-checkbox').forEach(cb => cb.checked = false);
                } else {
                    showStatus('操作失败: ' + data.message, 'error');
                }
            } catch (error) {
                showStatus('操作失败: ' + error.message, 'error');
            }
        }
        
        // RGB 灯珠控制
        function setLED(mode) {
            const color = document.getElementById('colorPicker').value;
            const brightness = brightnessSlider.value;
            
            fetch('/led', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ mode, color, brightness })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showStatus('LED 设置成功', 'success');
                } else {
                    showStatus('LED 设置失败', 'error');
                }
            })
            .catch(error => {
                showStatus('LED 设置失败: ' + error.message, 'error');
            });
        }
        
        // 亮度滑块
        brightnessSlider.addEventListener('input', (e) => {
            brightnessValue.textContent = e.target.value;
        });
        
        // 获取系统状态（IP 地址等）
        async function fetchSystemStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                
                const ipDisplay = document.getElementById('ipDisplay');
                
                if (data.connected) {
                    ipDisplay.textContent = `🌍 局域网 IP: ${data.sta_ip}`;
                    ipDisplay.style.color = '#c6f6d5';  // 绿色表示已连接
                } else if (data.ap_mode) {
                    ipDisplay.textContent = `📡 AP 模式 IP: ${data.ap_ip} (未连接局域网)`;
                    ipDisplay.style.color = '#fed7d7';  // 红色表示 AP 模式
                } else {
                    ipDisplay.textContent = '🌍 局域网 IP: 未连接';
                    ipDisplay.style.color = '#fed7d7';
                }
            } catch (error) {
                console.error('获取系统状态失败:', error);
                document.getElementById('ipDisplay').textContent = '🌍 局域网 IP: 获取失败';
            }
        }
        
        // 页面加载时刷新图片列表和系统状态
        refreshImageList();
        fetchSystemStatus();
        
        // 每 10 秒自动刷新一次状态
        setInterval(fetchSystemStatus, 10000);
    </script>
</body>
</html>
)rawliteral";

// 初始化 WiFi
void WebServer_Init() {
    Serial.println("\n========== WiFi 初始化 ==========");
    
    // 创建 SD 卡互斥锁
    if (sdCardMutex == NULL) {
        sdCardMutex = xSemaphoreCreateMutex();
        Serial.println("✓ SD 卡互斥锁创建成功");
    }
    
    // 创建上传目录
    if (!SD_MMC.exists(UPLOAD_DIR)) {
        SD_MMC.mkdir(UPLOAD_DIR);
        Serial.printf("✓ 创建上传目录: %s\n", UPLOAD_DIR);
    }
    
    // 🔧 【配网逻辑】尝试从 NVS 读取 WiFi 配置
    String savedSSID, savedPassword;
    bool hasConfig = loadWiFiConfig(savedSSID, savedPassword);
    
    if (hasConfig && savedSSID.length() > 0) {
        Serial.println("✓ 检测到已保存的 WiFi 配置");
        Serial.printf("  SSID: %s\n", savedSSID.c_str());
        
        // 尝试连接到保存的 WiFi
        if (connectToWiFi(savedSSID, savedPassword, WIFI_CONNECT_TIMEOUT)) {
            // 连接成功，使用 STA 模式
            isAPMode = false;
            Serial.println("✓ WiFi 连接成功 (STA 模式)");
            Serial.printf("  IP 地址: %s\n", WiFi.localIP().toString().c_str());
            
            // 可选：启动隐藏 AP 作为备用（注释掉则完全不开启 AP）
            // WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, 1, true);  // 最后一个参数 true 表示隐藏
            // Serial.println("✓ 备用 AP 已启动（隐藏）");
        } else {
            // 连接失败，启动 AP 模式
            Serial.println("✗ WiFi 连接失败，启动 AP 配网模式");
            WiFi.mode(WIFI_AP);
            WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
            isAPMode = true;
            
            IPAddress apIP = WiFi.softAPIP();
            Serial.printf("✓ AP 模式已启动\n");
            Serial.printf("  SSID: %s\n", WIFI_AP_SSID);
            Serial.printf("  密码: %s\n", WIFI_AP_PASSWORD);
            Serial.printf("  IP 地址: %s\n", apIP.toString().c_str());
        }
    } else {
        // 没有保存的配置，直接启动 AP 模式
        Serial.println("✓ 未检测到 WiFi 配置，启动 AP 配网模式");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        isAPMode = true;
        
        IPAddress apIP = WiFi.softAPIP();
        Serial.printf("✓ AP 模式已启动\n");
        Serial.printf("  SSID: %s\n", WIFI_AP_SSID);
        Serial.printf("  密码: %s\n", WIFI_AP_PASSWORD);
        Serial.printf("  IP 地址: %s\n", apIP.toString().c_str());
    }
    
    // 启动 mDNS 服务
    if (MDNS.begin(MDNS_HOSTNAME)) {
        Serial.printf("✓ mDNS 服务已启动\n");
        Serial.printf("  访问地址: http://%s.local\n", MDNS_HOSTNAME);
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("✗ mDNS 启动失败");
    }
    
    // 配置 Web 服务器路由
    
    // 主页
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });
    
    // 🔧 【新增】WiFi 配网页面
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", wifi_html);
    });
    
    // 🔧 【新增】系统状态 API
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        
        // 检查 STA 模式连接状态
        bool connected = (WiFi.status() == WL_CONNECTED);
        String staIP = connected ? WiFi.localIP().toString() : "未连接";
        
        json += "\"sta_ip\":\"" + staIP + "\",";
        json += "\"connected\":" + String(connected ? "true" : "false") + ",";
        json += "\"ap_mode\":" + String(isAPMode ? "true" : "false") + ",";
        json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\"";
        
        json += "}";
        
        request->send(200, "application/json", json);
    });
    
    // 🔧 【新增】WiFi 配置保存接口
    server.on("/setwifi", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // 只处理完整的数据包
            if (index + len != total) {
                return;
            }
            
            // 解析 JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                Serial.printf("✗ JSON 解析失败: %s\n", error.c_str());
                request->send(400, "application/json", "{\"success\":false,\"message\":\"JSON 解析失败\"}");
                return;
            }
            
            String ssid = doc["ssid"].as<String>();
            String password = doc["password"].as<String>();
            
            if (ssid.length() == 0) {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"SSID 不能为空\"}");
                return;
            }
            
            // 保存到 NVS
            if (saveWiFiConfig(ssid, password)) {
                Serial.printf("✓ WiFi 配置已保存\n");
                Serial.printf("  SSID: %s\n", ssid.c_str());
                
                request->send(200, "application/json", "{\"success\":true,\"message\":\"配置保存成功\"}");
                
                // 延迟 2 秒后重启
                delay(2000);
                Serial.println("✓ 正在重启...");
                ESP.restart();
            } else {
                Serial.println("✗ WiFi 配置保存失败");
                request->send(500, "application/json", "{\"success\":false,\"message\":\"配置保存失败\"}");
            }
        }
    );
    
    // 文件上传
    server.on("/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200, "application/json", "{\"success\":true}");
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            // 开始上传
            if (index == 0) {
                Serial.printf("\n--- 开始上传文件: %s ---\n", filename.c_str());
                uploadFilename = filename;
                uploadedBytes = 0;
                
                // 获取 SD 卡锁
                if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    String filepath = String(UPLOAD_DIR) + "/temp_" + filename;
                    uploadFile = SD_MMC.open(filepath.c_str(), FILE_WRITE);
                    
                    if (!uploadFile) {
                        Serial.println("✗ 无法创建临时文件");
                        xSemaphoreGive(sdCardMutex);
                        request->send(500, "application/json", "{\"success\":false,\"message\":\"无法创建文件\"}");
                        return;
                    }
                } else {
                    Serial.println("✗ 无法获取 SD 卡锁");
                    request->send(503, "application/json", "{\"success\":false,\"message\":\"SD 卡忙\"}");
                    return;
                }
            }
            
            // 写入数据块
            if (uploadFile && len) {
                uploadFile.write(data, len);
                uploadedBytes += len;
                
                // 每 100KB 打印一次进度
                if (uploadedBytes % 102400 < len) {
                    Serial.printf("  已上传: %d KB\n", uploadedBytes / 1024);
                }
            }
            
            // 上传完成
            if (final) {
                if (uploadFile) {
                    uploadFile.close();
                    
                    // 重命名临时文件
                    String tempPath = String(UPLOAD_DIR) + "/temp_" + filename;
                    String finalPath = String(UPLOAD_DIR) + "/" + filename;
                    
                    // 如果目标文件已存在，先删除
                    if (SD_MMC.exists(finalPath.c_str())) {
                        SD_MMC.remove(finalPath.c_str());
                    }
                    
                    // 重命名
                    SD_MMC.rename(tempPath.c_str(), finalPath.c_str());
                    
                    xSemaphoreGive(sdCardMutex);
                    
                    Serial.printf("✓ 上传完成: %s (%d 字节)\n", filename.c_str(), uploadedBytes);
                } else {
                    xSemaphoreGive(sdCardMutex);
                    Serial.println("✗ 上传失败");
                }
            }
        }
    );
    
    // 列出图片文件
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        String jsonList;
        // 🔧 【核心修复】：使用相对路径，让 SD_MMC 库自动处理挂载点
        if (listImageFiles(UPLOAD_DIR, jsonList)) {
            request->send(200, "application/json", jsonList);
        } else {
            request->send(200, "application/json", "{\"files\":[]}");
        }
    });
    
    // 显示图片
    server.on("/display", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("file")) {
            String filename = request->getParam("file")->value();
            // 🔧 【核心修复】：使用相对路径，让 SD_MMC 库自动处理挂载点
            String filepath = String(UPLOAD_DIR) + "/" + filename;
            
            // 更新当前显示文件
            strncpy(currentDisplayFile, filepath.c_str(), sizeof(currentDisplayFile) - 1);
            
            Serial.printf("Web 请求显示: %s\n", filepath.c_str());
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"缺少文件参数\"}");
        }
    });
    
    // 删除图片
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("file")) {
            String filename = request->getParam("file")->value();
            
            if (deleteImageFile(filename.c_str())) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(500, "application/json", "{\"success\":false,\"message\":\"删除失败\"}");
            }
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"缺少文件参数\"}");
        }
    });
    
    // 设置播放列表
    server.on("/playlist", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // 只处理完整的数据包
            if (index + len != total) {
                return;
            }
            
            // 解析 JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                Serial.printf("✗ JSON 解析失败: %s\n", error.c_str());
                request->send(400, "application/json", "{\"success\":false,\"message\":\"JSON 解析失败\"}");
                return;
            }
            
            // 清空现有播放列表
            customPlaylist.clear();
            
            // 获取播放列表数组
            JsonArray playlist = doc["playlist"];
            
            if (playlist.size() == 0) {
                // 空数组，恢复全局轮播
                useCustomPlaylist = false;
                Serial.println("✓ 已恢复全局轮播模式");
                request->send(200, "application/json", "{\"success\":true,\"message\":\"已恢复全局轮播\"}");
                return;
            }
            
            // 添加文件到播放列表
            for (JsonVariant file : playlist) {
                String filename = file.as<String>();
                customPlaylist.push_back(filename);
                Serial.printf("  添加到播放列表: %s\n", filename.c_str());
            }
            
            // 启用自定义播放列表
            useCustomPlaylist = true;
            
            Serial.printf("✓ 播放列表已设置 (%d 张图片)\n", customPlaylist.size());
            
            String response = "{\"success\":true,\"count\":" + String(customPlaylist.size()) + "}";
            request->send(200, "application/json", response);
        }
    );
    
    // RGB 灯珠控制
    server.on("/led", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // 只处理完整的数据包
            if (index + len != total) {
                return;
            }
            
            // 解析 JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                Serial.printf("✗ LED JSON 解析失败: %s\n", error.c_str());
                request->send(400, "application/json", "{\"success\":false,\"message\":\"JSON 解析失败\"}");
                return;
            }
            
            // 提取参数
            String mode = doc["mode"].as<String>();
            String color = doc["color"].as<String>();
            int brightness = doc["brightness"].as<int>();
            
            Serial.printf("\n--- LED 控制请求 ---\n");
            Serial.printf("  模式: %s\n", mode.c_str());
            Serial.printf("  颜色: %s\n", color.c_str());
            Serial.printf("  亮度: %d%%\n", brightness);
            
            // 设置模式
            if (mode == "solid") {
                LED_SetMode(LED_SOLID);
            } else if (mode == "flow") {
                LED_SetMode(LED_FLOW);
            } else if (mode == "breathe") {
                LED_SetMode(LED_BREATHE);
            } else if (mode == "off") {
                LED_SetMode(LED_OFF);
            } else {
                Serial.printf("✗ 未知模式: %s\n", mode.c_str());
                request->send(400, "application/json", "{\"success\":false,\"message\":\"未知模式\"}");
                return;
            }
            
            // 设置颜色 (将 16 进制字符串转换为 CRGB)
            if (color.length() > 0) {
                CRGB rgbColor = hexToRGB(color);
                LED_SetColor(rgbColor);
            }
            
            // 设置亮度 (将 0-100 映射到 0-255)
            if (brightness >= 0 && brightness <= 100) {
                uint8_t ledBrightness = map(brightness, 0, 100, 0, 255);
                LED_SetBrightness(ledBrightness);
            }
            
            Serial.println("✓ LED 控制成功");
            request->send(200, "application/json", "{\"success\":true}");
        }
    );
    
    // 启动服务器
    server.begin();
    Serial.println("✓ Web 服务器已启动");
    Serial.println("==================================\n");
}

// 主循环处理 (异步库不需要)
void WebServer_Loop() {
    // ESPAsyncWebServer 是异步的，不需要在 loop 中调用
}

// 获取本地 IP (STA 模式)
String getLocalIP() {
    return WiFi.localIP().toString();
}

// 获取 AP IP
String getAPIP() {
    return WiFi.softAPIP().toString();
}

// 检查是否有客户端连接
bool isClientConnected() {
    return WiFi.softAPgetStationNum() > 0;
}

// 列出图片文件
bool listImageFiles(const char* directory, String& jsonList) {
    Serial.printf("\n--- 开始列出图片文件 ---\n");
    Serial.printf("目录路径: %s\n", directory);
    
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        Serial.println("✗ 无法获取 SD 卡锁");
        return false;
    }
    
    File dir = SD_MMC.open(directory);
    if (!dir) {
        Serial.println("✗ 无法打开目录");
        xSemaphoreGive(sdCardMutex);
        return false;
    }
    
    if (!dir.isDirectory()) {
        Serial.println("✗ 路径不是目录");
        dir.close();
        xSemaphoreGive(sdCardMutex);
        return false;
    }
    
    Serial.println("✓ 目录打开成功，开始遍历文件...");
    
    jsonList = "{\"files\":[";
    bool first = true;
    int fileCount = 0;
    
    File file = dir.openNextFile();
    while (file) {
        Serial.printf("  发现文件: %s (目录: %s)\n", file.name(), file.isDirectory() ? "是" : "否");
        
        if (!file.isDirectory()) {
            String filename = String(file.name());
            
            // 🔧 【核心修复】：file.name() 可能返回完整路径，需要提取文件名
            // 例如: "/sdcard/uploaded/123.jpg" -> "123.jpg"
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash >= 0) {
                filename = filename.substring(lastSlash + 1);
            }
            
            Serial.printf("    处理后的文件名: %s\n", filename.c_str());
            
            // 过滤图片文件
            if (filename.endsWith(".jpg") || filename.endsWith(".jpeg") || 
                filename.endsWith(".png") || filename.endsWith(".bmp") ||
                filename.endsWith(".JPG") || filename.endsWith(".JPEG") ||
                filename.endsWith(".PNG") || filename.endsWith(".BMP")) {
                
                if (!first) jsonList += ",";
                jsonList += "\"" + filename + "\"";
                first = false;
                fileCount++;
                Serial.printf("    ✓ 添加到列表: %s\n", filename.c_str());
            }
        }
        file = dir.openNextFile();
    }
    
    jsonList += "]}";
    dir.close();
    xSemaphoreGive(sdCardMutex);
    
    Serial.printf("✓ 列表生成完成，共 %d 个图片文件\n", fileCount);
    Serial.printf("JSON: %s\n", jsonList.c_str());
    Serial.println("--- 列出图片文件完成 ---\n");
    
    return true;
}

// 删除图片文件
bool deleteImageFile(const char* filename) {
    if (isFileInUse(filename)) {
        Serial.printf("✗ 文件正在使用中，无法删除: %s\n", filename);
        return false;
    }
    
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }
    
    String filepath = String(UPLOAD_DIR) + "/" + filename;
    bool result = SD_MMC.remove(filepath.c_str());
    
    xSemaphoreGive(sdCardMutex);
    
    if (result) {
        Serial.printf("✓ 文件已删除: %s\n", filename);
    } else {
        Serial.printf("✗ 文件删除失败: %s\n", filename);
    }
    
    return result;
}

// 检查文件是否正在使用
bool isFileInUse(const char* filepath) {
    return (strcmp(currentDisplayFile, filepath) == 0);
}

// 锁定文件
void lockFile(const char* filepath) {
    strncpy(currentDisplayFile, filepath, sizeof(currentDisplayFile) - 1);
}

// 解锁文件
void unlockFile(const char* filepath) {
    if (strcmp(currentDisplayFile, filepath) == 0) {
        currentDisplayFile[0] = '\0';
    }
}

// ========== WiFi 配网辅助函数 ==========

// 从 NVS 加载 WiFi 配置
bool loadWiFiConfig(String& ssid, String& password) {
    preferences.begin("wifi", true);  // 只读模式
    
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    
    preferences.end();
    
    return (ssid.length() > 0);
}

// 保存 WiFi 配置到 NVS
bool saveWiFiConfig(const String& ssid, const String& password) {
    preferences.begin("wifi", false);  // 读写模式
    
    bool success = true;
    
    if (preferences.putString("ssid", ssid) == 0) {
        success = false;
    }
    
    if (preferences.putString("password", password) == 0) {
        success = false;
    }
    
    preferences.end();
    
    return success;
}

// 连接到 WiFi
bool connectToWiFi(const String& ssid, const String& password, unsigned long timeout) {
    Serial.printf("正在连接到 WiFi: %s\n", ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeout) {
            Serial.println("✗ WiFi 连接超时");
            return false;
        }
        
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    return true;
}

// 清除 WiFi 配置
void clearWiFiConfig() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    
    Serial.println("✓ WiFi 配置已清除");
}
