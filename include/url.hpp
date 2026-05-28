/**
 * @file url.hpp
 * @brief URL decoding and HTTP query-string parsing helpers.
 */

#pragma once

#include <map>
#include <string>

namespace app {

/** @brief Parsed HTTP query parameters keyed by decoded parameter name. */
using QueryParams = std::map<std::string, std::string>;

/**
 * @brief Decode a URL component.
 * @param value URL-encoded input, where '+' means space and '%XX' means byte.
 * @return Decoded string. Invalid percent escapes are copied unchanged.
 */
std::string urlDecode(const std::string& value);

/**
 * @brief Parse a URL query string into key/value pairs.
 * @param query Query text without the leading '?', for example "f=99.7&v=5".
 * @return Map of decoded keys to decoded values. Duplicate keys keep the last value.
 */
QueryParams parseQuery(const std::string& query);

}  // namespace app
