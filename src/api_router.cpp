/*
 * =============================================================================
 * RadioHijackC - HTTP API Router Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Parse API parameters, call radio/preset operations, and serialize responses
 *   matching the original MicroPython route behavior.
 *
 * Key helpers:
 *   jsonEscape(), quote(), boolean(), oneDecimal(), getParam(), parseFloat(),
 *   parseInt(), presetsJson(), statusJson().
 *
 * Route methods:
 *   status(), tune(), seek(), volume(), step(), scan(), scanHtml(), mute(),
 *   power(), presets().
 */

/**
 * @file api_router.cpp
 * @brief HTTP API implementation for radio controls and preset management.
 */

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

/**
 * @brief Escape a string for safe JSON output.
 * @param value Raw string.
 * @return JSON-escaped string without surrounding quotes.
 */
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

/** @brief Quote and escape a JSON string. @param value Raw string. @return Quoted JSON string. */
std::string quote(const std::string& value) { return "\"" + jsonEscape(value) + "\""; }

/** @brief Convert a boolean to JSON text. @param value Boolean value. @return "true" or "false". */
std::string boolean(bool value) { return value ? "true" : "false"; }

/** @brief Format a float with one decimal place. @param value Number to format. @return Decimal text. */
std::string oneDecimal(float value) {
  char buffer[24];
  std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(value));
  return buffer;
}

/**
 * @brief Read a query parameter with optional alias and fallback.
 * @param params Decoded query parameter map.
 * @param first Primary parameter name.
 * @param second Optional alias parameter name, or nullptr.
 * @param fallback Value returned when neither key exists.
 * @return Selected parameter value.
 */
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

/** @brief Parse a complete string as float. @param text Input text. @param value Output float. @return true if valid. */
bool parseFloat(const std::string& text, float& value) {
  char* end = nullptr;
  value = std::strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0';
}

/** @brief Parse a complete string as integer. @param text Input text. @param value Output integer. @return true if valid. */
bool parseInt(const std::string& text, int& value) {
  char* end = nullptr;
  value = static_cast<int>(std::strtol(text.c_str(), &end, 10));
  return end != text.c_str() && *end == '\0';
}

/** @brief Serialize preset store as a JSON object. @param store Preset store. @return JSON object text. */
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

/**
 * @brief Serialize complete radio state as the /status JSON body.
 * @param status Radio status snapshot.
 * @param ip Dashboard IP address.
 * @param presets Preset store to include in the response.
 * @return JSON object text.
 */
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

/**
 * @brief Store dependencies needed by route handlers.
 * @param radio Optional radio driver pointer.
 * @param presets Preset store reference.
 * @param ipAddress Dashboard IP address string.
 * @return Constructed router.
 */
ApiRouter::ApiRouter(Rda5807m* radio, PresetStore& presets, std::string ipAddress)
    : radio_(radio), presets_(presets), ipAddress_(std::move(ipAddress)) {}

/** @brief Dispatch an API path to its handler. @param path URL path. @param params Query parameters. @return HTTP response. */
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

/** @brief Poll radio RDS while the server is idle. @param None. @return Nothing. */
void ApiRouter::pollRadioRds() {
  if (radio_) {
    radio_->pollRds();
  }
}

/** @brief Build radio-unavailable JSON error. @param None. @return HTTP error response. */
HttpResponse ApiRouter::requireRadio() const {
  return HttpResponse::json("{\"result\":\"error\",\"message\":\"Radio not initialized\"}", 500,
                            "Internal Server Error");
}

/** @brief Handle status requests. @param None. @return JSON status response. */
HttpResponse ApiRouter::status() {
  if (!radio_) {
    std::string json = "{\"result\":\"error\",\"message\":\"Radio not initialized\",\"ip\":" + quote(ipAddress_) +
                       ",\"presets\":" + presetsJson(presets_) + "}";
    return HttpResponse::json(json, 500, "Internal Server Error");
  }
  return HttpResponse::json(statusJson(radio_->status(), ipAddress_, presets_));
}

/** @brief Handle tune requests. @param params Query parameters with f/freq. @return JSON tune response. */
HttpResponse ApiRouter::tune(const QueryParams& params) {
  if (!radio_) return requireRadio();
  float freq = config::kDefaultFrequencyMhz;
  if (!parseFloat(getParam(params, "f", "freq", "89.9"), freq)) {
    return HttpResponse::json("{\"result\":\"error\",\"message\":\"Invalid frequency\"}", 400, "Bad Request");
  }
  const float tuned = radio_->tune(freq);
  return HttpResponse::json("{\"result\":\"ok\",\"frequency\":" + oneDecimal(tuned) + ",\"freq\":" + oneDecimal(tuned) + "}");
}

/** @brief Handle seek requests. @param params Query parameters with dir. @return JSON seek response. */
HttpResponse ApiRouter::seek(const QueryParams& params) {
  if (!radio_) return requireRadio();
  const std::string direction = getParam(params, "dir", nullptr, "up");
  const float tuned = radio_->seek(direction != "down");
  return HttpResponse::json("{\"result\":\"ok\",\"frequency\":" + oneDecimal(tuned) + ",\"freq\":" + oneDecimal(tuned) + "}");
}

/** @brief Handle volume requests. @param params Query parameters with v/vol. @return JSON volume response. */
HttpResponse ApiRouter::volume(const QueryParams& params) {
  if (!radio_) return requireRadio();
  int vol = config::kDefaultVolume;
  if (!parseInt(getParam(params, "v", "vol", "5"), vol)) {
    return HttpResponse::json("{\"result\":\"error\",\"message\":\"Invalid volume\"}", 400, "Bad Request");
  }
  vol = radio_->setVolume(vol);
  return HttpResponse::json("{\"result\":\"ok\",\"volume\":" + std::to_string(vol) + ",\"vol\":" + std::to_string(vol) + "}");
}

/** @brief Handle relative tuning requests. @param params Query parameters with mhz. @return JSON tune response. */
HttpResponse ApiRouter::step(const QueryParams& params) {
  if (!radio_) return requireRadio();
  float mhz = 0.1f;
  if (!parseFloat(getParam(params, "mhz", nullptr, "0.1"), mhz)) {
    return HttpResponse::json("{\"result\":\"error\",\"message\":\"Invalid step\"}", 400, "Bad Request");
  }
  const float tuned = radio_->tune(radio_->currentFrequency() + mhz);
  return HttpResponse::json("{\"result\":\"ok\",\"frequency\":" + oneDecimal(tuned) + ",\"freq\":" + oneDecimal(tuned) + "}");
}

/** @brief Handle scan requests. @param params Range/step/RSSI query parameters. @return JSON station list. */
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

/** @brief Handle scan HTML fragment requests. @param params Range/step/RSSI query parameters. @return HTML buttons. */
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

/** @brief Handle mute requests. @param params Query parameter on=1/0. @return JSON mute response. */
HttpResponse ApiRouter::mute(const QueryParams& params) {
  if (!radio_) return requireRadio();
  const bool on = getParam(params, "on", nullptr, "1") != "0";
  return HttpResponse::json("{\"result\":\"ok\",\"muted\":" + boolean(radio_->mute(on)) + "}");
}

/** @brief Handle power requests. @param params Query parameter on=1/0. @return JSON power response. */
HttpResponse ApiRouter::power(const QueryParams& params) {
  if (!radio_) return requireRadio();
  const bool on = getParam(params, "on", nullptr, "1") != "0";
  return HttpResponse::json("{\"result\":\"ok\",\"powered\":" + boolean(radio_->power(on)) + "}");
}

/** @brief Handle preset list/save/delete requests. @param params action/name/freq query parameters. @return JSON presets. */
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
