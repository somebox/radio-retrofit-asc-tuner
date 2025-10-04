#include <unity.h>
#include "hardware/HardwareConfig.h"

using namespace HardwareConfig;

// Test that all preset buttons have valid configurations
void test_preset_buttons_configured() {
    TEST_ASSERT_EQUAL(8, NUM_PRESETS);
    
    // Verify each button has a name, row, and column
    for (int i = 0; i < NUM_PRESETS; i++) {
        TEST_ASSERT_NOT_NULL(PRESET_BUTTONS[i].name);
        TEST_ASSERT_EQUAL(PRESET_BUTTON_ROW, PRESET_BUTTONS[i].row);
        TEST_ASSERT_TRUE(PRESET_BUTTONS[i].col >= 0);
        TEST_ASSERT_TRUE(PRESET_BUTTONS[i].col < KEYPAD_COLS);
    }
}

// Test that all preset LEDs have valid configurations
void test_preset_leds_configured() {
    // Verify each LED has valid SW/CS pins
    for (int i = 0; i < NUM_PRESETS; i++) {
        TEST_ASSERT_TRUE(PRESET_LEDS[i].sw_pin >= 0);
        TEST_ASSERT_TRUE(PRESET_LEDS[i].sw_pin < LED_MATRIX_ROWS);
        TEST_ASSERT_TRUE(PRESET_LEDS[i].cs_pin >= 0);
        TEST_ASSERT_TRUE(PRESET_LEDS[i].cs_pin < LED_MATRIX_COLS);
    }
}

// Test helper functions
void test_getPresetButton_helper() {
    // Valid indices
    for (int i = 0; i < NUM_PRESETS; i++) {
        const PresetButton* btn = getPresetButton(i);
        TEST_ASSERT_NOT_NULL(btn);
        TEST_ASSERT_EQUAL_STRING(PRESET_BUTTONS[i].name, btn->name);
        TEST_ASSERT_EQUAL(PRESET_BUTTONS[i].row, btn->row);
        TEST_ASSERT_EQUAL(PRESET_BUTTONS[i].col, btn->col);
    }
    
    // Invalid indices
    TEST_ASSERT_NULL(getPresetButton(-1));
    TEST_ASSERT_NULL(getPresetButton(NUM_PRESETS));
    TEST_ASSERT_NULL(getPresetButton(100));
}

void test_getPresetLED_helper() {
    // Valid indices
    for (int i = 0; i < NUM_PRESETS; i++) {
        const PresetLED* led = getPresetLED(i);
        TEST_ASSERT_NOT_NULL(led);
        TEST_ASSERT_EQUAL(PRESET_LEDS[i].sw_pin, led->sw_pin);
        TEST_ASSERT_EQUAL(PRESET_LEDS[i].cs_pin, led->cs_pin);
    }
    
    // Invalid indices
    TEST_ASSERT_NULL(getPresetLED(-1));
    TEST_ASSERT_NULL(getPresetLED(NUM_PRESETS));
    TEST_ASSERT_NULL(getPresetLED(100));
}

// Test button-to-index lookup
void test_findPresetByButton() {
    // Test finding each configured button
    for (int i = 0; i < NUM_PRESETS; i++) {
        int found = findPresetByButton(PRESET_BUTTONS[i].row, PRESET_BUTTONS[i].col);
        TEST_ASSERT_EQUAL_MESSAGE(i, found, "Button lookup should return correct index");
    }
    
    // Test invalid button positions
    TEST_ASSERT_EQUAL(-1, findPresetByButton(99, 99));
    TEST_ASSERT_EQUAL(-1, findPresetByButton(PRESET_BUTTON_ROW, 99));
}

// Test no duplicate columns (each button has unique column)
void test_no_duplicate_button_columns() {
    for (int i = 0; i < NUM_PRESETS; i++) {
        for (int j = i + 1; j < NUM_PRESETS; j++) {
            TEST_ASSERT_NOT_EQUAL_MESSAGE(
                PRESET_BUTTONS[i].col, 
                PRESET_BUTTONS[j].col,
                "Each button must have unique column");
        }
    }
}

// Test expected button names
void test_button_names() {
    TEST_ASSERT_EQUAL_STRING("Preset 1", PRESET_BUTTONS[0].name);
    TEST_ASSERT_EQUAL_STRING("Preset 2", PRESET_BUTTONS[1].name);
    TEST_ASSERT_EQUAL_STRING("Preset 3", PRESET_BUTTONS[2].name);
    TEST_ASSERT_EQUAL_STRING("Preset 4", PRESET_BUTTONS[3].name);
    TEST_ASSERT_EQUAL_STRING("Preset 5", PRESET_BUTTONS[4].name);
    TEST_ASSERT_EQUAL_STRING("Preset 6", PRESET_BUTTONS[5].name);
    TEST_ASSERT_EQUAL_STRING("Preset 7", PRESET_BUTTONS[6].name);
    TEST_ASSERT_EQUAL_STRING("Memory", PRESET_BUTTONS[7].name);
}

// Test that there's a gap in column assignments (col 5 is skipped based on actual config)
void test_column_gap_exists() {
    // Verify column 5 is not used (gap between preset 4 and 5)
    bool col5_used = false;
    for (int i = 0; i < NUM_PRESETS; i++) {
        if (PRESET_BUTTONS[i].col == 5) {
            col5_used = true;
            break;
        }
    }
    TEST_ASSERT_FALSE_MESSAGE(col5_used, "Column 5 should be skipped (PCB gap)");
}

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_preset_buttons_configured);
    RUN_TEST(test_preset_leds_configured);
    RUN_TEST(test_getPresetButton_helper);
    RUN_TEST(test_getPresetLED_helper);
    RUN_TEST(test_findPresetByButton);
    RUN_TEST(test_no_duplicate_button_columns);
    RUN_TEST(test_button_names);
    RUN_TEST(test_column_gap_exists);
    
    return UNITY_END();
}

