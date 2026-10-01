import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_MOTION,
    DEVICE_CLASS_MOVING,
    DEVICE_CLASS_VIBRATION,
)

from . import IIS2DULPXComponent

CODEOWNERS = ["@SolderedElectronics"]

CONF_IIS2DULPX_ID = "iis2dulpx_id"

TYPE_MOTION = "motion"
TYPE_FREE_FALL = "free_fall"
TYPE_SINGLE_TAP = "single_tap"
TYPE_DOUBLE_TAP = "double_tap"
TYPE_TRIPLE_TAP = "triple_tap"
TYPE_TILT = "tilt"


def _event_schema(device_class=cv.UNDEFINED):
    return binary_sensor.binary_sensor_schema(device_class=device_class).extend(
        {
            cv.GenerateID(CONF_IIS2DULPX_ID): cv.use_id(IIS2DULPXComponent),
        }
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        TYPE_MOTION: _event_schema(DEVICE_CLASS_MOTION),
        TYPE_FREE_FALL: _event_schema(),
        TYPE_SINGLE_TAP: _event_schema(DEVICE_CLASS_VIBRATION),
        TYPE_DOUBLE_TAP: _event_schema(DEVICE_CLASS_VIBRATION),
        TYPE_TRIPLE_TAP: _event_schema(DEVICE_CLASS_VIBRATION),
        TYPE_TILT: _event_schema(DEVICE_CLASS_MOVING),
    },
    key=CONF_TYPE,
    lower=True,
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_IIS2DULPX_ID])
    sens = await binary_sensor.new_binary_sensor(config)
    cg.add(getattr(hub, f"set_{config[CONF_TYPE]}_binary_sensor")(sens))
