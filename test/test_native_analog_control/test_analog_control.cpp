/**
 * @file test_analog_control.cpp
 * @brief Unit tests for AnalogControl class
 * 
 * Uses test injection (_test_injectValue) to simulate ADC readings
 * and validate state machine behavior matches buttons/encoders.
 */

#include <unity.h>
#include "platform/InputControls.h"

using namespace Input;

void setUp(void) {
  // Called before each test
}

void tearDown(void) {
  // Called after each test
}

// ============================================================================
// Basic State Tests (matching button/encoder pattern)
// ============================================================================

void test_analog_initial_state() {
  AnalogControl analog(32, 10);
  
  TEST_ASSERT_EQUAL(0, analog.value());
  TEST_ASSERT_EQUAL(0, analog.rawValue());
  TEST_ASSERT_FALSE(analog.changed());
  TEST_ASSERT_EQUAL(0, analog.delta());
}

void test_analog_value_change() {
  AnalogControl analog(32, 10, 100);  // deadzone=10, interval=100ms
  
  // Frame 1: Inject value above deadzone
  analog.update(100);
  analog._test_injectValue(100, 100);
  
  TEST_ASSERT_EQUAL(100, analog.value());
  TEST_ASSERT_TRUE(analog.changed());
  TEST_ASSERT_EQUAL(100, analog.delta());
}

void test_analog_change_detection_resets() {
  AnalogControl analog(32, 10, 100);
  
  // Frame 1: Change value
  analog.update(100);
  analog._test_injectValue(100, 100);
  TEST_ASSERT_TRUE(analog.changed());
  
  // Frame 2: Update syncs state, no new value
  analog.update(110);
  analog._test_injectValue(100, 110);  // Same value
  TEST_ASSERT_FALSE(analog.changed());  // No longer changed
  TEST_ASSERT_EQUAL(100, analog.value());  // Value stays
}

void test_analog_multiple_changes() {
  AnalogControl analog(32, 10, 100);
  
  // Frame 1: First change
  analog.update(100);
  analog._test_injectValue(100, 100);
  TEST_ASSERT_EQUAL(100, analog.value());
  
  // Frame 2: Second change (enough time passed)
  analog.update(210);
  analog._test_injectValue(200, 210);
  TEST_ASSERT_TRUE(analog.changed());
  TEST_ASSERT_EQUAL(200, analog.value());
  TEST_ASSERT_EQUAL(100, analog.delta());
}

// ============================================================================
// Deadzone Tests
// ============================================================================

void test_analog_deadzone_filters_small_changes() {
  AnalogControl analog(32, 50, 0);  // Large deadzone, no time throttle
  
  // Frame 1: Initial value
  analog.update(100);
  analog._test_injectValue(1000, 100);
  TEST_ASSERT_EQUAL(1000, analog.value());
  
  // Frame 2: Small change within deadzone
  analog.update(110);
  analog._test_injectValue(1030, 110);  // +30, under 50 threshold
  TEST_ASSERT_FALSE(analog.changed());
  TEST_ASSERT_EQUAL(1000, analog.value());  // Should not update
  
  // Frame 3: Large change exceeds deadzone
  analog.update(120);
  analog._test_injectValue(1100, 120);  // +100, exceeds 50 threshold
  TEST_ASSERT_TRUE(analog.changed());
  TEST_ASSERT_EQUAL(1100, analog.value());  // Should update
}

// ============================================================================
// Time Throttling Tests
// ============================================================================

void test_analog_time_throttle_prevents_rapid_updates() {
  AnalogControl analog(32, 10, 150);  // Small deadzone, 150ms throttle
  
  // Frame 1: Initial change
  analog.update(100);
  analog._test_injectValue(1000, 100);
  TEST_ASSERT_EQUAL(1000, analog.value());
  
  // Frame 2: Try to change too soon (50ms later)
  analog.update(150);
  analog._test_injectValue(2000, 150);  // Big change, but too soon
  TEST_ASSERT_FALSE(analog.changed());
  TEST_ASSERT_EQUAL(1000, analog.value());  // Throttled
  
  // Frame 3: After throttle period (200ms total)
  analog.update(300);
  analog._test_injectValue(2000, 300);  // Same new value, now accepted
  TEST_ASSERT_TRUE(analog.changed());
  TEST_ASSERT_EQUAL(2000, analog.value());  // Updated
}

// ============================================================================
// Value Conversion Tests
// ============================================================================

void test_analog_conversions() {
  AnalogControl analog(32, 10, 0);
  
  // Inject known values and test conversions
  analog.update(100);
  analog._test_injectValue(4095, 100);  // 12-bit max
  
  TEST_ASSERT_EQUAL(255, analog.valueAsByte());  // Should convert to 8-bit max
  TEST_ASSERT_EQUAL(100, analog.valueAsPercent());  // Should be 100%
}

void test_analog_conversions_mid_range() {
  AnalogControl analog(32, 10, 0);
  
  analog.update(100);
  analog._test_injectValue(2048, 100);  // ~50% of 4095
  
  TEST_ASSERT_EQUAL(127, analog.valueAsByte());  // ~50% of 255
  TEST_ASSERT_EQUAL(50, analog.valueAsPercent());  // 50%
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
  UNITY_BEGIN();
  
  // Basic state tests
  RUN_TEST(test_analog_initial_state);
  RUN_TEST(test_analog_value_change);
  RUN_TEST(test_analog_change_detection_resets);
  RUN_TEST(test_analog_multiple_changes);
  
  // Noise reduction tests
  RUN_TEST(test_analog_deadzone_filters_small_changes);
  RUN_TEST(test_analog_time_throttle_prevents_rapid_updates);
  
  // Conversion tests
  RUN_TEST(test_analog_conversions);
  RUN_TEST(test_analog_conversions_mid_range);
  
  return UNITY_END();
}

