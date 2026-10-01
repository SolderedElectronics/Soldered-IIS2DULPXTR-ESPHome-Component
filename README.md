# Soldered IIS2DULPX Accelerometer ESPHome Component

| ![IIS2DULPX breakout board](https://cms.soldered.com/products/333363/media/333363_featured-photo_343e6d.png) |
| :--------------------------------------------------------------------------------------: |
|                      [IIS2DULPX breakout board](https://www.solde.red/333363)                       |

The IIS2DULPX Accelerometer breakout brings industrial-grade motion sensing to your projects while barely touching your battery, sipping just 3 µA in ultra-low-power mode and dropping to 12 nA in deep power-down. Built around STMicroelectronics' IIS2DULPX, it measures 3-axis acceleration across four selectable ranges (±2g to ±16g) at up to 800 Hz, with on-chip algorithms for wake-up, free-fall, tap recognition, step counting and tilt. Part of the [Qwiic ecosystem](https://soldered.com/collections/qwiic-ecosystem).

External ESPHome component for the Soldered IIS2DULPX Accelerometer breakout. Communication is I2C only, default address `0x19`.

Not covered by this component: SPI, Qvar (electrostatic sensing), 6D orientation, significant motion, FIFO and the machine-learning core (MLC/FSM). The sensor supports these, this component doesn't expose them.

## Repository Contents

- **components/** - the ESPHome external component (Python config + C++ implementation)
- **examples/** - example YAML configs showing how to use the component

## Usage

Reference this repo directly from your own ESPHome YAML (no need to clone it locally):

```yaml
external_components:
  - source: github://SolderedElectronics/Soldered-IIS2DULPXTR-ESPHome-Component
    components: [iis2dulpx]

i2c:
  sda: GPIO8
  scl: GPIO9

sensor:
  - platform: iis2dulpx
    update_interval: 1s
    accel_x:
      name: "IIS2DULPX Accel X"
    accel_y:
      name: "IIS2DULPX Accel Y"
    accel_z:
      name: "IIS2DULPX Accel Z"
    temperature:
      name: "IIS2DULPX Temperature"
    step_count:
      name: "IIS2DULPX Step Count"

binary_sensor:
  - platform: iis2dulpx
    type: motion
    name: "IIS2DULPX Motion"
```

See [`examples/basic.yaml`](examples/basic.yaml) for a full working example.

### Configuration variables

The `sensor` platform entry owns the sensor configuration. Include it even if you only use binary sensors.

- **address** (*Optional*, int): I2C address of the sensor. Defaults to `0x19`.
- **update_interval** (*Optional*, [Time](https://esphome.io/guides/configuration-types#config-time)): how often
  values are read and published. Event binary sensors are also polled at this rate. Defaults to `60s`.
- **range** (*Optional*, string): full-scale range, one of `2G`, `4G`, `8G`, `16G`. Defaults to `2G`.
- **odr** (*Optional*, string): output data rate and power mode. The suffix is the power mode: `ULP` ultra-low-power,
  `LP` low-power, `HP` high-performance. One of `1.6HZ_ULP`, `3HZ_ULP`, `25HZ_ULP`, `6HZ_LP`, `12.5HZ_LP`, `25HZ_LP`,
  `50HZ_LP`, `100HZ_LP`, `200HZ_LP`, `400HZ_LP`, `800HZ_LP`, `6HZ_HP`, `12.5HZ_HP`, `25HZ_HP`, `50HZ_HP`, `100HZ_HP`,
  `200HZ_HP`, `400HZ_HP`, `800HZ_HP`. Defaults to `100HZ_HP`.
- **accel_x**, **accel_y**, **accel_z** (*Optional*): [Sensors](https://esphome.io/components/sensor/) publishing
  acceleration in m/s².
- **temperature** (*Optional*): [Sensor](https://esphome.io/components/sensor/) publishing the sensor's internal die
  temperature in °C. This is meant for thermal compensation, not for measuring ambient temperature.
- **step_count** (*Optional*): [Sensor](https://esphome.io/components/sensor/) publishing the on-chip step counter's
  count (16 bit). The count goes back to 0 on every boot, because the sensor is reset during setup.
- **motion_threshold** (*Optional*, float): wake-up threshold in g, used by the `motion` binary sensor. 1 step =
  range / 64 (0.03125 g at `2G`), at most 63 steps. Defaults to `0.25`.
- **motion_duration** (*Optional*, int): number of ODR cycles the threshold must be exceeded for, one of `0`, `1`, `2`,
  `3`, `7`, `11`, `15`. Defaults to `2`.
- **free_fall_threshold** (*Optional*, string): free-fall threshold, one of `156MG`, `219MG`, `250MG`, `312MG`,
  `344MG`, `406MG`, `469MG`, `500MG`. Defaults to `312MG`.
- **free_fall_duration** (*Optional*, int): number of ODR cycles, `0`-`63`, the board must stay in free-fall before the
  event fires. Defaults to `3`.
- **tap_axis** (*Optional*, string): axis tap detection runs on, one of `X`, `Y`, `Z`. Defaults to `Z`.
- **tap_threshold** (*Optional*, float): minimum tap peak in g, `0.0625`-`3.9375` in 0.0625 g steps. Defaults to
  `0.5`.

The `iis2dulpx` component also provides a `binary_sensor` platform:

- **platform: iis2dulpx** (required)
- **type** (*Required*, string): one of `motion`, `free_fall`, `single_tap`, `double_tap`, `triple_tap`, `tilt`.
- **name** (*Required*, string)

The sensor latches each event until it is read. A binary sensor shows `on` for one update interval when its event
happened since the previous update, then goes back to `off`. For `motion`, that means it stays `on` while the board
keeps moving. Choose a short `update_interval` (e.g. `500ms`-`1s`) if you use event binary sensors. No INT pin needs to
be wired. Like the Arduino library and the ESP-IDF component, enabled events are also routed to the INT1 pin.

Tap timing parameters (still/shock/latency windows) are counted in samples, so they depend on `odr`. They use the
values from ST's reference tap example for this sensor family, which runs at 400 Hz. Use `400HZ_HP` or `400HZ_LP` for
tap detection. The board must be still just before and after a tap, so taps are detected reliably with the board
resting on a surface, not held in a hand. Adjust `tap_threshold` for your mounting.

### Examples

- [`examples/basic.yaml`](examples/basic.yaml) - reads X, Y, Z acceleration, temperature, step count and motion state.
- [`examples/gestures.yaml`](examples/gestures.yaml) - free-fall, single/double tap and tilt detection at 400 Hz.

The examples are written for an ESP32-S3 board (`esp32-s3-devkitc-1`) with the sensor on `GPIO8` (SDA) / `GPIO9` (SCL).

### Development

Run the following before committing to auto-format the component against ESPHome's own style:

```sh
pip install clang-format==13.0.1
find components \( -name "*.cpp" -o -name "*.h" \) -not -name "iis2dulpx_reg*" | xargs clang-format -i
```

The vendored STMicroelectronics driver (`iis2dulpx_reg*`) is kept unmodified and is excluded from formatting. CI runs
the same check on every push/PR via `.github/workflows/format_check.yml` and fails on unformatted code.
`.github/workflows/build.yml` compiles every YAML under `examples/` on every push/PR.

### Hardware design

You can find hardware design for this board in the _IIS2DULPXTR Accelerometer breakout_ hardware repository.

### Documentation

Access library documentation [here](https://docs.soldered.com/).

### About Soldered

<img src="https://raw.githubusercontent.com/SolderedElectronics/Soldered-Generic-Arduino-Library/dev/extras/Soldered-logo-color.png" alt="soldered-logo" width="500"/>

At Soldered, we design and manufacture a wide selection of electronic products to help you turn your ideas into acts and bring you one step closer to your final project. Our products are intented for makers and crafted in-house by our experienced team in Osijek, Croatia. We believe that sharing is a crucial element for improvement and innovation, and we work hard to stay connected with all our makers regardless of their skill or experience level. Therefore, all our products are open-source. Finally, we always have your back. If you face any problem concerning either your shopping experience or your electronics project, our team will help you deal with it, offering efficient customer service and cost-free technical support anytime. Some of those might be useful for you:

- [Web Store](https://www.soldered.com/shop)
- [Tutorials & Projects](https://soldered.com/learn)
- [Documentation](https://docs.soldered.com)

### Original source

This component uses STMicroelectronics' platform-independent IIS2DULPX driver (`iis2dulpx_reg.c`/`iis2dulpx_reg.h`), vendored unmodified from the [Soldered IIS2DULPXTR Arduino library](https://github.com/SolderedElectronics/Soldered-IIS2DULPXTR-Accelerometer-Arduino-Library), which takes it from [stm32duino/IIS2DULPX](https://github.com/stm32duino/IIS2DULPX). BSD-3-Clause, Copyright (c) 2024 STMicroelectronics - see `components/iis2dulpx/ST_IIS2DULPX_LICENSE`. Thank you, STMicroelectronics.

### Open-source license

Soldered invests vast amounts of time into hardware & software for these products, which are all open-source. Please support future development by buying one of our products.

Check license details in the LICENSE file. Long story short, use these open-source files for any purpose you want to, as long as you apply the same open-source licence to it and disclose the original source. No warranty - all designs in this repository are distributed in the hope that they will be useful, but without any warranty. They are provided "AS IS", therefore without warranty of any kind, either expressed or implied. The entire quality and performance of what you do with the contents of this repository are your responsibility. In no event, Soldered (TAVU) will be liable for your damages, losses, including any general, special, incidental or consequential damage arising out of the use or inability to use the contents of this repository.

## Have fun!

And thank you from your fellow makers at Soldered Electronics.
