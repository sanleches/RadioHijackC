/*
 * =============================================================================
 * RadioHijackC - Wi-Fi Manager Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Initialize CYW43, connect to configured Wi-Fi as a station, report the DHCP
 *   address, or start the fallback self-hosted AP and DHCP server.
 *
 * Mode selection:
 *   Station mode is attempted first. AP mode starts only when credentials are
 *   missing or connection times out/fails.
 *
 * Reported address:
 *   Station DHCP IP or fallback AP gateway 192.168.4.1.
 */

/**
 * @file wifi_manager.cpp
 * @brief CYW43 Wi-Fi station/AP setup implementation.
 */

#include "wifi_manager.hpp"

#include "app_config.hpp"
#include "logger.hpp"

#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

namespace app {

/** @brief Initialize CYW43 Wi-Fi support. @param None. @return true if initialized. */
bool WifiManager::begin() {
  if (initialized_) {
    return true;
  }
  if (cyw43_arch_init() != 0) {
    Logger::info("CYW43 init failed");
    return false;
  }
  initialized_ = true;
  return true;
}

/**
 * @brief Try configured station Wi-Fi first, then fall back to access point mode.
 * @param None.
 * @return Dashboard IP address.
 */
std::string WifiManager::connectOrStartAccessPoint() {
  if (!begin()) {
    return ipAddress_;
  }

  accessPointMode_ = false;
  if (config::kWifiSsid[0] != '\0' && config::kWifiPassword[0] != '\0') {
    cyw43_arch_enable_sta_mode();
    Logger::info("Connecting to Wi-Fi: %s", config::kWifiSsid);
    const int result = cyw43_arch_wifi_connect_timeout_ms(
        config::kWifiSsid, config::kWifiPassword, CYW43_AUTH_WPA2_AES_PSK, 25000);
    if (result == 0) {
      ipAddress_ = currentStaIp();
      Logger::info("Connected. IP: %s", ipAddress_.c_str());
      return ipAddress_;
    }
    Logger::info("STA connection failed: %d", result);
  } else {
    Logger::info("No Wi-Fi credentials set");
  }

  return startAccessPoint();
}

/** @brief Read station-mode IP from lwIP. @param None. @return IPv4 address string. */
std::string WifiManager::currentStaIp() const {
  if (netif_default == nullptr) {
    return "0.0.0.0";
  }
  const char* ip = ip4addr_ntoa(netif_ip4_addr(netif_default));
  return ip ? std::string(ip) : std::string("0.0.0.0");
}

/** @brief Enable fallback AP and DHCP server. @param None. @return AP IP address. */
std::string WifiManager::startAccessPoint() {
  accessPointMode_ = true;
  cyw43_arch_enable_ap_mode(config::kApSsid, config::kApPassword, CYW43_AUTH_WPA2_AES_PSK);
  ipAddress_ = "192.168.4.1";
  cyw43_arch_lwip_begin();
  dhcpServer_.begin(192, 168, 4, 1);
  cyw43_arch_lwip_end();
  Logger::info("AP started: %s password: %s IP: %s", config::kApSsid, config::kApPassword, ipAddress_.c_str());
  return ipAddress_;
}

}  // namespace app
