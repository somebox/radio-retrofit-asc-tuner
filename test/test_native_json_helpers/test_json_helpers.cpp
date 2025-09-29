#include <unity.h>

#include "events/JsonHelpers.h"

namespace {

void test_escape_handles_control_characters() {
  const std::string escaped = events::json::escape("\tQuote\n");
  TEST_ASSERT_EQUAL_STRING("\\tQuote\\n", escaped.c_str());
}

void test_string_field_trims_empty_input() {
  auto field = events::json::string_field("name", "");
  TEST_ASSERT_FALSE(field.enabled);
}

void test_object_builder_skips_disabled() {
  const std::string json = events::json::object({
      events::json::string_field("name", "radio"),
      events::json::number_field("value", 42),
      events::json::field("skip", "true", false),
  });
  TEST_ASSERT_EQUAL_STRING("{\"name\":\"radio\",\"value\":42}", json.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_escape_handles_control_characters);
  RUN_TEST(test_string_field_trims_empty_input);
  RUN_TEST(test_object_builder_skips_disabled);
  return UNITY_END();
}

