import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from esphome import automation

AUTO_LOAD = ["select", "number", "text_sensor", "sensor"]
DEPENDENCIES = ["i2c"]

CONF_ON_PRESET = "on_preset"
CONF_PRESET_NUM = "preset_num"

radio_controller_ns = cg.esphome_ns.namespace("radio_controller")
RadioControllerComponent = radio_controller_ns.class_("RadioControllerComponent", cg.Component)

# Trigger for preset button presses
PresetTrigger = radio_controller_ns.class_(
    "PresetTrigger", automation.Trigger.template(cg.int_)
)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RadioControllerComponent),
    cv.Optional(CONF_ON_PRESET): automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PresetTrigger),
            cv.Required(CONF_PRESET_NUM): cv.int_range(min=0, max=7),
        }
    ),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Register preset button triggers
    for conf in config.get(CONF_ON_PRESET, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], cg.int_(conf[CONF_PRESET_NUM]))
        await automation.build_automation(trigger, [(cg.int_, "preset")], conf)
        cg.add(var.add_preset_trigger(conf[CONF_PRESET_NUM], trigger))
