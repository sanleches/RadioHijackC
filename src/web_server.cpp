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

struct ParsedTarget {
  std::string path;
  std::string query;
};

struct ClientState {
  ApiRouter* router = nullptr;
  std::string request;
  std::string response;
  size_t sent = 0;
  bool closing = false;
};

ParsedTarget splitTarget(const std::string& target) {
  const size_t question = target.find('?');
  if (question == std::string::npos) {
    return {target, ""};
  }
  return {target.substr(0, question), target.substr(question + 1)};
}

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

void onError(void* arg, err_t /*error*/) {
  delete static_cast<ClientState*>(arg);
}

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

WebServer::WebServer(ApiRouter& router, SerialConsole& serialConsole)
    : router_(router), serialConsole_(serialConsole) {}

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
