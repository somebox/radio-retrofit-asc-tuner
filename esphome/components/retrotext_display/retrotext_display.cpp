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
  
  // Display startup message with shimmer effect
  this->set_text("CONNECTING...");
  this->set_shimmer_mode(true);
}

void RetroTextDisplay::loop() {
  // Update shimmer animation phase
  if (this->shimmer_enabled_) {
    this->shimmer_phase_ += 0.15f;  // Animation speed
    if (this->shimmer_phase_ >= 6.28318530718f) {  // 2*PI
      this->shimmer_phase_ -= 6.28318530718f;
    }
    // Force display update when shimmering
    this->update_display_();
  }
  
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
    
    // Wait scroll_start_delay_ms before starting to scroll (gives user time to read)
    if (now - this->text_set_time_ < this->scroll_start_delay_ms_) {
      return;  // Don't scroll yet
    }
    
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
  
  // Detect stationary prefix (icon + space = 2 chars)
  // Check if text starts with play (128) or stop (129) icon
  this->stationary_prefix_chars_ = 0;
  if (this->text_length_ >= 2) {
    uint8_t first_byte = (uint8_t)this->text_buffer_[0];
    if ((first_byte == 128 || first_byte == 129) && this->text_buffer_[1] == ' ') {
      this->stationary_prefix_chars_ = 2;  // Icon + space stay put
    }
  }
  
  // Reset scroll position and timing when text changes
  this->scroll_position_ = 0;
  uint32_t now = millis();
  this->text_set_time_ = now;      // Record when text was set
  this->last_scroll_time_ = now;   // Reset scroll timer
  
  this->text_dirty_ = true;
  
  ESP_LOGD(TAG, "Set text: '%s' (prefix_chars=%d)", this->text_buffer_, this->stationary_prefix_chars_);
}

void RetroTextDisplay::set_text_with_brightness(const char *text, uint8_t date_brightness, uint8_t time_brightness, int split_pos) {
  if (text == nullptr) {
    return;
  }
  
  // Clear the text buffer and framebuffer
  memset(this->text_buffer_, 0, sizeof(this->text_buffer_));
  this->buffer_.fill(0);
  
  // Copy text to buffer
  strncpy(this->text_buffer_, text, MAX_TEXT_LENGTH - 1);
  this->text_buffer_[MAX_TEXT_LENGTH - 1] = '\0';
  this->text_length_ = strlen(this->text_buffer_);
  
  // Render with variable brightness and UTF-8 support
  int x_pos = 0;
  size_t byte_pos = 0;
  int char_count = 0;
  
  while (byte_pos < this->text_length_ && char_count < 18) {
    size_t bytes_consumed = 0;
    uint8_t glyph = map_utf8_to_glyph(&this->text_buffer_[byte_pos], bytes_consumed);
    
    uint8_t char_brightness = (char_count < split_pos) ? date_brightness : time_brightness;
    this->draw_character_(glyph, x_pos, char_brightness);
    x_pos += 4;
    
    byte_pos += bytes_consumed;
    char_count++;
  }
  
  // Update display immediately (bypass normal render cycle)
  this->update_display_();
  
  // Reset scroll state
  this->scroll_position_ = 0;
  uint32_t now = millis();
  this->text_set_time_ = now;
  this->last_scroll_time_ = now;
  
  ESP_LOGV(TAG, "Set text with brightness: '%s' (date:%d, time:%d, split:%d)", 
           this->text_buffer_, date_brightness, time_brightness, split_pos);
}

void RetroTextDisplay::set_brightness(uint8_t brightness) {
  this->brightness_ = brightness;
  // Update all drivers with new brightness
  for (size_t i = 0; i < 3; i++) {
    if (this->drivers_[i] && this->drivers_[i]->is_initialized()) {
      this->drivers_[i]->set_global_current(brightness);
    }
  }
  ESP_LOGD(TAG, "Brightness set to: %d", brightness);
}

void RetroTextDisplay::clear() {
  this->buffer_.fill(0);
  this->text_buffer_[0] = '\0';
  this->update_display_();
  ESP_LOGD(TAG, "Display cleared");
}

void RetroTextDisplay::set_shimmer_mode(bool enabled) {
  this->shimmer_enabled_ = enabled;
  if (enabled) {
    this->shimmer_phase_ = 0.0f;  // Reset phase
    ESP_LOGD(TAG, "Shimmer mode enabled");
  } else {
    ESP_LOGD(TAG, "Shimmer mode disabled");
    // Force one final update with normal brightness
    this->update_display_();
  }
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
  
  // Check if we should scroll (accounting for stationary prefix)
  size_t scrollable_length = this->text_length_ - this->stationary_prefix_chars_;
  size_t available_display_chars = 18 - this->stationary_prefix_chars_;
  
  bool should_scroll = false;
  if (this->scroll_mode_ == SCROLL_ALWAYS && scrollable_length > 0) {
    should_scroll = true;
  } else if (this->scroll_mode_ == SCROLL_AUTO && scrollable_length > available_display_chars) {
    should_scroll = true;
  }
  
  int x_pos = 0;
  
  // FIRST: Render stationary prefix (icon + space) if present
  if (this->stationary_prefix_chars_ > 0) {
    for (uint8_t i = 0; i < this->stationary_prefix_chars_ && i < this->text_length_; i++) {
      size_t bytes_consumed = 0;
      uint8_t glyph = map_utf8_to_glyph(&this->text_buffer_[i], bytes_consumed);
      this->draw_character_(glyph, x_pos, this->brightness_);
      x_pos += 4;
    }
  }
  
  // SECOND: Render scrollable portion
  if (should_scroll && scrollable_length > 0) {
    // Scrolling text - render after the prefix
    // Scrollable text starts at stationary_prefix_chars_ offset in buffer
    for (size_t display_pos = 0; display_pos < available_display_chars; display_pos++) {
      // Calculate position in scrollable portion (with wraparound)
      int text_pos = (this->scroll_position_ + display_pos) % (scrollable_length + 3);
      
      uint8_t glyph;
      if (text_pos < (int)scrollable_length) {
        // Character from scrollable text
        size_t bytes_consumed = 0;
        size_t buffer_pos = this->stationary_prefix_chars_ + text_pos;
        glyph = map_utf8_to_glyph(&this->text_buffer_[buffer_pos], bytes_consumed);
      } else {
        // Separator between scroll cycles: " * "
        int separator_pos = text_pos - scrollable_length;
        if (separator_pos == 0 || separator_pos == 2) {
          glyph = ' ';
        } else {
          glyph = '*';
        }
      }
      
      this->draw_character_(glyph, x_pos, this->brightness_);
      x_pos += 4;
    }
  } else {
    // Static display - no scrolling, render remaining chars after prefix
    size_t byte_pos = this->stationary_prefix_chars_;
    int char_count = this->stationary_prefix_chars_;
    
    while (byte_pos < this->text_length_ && char_count < 18) {
      size_t bytes_consumed = 0;
      uint8_t glyph = map_utf8_to_glyph(&this->text_buffer_[byte_pos], bytes_consumed);
      
      this->draw_character_(glyph, x_pos, this->brightness_);
      x_pos += 4;
      
      byte_pos += bytes_consumed;
      char_count++;
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
      
      // Apply shimmer effect if enabled
      if (this->shimmer_enabled_ && pixel_brightness > 0) {
        // Calculate wave position: sine wave moving from left to right
        // with vertical offset to create angled wave effect
        float wave_position = (float)x / 72.0f * 12.56637061436f;  // 2 full periods across display
        float vertical_offset = (float)y * 0.523598775598f;  // PI/6 offset per row for angle
        float wave = sinf(wave_position + vertical_offset - this->shimmer_phase_);
        
        // Modulate brightness: 0.6 to 1.4 multiplier (keeps pixels visible but creates wave)
        float brightness_mult = 1.0f + (wave * 0.4f);
        pixel_brightness = (uint8_t)(pixel_brightness * brightness_mult);
        
        // Clamp to valid range
        if (pixel_brightness > 255) pixel_brightness = 255;
      }
      
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

void RetroTextDisplay::draw_character_(uint8_t glyph_index, int x_offset, uint8_t brightness) {
  // Draw a 4×6 character starting at x_offset
  // Glyph index should be pre-mapped using map_utf8_to_glyph()
  for (int row = 0; row < 6; row++) {
    uint8_t glyph_row = this->get_glyph_row_(glyph_index, row);
    
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

uint8_t RetroTextDisplay::get_glyph_row_(uint8_t glyph_index, int row) const {
  // Font format: [width, height, first_char, ...glyph_data...]
  // modern_font4x6[0] = 4 (width)
  // modern_font4x6[1] = 6 (height)
  // modern_font4x6[2] = 32 (first ASCII char - space)
  
  // Bounds check
  if (row < 0 || row >= 6) {
    return 0x00;
  }
  
  // Check if character is in font range
  // Range: 32-126 (standard ASCII) + 128-159 (extended)
  if (glyph_index < 32 || (glyph_index > 126 && glyph_index < 128) || glyph_index > 159) {
    return 0x00;  // Return blank for unsupported characters
  }
  
  // Calculate offset in font array
  // Header is 3 bytes, then 6 bytes per character
  uint8_t char_index;
  if (glyph_index <= 126) {
    // Standard ASCII range (32-126)
    char_index = glyph_index - 32;  // Offset from first char (space)
  } else {
    // Extended range (128-159)
    // After ASCII 32-126 (95 chars), we have 127 (1 char), then 128+ (extended)
    char_index = 95 + (glyph_index - 127);  // 95 ASCII chars, then extended
  }
  
  uint16_t glyph_offset = 3 + (char_index * 6) + row;
  
  // Return the row data
  return modern_font4x6[glyph_offset];
}

}  // namespace retrotext_display
}  // namespace esphome
