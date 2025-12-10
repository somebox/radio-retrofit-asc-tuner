import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import tca8418_keypad, retrotext_display, select, i2c
from esphome.components import text_sensor as text_sensor_component
from esphome.const import CONF_ID, CONF_ICON
from esphome import automation

DEPENDENCIES = ['tca8418_keypad', 'retrotext_display']

radio_controller_ns = cg.esphome_ns.namespace('radio_controller')
RadioController = radio_controller_ns.class_('RadioController', cg.Component)
RadioControllerSelect = radio_controller_ns.class_('RadioControllerSelect', select.Select, cg.Component)

CONF_KEYPAD_ID = 'keypad_id'
CONF_DISPLAY_ID = 'display_id'
CONF_I2C_ID = 'i2c_id'
CONF_PRESETS = 'presets'
CONF_BUTTON = 'button'
CONF_ROW = 'row'
CONF_COLUMN = 'column'
CONF_NAME = 'name'
CONF_DISPLAY = 'display'
CONF_TARGET = 'target'
CONF_SERVICE = 'service'
CONF_DATA = 'data'
CONF_CONTROLS = 'controls'
CONF_ENCODER_BUTTON = 'encoder_button'
CONF_MEMORY_BUTTON = 'memory_button'

BUTTON_SCHEMA = cv.Schema({
    cv.Required(CONF_ROW): cv.int_range(min=0, max=7),
    cv.Required(CONF_COLUMN): cv.int_range(min=0, max=9),
})

PRESET_SCHEMA = cv.Schema({
    cv.Required(CONF_BUTTON): BUTTON_SCHEMA,
    # Display text (defaults to target if not specified)
    cv.Optional(CONF_DISPLAY): cv.string,
    # Target value to pass to service (e.g., media_id)
    cv.Optional(CONF_TARGET): cv.string,
    # Optional: Direct service call (overrides default service)
    cv.Optional(CONF_SERVICE): cv.string,
    # Optional: Additional data to pass to service
    cv.Optional(CONF_DATA): cv.Schema({cv.string: cv.string_strict}),
    # Backwards compatibility: 'name' is alias for 'display'
    cv.Optional(CONF_NAME): cv.string,
})

CONTROLS_SCHEMA = cv.Schema({
    cv.Optional(CONF_ENCODER_BUTTON): BUTTON_SCHEMA,
    cv.Optional(CONF_MEMORY_BUTTON): BUTTON_SCHEMA,
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RadioController),
    cv.Required(CONF_KEYPAD_ID): cv.use_id(tca8418_keypad.TCA8418Component),
    cv.Required(CONF_DISPLAY_ID): cv.use_id(retrotext_display.RetroTextDisplay),
    cv.Optional(CONF_I2C_ID): cv.use_id(i2c.I2CBus),
    cv.Optional(CONF_PRESETS, default=[]): cv.ensure_list(PRESET_SCHEMA),
    cv.Optional(CONF_CONTROLS): CONTROLS_SCHEMA,
    # Default service to call for all presets (can be overridden per-preset)
    cv.Optional(CONF_SERVICE, default="script.radio_play_preset"): cv.string,
    # Mode selector text sensor
    cv.Optional("mode_text_sensor"): cv.use_id(text_sensor_component.TextSensor),
}).extend(cv.COMPONENT_SCHEMA)

# Text sensor platform for current preset
TEXT_SENSOR_SCHEMA = text_sensor_component.text_sensor_schema(
    icon="mdi:radio"
).extend({
    cv.GenerateID(CONF_ID): cv.use_id(RadioController),
})

# Select platform for preset selection
SELECT_SCHEMA = select.select_schema(
    RadioControllerSelect,
    icon="mdi:radio"
).extend({
    cv.GenerateID(CONF_ID): cv.use_id(RadioController),
})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # ArduinoJson is provided by ESPHome core (6.x)
    
    # Get references to keypad and display
    keypad = await cg.get_variable(config[CONF_KEYPAD_ID])
    display = await cg.get_variable(config[CONF_DISPLAY_ID])
    
    cg.add(var.set_keypad(keypad))
    cg.add(var.set_display(display))
    
    # Optional: Get I2C bus for automatic panel LED initialization at 0x55
    if CONF_I2C_ID in config:
        i2c_bus = await cg.get_variable(config[CONF_I2C_ID])
        cg.add(var.set_i2c_bus(i2c_bus))
    
    # Set default service
    cg.add(var.set_default_service(config[CONF_SERVICE]))
    
    # Add presets
    for preset in config[CONF_PRESETS]:
        button = preset[CONF_BUTTON]
        
        # Determine display text (display > name > target)
        display_text = preset.get(CONF_DISPLAY) or preset.get(CONF_NAME) or preset.get(CONF_TARGET, "")
        
        # Get target value
        target = preset.get(CONF_TARGET, "")
        
        # Get service (preset-specific or default)
        service = preset.get(CONF_SERVICE, "")
        
        # Create preset
        cg.add(var.add_preset(
            button[CONF_ROW],
            button[CONF_COLUMN],
            display_text,
            target,
            service
        ))
        
        # Add any additional data key-value pairs
        if CONF_DATA in preset:
            for key, value in preset[CONF_DATA].items():
                cg.add(var.add_preset_data(
                    button[CONF_ROW],
                    button[CONF_COLUMN],
                    key,
                    value
                ))
    
    # Add controls
    if CONF_CONTROLS in config:
        controls = config[CONF_CONTROLS]
        if CONF_ENCODER_BUTTON in controls:
            encoder = controls[CONF_ENCODER_BUTTON]
            cg.add(var.set_encoder_button(encoder[CONF_ROW], encoder[CONF_COLUMN]))
        if CONF_MEMORY_BUTTON in controls:
            memory = controls[CONF_MEMORY_BUTTON]
            cg.add(var.set_memory_button(memory[CONF_ROW], memory[CONF_COLUMN]))
    
    # Add mode text sensor if configured
    if "mode_text_sensor" in config:
        mode_sensor = await cg.get_variable(config["mode_text_sensor"])
        cg.add(var.set_mode_text_sensor(mode_sensor))
    
    # Note: Preset slot text sensors are created and registered in C++
    # See radio_controller.cpp setup() method for sensor initialization


@automation.register_condition("radio_controller.is_preset_active", automation.Condition, cv.Schema({
    cv.GenerateID(): cv.use_id(RadioController),
    cv.Required(CONF_NAME): cv.string,
}))
async def radio_controller_is_preset_active_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent, config[CONF_NAME])


# Text sensor platform  
async def to_code_text_sensor(config):
    parent = await cg.get_variable(config[CONF_ID])
    sens = await text_sensor_component.new_text_sensor(config)
    cg.add(parent.set_preset_text_sensor(sens))


# Select platform  
async def to_code_select(config):
    parent = await cg.get_variable(config[CONF_ID])
    sel = await select.new_select(config, options=[])
    await cg.register_component(sel, config)
    cg.add(sel.set_parent(parent))
    cg.add(parent.set_preset_select(sel))