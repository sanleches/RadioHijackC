#include "url.hpp"

#include <cstdlib>

namespace app {

std::string urlDecode(const std::string& value) {
  std::string result;
  result.reserve(value.size());
  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '+') {
      result.push_back(' ');
    } else if (value[i] == '%' && i + 2 < value.size()) {
      const std::string hex = value.substr(i + 1, 2);
      char* end = nullptr;
      const long decoded = std::strtol(hex.c_str(), &end, 16);
      if (end != hex.c_str() && *end == '\0') {
        result.push_back(static_cast<char>(decoded));
        i += 2;
      } else {
        result.push_back(value[i]);
      }
    } else {
      result.push_back(value[i]);
    }
  }
  return result;
}

QueryParams parseQuery(const std::string& query) {
  QueryParams params;
  size_t start = 0;
  while (start < query.size()) {
    const size_t end = query.find('&', start);
    const std::string pair = query.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!pair.empty()) {
      const size_t equals = pair.find('=');
      const std::string key = urlDecode(pair.substr(0, equals));
      const std::string value = equals == std::string::npos ? "" : urlDecode(pair.substr(equals + 1));
      params[key] = value;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return params;
}

}  // namespace app
