#ifndef IFONT4X6_H
#define IFONT4X6_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <stdint.h>
#endif

/**
 * Abstract interface for 4x6 pixel fonts
 * 
 * All fonts in this system are 4 pixels wide by 6 pixels tall.
 * Characters are stored as 6 rows of bit patterns, with 4 bits per row
 * representing the pixel data (1 = pixel on, 0 = pixel off).
 */
class IFont4x6 {
public:
  virtual ~IFont4x6() = default;
  
  /**
   * Get the bit pattern for a specific row of a character
   * 
   * @param character ASCII character code (0-based, where 0 = ASCII 32 space)
   * @param row Row index (0-5, where 0 is top row)
   * @return 8-bit value where the lower 4 bits contain the pixel pattern
   */
  virtual uint8_t getCharacterPattern(uint8_t character, uint8_t row) const = 0;
  
  /**
   * Get the complete 6-row pattern for a character
   * 
   * @param character ASCII character code (0-based, where 0 = ASCII 32 space)
   * @param pattern Output array of 6 bytes to store the pattern
   */
  virtual void getCharacterGlyph(uint8_t character, uint8_t pattern[6]) const {
    for (int row = 0; row < 6; row++) {
      pattern[row] = getCharacterPattern(character, row);
    }
  }
  
  /**
   * Check if a character is supported by this font
   * 
   * @param character ASCII character code (0-based, where 0 = ASCII 32 space)
   * @return true if the character is supported, false otherwise
   */
  virtual bool hasCharacter(uint8_t character) const = 0;
  
  /**
   * Get the ASCII range supported by this font
   * 
   * @param min_char Reference to store minimum supported character code
   * @param max_char Reference to store maximum supported character code
   */
  virtual void getCharacterRange(uint8_t& min_char, uint8_t& max_char) const = 0;
  
  /**
   * Get the font name for debugging/identification
   * 
   * @return Font name as a C string
   */
  virtual const char* getFontName() const = 0;
};

#endif // IFONT4X6_H
