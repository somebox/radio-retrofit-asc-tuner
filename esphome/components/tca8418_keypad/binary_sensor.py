"""Binary Sensor Platform for TCA8418 Keypad.

Allows exposing individual keys as binary sensors.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID
from . import tca8418_keypad_ns, TCA8418Component

DEPENDENCIES = ["tca8418_keypad"]

CONF_TCA8418_KEYPAD_ID = "tca8418_keypad_id"
CONF_ROW = "row"
CONF_COLUMN = "column"

TCA8418BinarySensor = tca8418_keypad_ns.class_(
    "TCA8418BinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(TCA8418BinarySensor).extend(
    {
        cv.GenerateID(CONF_TCA8418_KEYPAD_ID): cv.use_id(TCA8418Component),
        cv.Required(CONF_ROW): cv.int_range(min=0, max=7),
        cv.Required(CONF_COLUMN): cv.int_range(min=0, max=9),
    }
)


async def to_code(config):
    """Generate code for binary sensor."""
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    
    parent = await cg.get_variable(config[CONF_TCA8418_KEYPAD_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_position(config[CONF_ROW], config[CONF_COLUMN]))
    cg.add(parent.register_key_sensor(config[CONF_ROW], config[CONF_COLUMN], var))
