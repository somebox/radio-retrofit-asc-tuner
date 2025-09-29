#ifndef ICONS4X6FONT_H
#define ICONS4X6FONT_H

#include "IFont4x6.h"

/**
 * Adapter for icon 4x6 font stored in PROGMEM
 * 
 * This adapter works with icon fonts that map symbols to ASCII characters:
 * - '!' -> ♪ (Musical note)
 * - '"' -> ♫ (Beamed eighth notes)  
 * - '#' -> ► (Play symbol)
 * - '$' -> ⏸ (Pause symbol)
 * - '%' -> ⏹ (Stop symbol)
 * - '&' -> ⏪ (Fast backward)
 * - '\'' -> ⏩ (Fast forward)
 * - '(' -> 🔊 (Speaker high volume)
 * - ')' -> 🔇 (Speaker muted)
 * - '*' -> ★ (Star/favorite)
 * - '<' -> ◀ (Previous track)
 * - '>' -> ▶ (Next track)
 * 
 * Numbers 0-9 and basic punctuation are also supported.
 */
class Icons4x6Font : public IFont4x6 {
public:
  /**
   * Constructor
   * 
   * @param font_data Pointer to PROGMEM icon font data
   */
  Icons4x6Font(const unsigned char* font_data);
  
  // IFont4x6 interface implementation
  uint8_t getCharacterPattern(uint8_t character, uint8_t row) const override;
  bool hasCharacter(uint8_t character) const override;
  void getCharacterRange(uint8_t& min_char, uint8_t& max_char) const override;
  const char* getFontName() const override;
  
private:
  const unsigned char* font_data_;
  uint8_t font_width_;
  uint8_t font_height_;
  uint8_t start_char_;
  uint8_t num_chars_;
  
  void parseHeader();
};

#endif // ICONS4X6FONT_H


