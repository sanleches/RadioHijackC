#include "rda5807m.hpp"

#include "app_config.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cmath>

#include "hardware/gpio.h"
#include "pico/stdlib.h"

namespace app {

Rda5807m::Rda5807m(i2c_inst_t* i2c, uint sdaPin, uint sclPin, uint32_t busFrequencyHz)
    : i2c_(i2c), sdaPin_(sdaPin), sclPin_(sclPin), busFrequencyHz_(busFrequencyHz),
      volume_(config::kDefaultVolume), frequency_(config::kDefaultFrequencyMhz) {}

bool Rda5807m::begin() {
  i2c_init(i2c_, busFrequencyHz_);
  gpio_set_function(sdaPin_, GPIO_FUNC_I2C);
  gpio_set_function(sclPin_, GPIO_FUNC_I2C);
  gpio_pull_up(sdaPin_);
  gpio_pull_up(sclPin_);
  sleep_ms(200);

  if (!initRadio()) {
    initialized_ = false;
    return false;
  }
  setVolume(config::kDefaultVolume);
  tune(config::kDefaultFrequencyMhz);
  initialized_ = true;
  return true;
}

bool Rda5807m::initRadio() {
  if (!writeRegister(kRegPower, 0x0002)) {
    return false;
  }
  sleep_ms(80);

  const uint16_t power = kBitDhiz | kBitDmute | kBitRdsEn | kBitNewMethod | kBitEnable;
  if (!writeRegister(kRegPower, power)) {
    return false;
  }
  writeRegister(kRegConfig, 0x0040);
  writeRegister(kRegVolume, 0x88A0 | (volume_ & 0x0F));
  sleep_ms(150);
  powered_ = true;
  muted_ = false;
  return true;
}

bool Rda5807m::power(bool on) {
  if (on) {
    if (!powered_) {
      if (!initRadio()) {
        return false;
      }
      setVolume(volume_);
      tune(frequency_);
    }
  } else {
    writeRegister(kRegPower, 0x0000);
    powered_ = false;
  }
  return powered_;
}

float Rda5807m::tune(float frequencyMhz, uint32_t settleMs) {
  if (!powered_) {
    power(true);
  }

  frequencyMhz = std::min(108.0f, std::max(87.0f, frequencyMhz));
  const uint16_t channel = channelForFrequency(frequencyMhz);
  const uint16_t reg3 = static_cast<uint16_t>((channel << 6) | kBitTune);
  writeRegister(kRegChannel, reg3);
  sleep_ms(settleMs);

  frequency_ = std::round(frequencyMhz * 10.0f) / 10.0f;
  currentFrequency();
  rds_.clear();
  return frequency_;
}

float Rda5807m::seek(bool up) {
  if (!powered_) {
    power(true);
  }

  uint16_t reg2 = regPower_ ? regPower_ : static_cast<uint16_t>(kBitDhiz | kBitDmute | kBitRdsEn | kBitNewMethod | kBitEnable);
  if (up) {
    reg2 |= kBitSeekup;
  } else {
    reg2 &= ~kBitSeekup;
  }
  writeRegister(kRegPower, reg2 | kBitSeek);
  waitSeekTuneComplete(6000);
  writeRegister(kRegPower, regPower_ & ~kBitSeek);
  sleep_ms(120);

  frequency_ = currentFrequency();
  rds_.clear();
  return frequency_;
}

float Rda5807m::currentFrequency() {
  uint16_t status = 0;
  if (readRegister(kRegStatus, status)) {
    frequency_ = frequencyForChannel(status & 0x03FF);
  }
  return frequency_;
}

uint8_t Rda5807m::setVolume(int volume) {
  volume = std::min(15, std::max(0, volume));
  const uint16_t base = regVolume_ ? regVolume_ : 0x88A0;
  writeRegister(kRegVolume, static_cast<uint16_t>((base & 0xFFF0) | volume));
  volume_ = static_cast<uint8_t>(volume);
  return volume_;
}

bool Rda5807m::mute(bool on) {
  uint16_t reg2 = regPower_ ? regPower_ : static_cast<uint16_t>(kBitDhiz | kBitDmute | kBitRdsEn | kBitNewMethod | kBitEnable);
  if (on) {
    reg2 &= ~kBitDmute;
  } else {
    reg2 |= kBitDmute;
  }
  writeRegister(kRegPower, reg2);
  muted_ = on;
  return muted_;
}

bool Rda5807m::pollRds() {
  if (!powered_) {
    return false;
  }
  uint16_t status = 0;
  if (!readRegister(kRegStatus, status) || (status & 0x8000) == 0) {
    return false;
  }
  uint16_t blocks[4] = {};
  if (!readRegisters(kRegRdsa, 4, blocks)) {
    return false;
  }
  return rds_.process(blocks[0], blocks[1], blocks[2], blocks[3]);
}

RadioStatus Rda5807m::status() {
  RadioStatus result;
  uint16_t statusReg = 0;
  uint16_t rssiReg = 0;
  readRegister(kRegStatus, statusReg);
  readRegister(kRegRssi, rssiReg);
  frequency_ = frequencyForChannel(statusReg & 0x03FF);
  pollRds();

  result.powered = powered_;
  result.muted = muted_;
  result.frequency = frequency_;
  result.volume = volume_;
  result.station = rds_.station();
  result.song = rds_.text();
  result.rssi = static_cast<uint8_t>((rssiReg >> 9) & 0x7F);
  result.stc = (statusReg & 0x4000) != 0;
  result.rdsReady = (statusReg & 0x8000) != 0;
  result.stereo = (statusReg & 0x0400) != 0;
  result.seekFail = (statusReg & 0x2000) != 0;
  result.readChannel = statusReg & 0x03FF;
  result.debugReg2 = regPower_;
  result.debugReg3 = regChannel_;
  result.debugReg5 = regVolume_;
  result.debugStatusRaw = statusReg;
  result.debugRssiRaw = rssiReg;
  return result;
}

std::vector<ScanStation> Rda5807m::scan(float start, float stop, float step, uint8_t minRssi) {
  start = std::max(87.0f, start);
  stop = std::min(108.0f, stop);
  step = std::max(0.1f, step);

  std::vector<ScanStation> found;
  const float original = frequency_;
  for (float freq = start; freq <= stop + 0.001f; freq = std::round((freq + step) * 10.0f) / 10.0f) {
    const float tuned = tune(freq, 70);
    uint16_t rssiReg = 0;
    readRegister(kRegRssi, rssiReg);
    const uint8_t rssi = static_cast<uint8_t>((rssiReg >> 9) & 0x7F);
    if (rssi >= minRssi) {
      found.push_back({tuned, rssi});
    }
  }
  tune(original);
  return found;
}

bool Rda5807m::writeRegister(uint8_t reg, uint16_t value) {
  const uint8_t data[] = {reg, static_cast<uint8_t>((value >> 8) & 0xFF), static_cast<uint8_t>(value & 0xFF)};
  const int written = i2c_write_blocking(i2c_, kAddress, data, sizeof(data), false);
  if (written != static_cast<int>(sizeof(data))) {
    Logger::info("RDA5807M write failed: reg=0x%02x", reg);
    return false;
  }
  if (reg == kRegPower) {
    regPower_ = value;
  } else if (reg == kRegChannel) {
    regChannel_ = value;
  } else if (reg == kRegVolume) {
    regVolume_ = value;
  }
  return true;
}

bool Rda5807m::readRegister(uint8_t reg, uint16_t& value) {
  if (i2c_write_blocking(i2c_, kAddress, &reg, 1, true) != 1) {
    return false;
  }
  uint8_t data[2] = {};
  if (i2c_read_blocking(i2c_, kAddress, data, sizeof(data), false) != static_cast<int>(sizeof(data))) {
    return false;
  }
  value = static_cast<uint16_t>((data[0] << 8) | data[1]);
  return true;
}

bool Rda5807m::readRegisters(uint8_t startReg, uint8_t count, uint16_t* values) {
  if (i2c_write_blocking(i2c_, kAddress, &startReg, 1, true) != 1) {
    return false;
  }
  uint8_t data[8] = {};
  const uint8_t bytes = count * 2;
  if (bytes > sizeof(data)) {
    return false;
  }
  if (i2c_read_blocking(i2c_, kAddress, data, bytes, false) != bytes) {
    return false;
  }
  for (uint8_t i = 0; i < count; ++i) {
    values[i] = static_cast<uint16_t>((data[i * 2] << 8) | data[i * 2 + 1]);
  }
  return true;
}

bool Rda5807m::waitSeekTuneComplete(uint32_t timeoutMs) {
  const absolute_time_t deadline = make_timeout_time_ms(timeoutMs);
  while (!time_reached(deadline)) {
    uint16_t status = 0;
    if (readRegister(kRegStatus, status) && (status & 0x4000) != 0) {
      return true;
    }
    sleep_ms(20);
  }
  return false;
}

uint16_t Rda5807m::channelForFrequency(float frequencyMhz) const {
  frequencyMhz = std::min(108.0f, std::max(87.0f, frequencyMhz));
  return static_cast<uint16_t>(std::lround((frequencyMhz - 87.0f) * 10.0f));
}

float Rda5807m::frequencyForChannel(uint16_t channel) const {
  return std::round((87.0f + static_cast<float>(channel & 0x03FF) * 0.1f) * 10.0f) / 10.0f;
}

}  // namespace app
