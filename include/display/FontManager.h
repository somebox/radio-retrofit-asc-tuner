#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include "display/IFont4x6.h"
#include "display/fonts/Bitpacked4x6Font.h"
#include "display/fonts/Icons4x6Font.h"
#include "display/SignTextController.h"  // For Font enum
#include <memory>

/**
 * Font manager that provides access to all available 4x6 fonts
 * 
 * This class manages font instances and provides a clean interface
 * for accessing fonts by the Font enum used throughout the system.
 */
class FontManager {
public:
  FontManager();
  ~FontManager();
  
  /**
   * Get a font instance by Font enum
   * 
   * @param font The font type to retrieve
   * @return Pointer to the font instance, or nullptr if not available
   */
  IFont4x6* getFont(RetroText::Font font);
  
  /**
   * Get the default font (modern font)
   * 
   * @return Pointer to the default font instance
   */
  IFont4x6* getDefaultFont();
  
  /**
   * Check if a font is available
   * 
   * @param font The font type to check
   * @return true if the font is available, false otherwise
   */
  bool isFontAvailable(RetroText::Font font);
  
private:
  std::unique_ptr<Bitpacked4x6Font> modern_font_;
  std::unique_ptr<Bitpacked4x6Font> retro_font_;
  std::unique_ptr<Icons4x6Font> icon_font_;
  
  void initializeFonts();
};

#endif // FONT_MANAGER_H


