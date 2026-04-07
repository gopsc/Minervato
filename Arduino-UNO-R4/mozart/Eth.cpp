#include "prj.h"
namespace qing {

  void Eth::print_local_ip(){
    out.print("IP address: " + get_local_ip() + "\n", 2);
  }

  bool Eth::begin()  {

    // You can use Ethernet.init(pin) to configure the CS pin
    Ethernet.init(cs_pin);   /* Ethernet.init(int) => void */
    //Ethernet.init(10);  /* Most Arduino shields */
    //Ethernet.init(5);   /* MKR ETH Shield */
    //Ethernet.init(0);   /* Teensy 2.0 */
    //Ethernet.init(20);  /* Teensy++ 2.0 */
    //Ethernet.init(15);  /* ESP8266 with Adafruit FeatherWing Ethernet */
    //Ethernet.init(33);  /* ESP32 with Adafruit FeatherWing Ethernet */

    //bool res = Ethernet.begin(mac, ip);   // 静态IP
    bool res = Ethernet.begin(mac);       // DHCP IP地址
    if (res == false) {

    } else if (res && Ethernet.hardwareStatus() == EthernetNoHardware) {

      res = false;
      out.print("Ethernet shield was not found. Sorry, cant't run without hardware. :(\n", 0);

    } else if (res && Ethernet.linkStatus() == LinkOFF) {

      res = false;
      out.print("ethernet is not connectted\n", 0);

    } else if (res == true) {

    }


    /* 返回结果 */
    return (eth_has_init = res);
  }

}