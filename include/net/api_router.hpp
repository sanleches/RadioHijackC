/*
 * =============================================================================
 * RadioHijackC - HTTP API Router Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares the route dispatcher that translates browser/API requests into
 *   radio, scan, power, mute, and preset actions.
 *
 * Routes handled:
 *   /status, /tune, /seek, /vol, /volume, /step, /scan, /scanhtml,
 *   /mute, /power, /presets and matching /api/* aliases.
 *
 * Main class:
 *   ApiRouter - Owns no hardware directly; it coordinates Rda5807m and
 *   PresetStore and returns HttpResponse objects.
 */

/**
 * @file api_router.hpp
 * @brief HTTP API route dispatcher for radio, scan, power, mute, and presets.
 */

#pragma once

#include "net/http_response.hpp"
#include "radio/rda5807m.hpp"
#include "storage/preset_store.hpp"
#include "util/url.hpp"

#include <string>

namespace app {

/**
 * @brief Converts parsed HTTP paths and query parameters into radio actions.
 */
class ApiRouter {
 public:
  /**
   * @brief Construct an API router.
   * @param radio Radio driver pointer. nullptr makes radio routes return errors.
   * @param presets Preset store used by status and preset routes.
   * @param ipAddress Dashboard IP address reported in status JSON.
   * @return Constructed router.
   */
  ApiRouter(Rda5807m* radio, PresetStore& presets, std::string ipAddress);

  /**
   * @brief Handle one parsed HTTP API request.
   * @param path URL path without query string.
   * @param params Decoded query parameters.
   * @return HTTP response for the request.
   */
  HttpResponse handle(const std::string& path, const QueryParams& params);

  /**
   * @brief Let the radio parser consume pending RDS data during idle server time.
   * @param None.
   * @return Nothing.
   */
  void pollRadioRds();

 private:
  /**
   * @brief Build the standard JSON error used when the radio is unavailable.
   * @param None.
   * @return 500 JSON error response.
   */
  HttpResponse requireRadio() const;

  /** @brief Handle /status. @param None. @return JSON status response. */
  HttpResponse status();

  /** @brief Handle /tune. @param params Query parameters including f/freq. @return JSON tune result. */
  HttpResponse tune(const QueryParams& params);

  /** @brief Handle /seek. @param params Query parameters including dir. @return JSON seek result. */
  HttpResponse seek(const QueryParams& params);

  /** @brief Handle /vol and /volume. @param params Query parameters including v/vol. @return JSON volume result. */
  HttpResponse volume(const QueryParams& params);

  /** @brief Handle /step. @param params Query parameters including mhz. @return JSON step result. */
  HttpResponse step(const QueryParams& params);

  /** @brief Handle /scan. @param params Scan range, step, and RSSI threshold parameters. @return JSON scan result. */
  HttpResponse scan(const QueryParams& params);

  /** @brief Handle /scanhtml. @param params Scan range, step, and RSSI threshold parameters. @return HTML button fragment. */
  HttpResponse scanHtml(const QueryParams& params);

  /** @brief Handle /mute. @param params Query parameter on=1/0. @return JSON mute state. */
  HttpResponse mute(const QueryParams& params);

  /** @brief Handle /power. @param params Query parameter on=1/0. @return JSON power state. */
  HttpResponse power(const QueryParams& params);

  /** @brief Handle /presets. @param params action/name/freq parameters. @return JSON preset list. */
  HttpResponse presets(const QueryParams& params);

  Rda5807m* radio_;
  PresetStore& presets_;
  std::string ipAddress_;
};

}  // namespace app
