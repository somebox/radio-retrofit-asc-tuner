import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from . import radio_controller_ns, RadioController

MODE_SWITCH_SCHEMA = switch.switch_schema(
    RadioController,
    icon="mdi:account-voice"
)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RadioController),
})


