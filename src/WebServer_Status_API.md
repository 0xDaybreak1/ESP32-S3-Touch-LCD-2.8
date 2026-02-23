# Web 控制台动态 IP 显示功能

## 📋 文档信息

**创建时间**: 2026-02-23  
**创建者**: Kiro  
**版本**: v1.0  

---

## 🎯 功能概述

在 Web 控制台页面动态显示 ESP32-S3 开发板的局域网 IP 地址，解决用户不知道设备 IP 的痛点。

---

## 🔧 技术实现

### 1. 后端 API (`/status`)

**路由**: `GET /status`

**功能**: 返回系统状态信息，包括 IP 地址和连接状态

**实现代码**:

```cpp
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
```

**返回数据格式**:

```json
{
  "sta_ip": "192.168.1.105",
  "connected": true,
  "ap_mode": false,
  "ap_ip": "192.168.4.1"
}
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `sta_ip` | String | STA 模式的 IP 地址，未连接时为 "未连接" |
| `connected` | Boolean | 是否已连接到局域网 WiFi |
| `ap_mode` | Boolean | 是否处于 AP 模式 |
| `ap_ip` | String | AP 模式的 IP 地址 |

---

### 2. 前端 UI

**HTML 结构**:

在页面 header 区域添加 IP 显示元素：

```html
<div class="header">
    <h1>🖼️ ESP32 图片显示控制台</h1>
    <p>WiFi 无线图片传输与显示控制</p>
    <p id="ipDisplay" style="margin-top: 10px; font-weight: bold; color: #e2e8f0;">
        🌍 局域网 IP: 获取中...
    </p>
</div>
```

**样式说明**:

- 初始颜色：`#e2e8f0` (浅灰色)
- 已连接：`#c6f6d5` (绿色)
- AP 模式/未连接：`#fed7d7` (红色)

---

### 3. 前端逻辑

**JavaScript 实现**:

```javascript
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
```

**功能说明**:

1. **页面加载时**: 自动调用 `fetchSystemStatus()` 获取 IP
2. **定时刷新**: 每 10 秒自动刷新一次状态
3. **状态可视化**: 根据连接状态显示不同颜色和文本
4. **错误处理**: 网络请求失败时显示 "获取失败"

---

## 📊 显示效果

### 场景 1：STA 模式已连接

```
🌍 局域网 IP: 192.168.1.105
```

- 颜色：绿色 (`#c6f6d5`)
- 说明：设备已连接到局域网 WiFi

### 场景 2：AP 配网模式

```
📡 AP 模式 IP: 192.168.4.1 (未连接局域网)
```

- 颜色：红色 (`#fed7d7`)
- 说明：设备处于 AP 热点模式，等待配网

### 场景 3：未连接

```
🌍 局域网 IP: 未连接
```

- 颜色：红色 (`#fed7d7`)
- 说明：设备未连接到任何 WiFi

### 场景 4：获取失败

```
🌍 局域网 IP: 获取失败
```

- 颜色：默认
- 说明：网络请求失败

---

## 🚀 使用场景

### 场景 1：首次配网后查看 IP

1. 用户完成 WiFi 配网
2. 设备重启并连接到家庭 WiFi
3. 用户访问 `http://vision.local`
4. 页面自动显示局域网 IP：`192.168.1.105`
5. 用户可以记录此 IP，后续直接访问

### 场景 2：路由器 DHCP 变更

1. 路由器重启，设备获得新的 IP
2. 用户访问 Web 页面
3. 页面自动显示新的 IP 地址
4. 无需查看路由器管理界面

### 场景 3：AP 模式提示

1. 设备无法连接到配置的 WiFi
2. 自动降级到 AP 模式
3. 用户连接到 AP 热点
4. 页面显示：`📡 AP 模式 IP: 192.168.4.1 (未连接局域网)`
5. 用户知道需要重新配网

---

## 🔍 技术细节

### WiFi 状态检测

```cpp
bool connected = (WiFi.status() == WL_CONNECTED);
```

**WiFi.status() 返回值**:

| 状态 | 值 | 说明 |
|------|-----|------|
| `WL_IDLE_STATUS` | 0 | 空闲状态 |
| `WL_NO_SSID_AVAIL` | 1 | 找不到 SSID |
| `WL_SCAN_COMPLETED` | 2 | 扫描完成 |
| `WL_CONNECTED` | 3 | 已连接 |
| `WL_CONNECT_FAILED` | 4 | 连接失败 |
| `WL_CONNECTION_LOST` | 5 | 连接丢失 |
| `WL_DISCONNECTED` | 6 | 已断开 |

### IP 地址获取

```cpp
// STA 模式 IP
String staIP = WiFi.localIP().toString();

// AP 模式 IP
String apIP = WiFi.softAPIP().toString();
```

**默认 IP**:

- STA 模式：由路由器 DHCP 分配（如 `192.168.1.105`）
- AP 模式：固定为 `192.168.4.1`

### 自动刷新机制

```javascript
setInterval(fetchSystemStatus, 10000);  // 10 秒刷新一次
```

**刷新间隔选择**:

- 10 秒：平衡实时性和性能
- 可根据需求调整（5 秒、30 秒等）
- 过短会增加服务器负载

---

## 🧪 测试用例

### 测试 1：STA 模式连接成功

**步骤**:
1. 设备配网成功，连接到家庭 WiFi
2. 访问 Web 页面

**预期结果**:
- 显示：`🌍 局域网 IP: 192.168.x.x`
- 颜色：绿色

### 测试 2：AP 配网模式

**步骤**:
1. 设备首次启动或连接失败
2. 连接到 AP 热点
3. 访问 `http://192.168.4.1`

**预期结果**:
- 显示：`📡 AP 模式 IP: 192.168.4.1 (未连接局域网)`
- 颜色：红色

### 测试 3：自动刷新

**步骤**:
1. 打开 Web 页面
2. 等待 10 秒

**预期结果**:
- 页面自动刷新 IP 状态
- 无需手动刷新页面

### 测试 4：网络断开

**步骤**:
1. 设备已连接到 WiFi
2. 关闭路由器
3. 等待 10 秒

**预期结果**:
- 显示：`🌍 局域网 IP: 未连接`
- 颜色：红色

---

## 🔧 扩展功能

### 1. 显示更多信息

```javascript
// 显示 MAC 地址
ipDisplay.textContent = `🌍 IP: ${data.sta_ip} | MAC: ${data.mac}`;

// 显示信号强度
ipDisplay.textContent = `🌍 IP: ${data.sta_ip} | 信号: ${data.rssi} dBm`;
```

### 2. 一键复制 IP

```html
<button onclick="copyIP()">📋 复制 IP</button>

<script>
function copyIP() {
    const ip = document.getElementById('ipDisplay').textContent.split(': ')[1];
    navigator.clipboard.writeText(ip);
    alert('IP 已复制: ' + ip);
}
</script>
```

### 3. 二维码显示

生成包含 IP 地址的二维码，方便手机扫码访问：

```javascript
// 使用 qrcode.js 库
const qr = new QRCode(document.getElementById('qrcode'), {
    text: `http://${data.sta_ip}`,
    width: 128,
    height: 128
});
```

---

## 📚 相关文档

- [WebServer_Driver_notes.md](./WebServer_Driver_notes.md) - 模块说明文档
- [WebServer_WiFi_Provisioning.md](./WebServer_WiFi_Provisioning.md) - WiFi 配网技术详解
- [WIFI_IMAGE_TRANSFER_README.md](./WIFI_IMAGE_TRANSFER_README.md) - WiFi 功能完整文档

---

**文档版本**: v1.0  
**创建时间**: 2026-02-23  
**创建者**: Kiro  
**最后更新**: 2026-02-23
