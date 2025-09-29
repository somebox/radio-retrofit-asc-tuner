import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UART_ID

AUTO_LOAD = ["switch", "number", "text_sensor"]

bridge_ns = cg.esphome_ns.namespace("radio_controller")
HomeAssistantBridgeComponent = bridge_ns.class_("HomeAssistantBridgeComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HomeAssistantBridgeComponent),
    cv.GenerateID(CONF_UART_ID): cv.use_id(uart.UARTComponent),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    uart_component = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart_parent(uart_component))

    frame_lambda = cg.RawExpression('[](const std::string &frame) { /* TODO: parse events */ }')
    cg.add(var.register_frame_callback(frame_lambda))


