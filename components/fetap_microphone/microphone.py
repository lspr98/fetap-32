from esphome import pins
import esphome.codegen as cg
from esphome.components import microphone
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_GPIO
)

# DEPENDENCIES = ["microphone"]

CONF_I2S_LRCLK_PIN = "i2s_lrclk_pin"
CONF_I2S_BCLK_PIN = "i2s_bclk_pin"
CONF_I2S_DIN_PIN = "i2s_din_pin"

fetap_ns = cg.esphome_ns.namespace("fetap")
FetapMicrophone = fetap_ns.class_(
    "FetapMicrophone", microphone.Microphone, cg.Component
    )

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(FetapMicrophone),
        cv.Required(CONF_I2S_LRCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_I2S_BCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_I2S_DIN_PIN): pins.internal_gpio_output_pin_number,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await microphone.register_microphone(var, config)
    await cg.register_component(var, config)

    cg.add(var.set_din_pin(config[CONF_I2S_DIN_PIN]))
    cg.add(var.set_bclk_pin(config[CONF_I2S_BCLK_PIN]))
    cg.add(var.set_lrclk_pin(config[CONF_I2S_LRCLK_PIN]))