#include "api_router.hpp"

#include "app_config.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <utility>

namespace app {
namespace {

std::string jsonEscape(const std::string& value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (char c : value) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

std::string quote(const std::string& value) { return "\"" + jsonEscape(value) + "\""; }
std::string boolean(bool value) { return value ? "true" : "false"; }

std::string oneDecimal(float value) {
  char buffer[24];
  std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(value));
  return buffer;
}

std::string getParam(const QueryParams& params, const char* first, const char* second, const char* fallback) {
  auto it = params.find(first);
  if (it != params.end()) {
    return it->second;
  }
  if (second != nullptr) {
    it = params.find(second);
    if (it != params.end()) {
      return it->second;
    }
  }
  return fallback;
}

bool parseFloat(const std::string& text, float& value) {
  char* end = nullptr;
  value = std::strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0';
}

bool parseInt(const std::string& text, int& value) {
  char* end = nullptr;
  value = static_cast<int>(std::strtol(text.c_str(), &end, 10));
  return end != text.c_str() && *end == '\0';
}

std::string presetsJson(const PresetStore& store) {
  std::string json = "{";
  bool first = true;
  for (const Preset& preset : store.all()) {
    if (!first) {
      json += ",";
    }
    first = false;
    json += quote(preset.name) + ":" + oneDecimal(preset.frequency);
  }
  json += "}";
  return json;
}

std::string statusJson(const RadioStatus& status, const std::string& ip, const PresetStore& presets) {
  std::string json = "{";
  json += "\"powered\":" + boolean(status.powered);
  json += ",\"muted\":" + boolean(status.muted);
  json += ",\"frequency\":" + oneDecimal(status.frequency);
  json += ",\"freq\":" + oneDecimal(status.frequency);
  json += ",\"volume\":" + std::to_string(status.volume);
  json += ",\"vol\":" + std::to_string(status.volume);
  json += ",\"station\":" + quote(status.station);
  json += ",\"song\":" + quote(status.song);
  json += ",\"rssi\":" + std::to_string(status.rssi);
  json += ",\"stc\":" + boolean(status.stc);
  json += ",\"rds_ready\":" + boolean(status.rdsReady);
  json += ",\"stereo\":" + boolean(status.stereo);
  json += ",\"seek_fail\":" + boolean(status.seekFail);
  json += ",\"read_channel\":" + std::to_string(status.readChannel);
  json += ",\"debug_reg2\":" + std::to_string(status.debugReg2);
  json += ",\"debug_reg3\":" + std::to_string(status.debugReg3);
  json += ",\"debug_reg5\":" + std::to_string(status.debugReg5);
  json += ",\"debug_status_raw\":" + std::to_string(status.debugStatusRaw);
  json += ",\"debug_rssi_raw\":" + std::to_string(status.debugRssiRaw);
  json += ",\"result\":\"ok\"";
  json += ",\"ip\":" + quote(ip);
  json += ",\"presets\":" + presetsJson(presets);
  json += "}";
  return json;
}

}  // namespace

ApiRouter::ApiRouter(Rda5807m* radio, PresetStore& presets, std::string ipAddress)
    : radio_(radio), presets_(presets), ipAddress_(std::move(ipAddress)) {}

HttpResponse ApiRouter::handle(const std::string& path, const QueryParams& params) {
  if (path == "/status" || path == "/api/status") return status();
  if (path == "/tune" || path == "/api/tune") return tune(params);
  if (path == "/seek" || path == "/api/seek") return seek(params);
  if (path == "/vol" || path == "/volume" || path == "/api/volume") return volume(params);
  if (path == "/step" || path == "/api/step") return step(params);
  if (path == "/scan" || path == "/api/scan") return scan(params);
  if (path == "/scanhtml" || path == "/api/scanhtml") return scanHtml(params);
  if (path == "/mute" || path == "/api/mute") return mute(params);
  if (path == "/power" || path == "/api/power") return power(params);
  if (path == "/presets" || path == "/api/presets") return presets(params);
  return HttpResponse::notFound();
}

void ApiRouter::pollRadioRds() {
  if (radio_) {
    radio_->pollRds();
  }
}

HttpResponse ApiRouter::requireRadio() const {
  return HttpResponse::json("{\"result\":\"error\",\"message\":\"Radio not initialized\"}", 500,
                            "Internal Server Error");
}

HttpResponse ApiRouter::status() {
  if (!radio_) {
    std::string json = "{\"result\":\"error\",\"message\":\"Radio not initialized\",\"ip\":" + quote(ipAddress_) +
                       ",\"presets\":" + presetsJson(presets_) + "}";
    return HttpResponse::json(json, 500, "Internal Server Error");
  }
  return HttpResponse::json(statusJson(radio_->status(), ipAddress_, presets_));
}

HttpResponse ApiRouter::tune(const QueryParams& params) {
  if (!radio_) return requireRadio();
  float freq = config::kDefaultFrequencyMhz;
  if (!parseFloat(getParam(params, "f", "freq", "89.9"), freq)) {
    return HttpResponse::json("{\"result\":\"error\",\"message\":\"Invalid frequency\"}", 400, "Bad Request");
  }
  const float tuned = radio_->tune(freq);
  return HttpResponse::json("{\"result\":\"ok\",\"frequency\":" + oneDecimal(tuned) + ",\"freq\":" + oneDecimal(tuned) + "}");
}

HttpResponse ApiRouter::seek(const QueryParams& params) {
  if (!radio_) return requireRadio();
  const std::string direction = getParam(params, "dir", nullptr, "up");
  const float tuned = radio_->seek(direction != "down");
  return HttpResponse::json("{\"result\":\"ok\",\"frequency\":" + oneDecimal(tuned) + ",\"freq\":" + oneDecimal(tuned) + "}");
}

HttpResponse ApiRouter::volume(const QueryParams& params) {
  if (!radio_) return requireRadio();
  int vol = config::kDefaultVolume;
  if (!parseInt(getParam(params, "v", "vol", "5"), vol)) {
    return HttpResponse::json("{\"result\":\"error\",\"message\":\"Invalid volume\"}", 400, "Bad Request");
  }
  vol = radio_->setVolume(vol);
  return HttpResponse::json("{\"result\":\"ok\",\"volume\":" + std::to_string(vol) + ",\"vol\":" + std::to_string(vol) + "}");
}

HttpResponse ApiRouter::step(const QueryParams& params) {
  if (!radio_) return requireRadio();
  float mhz = 0.1f;
  if (!parseFloat(getParam(params, "mhz", nullptr, "0.1"), mhz)) {
    return HttpResponse::json("{\"result\":\"error\",\"message\":\"Invalid step\"}", 400, "Bad Request");
  }
  const float tuned = radio_->tune(radio_->currentFrequency() + mhz);
  return HttpResponse::json("{\"result\":\"ok\",\"frequency\":" + oneDecimal(tuned) + ",\"freq\":" + oneDecimal(tuned) + "}");
}

HttpResponse ApiRouter::scan(const QueryParams& params) {
  if (!radio_) return requireRadio();
  float start = 87.0f;
  float stop = 108.0f;
  float stepValue = 0.2f;
  int minRssi = 10;
  parseFloat(getParam(params, "start", nullptr, "87.0"), start);
  parseFloat(getParam(params, "end", "stop", "108.0"), stop);
  parseFloat(getParam(params, "step", nullptr, "0.2"), stepValue);
  parseInt(getParam(params, "minrssi", "min_rssi", "10"), minRssi);

  const auto stations = radio_->scan(start, stop, stepValue, static_cast<uint8_t>(std::max(0, minRssi)));
  std::string json = "{\"result\":\"ok\",\"stations\":[";
  for (size_t i = 0; i < stations.size(); ++i) {
    if (i != 0) json += ",";
    json += "{\"freq\":" + oneDecimal(stations[i].frequency) + ",\"rssi\":" + std::to_string(stations[i].rssi) + "}";
  }
  json += "]}";
  return HttpResponse::json(json);
}

HttpResponse ApiRouter::scanHtml(const QueryParams& params) {
  if (!radio_) return HttpResponse::fragment("<div class=\"small\">Radio not initialized</div>");
  float start = 87.0f;
  float stop = 108.0f;
  float stepValue = 0.2f;
  int minRssi = 10;
  parseFloat(getParam(params, "start", nullptr, "87.0"), start);
  parseFloat(getParam(params, "end", "stop", "108.0"), stop);
  parseFloat(getParam(params, "step", nullptr, "0.2"), stepValue);
  parseInt(getParam(params, "minrssi", "min_rssi", "10"), minRssi);

  const auto stations = radio_->scan(start, stop, stepValue, static_cast<uint8_t>(std::max(0, minRssi)));
  if (stations.empty()) {
    return HttpResponse::fragment("<div class=\"small\">No stations found. Try a better antenna or lower RSSI threshold.</div>");
  }

  std::string html;
  for (const auto& station : stations) {
    const std::string freq = oneDecimal(station.frequency);
    html += "<button type=\"button\" class=\"warn\" data-freq=\"" + freq + "\">" + freq +
            " MHz RSSI " + std::to_string(station.rssi) + "</button>";
  }
  return HttpResponse::fragment(html);
}

HttpResponse ApiRouter::mute(const QueryParams& params) {
  if (!radio_) return requireRadio();
  const bool on = getParam(params, "on", nullptr, "1") != "0";
  return HttpResponse::json("{\"result\":\"ok\",\"muted\":" + boolean(radio_->mute(on)) + "}");
}

HttpResponse ApiRouter::power(const QueryParams& params) {
  if (!radio_) return requireRadio();
  const bool on = getParam(params, "on", nullptr, "1") != "0";
  return HttpResponse::json("{\"result\":\"ok\",\"powered\":" + boolean(radio_->power(on)) + "}");
}

HttpResponse ApiRouter::presets(const QueryParams& params) {
  const std::string action = getParam(params, "action", nullptr, "list");
  if (action == "save") {
    std::string name = getParam(params, "name", nullptr, "Station");
    if (name.empty()) {
      name = "Station";
    }
    float freq = 0.0f;
    if (!parseFloat(getParam(params, "freq", "f", "0"), freq)) {
      return HttpResponse::json("{\"result\":\"error\",\"message\":\"Invalid frequency\"}", 400, "Bad Request");
    }
    freq = std::round(freq * 10.0f) / 10.0f;
    if (freq < 87.0f || freq > 108.0f) {
      return HttpResponse::json("{\"result\":\"error\",\"message\":\"Frequency must be 87.0 to 108.0 MHz\"}", 400,
                                "Bad Request");
    }
    presets_.set(name, freq);
    presets_.save();
  } else if (action == "delete") {
    if (presets_.remove(getParam(params, "name", nullptr, ""))) {
      presets_.save();
    }
  }
  return HttpResponse::json("{\"result\":\"ok\",\"presets\":" + presetsJson(presets_) + "}");
}

}  // namespace app
