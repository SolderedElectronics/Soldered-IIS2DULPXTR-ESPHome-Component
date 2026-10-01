import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_RANGE,
    CONF_TEMPERATURE,
    DEVICE_CLASS_TEMPERATURE,
    ICON_COUNTER,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_CELSIUS,
    UNIT_METER_PER_SECOND_SQUARED,
)

from . import IIS2DULPXComponent, iis2dulpx_ns

CODEOWNERS = ["@SolderedElectronics"]
DEPENDENCIES = ["i2c"]

CONF_ACCEL_X = "accel_x"
CONF_ACCEL_Y = "accel_y"
CONF_ACCEL_Z = "accel_z"
CONF_ODR = "odr"
CONF_STEP_COUNT = "step_count"
CONF_MOTION_THRESHOLD = "motion_threshold"
CONF_MOTION_DURATION = "motion_duration"
CONF_FREE_FALL_THRESHOLD = "free_fall_threshold"
CONF_FREE_FALL_DURATION = "free_fall_duration"
CONF_TAP_AXIS = "tap_axis"
CONF_TAP_THRESHOLD = "tap_threshold"

IIS2DULPXRange = iis2dulpx_ns.enum("IIS2DULPXRange")
RANGE_OPTIONS = {
    "2G": IIS2DULPXRange.RANGE_2G,
    "4G": IIS2DULPXRange.RANGE_4G,
    "8G": IIS2DULPXRange.RANGE_8G,
    "16G": IIS2DULPXRange.RANGE_16G,
}
RANGE_G = {"2G": 2, "4G": 4, "8G": 8, "16G": 16}

# The suffix selects the power mode: ULP = ultra-low-power, LP = low-power,
# HP = high-performance.
IIS2DULPXODR = iis2dulpx_ns.enum("IIS2DULPXODR")
ODR_OPTIONS = {
    "1.6HZ_ULP": IIS2DULPXODR.ODR_1HZ6_ULP,
    "3HZ_ULP": IIS2DULPXODR.ODR_3HZ_ULP,
    "25HZ_ULP": IIS2DULPXODR.ODR_25HZ_ULP,
    "6HZ_LP": IIS2DULPXODR.ODR_6HZ_LP,
    "12.5HZ_LP": IIS2DULPXODR.ODR_12HZ5_LP,
    "25HZ_LP": IIS2DULPXODR.ODR_25HZ_LP,
    "50HZ_LP": IIS2DULPXODR.ODR_50HZ_LP,
    "100HZ_LP": IIS2DULPXODR.ODR_100HZ_LP,
    "200HZ_LP": IIS2DULPXODR.ODR_200HZ_LP,
    "400HZ_LP": IIS2DULPXODR.ODR_400HZ_LP,
    "800HZ_LP": IIS2DULPXODR.ODR_800HZ_LP,
    "6HZ_HP": IIS2DULPXODR.ODR_6HZ_HP,
    "12.5HZ_HP": IIS2DULPXODR.ODR_12HZ5_HP,
    "25HZ_HP": IIS2DULPXODR.ODR_25HZ_HP,
    "50HZ_HP": IIS2DULPXODR.ODR_50HZ_HP,
    "100HZ_HP": IIS2DULPXODR.ODR_100HZ_HP,
    "200HZ_HP": IIS2DULPXODR.ODR_200HZ_HP,
    "400HZ_HP": IIS2DULPXODR.ODR_400HZ_HP,
    "800HZ_HP": IIS2DULPXODR.ODR_800HZ_HP,
}

IIS2DULPXFreeFallThreshold = iis2dulpx_ns.enum("IIS2DULPXFreeFallThreshold")
FREE_FALL_THRESHOLD_OPTIONS = {
    "156MG": IIS2DULPXFreeFallThreshold.FF_THS_156MG,
    "219MG": IIS2DULPXFreeFallThreshold.FF_THS_219MG,
    "250MG": IIS2DULPXFreeFallThreshold.FF_THS_250MG,
    "312MG": IIS2DULPXFreeFallThreshold.FF_THS_312MG,
    "344MG": IIS2DULPXFreeFallThreshold.FF_THS_344MG,
    "406MG": IIS2DULPXFreeFallThreshold.FF_THS_406MG,
    "469MG": IIS2DULPXFreeFallThreshold.FF_THS_469MG,
    "500MG": IIS2DULPXFreeFallThreshold.FF_THS_500MG,
}

IIS2DULPXTapAxis = iis2dulpx_ns.enum("IIS2DULPXTapAxis")
TAP_AXIS_OPTIONS = {
    "X": IIS2DULPXTapAxis.TAP_AXIS_X,
    "Y": IIS2DULPXTapAxis.TAP_AXIS_Y,
    "Z": IIS2DULPXTapAxis.TAP_AXIS_Z,
}

# Wake-up duration values the sensor supports, in ODR cycles (WAKE_UP_DUR.wake_dur
# plus the WAKE_UP_DUR_EXT.wu_dur_extended range extension).
MOTION_DURATION_OPTIONS = [0, 1, 2, 3, 7, 11, 15]

# Tap peak threshold: 6 bit field, 1 LSB = 62.5 mg.
TAP_THRESHOLD_LSB_G = 0.0625


def validate_motion_threshold(config):
    # WAKE_UP_THS.wk_ths is 6 bits wide, 1 LSB = full-scale / 64.
    lsb_g = RANGE_G[config[CONF_RANGE]] / 64.0
    threshold = config[CONF_MOTION_THRESHOLD]
    if not lsb_g <= threshold <= 63 * lsb_g:
        raise cv.Invalid(
            f"{CONF_MOTION_THRESHOLD} must be between {lsb_g:g} and {63 * lsb_g:g} g "
            f"at range {config[CONF_RANGE]}",
            path=[CONF_MOTION_THRESHOLD],
        )
    return config


ACCEL_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_METER_PER_SECOND_SQUARED,
    accuracy_decimals=3,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(IIS2DULPXComponent),
            cv.Optional(CONF_ACCEL_X): ACCEL_SCHEMA,
            cv.Optional(CONF_ACCEL_Y): ACCEL_SCHEMA,
            cv.Optional(CONF_ACCEL_Z): ACCEL_SCHEMA,
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_STEP_COUNT): sensor.sensor_schema(
                icon=ICON_COUNTER,
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_RANGE, default="2G"): cv.enum(RANGE_OPTIONS, upper=True),
            cv.Optional(CONF_ODR, default="100HZ_HP"): cv.enum(ODR_OPTIONS, upper=True),
            cv.Optional(CONF_MOTION_THRESHOLD, default=0.25): cv.positive_float,
            cv.Optional(CONF_MOTION_DURATION, default=2): cv.one_of(*MOTION_DURATION_OPTIONS, int=True),
            cv.Optional(CONF_FREE_FALL_THRESHOLD, default="312MG"): cv.enum(
                FREE_FALL_THRESHOLD_OPTIONS, upper=True, space=""
            ),
            cv.Optional(CONF_FREE_FALL_DURATION, default=3): cv.int_range(min=0, max=63),
            cv.Optional(CONF_TAP_AXIS, default="Z"): cv.enum(TAP_AXIS_OPTIONS, upper=True),
            cv.Optional(CONF_TAP_THRESHOLD, default=0.5): cv.float_range(
                min=TAP_THRESHOLD_LSB_G, max=63 * TAP_THRESHOLD_LSB_G
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x19)),
    validate_motion_threshold,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_range(config[CONF_RANGE]))
    cg.add(var.set_odr(config[CONF_ODR]))

    motion_lsb_g = RANGE_G[config[CONF_RANGE]] / 64.0
    cg.add(var.set_motion_threshold(round(config[CONF_MOTION_THRESHOLD] / motion_lsb_g)))
    cg.add(var.set_motion_duration(config[CONF_MOTION_DURATION]))
    cg.add(var.set_free_fall_threshold(config[CONF_FREE_FALL_THRESHOLD]))
    cg.add(var.set_free_fall_duration(config[CONF_FREE_FALL_DURATION]))
    cg.add(var.set_tap_axis(config[CONF_TAP_AXIS]))
    cg.add(var.set_tap_threshold(round(config[CONF_TAP_THRESHOLD] / TAP_THRESHOLD_LSB_G)))

    if conf := config.get(CONF_ACCEL_X):
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_accel_x_sensor(sens))

    if conf := config.get(CONF_ACCEL_Y):
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_accel_y_sensor(sens))

    if conf := config.get(CONF_ACCEL_Z):
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_accel_z_sensor(sens))

    if conf := config.get(CONF_TEMPERATURE):
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_temperature_sensor(sens))

    if conf := config.get(CONF_STEP_COUNT):
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_step_count_sensor(sens))
