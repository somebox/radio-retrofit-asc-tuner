/**
 * RetroText Display Component for ESPHome
 * 
 * Displays text on a 3-board IS31FL3737 LED matrix display.
 * - 3× IS31FL3737 drivers (12×12 matrix each)
 * - Physical layout: 24×6 pixels per board = 72×6 total
 * - Character display: 18 characters (4×6 pixel glyphs)
 * 
 * Based on existing RetroText hardware and IS31FL373x driver library.
 */
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "is31fl3737_driver.h"
#include <array>
#include <memory>

namespace esphome {
namespace retrotext_display {

class RetroTextDisplay : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  // Configuration
  void set_i2c_bus(i2c::I2CBus *bus) { this->i2c_bus_ = bus; }
  void set_brightness(uint8_t brightness);
  void set_board_addresses(uint8_t addr1, uint8_t addr2, uint8_t addr3);
  void set_scroll_delay(uint32_t delay_ms) { this->scroll_delay_ms_ = delay_ms; }
  void set_scroll_mode(uint8_t mode) { this->scroll_mode_ = mode; }

  // Public API
  void set_text(const char *text);
  void set_text_with_brightness(const char *text, uint8_t date_brightness, uint8_t time_brightness, int split_pos);
  void clear();
  
  // Scroll modes
  enum ScrollMode {
    SCROLL_AUTO = 0,    // Scroll only if text > 18 chars
    SCROLL_ALWAYS = 1,  // Always scroll
    SCROLL_NEVER = 2    // Never scroll (truncate)
  };

 protected:
  // Configuration
  i2c::I2CBus *i2c_bus_{nullptr};
  uint8_t brightness_{128};
  std::array<uint8_t, 3> board_addresses_;
  
  // IS31FL3737 drivers (one per board)
  std::array<std::unique_ptr<IS31FL3737Driver>, 3> drivers_;
  
  // Display buffer (72 columns × 6 rows)
  std::array<uint8_t, 72 * 6> buffer_;
  
  // Text buffer (increased to support scrolling longer text)
  static const size_t MAX_TEXT_LENGTH = 128;
  char text_buffer_[MAX_TEXT_LENGTH];
  bool text_dirty_{false};
  
  // Scrolling state
  uint8_t scroll_mode_{SCROLL_AUTO};
  uint32_t scroll_delay_ms_{300};
  uint32_t scroll_start_delay_ms_{1000};  // Wait 1s before starting to scroll
  uint32_t text_set_time_{0};   // When text was last changed
  uint32_t last_scroll_time_{0};
  int scroll_position_{0};
  size_t text_length_{0};
  uint8_t stationary_prefix_chars_{0};  // Number of chars at start that don't scroll
  
  // Internal methods
  bool initialize_boards_();
  void render_text_();
  void update_display_();
  void set_pixel_(int x, int y, uint8_t brightness);
  void draw_character_(uint8_t glyph_index, int x_offset, uint8_t brightness);
  uint8_t get_glyph_row_(uint8_t glyph_index, int row) const;
  
  // Coordinate helpers
  int get_board_for_x_(int x) const;
  int get_local_x_(int x) const;
};

}  // namespace retrotext_display
}  // namespace esphome
