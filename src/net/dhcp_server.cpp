/*
 * =============================================================================
 * RadioHijackC - Fallback AP DHCP Server Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Provide enough DHCP behavior for a phone/laptop to join RadioHijack-AP and
 *   receive an IP address for opening http://192.168.4.1.
 *
 * Supported DHCP messages:
 *   DISCOVER -> OFFER
 *   REQUEST  -> ACK
 *
 * Deliberate limits:
 *   Single stable lease only. No lease table, renewal tracking, or DNS server.
 */

/**
 * @file dhcp_server.cpp
 * @brief Minimal DHCP OFFER/ACK responder used by fallback AP mode.
 */

#include "net/dhcp_server.hpp"

#include "util/logger.hpp"

#include <array>
#include <cstring>

#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

namespace app {
namespace {

constexpr uint16_t kServerPort = 67;
constexpr uint16_t kClientPort = 68;
constexpr size_t kBootpHeaderSize = 236;
constexpr uint8_t kOptionMessageType = 53;
constexpr uint8_t kOptionSubnetMask = 1;
constexpr uint8_t kOptionRouter = 3;
constexpr uint8_t kOptionDns = 6;
constexpr uint8_t kOptionServerId = 54;
constexpr uint8_t kOptionLeaseTime = 51;
constexpr uint8_t kOptionEnd = 255;
constexpr uint8_t kDhcpDiscover = 1;
constexpr uint8_t kDhcpOffer = 2;
constexpr uint8_t kDhcpRequest = 3;
constexpr uint8_t kDhcpAck = 5;

/**
 * @brief Extract DHCP option 53 from a packet.
 * @param data Pointer to the copied BOOTP/DHCP packet bytes.
 * @param length Number of valid bytes in @p data.
 * @return DHCP message type, or 0 if not found/invalid.
 */
uint8_t readMessageType(const uint8_t* data, size_t length) {
  if (length < kBootpHeaderSize + 4) {
    return 0;
  }
  size_t index = kBootpHeaderSize + 4;
  while (index < length) {
    const uint8_t option = data[index++];
    if (option == kOptionEnd) {
      break;
    }
    if (option == 0) {
      continue;
    }
    if (index >= length) {
      break;
    }
    const uint8_t optionLength = data[index++];
    if (index + optionLength > length) {
      break;
    }
    if (option == kOptionMessageType && optionLength == 1) {
      return data[index];
    }
    index += optionLength;
  }
  return 0;
}

/**
 * @brief Append a generic DHCP option to an output packet.
 * @param out Output DHCP packet buffer.
 * @param index Current write offset, updated after the append.
 * @param option DHCP option number.
 * @param value Pointer to option bytes.
 * @param length Number of bytes in @p value.
 * @return Nothing.
 */
void appendOption(std::array<uint8_t, 312>& out, size_t& index, uint8_t option, const uint8_t* value, uint8_t length) {
  out[index++] = option;
  out[index++] = length;
  std::memcpy(&out[index], value, length);
  index += length;
}

/**
 * @brief Append a DHCP option containing one IPv4 address.
 * @param out Output DHCP packet buffer.
 * @param index Current write offset, updated after the append.
 * @param option DHCP option number.
 * @param ip Four-byte IPv4 address.
 * @return Nothing.
 */
void appendIpOption(std::array<uint8_t, 312>& out, size_t& index, uint8_t option, const uint8_t ip[4]) {
  appendOption(out, index, option, ip, 4);
}

}  // namespace

/**
 * @brief Start the UDP DHCP responder.
 * @param gatewayA Gateway IP octet 1.
 * @param gatewayB Gateway IP octet 2.
 * @param gatewayC Gateway IP octet 3.
 * @param gatewayD Gateway IP octet 4.
 * @return true if UDP binding succeeded.
 */
bool DhcpServer::begin(uint8_t gatewayA, uint8_t gatewayB, uint8_t gatewayC, uint8_t gatewayD) {
  stop();
  gateway_[0] = gatewayA;
  gateway_[1] = gatewayB;
  gateway_[2] = gatewayC;
  gateway_[3] = gatewayD;
  lease_[0] = gatewayA;
  lease_[1] = gatewayB;
  lease_[2] = gatewayC;
  lease_[3] = 2;

  pcb_ = udp_new_ip_type(IPADDR_TYPE_V4);
  if (pcb_ == nullptr) {
    Logger::info("DHCP UDP allocation failed");
    return false;
  }

  if (udp_bind(pcb_, IP_ANY_TYPE, kServerPort) != ERR_OK) {
    Logger::info("DHCP bind failed");
    udp_remove(pcb_);
    pcb_ = nullptr;
    return false;
  }

  udp_recv(pcb_, receive, this);
  Logger::info("DHCP server ready: %u.%u.%u.%u", lease_[0], lease_[1], lease_[2], lease_[3]);
  return true;
}

/** @brief Stop the UDP DHCP responder. @param None. @return Nothing. */
void DhcpServer::stop() {
  if (pcb_ != nullptr) {
    udp_remove(pcb_);
    pcb_ = nullptr;
  }
}

/**
 * @brief lwIP UDP receive trampoline.
 * @param arg DhcpServer instance pointer.
 * @param pcb UDP PCB that received the packet.
 * @param packet Incoming packet buffer.
 * @param address Source IP address, unused.
 * @param port Source UDP port, unused.
 * @return Nothing.
 */
void DhcpServer::receive(void* arg, udp_pcb* pcb, pbuf* packet, const ip_addr_t* /*address*/, uint16_t /*port*/) {
  auto* server = static_cast<DhcpServer*>(arg);
  if (server != nullptr && packet != nullptr) {
    server->handle(pcb, packet);
  }
  if (packet != nullptr) {
    pbuf_free(packet);
  }
}

/**
 * @brief Parse DHCP DISCOVER/REQUEST and send OFFER/ACK.
 * @param pcb UDP PCB used for replies.
 * @param packet Incoming packet buffer.
 * @return Nothing.
 */
void DhcpServer::handle(udp_pcb* pcb, pbuf* packet) {
  std::array<uint8_t, 312> request{};
  const size_t copied = pbuf_copy_partial(packet, request.data(), request.size(), 0);
  if (copied < kBootpHeaderSize + 4 || request[0] != 1 || request[2] != 6) {
    return;
  }

  const uint8_t messageType = readMessageType(request.data(), copied);
  if (messageType != kDhcpDiscover && messageType != kDhcpRequest) {
    return;
  }

  std::array<uint8_t, 312> response{};
  response[0] = 2;
  response[1] = request[1];
  response[2] = request[2];
  response[3] = request[3];
  std::memcpy(&response[4], &request[4], 4);
  std::memcpy(&response[16], lease_, 4);
  std::memcpy(&response[20], gateway_, 4);
  std::memcpy(&response[28], &request[28], 16);
  response[236] = 0x63;
  response[237] = 0x82;
  response[238] = 0x53;
  response[239] = 0x63;

  size_t index = 240;
  const uint8_t responseType = messageType == kDhcpDiscover ? kDhcpOffer : kDhcpAck;
  appendOption(response, index, kOptionMessageType, &responseType, 1);
  appendIpOption(response, index, kOptionServerId, gateway_);
  const uint8_t leaseSeconds[] = {0x00, 0x00, 0x0E, 0x10};
  appendOption(response, index, kOptionLeaseTime, leaseSeconds, sizeof(leaseSeconds));
  const uint8_t subnet[] = {255, 255, 255, 0};
  appendIpOption(response, index, kOptionSubnetMask, subnet);
  appendIpOption(response, index, kOptionRouter, gateway_);
  appendIpOption(response, index, kOptionDns, gateway_);
  response[index++] = kOptionEnd;

  ip_addr_t broadcast;
  IP4_ADDR(ip_2_ip4(&broadcast), 255, 255, 255, 255);
  pbuf* out = pbuf_alloc(PBUF_TRANSPORT, index, PBUF_RAM);
  if (out == nullptr) {
    return;
  }
  std::memcpy(out->payload, response.data(), index);
  udp_sendto(pcb, out, &broadcast, kClientPort);
  pbuf_free(out);
}

}  // namespace app
