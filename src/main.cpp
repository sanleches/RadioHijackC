/*
 * =============================================================================
 * RadioHijackC - Firmware Entry Point
 * =============================================================================
 *
 * Startup order:
 *   1. Initialize USB serial stdio.
 *   2. Load flash-backed presets.
 *   3. Initialize RDA5807M radio on I2C.
 *   4. Connect to Wi-Fi or start the fallback AP.
 *   5. Print dashboard status over serial.
 *   6. Start the HTTP server forever.
 *
 * Owns:
 *   Top-level object lifetime and dependency wiring.
 */

/**
 * @file main.cpp
 * @brief Firmware entry point and top-level object wiring.
 *
 * Initializes USB serial, presets, radio hardware, Wi-Fi/AP mode, serial status
 * reporting, API routing, and the HTTP server. After setup, WebServer::serve()
 * owns the main loop.
 */

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

/**
 * @brief Start the RadioHijack firmware.
 * @param None.
 * @return 0 if the server ever exits, though normal operation runs forever.
 */
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
