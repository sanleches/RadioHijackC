/*
 * =============================================================================
 * RadioHijackC - Serial Status Console Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares non-blocking USB serial status reporting for users who open the
 *   serial port after boot and still need the dashboard address.
 *
 * Behavior:
 *   Prints status at startup, every 60 seconds, and immediately when 's', 'S',
 *   or '?' is received over USB serial.
 *
 * Main class:
 *   SerialConsole - Poll-driven serial command/status helper.
 */

/**
 * @file serial_console.hpp
 * @brief Non-blocking USB serial status console.
 */

#pragma once

#include "net/wifi_manager.hpp"
#include "radio/rda5807m.hpp"

#include "pico/time.h"

namespace app {

/**
 * @brief Periodically reports network/radio status and handles serial commands.
 */
class SerialConsole {
 public:
  /**
   * @brief Construct a serial console bound to current radio and Wi-Fi state.
   * @param radio Optional radio pointer. nullptr means radio initialization failed.
   * @param wifi Wi-Fi manager reference used to report network mode and IP.
   * @return Constructed console object.
   */
  SerialConsole(Rda5807m* radio, const WifiManager& wifi);

  /**
   * @brief Service serial input and periodic status printing without blocking.
   * @param None.
   * @return Nothing.
   */
  void poll();

  /**
   * @brief Print the current dashboard URL and radio status to USB serial.
   * @param None.
   * @return Nothing.
   */
  void printStatus();

 private:
  static constexpr uint32_t kPeriodicReportMs = 60000;

  Rda5807m* radio_;
  const WifiManager& wifi_;
  absolute_time_t nextReport_;
};

}  // namespace app
