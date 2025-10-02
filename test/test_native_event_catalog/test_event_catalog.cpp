#include <unity.h>

#include "platform/events/Events.h"

namespace {

void test_catalog_lookup_by_type() {
  const EventCatalogEntry& entry = eventCatalogLookup(EventType::BrightnessChanged);
  TEST_ASSERT_EQUAL_UINT16(0, entry.id);
  TEST_ASSERT_EQUAL_STRING("settings.brightness", entry.name);
}

void test_catalog_lookup_by_id() {
  const EventCatalogEntry& entry = eventCatalogLookup(3);
  TEST_ASSERT_EQUAL(static_cast<int>(EventType::ModeChanged), static_cast<int>(entry.type));
  TEST_ASSERT_EQUAL_STRING("system.mode", entry.name);
}

void test_catalog_lookup_unknown() {
  const EventCatalogEntry& entry = eventCatalogLookup(static_cast<uint16_t>(9999));
  TEST_ASSERT_EQUAL_STRING("unknown", entry.name);
}

}  // namespace

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_catalog_lookup_by_type);
  RUN_TEST(test_catalog_lookup_by_id);
  RUN_TEST(test_catalog_lookup_unknown);
  return UNITY_END();
}

