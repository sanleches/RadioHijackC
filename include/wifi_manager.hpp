#pragma once

#include "dhcp_server.hpp"

#include <string>

namespace app {

class WifiManager {
 public:
  bool begin();
  std::string connectOrStartAccessPoint();
  const std::string& ipAddress() const { return ipAddress_; }
  bool accessPointMode() const { return accessPointMode_; }

 private:
  std::string currentStaIp() const;
  std::string startAccessPoint();

  bool initialized_ = false;
  bool accessPointMode_ = false;
  std::string ipAddress_ = "0.0.0.0";
  DhcpServer dhcpServer_;
};

}  // namespace app
