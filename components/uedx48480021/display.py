import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display, spi
from esphome.const import (
    CONF_ID,
    CONF_LAMBDA,
    CONF_RESET_PIN,
)
from esphome import pins

CODEOWNERS = ["@jargordon"]
DEPENDENCIES = ["spi"]

uedx48480021_ns = cg.esphome_ns.namespace("uedx48480021")
UEDX48480021 = uedx48480021_ns.class_(
    "UEDX48480021", cg.PollingComponent, display.DisplayBuffer, spi.SPIDevice
)

CONF_DE_PIN = "de_pin"
CONF_PCLK_PIN = "pclk_pin"
CONF_HSYNC_PIN = "hsync_pin"
CONF_VSYNC_PIN = "vsync_pin"
CONF_BACKLIGHT_PIN = "backlight_pin"
CONF_DATA_PINS = "data_pins"

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(UEDX48480021),
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_DE_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_PCLK_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_HSYNC_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_VSYNC_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_BACKLIGHT_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_DATA_PINS): cv.Schema(
                {
                    cv.Required("red"): cv.All(
                        [pins.internal_gpio_output_pin_schema], cv.Length(min=5, max=5)
                    ),
                    cv.Required("green"): cv.All(
                        [pins.internal_gpio_output_pin_schema], cv.Length(min=6, max=6)
                    ),
                    cv.Required("blue"): cv.All(
                        [pins.internal_gpio_output_pin_schema], cv.Length(min=5, max=5)
                    ),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema(cs_pin_required=False)),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await display.register_display(var, config)
    await spi.register_spi_device(var, config)

    rst = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(rst))

    de = await cg.gpio_pin_expression(config[CONF_DE_PIN])
    cg.add(var.set_de_pin(de))

    pclk = await cg.gpio_pin_expression(config[CONF_PCLK_PIN])
    cg.add(var.set_pclk_pin(pclk))

    hsync = await cg.gpio_pin_expression(config[CONF_HSYNC_PIN])
    cg.add(var.set_hsync_pin(hsync))

    vsync = await cg.gpio_pin_expression(config[CONF_VSYNC_PIN])
    cg.add(var.set_vsync_pin(vsync))

    if CONF_BACKLIGHT_PIN in config:
        bl = await cg.gpio_pin_expression(config[CONF_BACKLIGHT_PIN])
        cg.add(var.set_backlight_pin(bl))

    data_pins = config[CONF_DATA_PINS]
    for color in ["red", "green", "blue"]:
        for pin_config in data_pins[color]:
            pin = await cg.gpio_pin_expression(pin_config)
            cg.add(getattr(var, f"add_{color}_pin")(pin))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
