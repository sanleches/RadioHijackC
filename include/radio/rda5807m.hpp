/*
 * =============================================================================
 * RadioHijackC - RDA5807M Radio Driver Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares the FM radio driver used by the dashboard and API.
 *
 * Hardware:
 *   RDA5807M over I2C, default address 0x11, wired to Pico I2C0 on GP4/GP5.
 *
 * Features:
 *   Power, tune, seek, volume, mute, RSSI/status, scanning, and RDS polling.
 *
 * Main types:
 *   RadioStatus - Full status snapshot for API/serial output.
 *   ScanStation - Frequency + RSSI result from scans.
 *   Rda5807m    - High-level driver class.
 */

/**
 * @file rda5807m.hpp
 * @brief RDA5807M FM radio driver and high-level radio operations.
 */

#pragma once

#include "radio/rds_parser.hpp"

#include <cstdint>
#include <vector>

#include "hardware/i2c.h"

namespace app {

/**
 * @brief Snapshot of radio state returned to the HTTP API and serial console.
 */
struct RadioStatus {
  /** @brief true when the radio chip is powered/enabled. */
  bool powered = false;
  /** @brief true when audio is muted. */
  bool muted = false;
  /** @brief Current tuned frequency in MHz. */
  float frequency = 0.0f;
  /** @brief Current volume level, 0 to 15. */
  uint8_t volume = 0;
  /** @brief Latest decoded RDS station name. */
  std::string station;
  /** @brief Latest decoded RDS RadioText/song text. */
  std::string song;
  /** @brief Received signal strength indicator, 0 to 127. */
  uint8_t rssi = 0;
  /** @brief Seek/tune-complete status bit. */
  bool stc = false;
  /** @brief true when the chip reports an RDS group is ready. */
  bool rdsReady = false;
  /** @brief true when the current station is stereo. */
  bool stereo = false;
  /** @brief true when the last seek failed. */
  bool seekFail = false;
  /** @brief Raw channel number reported by the status register. */
  uint16_t readChannel = 0;
  /** @brief Cached power/control register value for diagnostics. */
  uint16_t debugReg2 = 0;
  /** @brief Cached channel register value for diagnostics. */
  uint16_t debugReg3 = 0;
  /** @brief Cached volume register value for diagnostics. */
  uint16_t debugReg5 = 0;
  /** @brief Raw status register value for diagnostics. */
  uint16_t debugStatusRaw = 0;
  /** @brief Raw RSSI register value for diagnostics. */
  uint16_t debugRssiRaw = 0;
};

/**
 * @brief One station found during a scan.
 */
struct ScanStation {
  /** @brief Station frequency in MHz. */
  float frequency = 0.0f;
  /** @brief Signal strength measured at that frequency. */
  uint8_t rssi = 0;
};

/**
 * @brief Driver for the RDA5807M I2C FM receiver module.
 */
class Rda5807m {
 public:
  /**
   * @brief Construct a radio driver bound to an I2C bus and pins.
   * @param i2c Pico SDK I2C instance, usually i2c0.
   * @param sdaPin GPIO used for SDA.
   * @param sclPin GPIO used for SCL.
   * @param busFrequencyHz I2C bus speed in hertz.
   * @return Constructed driver object.
   */
  Rda5807m(i2c_inst_t* i2c, uint sdaPin, uint sclPin, uint32_t busFrequencyHz);

  /**
   * @brief Initialize I2C pins, reset/configure the radio, set volume, and tune default frequency.
   * @param None.
   * @return true if the chip accepted initialization writes, otherwise false.
   */
  bool begin();

  /**
   * @brief Report whether begin() completed successfully.
   * @param None.
   * @return true when the radio has initialized.
   */
  bool initialized() const { return initialized_; }

  /**
   * @brief Turn the radio chip on or off.
   * @param on true to enable the radio, false to power it down.
   * @return Current powered state after the operation.
   */
  bool power(bool on);

  /**
   * @brief Tune to a specific FM frequency.
   * @param frequencyMhz Desired frequency in MHz. Values are clamped to 87.0-108.0.
   * @param settleMs Delay after writing the tune register before reading status.
   * @return Actual tracked frequency in MHz.
   */
  float tune(float frequencyMhz, uint32_t settleMs = 250);

  /**
   * @brief Seek to the next receivable station.
   * @param up true to seek upward, false to seek downward.
   * @return Frequency in MHz reported after the seek completes or times out.
   */
  float seek(bool up);

  /**
   * @brief Read the current tuned frequency from the chip status register.
   * @param None.
   * @return Current frequency in MHz.
   */
  float currentFrequency();

  /**
   * @brief Set audio volume.
   * @param volume Requested volume level. Values are clamped to 0-15.
   * @return Applied volume level.
   */
  uint8_t setVolume(int volume);

  /**
   * @brief Mute or unmute audio output.
   * @param on true to mute, false to unmute.
   * @return Current muted state after the operation.
   */
  bool mute(bool on);

  /**
   * @brief Poll the chip for one ready RDS group and pass it to the parser.
   * @param None.
   * @return true if a supported RDS group was processed.
   */
  bool pollRds();

  /**
   * @brief Read all status fields used by the web API.
   * @param None.
   * @return RadioStatus snapshot.
   */
  RadioStatus status();

  /**
   * @brief Scan a frequency range and collect stations above a signal threshold.
   * @param start Start frequency in MHz.
   * @param stop End frequency in MHz.
   * @param step Frequency increment in MHz. Values below 0.1 are clamped.
   * @param minRssi Minimum RSSI required to include a station.
   * @return Vector of detected stations. The original frequency is restored afterward.
   */
  std::vector<ScanStation> scan(float start, float stop, float step, uint8_t minRssi);

 private:
  static constexpr uint8_t kAddress = 0x11;
  static constexpr uint8_t kRegPower = 0x02;
  static constexpr uint8_t kRegChannel = 0x03;
  static constexpr uint8_t kRegConfig = 0x04;
  static constexpr uint8_t kRegVolume = 0x05;
  static constexpr uint8_t kRegStatus = 0x0A;
  static constexpr uint8_t kRegRssi = 0x0B;
  static constexpr uint8_t kRegRdsa = 0x0C;

  static constexpr uint16_t kBitDhiz = 0x8000;
  static constexpr uint16_t kBitDmute = 0x4000;
  static constexpr uint16_t kBitSeekup = 0x0200;
  static constexpr uint16_t kBitSeek = 0x0100;
  static constexpr uint16_t kBitRdsEn = 0x0008;
  static constexpr uint16_t kBitNewMethod = 0x0004;
  static constexpr uint16_t kBitEnable = 0x0001;
  static constexpr uint16_t kBitTune = 0x0010;

  /**
   * @brief Send the RDA5807M initialization sequence.
   * @param None.
   * @return true if required register writes succeeded.
   */
  bool initRadio();

  /**
   * @brief Write one 16-bit RDA5807M register.
   * @param reg Register address.
   * @param value 16-bit value to write.
   * @return true if I2C wrote all bytes.
   */
  bool writeRegister(uint8_t reg, uint16_t value);

  /**
   * @brief Read one 16-bit RDA5807M register.
   * @param reg Register address.
   * @param value Output parameter receiving the register value.
   * @return true if the I2C transaction succeeded.
   */
  bool readRegister(uint8_t reg, uint16_t& value);

  /**
   * @brief Read consecutive 16-bit RDA5807M registers.
   * @param startReg First register address.
   * @param count Number of registers to read.
   * @param values Output buffer with at least @p count elements.
   * @return true if the I2C transaction succeeded.
   */
  bool readRegisters(uint8_t startReg, uint8_t count, uint16_t* values);

  /**
   * @brief Wait for the seek/tune-complete bit.
   * @param timeoutMs Maximum wait time in milliseconds.
   * @return true if STC was observed before the timeout.
   */
  bool waitSeekTuneComplete(uint32_t timeoutMs);

  /**
   * @brief Convert an FM frequency to an RDA5807M channel number.
   * @param frequencyMhz Frequency in MHz.
   * @return Channel index relative to 87.0 MHz at 100 kHz spacing.
   */
  uint16_t channelForFrequency(float frequencyMhz) const;

  /**
   * @brief Convert an RDA5807M channel number to an FM frequency.
   * @param channel Raw channel number from the chip.
   * @return Frequency in MHz rounded to one decimal place.
   */
  float frequencyForChannel(uint16_t channel) const;

  i2c_inst_t* i2c_;
  uint sdaPin_;
  uint sclPin_;
  uint32_t busFrequencyHz_;
  bool initialized_ = false;
  bool powered_ = false;
  bool muted_ = false;
  uint8_t volume_ = 0;
  float frequency_ = 0.0f;
  uint16_t regPower_ = 0;
  uint16_t regChannel_ = 0;
  uint16_t regVolume_ = 0;
  RdsParser rds_;
};

}  // namespace app
