#ifdef ARDUINO

#include <Arduino.h>
#include "platform/Time.h"

namespace platform {

unsigned long millis() {
  return ::millis();
}

void advanceTime(unsigned long) {}
void resetTime() {}

}  // namespace platform

#endif


