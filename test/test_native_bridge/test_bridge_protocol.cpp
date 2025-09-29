#include <unity.h>

#include "bridge/BridgeProtocol.h"

namespace {

void test_encode_decode_command_round_trip() {
  bridge::CommandMessage command;
  command.name = bridge::CommandName::kSetMode;
  command.mode = 2;
  command.preset = 5;
  command.value = 10;
  command.mode_name = "clock";
  command.text = "Night Mode";

  const std::string encoded = bridge::encodeCommand(command);
  TEST_ASSERT_FALSE_MESSAGE(encoded.empty(), "Encoded command should not be empty");
  TEST_ASSERT_EQUAL_CHAR('\n', encoded.back());

  bridge::CommandMessage decoded;
  TEST_ASSERT_TRUE(bridge::decodeCommand(encoded, &decoded));
  TEST_ASSERT_EQUAL(static_cast<int>(bridge::CommandName::kSetMode), static_cast<int>(decoded.name));
  TEST_ASSERT_EQUAL(2, decoded.mode);
  TEST_ASSERT_EQUAL(5, decoded.preset);
  TEST_ASSERT_EQUAL(10, decoded.value);
  TEST_ASSERT_EQUAL_STRING("clock", decoded.mode_name.c_str());
  TEST_ASSERT_EQUAL_STRING("Night Mode", decoded.text.c_str());
}

void test_decode_command_without_optional_fields() {
  const std::string frame = "{\"cmd\":\"set_volume\",\"value\":180}\n";
  bridge::CommandMessage decoded;
  TEST_ASSERT_TRUE(bridge::decodeCommand(frame, &decoded));
  TEST_ASSERT_EQUAL(static_cast<int>(bridge::CommandName::kSetVolume), static_cast<int>(decoded.name));
  TEST_ASSERT_EQUAL(180, decoded.value);
  TEST_ASSERT_EQUAL(-1, decoded.mode);
  TEST_ASSERT_EQUAL(-1, decoded.preset);
  TEST_ASSERT_TRUE(decoded.mode_name.empty());
  TEST_ASSERT_TRUE(decoded.text.empty());
}

void test_encode_decode_event_with_escaping() {
  bridge::EventMessage event;
  event.name = bridge::EventName::kBrightnessChanged;
  event.timestamp = 12345;
  event.i1 = 42;
  event.i2 = -3;
  event.text = "Quote \"and newline\n";

  const std::string encoded = bridge::encodeEvent(event);
  TEST_ASSERT_FALSE(encoded.empty());
  TEST_ASSERT_EQUAL_CHAR('\n', encoded.back());
  TEST_ASSERT_NOT_EQUAL(std::string::npos, encoded.find("\\\""));

  bridge::EventMessage decoded;
  TEST_ASSERT_TRUE(bridge::decodeEvent(encoded, &decoded));
  TEST_ASSERT_EQUAL(static_cast<int>(bridge::EventName::kBrightnessChanged), static_cast<int>(decoded.name));
  TEST_ASSERT_EQUAL_UINT32(12345, decoded.timestamp);
  TEST_ASSERT_EQUAL(42, decoded.i1);
  TEST_ASSERT_EQUAL(-3, decoded.i2);
  TEST_ASSERT_EQUAL_STRING("Quote \"and newline\n", decoded.text.c_str());
}

void test_reject_unknown_command() {
  const std::string frame = "{\"cmd\":\"unknown\"}\n";
  bridge::CommandMessage decoded;
  TEST_ASSERT_FALSE(bridge::decodeCommand(frame, &decoded));
}

void test_reject_unknown_event() {
  const std::string frame = "{\"type\":\"unknown\"}\n";
  bridge::EventMessage decoded;
  TEST_ASSERT_FALSE(bridge::decodeEvent(frame, &decoded));
}

}  // namespace

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_encode_decode_command_round_trip);
  RUN_TEST(test_decode_command_without_optional_fields);
  RUN_TEST(test_encode_decode_event_with_escaping);
  RUN_TEST(test_reject_unknown_command);
  RUN_TEST(test_reject_unknown_event);
  return UNITY_END();
}


