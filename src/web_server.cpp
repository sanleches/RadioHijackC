/*
 * =============================================================================
 * RadioHijackC - Raw lwIP Web Server Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Accept TCP clients on port 80, parse minimal HTTP/1.x requests, serve the
 *   dashboard, dispatch API routes, and send responses using raw lwIP callbacks.
 *
 * Callback flow:
 *   onAccept() -> onReceive() -> buildResponse() -> sendMore() -> onSent().
 *
 * Main loop duties:
 *   Poll RDS data and serial console status while lwIP background mode handles
 *   network work.
 */

/**
 * @file web_server.cpp
 * @brief Raw lwIP TCP HTTP server implementation.
 */

#include "web_server.hpp"

#include "app_config.hpp"
#include "http_response.hpp"
#include "logger.hpp"
#include "url.hpp"
#include "web_page.hpp"

#include <algorithm>
#include <string>

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

namespace app {
namespace {

/** @brief URL target split into path and query text. */
struct ParsedTarget {
  std::string path;
  std::string query;
};

/** @brief Per-client state kept by lwIP callbacks until the connection closes. */
struct ClientState {
  ApiRouter* router = nullptr;
  std::string request;
  std::string response;
  size_t sent = 0;
  bool closing = false;
};

/**
 * @brief Split an HTTP request target into path and query string.
 * @param target Raw target from the request line, for example "/status?_=1".
 * @return Parsed path and query components.
 */
ParsedTarget splitTarget(const std::string& target) {
  const size_t question = target.find('?');
  if (question == std::string::npos) {
    return {target, ""};
  }
  return {target.substr(0, question), target.substr(question + 1)};
}

/**
 * @brief Parse a complete HTTP request and build the response bytes.
 * @param router API router used for non-index routes.
 * @param request Complete HTTP request headers.
 * @return Serialized HTTP response.
 */
std::string buildResponse(ApiRouter& router, const std::string& request) {
  const size_t lineEnd = request.find("\r\n");
  const std::string requestLine = request.substr(0, lineEnd);
  const size_t firstSpace = requestLine.find(' ');
  const size_t secondSpace = firstSpace == std::string::npos ? std::string::npos : requestLine.find(' ', firstSpace + 1);
  if (firstSpace == std::string::npos || secondSpace == std::string::npos) {
    return HttpResponse::text("Bad request", 400, "Bad Request").serialize();
  }

  const std::string method = requestLine.substr(0, firstSpace);
  const std::string target = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
  Logger::info("Request: %s %s", method.c_str(), target.c_str());

  HttpResponse response = HttpResponse::notFound();
  if (method != "GET" && method != "HEAD") {
    response = HttpResponse::text("Method not allowed", 405, "Method Not Allowed");
  } else {
    const ParsedTarget parsed = splitTarget(target);
    if (parsed.path == "/" || parsed.path == "/index.html") {
      response = HttpResponse::html(webPageHtml());
    } else {
      response = router.handle(parsed.path, parseQuery(parsed.query));
    }
  }
  return response.serialize(method == "HEAD");
}

/**
 * @brief Detach callbacks, close/abort the PCB, and free client state.
 * @param pcb TCP connection control block.
 * @param state Allocated per-client state.
 * @return ERR_OK for lwIP callback compatibility.
 */
err_t closeClient(tcp_pcb* pcb, ClientState* state) {
  tcp_arg(pcb, nullptr);
  tcp_recv(pcb, nullptr);
  tcp_sent(pcb, nullptr);
  tcp_poll(pcb, nullptr, 0);

  const err_t error = tcp_close(pcb);
  if (error != ERR_OK) {
    tcp_abort(pcb);
  }
  delete state;
  return ERR_OK;
}

/**
 * @brief Write as much pending response data as lwIP can currently accept.
 * @param pcb TCP connection control block.
 * @param state Per-client state containing response bytes and send offset.
 * @return lwIP error code, usually ERR_OK.
 */
err_t sendMore(tcp_pcb* pcb, ClientState* state) {
  while (state->sent < state->response.size()) {
    const u16_t available = tcp_sndbuf(pcb);
    if (available == 0) {
      return ERR_OK;
    }
    const size_t remaining = state->response.size() - state->sent;
    const u16_t chunk = static_cast<u16_t>(std::min<size_t>({remaining, available, 1460}));
    const err_t error = tcp_write(pcb, state->response.data() + state->sent, chunk, TCP_WRITE_FLAG_COPY);
    if (error == ERR_MEM) {
      return ERR_OK;
    }
    if (error != ERR_OK) {
      return closeClient(pcb, state);
    }
    state->sent += chunk;
  }

  tcp_output(pcb);
  if (state->sent >= state->response.size()) {
    state->closing = true;
    return closeClient(pcb, state);
  }
  return ERR_OK;
}

/**
 * @brief lwIP callback after queued TCP bytes are acknowledged.
 * @param arg ClientState pointer.
 * @param pcb TCP connection control block.
 * @param len Number of bytes acknowledged by lwIP, unused.
 * @return lwIP error code.
 */
err_t onSent(void* arg, tcp_pcb* pcb, u16_t /*len*/) {
  auto* state = static_cast<ClientState*>(arg);
  if (state == nullptr) {
    return ERR_OK;
  }
  if (state->closing) {
    return closeClient(pcb, state);
  }
  return sendMore(pcb, state);
}

/**
 * @brief lwIP periodic client callback used to close deferred connections.
 * @param arg ClientState pointer.
 * @param pcb TCP connection control block.
 * @return lwIP error code.
 */
err_t onPoll(void* arg, tcp_pcb* pcb) {
  auto* state = static_cast<ClientState*>(arg);
  if (state == nullptr) {
    return ERR_OK;
  }
  if (state->closing) {
    return closeClient(pcb, state);
  }
  return ERR_OK;
}

/**
 * @brief lwIP callback when a connection aborts or errors.
 * @param arg ClientState pointer owned by the failed connection.
 * @param error lwIP error code, unused.
 * @return Nothing.
 */
void onError(void* arg, err_t /*error*/) {
  delete static_cast<ClientState*>(arg);
}

/**
 * @brief lwIP callback for incoming TCP data.
 * @param arg ClientState pointer.
 * @param pcb TCP connection control block.
 * @param packet Received pbuf chain, or nullptr when the peer closed.
 * @param error lwIP receive status.
 * @return lwIP error code.
 */
err_t onReceive(void* arg, tcp_pcb* pcb, pbuf* packet, err_t error) {
  auto* state = static_cast<ClientState*>(arg);
  if (state == nullptr) {
    if (packet != nullptr) {
      pbuf_free(packet);
    }
    return ERR_OK;
  }

  if (packet == nullptr) {
    return closeClient(pcb, state);
  }
  if (error != ERR_OK) {
    pbuf_free(packet);
    return closeClient(pcb, state);
  }

  for (pbuf* part = packet; part != nullptr; part = part->next) {
    state->request.append(static_cast<const char*>(part->payload), part->len);
  }
  tcp_recved(pcb, packet->tot_len);
  pbuf_free(packet);

  if (state->request.size() > 2048) {
    state->response = HttpResponse::text("Request too large", 413, "Payload Too Large").serialize();
    return sendMore(pcb, state);
  }

  if (state->request.find("\r\n\r\n") != std::string::npos) {
    state->response = buildResponse(*state->router, state->request);
    return sendMore(pcb, state);
  }

  return ERR_OK;
}

/**
 * @brief lwIP callback for accepted TCP clients.
 * @param arg ApiRouter pointer assigned to the listening PCB.
 * @param newPcb Newly accepted client PCB.
 * @param error lwIP accept status.
 * @return lwIP error code.
 */
err_t onAccept(void* arg, tcp_pcb* newPcb, err_t error) {
  if (error != ERR_OK || newPcb == nullptr) {
    return ERR_VAL;
  }

  auto* router = static_cast<ApiRouter*>(arg);
  auto* state = new ClientState();
  state->router = router;
  state->request.reserve(2048);

  tcp_arg(newPcb, state);
  tcp_recv(newPcb, onReceive);
  tcp_sent(newPcb, onSent);
  tcp_err(newPcb, onError);
  tcp_poll(newPcb, onPoll, 4);
  return ERR_OK;
}

}  // namespace

/**
 * @brief Store router and serial console references.
 * @param router API router for HTTP requests.
 * @param serialConsole Console polled from the main server loop.
 * @return Constructed server object.
 */
WebServer::WebServer(ApiRouter& router, SerialConsole& serialConsole)
    : router_(router), serialConsole_(serialConsole) {}

/**
 * @brief Configure the raw TCP listener and run the firmware service loop.
 * @param None.
 * @return Nothing during normal operation; returns only on listener setup failure.
 */
void WebServer::serve() {
  cyw43_arch_lwip_begin();
  tcp_pcb* pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (pcb == nullptr) {
    cyw43_arch_lwip_end();
    Logger::info("TCP allocation failed");
    return;
  }

  err_t error = tcp_bind(pcb, IP_ANY_TYPE, config::kHttpPort);
  if (error != ERR_OK) {
    tcp_close(pcb);
    cyw43_arch_lwip_end();
    Logger::info("TCP bind failed: %d", error);
    return;
  }

  tcp_pcb* listener = tcp_listen_with_backlog(pcb, config::kHttpListenBacklog);
  if (listener == nullptr) {
    tcp_close(pcb);
    cyw43_arch_lwip_end();
    Logger::info("TCP listen failed");
    return;
  }

  tcp_arg(listener, &router_);
  tcp_accept(listener, onAccept);
  cyw43_arch_lwip_end();

  Logger::info("HTTP server listening on port %u", config::kHttpPort);

  while (true) {
    router_.pollRadioRds();
    serialConsole_.poll();
    sleep_ms(25);
  }
}

}  // namespace app
