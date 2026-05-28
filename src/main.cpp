#include "api_router.hpp"
#include "app_config.hpp"
#include "logger.hpp"
#include "preset_store.hpp"
#include "rda5807m.hpp"
#include "serial_console.hpp"
#include "web_server.hpp"
#include "wifi_manager.hpp"

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include <string>

int main() {
  stdio_init_all();
  sleep_ms(1500);

  app::Logger::info("Starting RadioHijack C++ firmware");

  app::PresetStore presets;
  presets.load();

  i2c_inst_t* i2c = app::config::kI2cId == 0 ? i2c0 : i2c1;
  app::Rda5807m radio(i2c, app::config::kSdaPin, app::config::kSclPin, app::config::kI2cFrequencyHz);
  const bool radioReady = radio.begin();
  if (radioReady) {
    app::Logger::info("Radio initialized successfully");
  } else {
    app::Logger::info("Warning: radio initialization failed. Check RDA5807M power, ground, SDA GP4, SCL GP5, and pull-ups.");
  }

  app::WifiManager wifi;
  const std::string ipAddress = wifi.connectOrStartAccessPoint();

  app::ApiRouter router(radioReady ? &radio : nullptr, presets, ipAddress);
  app::SerialConsole serialConsole(radioReady ? &radio : nullptr, wifi);
  serialConsole.printStatus();

  app::WebServer server(router, serialConsole);
  server.serve();

  return 0;
}
