"""Panel LEDs Component for ESPHome

Controls IS31FL3737 LED matrix for preset buttons and mode indicators.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]

panel_leds_ns = cg.esphome_ns.namespace("panel_leds")
PanelLEDs = panel_leds_ns.class_(
    "PanelLEDs", cg.Component, i2c.I2CDevice
)

CONF_BRIGHTNESS = "brightness"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PanelLEDs),
        cv.GenerateID(i2c.CONF_I2C_ID): cv.use_id(i2c.I2CBus),
        cv.Optional(CONF_BRIGHTNESS, default=128): cv.int_range(min=0, max=255),
    }
).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(0x55))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    
    # Get I2C bus
    i2c_bus = await cg.get_variable(config[i2c.CONF_I2C_ID])
    cg.add(var.set_i2c_bus(i2c_bus))
    
    # Set brightness
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
