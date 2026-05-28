#pragma once

#include "rda5807m.hpp"
#include "wifi_manager.hpp"

#include "pico/time.h"

namespace app {

// Non-blocking USB serial helper. It periodically repeats the dashboard URL and
// also prints it immediately when the user sends 's', 'S', or '?' over serial.
class SerialConsole {
 public:
  SerialConsole(Rda5807m* radio, const WifiManager& wifi);

  void poll();
  void printStatus();

 private:
  static constexpr uint32_t kPeriodicReportMs = 60000;

  Rda5807m* radio_;
  const WifiManager& wifi_;
  absolute_time_t nextReport_;
};

}  // namespace app
