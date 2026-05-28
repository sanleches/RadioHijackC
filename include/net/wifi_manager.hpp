/*
 * =============================================================================
 * RadioHijackC - Wi-Fi Manager Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares Wi-Fi startup logic for station mode with self-hosted AP fallback.
 *
 * Modes:
 *   STA - Connects to configured home Wi-Fi and reports DHCP IP.
 *   AP  - Starts RadioHijack-AP at 192.168.4.1 and starts local DHCP.
 *
 * Main class:
 *   WifiManager - Owns CYW43 init, selected network mode, and IP reporting.
 */

/**
 * @file wifi_manager.hpp
 * @brief Wi-Fi station connection and fallback access point management.
 */

#pragma once

#include "net/dhcp_server.hpp"
#include "ui/status_led.hpp"

#include <string>

namespace app {

/**
 * @brief Owns CYW43 Wi-Fi initialization and network mode selection.
 */
class WifiManager {
 public:
  /**
   * @brief Initialize the CYW43 Wi-Fi driver once.
   * @param None.
   * @return true if CYW43 initialized or was already initialized.
   */
  bool begin();

  /**
   * @brief Connect to configured Wi-Fi, or start the self-hosted AP if STA fails.
   * @param statusLed LED controller used to show boot/connection progress and final mode.
   * @return IP address where the dashboard should be opened.
   */
  std::string connectOrStartAccessPoint(StatusLed& statusLed);

  /**
   * @brief Return the current dashboard IP address.
   * @param None.
   * @return Constant reference to the stored IPv4 address string.
   */
  const std::string& ipAddress() const { return ipAddress_; }

  /**
   * @brief Report whether the firmware is serving its fallback access point.
   * @param None.
   * @return true in AP mode, false in station mode.
   */
  bool accessPointMode() const { return accessPointMode_; }

 private:
  /**
   * @brief Read the current station-mode IP from lwIP.
   * @param None.
   * @return IPv4 address string, or "0.0.0.0" if unavailable.
   */
  std::string currentStaIp() const;

  /**
   * @brief Start the fallback AP and DHCP server.
   * @param None.
   * @return AP gateway/dashboard IP address.
   */
  std::string startAccessPoint();

  bool initialized_ = false;
  bool accessPointMode_ = false;
  std::string ipAddress_ = "0.0.0.0";
  DhcpServer dhcpServer_;
};

}  // namespace app
