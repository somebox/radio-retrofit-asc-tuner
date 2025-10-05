/**
 * IS31FL3737 Register Definitions
 * 
 * Adapted from IS31FL373x Driver library
 * https://github.com/somebox/IS31FL373x-Driver
 * 
 * Original copyright notice:
 * Copyright (c) 2024 Jeremy Kenyon
 * Licensed under MIT License
 */
#pragma once

#include <cstdint>

namespace esphome {
namespace retrotext_display {

// Special registers
constexpr uint8_t IS31FL3737_REG_UNLOCK = 0xFE;
constexpr uint8_t IS31FL3737_REG_COMMAND = 0xFD;
constexpr uint8_t IS31FL3737_UNLOCK_VALUE = 0xC5;

// Page definitions
constexpr uint8_t IS31FL3737_PAGE_LED_CTRL = 0x00;  // LED On/Off control
constexpr uint8_t IS31FL3737_PAGE_PWM = 0x01;       // PWM duty for each LED
constexpr uint8_t IS31FL3737_PAGE_ABM = 0x02;       // Auto Breath Mode
constexpr uint8_t IS31FL3737_PAGE_FUNCTION = 0x03;  // Configuration

// Function page registers
constexpr uint8_t IS31FL3737_REG_CONFIG = 0x00;         // Configuration register
constexpr uint8_t IS31FL3737_REG_GLOBAL_CURRENT = 0x01; // Global current control
constexpr uint8_t IS31FL3737_REG_RESET = 0x11;          // Software reset

// Configuration register bits
constexpr uint8_t IS31FL3737_CONFIG_SSD = 0x01;  // Software Shutdown (0=shutdown, 1=normal)

// Matrix dimensions for IS31FL3737
constexpr uint8_t IS31FL3737_MATRIX_WIDTH = 12;
constexpr uint8_t IS31FL3737_MATRIX_HEIGHT = 12;
constexpr uint16_t IS31FL3737_PWM_BUFFER_SIZE = 144;  // 12×12

// Register stride (columns per row in register space)
constexpr uint8_t IS31FL3737_REGISTER_STRIDE = 16;

// ADDR pin to I2C address mapping (from IS31FL373x driver lib)
// Base address: 0b1010000 (0x50)
enum class IS31FL3737_ADDR {
  GND = 0x50,  // ADDR=GND:  0x50 + 0b0000 = 0x50
  SCL = 0x55,  // ADDR=SCL:  0x50 + 0b0101 = 0x55
  SDA = 0x5A,  // ADDR=SDA:  0x50 + 0b1010 = 0x5A  ← CORRECTED
  VCC = 0x5F   // ADDR=VCC:  0x50 + 0b1111 = 0x5F  ← CORRECTED
};

}  // namespace retrotext_display
}  // namespace esphome
