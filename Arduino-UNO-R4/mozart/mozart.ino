/**
 * 项目：Arduino Shell 作者：qing 版本：0.0.1
 *
 * 此代码使得ARDUINO微控制器重复地接收一个串口命令，然后返回此命令的结果。
 * 可以通过回调数组来模块化地添加指令处理方案。
 * 
 * 消息的接收在某些情况下可能会失灵，需要多次尝试。
 * 
 *
 * Project: Arduino Shell Author: qing
 * This code enables the ARDUINO microcontroller to continuously receive serial commands and return the corresponding results.
 * Command handling solutions can be modularly added through a callback array.
 *
 * ---------------------------
 * ---------------------------
 * 版本：0.0.2
 *
 * 因为串口的抽象级别比较低，新版本我们将采用网口进行通信。
 * 初步拟定使用交换机进行组网（静态IP），使用HTTP协议建立服务。
 * 
 * ---------------------------
 * ---------------------------
 * 版本：0.0.3
 *
 * 使COPS设备能够通过API被设置，并且具有欢迎页面
 *
 *  curl -X POST http://192.168.254.101:8001/api -d '{"cmd": "MPU6050"}'
 * ---------------------------
 * ---------------------------
 * 版本：0.0.4
 *
 * 重构项目，使用SH_MA类进行匹配
 * 
 * ---------------------------
 * ---------------------------
 * 版本：0.0.5
 * 可以展示可用方法
 * ---------------------------
 * ---------------------------
 * 版本：0.0.6
 * 能够访问SD卡文件
 * ---------------------------
 * ---------------------------
 * 版本：0.0.7
 * 能够修改SD卡文件
 * eg. curl -X POST http://192.168.254.101:8002/api/remove -d '{"filename": "xxx.htm"}'
 * 
 * FIXME: 流式写文件
 * ---------------------------
 * ---------------------------
 * 版本：0.1.0
 * 删除了固态传感器驱动，但是目前动态驱动仅有一个
 *
 * 项目模块化、增加了 TFT 模块
 * 
 * 
 * ---------------------------
 * ---------------------------
 * 版本：0.1.1
 *
 * 进一步模块化
 * 
 * ---------------------------
 * ---------------------------
 */

/*
 * 这些是此项目的头文件
 */
#include "prj.h"
#include <vector>


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/*
 * 这是来自官方的库对象
 */
static EthernetServer server(80);         /* 标准http端口 */
                                          /* （如果微控制器没找到dhcp服务，使用固定的ip地址） */


/*
 * 这些是原生库对象
 */
qing::Prj_Mozart  prj;                            /* 莫扎特项目 */
qing::Sh_ma       sh;                             /* 命令行模式对象*/

qing::Out         out       = qing::Out(SERIAL_RATE); /* 输出对象 */
qing::I2c         i2c;                                /* I2C对象 */
qing::Elk         elk;                                /* 脚本语言对象 */
//qing::Tft         tft;                                /* TFT屏幕 */
qing::Fs          fs        = qing::Fs(SD_PIN);       /* 文件系统 */
qing::Eth         eth       = qing::Eth(ETH_PIN);     /* 以太网 */
qing::Oled        oled;                               /* OLED屏幕 */

qing::MPU6050     mpu6050;                                        /* 加速度和温度传感器 */
qing::SGP30       sgp30;                                          /* 空气质量传感器 */
qing::SHT3X       sht3x;                                          /* 温湿度传感器 */
qing::BME280      bme280    = qing::BME280(qing::BME280::ADDR0);  /* 温湿度大气压传感器 */
qing::BH1750      bh1750;                                         /* 光照传感器 */
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/* 面向过程 - 抽象层 */

/*
 * 死循环
 */
void loop_forever() {
  while(true){
    delay(1000);    
  }
}

/*
 * 自定义异常类
 *
 * Arduino IDE 不支持throw
 */
class Arduino_Exception {
private:
  String msg;
public:
  Arduino_Exception(String msg) {
    this->msg = msg;
  }
  String what() {
    return msg;
  }
};







/*
 * 这个函数会在启动后运行一次
 */
void setup() {
  /* 启动一组模块 */
  prj.init_and_add(out, out);
  prj.init_and_add(i2c, out);
  //prj.init_and_add(tft, out);
  prj.init_and_add(elk, out);
  prj.init_and_add(fs,  out);
  prj.init_and_add(eth, out);
  prj.init_and_add(oled, out);

  /* 显示获取到的IP地址 */
  if (eth.is_running()) {
    eth.print_local_ip();
  }
  else {
    oled.print("Ethernet not connected\n", 2);
  }

  /* 启动一组匹配命令 */
  sh.init_and_add(mpu6050, out);
  sh.init_and_add(sgp30,   out);
  sh.init_and_add(sht3x,   out);
  sh.init_and_add(bme280,  out);
  sh.init_and_add(bh1750,  out);

  /* 项目启动 */
  out.print("Project " + PRJ_NAME + " start!\n", 1);

}

/*
 * 这个函数会在setup()之后被反复调用
 */
void loop() {

  //read_from_serial();

  if (eth.is_running()) {
    listen_http();
  }

  else {
    out.print("Nothing to do...\n", 1);
    loop_forever();
  }

}





















////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

/*
 * 递归创建目录函数
 */
void createDirectoryRecursive(String path) {
  // 检查路径是否已经存在
  if (SD.exists(path)) {
    return;  // 路径已存在，无需创建
  }
  
  // 检查父目录是否存在
  int lastSlashIndex = path.lastIndexOf('/');
  if (lastSlashIndex > 0) {
    // 递归创建父目录
    String parentPath = path.substring(0, lastSlashIndex);
    createDirectoryRecursive(parentPath);
  }
  
  // 创建当前目录
  SD.mkdir(path);
}

/*
 * 监听http请求
 */
void listen_http() {
  EthernetClient client = server.available();

  if (client) {
    out.print("\nNew client connected\n", 0);

    String currentLine = "";
    String requestMethod = "";
    String requestPath = "";
    String requestData = "";  // 重命名变量，使其适用于POST和DELETE
    int contentLength = 0;
    unsigned long connectionStart = millis();

    while (client.connected() && (millis() - connectionStart < TIMEOUT_MS)) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n') {
          // 处理GET方法
          if (currentLine.startsWith("GET")) {
            int firstSpace = currentLine.indexOf(' ');
            int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
            if (secondSpace > firstSpace) {
              requestMethod = currentLine.substring(0, firstSpace);
              requestPath = currentLine.substring(firstSpace + 1, secondSpace);
            }
          } 
          // 处理POST方法
          else if (currentLine.startsWith("POST")) {
            int firstSpace = currentLine.indexOf(' ');
            int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
            if (secondSpace > firstSpace) {
              requestMethod = currentLine.substring(0, firstSpace);
              requestPath = currentLine.substring(firstSpace + 1, secondSpace);
            }
          } 
          // 新增：处理DELETE方法
          else if (currentLine.startsWith("DELETE")) {
            int firstSpace = currentLine.indexOf(' ');
            int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
            if (secondSpace > firstSpace) {
              requestMethod = currentLine.substring(0, firstSpace);
              requestPath = currentLine.substring(firstSpace + 1, secondSpace);
            }
          } 
          // 处理Content-Length头（适用于POST和DELETE）
          else if (currentLine.startsWith("Content-Length: ")) {
            contentLength = currentLine.substring(16).toInt();
          } 
          // 空行表示请求头结束，开始处理请求体
          else if (currentLine.length() == 0) {
            // 读取请求数据（适用于POST和DELETE）
            if ((requestMethod == "POST" || requestMethod == "DELETE") && contentLength > 0) {
              unsigned long dataStart = millis();
              // 等待数据到达，超时时间2秒
              while (client.available() < contentLength && (millis() - dataStart < 2000)) {
                delay(10);
              }

              // 读取指定长度的数据
              for (int i = 0; i < contentLength; i++) {
                if (client.available()) {
                  requestData += (char)client.read();
                } else {
                  out.print("Error: Data incomplete\n", 0);
                  break;
                }
              }
            }

            // 发送响应
            sendResponse(client, requestMethod, requestPath, requestData);
            break;
          }
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    client.stop();
    out.print("Client disconnected\n", 0);
  }
}


void sendResponse(EthernetClient &client, const String &method,
                  const String &path, const String &data) {
  
  if (method == "GET" && fs.isWorking() && DEBUG) {
    String filename = path;
    if (filename.startsWith("/")) {
      filename = filename.substring(1);
    }
    
    // 尝试打开文件或目录
    File file = SD.open(filename);
    if (file) {
      // 检查是否是目录
      if (file.isDirectory()) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        
        // 生成目录列表HTML
        client.println("<!DOCTYPE html><html><head><title>Directory Listing</title>");
        client.println("<style>body { font-family: Arial, sans-serif; margin: 20px; }");
        client.println("ul { list-style-type: none; padding: 0; }");
        client.println("li { margin: 5px 0; }");
        client.println("a { text-decoration: none; color: #0366d6; }");
        client.println("a:hover { text-decoration: underline; }</style></head>");
        client.println("<body><h1>Directory: " + path + "</h1><ul>");
        
        // 添加父目录链接（如果不是根目录）
        if (path != "/") {
          String parentPath = path;
          int lastSlash = parentPath.lastIndexOf('/');
          if (lastSlash > 0) {
            parentPath = parentPath.substring(0, lastSlash);
          } else {
            parentPath = "/";
          }
          client.println("<li><a href=\"" + parentPath + "\">../</a></li>");
        }
        
        // 列出目录内容
        while (true) {
          File entry = file.openNextFile();
          if (!entry) {
            break;
          }
          
          String entryName = entry.name();
          // 直接使用文件名，不要尝试移除前缀
          // 这样可以确保文件名保持完整，不会错误地移除与文件夹名相同的前缀
          
          if (entryName.length() > 0) {
            String entryPath = path;
            if (!entryPath.endsWith("/")) {
              entryPath += "/";
            }
            entryPath += entryName;
            
            client.print("<li><a href=\"");
            client.print(entryPath);
            client.print("\">");
            client.print(entryName);
            
            if (entry.isDirectory()) {
              client.print("/");
            }
            
            client.print("</a>");
            
            if (!entry.isDirectory()) {
              client.print(" (");
              client.print(entry.size());
              client.print(" bytes)");
            }
            
            client.println("</li>");
          }
          entry.close();
        }
        
        client.println("</ul></body></html>");
        file.close();
      } else {
        // 是文件，按原方式处理
        // 根据文件扩展名设置Content-Type
        String contentType = "text/plain";
        String lowerFilename = filename;
        lowerFilename.toLowerCase();
        
        if (lowerFilename.endsWith(".html") || lowerFilename.endsWith(".htm") || 
            lowerFilename.indexOf(".htm") != -1) {
          contentType = "text/html";
        } else if (lowerFilename.endsWith(".css")) {
          contentType = "text/css";
        } else if (lowerFilename.endsWith(".js")) {
          contentType = "application/javascript";
        } else if (lowerFilename.endsWith(".png")) {
          contentType = "image/png";
        } else if (lowerFilename.endsWith(".jpg") || lowerFilename.endsWith(".jpeg")) {
          contentType = "image/jpeg";
        } else if (lowerFilename.endsWith(".gif")) {
          contentType = "image/gif";
        }
        
        client.println("HTTP/1.1 200 OK");
        client.print("Content-Type: ");
        client.println(contentType);
        //client.println("; charset=utf-8");  /* 添加字符集 */
        client.println("Connection: close");
        client.println();
        
        // 发送文件内容
        while (file.available()) {
          client.write(file.read());
        }
        file.close();
      }
    } else {
      // 文件不存在，返回404
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.print("File not found: ");
      client.print(filename);
    }
  }
  else if (method == "POST") {
    // 处理 /show POST请求
    if (path == "/api/show") {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      String msg = sh.show();
      client.print(msg);
    }
    // 处理 /api/create POST请求 - 创建空白文件或覆盖为空白
    else if (path == "/api/create" && fs.isWorking() && DEBUG) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" && json.hasOwnProperty("filename")) {
        
        String filename = json["filename"];
        
        // 确保文件名不以斜杠开头
        if (filename.startsWith("/")) {
          filename = filename.substring(1);
        }
        
        // 确保目标目录存在
        int lastSlashIndex = filename.lastIndexOf('/');
        if (lastSlashIndex > 0) {
          String directory = filename.substring(0, lastSlashIndex);
          // 递归创建目录
          createDirectoryRecursive(directory);
        }
        
        // 如果文件已存在，先删除它
        if (SD.exists(filename)) {
          SD.remove(filename);
        }
        
        // 创建空白文件
        File file = SD.open(filename, FILE_WRITE);
        if (file) {
          file.close();
          client.print("{\"status\":\"success\",\"message\":\"Empty file created successfully\",\"filename\":\"");
          client.print(filename);
          client.print("\"}");
        } else {
          client.print("{\"status\":\"error\",\"message\":\"Failed to create file\",\"filename\":\"");
          client.print(filename);
          client.print("\"}");
        }
      } else {
        client.print("{\"status\":\"error\",\"message\":\"Invalid JSON format. Required field: filename\"}");
      }
    }
    // 处理 /api/append POST请求 - 在文件后面追加内容
    else if (path == "/api/append" && fs.isWorking() && DEBUG) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" 
            && json.hasOwnProperty("filename") 
            && json.hasOwnProperty("content")) {
        
        String filename = json["filename"];
        String content = json["content"];
        
        // 确保文件名不以斜杠开头
        if (filename.startsWith("/")) {
          filename = filename.substring(1);
        }
        
        // 确保目标目录存在（如果文件不存在需要创建）
        int lastSlashIndex = filename.lastIndexOf('/');
        if (lastSlashIndex > 0) {
          String directory = filename.substring(0, lastSlashIndex);
          // 递归创建目录
          createDirectoryRecursive(directory);
        }
        
        // 打开文件进行追加写入（如果文件不存在，FILE_WRITE会创建它）
        File file = SD.open(filename, FILE_WRITE);
        if (file) {
          // 移动到文件末尾（尽管FILE_WRITE模式默认应该就在末尾）
          file.seek(file.size());
          
          size_t bytesWritten = file.print(content);
          file.close();
          
          if (bytesWritten > 0) {
            client.print("{\"status\":\"success\",\"message\":\"Content appended successfully\",\"filename\":\"");
            client.print(filename);
            client.print("\",\"bytesWritten\":");
            client.print(bytesWritten);
            client.print("}");
          } else {
            client.print("{\"status\":\"error\",\"message\":\"Failed to append content\",\"filename\":\"");
            client.print(filename);
            client.print("\"}");
          }
        } else {
          client.print("{\"status\":\"error\",\"message\":\"Failed to open file for appending\",\"filename\":\"");
          client.print(filename);
          client.print("\"}");
        }
      } else {
        client.print("{\"status\":\"error\",\"message\":\"Invalid JSON format. Required fields: filename, content\"}");
      }
    }
    // 处理 /api/filesystem POST请求 - 保存文件
    else if (path == "/api/upload" && fs.isWorking() && DEBUG) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" 
            && json.hasOwnProperty("filename") 
            && json.hasOwnProperty("content")) {
        
        String filename = json["filename"];
        String content = json["content"];
        
        // 确保文件名不以斜杠开头
        if (filename.startsWith("/")) {
          filename = filename.substring(1);
        }
        
        // 如果文件已存在，先删除它（实现覆盖写入）
        if (SD.exists(filename)) {
          SD.remove(filename);
        }
        
        // 尝试打开文件进行写入（如果文件不存在，FILE_WRITE会创建它）
        File file = SD.open(filename, FILE_WRITE);
        if (file) {
          size_t bytesWritten = file.print(content);
          file.close();
          
          if (bytesWritten == content.length()) {
            client.print("{\"status\":\"success\",\"message\":\"File saved successfully\",\"filename\":\"");
            client.print(filename);
            client.print("\",\"bytesWritten\":");
            client.print(bytesWritten);
            client.print("}");
          } else {
            client.print("{\"status\":\"error\",\"message\":\"Failed to write all content\",\"filename\":\"");
            client.print(filename);
            client.print("\",\"expected\":");
            client.print(content.length());
            client.print(",\"actual\":");
            client.print(bytesWritten);
            client.print("}");
          }
        } else {
          client.print("{\"status\":\"error\",\"message\":\"Failed to open file for writing\",\"filename\":\"");
          client.print(filename);
          client.print("\"}");
        }
      } else {
        client.print("{\"status\":\"error\",\"message\":\"Invalid JSON format. Required fields: filename, content\"}");
      }
    }

    // 处理 /api/sensor POST请求（保持原有功能）
    else if (path == "/api/sensor") {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" 
            && json.hasOwnProperty("cmd")) {
        String cmd = json["cmd"];
        String res = sh.process(cmd);
        client.print(res);
      } else {
        client.print("ERR");
      }
    }

    // 处理 /api/extension POST请求
    /* FIXME: 应该还可以扩展 */
    else if (path == "/api/extension") {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();

      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object"
            && json.hasOwnProperty("addr")
            && json.hasOwnProperty("cmd")
            && i2c.is_running()) {
              
// 定义命令类型
#define CMD_GET_STATUS 0x01
#define CMD_SET_VALUE 0x02
#define CMD_GET_VALUE 0x03
#define CMD_GET_VERSION 0x04
#define CMD_RESET_DEVICE 0x05

        int addr = json["addr"];
        String cmd = json["cmd"];
        JSONVar jsonRes;
        
        if (cmd == String("get")) {
          // 通过WIRE直接发送命令并读取响应
          Wire.beginTransmission(addr);
          Wire.write(CMD_GET_VALUE); // 发送获取值命令
          Wire.endTransmission();
          
          // 等待设备准备好响应
          delay(10);
          
          String res = "No response";
          // 读取响应
          int bytesRead = Wire.requestFrom(addr, 1+sizeof(int));
          if (bytesRead == 1+sizeof(int)) {
            byte b[sizeof(int)];
            for (int i = 0; i < sizeof(int); i++) {
              b[i] = Wire.read();
            }
            // 将响应转换为数字（假设大端格式）
            int value = (b[0] << 8) | b[1]; // 大端格式
            res = String(value);
          }
          jsonRes["msg"] = "OK";
          jsonRes["data"] = res;
          client.print(JSON.stringify(jsonRes));
        } else if (cmd == String("set") && json.hasOwnProperty("value")) {
          // 处理set命令
          int value = json["value"];
          
          // 通过WIRE直接发送命令
          Wire.beginTransmission(addr);
          Wire.write(CMD_SET_VALUE); // 发送设置值命令
          Wire.write(static_cast<uint8_t>(value)); // 发送值
          Wire.endTransmission();
          
          // 等待设备准备好响应
          delay(10);
          
          // 读取响应确认
          int bytesRead = Wire.requestFrom(addr, 1);
          if (bytesRead == 1) {
            byte response = Wire.read();
            if (response == 0x00) {
              jsonRes["msg"] = "OK";
              jsonRes["data"] = "Set success";
            } else {
              jsonRes["msg"] = "ERROR";
              jsonRes["data"] = "Set failed";
            }
          } else {
            jsonRes["msg"] = "ERROR";
            jsonRes["data"] = "No response";
          }
          client.print(JSON.stringify(jsonRes));
        } else {
          jsonRes["msg"] = "ERROR";
          jsonRes["data"] = "Invalid command " + json["cmd"];
          client.print(JSON.stringify(jsonRes));
        }
      } else {
        JSONVar jsonRes;
        jsonRes["msg"] = "ERROR";
        jsonRes["data"] = "Invalid request";
        client.print(JSON.stringify(jsonRes));
      }
    }

    else if (path == "/api/show_ext" && i2c.is_running() && DEBUG) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();

      // 创建json数组
      JSONVar jsonArray;
      std::vector<std::pair<String, String>> deviceList = i2c.Scan();
      for (int i = 0; i < deviceList.size(); i++) {
        // 创建设备对象
        JSONVar deviceObj;
        deviceObj["id"] = deviceList[i].first;
        deviceObj["version"] = deviceList[i].second;
        jsonArray[i] = deviceObj;
      }
      client.print(JSON.stringify(jsonArray));
    }

    // 处理 /api/list POST请求 - 列出文件夹中的所有文件
    else if (path == "/api/list" && fs.isWorking() && DEBUG) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" && json.hasOwnProperty("path")) {
        String dirPath = json["path"];
        
        // 确保路径不以斜杠开头
        if (dirPath.startsWith("/")) {
          dirPath = dirPath.substring(1);
        }
        
        // 尝试打开目录
        File dir = SD.open(dirPath);
        if (dir && dir.isDirectory()) {
          // 开始构建JSON数组
          client.print("[");
          
          bool firstEntry = true;
          while (true) {
            File entry = dir.openNextFile();
            if (!entry) {
              break;  // 没有更多文件
            }
            
            // 添加逗号分隔符（除了第一个条目）
            if (!firstEntry) {
              client.print(",");
            }
            firstEntry = false;
            
            // 构建文件信息JSON对象
            client.print("{");
            client.print("\"name\":\"");
            // 转义文件名中的特殊字符
            String fileName = entry.name();
            for (int i = 0; i < fileName.length(); i++) {
              char c = fileName.charAt(i);
              switch (c) {
                case '\\': client.print("\\\\"); break;
                case '\"': client.print("\\\""); break;
                case '\n': client.print("\\n"); break;
                case '\r': client.print("\\r"); break;
                case '\t': client.print("\\t"); break;
                default: client.print(c);
              }
            }
            client.print("\",\"isDirectory\":");
            client.print(entry.isDirectory() ? "true" : "false");
            
            // 如果是文件，添加大小信息
            if (!entry.isDirectory()) {
              client.print(",\"size\":");
              client.print(entry.size());
            }
            
            client.print("}");
            entry.close();
          }
          
          // 结束JSON数组
          client.print("]");
          dir.close();
        } else {
          // 目录不存在或不是目录
          client.print("{\"status\":\"error\",\"message\":\"Invalid directory path\"}");
        }
      } else {
        // 无效的JSON格式
        client.print("{\"status\":\"error\",\"message\":\"Invalid JSON format. Required field: path\"}");
      }
    }
    
    /* 新的传感器消息获取函数，采用脚本，需要sd卡 和 js解释器 */
    else if (path == "/api/sensors" && fs.isWorking() && elk.is_running()) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" 
            && json.hasOwnProperty("path")) {
        String path = json["path"];
        
        String filename = path;
        if (filename.startsWith("/")) {
          filename = filename.substring(1);
        }
        
    
        // 尝试打开文件或目录
        File file = SD.open(filename);
        if (file && !file.isDirectory()) {
          // 发送文件内容
          String cmd = "";
          while (file.available()) {
            cmd += char(file.read());
          }
          file.close();
          String res =elk.test(cmd);
          client.print(res);
        } else {
          client.print("ERR FILETYPE");
        }
      } else {
        client.print("ERR OPENDING");
      }
    }


    // 处理未知的POST路径
    else {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.print("POST endpoint not found: ");
      client.print(path);
    }
    

    
    out.print("\nReceived POST:\n", 0);
    out.print("Path: ", 0);
    out.print(path + "\n", 0);
    out.print("Data: ", 0);
    out.print(data + "\n", 0);
  }
  // 新增：处理DELETE请求，用于删除文件
  else if (method == "DELETE") {
    // 处理 /api/remove 请求
    if (path == "/api/remove" && fs.isWorking()  && DEBUG) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      
      // 解析JSON数据获取要删除的文件名
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" && json.hasOwnProperty("filename")) {
        String filename = json["filename"];
        
        // 确保文件名不以斜杠开头
        if (filename.startsWith("/")) {
          filename = filename.substring(1);
        }
        
        // 检查文件是否存在
        if (SD.exists(filename)) {
          // 尝试删除文件
          if (SD.remove(filename)) {
            client.print("{\"status\":\"success\",\"message\":\"File deleted successfully\",\"filename\":\"");
            client.print(filename);
            client.print("\"}");
            
            out.print("Deleted file: ", 0);
            out.print(filename + "\n", 0);
          } else {
            client.print("{\"status\":\"error\",\"message\":\"Failed to delete file\",\"filename\":\"");
            client.print(filename);
            client.print("\"}");
            
            out.print("Failed to delete file: ", 0);
            out.print(filename + "\n", 0);
          }
        } else {
          client.print("{\"status\":\"error\",\"message\":\"File not found\",\"filename\":\"");
          client.print(filename);
          client.print("\"}");
          
          out.print("File not found for deletion: ", 0);
          out.print(filename + "\n", 0);
        }
      } else {
        client.print("{\"status\":\"error\",\"message\":\"Invalid JSON format. Required field: filename\"}");
      }
    }
    // 处理未知的DELETE路径
    else {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.print("DELETE endpoint not found: ");
      client.print(path);
    }
    
    out.print("\nReceived DELETE:\n", 0);
    out.print("Path: ", 0);
    out.print(path + "\n", 0);
    out.print("Data: ", 0);
    out.print(data + "\n", 0);
  }
  // 处理不支持的HTTP方法
  else {
    out.print("HTTP/1.1 405 Method Not Allowed\n", 0);
    out.print("Content-Type: text/plain\n", 0);
    out.print("Connection: close\n", 0);
    out.print("\n", 0);
    out.print("Method not supported: ", 0);
    out.print(method, 0);
  }
}


