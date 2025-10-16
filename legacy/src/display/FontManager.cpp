#include "display/FontManager.h"
#include <display/fonts/modern_font4x6.h>
#include <display/fonts/retro_font4x6.h>
#include <display/fonts/icon_font4x6.h>

FontManager::FontManager() {
  initializeFonts();
}

FontManager::~FontManager() {
  // Unique pointers handle cleanup automatically
}

IFont4x6* FontManager::getFont(RetroText::Font font) {
  switch (font) {
    case RetroText::MODERN_FONT:
      return modern_font_.get();
    case RetroText::ARDUBOY_FONT:
      return retro_font_.get();
    case RetroText::ICON_FONT:
      return icon_font_.get();
    default:
      return getDefaultFont();
  }
}

IFont4x6* FontManager::getDefaultFont() {
  return modern_font_.get();
}

bool FontManager::isFontAvailable(RetroText::Font font) {
  return getFont(font) != nullptr;
}

void FontManager::initializeFonts() {
  // Initialize modern font
  modern_font_ = std::make_unique<Bitpacked4x6Font>(modern_font4x6, "Modern 4x6");
  
  // Initialize retro font  
  retro_font_ = std::make_unique<Bitpacked4x6Font>(retro_font4x6, "Retro 4x6");
  
  // Initialize icon font
  icon_font_ = std::make_unique<Icons4x6Font>(icon_font4x6);
}


