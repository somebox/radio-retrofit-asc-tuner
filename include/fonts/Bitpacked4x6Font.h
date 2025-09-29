#ifndef BITPACKED4X6FONT_H
#define BITPACKED4X6FONT_H

#include "IFont4x6.h"

/**
 * Adapter for bitpacked 4x6 fonts stored in PROGMEM
 * 
 * This adapter works with fonts stored in the format:
 * - Header: width (4), height (6), start_char (32)
 * - Data: 6 bytes per character, with bit patterns in upper 4 bits
 * 
 * Used for modern_font4x6 and retro_font4x6.
 */
class Bitpacked4x6Font : public IFont4x6 {
public:
  /**
   * Constructor
   * 
   * @param font_data Pointer to PROGMEM font data
   * @param font_name Name of the font for identification
   */
  Bitpacked4x6Font(const unsigned char* font_data, const char* font_name);
  
  // IFont4x6 interface implementation
  uint8_t getCharacterPattern(uint8_t character, uint8_t row) const override;
  bool hasCharacter(uint8_t character) const override;
  void getCharacterRange(uint8_t& min_char, uint8_t& max_char) const override;
  const char* getFontName() const override;
  
private:
  const unsigned char* font_data_;
  const char* font_name_;
  uint8_t font_width_;
  uint8_t font_height_;
  uint8_t start_char_;
  uint8_t num_chars_;
  
  void parseHeader();
};

#endif // BITPACKED4X6FONT_H


