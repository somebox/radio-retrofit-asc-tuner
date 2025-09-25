#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>

struct I2CKnownDevice {
  uint8_t address;       // 7-bit I2C address
  const char* name;      // Human-readable device name
};

// Scan the I2C bus and print results to the provided stream (defaults to Serial).
// If a known device table is provided, names are printed for matching addresses.
// Returns the number of devices found.
inline int scanI2CBus(const I2CKnownDevice* known, size_t known_count, Stream& out = Serial) {
  int device_count = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      device_count++;
      out.printf("I2C device found at 0x%02X (%d)", address, address);

      // Try to resolve a name
      const char* name = nullptr;
      for (size_t i = 0; i < known_count; i++) {
        if (known[i].address == address) {
          name = known[i].name;
          break;
        }
      }
      if (name) {
        out.print(" - ");
        out.print(name);
      }
      out.println();
    }
  }
  return device_count;
}


