/**
 * RetroText Display Component for ESPHome
 * Implementation
 */
#include "retrotext_display.h"
#include "font_4x6.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace retrotext_display {

static const char *const TAG = "retrotext_display";

void RetroTextDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Setting up RetroText Display...");
  
  // Clear buffers
  this->buffer_.fill(0);
  this->text_buffer_[0] = '\0';
  this->text_dirty_ = false;
  
  // Initialize all 3 IS31FL3737 boards
  if (!this->initialize_boards_()) {
    ESP_LOGE(TAG, "Failed to initialize IS31FL3737 boards");
    this->mark_failed();
    return;
  }
  
  ESP_LOGCONFIG(TAG, "RetroText Display initialized successfully");
  
  // Display startup message
  this->set_text("CONNECTING...");
}

void RetroTextDisplay::loop() {
  // Render text if it changed
  if (this->text_dirty_) {
    this->render_text_();
    this->update_display_();
    this->text_dirty_ = false;
  }
  
  // Handle scrolling for long text
  bool should_scroll = false;
  if (this->scroll_mode_ == SCROLL_ALWAYS) {
    should_scroll = true;
  } else if (this->scroll_mode_ == SCROLL_AUTO && this->text_length_ > 18) {
    should_scroll = true;
  }
  
  if (should_scroll) {
    uint32_t now = millis();
    if (now - this->last_scroll_time_ >= this->scroll_delay_ms_) {
      this->last_scroll_time_ = now;
      
      // Advance scroll position
      this->scroll_position_++;
      
      // Calculate max scroll position (text length + 3 spaces for wrap-around)
      int max_position = this->text_length_ + 3;
      if (this->scroll_position_ >= max_position) {
        this->scroll_position_ = 0;
      }
      
      // Re-render at new position
      this->render_text_();
      this->update_display_();
    }
  }
}

void RetroTextDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "RetroText Display:");
  ESP_LOGCONFIG(TAG, "  Board 1 Address: 0x%02X", this->board_addresses_[0]);
  ESP_LOGCONFIG(TAG, "  Board 2 Address: 0x%02X", this->board_addresses_[1]);
  ESP_LOGCONFIG(TAG, "  Board 3 Address: 0x%02X", this->board_addresses_[2]);
  ESP_LOGCONFIG(TAG, "  Brightness: %d", this->brightness_);
  ESP_LOGCONFIG(TAG, "  Resolution: 72×6 pixels (18 characters)");
  
  const char *scroll_mode_str = "unknown";
  if (this->scroll_mode_ == SCROLL_AUTO) scroll_mode_str = "auto";
  else if (this->scroll_mode_ == SCROLL_ALWAYS) scroll_mode_str = "always";
  else if (this->scroll_mode_ == SCROLL_NEVER) scroll_mode_str = "never";
  ESP_LOGCONFIG(TAG, "  Scroll Mode: %s", scroll_mode_str);
  ESP_LOGCONFIG(TAG, "  Scroll Delay: %dms", this->scroll_delay_ms_);
  
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  FAILED - Communication error");
  }
}

float RetroTextDisplay::get_setup_priority() const {
  // Initialize after I2C
  return setup_priority::DATA;
}

void RetroTextDisplay::set_board_addresses(uint8_t addr1, uint8_t addr2, uint8_t addr3) {
  this->board_addresses_[0] = addr1;
  this->board_addresses_[1] = addr2;
  this->board_addresses_[2] = addr3;
}

void RetroTextDisplay::set_text(const char *text) {
  if (text == nullptr) {
    return;
  }
  
  // Clear the text buffer first
  memset(this->text_buffer_, 0, sizeof(this->text_buffer_));
  
  // Copy text to buffer (up to MAX_TEXT_LENGTH)
  strncpy(this->text_buffer_, text, MAX_TEXT_LENGTH - 1);
  this->text_buffer_[MAX_TEXT_LENGTH - 1] = '\0';
  
  // Calculate actual text length
  this->text_length_ = strlen(this->text_buffer_);
  
  // Reset scroll position when text changes
  this->scroll_position_ = 0;
  this->last_scroll_time_ = millis();
  
  this->text_dirty_ = true;
  
  ESP_LOGD(TAG, "Set text: '%s'", this->text_buffer_);
}

void RetroTextDisplay::clear() {
  this->buffer_.fill(0);
  this->text_buffer_[0] = '\0';
  this->update_display_();
  ESP_LOGD(TAG, "Display cleared");
}

bool RetroTextDisplay::initialize_boards_() {
  ESP_LOGD(TAG, "Initializing IS31FL3737 boards...");
  
  if (this->i2c_bus_ == nullptr) {
    ESP_LOGE(TAG, "I2C bus not set");
    return false;
  }
  
  // Initialize each driver
  for (size_t i = 0; i < 3; i++) {
    uint8_t addr = this->board_addresses_[i];
    
    // Create driver instance (using new instead of make_unique for C++11 compatibility)
    this->drivers_[i].reset(new IS31FL3737Driver());
    
    // Initialize the driver
    if (!this->drivers_[i]->begin(addr, this->i2c_bus_)) {
      ESP_LOGE(TAG, "Failed to initialize board %d at address 0x%02X", i + 1, addr);
      return false;
    }
    
    // Set brightness/current
    this->drivers_[i]->set_global_current(this->brightness_ / 2);  // Scale down for current control
    
    ESP_LOGD(TAG, "Board %d at 0x%02X: initialized", i + 1, addr);
  }
  
  return true;
}

void RetroTextDisplay::render_text_() {
  // Clear buffer
  this->buffer_.fill(0);
  
  // Check if we should scroll
  bool should_scroll = false;
  if (this->scroll_mode_ == SCROLL_ALWAYS) {
    should_scroll = true;
  } else if (this->scroll_mode_ == SCROLL_AUTO && this->text_length_ > 18) {
    should_scroll = true;
  }
  
  int x_pos = 0;
  
  if (should_scroll) {
    // Render 18 characters starting from scroll_position_ with wraparound
    for (int display_pos = 0; display_pos < 18; display_pos++) {
      // Calculate source position in text buffer (with wraparound)
      int text_pos = (this->scroll_position_ + display_pos) % (this->text_length_ + 3);
      
      char ch;
      if (text_pos < (int)this->text_length_) {
        // Character from actual text
        ch = this->text_buffer_[text_pos];
      } else {
        // Spacing between scroll cycles (3 spaces)
        ch = ' ';
      }
      
      this->draw_character_(ch, x_pos, this->brightness_);
      x_pos += 4;  // 4 pixels per character width
    }
  } else {
    // Static display - no scrolling, just render the text as-is
    for (size_t i = 0; i < this->text_length_ && i < 18; i++) {
      this->draw_character_(this->text_buffer_[i], x_pos, this->brightness_);
      x_pos += 4;  // 4 pixels per character width
    }
  }
}

void RetroTextDisplay::update_display_() {
  // Push buffer to all 3 IS31FL3737 boards
  // Using coordinate mapping from working DisplayManager.cpp
  
  // Clear all driver PWM buffers first
  for (int i = 0; i < 3; i++) {
    if (this->drivers_[i] && this->drivers_[i]->is_initialized()) {
      this->drivers_[i]->clear();
    }
  }
  
  // Process all pixels and determine board AFTER coordinate flip
  for (int y = 0; y < 6; y++) {
    for (int x = 0; x < 72; x++) {
      int buffer_index = y * 72 + x;
      uint8_t pixel_brightness = this->buffer_[buffer_index];
      
      // Write ALL pixels (including zeros) to properly clear the display
      
      // Apply coordinate flip (from DisplayManager.cpp line 131-132)
      // Display is mounted upside down, so flip both X and Y
      int screen_x = (72 - x - 1);  // Flip X across entire display
      int screen_y = (6 - y - 1);   // Flip Y
      
      // Determine which board AFTER coordinate flip (from DisplayManager.cpp line 135)
      int board = screen_x / 24;
      if (board < 0 || board >= 3 || !this->drivers_[board] || !this->drivers_[board]->is_initialized()) {
        continue;
      }
      
      // Calculate local coordinates within the board
      int local_x = screen_x % 24;  // 0-23 within board
      int local_y = screen_y;       // 0-5
      
      // Convert 24×6 logical to 12×12 physical (from DisplayManager.cpp line 144-157)
      // RetroText PCB layout: 6 characters in a row
      // - Characters 0,1,2 use SW1-6 (top half: CS1-4, CS5-8, CS9-12)
      // - Characters 3,4,5 use SW7-12 (bottom half: CS1-4, CS5-8, CS9-12)
      int char_index = local_x / 4;      // Which character (0-5) within board
      int char_pixel_x = local_x % 4;    // Pixel within character (0-3)
      
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
      
      this->drivers_[board]->set_pixel(physical_x, physical_y, pixel_brightness);
    }
  }
  
  // Push all boards to hardware
  for (size_t board = 0; board < 3; board++) {
    if (this->drivers_[board] && this->drivers_[board]->is_initialized()) {
      this->drivers_[board]->show();
    }
  }
  
  // ESP_LOGD(TAG, "Display updated");
}

void RetroTextDisplay::set_pixel_(int x, int y, uint8_t brightness) {
  // Bounds check
  if (x < 0 || x >= 72 || y < 0 || y >= 6) {
    return;
  }
  
  // Set pixel in buffer
  int index = y * 72 + x;
  this->buffer_[index] = brightness;
}

int RetroTextDisplay::get_board_for_x_(int x) const {
  return x / 24;  // 0-23=board0, 24-47=board1, 48-71=board2
}

int RetroTextDisplay::get_local_x_(int x) const {
  return x % 24;  // Local x within the board
}

void RetroTextDisplay::draw_character_(uint8_t ascii_char, int x_offset, uint8_t brightness) {
  // Draw a 4×6 character starting at x_offset
  // Character positioning logic from DisplayManager.cpp line 215
  for (int row = 0; row < 6; row++) {
    uint8_t glyph_row = this->get_glyph_row_(ascii_char, row);
    
    // Draw 4 pixels for this row with bit reversal
    for (int col = 0; col < 4; col++) {
      // Check if bit is set (bits 7-4 in the font data)
      if (glyph_row & (0x10 << col)) {
        // Apply bit reversal: 3-col (from DisplayManager.cpp)
        int x_pos = x_offset + (3 - col);
        if (x_pos >= 0 && x_pos < 72) {
          this->set_pixel_(x_pos, row, brightness);
        }
      }
    }
  }
}

uint8_t RetroTextDisplay::get_glyph_row_(uint8_t ascii_char, int row) const {
  // Font format: [width, height, first_char, ...glyph_data...]
  // modern_font4x6[0] = 4 (width)
  // modern_font4x6[1] = 6 (height)
  // modern_font4x6[2] = 32 (first ASCII char - space)
  
  // Bounds check
  if (row < 0 || row >= 6) {
    return 0x00;
  }
  
  // Check if character is in font range (ASCII 32-126)
  if (ascii_char < 32 || ascii_char > 126) {
    return 0x00;  // Return blank for unsupported characters
  }
  
  // Calculate offset in font array
  // Header is 3 bytes, then 6 bytes per character
  uint8_t char_index = ascii_char - 32;  // Offset from first char (space)
  uint16_t glyph_offset = 3 + (char_index * 6) + row;
  
  // Return the row data
  return modern_font4x6[glyph_offset];
}

}  // namespace retrotext_display
}  // namespace esphome
