#include "AnnouncementModule.h"

// Forward declaration of display function from main.cpp
extern void display_static_message(String message, bool use_modern_font = true, int display_time_ms = 0);

AnnouncementModule::AnnouncementModule(DisplayManager* display_manager)
  : display_manager_(display_manager), state_(ANNOUNCEMENT_IDLE), 
    start_time_(0), display_duration_(1000), is_active_(false) {
}

void AnnouncementModule::show(const String& text, unsigned long duration_ms) {
  // If already showing the same text, don't restart timer
  if (state_ == ANNOUNCEMENT_ACTIVE && current_text_ == text) {
    return;
  }

  // Clear any existing announcement first
  clear();

  current_text_ = text;
  display_duration_ = duration_ms;
  start_time_ = millis();
  state_ = ANNOUNCEMENT_ACTIVE;
  is_active_ = true;

  Serial.printf("Announcement: %s (%lums)\n", text.c_str(), duration_ms);
}

void AnnouncementModule::hold() {
  // Reset timer if announcement is active and has a duration > 0
  if (state_ == ANNOUNCEMENT_ACTIVE && display_duration_ > 0) {
    start_time_ = millis();
  }
}

void AnnouncementModule::setDuration(unsigned long duration_ms) {
  // Change duration and reset timer if announcement is active
  if (state_ == ANNOUNCEMENT_ACTIVE) {
    display_duration_ = duration_ms;
    start_time_ = millis();
  }
}

void AnnouncementModule::update() {
  if (state_ == ANNOUNCEMENT_IDLE) return;

  unsigned long elapsed = millis() - start_time_;

  switch (state_) {
    case ANNOUNCEMENT_ACTIVE:
      // Display the announcement text using DisplayManager directly
      if (display_manager_) {
        display_manager_->displayStaticText(current_text_, true);
      }

      // Timeout if elapsed time exceeded (all durations are now > 0)
      if (elapsed >= display_duration_) {
        state_ = ANNOUNCEMENT_TIMEOUT;
        is_active_ = false;
        Serial.printf("Announcement timeout: %s\n", current_text_.c_str());
      }
      break;

    case ANNOUNCEMENT_TIMEOUT:
      state_ = ANNOUNCEMENT_IDLE;
      break;

    default:
      break;
  }
}

void AnnouncementModule::clear() {
  state_ = ANNOUNCEMENT_IDLE;
  is_active_ = false;
  current_text_ = "";
}
