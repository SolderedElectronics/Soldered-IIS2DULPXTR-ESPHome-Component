/**
 * @file iis2dulpx.h
 * @brief Public API for the iis2dulpx ESPHome sensor component
 * @author Soldered Electronics
 */

#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include "iis2dulpx_reg.h"

namespace esphome {
namespace iis2dulpx {

/// Full-scale range of the accelerometer.
enum IIS2DULPXRange : uint8_t {
  RANGE_2G = IIS2DULPX_2g,
  RANGE_4G = IIS2DULPX_4g,
  RANGE_8G = IIS2DULPX_8g,
  RANGE_16G = IIS2DULPX_16g,
};

/// Output data rate and power mode (ULP = ultra-low-power, LP = low-power, HP = high-performance).
enum IIS2DULPXODR : uint8_t {
  ODR_1HZ6_ULP = IIS2DULPX_1Hz6_ULP,
  ODR_3HZ_ULP = IIS2DULPX_3Hz_ULP,
  ODR_25HZ_ULP = IIS2DULPX_25Hz_ULP,
  ODR_6HZ_LP = IIS2DULPX_6Hz_LP,
  ODR_12HZ5_LP = IIS2DULPX_12Hz5_LP,
  ODR_25HZ_LP = IIS2DULPX_25Hz_LP,
  ODR_50HZ_LP = IIS2DULPX_50Hz_LP,
  ODR_100HZ_LP = IIS2DULPX_100Hz_LP,
  ODR_200HZ_LP = IIS2DULPX_200Hz_LP,
  ODR_400HZ_LP = IIS2DULPX_400Hz_LP,
  ODR_800HZ_LP = IIS2DULPX_800Hz_LP,
  ODR_6HZ_HP = IIS2DULPX_6Hz_HP,
  ODR_12HZ5_HP = IIS2DULPX_12Hz5_HP,
  ODR_25HZ_HP = IIS2DULPX_25Hz_HP,
  ODR_50HZ_HP = IIS2DULPX_50Hz_HP,
  ODR_100HZ_HP = IIS2DULPX_100Hz_HP,
  ODR_200HZ_HP = IIS2DULPX_200Hz_HP,
  ODR_400HZ_HP = IIS2DULPX_400Hz_HP,
  ODR_800HZ_HP = IIS2DULPX_800Hz_HP,
};

/// Free-fall detection threshold.
enum IIS2DULPXFreeFallThreshold : uint8_t {
  FF_THS_156MG = IIS2DULPX_156_mg,
  FF_THS_219MG = IIS2DULPX_219_mg,
  FF_THS_250MG = IIS2DULPX_250_mg,
  FF_THS_312MG = IIS2DULPX_312_mg,
  FF_THS_344MG = IIS2DULPX_344_mg,
  FF_THS_406MG = IIS2DULPX_406_mg,
  FF_THS_469MG = IIS2DULPX_469_mg,
  FF_THS_500MG = IIS2DULPX_500_mg,
};

/// Axis tap detection runs on.
enum IIS2DULPXTapAxis : uint8_t {
  TAP_AXIS_X = IIS2DULPX_TAP_ON_X,
  TAP_AXIS_Y = IIS2DULPX_TAP_ON_Y,
  TAP_AXIS_Z = IIS2DULPX_TAP_ON_Z,
};

/**
 * @brief Soldered IIS2DULPX accelerometer breakout board.
 *
 * Thin ESPHome wrapper around STMicroelectronics' platform-independent IIS2DULPX driver, which
 * is vendored unmodified in iis2dulpx_reg.c and iis2dulpx_reg.h (same copy the Soldered Arduino
 * library ships). Every event (wake-up, free-fall, tap, tilt) is latched by the sensor and read
 * back on the update interval, so no INT pin needs to be wired.
 */
class IIS2DULPXComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_accel_x_sensor(sensor::Sensor *sensor) { this->accel_x_sensor_ = sensor; }
  void set_accel_y_sensor(sensor::Sensor *sensor) { this->accel_y_sensor_ = sensor; }
  void set_accel_z_sensor(sensor::Sensor *sensor) { this->accel_z_sensor_ = sensor; }
  void set_temperature_sensor(sensor::Sensor *sensor) { this->temperature_sensor_ = sensor; }
  void set_step_count_sensor(sensor::Sensor *sensor) { this->step_count_sensor_ = sensor; }

  void set_motion_binary_sensor(binary_sensor::BinarySensor *sensor) { this->motion_binary_sensor_ = sensor; }
  void set_free_fall_binary_sensor(binary_sensor::BinarySensor *sensor) { this->free_fall_binary_sensor_ = sensor; }
  void set_single_tap_binary_sensor(binary_sensor::BinarySensor *sensor) { this->single_tap_binary_sensor_ = sensor; }
  void set_double_tap_binary_sensor(binary_sensor::BinarySensor *sensor) { this->double_tap_binary_sensor_ = sensor; }
  void set_triple_tap_binary_sensor(binary_sensor::BinarySensor *sensor) { this->triple_tap_binary_sensor_ = sensor; }
  void set_tilt_binary_sensor(binary_sensor::BinarySensor *sensor) { this->tilt_binary_sensor_ = sensor; }

  void set_range(IIS2DULPXRange range) { this->range_ = range; }
  void set_odr(IIS2DULPXODR odr) { this->odr_ = odr; }
  /// Raw WAKE_UP_THS.wk_ths value, 1 LSB = full-scale / 64.
  void set_motion_threshold(uint8_t threshold) { this->motion_threshold_ = threshold; }
  /// Wake-up duration in ODR cycles, one of 0, 1, 2, 3, 7, 11, 15.
  void set_motion_duration(uint8_t duration) { this->motion_duration_ = duration; }
  void set_free_fall_threshold(IIS2DULPXFreeFallThreshold threshold) { this->free_fall_threshold_ = threshold; }
  /// Free-fall duration in ODR cycles, 0..63.
  void set_free_fall_duration(uint8_t duration) { this->free_fall_duration_ = duration; }
  void set_tap_axis(IIS2DULPXTapAxis axis) { this->tap_axis_ = axis; }
  /// Raw TAP_CFG4.peak_ths value, 1 LSB = 62.5 mg.
  void set_tap_threshold(uint8_t threshold) { this->tap_threshold_ = threshold; }

 protected:
  /// Read callback handed to the ST driver.
  static int32_t i2c_read(void *handle, uint8_t reg, uint8_t *data, uint16_t len);
  /// Write callback handed to the ST driver.
  static int32_t i2c_write(void *handle, uint8_t reg, uint8_t *data, uint16_t len);
  /// Delay callback handed to the ST driver.
  static void delay_ms(uint32_t ms);

  bool uses_wake_up_() const { return this->motion_binary_sensor_ != nullptr; }
  bool uses_free_fall_() const { return this->free_fall_binary_sensor_ != nullptr; }
  bool uses_tap_() const {
    return this->single_tap_binary_sensor_ != nullptr || this->double_tap_binary_sensor_ != nullptr ||
           this->triple_tap_binary_sensor_ != nullptr;
  }
  bool uses_tilt_() const { return this->tilt_binary_sensor_ != nullptr; }
  bool uses_embedded_functions_() const { return this->step_count_sensor_ != nullptr || this->uses_tilt_(); }

  /// Configures wake-up (motion) detection, only called when a motion binary sensor is configured.
  bool configure_wake_up_();
  /// Configures free-fall detection, only called when a free-fall binary sensor is configured.
  bool configure_free_fall_();
  /// Configures tap detection, only called when at least one tap binary sensor is configured.
  bool configure_tap_();
  /// Enables the on-chip step counter, only called when a step count sensor is configured.
  bool configure_step_counter_();
  /// Enables the on-chip tilt detector, only called when a tilt binary sensor is configured.
  bool configure_tilt_();
  /// Routes the enabled events to INT1 and latches them until read.
  bool configure_interrupts_();

  sensor::Sensor *accel_x_sensor_{nullptr};
  sensor::Sensor *accel_y_sensor_{nullptr};
  sensor::Sensor *accel_z_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *step_count_sensor_{nullptr};

  binary_sensor::BinarySensor *motion_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *free_fall_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *single_tap_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *double_tap_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *triple_tap_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *tilt_binary_sensor_{nullptr};

  IIS2DULPXRange range_{RANGE_2G};
  IIS2DULPXODR odr_{ODR_100HZ_HP};
  uint8_t motion_threshold_{8};
  uint8_t motion_duration_{2};
  IIS2DULPXFreeFallThreshold free_fall_threshold_{FF_THS_312MG};
  uint8_t free_fall_duration_{3};
  IIS2DULPXTapAxis tap_axis_{TAP_AXIS_Z};
  uint8_t tap_threshold_{8};

  ::iis2dulpx_ctx_t ctx_{};
  ::iis2dulpx_md_t mode_{};
};

}  // namespace iis2dulpx
}  // namespace esphome
