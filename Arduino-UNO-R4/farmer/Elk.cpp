/* 
 * 使用Arduino的elk JS移植库来提供运行解释脚本的能力
 *
 * 使得微控制器能够运行Javascript的传感器代码
 */

#include "prj.h"

/* 以C语言形式引入elk库 */
extern "C" {
  #include "elk.h"
}

namespace qing {

  /* JS内存缓冲区大小 */
  static constexpr int BUFFER_SIZE = 4096;
  
  /* JS运行时内存 */
  static char buf[BUFFER_SIZE]; // Runtime JS memory
  
  /* js结构体 */
  static struct js *js = nullptr;
  
  
  /*
   * 延时函数
   */
  static void myDelay(int milli) {
    delay(milli);
  }
  
  /*
   * i2c通信设置
   *
   * 启动针对某个地址的通信
   */
  static void i2c_trans(int addr) {
    Wire.beginTransmission(addr);
  }
  
  /*
   * i2c关闭通信
   *
   * FIXME: 增加结果的判断
   */
   
  static int i2c_end() {
    return Wire.endTransmission(); /* 成功返回0 */
  }
  
  /*
   * i2c写数据
   *
   * FIXME: 错误判断
   */
  static void i2c_write(int data) {
    Wire.write(data);
  }
  
  
  /*
   * i2c请求数据
   *
   * FIXME: 它返回什么？
   */
  static void i2c_request(int addr, int len) {
    Wire.requestFrom(addr, len);
  }
  
  /*
   * i2c读取数据
   *
   * 如果失败返回-1
   */
  static int i2c_read() {
    if (Wire.available()) {
      return Wire.read();
    } else {
      return -1;
    }
  }
  
  
    bool Elk::begin() {
      return (elk_has_init = true);
    }
  
    /*
     * 运行脚本，每次运行都清空之前的上下文
     */
    String Elk::test(String code) {
      js = js_create(buf, sizeof(buf));
      jsval_t global = js_glob(js), i2c = js_mkobj(js);    /* Equivalent to: */
      js_set(js, global, "i2c",      i2c);                      /* let i2c = {};  */
      js_set(js, global, "delay",    js_import(js, (uintptr_t) myDelay, "vi"));    /* delay() */
      js_set(js, i2c,    "trans",    js_import(js, (uintptr_t)i2c_trans, "vi"));      /* i2c.trans(addr) */
      js_set(js, i2c,    "end",      js_import(js, (uintptr_t)i2c_end,   "i"));       /* i2c.close() */
      js_set(js, i2c,    "write",    js_import(js, (uintptr_t)i2c_write, "vi"));      /* i2c.write(data) */
      js_set(js, i2c,    "request",  js_import(js, (uintptr_t)i2c_request, "vii")); /* i2c.request(addr, len) */
      js_set(js, i2c,    "read",     js_import(js, (uintptr_t)i2c_read, "i"));         /* i2c.read() */
      jsval_t result = js_eval(js, code.c_str(), ~0); /* 执行代码 */
      return  js_str(js, result);                     /* 返回运行结果 */
    }

}
