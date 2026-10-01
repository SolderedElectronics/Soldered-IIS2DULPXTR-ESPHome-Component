import esphome.codegen as cg
from esphome.components import i2c

CODEOWNERS = ["@SolderedElectronics"]

iis2dulpx_ns = cg.esphome_ns.namespace("iis2dulpx")
IIS2DULPXComponent = iis2dulpx_ns.class_("IIS2DULPXComponent", cg.PollingComponent, i2c.I2CDevice)
