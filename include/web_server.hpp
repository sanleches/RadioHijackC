/*
 * =============================================================================
 * RadioHijackC - Web Server Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares the raw lwIP HTTP server that accepts browser connections and
 *   serves both the dashboard and JSON/HTML API routes.
 *
 * Main class:
 *   WebServer - Configures TCP port 80, receives HTTP requests, dispatches API
 *   routes, and polls background services in the main loop.
 */

/**
 * @file web_server.hpp
 * @brief Raw lwIP HTTP server for dashboard and API requests.
 */

#pragma once

#include "api_router.hpp"
#include "serial_console.hpp"

namespace app {

/**
 * @brief Accepts HTTP clients and dispatches requests to ApiRouter.
 */
class WebServer {
 public:
  /**
   * @brief Construct the HTTP server wrapper.
   * @param router API router that handles non-index routes.
   * @param serialConsole Serial console polled from the server loop.
   * @return Constructed server object.
   */
  WebServer(ApiRouter& router, SerialConsole& serialConsole);

  /**
   * @brief Start listening on the configured HTTP port and run forever.
   * @param None.
   * @return Nothing. This function only returns if setup fails.
   */
  void serve();

 private:
  ApiRouter& router_;
  SerialConsole& serialConsole_;
};

}  // namespace app
