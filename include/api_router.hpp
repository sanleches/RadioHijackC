#pragma once

#include "http_response.hpp"
#include "preset_store.hpp"
#include "rda5807m.hpp"
#include "url.hpp"

#include <string>

namespace app {

class ApiRouter {
 public:
  ApiRouter(Rda5807m* radio, PresetStore& presets, std::string ipAddress);

  HttpResponse handle(const std::string& path, const QueryParams& params);
  void pollRadioRds();

 private:
  HttpResponse requireRadio() const;
  HttpResponse status();
  HttpResponse tune(const QueryParams& params);
  HttpResponse seek(const QueryParams& params);
  HttpResponse volume(const QueryParams& params);
  HttpResponse step(const QueryParams& params);
  HttpResponse scan(const QueryParams& params);
  HttpResponse scanHtml(const QueryParams& params);
  HttpResponse mute(const QueryParams& params);
  HttpResponse power(const QueryParams& params);
  HttpResponse presets(const QueryParams& params);

  Rda5807m* radio_;
  PresetStore& presets_;
  std::string ipAddress_;
};

}  // namespace app
