#pragma once

#include "display/DisplayManager.h"

enum AnnouncementState {
  ANNOUNCEMENT_IDLE,
  ANNOUNCEMENT_ACTIVE,
  ANNOUNCEMENT_TIMEOUT
};

class AnnouncementModule {
private:
  DisplayManager* display_manager_;
  AnnouncementState state_;
  String current_text_;
  unsigned long start_time_;
  unsigned long display_duration_;
  bool is_active_;

public:
  AnnouncementModule(DisplayManager* display_manager);
  
  void show(const String& text, unsigned long duration_ms = 1000);
  void hold();  // Keep announcement active (reset timer if duration > 0)
  void setDuration(unsigned long duration_ms);  // Change current duration
  void update();
  bool isActive() const { return is_active_; }
  void clear();
};
