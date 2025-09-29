#ifndef ARDUINO

#include "platform/Time.h"

namespace {

unsigned long mock_time_ms = 0;

}  // namespace

namespace platform {

unsigned long millis() {
  return mock_time_ms;
}

void advanceTime(unsigned long ms) {
  mock_time_ms += ms;
}

void resetTime() {
  mock_time_ms = 0;
}

}  // namespace platform

#endif


