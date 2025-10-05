"""RetroText Display Component for ESPHome

Displays text on a 3-board IS31FL3737 LED matrix display (72×6 pixels, 18 characters).
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]

retrotext_display_ns = cg.esphome_ns.namespace("retrotext_display")
RetroTextDisplay = retrotext_display_ns.class_(
    "RetroTextDisplay", cg.Component, i2c.I2CDevice
)

CONF_BRIGHTNESS = "brightness"
CONF_BOARDS = "boards"
CONF_ADDRESS = "address"
CONF_SCROLL_DELAY = "scroll_delay"
CONF_SCROLL_MODE = "scroll_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RetroTextDisplay),
        cv.GenerateID(i2c.CONF_I2C_ID): cv.use_id(i2c.I2CBus),
        cv.Optional(CONF_BRIGHTNESS, default=128): cv.int_range(min=0, max=255),
        cv.Required(CONF_BOARDS): cv.All(
            cv.ensure_list(cv.i2c_address),
            cv.Length(min=3, max=3),
        ),
        cv.Optional(CONF_SCROLL_DELAY, default="300ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_SCROLL_MODE, default="auto"): cv.enum(
            {"auto": 0, "always": 1, "never": 2}, upper=False
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Get I2C bus
    i2c_bus = await cg.get_variable(config[i2c.CONF_I2C_ID])
    cg.add(var.set_i2c_bus(i2c_bus))
    
    # Set brightness
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
    
    # Set board addresses
    boards = config[CONF_BOARDS]
    cg.add(var.set_board_addresses(boards[0], boards[1], boards[2]))
    
    # Set scroll configuration
    cg.add(var.set_scroll_delay(config[CONF_SCROLL_DELAY]))
    cg.add(var.set_scroll_mode(config[CONF_SCROLL_MODE]))
