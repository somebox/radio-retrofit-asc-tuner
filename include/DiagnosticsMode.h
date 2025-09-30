#pragma once

#include <Arduino.h>
#include "RadioHardware.h"
#include "Events.h"

/**
 * @brief Interactive serial console for hardware testing and debugging
 * 
 * Provides simple commands for LED control, input monitoring, event monitoring,
 * and system information. Automatically suppresses periodic log messages while active.
 */
class DiagnosticsMode {
public:
  DiagnosticsMode(RadioHardware* hardware, EventBus* event_bus);
  
  /**
   * @brief Initialize diagnostics mode
   */
  void begin();
  
  /**
   * @brief Activate diagnostics mode
   * @param reason Optional reason string (e.g., "Hardware init failed")
   * 
   * Displays welcome message and command prompt.
   * Call on boot failure or when user presses key in Serial Monitor.
   */
  void activate(const char* reason = nullptr);
  
  /**
   * @brief Deactivate diagnostics mode
   * 
   * Returns to normal operation, re-enables periodic logging.
   */
  void deactivate();
  
  /**
   * @brief Process a command from serial input
   * @param command The command string to parse and execute
   * 
   * Called by main.cpp when serial input is available.
   * Main reads the line, passes it here for processing.
   */
  void processCommand(const String& command);
  
  /**
   * @brief Check if diagnostics mode is currently active
   * @return true if active (user is interacting with console)
   * 
   * Use this to suppress periodic log messages:
   * @code
   * #ifdef ENABLE_DIAGNOSTICS
   *   if (!diagnostics.isActive())
   * #endif
   *   {
   *     Serial.printf("FPS: %d\n", fps);
   *   }
   * @endcode
   */
  bool isActive() const { return active_; }

private:
  // Command handlers
  void showWelcome(const char* reason);
  void showHelp();
  void handleLEDCommand(const String& args);
  void handleControlsCommand();
  void handleEventsCommand();
  void handleInfoCommand();
  void handleConfigCommand();
  
  // Event monitoring callback
  static void eventMonitorCallback(const events::Event& evt, void* context);
  
  // Control monitoring callback
  static void controlMonitorCallback(const events::Event& evt, void* context);
  
  // Stop monitoring (called on exit or Ctrl+C detection)
  void stopMonitoring();
  
  // Hardware access
  RadioHardware* hardware_;
  EventBus* event_bus_;
  
  // State
  bool active_;
  unsigned long last_activity_;
  
  // Monitoring flags
  bool monitoring_events_;
  bool monitoring_controls_;
  
  // Auto-exit timeout (disabled - user must exit manually)
  static const unsigned long TIMEOUT_MS = 0;  // 0 = disabled
};

/**
 * @brief Helper macro for conditional logging when diagnostics enabled
 * 
 * Usage:
 * @code
 * LOG_IF_NOT_DIAGNOSTICS("Periodic status message");
 * @endcode
 */
#ifdef ENABLE_DIAGNOSTICS
  extern DiagnosticsMode* g_diagnostics_instance;
  #define LOG_IF_NOT_DIAGNOSTICS(msg) \
    do { \
      if (!g_diagnostics_instance || !g_diagnostics_instance->isActive()) { \
        Serial.println(msg); \
      } \
    } while(0)
#else
  #define LOG_IF_NOT_DIAGNOSTICS(msg) Serial.println(msg)
#endif
