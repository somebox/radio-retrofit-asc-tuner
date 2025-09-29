#include "fonts/Icons4x6Font.h"

Icons4x6Font::Icons4x6Font(const unsigned char* font_data)
  : font_data_(font_data)
  , font_width_(0)
  , font_height_(0)
  , start_char_(0)
  , num_chars_(0)
{
  parseHeader();
}

uint8_t Icons4x6Font::getCharacterPattern(uint8_t character, uint8_t row) const {
  if (!hasCharacter(character) || row >= 6) {
    return 0;
  }
  
  // Calculate offset in font data
  // Header is 3 bytes: width, height, start_char
  // Each character is 6 bytes (6 rows)
  int char_offset = 3 + (character * 6) + row;
  
  // Icon font stores patterns directly in the lower 4 bits (no shift needed)
#ifdef ARDUINO
  uint8_t pattern = pgm_read_byte(&font_data_[char_offset]);
#else
  uint8_t pattern = font_data_[char_offset];
#endif
  return pattern & 0x0F;
}

bool Icons4x6Font::hasCharacter(uint8_t character) const {
  return character < num_chars_;
}

void Icons4x6Font::getCharacterRange(uint8_t& min_char, uint8_t& max_char) const {
  min_char = 0;  // Always 0-based (ASCII 32 = character 0)
  max_char = num_chars_ - 1;
}

const char* Icons4x6Font::getFontName() const {
  return "Icon Font 4x6";
}

void Icons4x6Font::parseHeader() {
  if (!font_data_) {
    return;
  }
  
  // Read header from PROGMEM
#ifdef ARDUINO
  font_width_ = pgm_read_byte(&font_data_[0]);
  font_height_ = pgm_read_byte(&font_data_[1]);
  start_char_ = pgm_read_byte(&font_data_[2]);
#else
  font_width_ = font_data_[0];
  font_height_ = font_data_[1];
  start_char_ = font_data_[2];
#endif
  
  // Calculate number of characters based on font data size
  // Icon font covers ASCII 32-126 (95 characters)
  num_chars_ = 95;
}
