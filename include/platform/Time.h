#pragma once

#include <cstdint>

namespace platform {

unsigned long millis();
void advanceTime(unsigned long ms);
void resetTime();

}  // namespace platform


