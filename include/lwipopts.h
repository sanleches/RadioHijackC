/**
 * @file lwipopts.h
 * @brief lwIP compile-time options for Pico W background Wi-Fi mode.
 *
 * The firmware uses lwIP raw TCP/UDP callbacks without an RTOS. Sockets and
 * netconn APIs are disabled because they require a sys_arch port, while raw TCP
 * is enabled for the HTTP server and UDP is enabled for the fallback AP DHCP
 * responder.
 */

#pragma once

// Pico W CYW43 background mode uses lwIP without an RTOS. Keep the raw TCP API
// enabled and disable socket/netconn layers that require a sys_arch port.
#define NO_SYS 1
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_RAW 1

#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_ICMP 1
#define LWIP_DHCP 1
#define LWIP_DNS 1
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_NETIF_STATUS_CALLBACK 1

#define MEM_ALIGNMENT 4
#define MEM_SIZE (64 * 1024)
#define MEMP_NUM_TCP_PCB 8
#define MEMP_NUM_TCP_SEG 32
#define PBUF_POOL_SIZE 24

#define TCP_MSS 1460
#define TCP_WND (8 * TCP_MSS)
#define TCP_SND_BUF (8 * TCP_MSS)
#define TCP_SND_QUEUELEN ((4 * TCP_SND_BUF + (TCP_MSS - 1)) / TCP_MSS)

#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_NETIF_TX_SINGLE_PBUF 1
