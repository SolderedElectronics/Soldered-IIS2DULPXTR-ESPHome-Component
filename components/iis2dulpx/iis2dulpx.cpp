/**
 * @file iis2dulpx.cpp
 * @brief Implementation for the iis2dulpx ESPHome sensor component
 * @author Soldered Electronics
 */

#include "iis2dulpx.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace iis2dulpx {

static const char *const TAG = "iis2dulpx";

/// Earth's standard gravity, used to convert mg to m/s^2.
static const float GRAVITY_EARTH = 9.80665f;

/// Time the sensor needs to leave deep power-down after the first I2C transaction (datasheet, AN5812).
static const uint32_t DEEP_POWER_DOWN_EXIT_MS = 25;
/// Time the sensor needs to initialise its embedded functions (iis2dulpx_reg.h, IIS2DULPX_SENSOR_EMB_FUNC_ON).
static const uint32_t EMBEDDED_FUNCTIONS_INIT_MS = 10;

/// Returns the anti-aliasing bandwidth for an ODR. The low-power rates below 50 Hz only accept a
/// subset of bandwidths (see iis2dulpx_mode_set()), so the widest allowed one is picked for those.
static iis2dulpx_bw_t bandwidth_for_odr(IIS2DULPXODR odr) {
  switch (odr) {
    case ODR_6HZ_LP:
      return IIS2DULPX_ODR_div_16;
    case ODR_12HZ5_LP:
      return IIS2DULPX_ODR_div_8;
    case ODR_25HZ_LP:
      return IIS2DULPX_ODR_div_4;
    default:
      return IIS2DULPX_ODR_div_2;
  }
}

/// Maps a wake-up duration in ODR cycles to the driver's encoding.
static iis2dulpx_wake_dur_t wake_duration_for_cycles(uint8_t cycles) {
  switch (cycles) {
    case 0:
      return IIS2DULPX_0_ODR;
    case 1:
      return IIS2DULPX_1_ODR;
    case 2:
      return IIS2DULPX_2_ODR;
    case 3:
      return IIS2DULPX_3_ODR;
    case 7:
      return IIS2DULPX_7_ODR;
    case 11:
      return IIS2DULPX_11_ODR;
    default:
      return IIS2DULPX_15_ODR;
  }
}

void IIS2DULPXComponent::setup() {
  ESP_LOGCONFIG(TAG, "Running setup");

  this->ctx_.write_reg = IIS2DULPXComponent::i2c_write;
  this->ctx_.read_reg = IIS2DULPXComponent::i2c_read;
  this->ctx_.mdelay = IIS2DULPXComponent::delay_ms;
  this->ctx_.handle = this;

  // The first transaction wakes the sensor from deep power-down and is expected to fail, same as
  // the Arduino library's begin().
  uint8_t id = 0;
  (void) iis2dulpx_device_id_get(&this->ctx_, &id);
  delay(DEEP_POWER_DOWN_EXIT_MS);

  if (iis2dulpx_device_id_get(&this->ctx_, &id) != 0) {
    ESP_LOGE(TAG, "Communication failed");
    this->mark_failed();
    return;
  }
  if (id != IIS2DULPX_ID) {
    ESP_LOGE(TAG, "Wrong WHO_AM_I: 0x%02X, expected 0x%02X", id, IIS2DULPX_ID);
    this->mark_failed();
    return;
  }

  // Reset to power-on defaults so a configuration left over from before an ESP-only reboot doesn't
  // stay active.
  if (iis2dulpx_init_set(&this->ctx_, IIS2DULPX_RESET) != 0) {
    ESP_LOGE(TAG, "Soft reset failed");
    this->mark_failed();
    return;
  }

  // Enable the I3C anti-spike filter, same as the Arduino library's begin().
  iis2dulpx_i3c_cfg_t i3c_cfg{};
  if (iis2dulpx_i3c_configure_get(&this->ctx_, &i3c_cfg) != 0) {
    ESP_LOGE(TAG, "Reading the I3C configuration failed");
    this->mark_failed();
    return;
  }
  i3c_cfg.asf_on = PROPERTY_ENABLE;
  if (iis2dulpx_i3c_configure_set(&this->ctx_, &i3c_cfg) != 0 ||
      iis2dulpx_mem_bank_set(&this->ctx_, IIS2DULPX_MAIN_MEM_BANK) != 0) {
    ESP_LOGE(TAG, "Writing the interface configuration failed");
    this->mark_failed();
    return;
  }

  // Block data update and register address auto-increment, plus the embedded functions block when
  // the step counter or tilt detector is used.
  const iis2dulpx_init_t init =
      this->uses_embedded_functions_() ? IIS2DULPX_SENSOR_EMB_FUNC_ON : IIS2DULPX_SENSOR_ONLY_ON;
  if (iis2dulpx_init_set(&this->ctx_, init) != 0) {
    ESP_LOGE(TAG, "Initialization failed");
    this->mark_failed();
    return;
  }
  if (this->uses_embedded_functions_())
    delay(EMBEDDED_FUNCTIONS_INIT_MS);

  if (this->uses_wake_up_() && !this->configure_wake_up_()) {
    this->mark_failed();
    return;
  }
  if (this->uses_free_fall_() && !this->configure_free_fall_()) {
    this->mark_failed();
    return;
  }
  if (this->uses_tap_() && !this->configure_tap_()) {
    this->mark_failed();
    return;
  }
  if (this->step_count_sensor_ != nullptr && !this->configure_step_counter_()) {
    this->mark_failed();
    return;
  }
  if (this->uses_tilt_() && !this->configure_tilt_()) {
    this->mark_failed();
    return;
  }
  if ((this->uses_wake_up_() || this->uses_free_fall_() || this->uses_tap_() || this->uses_tilt_()) &&
      !this->configure_interrupts_()) {
    this->mark_failed();
    return;
  }

  // Leave power-down last, once every detector is configured.
  this->mode_.odr = static_cast<iis2dulpx_odr_t>(this->odr_);
  this->mode_.fs = static_cast<iis2dulpx_fs_t>(this->range_);
  this->mode_.bw = bandwidth_for_odr(this->odr_);
  if (iis2dulpx_mode_set(&this->ctx_, &this->mode_) != 0) {
    ESP_LOGE(TAG, "Setting the range and output data rate failed");
    this->mark_failed();
    return;
  }
}

bool IIS2DULPXComponent::configure_wake_up_() {
  iis2dulpx_wakeup_config_t wakeup{};
  wakeup.wake_dur = wake_duration_for_cycles(this->motion_duration_);
  wakeup.sleep_dur = 0;
  wakeup.wake_ths = this->motion_threshold_;
  wakeup.wake_ths_weight = 0;  // 1 LSB = full-scale / 64
  wakeup.wake_enable = IIS2DULPX_SLEEP_OFF;
  wakeup.inact_odr = IIS2DULPX_ODR_NO_CHANGE;
  if (iis2dulpx_wakeup_config_set(&this->ctx_, wakeup) != 0) {
    ESP_LOGE(TAG, "Configuring wake-up detection failed");
    return false;
  }

  // iis2dulpx_wakeup_config_set() only enables the per-axis wake-up comparators together with
  // sleep mode, so enable them separately, same as the Arduino library's Enable_Wake_Up_Detection().
  iis2dulpx_ctrl1_t ctrl1;
  if (iis2dulpx_read_reg(&this->ctx_, IIS2DULPX_CTRL1, reinterpret_cast<uint8_t *>(&ctrl1), 1) != 0) {
    ESP_LOGE(TAG, "Reading CTRL1 failed");
    return false;
  }
  ctrl1.wu_x_en = PROPERTY_ENABLE;
  ctrl1.wu_y_en = PROPERTY_ENABLE;
  ctrl1.wu_z_en = PROPERTY_ENABLE;
  if (iis2dulpx_write_reg(&this->ctx_, IIS2DULPX_CTRL1, reinterpret_cast<uint8_t *>(&ctrl1), 1) != 0) {
    ESP_LOGE(TAG, "Enabling wake-up axes failed");
    return false;
  }

  return true;
}

bool IIS2DULPXComponent::configure_free_fall_() {
  if (iis2dulpx_ff_thresholds_set(&this->ctx_, static_cast<iis2dulpx_ff_thresholds_t>(this->free_fall_threshold_)) !=
          0 ||
      iis2dulpx_ff_duration_set(&this->ctx_, this->free_fall_duration_) != 0) {
    ESP_LOGE(TAG, "Configuring free-fall detection failed");
    return false;
  }
  return true;
}

bool IIS2DULPXComponent::configure_tap_() {
  iis2dulpx_tap_config_t tap{};
  if (iis2dulpx_tap_config_get(&this->ctx_, &tap) != 0) {
    ESP_LOGE(TAG, "Reading the tap configuration failed");
    return false;
  }

  // Values from ST's lis2duxs12_tap.c example (the LIS2DUXS12 shares this sensor's register map
  // and tap engine), verified on hardware at 400 Hz. Timing fields are in samples, so they scale
  // with the ODR. pre_still_n must be set explicitly: at its power-on value of 0 no tap is ever
  // detected.
  tap.axis = static_cast<iis2dulpx_axis_t>(this->tap_axis_);
  tap.peak_ths = this->tap_threshold_;
  tap.pre_still_ths = 2;   // 125 mg
  tap.post_still_ths = 8;  // 500 mg
  tap.post_still_time = 8;
  tap.shock_wait_time = 3;
  tap.latency = 4;
  tap.wait_end_latency = 1;
  tap.rebound = 0;
  tap.inverted_peak_time = 4;
  tap.pre_still_start = 0;
  tap.pre_still_n = 10;
  tap.single_tap_on = this->single_tap_binary_sensor_ != nullptr;
  tap.double_tap_on = this->double_tap_binary_sensor_ != nullptr;
  tap.triple_tap_on = this->triple_tap_binary_sensor_ != nullptr;

  if (iis2dulpx_tap_config_set(&this->ctx_, tap) != 0) {
    ESP_LOGE(TAG, "Configuring tap detection failed");
    return false;
  }
  return true;
}

bool IIS2DULPXComponent::configure_step_counter_() {
  // Same sequence as ST's iis2dulpx_pedometer.c example.
  iis2dulpx_stpcnt_mode_t mode{};
  mode.step_counter_enable = PROPERTY_ENABLE;
  mode.false_step_rej = PROPERTY_DISABLE;
  mode.step_counter_in_fifo = PROPERTY_DISABLE;

  if (iis2dulpx_stpcnt_debounce_set(&this->ctx_, 4) != 0 || iis2dulpx_stpcnt_mode_set(&this->ctx_, mode) != 0 ||
      iis2dulpx_stpcnt_rst_step_set(&this->ctx_) != 0) {
    ESP_LOGE(TAG, "Configuring the step counter failed");
    return false;
  }
  return true;
}

bool IIS2DULPXComponent::configure_tilt_() {
  if (iis2dulpx_tilt_mode_set(&this->ctx_, PROPERTY_ENABLE) != 0 ||
      iis2dulpx_embedded_int_cfg_set(&this->ctx_, IIS2DULPX_EMBEDDED_INT_LATCHED) != 0) {
    ESP_LOGE(TAG, "Configuring tilt detection failed");
    return false;
  }

  iis2dulpx_emb_pin_int_route_t emb_route{};
  if (iis2dulpx_emb_pin_int1_route_get(&this->ctx_, &emb_route) != 0) {
    ESP_LOGE(TAG, "Reading the embedded interrupt routing failed");
    return false;
  }
  emb_route.tilt = PROPERTY_ENABLE;
  if (iis2dulpx_emb_pin_int1_route_set(&this->ctx_, &emb_route) != 0) {
    ESP_LOGE(TAG, "Routing tilt detection failed");
    return false;
  }
  return true;
}

bool IIS2DULPXComponent::configure_interrupts_() {
  // Events are routed to INT1 the same way the Arduino library and the Soldered ESP-IDF component
  // do it. The pin itself doesn't need to be connected, the status registers are polled instead.
  iis2dulpx_pin_int_route_t route{};
  if (iis2dulpx_pin_int1_route_get(&this->ctx_, &route) != 0) {
    ESP_LOGE(TAG, "Reading the interrupt routing failed");
    return false;
  }
  route.wake_up = this->uses_wake_up_();
  route.free_fall = this->uses_free_fall_();
  route.tap = this->uses_tap_();
  route.emb_function = this->uses_tilt_();
  if (iis2dulpx_pin_int1_route_set(&this->ctx_, &route) != 0) {
    ESP_LOGE(TAG, "Routing interrupts failed");
    return false;
  }

  // Latch events until their source register is read, otherwise a short event (a tap lasts a few
  // ms) is gone again before the next update.
  iis2dulpx_int_config_t int_config{};
  if (iis2dulpx_int_config_get(&this->ctx_, &int_config) != 0) {
    ESP_LOGE(TAG, "Reading the interrupt configuration failed");
    return false;
  }
  int_config.int_cfg = IIS2DULPX_INT_LATCHED;
  if (iis2dulpx_int_config_set(&this->ctx_, &int_config) != 0) {
    ESP_LOGE(TAG, "Enabling interrupts failed");
    return false;
  }
  return true;
}

void IIS2DULPXComponent::update() {
  if (this->is_failed())
    return;

  if (this->accel_x_sensor_ != nullptr || this->accel_y_sensor_ != nullptr || this->accel_z_sensor_ != nullptr) {
    iis2dulpx_xl_data_t data{};
    if (iis2dulpx_xl_data_get(&this->ctx_, &this->mode_, &data) != 0) {
      ESP_LOGW(TAG, "Reading acceleration failed");
      this->status_set_warning();
      return;
    }

    const float accel_x = data.mg[0] / 1000.0f * GRAVITY_EARTH;
    const float accel_y = data.mg[1] / 1000.0f * GRAVITY_EARTH;
    const float accel_z = data.mg[2] / 1000.0f * GRAVITY_EARTH;
    ESP_LOGD(TAG, "Got accel_x=%.3f accel_y=%.3f accel_z=%.3f m/s^2", accel_x, accel_y, accel_z);

    if (this->accel_x_sensor_ != nullptr)
      this->accel_x_sensor_->publish_state(accel_x);
    if (this->accel_y_sensor_ != nullptr)
      this->accel_y_sensor_->publish_state(accel_y);
    if (this->accel_z_sensor_ != nullptr)
      this->accel_z_sensor_->publish_state(accel_z);
  }

  if (this->temperature_sensor_ != nullptr) {
    iis2dulpx_outt_data_t data{};
    if (iis2dulpx_outt_data_get(&this->ctx_, &data) != 0) {
      ESP_LOGW(TAG, "Reading temperature failed");
      this->status_set_warning();
      return;
    }
    ESP_LOGD(TAG, "Got temperature=%.1f C", data.heat.deg_c);
    this->temperature_sensor_->publish_state(data.heat.deg_c);
  }

  if (this->step_count_sensor_ != nullptr) {
    uint16_t steps = 0;
    if (iis2dulpx_stpcnt_steps_get(&this->ctx_, &steps) != 0) {
      ESP_LOGW(TAG, "Reading the step count failed");
      this->status_set_warning();
      return;
    }
    ESP_LOGD(TAG, "Got step_count=%u", steps);
    this->step_count_sensor_->publish_state(steps);
  }

  if (this->uses_wake_up_() || this->uses_free_fall_() || this->uses_tap_()) {
    // Reads (and so clears) the latched wake-up, free-fall and tap source registers in one go.
    iis2dulpx_all_sources_t sources{};
    if (iis2dulpx_all_sources_get(&this->ctx_, &sources) != 0) {
      ESP_LOGW(TAG, "Reading the event status failed");
      this->status_set_warning();
      return;
    }

    if (this->uses_wake_up_() && sources.wake_up)
      ESP_LOGD(TAG, "Detected motion");
    if (this->uses_free_fall_() && sources.free_fall)
      ESP_LOGD(TAG, "Detected free-fall");
    if (this->single_tap_binary_sensor_ != nullptr && sources.single_tap)
      ESP_LOGD(TAG, "Detected single tap");
    if (this->double_tap_binary_sensor_ != nullptr && sources.double_tap)
      ESP_LOGD(TAG, "Detected double tap");
    if (this->triple_tap_binary_sensor_ != nullptr && sources.triple_tap)
      ESP_LOGD(TAG, "Detected triple tap");

    if (this->motion_binary_sensor_ != nullptr)
      this->motion_binary_sensor_->publish_state(sources.wake_up);
    if (this->free_fall_binary_sensor_ != nullptr)
      this->free_fall_binary_sensor_->publish_state(sources.free_fall);
    if (this->single_tap_binary_sensor_ != nullptr)
      this->single_tap_binary_sensor_->publish_state(sources.single_tap);
    if (this->double_tap_binary_sensor_ != nullptr)
      this->double_tap_binary_sensor_->publish_state(sources.double_tap);
    if (this->triple_tap_binary_sensor_ != nullptr)
      this->triple_tap_binary_sensor_->publish_state(sources.triple_tap);
  }

  if (this->uses_tilt_()) {
    // The main-page copy of the embedded function status, verified on hardware to report the
    // latched tilt event. Also saves the two memory bank switches iis2dulpx_embedded_status_get()
    // does.
    iis2dulpx_emb_func_status_mainpage_t status;
    if (iis2dulpx_read_reg(&this->ctx_, IIS2DULPX_EMB_FUNC_STATUS_MAINPAGE, reinterpret_cast<uint8_t *>(&status), 1) !=
        0) {
      ESP_LOGW(TAG, "Reading the tilt status failed");
      this->status_set_warning();
      return;
    }
    if (status.is_tilt)
      ESP_LOGD(TAG, "Detected tilt");
    this->tilt_binary_sensor_->publish_state(status.is_tilt);
  }

  this->status_clear_warning();
}

int32_t IIS2DULPXComponent::i2c_read(void *handle, uint8_t reg, uint8_t *data, uint16_t len) {
  auto *component = static_cast<IIS2DULPXComponent *>(handle);
  if (component == nullptr)
    return -1;
  return component->read_register(reg, data, len) == i2c::ERROR_OK ? 0 : -1;
}

int32_t IIS2DULPXComponent::i2c_write(void *handle, uint8_t reg, uint8_t *data, uint16_t len) {
  auto *component = static_cast<IIS2DULPXComponent *>(handle);
  if (component == nullptr)
    return -1;
  return component->write_register(reg, data, len) == i2c::ERROR_OK ? 0 : -1;
}

void IIS2DULPXComponent::delay_ms(uint32_t ms) { delay(ms); }

void IIS2DULPXComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "IIS2DULPX:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with IIS2DULPX failed!");
    return;
  }
  LOG_UPDATE_INTERVAL(this);

  ESP_LOGCONFIG(TAG,
                "  Range: +-%ug\n"
                "  ODR code: 0x%02X",
                2u << this->range_, (unsigned) this->odr_);
  if (this->uses_wake_up_())
    ESP_LOGCONFIG(TAG, "  Motion threshold: %u LSB, duration: %u ODR cycles", this->motion_threshold_,
                  this->motion_duration_);
  if (this->uses_free_fall_())
    ESP_LOGCONFIG(TAG, "  Free-fall threshold code: %u, duration: %u ODR cycles", (unsigned) this->free_fall_threshold_,
                  this->free_fall_duration_);
  if (this->uses_tap_())
    ESP_LOGCONFIG(TAG, "  Tap axis code: %u, threshold: %u LSB", (unsigned) this->tap_axis_, this->tap_threshold_);

  LOG_SENSOR("  ", "Accel X", this->accel_x_sensor_);
  LOG_SENSOR("  ", "Accel Y", this->accel_y_sensor_);
  LOG_SENSOR("  ", "Accel Z", this->accel_z_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Step Count", this->step_count_sensor_);
  LOG_BINARY_SENSOR("  ", "Motion", this->motion_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Free Fall", this->free_fall_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Single Tap", this->single_tap_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Double Tap", this->double_tap_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Triple Tap", this->triple_tap_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Tilt", this->tilt_binary_sensor_);
}

}  // namespace iis2dulpx
}  // namespace esphome
