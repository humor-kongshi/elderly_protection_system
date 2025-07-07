# This file is executed on every boot (including wake-boot from deepsleep)
#import esp
#esp.osdebug(None)
#import webrepl
#webrepl.start()

import network
import socket
import time
import _thread
from machine import UART, Pin,RTC
import hashlib
from umqtt.simple import MQTTClient
import json
import umail

# 阿里云IoT平台相关配置
info = {"clientId":"your_client_id",
        "username":"your_username","mqttHostUrl":"xxx.mqtt.iothub.aliyuncs.com",
        "passwd":"your_passwd","port":1883}
PRODUCT_KEY = "product_key"
DEVICE_NAME = "ESP32_S3"
DEVICE_SECRET = "your_device_secret"
REGION_ID = "cn-shanghai"             

subTopic = "/" + PRODUCT_KEY + "/" + DEVICE_NAME + "/user/get"

message_received = False

# 配置参数
WIFI_SSID = "your_ssid"
WIFI_PASSWORD = "your_password"
IMAGE_FILE = "received.jpg"
TEXT_FILE = "coordinates.txt"
ak = "your_baidu_map_ak"
uart2 = UART(2, 115200)
uart1 = UART(1,9600,tx=33,rx=32) 

# 全局变量
image_updated = False
text_content = "No data received yet"
file_lock = _thread.allocate_lock()
img_lock = _thread.allocate_lock()

# 邮件服务器配置
SMTP_SERVER = "smtp.xxx.com"
SMTP_PORT = 465
SENDER_EMAIL = "xxx@xx.com"
SENDER_PASSWORD = "your_email_passwd"
RECIPIENT_EMAIL = "xxxxx@xx.com"
SEND_TIME = 100


site1_x = 0.0
site1_y = 0.0
site2_x = 0.0
site2_y = 0.0
alarm_triggered = False 

def convert_coordinate(lng, lat, from_type=1, to_type=5):
    """
    调用百度地图API进行坐标转换
    
    参数:
        lng: 经度
        lat: 纬度
        from_type: 源坐标类型（1: GPS坐标，2: 搜狗地图坐标，3: 火星坐标等）
        to_type: 目标坐标类型（5: 百度BD-09坐标）
    返回:
        (转换后的经度, 转换后的纬度) 或 None
    """
    try:
        # 构建API请求URL
        api_url = f"http://api.map.baidu.com/geoconv/v1/?coords={lng},{lat}&from={from_type}&to={to_type}&ak={ak}"
        
        # 解析URL
        host = "api.map.baidu.com"
        path = f"/geoconv/v1/?coords={lng},{lat}&from={from_type}&to={to_type}&ak={ak}"
        
        # 创建socket连接
        addr_info = socket.getaddrinfo(host, 80)
        addr = addr_info[0][-1]
        s = socket.socket()
        s.settimeout(10)  # 设置超时时间10秒
        s.connect(addr)
        
        # 发送HTTP请求
        request = (
            f"GET {path} HTTP/1.1\r\n"
            f"Host: {host}\r\n"
            f"User-Agent: ESP32 MicroPython\r\n"
            f"Connection: close\r\n\r\n"
        )
        s.send(request.encode())
        
        # 接收响应
        response = b""
        while True:
            data = s.recv(1024)
            if not data:
                break
            response += data
        
        # 关闭连接
        s.close()
        
        # 解析HTTP响应
        header_separator = b"\r\n\r\n"
        sep_idx = response.find(header_separator)
        if sep_idx == -1:
            print("未找到HTTP头分隔符")
            return None
        
        # 提取JSON数据
        json_data = response[sep_idx + 4:].decode()
        
        # 解析JSON
        result = json.loads(json_data)
        
        # 检查API返回状态
        if result.get("status") == 0 and "result" in result and len(result["result"]) > 0:
            converted_lng = result["result"][0]["x"]
            converted_lat = result["result"][0]["y"]
            print(f"坐标转换成功: ({lng}, {lat}) -> ({converted_lng}, {converted_lat})")
            return (converted_lng, converted_lat)
        else:
            print(f"坐标转换失败，错误码: {result.get('status')}，消息: {result.get('message')}")
            return None
            
    except Exception as e:
        print(f"坐标转换出错: {str(e)}")
        return None
    finally:
        try:
            s.close()
        except:
            pass

#IMAGE_URL = "http://api.map.baidu.com/staticimage/v2?ak=ljLl5DU2BhOnpygvpZx7BpcEBGFdN00i&center=114.3934,30.52414&width=320&height=240&zoom=15&markers=114.3934,30.52414&markerStyles=l,,0xFF0000"
SAVE_PATH = "baidu_map.jpg"
def get_baidu_map_image(IMAGE_URL):
    """通过socket获取百度地图图片并保存"""
    try:
        # 解析URL
        if IMAGE_URL.startswith("http://"):
            url = IMAGE_URL[7:]  # 去除http://前缀
        else:
            print("URL必须以http://开头")
            return False
        
        # 分割主机和路径
        host_end_idx = url.find("/")
        if host_end_idx == -1:
            host = url
            path = "/"
        else:
            host = url[:host_end_idx]
            path = url[host_end_idx:]
        
        # 创建socket并连接
        addr_info = socket.getaddrinfo(host, 80)  # 80是HTTP默认端口
        addr = addr_info[0][-1]
        s = socket.socket()
        s.settimeout(20)  # 设置超时时间10秒
        s.connect(addr)
        print(f"已连接到 {host}")
        
        # 发送HTTP GET请求
        request = (
            f"GET {path} HTTP/1.1\r\n"
            f"Host: {host}\r\n"
            f"User-Agent: ESP32 MicroPython\r\n"
            f"Connection: close\r\n\r\n"
        )
        s.send(request.encode())
        print("已发送请求，等待响应...")
        
        # 接收响应（先接收HTTP头，再处理图片数据）
        response = b""
        while True:
            data = s.recv(2048)  # 每次接收2KB
            if not data:
                break
            response += data
        
        # 关闭连接
        s.close()
        
        # 分离HTTP头和图片数据（HTTP头和内容以\r\n\r\n分隔）
        header_separator = b"\r\n\r\n"
        sep_idx = response.find(header_separator)
        if sep_idx == -1:
            print("未找到HTTP头分隔符")
            return False
        
        # 提取HTTP状态码
        headers = response[:sep_idx].decode()
        if "200 OK" not in headers:
            print(f"请求失败，状态码: {headers.split()[1]}")
            return False
        
        # 提取图片二进制数据
        image_data = response[sep_idx + 4:]  # +4是跳过\r\n\r\n
        if len(image_data) < 100:  # 简单判断图片数据是否有效
            print("获取的图片数据异常")
            return False
        
        # 保存图片
        with open(SAVE_PATH, "wb") as f:
            f.write(image_data)
        
        print(f"图片保存成功！路径: {SAVE_PATH}，大小: {len(image_data)}字节")
        return True
        
    except Exception as e:
        print(f"获取图片失败: {str(e)}")
        return False
    finally:
        # 确保socket关闭
        try:
            s.close()
        except:
            pass

def send_email(content:str):
    global SEND_TIME
    try:
        if time.time()-SEND_TIME>=10:
            smtp = umail.SMTP('smtp.163.com', 465, ssl=True)
            smtp.login(SENDER_EMAIL, SENDER_PASSWORD)
            smtp.to(RECIPIENT_EMAIL)
            smtp.write(content)
            smtp.send()
            smtp.quit()
            SEND_TIME = time.time()
            print("发送成功")
        else:
            print("避免重复发送")
            print(time.time()-SEND_TIME)
    except Exception as e:
        print(e)
    return True

# 连接WiFi
def connect_wifi():
    sta_if = network.WLAN(network.STA_IF)
    if not sta_if.isconnected():
        print('Connecting to network...')
        sta_if.active(True)
        #sta_if.ifconfig(('192.168.43.100', '255.255.255.0', '192.168.43.1', '223.5.5.5'))
        sta_if.connect(WIFI_SSID, WIFI_PASSWORD)
        while not sta_if.isconnected():
            pass
    print('Network config:', sta_if.ifconfig())
    return sta_if.ifconfig()[0]


def is_point_in_rectangle(point_x, point_y, site1_x, site1_y, site2_x, site2_y):
    """
    判断给定的经纬度坐标是否在矩形区域内
    
    参数:
    point_x (float): 待判断点的经度
    point_y (float): 待判断点的纬度
    site1_x (float): 矩形一个对角点的经度
    site1_y (float): 矩形一个对角点的纬度
    site2_x (float): 矩形另一个对角点的经度
    site2_y (float): 矩形另一个对角点的纬度
    
    返回:
    bool: 如果点在矩形内返回 True，否则返回 False
    """
    # 计算矩形的最小和最大经度
    min_x = min(site1_x, site2_x)
    max_x = max(site1_x, site2_x)
    
    # 计算矩形的最小和最大纬度
    min_y = min(site1_y, site2_y)
    max_y = max(site1_y, site2_y)
    
    # 判断点是否在矩形范围内
    return (min_x <= point_x <= max_x) and (min_y <= point_y <= max_y) 


def write_to_first_line(file_path, content, max_lines=20):
    """
    将内容写入文件的第一行，并保留最近的max_lines行数据，自动添加时间戳
    如果新内容与当前第一行相同（不包含时间戳），则不执行写入
    
    参数:
        file_path (str): 文件路径
        content (str): 要写入的内容
        max_lines (int): 最多保留的行数，默认为20
    """
    try:
        # 读取现有文件内容
        try:
            with open(file_path, 'r') as f:
                lines = f.readlines()
                if not lines:  # 文件为空，直接写入
                    pass
                else:
                    # 提取当前第一行的内容（不包含时间戳）
                    first_line = lines[0].strip()
                    # 查找时间戳开始位置
                    timestamp_start = first_line.rfind('[')
                    if timestamp_start != -1:
                        existing_content = first_line[:timestamp_start].strip()
                        # 比较新内容与现有内容（忽略时间戳）
                        if existing_content == content:
                            print("内容与第一行相同，跳过写入")
                            return False
        except (OSError, FileNotFoundError):
            lines = []
        
        # 获取当前时间戳
        rtc = RTC()
        timestamp = rtc.datetime()
        time_str = f"{timestamp[0]}-{timestamp[1]:02d}-{timestamp[2]:02d} {timestamp[4]:02d}:{timestamp[5]:02d}:{timestamp[6]:02d}"
        
        # 添加时间戳到内容末尾
        content_with_timestamp = f"{content} [{time_str}]"
        
        # 添加新内容到首行
        lines.insert(0, content_with_timestamp + '\n')
        
        # 截断超出限制的行
        if len(lines) > max_lines:
            lines = lines[:max_lines]
        
        # 写回文件
        with open(file_path, 'w') as f:
            for line in lines:
                f.write(line)
            
        return True
        
    except Exception as e:
        print(f"写入文件失败: {e}")
        return False 
        
    except Exception as e:
        print(f"写入文件失败: {e}")
        return False
    
def message_callback(topic, msg):
    #print(f"收到来自 {topic.decode()} 的消息: {msg.decode()}")
    # 在这里可以添加解析传感器数据的代码
    # 例如: 解析JSON格式的数据
    global message_received
    try:
        data = json.loads(msg.decode())
        data_str = str(data["items"]["GeoLocation"]["value"])
        print(data_str)
        try:
        # 简单解析JSON格式字符串
            data = eval(data_str.replace("'", "\""))  # 注意：仅适用于简单JSON
            latitude = data.get('Latitude')
            longitude = data.get('Longitude')
            with file_lock:
                if latitude:
                    if latitude == 90:
                        write_to_first_line("coordinates.txt","unable to locate")
                            
                    else:
                        write_to_first_line("coordinates.txt",f"Latitude:{latitude},Longitude:{longitude}")
                        if alarm_triggered:
                            if is_point_in_rectangle(latitude,longitude,site1_x,site1_y,site2_x,site2_y):
                                pass
                            else:
                                send_email(f"Subject:位置报警:纬度:{latitude},经度:{longitude}")
                        if longitude == 0:
                                pass
                        else:
                            with img_lock:        
                                lng,lat = convert_coordinate(longitude,latitude)
                                IMAGE_URL = f"http://api.map.baidu.com/staticimage/v2?ak={ak}&center={lng},{lat}&width=320&height=240&zoom=15&markers={lng},{lat}&markerStyles=l,,0xFF0000"

                                get_baidu_map_image(IMAGE_URL)
                    print("经纬度已保存到coordinates.txt")
                    message_received = True
                else:
                    print("未找到经纬度数据")
        except Exception as e:
            print(f"处理失败: {e}")
    except:
        pass

# 主函数
def get_aliyun_data():
    global message_received
    mqtt_client = MQTTClient(info["clientId"],
                        info["mqttHostUrl"],
                        info["port"],
                        info["username"],
                        info["passwd"], keepalive=100)
    
    # 设置消息回调函数
    mqtt_client.set_callback(message_callback)
    print("mqtt开始连接")
    try:
        # 连接到MQTT代理服务器
        mqtt_client.connect()
        print("已连接到阿里云IoT平台")
        
        # 订阅设备数据接收Topic
        # 替换为你需要订阅的具体Topic
        topic = subTopic
        mqtt_client.subscribe(topic)
        print(f"已订阅主题: {topic}")
        
        # 循环接收消息
        while True:
            mqtt_client.wait_msg()
    except Exception as e:
        print(f"连接或接收消息时出错: {e}")
    finally:
        # 断开连接
        mqtt_client.disconnect()
        print("已断开与阿里云IoT平台的连接")
        message_received = False
        time.sleep(1)

# 串口监听线程
def serial_monitor():
    
        
    time.sleep(0.1)

# Web服务器线程

def web_server():
    global site1_x, site1_y, site2_x, site2_y, alarm_triggered
    
    html = """<!DOCTYPE html>
<html>
<head>
    <title>ESP32资源查看器</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #444; }
        .container { max-width: 800px; margin: 0 auto; }
        .card { background: #f9f9f9; border-radius: 8px; padding: 15px; margin-bottom: 20px; }
        .btn { background-color: #4CAF50; color: white; padding: 10px 15px; text-decoration: none; border-radius: 4px; }
        pre { background-color: #f5f5f5; padding: 10px; overflow-x: auto; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; }
        input { width: 100%; padding: 8px; margin-bottom: 10px; }
        .coordinate-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        .switch { position: relative; display: inline-block; width: 60px; height: 34px; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }
        .slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .slider { background-color: #2196F3; }
        input:focus + .slider { box-shadow: 0 0 1px #2196F3; }
        input:checked + .slider:before { transform: translateX(26px); }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32资源查看器</h1>
        
        <div class="card">
            <h3>报警设置</h3>
            <label class="switch">
                <input type="checkbox" id="alarmSwitch">
                <span class="slider"></span>
            </label>
            <label for="alarmSwitch">启用报警</label>
            <p id="alarmStatus">当前状态: <span id="alarmText">未启用</span></p>
        </div>
        
        <div class="card">
            <h3>坐标管理</h3>
            <form id="coordForm" method="post" action="/update_coords">
                <div class="coordinate-grid">
                    <div>
                        <h4>左上角</h4>
                        <div class="form-group">
                            <label for="site1_x">纬度坐标:</label>
                            <input type="number" id="site1_x" name="site1_x" step="0.0001" required>
                        </div>
                        <div class="form-group">
                            <label for="site1_y">经度坐标:</label>
                            <input type="number" id="site1_y" name="site1_y" step="0.0001" required>
                        </div>
                    </div>
                    <div>
                        <h4>右下角</h4>
                        <div class="form-group">
                            <label for="site2_x">纬度坐标:</label>
                            <input type="number" id="site2_x" name="site2_x" step="0.0001" required>
                        </div>
                        <div class="form-group">
                            <label for="site2_y">经度坐标:</label>
                            <input type="number" id="site2_y" name="site2_y" step="0.0001" required>
                        </div>
                    </div>
                </div>
                <button type="submit" class="btn">保存坐标</button>
            </form>
        </div>
        
        <div class="card">
            <h3>GPS历史记录</h3>
            <a href="/download.txt" class="btn">下载文本文件</a>
            <pre id="txtContent">加载中...</pre>
        </div>
        <div class="card">
            <h3>位置图片预览</h3>
            <a href="/download.jpg" class="btn">下载图片</a>
            <img id="imgPreview" class="img-preview" src="/image.jpg" alt="GPS位置图像">
        </div>
        <div class="card">
            <h3>系统信息</h3>
            <p>当前时间: <span id="currentTime">加载中...</span></p>
            <p>存储空间: <span id="storageInfo">加载中...</span></p>
            <p>报警状态: <span id="systemAlarmStatus">未启用</span></p>
        </div>
    </div>
    
    <script>
        // 更新系统信息
        function updateSystemInfo() {
            fetch('/system_info')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('currentTime').textContent = data.time;
                    document.getElementById('storageInfo').textContent = data.storage;
                    document.getElementById('systemAlarmStatus').textContent = data.alarm ? '启用' : '未启用';
                });
        }
        
        // 加载文本内容
        function loadTextContent() {
            fetch('/text_content')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('txtContent').textContent = data;
                });
        }
        
        // 加载坐标数据
        function loadCoordinates() {
            fetch('/coordinates')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('site1_x').value = data.site1_x;
                    document.getElementById('site1_y').value = data.site1_y;
                    document.getElementById('site2_x').value = data.site2_x;
                    document.getElementById('site2_y').value = data.site2_y;
                });
        }
        
        // 加载报警状态
        function loadAlarmStatus() {
            fetch('/alarm_status')
                .then(response => response.json())
                .then(data => {
                    const isAlarmEnabled = data.alarm;
                    document.getElementById('alarmSwitch').checked = isAlarmEnabled;
                    document.getElementById('alarmText').textContent = isAlarmEnabled ? '已启用' : '未启用';
                    document.getElementById('alarmText').style.color = isAlarmEnabled ? 'red' : 'gray';
                });
        }
        
        // 切换报警状态
        document.getElementById('alarmSwitch').addEventListener('change', function() {
            const isChecked = this.checked;
            document.getElementById('alarmText').textContent = isChecked ? '已启用' : '未启用';
            document.getElementById('alarmText').style.color = isChecked ? 'red' : 'gray';
            
            fetch('/set_alarm', {
                method: 'POST',
                body: JSON.stringify({ alarm: isChecked }),
                headers: {
                    'Content-Type': 'application/json'
                }
            })
            .then(response => response.text())
            .then(message => {
                console.log(message);
                updateSystemInfo(); // 更新系统信息中的报警状态
            })
            .catch(error => {
                alert('设置报警状态失败: ' + error);
                // 恢复之前的状态
                this.checked = !isChecked;
                document.getElementById('alarmText').textContent = isChecked ? '未启用' : '已启用';
                document.getElementById('alarmText').style.color = isChecked ? 'gray' : 'red';
            });
        });
        
        // 表单提交处理
        document.getElementById('coordForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const formData = new FormData(this);
            const data = Object.fromEntries(formData.entries());
            
            fetch('/update_coords', {
                method: 'POST',
                body: JSON.stringify(data),
                headers: {
                    'Content-Type': 'application/json'
                }
            })
            .then(response => response.text())
            .then(message => {
                alert(message);
                loadCoordinates(); // 重新加载更新后的坐标
            })
            .catch(error => {
                alert('保存失败: ' + error);
            });
        });
        
        // 定期刷新
        setInterval(updateSystemInfo, 5000);
        window.onload = function() {
            updateSystemInfo();
            loadTextContent();
            loadCoordinates();
            loadAlarmStatus();
        };
    </script>
</body>
</html>"""

    # 创建套接字
    addr = socket.getaddrinfo('0.0.0.0', 80)[0][-1]
    s = socket.socket()
    s.bind(addr)
    s.listen(1)
    print('Web服务器运行中，访问: http://' + ip + '/')

    while True:
        try:
            cl, addr = s.accept()
            print('客户端连接来自:', addr)
            request = cl.recv(1024)
            request = str(request)
            
            # 解析请求路径
            try:
                path = request.split(' ')[1]
            except:
                path = '/'
            
            # 处理不同路径的请求
            if path == '/':
                # 返回HTML页面
                cl.send('HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n')
                cl.send(html)
            elif path == '/text_content':
                # 返回文本内容
                try:
                    with file_lock:
                        f = open('coordinates.txt', 'r')
                        content = f.read()
                        f.close()
                        cl.send('HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n')
                        cl.send(content)
                except Exception as e:
                    cl.send('HTTP/1.1 404 Not Found\r\n\r\n')
                    print('文本文件未找到:', e)
            elif path == '/download.txt':
                # 下载文本文件
                try:
                    with file_lock:
                        f = open('coordinates.txt', 'r')
                        content = f.read()
                        f.close()
                        cl.send('HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Disposition: attachment; filename=data.txt\r\n\r\n')
                        cl.send(content)
                except Exception as e:
                    cl.send('HTTP/1.1 404 Not Found\r\n\r\n')
                    print('文本文件未找到:', e)
            elif path == '/image.jpg':
                # 返回图片
                try:
                    with img_lock:
                        f = open('baidu_map.jpg', 'rb')
                        cl.send('HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\n\r\n')
                        cl.send(f.read())
                        f.close()
                except Exception as e:
                    cl.send('HTTP/1.1 404 Not Found\r\n\r\n')
                    print('图片文件未找到:', e)
            elif path == '/download.jpg':
                # 下载图片
                try:
                    with img_lock:
                        f = open('baidu_map.jpg', 'rb')
                        cl.send('HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Disposition: attachment; filename=image.jpg\r\n\r\n')
                        cl.send(f.read())
                        f.close()
                except Exception as e:
                    cl.send('HTTP/1.1 404 Not Found\r\n\r\n')
                    print('图片文件未找到:', e)
            elif path == '/coordinates':
                # 返回坐标数据
                try:
                    response = {
                        "site1_x": site1_x,
                        "site1_y": site1_y,
                        "site2_x": site2_x,
                        "site2_y": site2_y
                    }
                    cl.send('HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n')
                    cl.send(str(response).replace("'", '"'))
                except Exception as e:
                    cl.send('HTTP/1.1 500 Internal Server Error\r\n\r\n')
                    print('获取坐标失败:', e)
            elif path == '/update_coords':
                # 更新坐标
                try:
                    # 提取POST数据
                    start_idx = request.find('{')
                    end_idx = request.rfind('}')  # 从后向前找最后一个 }
    
                    if start_idx != -1 and end_idx != -1 and end_idx > start_idx:
                        json_data = request[start_idx:end_idx+1]
                        #print("提取的JSON:", json_data)
                    # 解析JSON数据
                    data = json.loads(json_data)
                    
                    # 更新全局变量
                    site1_x = float(data.get('site1_x', site1_x))
                    site1_y = float(data.get('site1_y', site1_y))
                    site2_x = float(data.get('site2_x', site2_x))
                    site2_y = float(data.get('site2_y', site2_y))
                    
                    cl.send('HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n')
                    cl.send('坐标已成功更新')
                    
                    # 可选：将更新后的坐标写入文件备份
                    '''try:
                        with file_lock:
                            with open('coordinates.txt', 'w') as f:
                                f.write(f"{site1_x},{site1_y},{site2_x},{site2_y}")
                    except Exception as e:
                        print('备份坐标到文件失败:', e)'''
                except Exception as e:
                    cl.send('HTTP/1.1 500 Internal Server Error\r\n\r\n')
                    print('更新坐标失败:', e)
            elif path == '/alarm_status':
                # 返回报警状态
                try:
                    response = {
                        "alarm": alarm_triggered
                    }
                    cl.send('HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n')
                    cl.send(str(response).replace("'", '"'))
                except Exception as e:
                    cl.send('HTTP/1.1 500 Internal Server Error\r\n\r\n')
                    print('获取报警状态失败:', e)
            elif path == '/set_alarm':
                # 设置报警状态
                try:
                    # 提取POST数据
                    start_idx = request.find('{')
                    end_idx = request.rfind('}')  # 从后向前找最后一个 }
    
                    if start_idx != -1 and end_idx != -1 and end_idx > start_idx:
                        json_data = request[start_idx:end_idx+1]
                    
                    # 解析JSON数据
                    data = json.loads(json_data)
                    print(data)
                    # 更新全局变量
                    alarm_triggered = bool(data.get('alarm', False))
                    print(alarm_triggered)
                    cl.send('HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n')
                    cl.send(f'报警状态已设置为: {"启用" if alarm_triggered else "未启用"}')
                    
                    # 可选：将报警状态写入文件备份
                    try:
                        with file_lock:
                            with open('alarm_status.txt', 'w') as f:
                                f.write('1' if alarm_triggered else '0')
                    except Exception as e:
                        print('备份报警状态到文件失败:', e)
                except Exception as e:
                    cl.send('HTTP/1.1 500 Internal Server Error\r\n\r\n')
                    print('设置报警状态失败:', e)
            elif path == '/system_info':
                # 返回系统信息
                try:
                    import os
                    # 获取文件系统信息
                    fs_stat = os.statvfs('/')
                    total = fs_stat[0] * fs_stat[2]
                    free = fs_stat[0] * fs_stat[3]
                    used = total - free
                    storage_info = f"已用: {used/1024:.1f}KB, 空闲: {free/1024:.1f}KB, 总计: {total/1024:.1f}KB"
                    
                    rtc = RTC()
                    timestamp = rtc.datetime()
                    time_str = f"{timestamp[0]}-{timestamp[1]:02d}-{timestamp[2]:02d} {timestamp[4]:02d}:{timestamp[5]:02d}:{timestamp[6]:02d}"
                    
                    response = {
                        "time": time_str,
                        "storage": storage_info,
                        "alarm": alarm_triggered
                    }
                    
                    cl.send('HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n')
                    cl.send(str(response).replace("'", '"'))
                except Exception as e:
                    cl.send('HTTP/1.1 500 Internal Server Error\r\n\r\n')
                    print('获取系统信息失败:', e)
            else:
                # 未知路径
                cl.send('HTTP/1.1 404 Not Found\r\n\r\n')
                cl.send('<h1>404 Not Found</h1>')
            
            cl.close()
        except Exception as e:
            print('处理请求时出错:', e)
            try:
                cl.close()
            except:
                pass
        time.sleep(1)

ip = connect_wifi()
'''
lng,lat = convert_coordinate(114.393356,30.525217)
IMAGE_URL = f"http://api.map.baidu.com/staticimage/v2?ak={ak}&center={lng},{lat}&width=320&height=240&zoom=15&markers={lng},{lat}&markerStyles=l,,0xFF0000"

get_baidu_map_image(IMAGE_URL)'''

# 启动串口监听线程
_thread.start_new_thread(serial_monitor, ())
    
# 启动Web服务器线程
_thread.start_new_thread(web_server, ())
    
_thread.start_new_thread(get_aliyun_data,())
    
# 主线程可以执行其他任务或保持运行
print("ESP32 Server started successfully!")
while True:
    if uart1.any():
        data1 = uart1.read().decode('utf-8')
        print(data1)
        if data1 == 'a\r\n':
            send_email("Subject:语音求助")
            print('OK')
    if uart2.any():
        data2 = uart2.read().decode()
        print(data2)
        if data2 == 'SOS1: Fall\n':
            send_email("Subject:摔倒报警")
            uart1.write('fall')
        elif data2 == "SOS2: Hand \n":
            send_email("Subject:手势求助")
            uart1.write('gesture')
        else:
            pass
    time.sleep(1) 