import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from . import radio_controller_ns, RadioController

AUTO_LOAD = ["text_sensor"]

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    icon="mdi:radio"
).extend({
    cv.GenerateID("radio_controller_id"): cv.use_id(RadioController),
})


async def to_code(config):
    parent = await cg.get_variable(config["radio_controller_id"])
    var = await text_sensor.new_text_sensor(config)
    cg.add(parent.set_preset_text_sensor(var))
