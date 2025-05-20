import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import text_sensor

CONF_DIAL_PIN = "dial_pin"
CONF_DIAL_TIMEOUT = "dial_timeout"

fetap_ns = cg.esphome_ns.namespace("fetap")
FetapDialSensor = fetap_ns.class_(
    "FetapDialSensor", text_sensor.TextSensor, cg.Component
)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(FetapDialSensor).extend(
    {
        cv.Required(CONF_DIAL_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_DIAL_TIMEOUT, default=0): cv.int_range(min=0, max=1000000)
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)

    cg.add(var.set_dial_pin(config[CONF_DIAL_PIN]))
    cg.add(var.set_dial_timeout(config[CONF_DIAL_TIMEOUT]))
