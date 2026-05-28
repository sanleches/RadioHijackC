#pragma once

#include <cstdint>

#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

namespace app {

// Minimal DHCP responder for the Pico W fallback access point. It leases a
// single stable client address, which is enough for the local radio dashboard.
class DhcpServer {
 public:
  bool begin(uint8_t gatewayA, uint8_t gatewayB, uint8_t gatewayC, uint8_t gatewayD);
  void stop();

 private:
  static void receive(void* arg, udp_pcb* pcb, pbuf* packet, const ip_addr_t* address, uint16_t port);
  void handle(udp_pcb* pcb, pbuf* packet);

  udp_pcb* pcb_ = nullptr;
  uint8_t gateway_[4] = {};
  uint8_t lease_[4] = {};
};

}  // namespace app
