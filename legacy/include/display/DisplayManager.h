#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "IS31FL373x.h"
#include <array>
#include <memory>
#include "platform/I2CScan.h"
#include "display/FontManager.h"
#include "display/SignTextController.h"  // For Font enum

class DisplayManager {
public:
  // Constructor - initializes the display with board configuration
  DisplayManager(int num_boards = 3, int board_width = 24, int board_height = 6);
  
  // Destructor
  ~DisplayManager();
  
  // Initialization
  bool initialize();
  bool verifyDrivers();
  void scanI2C();
  void printDisplayConfiguration();
  
  // Display properties
  int getWidth() const { return total_width_; }
  int getHeight() const { return total_height_; }
  int getCharacterWidth() const { return character_width_; }
  int getMaxCharacters() const { return max_characters_; }
  
  // Pixel operations
  void setPixel(int x, int y, uint8_t brightness);
  uint8_t getPixel(int x, int y) const;
  bool isValidPosition(int x, int y) const;
  
  // Buffer operations
  void clearBuffer();
  void fillBuffer(uint8_t brightness);
  void dimBuffer(uint8_t amount);
  void updateDisplay();  // Push buffer to hardware
  
  // Higher-level drawing operations
  void drawCharacter(uint8_t character_pattern[6], int x_offset, uint8_t brightness);
  void drawGlyph4x6(int x, int y, uint8_t rows[6], uint8_t brightness);
  void drawText(const String& text, int start_x, uint8_t brightness, RetroText::Font font = RetroText::MODERN_FONT);
  void displayStaticText(const String& text, RetroText::Font font = RetroText::MODERN_FONT);
  
  // Legacy method for backward compatibility
  void drawText(const String& text, int start_x, uint8_t brightness, bool use_alt_font);
  void displayStaticText(const String& text, bool use_alt_font);
  
  // Configuration
  void setGlobalBrightness(uint8_t brightness);
  void setBoardBrightness(int board_index, uint8_t brightness);

  // Brightness management
  void setBrightnessLevel(uint8_t value);
  uint8_t getBrightnessLevel() const { return current_brightness_level_; }

  // Initialization helpers
  void showTestPattern();
  
  // Hardware-specific methods
  int getBoardForPixel(int x) const;
  
  // Font access
  uint8_t getCharacterPattern(uint8_t character, uint8_t row, RetroText::Font font = RetroText::MODERN_FONT) const;
  
  // Legacy method for backward compatibility  
  uint8_t getCharacterPattern(uint8_t character, uint8_t row, bool use_alt_font) const;
  
  // SignTextController factory methods
  std::unique_ptr<RetroText::SignTextController> createModernTextController();
  std::unique_ptr<RetroText::SignTextController> createRetroTextController();
  
  // Helper for displaying static messages (used during initialization)
  void displayStaticMessage(const String& message, RetroText::Font font = RetroText::MODERN_FONT, int duration_ms = 0);
  
  // Text rendering helpers
  static uint8_t getCharacterBrightness(char c, const String& text, int char_pos, bool is_time_display = false);
  
private:
  // Hardware configuration
  int num_boards_;
  int board_width_;
  int board_height_;
  int total_width_;
  int total_height_;
  int character_width_;
  int max_characters_;
  
  // Hardware abstraction - IS31FL373x driver integration
  static const int PIXELS_PER_BOARD = 12 * 12;  // IS31FL3737 native configuration
  std::array<std::unique_ptr<IS31FL3737>, 4> drivers_;  // Individual drivers for each board

  // Font management
  std::unique_ptr<FontManager> font_manager_;

  // Brightness management
  uint8_t current_brightness_level_;
  
  // Internal helper methods
  void initializeDrivers();
  void convertLogicalToPhysical(int logical_x, int logical_y, int& physical_x, int& physical_y);
  
  // Helper methods for display information
  uint8_t getI2CAddressFromADDR(ADDR addr) const;
  const char* getADDRPinName(ADDR addr) const;
  bool testDriverCommunication(int driver_index);
  
  // Text brightness helpers
  static bool isWordCapitalized(const String& text, int start_pos);
};

// Forward declaration for backward compatibility
void display_static_message(String message, bool use_modern_font = true, int display_time_ms = 0);

#endif // DISPLAY_MANAGER_H
