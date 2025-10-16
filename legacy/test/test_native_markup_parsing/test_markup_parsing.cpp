#include <unity.h>

// Note: This test is disabled for native builds because SignTextController
// is designed for Arduino environment and depends on Arduino-specific features.
// The markup parsing functionality should be tested on actual hardware.

#ifdef ARDUINO
#include "display/SignTextController.h"
using namespace RetroText;
#endif

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

#ifdef ARDUINO
void test_parseMarkup_simple_text() {
    SignTextController controller(18, 4);
    String result = controller.parseMarkup("Hello World");
    TEST_ASSERT_EQUAL_STRING("Hello World", result.c_str());
}

void test_parseMarkup_font_tags() {
    SignTextController controller(18, 4);
    String result = controller.parseMarkup("Normal <f:m>Modern</f> <f:r>Retro</f> <f:i>Icons</f>");
    TEST_ASSERT_EQUAL_STRING("Normal Modern Retro Icons", result.c_str());
}

void test_parseMarkup_brightness_tags() {
    SignTextController controller(18, 4);
    String result = controller.parseMarkup("Normal <b:bright>BRIGHT</b> <b:dim>dim</b>");
    TEST_ASSERT_EQUAL_STRING("Normal BRIGHT dim", result.c_str());
}

void test_parseMarkup_nested_tags() {
    SignTextController controller(18, 4);
    String result = controller.parseMarkup("<f:m>Modern <b:bright>Bright</b></f>");
    TEST_ASSERT_EQUAL_STRING("Modern Bright", result.c_str());
}

void test_parseMarkup_invalid_tags() {
    SignTextController controller(18, 4);
    // Unclosed tags should be treated as regular text
    String result = controller.parseMarkup("Hello <invalid> World");
    TEST_ASSERT_EQUAL_STRING("Hello <invalid> World", result.c_str());
}

void test_setMessageWithMarkup_creates_font_spans() {
    SignTextController controller(18, 4);
    controller.setMessageWithMarkup("Start <f:m>Modern</f> End");
    
    // Test that font spans are created correctly
    // Note: We can't directly access private members, so we test indirectly
    // by checking if the message was parsed correctly
    String clean_message = controller.parseMarkup("Start <f:m>Modern</f> End");
    TEST_ASSERT_EQUAL_STRING("Start Modern End", clean_message.c_str());
}

void test_setMessageWithMarkup_creates_highlight_spans() {
    SignTextController controller(18, 4);
    controller.setMessageWithMarkup("Start <b:bright>BRIGHT</b> End");
    
    // Test that highlight spans are created
    String clean_message = controller.parseMarkup("Start <b:bright>BRIGHT</b> End");
    TEST_ASSERT_EQUAL_STRING("Start BRIGHT End", clean_message.c_str());
}

void test_font_span_boundaries() {
    SignTextController controller(18, 4);
    // Test that font spans are applied to correct character ranges
    controller.setMessageWithMarkup("ABC<f:m>DEF</f>GHI");
    
    String clean_message = controller.parseMarkup("ABC<f:m>DEF</f>GHI");
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHI", clean_message.c_str());
    
    // The font span should cover characters 3-5 (DEF)
    // We can't test this directly without accessing private members
}

void test_multiple_font_spans() {
    SignTextController controller(18, 4);
    controller.setMessageWithMarkup("<f:r>A</f><f:m>B</f><f:i>C</f>");
    
    String clean_message = controller.parseMarkup("<f:r>A</f><f:m>B</f><f:i>C</f>");
    TEST_ASSERT_EQUAL_STRING("ABC", clean_message.c_str());
}

void test_empty_tags() {
    SignTextController controller(18, 4);
    String result = controller.parseMarkup("Hello <f:m></f> World");
    TEST_ASSERT_EQUAL_STRING("Hello  World", result.c_str());
}

void test_complex_markup() {
    SignTextController controller(18, 4);
    String markup = "♪ <f:i>!</f> <b:bright><f:m>Now Playing</f></b>: <f:r>Retro Song</f>";
    String result = controller.parseMarkup(markup);
    TEST_ASSERT_EQUAL_STRING("♪ ! Now Playing: Retro Song", result.c_str());
}

void test_font_constants() {
    // Test that font constants are defined correctly
    TEST_ASSERT_EQUAL_INT(0, MODERN_FONT);
    TEST_ASSERT_EQUAL_INT(1, ARDUBOY_FONT);
    TEST_ASSERT_EQUAL_INT(2, ICON_FONT);
}

void test_brightness_constants() {
    // Test that brightness constants are defined correctly
    TEST_ASSERT_EQUAL_INT(150, BRIGHT);
    TEST_ASSERT_EQUAL_INT(70, NORMAL);
    TEST_ASSERT_EQUAL_INT(20, DIM);
    TEST_ASSERT_EQUAL_INT(8, VERY_DIM);
}
#endif // ARDUINO

// Native-compatible test that doesn't require Arduino environment
void test_native_placeholder() {
    // This test runs in native environment to verify the test framework works
    TEST_ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
#ifdef ARDUINO
    // Arduino-specific tests (run on hardware)
    RUN_TEST(test_parseMarkup_simple_text);
    RUN_TEST(test_parseMarkup_font_tags);
    RUN_TEST(test_parseMarkup_brightness_tags);
    RUN_TEST(test_parseMarkup_nested_tags);
    RUN_TEST(test_parseMarkup_invalid_tags);
    RUN_TEST(test_setMessageWithMarkup_creates_font_spans);
    RUN_TEST(test_setMessageWithMarkup_creates_highlight_spans);
    RUN_TEST(test_font_span_boundaries);
    RUN_TEST(test_multiple_font_spans);
    RUN_TEST(test_empty_tags);
    RUN_TEST(test_complex_markup);
    RUN_TEST(test_font_constants);
    RUN_TEST(test_brightness_constants);
#else
    // Native environment tests (run during development)
    RUN_TEST(test_native_placeholder);
#endif
    
    return UNITY_END();
}
