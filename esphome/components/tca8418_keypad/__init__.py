"""TCA8418 I2C Keypad Matrix Controller Component for ESPHome.

This component provides an interface to the TCA8418 I2C keypad matrix controller,
supporting up to 8x10 matrix scanning with event-based key detection.

Based on Adafruit_TCA8418 library, adapted for ESPHome.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from esphome import automation

# Component will be in tca8418_keypad namespace
DEPENDENCIES = ["i2c"]

tca8418_keypad_ns = cg.esphome_ns.namespace("tca8418_keypad")
TCA8418Component = tca8418_keypad_ns.class_(
    "TCA8418Component", cg.Component, i2c.I2CDevice
)

# Triggers for key events
KeyPressTrigger = tca8418_keypad_ns.class_(
    "KeyPressTrigger", automation.Trigger.template(cg.uint8, cg.uint8, cg.uint8)
)
KeyReleaseTrigger = tca8418_keypad_ns.class_(
    "KeyReleaseTrigger", automation.Trigger.template(cg.uint8, cg.uint8, cg.uint8)
)

# Configuration constants
CONF_ROWS = "rows"
CONF_COLUMNS = "columns"
CONF_ON_KEY_PRESS = "on_key_press"
CONF_ON_KEY_RELEASE = "on_key_release"

# Configuration schema with triggers
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TCA8418Component),
            cv.Optional(CONF_ROWS, default=8): cv.int_range(min=1, max=8),
            cv.Optional(CONF_COLUMNS, default=10): cv.int_range(min=1, max=10),
            cv.Optional(CONF_ON_KEY_PRESS): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(KeyPressTrigger),
                }
            ),
            cv.Optional(CONF_ON_KEY_RELEASE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(KeyReleaseTrigger),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x34))
)


async def to_code(config):
    """Generate C++ code for the component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    
    # Set matrix configuration
    cg.add(var.set_matrix_size(config[CONF_ROWS], config[CONF_COLUMNS]))
    
    # Register on_key_press triggers
    for conf in config.get(CONF_ON_KEY_PRESS, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_key_press_trigger(trigger))
        await automation.build_automation(
            trigger, [(cg.uint8, "row"), (cg.uint8, "col"), (cg.uint8, "key")], conf
        )
    
    # Register on_key_release triggers
    for conf in config.get(CONF_ON_KEY_RELEASE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_key_release_trigger(trigger))
        await automation.build_automation(
            trigger, [(cg.uint8, "row"), (cg.uint8, "col"), (cg.uint8, "key")], conf
        )
