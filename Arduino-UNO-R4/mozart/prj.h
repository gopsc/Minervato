#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>

#include <Arduino_JSON.h>
//#include <ArduinoSTL.h> /* 不知道为啥不能放这里 */
#include <vector>
//#include <DIYables_TFT_Shield.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SGP30.h>
#include <ArtronShop_SHT3x.h>
#include <Adafruit_BME280.h>
#include <ArtronShop_BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*
 * 静态常量，程序预配置
 */
static const              String PRJ_NAME    = "MOZART"; /* 本项目名称 */
static constexpr          bool   DEBUG       = true;     /* 开启调试模式 */
static constexpr unsigned long   SERIAL_RATE = 115200;   /* 串口速率 */
static constexpr          int    SD_PIN      = 4;        /* SD卡SPI片选 */
static constexpr unsigned long   FS_MAX_SIZE = 10000;    /* 最大打开文件（10k） */
static constexpr          int    ETH_PIN     = 10;       /* 以太网SPI片选  */
static constexpr unsigned long   TIMEOUT_MS  = 5000;     /* HTTP传输5秒超时 */

static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; /* 网口的mac地址 */
//static IPAddress ip(192, 168, 254, 101);                    /* 静态IP用的地址，*/
                                                            /* 没用上 */

/* 回调函数变量类型定义 */
using shell_t = String (*) (String);


namespace qing {
  /*
   * 可启动接口
   *
   * 可以初始化，可以查看是否已经初始化
   */
  class Runnable {
  public:
    virtual bool is_running() = 0;  /* 是否正在运行 */
    virtual bool begin() = 0;       /* 启动 */
  };

  /*
   * 已命名接口
   *
   * 可以展示对象的名字。
   */
  class Named {
  public:
    virtual String show() = 0;  /* 展示名字 */
  };

  /*
   * 可打印接口
   *
   * 可以输出文字
   */
  class Printable {
  public:
    virtual void print(const String msg, const int mode) = 0;   /* 打印方法， 要传入打印模式 */
  };

  /* 
   * 匹配模式抽象基类
   *
   * 可进行匹配
   */
  class Sh_match: public Named, public Runnable {
    public:
      virtual String test(String param) =  0; /* 进行匹配 */
  };


  /*
   * ARDUINO 命令行类型
   * 可自由添加回调函数
   * 
   * 20250513:
   * 基本上就是用一个数组（或向量）来进行命令的保存
   * 然后遍历所有命令，直到遇到返回值为true的。
   *
   * （！）按照约定，返回值为true时该命令已被执行
   *
   *
   * Shell模式组类
   *
   * 该类存储一组模式匹类
   * 可以展示有哪些模式匹配类
   * 可以挨个匹配，停止在第一个有返回值的模式
   * 并返回结果
   *
   */
   class Sh_ma {
      public:
        /*
         * 直接添加模块
         */
        void add(Sh_match *ma) {
          vec.push_back(ma);
        }
        /*
        * 运行并添加模块
        */
        bool init_and_add(Sh_match& ma, Printable& out) {
          out.print("Init ", 1);    /* 打印模块的提示 */
          out.print(ma.show(), 1);
          out.print("...", 1);
          if (ma.begin()) {
            out.print("done\n", 1);
            this->add(&ma);
          }
          else {
            out.print("failed\n", 1);
          }
        }
        /* 
         * 执行匹配
         */
        String process(String cmd) {
          for (auto i=vec.begin(); i!=vec.end(); ++i) {
            String res = (*i)->test(cmd);
            if (res != "") return res;
          }
          /* 未能匹配 */
          return "Unkown Command: " + cmd;
        }

        /*
         * 展示所有的模块
         *
         * FIXME: 使用vector而不是JSON
         */
        String show() {
          JSONVar arr;
          int j = 0;
          for (auto i=vec.begin(); i!=vec.end(); ++i) {
           arr[j] =  (*i)->show();
           ++j;
          }

          // 返回
          return JSON.stringify(arr);
        }
      private:
        std::vector<Sh_match*> vec; /* 向量容器 */
   };


  /* 
   * 系统模块类
   */
  class  Sys_module: public Named, public Runnable {};


  /*
   * 项目类型
   *
   * 内部可添加模块
   */
  class Prj_Mozart {
  private:
    std::vector<Sys_module*> vec;   /* 用于存储系统模块的向量 */
  public:
    /*
     * 直接添加模块
     */
    void add(Sys_module& mod) {
      vec.push_back(&mod);
    }
    /*
     * 初始化并添加模块
     */
    bool init_and_add(Sys_module& mod, Printable& out) {
      out.print("Init ", 1);    /* 展示模块名 */
      out.print(mod.show(), 1);
      out.print("...", 1);
      if (mod.begin()) {
        out.print("done\n", 1);
        this->add(mod);
      }
      else {
        out.print("failed\n", 1);
      }
    }
  };


  /*
   * 输出模块
   *
   * 默认从串口输出，如果屏幕模块打开了，就拥有展示模式
   *
   * FIXME: 不依赖屏幕模块
   */
  class Out: public Sys_module, public Printable {
  private:
    String _NAME = "Out";       /* 存储模块名称 */
    bool out_has_init = false;  /* 存储模块启动状态 */
    unsigned long serial_rate;  /* 存储串口速率 */
  public:

    /*
     * 构造函数，需要输入串口速率
     */
    Out(unsigned long serial_rate) {
      this->serial_rate = serial_rate;
    }

    /*
     * 输出模块是否正在运行
     */
    bool is_running() override {
      return out_has_init;
    }

    /*
     * 展示模块名称
     */
    String show() override {
      return _NAME;
    }

    /* 
     * 初始化模块
     */
    bool begin() override {
      Serial.begin(serial_rate);
      return (out_has_init = true);
    }


    /*
     * 字符串输出，默认输出到串口
     *
     * - 0: 串口
     * - 1: TFT
     * - 2: OLED
     *
     * FIXME: 类别使用枚举
     */
    void print(const String msg, const int mode = 0) override;


  };




  /*
   * i2c 模块
   *
   * FIXME: 封装i2c操作
   */
  class I2c: public Sys_module {
  private:
    String _NAME = "I2c";       /* 模块名称 */
    bool i2c_has_init = false;  /* i2c模块初始化标志 */
    
    // 定义命令类型
    static const byte CMD_GET_VERSION = 0x04;
    
    /*
     * 将字节数组转换为字符串
     */
    void bytesToString(byte* bytes, int length, char* result, int maxResultLength) {
      // 确保结果缓冲区为空
      memset(result, 0, maxResultLength);
      
      // 复制字节到结果缓冲区，最多复制maxResultLength-1个字节，留出结尾符空间
      int copyLength = min(length, maxResultLength - 1);
      memcpy(result, bytes, copyLength);
      
      // 确保字符串以null结尾
      result[copyLength] = '\0';
    }
    
    /*
     * 从设备获取版本信息
     */
    String getDeviceVersion(byte address) {
      // 发送获取版本命令
      Wire.beginTransmission(address);
      Wire.write(CMD_GET_VERSION);
      int result = Wire.endTransmission();
      
      if (result != 0) {
        return "<无法获取版本>";
      }
      
      // 请求响应数据
      int bytesAvailable = Wire.requestFrom(address, 32);
      if (bytesAvailable <= 0) {
        return "<无法获取版本>";
      }
      
      // 读取响应数据
      byte response[32];
      int responseLength = 0;
      while (Wire.available() && responseLength < 32) {
        response[responseLength] = Wire.read();
        responseLength++;
      }
      
      // 转换为字符串
      char version[17];
      bytesToString(response, responseLength, version, sizeof(version));
      
      return String(version);
    }
  public:

    /*
     * 是否已经初始化
     */
    bool is_running() override {
      return i2c_has_init;
    }
    /*
     * 初始化板载I2C接口
     */
    bool begin() override {
      Wire.begin();
      return (i2c_has_init = true);
    }
    /*
     * 展示模块名
     */
    String show() override {
      return _NAME;
    }
    
    /*
     * 扫描I2C设备
     * 返回格式：std::vector<std::pair<String, String>>，每个元素是键值对(id: VERSION)
     */
    std::vector<std::pair<String, String>> Scan() {
      std::vector<std::pair<String, String>> devices;
      
      // 扫描0x01到0x7F的所有I2C地址
      for (byte address = 0x01; address <= 0x7F; address++) {
        Wire.beginTransmission(address);
        int result = Wire.endTransmission();
        
        if (result == 0) {
          // 检测到设备，获取版本信息
          String version = getDeviceVersion(address);
          
          // 格式化设备地址作为id
          String id = "0x";
          if (address < 0x10) {
            id += "0"; // 补零，确保格式一致
          }
          id += String(address, HEX);
          
          // 添加键值对到vector
          devices.push_back(std::make_pair(id, version));
          
          // 短暂延迟，避免总线过载
          delay(10);
        }
      }
      
      return devices;
    }
  };


// OLED FeatherWing buttons map to different pins depending on board.
// The I2C (Wire) bus may also be different.
#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
  #define WIRE Wire
#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32C6)
  #define BUTTON_A  7
  #define BUTTON_B  6
  #define BUTTON_C  5
  #define WIRE Wire
#elif defined(ESP32)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
  #define WIRE Wire
#elif defined(ARDUINO_STM32_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
  #define WIRE Wire
#elif defined(TEENSYDUINO)
  #define BUTTON_A  4
  #define BUTTON_B  3
  #define BUTTON_C  8
  #define WIRE Wire
#elif defined(ARDUINO_FEATHER52832)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
  #define WIRE Wire
#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  #define BUTTON_A  9
  #define BUTTON_B  8
  #define BUTTON_C  7
  #define WIRE Wire
#else // 32u4, M0, M4, nrf52840 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
  #define WIRE Wire
#endif
  /*
   * OLED 屏幕类
   *
   * 使用I2C接口
   */
  class Oled: public Sys_module, public Printable {
    private:
    String _NAME = "Oled";       /* 模块名称 */
    bool oled_has_init = false;  /* OLED模块初始化标志 */
    Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &WIRE);
    public:
    bool is_running() override {
      return oled_has_init;
    }
    /*
     * 初始化模块
     */
    bool begin() override {
      display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      return (oled_has_init = true);
    }
    /*
     */
    String show() override {
      return _NAME;
    }
    
    /*
     * 测试模块
     */
    void print(const String msg, const int _=0) override {
      display.setCursor(0,0);
      display.clearDisplay();
      display.print(msg);
      display.display();
    }
  };

   /*
   * TFT屏幕类
   *
   * 该模块会与I2C传感器冲突，但是不接模块时可用
   */
  //class Tft: public Sys_module, public Printable {
  //private:
  //  String                        _NAME         = "Tft";  /* 模块名 */
  //  bool                          tft_has_init  = false;  /* 屏幕启动状态 */
  //  DIYables_TFT_ILI9486_Shield   TFT_display;            /* 屏幕对象 */
  //  uint16_t                      MAGENTA       = DIYables_TFT::colorRGB(255, 0, 255);    /* ??? */
  //  uint16_t                      WHITE         = DIYables_TFT::colorRGB(255, 255, 255);  /* 白色 */
  //  uint16_t                      BLACK         = DIYables_TFT::colorRGB(0, 0, 0);        /* 黑色 */

  //public:

    /*
     * TFT屏幕是否已经启动
     */
  //  bool is_running() override {
  //    return tft_has_init;
  //  }

    /*
     * 启动模块
     *
     * FIXME: 抽象出设置方法
     */
  //  bool begin() override {

        /* 启动屏幕 */
  //      TFT_display.begin();

        /* Set the rotation (0 to 3) */
  //      TFT_display.setRotation(0);  // Rotate screen 90 degrees
  //      TFT_display.fillScreen(BLACK);

        /* Set text color and size */
  //      TFT_display.setTextColor(WHITE);
  //      TFT_display.setTextSize(2);  // Adjust text size as needed

        /* 它总是能够启动成功 */
  //      return (tft_has_init = true);

  //  }

    /*
     * 展示模块名
     */
  //  String show() override {
  //    return _NAME;
  //  }

    /*
     * 测试模块
     */
  //  void print(const String msg, const int _=0) override {
  //    TFT_display.print(msg);
  //  }

  //};


  /* 
   * JS脚本类型
   *
   * 能够运行js脚本
   */
  class Elk: public Sys_module {
  private:
    String _NAME = "Elk";    /* 模块的名字 */
    bool elk_has_init = false;  /* 解释器启动状态 */
  public:

    /*
     * 是否正在运行
     */
    bool is_running() override {
      return elk_has_init;
    }

    /*
     * 展示模块名
     */
    String show() override {
      return _NAME;
    }

    /*
     * 初始化
     */
    bool begin() override;

    /*
     * 运行解释器
     *
     * FIXME: 改名为eval()
     */
    String test(String code);

  };



  /*
   * 网络模块
   */
  class Eth: public Sys_module {
  private:
    String _NAME = "ETH";       /* 存储本模块名 */
    bool eth_has_init = false;  /* 以太网启动状态 */
    int cs_pin;                 /* 存储片选引脚 */

    /*
    * 从以太网模块获取iP地址
    */
    String get_local_ip() {
      IPAddress ip = Ethernet.localIP();
      return ip.toString();
    }

  public:

    /*
     * 构造函数，储存片选引脚
     */
    Eth(int pin) {
      cs_pin = pin;
    }

    /*
     * 模块是否已经初始化
     */
    bool is_running() override {
      return eth_has_init;
    }

    /*
     * 展示本模块的名字
     */
    String show() override {
      return _NAME;
    }

    /* 
    * -------- 初始化以太网 --------
    *
    * FIXME: 改为可通过文件配置
    */
    bool begin() override;


    /*
     * 展示本地网络地址（IP）
     */
    void print_local_ip();

  };



  /*
  * 封装文件系统模块，在Arduino中一般是操作sd卡模块
  *
  *
  * FIXME: 因为有的文件可能很大，这样可能不行
  *
  * 也许传入一个回调函数来处理文件流
  *
  * FIXME(20251029): 传回一个封装的文件对象，能够自动关闭
  */
  class Fs: public Sys_module {
  private:
    String _NAME = "Fs";      /* 存储模块的名字 */
    int cs_pin;               /* 存储模块所用的片选引脚 */
    bool sd_has_init = false; /* sd模块启动标志 */

    /* 
     * 如果之前sd卡未能启动，就尝试启动
     */
    bool try_before_all();

  public:

    /*
     * 构造函数，传入片选引脚编号
     */
    Fs(int cs_pin) {
      this->cs_pin = cs_pin;
    }

    /*
     * 文件系统是否已经启动
     */
    bool is_running() {
      return sd_has_init;
    }
        
    /* 
     * -------- 初始化SD卡 --------
     */
    bool begin() override;

    /*
     * 展示模块名
     */
    String show() override {
      return _NAME;
    }

    /*
     * 路径是否为目录
     */
    bool isDirectory(String path);

    /*
     * 尝试打开sd卡根目录，判断sd模块是否正常工作
     */
    bool isWorking();

    /*
     * 判断一个路径是否是文件
     */
    bool isFile(String path);

    /*
     * 打开一个文件返回它的全部内容
     */
    String Read(String filename);


    /*
     * 列表一个文件夹里的所有文件
     */
    String List(String filepath);

  };

  ////////////////////////////////////////

  /*
   * MPU6050加速度与温度传感器
   */
  class MPU6050: public Sh_match {
  private:
    String _NAME = "MPU6050"; /* 模块名 */
    bool has_init = false;    /* 是否已经初始化了 */
    Adafruit_MPU6050 mpu;     /* mpu6050对象 */
    /*
     * 获取当前加速度计的量程
     */
    String getAccelerometerRange() {
      mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
      switch (mpu.getAccelerometerRange()) {
        case MPU6050_RANGE_2_G:
          return "+-2G";
        case MPU6050_RANGE_4_G:
          return "+-4G";
        case MPU6050_RANGE_8_G:
          return "+-8G";
        case MPU6050_RANGE_16_G:
          return "\n+-16G";
      }

      /* 缺省值 */
      return "Unknow";
    }
    /*
     * 查询当前陀螺仪的量程
     */
    String getGyroRange() {
      mpu.setGyroRange(MPU6050_RANGE_500_DEG);
      switch (mpu.getGyroRange()) {
        case MPU6050_RANGE_250_DEG:
          return "+- 250 deg/s";
        case MPU6050_RANGE_500_DEG:
          return "+- 500 deg/s";
        case MPU6050_RANGE_1000_DEG:
          return "+- 1000 deg/s";
        case MPU6050_RANGE_2000_DEG:
          return "+- 2000 deg/s";
      }

      /* 缺省值 */
      return "Unknow";
    }
    /*
     * 查询传感器内部数字低通滤波器的带宽
     */
    String getFilterBandwidth() {
      mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
      switch (mpu.getFilterBandwidth()) {
        case MPU6050_BAND_260_HZ:
          return "260 Hz";
        case MPU6050_BAND_184_HZ:
          return "184 Hz";
        case MPU6050_BAND_94_HZ:
          return "94 Hz";
        case MPU6050_BAND_44_HZ:
          return "44 Hz";
        case MPU6050_BAND_21_HZ:
          return "21 Hz";
        case MPU6050_BAND_10_HZ:
          return "10 Hz";
        case MPU6050_BAND_5_HZ:
          return "5 Hz";
      }

      /* 缺省值 */
      return "Unknow";
    }
  public:
    /*
     * 模块是否正在运行
     */
    bool is_running() override {
      return has_init;
    }
    /*
     * 启动模块
     */
    bool begin() override {
      return (has_init = mpu.begin());
    }
    /*
     * 展示模块名
     */
    String show() override {
      return _NAME;
    }
    /*
     * 运行匹配
     *
     * FIXME: 增加一层抽象
     */
    String test(String cmd) override {
      if (cmd != _NAME || !has_init)
        return "";

      JSONVar obj;

      obj["msg"] = "OK";
      obj["Accelerometer range"]  = getAccelerometerRange();
      obj["Gyro range"]           = getGyroRange();
      obj["Filter bandwidth"]     = getFilterBandwidth();

      delay(100);


      /* Get new sensor events with the readings */
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      /* Print out the values */
      obj["Acceleration X(m/s^2)"] = a.acceleration.x;
      obj["Acceleration Y(m/s^2)"] = a.acceleration.y;
      obj["Acceleration Z(m/s^2)"] = a.acceleration.z;

      obj["Rotation X(rad/s)"] = g.gyro.x;
      obj["Rotation Y(rad/s)"] = g.gyro.y;
      obj["Rotation Z(rad/s)"] = g.gyro.z;

      obj["Temperature(degC)"] = temp.temperature;

      obj["msg"] = "OK";
      return JSON.stringify(obj);
    }
  };


  /*
   * SGP30空气质量传感器
   */
  class SGP30: public Sh_match {
  private:
    String _NAME = "SGP30"; /* 模块名 */
    bool has_init = false;  /* 是否已初始化 */
    Adafruit_SGP30 sgp;     /* SGP30对象 */
  public:
    /*
     * 模块是否已经初始化
     */
    bool is_running() override {
      return has_init;
    }
    /*
     * 启动SGP30模块
     */
    bool begin() override {
      if ((has_init = sgp.begin())) {
        sgp.setIAQBaseline(0x8E68, 0x8F41); /* SGP30需要基线校准 */
      }

      /* 返回结果 */
      return has_init;
    }
    /*
     * 展示模块名
     */
    String show() override {
      return _NAME;
    }
    /*
     * 运行匹配
     */
    String test(String cmd) override {

      /* 命令不是这个传感器 或者 SGP30未启动 */
      if (cmd != _NAME || !has_init) {
        return "";
      }

      JSONVar obj;

      /* 记录编号 Found SGP30 serial #?????? */
      String msg = "#";
      msg += String(sgp.serialnumber[0], HEX);  //转换为字符串
      msg += String(sgp.serialnumber[1], HEX);
      msg += String(sgp.serialnumber[2], HEX);
      obj["id"] = msg;

      if (sgp.IAQmeasure()) {
        obj["TVOC(ppb)"] = sgp.TVOC;
        obj["eCO2(ppm)"] = sgp.eCO2;
      }

      /* 原始值好像没有什么用 */
      if (sgp.IAQmeasureRaw()) {
        obj["Raw H2"] = sgp.rawH2;
        obj["Raw Ethanol"] = sgp.rawEthanol;
      }

      obj["msg"] = "OK";
      return JSON.stringify(obj);
    }
    
  };


  /*
   * SHT3X温湿度传感器
   */
  class SHT3X: public Sh_match {
  private:
    String _NAME = "SHT3X";   /* 模块的名字 */
    bool has_init = false;    /* 是否已经初始化 */
    ArtronShop_SHT3x sht = ArtronShop_SHT3x(0x44, &Wire);  // ADDR: 0 => 0x44, ADDR: 1 => 0x45
  public:
    /*
     * 展示模块名
     */
    String show() override {
      return _NAME;
    }
    /*
     * 是否已经初始化
     */
    bool is_running() override {
      return has_init;
    }
    /*
     *初始化模块
     */
    bool begin() override {
      return (has_init = sht.begin());
    }
    /*
     * 进行匹配
     */
    String test(String cmd) override {
      
      /* 命令不是这个传感器 或者模块未启动 */
      if (cmd != _NAME || !has_init) {
        return "";
      }


      JSONVar obj;

      if (!sht.measure()) {
        obj["msg"] = "ERR";
        return JSON.stringify(obj);
      }

      obj["Temperature(*C)"] = String(sht.temperature(), 1);
      obj["Humidity(%RH)"] = String(sht.humidity(), 1);
      
      obj["msg"] = "OK";
      return JSON.stringify(obj);

    }
  };


  /*
   * BH1750光照传感器
   */
  class BH1750: public Sh_match {
  private:
    String _NAME = "BH1750"; /* 模块名字 */
    bool has_init = false;   /* 模块是否已经初始化 */
    ArtronShop_BH1750 bh1750 = ArtronShop_BH1750(0x23, &Wire); // Non Jump ADDR: 0x23, Jump ADDR: 0x5C
  public:
    /*
     * 模块是否已经初始化
     */
    bool is_running() override {
      return has_init;
    }
    /*
     * 初始化方法
     */
    bool begin() override {
      return (has_init = bh1750.begin());
    }
    /*
     * 获取模块的名字
     */
    String show() {
      return _NAME;
    }
    /*
     * 进行匹配测试
     */
    String test(String cmd) override {

      /*
       * 如果命令不是BME280
       * 或者BME280在启动时没有检测到
       * 直接退出
       */
      if (cmd != _NAME || !has_init || !this->begin()) /* 再次启动失败 */
        return "";

      JSONVar obj;
      obj["Light(lx)"] = bh1750.light();
      obj["msg"] = "OK";
      return JSON.stringify(obj);

    }
  };

  /*
   * BME280温湿度大气压传感器
   */
  class BME280: public Sh_match {
  private:
    String _NAME = "BME280"; /* 模块名字 */
    bool has_init = false;   /* 模块是否已经初始化 */
    Adafruit_BME280 bme;     /* BME280模块 use I2C interface */
    uint16_t addr;           /* 有效模块地址 */
  public:
    static const uint16_t ADDR0 = 0x76; /*  模块地址*/
    static const uint16_t ADDR1 = 0x77;
    /*
     * 构造函数，需要输入模块地址
     */
    BME280(uint16_t addr) {
      this->addr = addr;
    }
    /*
     * 模块是否已经初始化
     */
    bool is_running() override {
      return has_init;
    }
    /*
     * 初始化方法
     */
    bool begin() override {
      return (has_init = bme.begin(addr));
    }
    /*
     * 获取模块的名字
     */
    String show() {
      return _NAME;
    }
    /*
     * 进行匹配测试
     */
    String test(String cmd) override {

      /*
       * 如果命令不是BME280
       * 或者BME280在启动时没有检测到
       * 直接退出
       */
      if (cmd != _NAME || !has_init)
        return "";


      JSONVar obj;
      if (bme.begin(addr)) {
        obj["Temperature(*C)"] = bme.readTemperature();
        obj["Humidity(%)"] = bme.readHumidity();
        obj["Pressure(hPa)"] = bme.readPressure() / 100.0F;
        obj["msg"] = "OK";
        return JSON.stringify(obj);
      }

      else {
        obj["msg"] = "ERR";
        return JSON.stringify(obj);
      }

    }
  };

}

extern qing::Prj_Mozart prj;      /* 莫扎特项目对象 */
extern qing::Sh_ma      sh;       /* 命令行模式对象*/

extern qing::Out        out;      /* 输出对象 */
extern qing::I2c        i2c;      /* I2C对象 */
extern qing::Elk        elk;      /* 脚本语言对象 */
//extern qing::Tft        tft;      /* TFT屏幕 */
extern qing::Fs         fs;       /* 文件系统 */
extern qing::Eth        eth;      /* 以太网 */
extern qing::Oled       oled;     /* OLED屏幕 */

extern qing::MPU6050    mpu6050;  /* 加速度和温度传感器 */
extern qing::SGP30      sgp30;    /* 空气质量传感器 */
extern qing::SHT3X      sht3x;    /* 温湿度传感器 */
extern qing::BME280     bme280;   /* 温湿度和大气压传感器 */
extern qing::BH1750     bh1750;   /* 光照传感器 */
