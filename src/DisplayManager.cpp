#include "DisplayManager.h"
#include <Wire.h>
#include <fonts/retro_font4x6.h>
#include <fonts/modern_font4x6.h>
#include "BrightnessLevels.h"

DisplayManager::DisplayManager(int num_boards, int board_width, int board_height)
  : num_boards_(num_boards)
  , board_width_(board_width)
  , board_height_(board_height)
  , total_width_(board_width * num_boards)
  , total_height_(board_height)
  , character_width_(4)
  , max_characters_(total_width_ / character_width_)
  , drivers_{nullptr, nullptr, nullptr, nullptr}
  , current_brightness_level_(DEFAULT_BRIGHTNESS)
{
}

DisplayManager::~DisplayManager() { }

bool DisplayManager::initialize() {
  Serial.println("Initializing DisplayManager...");
  
  // I2C is already initialized in main.cpp with default ESP32 pins
  Serial.println("Using existing I2C configuration (SDA=GPIO21, SCL=GPIO22)");
  
  // Initialize drivers
  Serial.println("Creating individual drivers...");
  initializeDrivers();
  
  // Initialize each driver
  for (int i = 0; i < num_boards_; i++) {
    if (drivers_[i]) {
      Serial.printf("Initializing driver %d...\n", i);
      
      // Test I2C communication before initializing
      ADDR addr_pin;
      switch (i) {
        case 0: addr_pin = ADDR::GND; break;
        case 1: addr_pin = ADDR::VCC; break;
        case 2: addr_pin = ADDR::SDA; break;
        case 3: addr_pin = ADDR::SCL; break;
        default: addr_pin = ADDR::GND; break;
      }
      
      uint8_t i2c_addr = getI2CAddressFromADDR(addr_pin);
      Wire.beginTransmission(i2c_addr);
      uint8_t error = Wire.endTransmission();
      
      if (error != 0) {
        Serial.printf("WARNING: Driver %d (0x%02X) not responding - skipping initialization\n", i, i2c_addr);
        continue;
      }
      
      drivers_[i]->begin();
      drivers_[i]->setGlobalCurrent(50);
      Serial.printf("Driver %d initialized successfully\n", i);
    }
  }
  
  Serial.println("Clearing all displays...");
  clearBuffer();
  Serial.println("Updating all displays...");
  updateDisplay();
  
  Serial.println("DisplayManager initialization complete");
  return true;
}

bool DisplayManager::verifyDrivers() {
  Serial.println("Verifying LED driver communication...");
  
  bool all_ok = true;
  for (int i = 0; i < num_boards_; i++) {
    if (!drivers_[i]) {
      Serial.printf("ERROR: Driver %d not initialized\n", i);
      all_ok = false;
      continue;
    }
    
    Serial.printf("Testing driver %d communication...\n", i);
    
    // Test I2C communication first
    if (!testDriverCommunication(i)) {
      Serial.printf("ERROR: Driver %d I2C communication failed!\n", i);
      all_ok = false;
      continue;
    }
    
    // Test basic operations
    drivers_[i]->clear();
    drivers_[i]->drawRect(2, 2, getWidth() - 4, getHeight() - 4, 100);
    drivers_[i]->show();
    drivers_[i]->clear();
    drivers_[i]->show();
    
    Serial.printf("Driver %d verified successfully\n", i);
  }
  
  if (all_ok) {
    Serial.printf("✓ All %d display drivers verified successfully\n", num_boards_);
  } else {
    Serial.printf("✗ Driver verification failed - some displays may not be connected\n");
  }
  
  return all_ok;
}

void DisplayManager::scanI2C() {
  Serial.println("\nScanning I2C bus for display devices...");
  // Known display addresses for IS31FL3737
  static const I2CKnownDevice known[] = {
    {0x50, "IS31FL3737 Display (GND)"},
    {0x5A, "IS31FL3737 Display (VCC)"},
    {0x5F, "IS31FL3737 Display (SDA)"}
  };
  int found = scanI2CBus(known, sizeof(known)/sizeof(known[0]));
  if (found == 0) {
    Serial.println("No I2C devices found via scan");
  } else {
    Serial.printf("Found %d I2C device(s) via scan\n", found);
  }
  Serial.println("I2C scan complete\n");
}

void DisplayManager::setPixel(int x, int y, uint8_t brightness) {
  if (!isValidPosition(x, y)) return;
  
  // Apply the same coordinate transformation as the old working code
  int screen_x = total_width_ - x - 1;
  int screen_y = total_height_ - y - 1;
  
  // Determine which board this pixel belongs to
  int board = screen_x / board_width_;
  if (board >= num_boards_ || !drivers_[board]) return;
  
  // Calculate local coordinates within the board (24x6 logical)
  int local_x = screen_x % board_width_;  // 0-23
  int local_y = screen_y;                 // 0-5
  
  // Convert 24x6 logical coordinates to 12x12 physical coordinates for RetroText PCB
  // 6 characters in a line: chars 0,1,2 use SW1-6, chars 3,4,5 use SW7-12
  int char_index = local_x / 4;  // Which character (0-5)
  int char_pixel_x = local_x % 4;  // Pixel within character (0-3)
  
  int physical_x, physical_y;
  
  if (char_index < 3) {
    // Characters 0,1,2: use SW1-6 (top half)
    physical_x = (char_index * 4) + char_pixel_x;  // CS1-4, CS5-8, CS9-12
    physical_y = local_y;  // SW1-6 (rows 0-5)
  } else {
    // Characters 3,4,5: use SW7-12 (bottom half)  
    physical_x = ((char_index - 3) * 4) + char_pixel_x;  // CS1-4, CS5-8, CS9-12
    physical_y = local_y + 6;  // SW7-12 (rows 6-11)
  }
  
  drivers_[board]->drawPixel(physical_x, physical_y, brightness);
}

uint8_t DisplayManager::getPixel(int x, int y) const {
  if (!isValidPosition(x, y)) return 0;
  
  // IS31FL373x drivers don't support pixel readback
  // Return 0 - this is mainly used for debugging
  // Most LED drivers don't support reading pixel values back
  return 0;
}

bool DisplayManager::isValidPosition(int x, int y) const {
  return (x >= 0 && x < total_width_ && y >= 0 && y < total_height_);
}

void DisplayManager::clearBuffer() {
  for (int i = 0; i < num_boards_; i++) {
    if (drivers_[i]) {
      drivers_[i]->clear();
    }
  }
}

void DisplayManager::fillBuffer(uint8_t brightness) {
  // fillScreen doesn't exist in IS31FL373x driver, so implement manually
  for (int y = 0; y < total_height_; y++) {
    for (int x = 0; x < total_width_; x++) {
      setPixel(x, y, brightness);
    }
  }
}

void DisplayManager::dimBuffer(uint8_t amount) {
  // Use master brightness control for dimming effect
  for (int i = 0; i < num_boards_; i++) {
    if (drivers_[i]) {
      uint8_t current_brightness = 255 - amount;
      if (current_brightness < 10) current_brightness = 10;  // Minimum visibility
      drivers_[i]->setMasterBrightness(current_brightness);
    }
  }
}

void DisplayManager::updateDisplay() {
  for (int i = 0; i < num_boards_; i++) {
    if (drivers_[i]) {
      drivers_[i]->show();
    }
  }
}

void DisplayManager::drawCharacter(uint8_t character_pattern[6], int x_offset, uint8_t brightness) {
  for (int row = 0; row < 6; row++) {
    uint8_t pattern = character_pattern[row];
    for (int col = 0; col < 4; col++) {
      int x_pos = x_offset + (3 - col);  // Character positioning logic
      if (x_pos >= 0 && x_pos < total_width_) {
        if ((pattern & (1 << col)) != 0) {
          setPixel(x_pos, row, brightness);
        } else {
          setPixel(x_pos, row, 0);
        }
      }
    }
  }
}

void DisplayManager::drawText(const String& text, int start_x, uint8_t brightness, bool use_alt_font) {
  for (int i = 0; i < text.length(); i++) {
    char c = text.charAt(i);
    uint8_t character = c - 32; // ASCII offset
    
    // Get character pattern
    uint8_t pattern[6];
    for (int row = 0; row < 6; row++) {
      pattern[row] = getCharacterPattern(character, row, use_alt_font);
    }
    
    // Draw character
    int char_x = start_x + (i * character_width_);
    if (char_x < total_width_) {
      drawCharacter(pattern, char_x, brightness);
    }
  }
}

void DisplayManager::setGlobalBrightness(uint8_t brightness) {
  for (int i = 0; i < num_boards_; i++) {
    if (drivers_[i]) {
      drivers_[i]->setGlobalCurrent(brightness);
    }
  }
}

void DisplayManager::setBoardBrightness(int board_index, uint8_t brightness) {
  if (board_index >= 0 && board_index < num_boards_ && drivers_[board_index]) {
    drivers_[board_index]->setGlobalCurrent(brightness);
  }
}

int DisplayManager::getBoardForPixel(int x) const {
  return x / board_width_;
}

// mapPixelToBoard removed - IS31FL373x_Canvas handles multi-board coordinates directly

uint8_t DisplayManager::getCharacterPattern(uint8_t character, uint8_t row, bool use_alt_font) const {
  if (use_alt_font) {
    // Alternative font
    if (character >= 0 && character <= (sizeof(modern_font4x6)-3)/6) {
      return pgm_read_byte(&modern_font4x6[3+character*6+row]) >> 4;
    }
  } else {
    // Original font
    if (character >= 0 && character <= (sizeof(retro_font4x6)-3)/6) {
      return retro_font4x6[3+character*6+row] >> 4;
    }
  }
  return 0;
}

// I2C functions removed - IS31FL373x driver handles I2C directly

// Private helper methods
void DisplayManager::initializeDrivers() {
  Serial.printf("Creating %d individual drivers\n", num_boards_);
  
  for (int i = 0; i < num_boards_ && i < 4; i++) {
    ADDR addr;
    
    // Initialize with the appropriate addresses for each board
    switch (i) {
      case 0: addr = ADDR::GND; break;
      case 1: addr = ADDR::VCC; break;
      case 2: addr = ADDR::SDA; break;
      case 3: addr = ADDR::SCL; break;
      default: addr = ADDR::GND; break;
    }
    
    Serial.printf("Creating driver %d with address %d\n", i, (int)addr);
    drivers_[i] = std::make_unique<IS31FL3737>(addr);
    Serial.printf("Driver %d created successfully\n", i);
  }
  
  Serial.println("All drivers created");
}

void DisplayManager::convertLogicalToPhysical(int logical_x, int logical_y, int& physical_x, int& physical_y) {
  // Convert 24x6 logical coordinates to 12x12 physical coordinates for RetroText PCB
  // Based on PCB_Layout.md: 6 characters arranged horizontally
  // Characters 0,1,2 use SW1-6 (CS1-4, CS5-8, CS9-12)
  // Characters 3,4,5 use SW7-12 (CS1-4, CS5-8, CS9-12)
  
  int char_index = logical_x / 4;      // Which character (0-5)
  int char_pixel_x = logical_x % 4;    // Pixel within character (0-3)
  
  if (char_index < 3) {
    // Characters 0,1,2: top half of chip (SW1-6)
    physical_x = (char_index * 4) + char_pixel_x;  // CS1-4, CS5-8, CS9-12
    physical_y = logical_y;                         // SW1-6 (rows 0-5)
  } else {
    // Characters 3,4,5: bottom half of chip (SW7-12)
    physical_x = ((char_index - 3) * 4) + char_pixel_x;  // CS1-4, CS5-8, CS9-12
    physical_y = logical_y + 6;                           // SW7-12 (rows 6-11)
  }
}

void DisplayManager::printDisplayConfiguration() {
  Serial.println("\n=== RetroText Display Configuration ===");
  Serial.printf("Total displays: %d\n", num_boards_);
  Serial.printf("Display resolution: %dx%d (total: %dx%d)\n", 
                board_width_, board_height_, total_width_, total_height_);
  Serial.printf("Characters per display: 6 (4x6 pixels each)\n");
  Serial.printf("I2C bus speed: 800 kHz\n\n");
  
  Serial.println("Display Layout (Left to Right):");
  Serial.println("┌─────────────────────────────────────────────────────────────┐");
  Serial.println("│ Pos │ ADDR Pin │ I2C Addr │ Connection │ Status             │");
  Serial.println("├─────────────────────────────────────────────────────────────┤");
  
  for (int i = 0; i < num_boards_; i++) {
    ADDR addr_pin;
    switch (i) {
      case 0: addr_pin = ADDR::GND; break;
      case 1: addr_pin = ADDR::VCC; break;
      case 2: addr_pin = ADDR::SDA; break;
      case 3: addr_pin = ADDR::SCL; break;
      default: addr_pin = ADDR::GND; break;
    }
    
    uint8_t i2c_addr = getI2CAddressFromADDR(addr_pin);
    const char* pin_name = getADDRPinName(addr_pin);
    
    // Test I2C communication
    Wire.beginTransmission(i2c_addr);
    uint8_t error = Wire.endTransmission();
    
    const char* status = (error == 0) ? "✓ Connected" : "✗ Not Found";
    
    Serial.printf("│  %d  │   %-6s │  0x%02X    │   %-8s │ %-18s │\n", 
                  i, pin_name, i2c_addr, pin_name, status);
  }
  
  Serial.println("└─────────────────────────────────────────────────────────────┘");
  Serial.println("\nPin Connections (standard I2C):");
  Serial.println("  VCC  → 3.3V");
  Serial.println("  GND  → Ground"); 
  Serial.println("  SDA  → GPIO21 (ESP32)");
  Serial.println("  SCL  → GPIO22 (ESP32)");
  Serial.println("  ADDR → Connect to GND/VCC/SDA/SCL for addressing\n");
}

uint8_t DisplayManager::getI2CAddressFromADDR(ADDR addr) const {
  // Based on IS31FL3737 datasheet and README.md
  switch (addr) {
    case ADDR::GND: return 0x50;  // ADDR pin connected to GND
    case ADDR::VCC: return 0x5A;  // ADDR pin connected to VCC  
    case ADDR::SDA: return 0x5F;  // ADDR pin connected to SDA
    case ADDR::SCL: return 0x55;  // ADDR pin connected to SCL
    default: return 0x50;
  }
}

const char* DisplayManager::getADDRPinName(ADDR addr) const {
  switch (addr) {
    case ADDR::GND: return "GND";
    case ADDR::VCC: return "VCC";
    case ADDR::SDA: return "SDA";
    case ADDR::SCL: return "SCL";
    default: return "GND";
  }
}

bool DisplayManager::testDriverCommunication(int driver_index) {
  if (driver_index >= num_boards_ || !drivers_[driver_index]) {
    return false;
  }
  
  // Get the I2C address for this driver
  ADDR addr_pin;
  switch (driver_index) {
    case 0: addr_pin = ADDR::GND; break;
    case 1: addr_pin = ADDR::VCC; break;
    case 2: addr_pin = ADDR::SDA; break;
    case 3: addr_pin = ADDR::SCL; break;
    default: return false;
  }
  
  uint8_t i2c_addr = getI2CAddressFromADDR(addr_pin);
  
  // Test I2C communication
  Wire.beginTransmission(i2c_addr);
  uint8_t error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.printf("  I2C Error %d for address 0x%02X (%s pin)\n", 
                  error, i2c_addr, getADDRPinName(addr_pin));
    return false;
  }
  
  Serial.printf("  I2C communication OK: 0x%02X (%s pin)\n",
                i2c_addr, getADDRPinName(addr_pin));
  return true;
}

void DisplayManager::displayStaticText(const String& text, bool use_alt_font) {
  // Clear the buffer first
  clearBuffer();

  // Align to character boundary (left-aligned at character 0)
  int start_x = 0;

  // Draw the text
  drawText(text, start_x, 255, use_alt_font);  // Full brightness for announcements

  // Update the display immediately
  updateDisplay();
}

// Brightness management
void DisplayManager::setBrightnessLevel(BrightnessLevel level) {
  current_brightness_level_ = level;
  uint8_t brightness_value = getBrightnessValue(level);
  setGlobalBrightness(brightness_value);
  Serial.printf("Display brightness level: %d%% (%d)\n", static_cast<int>(level) * 5, brightness_value);
}

// Initialization helpers
void DisplayManager::showTestPattern() {
  // Clear the buffer first
  clearBuffer();

  // Create a simple test pattern - checkerboard
  for (int x = 0; x < total_width_; x++) {
    for (int y = 0; y < total_height_; y++) {
      uint8_t brightness = ((x + y) % 2) == 0 ? getBrightnessValue(BRIGHTNESS_75_PERCENT) : getBrightnessValue(BRIGHTNESS_25_PERCENT);  // Alternating bright/dim
      setPixel(x, y, brightness);
    }
  }
  updateDisplay();

  Serial.println("Display test pattern shown");
}
