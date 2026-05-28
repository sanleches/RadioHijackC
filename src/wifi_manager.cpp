/*
 * =============================================================================
 * RadioHijackC - Wi-Fi Manager Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Initialize CYW43, connect to configured Wi-Fi as a station while blinking
 *   the status LED, report the DHCP address, or start the fallback self-hosted
 *   AP and DHCP server.
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
#include "pico/stdlib.h"
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
 * @param statusLed LED pattern controller used during connection and final mode.
 * @return Dashboard IP address.
 */
std::string WifiManager::connectOrStartAccessPoint(StatusLed& statusLed) {
  if (!begin()) {
    return ipAddress_;
  }

  statusLed.setMode(StatusLed::Mode::Booting);
  accessPointMode_ = false;
  if (config::kWifiSsid[0] != '\0' && config::kWifiPassword[0] != '\0') {
    cyw43_arch_enable_sta_mode();
    Logger::info("Connecting to Wi-Fi: %s", config::kWifiSsid);
    const int startResult = cyw43_arch_wifi_connect_async(config::kWifiSsid, config::kWifiPassword, CYW43_AUTH_WPA2_AES_PSK);
    if (startResult == 0) {
      const absolute_time_t connectDeadline = make_timeout_time_ms(25000);
      int linkStatus = CYW43_LINK_DOWN;
      while (!time_reached(connectDeadline)) {
        statusLed.poll();
        linkStatus = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        if (linkStatus == CYW43_LINK_UP) {
          ipAddress_ = currentStaIp();
          statusLed.setMode(StatusLed::Mode::WifiConnected);
          Logger::info("Connected. IP: %s", ipAddress_.c_str());
          return ipAddress_;
        }
        if (linkStatus == CYW43_LINK_FAIL || linkStatus == CYW43_LINK_NONET || linkStatus == CYW43_LINK_BADAUTH) {
          break;
        }
        sleep_ms(25);
      }
      Logger::info("STA connection failed. link status: %d", linkStatus);
    } else {
      Logger::info("STA connection start failed: %d", startResult);
    }
  } else {
    Logger::info("No Wi-Fi credentials set");
  }

  statusLed.setMode(StatusLed::Mode::FallbackAccessPoint);
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
