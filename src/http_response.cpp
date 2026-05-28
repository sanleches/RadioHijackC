/*
 * =============================================================================
 * RadioHijackC - HTTP Response Builder Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Build small HTTP/1.0 responses with no dynamic web framework dependency.
 *
 * Response helpers:
 *   html(), json(), text(), fragment(), notFound().
 *
 * Serialization:
 *   Adds CORS, no-cache headers, content length, and connection close.
 */

/**
 * @file http_response.cpp
 * @brief HTTP response construction and serialization implementation.
 */

#include "http_response.hpp"

#include <cstdio>
#include <utility>

namespace app {

/**
 * @brief Store response metadata and body.
 * @param status HTTP status code.
 * @param reason HTTP reason phrase.
 * @param contentType MIME type for Content-Type.
 * @param body Response body.
 * @return Constructed response object.
 */
HttpResponse::HttpResponse(int status, std::string reason, std::string contentType, std::string body)
    : status_(status), reason_(std::move(reason)), contentType_(std::move(contentType)), body_(std::move(body)) {}

/** @brief Create a 200 OK HTML response. @param body HTML body. @return Response object. */
HttpResponse HttpResponse::html(std::string body) {
  return {200, "OK", "text/html; charset=utf-8", std::move(body)};
}

/** @brief Create a JSON response. @param body JSON body. @param status HTTP status. @param reason Reason phrase. @return Response object. */
HttpResponse HttpResponse::json(std::string body, int status, std::string reason) {
  return {status, std::move(reason), "application/json", std::move(body)};
}

/** @brief Create a text response. @param body Text body. @param status HTTP status. @param reason Reason phrase. @return Response object. */
HttpResponse HttpResponse::text(std::string body, int status, std::string reason) {
  return {status, std::move(reason), "text/plain; charset=utf-8", std::move(body)};
}

/** @brief Create a 200 OK HTML fragment response. @param body HTML fragment. @return Response object. */
HttpResponse HttpResponse::fragment(std::string body) {
  return {200, "OK", "text/html; charset=utf-8", std::move(body)};
}

/** @brief Create a standard not-found response. @param None. @return 404 response object. */
HttpResponse HttpResponse::notFound() { return text("Not found", 404, "Not Found"); }

/**
 * @brief Serialize the response to HTTP/1.0 bytes.
 * @param headOnly true to emit headers only for HEAD requests.
 * @return Wire-format HTTP response string.
 */
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
