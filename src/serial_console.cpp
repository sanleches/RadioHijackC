#include "serial_console.hpp"

#include "app_config.hpp"
#include "logger.hpp"

#include "pico/stdlib.h"

namespace app {

SerialConsole::SerialConsole(Rda5807m* radio, const WifiManager& wifi)
    : radio_(radio), wifi_(wifi), nextReport_(make_timeout_time_ms(kPeriodicReportMs)) {}

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
