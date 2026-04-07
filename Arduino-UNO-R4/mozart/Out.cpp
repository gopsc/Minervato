#include "prj.h"
namespace qing {
  void Out::print(const String msg, const int mode) {
    
      if (mode == 2 && oled.is_running()) {
        oled.print(msg);
      }
      //if (mode == 1 && tft.is_running()) {
      //  tft.print(msg);
      //}
      if (out_has_init) {
        Serial.print(msg);
      }
    }
}