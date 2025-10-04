#include <unity.h>
#include "platform/InputControls.h"

using namespace Input;

void setUp(void) {
  // Set up before each test
}

void tearDown(void) {
  // Clean up after each test
}

// ============================================================================
// ButtonControl Tests
// ============================================================================

void test_button_initial_state() {
  ButtonControl btn;
  TEST_ASSERT_FALSE(btn.isPressed());
  TEST_ASSERT_FALSE(btn.wasJustPressed());
  TEST_ASSERT_FALSE(btn.wasJustReleased());
}

void test_button_press() {
  ButtonControl btn;
  
  btn.onPress(100);
  TEST_ASSERT_TRUE(btn.isPressed());
  TEST_ASSERT_TRUE(btn.wasJustPressed());
  TEST_ASSERT_FALSE(btn.wasJustReleased());
}

void test_button_press_edge_detection() {
  ButtonControl btn;
  
  btn.onPress(100);
  TEST_ASSERT_TRUE(btn.wasJustPressed());  // True on first check
  
  btn.update(150);  // Frame update
  TEST_ASSERT_TRUE(btn.isPressed());       // Still pressed
  TEST_ASSERT_FALSE(btn.wasJustPressed()); // But not "just" pressed anymore
}

void test_button_release() {
  ButtonControl btn;
  
  btn.onPress(100);
  btn.update(150);
  btn.onRelease(200);
  
  TEST_ASSERT_FALSE(btn.isPressed());
  TEST_ASSERT_TRUE(btn.wasJustReleased());
}

void test_button_short_press() {
  ButtonControl btn;
  
  btn.onPress(100);
  btn.onRelease(200);  // 100ms press
  
  TEST_ASSERT_FALSE(btn.wasLongPress(300));  // Not long enough
  TEST_ASSERT_FALSE(btn.isLongPressed(200, 300));
}

void test_button_long_press_detection() {
  ButtonControl btn;
  
  btn.onPress(100);
  
  // Check while holding
  TEST_ASSERT_FALSE(btn.isLongPressed(200, 300));  // 100ms - too short
  TEST_ASSERT_FALSE(btn.isLongPressed(350, 300));  // 250ms - still short
  TEST_ASSERT_TRUE(btn.isLongPressed(400, 300));   // 300ms - long!
  TEST_ASSERT_TRUE(btn.isLongPressed(500, 300));   // 400ms - still long
}

void test_button_was_long_press() {
  ButtonControl btn;
  
  btn.onPress(100);
  btn.onRelease(450);  // Held for 350ms
  
  TEST_ASSERT_TRUE(btn.wasLongPress(300));   // Yes, was long press
  TEST_ASSERT_FALSE(btn.wasLongPress(400));  // But not 400ms
}

void test_button_press_duration() {
  ButtonControl btn;
  
  btn.onPress(100);
  TEST_ASSERT_EQUAL(0, btn.pressDuration(100));
  TEST_ASSERT_EQUAL(50, btn.pressDuration(150));
  TEST_ASSERT_EQUAL(200, btn.pressDuration(300));
  
  btn.onRelease(300);
  TEST_ASSERT_EQUAL(0, btn.pressDuration(400));  // Released = 0 duration
}

// ============================================================================
// EncoderControl Tests
// ============================================================================

void test_encoder_initial_state() {
  EncoderControl enc;
  TEST_ASSERT_EQUAL(0, enc.position());
  TEST_ASSERT_EQUAL(0, enc.delta());
  TEST_ASSERT_FALSE(enc.changed());
}

void test_encoder_forward_turn() {
  EncoderControl enc;
  
  // Full detent clockwise: 00 -> 01 -> 11 -> 10 -> 00
  enc.onChannelPress(false, 100);   // B press: 00 -> 01
  enc.onChannelPress(true, 110);    // A press: 01 -> 11
  enc.onChannelRelease(false, 120); // B release: 11 -> 10
  enc.onChannelRelease(true, 130);  // A release: 10 -> 00
  
  TEST_ASSERT_EQUAL(1, enc.position());
  TEST_ASSERT_EQUAL(1, enc.delta());
  TEST_ASSERT_TRUE(enc.changed());
}

void test_encoder_backward_turn() {
  EncoderControl enc;
  
  // Full detent counter-clockwise: 00 -> 10 -> 11 -> 01 -> 00
  enc.onChannelPress(true, 100);    // A press: 00 -> 10
  enc.onChannelPress(false, 110);   // B press: 10 -> 11
  enc.onChannelRelease(true, 120);  // A release: 11 -> 01
  enc.onChannelRelease(false, 130); // B release: 01 -> 00
  
  TEST_ASSERT_EQUAL(-1, enc.position());
  TEST_ASSERT_EQUAL(-1, enc.delta());
}

void test_encoder_delta_resets() {
  EncoderControl enc;
  
  // Full detent turn
  enc.onChannelPress(false, 100);
  enc.onChannelPress(true, 110);
  enc.onChannelRelease(false, 120);
  enc.onChannelRelease(true, 130);
  TEST_ASSERT_EQUAL(1, enc.delta());
  
  // After update, delta should reset
  enc.update(140);
  TEST_ASSERT_EQUAL(0, enc.delta());
  TEST_ASSERT_EQUAL(1, enc.position());  // Position stays
}

void test_encoder_multiple_turns() {
  EncoderControl enc;
  
  // Turn forward 3 complete detents (each detent = 4 transitions)
  for (int i = 0; i < 3; i++) {
    // Full cycle: 00 -> 01 -> 11 -> 10 -> 00
    enc.onChannelPress(false, 100 + i * 40);   // 00 -> 01
    enc.onChannelPress(true, 110 + i * 40);    // 01 -> 11
    enc.onChannelRelease(false, 120 + i * 40); // 11 -> 10
    enc.onChannelRelease(true, 130 + i * 40);  // 10 -> 00
  }
  
  TEST_ASSERT_EQUAL(3, enc.position());
}

void test_encoder_button() {
  EncoderControl enc;
  
  ButtonControl& btn = enc.button();
  TEST_ASSERT_FALSE(btn.isPressed());
  
  btn.onPress(100);
  TEST_ASSERT_TRUE(btn.isPressed());
  TEST_ASSERT_TRUE(enc.button().isPressed());
}

// ============================================================================
// SwitchControl Tests
// ============================================================================

void test_switch_initial_state() {
  SwitchControl sw(4);  // 4-position switch
  TEST_ASSERT_EQUAL(0, sw.position());
  TEST_ASSERT_FALSE(sw.changed());
  TEST_ASSERT_EQUAL(4, sw.numPositions());
}

void test_switch_set_position() {
  SwitchControl sw(4);
  
  sw.setPosition(2, 100);
  TEST_ASSERT_EQUAL(2, sw.position());
  TEST_ASSERT_TRUE(sw.changed());
}

void test_switch_change_resets() {
  SwitchControl sw(4);
  
  sw.setPosition(1, 100);
  TEST_ASSERT_TRUE(sw.changed());
  
  sw.update(110);
  TEST_ASSERT_FALSE(sw.changed());  // No longer changed after update
  TEST_ASSERT_EQUAL(1, sw.position());  // Position stays
}

void test_switch_invalid_position() {
  SwitchControl sw(4);
  
  sw.setPosition(-1, 100);
  TEST_ASSERT_EQUAL(0, sw.position());  // Should stay at 0
  
  sw.setPosition(4, 110);  // Out of range (0-3 valid)
  TEST_ASSERT_EQUAL(0, sw.position());  // Should stay at 0
}

void test_switch_same_position_no_change() {
  SwitchControl sw(4);
  
  sw.setPosition(1, 100);
  sw.update(110);
  
  sw.setPosition(1, 120);  // Set to same position
  TEST_ASSERT_FALSE(sw.changed());  // Should not register as changed
}

// ============================================================================
// Integration/Scenario Tests
// ============================================================================

void test_button_rapid_press_release() {
  ButtonControl btn;
  
  // Frame 1: Press
  btn.update(100);  // Start of frame
  btn.onPress(100);
  TEST_ASSERT_TRUE(btn.wasJustPressed());
  
  // Frame 2: Release
  btn.update(110);  // Start of frame
  btn.onRelease(110);
  TEST_ASSERT_TRUE(btn.wasJustReleased());
  
  // Frame 3: Press again
  btn.update(120);  // Start of frame
  btn.onPress(120);
  TEST_ASSERT_TRUE(btn.wasJustPressed());  // New press detected
}

void test_encoder_with_button_pressed() {
  EncoderControl enc;
  
  // Press encoder button
  enc.button().onPress(100);
  
  // Full detent turn while button pressed
  enc.onChannelPress(false, 110);
  enc.onChannelPress(true, 120);
  enc.onChannelRelease(false, 130);
  enc.onChannelRelease(true, 140);
  
  TEST_ASSERT_TRUE(enc.button().isPressed());
  TEST_ASSERT_EQUAL(1, enc.position());
}

void test_button_ignores_duplicate_press() {
  ButtonControl btn;
  
  // First press - should work
  bool result1 = btn.onPress(100);
  TEST_ASSERT_TRUE(result1);
  TEST_ASSERT_TRUE(btn.isPressed());
  
  // Second press while already pressed - should be ignored
  bool result2 = btn.onPress(150);
  TEST_ASSERT_FALSE(result2);
  TEST_ASSERT_TRUE(btn.isPressed());  // Still pressed
  
  // Release - should work
  bool result3 = btn.onRelease(200);
  TEST_ASSERT_TRUE(result3);
  TEST_ASSERT_FALSE(btn.isPressed());
}

void test_button_ignores_release_when_not_pressed() {
  ButtonControl btn;
  
  // Release when not pressed - should be ignored
  bool result1 = btn.onRelease(100);
  TEST_ASSERT_FALSE(result1);
  TEST_ASSERT_FALSE(btn.isPressed());
  
  // Press - should work
  bool result2 = btn.onPress(150);
  TEST_ASSERT_TRUE(result2);
  TEST_ASSERT_TRUE(btn.isPressed());
  
  // Release - should work
  bool result3 = btn.onRelease(200);
  TEST_ASSERT_TRUE(result3);
  TEST_ASSERT_FALSE(btn.isPressed());
  
  // Another release - should be ignored
  bool result4 = btn.onRelease(250);
  TEST_ASSERT_FALSE(result4);
  TEST_ASSERT_FALSE(btn.isPressed());
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
  UNITY_BEGIN();
  
  // ButtonControl tests
  RUN_TEST(test_button_initial_state);
  RUN_TEST(test_button_press);
  RUN_TEST(test_button_press_edge_detection);
  RUN_TEST(test_button_release);
  RUN_TEST(test_button_short_press);
  RUN_TEST(test_button_long_press_detection);
  RUN_TEST(test_button_was_long_press);
  RUN_TEST(test_button_press_duration);
  
  // EncoderControl tests
  RUN_TEST(test_encoder_initial_state);
  RUN_TEST(test_encoder_forward_turn);
  RUN_TEST(test_encoder_backward_turn);
  RUN_TEST(test_encoder_delta_resets);
  RUN_TEST(test_encoder_multiple_turns);
  RUN_TEST(test_encoder_button);
  
  // SwitchControl tests
  RUN_TEST(test_switch_initial_state);
  RUN_TEST(test_switch_set_position);
  RUN_TEST(test_switch_change_resets);
  RUN_TEST(test_switch_invalid_position);
  RUN_TEST(test_switch_same_position_no_change);
  
  // Integration tests
  RUN_TEST(test_button_rapid_press_release);
  RUN_TEST(test_encoder_with_button_pressed);
  
  // Invalid state transition handling
  RUN_TEST(test_button_ignores_duplicate_press);
  RUN_TEST(test_button_ignores_release_when_not_pressed);
  
  return UNITY_END();
}

