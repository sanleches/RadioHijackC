/**
 * @file http_response.hpp
 * @brief Lightweight HTTP/1.0 response builder used by the raw TCP server.
 */

#pragma once

#include <string>

namespace app {

/**
 * @brief Complete HTTP response with status, content type, and body.
 */
class HttpResponse {
 public:
  /**
   * @brief Build a 200 OK HTML response.
   * @param body HTML document or fragment body.
   * @return Response object ready to serialize.
   */
  static HttpResponse html(std::string body);

  /**
   * @brief Build a JSON response.
   * @param body JSON body text.
   * @param status HTTP status code.
   * @param reason HTTP reason phrase.
   * @return Response object ready to serialize.
   */
  static HttpResponse json(std::string body, int status = 200, std::string reason = "OK");

  /**
   * @brief Build a plain text response.
   * @param body UTF-8 text body.
   * @param status HTTP status code.
   * @param reason HTTP reason phrase.
   * @return Response object ready to serialize.
   */
  static HttpResponse text(std::string body, int status = 200, std::string reason = "OK");

  /**
   * @brief Build a 200 OK HTML fragment response.
   * @param body HTML fragment body.
   * @return Response object ready to serialize.
   */
  static HttpResponse fragment(std::string body);

  /**
   * @brief Build a standard 404 Not Found response.
   * @param None.
   * @return Response object ready to serialize.
   */
  static HttpResponse notFound();

  /**
   * @brief Convert the response object to HTTP wire text.
   * @param headOnly If true, include headers only and omit the body.
   * @return Serialized HTTP/1.0 response.
   */
  std::string serialize(bool headOnly = false) const;

 private:
  /**
   * @brief Construct an HTTP response.
   * @param status HTTP status code.
   * @param reason HTTP reason phrase.
   * @param contentType MIME type returned in the Content-Type header.
   * @param body Response body bytes stored as a string.
   * @return Constructed response object.
   */
  HttpResponse(int status, std::string reason, std::string contentType, std::string body);

  int status_;
  std::string reason_;
  std::string contentType_;
  std::string body_;
};

}  // namespace app
