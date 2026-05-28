#include "http_response.hpp"

#include <cstdio>
#include <utility>

namespace app {

HttpResponse::HttpResponse(int status, std::string reason, std::string contentType, std::string body)
    : status_(status), reason_(std::move(reason)), contentType_(std::move(contentType)), body_(std::move(body)) {}

HttpResponse HttpResponse::html(std::string body) {
  return {200, "OK", "text/html; charset=utf-8", std::move(body)};
}

HttpResponse HttpResponse::json(std::string body, int status, std::string reason) {
  return {status, std::move(reason), "application/json", std::move(body)};
}

HttpResponse HttpResponse::text(std::string body, int status, std::string reason) {
  return {status, std::move(reason), "text/plain; charset=utf-8", std::move(body)};
}

HttpResponse HttpResponse::fragment(std::string body) {
  return {200, "OK", "text/html; charset=utf-8", std::move(body)};
}

HttpResponse HttpResponse::notFound() { return text("Not found", 404, "Not Found"); }

std::string HttpResponse::serialize(bool headOnly) const {
  char header[256];
  const int length = std::snprintf(header, sizeof(header),
                                   "HTTP/1.0 %d %s\r\n"
                                   "Content-Type: %s\r\n"
                                   "Content-Length: %u\r\n"
                                   "Access-Control-Allow-Origin: *\r\n"
                                   "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                                   "Pragma: no-cache\r\n"
                                   "Connection: close\r\n\r\n",
                                   status_, reason_.c_str(), contentType_.c_str(), static_cast<unsigned>(body_.size()));
  std::string response(header, length > 0 ? static_cast<size_t>(length) : 0);
  if (!headOnly) {
    response += body_;
  }
  return response;
}

}  // namespace app
