/*
 * =============================================================================
 * RadioHijackC - Logger Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Write formatted status/debug lines to Pico stdio, which is USB serial for
 *   this firmware.
 *
 * Design:
 *   Tiny wrapper around vprintf() so all modules use the same output path.
 */

/**
 * @file logger.cpp
 * @brief USB serial logging implementation.
 */

#include "logger.hpp"

#include <cstdarg>
#include <cstdio>

namespace app {

/**
 * @brief Print a formatted log line and append a newline.
 * @param format printf-compatible format string.
 * @param ... Values consumed by the format string.
 * @return Nothing.
 */
void Logger::info(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

}  // namespace app
