from esphome import pins
import esphome.codegen as cg
from esphome.components import speaker
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID, CONF_CHANNELS
)
CONF_MAX_CHANNELS = "max_channels"
CONF_I2S_LRCLK_PIN = "i2s_lrclk_pin"
CONF_I2S_BCLK_PIN = "i2s_bclk_pin"
CONF_I2S_DOUT_PIN = "i2s_dout_pin"

fetap_ns = cg.esphome_ns.namespace("fetap")
FetapSpeaker = fetap_ns.class_("FetapSpeaker", speaker.Speaker, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(FetapSpeaker),
        cv.Required(CONF_I2S_LRCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_I2S_BCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_I2S_DOUT_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_MAX_CHANNELS, default=1): cv.int_,
        cv.Optional(CONF_CHANNELS, default=1): cv.int_,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await speaker.register_speaker(var, config)
    cg.add(var.set_lrclk_pin(config[CONF_I2S_LRCLK_PIN]))
    cg.add(var.set_bclk_pin(config[CONF_I2S_BCLK_PIN]))
    cg.add(var.set_dout_pin(config[CONF_I2S_DOUT_PIN]))
