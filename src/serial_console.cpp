/*
 * =============================================================================
 * RadioHijackC - Serial Status Console Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Keep the dashboard address discoverable even if the serial terminal opens
 *   after boot.
 *
 * Commands:
 *   s, S, ? - Print network mode, web URL, and radio state immediately.
 *
 * Periodic output:
 *   Reprints status every 60 seconds without blocking the web server.
 */

/**
 * @file serial_console.cpp
 * @brief Non-blocking serial status console implementation.
 */

#include "serial_console.hpp"

#include "app_config.hpp"
#include "logger.hpp"

#include "pico/stdlib.h"

namespace app {

/**
 * @brief Bind the console to radio and Wi-Fi state providers.
 * @param radio Optional radio pointer.
 * @param wifi Wi-Fi manager reference.
 * @return Constructed console object.
 */
SerialConsole::SerialConsole(Rda5807m* radio, const WifiManager& wifi)
    : radio_(radio), wifi_(wifi), nextReport_(make_timeout_time_ms(kPeriodicReportMs)) {}

/** @brief Handle pending serial input and timed reports. @param None. @return Nothing. */
void SerialConsole::poll() {
  int input = getchar_timeout_us(0);
  while (input != PICO_ERROR_TIMEOUT) {
    if (input == 's' || input == 'S' || input == '?') {
      printStatus();
    }
    input = getchar_timeout_us(0);
  }

  if (time_reached(nextReport_)) {
    printStatus();
    nextReport_ = make_timeout_time_ms(kPeriodicReportMs);
  }
}

/** @brief Print current webpage and radio status. @param None. @return Nothing. */
void SerialConsole::printStatus() {
  Logger::info("================ RadioHijack Status ================");
  if (wifi_.accessPointMode()) {
    Logger::info("Network: self-hosted access point");
    Logger::info("AP SSID: %s", config::kApSsid);
    Logger::info("AP password: %s", config::kApPassword);
  } else {
    Logger::info("Network: connected to Wi-Fi");
    Logger::info("Wi-Fi SSID: %s", config::kWifiSsid);
  }
  Logger::info("Web page: http://%s", wifi_.ipAddress().c_str());

  if (radio_ == nullptr || !radio_->initialized()) {
    Logger::info("Radio: not initialized");
  } else {
    const RadioStatus status = radio_->status();
    Logger::info("Radio: %s, %s", status.powered ? "powered" : "off", status.muted ? "muted" : "unmuted");
    Logger::info("Frequency: %.1f MHz, volume: %u, RSSI: %u", static_cast<double>(status.frequency), status.volume,
                 status.rssi);
    Logger::info("Station: %s", status.station.c_str());
    Logger::info("Text: %s", status.song.c_str());
  }

  Logger::info("Send 's' over serial to print this status again.");
  Logger::info("====================================================");
}

}  // namespace app
