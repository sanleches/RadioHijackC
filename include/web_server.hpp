#pragma once

#include "api_router.hpp"
#include "serial_console.hpp"

namespace app {

class WebServer {
 public:
  WebServer(ApiRouter& router, SerialConsole& serialConsole);
  void serve();

 private:
  ApiRouter& router_;
  SerialConsole& serialConsole_;
};

}  // namespace app
