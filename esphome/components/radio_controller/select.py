import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID
from . import radio_controller_ns, RadioController, RadioControllerSelect

AUTO_LOAD = ["select"]

CONFIG_SCHEMA = select.select_schema(
    RadioControllerSelect,
    icon="mdi:radio"
).extend({
    cv.GenerateID("radio_controller_id"): cv.use_id(RadioController),
})


async def to_code(config):
    parent = await cg.get_variable(config["radio_controller_id"])
    var = await select.new_select(config, options=[])
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(parent.set_preset_select(var))
