#include <Wire.h>


// 定义I2C从机地址，可根据需要修改
#define SLAVE_ADDRESS 0x01

// 定义命令类型
#define CMD_GET_STATUS 0x01  /* status 状态 */
#define CMD_SET_VALUE 0x02   /* set value 设置值 */
#define CMD_GET_VALUE 0x03   /* get value 获取值 */
#define CMD_RESET_DEVICE 0x05  /* reset 重置设备 */

// FIXME: 改为动态设置引脚
#define READ_PIN A0  /* the pin to read 读取的引脚 */
#define WRITE_PIN 2  /* the pin to write 写的引脚 */



////////////////////////////////////

// 存储接收到的命令和参数
byte receivedCommand = 0;
byte receivedParams[4];
int paramsCount = 0;

// 响应数据缓冲区
byte responseBuffer[4];
int responseLength = 0;

/*
 * open the water flag
 * 
 * if counter > 0, open water
 * 
 * and every second, counter--
 *
 * 开关电磁阀的计数器
 * 如果计数器大于0则打开
 * 每秒计数器减1
 */
int counter = 0;


/*---------------------------------------------------------*/
// 读取Ax模拟值并转换为电压值（单位：毫伏）
void get_data(int pin, int &out) {
  int raw = analogRead(pin);     // 读取0-1023的原始值
  out = (raw * 5000) / 1023;     // 假设参考电压5V，换算成毫伏
}

// 设置值
void set_data(int val) { 
  counter = val;
}


/*---------------------------------------------------------*/
/*---------------------------------------------------------*/
// 当主机发送数据到从机时调用
void receiveEvent(int howMany) {
  paramsCount = 0;
  
  // 接收时亮灯
  digitalWrite(LED_BUILTIN, HIGH);
  
  // 读取所有可用数据
  while (Wire.available() && paramsCount < 4) {
    if (paramsCount == 0) {
      // 第一个字节是命令
      receivedCommand = Wire.read();
    } else {
      // 后续字节是参数
      receivedParams[paramsCount - 1] = Wire.read();
    }
    paramsCount++;
  }
  
  // 处理命令
  processCommand();
  
  // 调试信息
  Serial.print("Received Command: 0x");
  Serial.println(receivedCommand, HEX);
  Serial.print("Response Length: ");
  Serial.println(responseLength);
  
  // 接收完成后熄灭LED
  digitalWrite(LED_BUILTIN, LOW);
}

/*---------------------------------------------------------*/
/*---------------------------------------------------------*/
// 处理接收到的命令
void processCommand() {
  responseLength = 0;
  
  switch (receivedCommand) {
    case CMD_GET_STATUS:
      // 返回设备状态（2字节：状态+保留字节）
      responseBuffer[0] = 0x00; // 状态：1=正常，0=异常
      responseLength = 1;
      break;
      
    case CMD_SET_VALUE:
      // 设置设备值（参数：1字节值）
      if (paramsCount >= 2) {
        int deviceValue = receivedParams[0];
        set_data(deviceValue);
        // 返回确认和新值（2字节：确认+新值）
        responseBuffer[0] = 0x00; // 0=成功
        responseBuffer[1] = deviceValue;
      } else {
        // 返回错误（2字节：错误+原命令）
        responseBuffer[0] = 0x01; // 1=参数错误
        responseBuffer[1] = receivedCommand;
      }
      responseLength = 2;
      break;
      
    case CMD_GET_VALUE:
      // 读取A0口模拟值并转换为电压值（单位：毫伏）
      int val;
      get_data(READ_PIN, val);
      // 返回设备值（2字节：状态+值）
      responseBuffer[0] = 0x00; // 0=成功
      for( int i=1; i< sizeof(responseBuffer); i++)
        responseBuffer[i] = ((byte*)&val)[i-1];
      responseLength = 2;
      break;

 
    case CMD_RESET_DEVICE:
      // 重置设备
      counter = 0;
      
      // 返回确认（1字节）
      responseBuffer[0] = 0x00; // 0=成功
      responseLength = 1;
      break;

      
    default:
      // 无效命令，返回错误（2字节：错误+原命令）
      responseBuffer[0] = 0x02; // 2=无效命令
      responseBuffer[1] = receivedCommand;
      responseLength = 2;
      break;
  }
}

/*---------------------------------------------------------*/
/*---------------------------------------------------------*/
// 当主机请求从机数据时调用
void requestEvent() {
  // 发送响应数据
  for (int i = 0; i < responseLength; i++) {
    Wire.write(responseBuffer[i]);
  }
  
  // 调试信息
  Serial.print("Sent Response: ");
  for (int i = 0; i < responseLength; i++) {
    Serial.print("0x");
    Serial.print(responseBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

/*---------------------------------------------------------*/
/*---------------------------------------------------------*/
void setup() {
  pinMode(READ_PIN,  INPUT);
  pinMode(WRITE_PIN, OUTPUT);
  
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  Serial.begin(115200);
  Serial.println("I2C Slave Device Ready");
  Serial.print("Slave Address: 0x");
  Serial.println(SLAVE_ADDRESS, HEX);
}

// 主循环
void loop() {
  if (counter > 0) { /* open water */
    digitalWrite(WRITE_PIN, HIGH);
    counter--;
  }
  else  /* close water */
    digitalWrite(WRITE_PIN, LOW);

  delay(1000); /* one second */
}
