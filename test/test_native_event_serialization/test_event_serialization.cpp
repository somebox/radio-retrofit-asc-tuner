#include <unity.h>

#include "platform/events/Events.h"
#include "platform/JsonHelpers.h"

namespace {

void test_event_constructor_populates_catalog_fields() {
  events::Event evt(EventType::VolumeChanged);
  TEST_ASSERT_EQUAL(static_cast<int>(EventType::VolumeChanged), static_cast<int>(evt.type));
  TEST_ASSERT_EQUAL_UINT16(8, evt.type_id);
  TEST_ASSERT_EQUAL_STRING("settings.volume", evt.type_name);
}

void test_event_payload_round_trip() {
  events::Event evt(EventType::BrightnessChanged);
  evt.value = events::json::object({
      events::json::number_field("value", 123),
      events::json::boolean_field("override", true),
  });

  TEST_ASSERT_EQUAL_STRING("{\"value\":123,\"override\":true}", evt.value.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_event_constructor_populates_catalog_fields);
  RUN_TEST(test_event_payload_round_trip);
  return UNITY_END();
}

