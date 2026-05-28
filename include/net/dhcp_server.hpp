/*
 * =============================================================================
 * RadioHijackC - Fallback AP DHCP Server Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares a tiny DHCP responder used only when the Pico starts its own Wi-Fi
 *   access point because normal Wi-Fi connection failed.
 *
 * Protocol behavior:
 *   Listens on UDP 67 and replies to DHCP DISCOVER/REQUEST with one stable
 *   client lease, normally 192.168.4.2, using 192.168.4.1 as gateway/DNS.
 *
 * Main class:
 *   DhcpServer - Single-purpose UDP DHCP helper for local dashboard access.
 */

/**
 * @file dhcp_server.hpp
 * @brief Minimal DHCP responder for the Pico W fallback access point.
 */

#pragma once

#include <cstdint>

#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

namespace app {

/**
 * @brief Single-lease DHCP server for AP fallback mode.
 *
 * This is intentionally small: it replies to DISCOVER and REQUEST packets with
 * one stable address, which is sufficient for a phone or laptop to open the
 * local dashboard at 192.168.4.1.
 */
class DhcpServer {
 public:
  /**
   * @brief Start listening for DHCP packets on UDP port 67.
   * @param gatewayA First octet of the gateway/AP IPv4 address.
   * @param gatewayB Second octet of the gateway/AP IPv4 address.
   * @param gatewayC Third octet of the gateway/AP IPv4 address.
   * @param gatewayD Fourth octet of the gateway/AP IPv4 address.
   * @return true if the UDP PCB was allocated and bound successfully.
   */
  bool begin(uint8_t gatewayA, uint8_t gatewayB, uint8_t gatewayC, uint8_t gatewayD);

  /**
   * @brief Stop the DHCP server and release the UDP PCB.
   * @param None.
   * @return Nothing.
   */
  void stop();

 private:
  /**
   * @brief lwIP UDP receive callback trampoline.
   * @param arg Pointer to the DhcpServer instance.
   * @param pcb UDP protocol control block receiving the packet.
   * @param packet Incoming DHCP packet buffer. Ownership is released in this callback.
   * @param address Source IP address reported by lwIP.
   * @param port Source UDP port reported by lwIP.
   * @return Nothing.
   */
  static void receive(void* arg, udp_pcb* pcb, pbuf* packet, const ip_addr_t* address, uint16_t port);

  /**
   * @brief Parse one DHCP request and send OFFER/ACK when appropriate.
   * @param pcb UDP protocol control block used to send the reply.
   * @param packet Incoming DHCP packet buffer.
   * @return Nothing.
   */
  void handle(udp_pcb* pcb, pbuf* packet);

  udp_pcb* pcb_ = nullptr;
  uint8_t gateway_[4] = {};
  uint8_t lease_[4] = {};
};

}  // namespace app
