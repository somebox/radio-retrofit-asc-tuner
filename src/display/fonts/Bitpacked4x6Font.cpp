#include "display/fonts/Bitpacked4x6Font.h"

Bitpacked4x6Font::Bitpacked4x6Font(const unsigned char* font_data, const char* font_name)
  : font_data_(font_data)
  , font_name_(font_name)
  , font_width_(0)
  , font_height_(0)
  , start_char_(0)
  , num_chars_(0)
{
  parseHeader();
}

uint8_t Bitpacked4x6Font::getCharacterPattern(uint8_t character, uint8_t row) const {
  if (!hasCharacter(character) || row >= 6) {
    return 0;
  }
  
  // Calculate offset in font data
  // Header is 3 bytes: width, height, start_char
  // Each character is 6 bytes (6 rows)
  int char_offset = 3 + (character * 6) + row;
  
  // Read from PROGMEM and extract upper 4 bits
#ifdef ARDUINO
  uint8_t pattern = pgm_read_byte(&font_data_[char_offset]);
#else
  uint8_t pattern = font_data_[char_offset];
#endif
  return (pattern >> 4) & 0x0F;
}

bool Bitpacked4x6Font::hasCharacter(uint8_t character) const {
  return character < num_chars_;
}

void Bitpacked4x6Font::getCharacterRange(uint8_t& min_char, uint8_t& max_char) const {
  min_char = 0;  // Always 0-based (ASCII 32 = character 0)
  max_char = num_chars_ - 1;
}

const char* Bitpacked4x6Font::getFontName() const {
  return font_name_;
}

void Bitpacked4x6Font::parseHeader() {
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
  // This is a bit tricky since we don't have the size directly
  // We'll use a reasonable default for ASCII fonts (95 characters: 32-126)
  num_chars_ = 95;
}
