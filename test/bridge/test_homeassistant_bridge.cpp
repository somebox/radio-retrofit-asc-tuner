#include <unity.h>
#include <sstream>

#include "HomeAssistantBridge.h"
#include "Events.h"
#include "events/JsonHelpers.h"

namespace {

// Mock serial for testing
class MockSerial : public HardwareSerial {
 public:
  MockSerial() : HardwareSerial(0) {}
  
  void begin(unsigned long baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1, bool invert = false, unsigned long timeout_ms = 20000UL, uint8_t rxfifo_full_thrhd = 112) override {
    baud_ = baud;
  }
  
  void print(const char* str) override {
    output_ += str;
  }
  
  void print(int value) override {
    output_ += std::to_string(value);
  }
  
  void println(const char* str) override {
    output_ += str;
    output_ += "\n";
  }
  
  void println() override {
    output_ += "\n";
  }
  
  std::string getOutput() const { return output_; }
  void clearOutput() { output_.clear(); }
  
 private:
  unsigned long baud_{0};
  std::string output_;
};

void test_publish_event_serialization() {
  MockSerial mock_serial;
  SerialHomeAssistantBridge bridge(mock_serial, 115200);
  
  bridge.begin();
  
  events::Event evt(EventType::BrightnessChanged);
  evt.timestamp = 12345;
  evt.value = events::json::object({
      events::json::number_field("value", 180),
  });
  
  bridge.publishEvent(evt);
  
  const std::string output = mock_serial.getOutput();
  TEST_ASSERT_FALSE_MESSAGE(output.empty(), "Bridge should produce output");
  
  // Verify JSON structure
  TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("\"type_id\":4"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("\"type_name\":\"settings.brightness\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("\"timestamp\":12345"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("\"value\":{\"value\":180}"));
  TEST_ASSERT_EQUAL_CHAR('\n', output.back());
}

void test_publish_event_with_complex_payload() {
  MockSerial mock_serial;
  SerialHomeAssistantBridge bridge(mock_serial);
  
  bridge.begin();
  
  events::Event evt(EventType::ModeChanged);
  evt.timestamp = 67890;
  evt.value = events::json::object({
      events::json::number_field("value", 2),
      events::json::string_field("name", "clock"),
      events::json::number_field("preset", 3, true),
  });
  
  bridge.publishEvent(evt);
  
  const std::string output = mock_serial.getOutput();
  
  // Verify the complex JSON payload is preserved
  TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("\"type_id\":7"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("\"type_name\":\"system.mode\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("\"value\":{\"value\":2,\"name\":\"clock\",\"preset\":3}"));
}

void test_catalog_consistency() {
  // Verify that the event catalog matches what the bridge expects
  events::Event evt(EventType::PresetPressed);
  TEST_ASSERT_EQUAL_UINT16(0, evt.type_id);
  TEST_ASSERT_EQUAL_STRING("preset.pressed", evt.type_name);
  
  events::Event evt2(EventType::VolumeChanged);
  TEST_ASSERT_EQUAL_UINT16(8, evt2.type_id);
  TEST_ASSERT_EQUAL_STRING("settings.volume", evt2.type_name);
}

}  // namespace

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_publish_event_serialization);
  RUN_TEST(test_publish_event_with_complex_payload);
  RUN_TEST(test_catalog_consistency);
  return UNITY_END();
}

