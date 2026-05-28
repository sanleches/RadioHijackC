/*
 * =============================================================================
 * RadioHijackC - Logger Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Write formatted status/debug lines to Pico stdio when a USB serial host is
 *   actually connected.
 *
 * Design:
 *   Tiny wrapper around vprintf() so all modules use the same output path. If
 *   the device is powered from a USB charger, logging is skipped so firmware
 *   startup and Wi-Fi work continue normally.
 */

/**
 * @file logger.cpp
 * @brief USB serial logging implementation.
 */

#include "util/logger.hpp"

#include <cstdarg>
#include <cstdio>

#include "pico/stdio_usb.h"

namespace app {

/**
 * @brief Print a formatted log line and append a newline.
 * @param format printf-compatible format string.
 * @param ... Values consumed by the format string.
 * @return Nothing.
 */
void Logger::info(const char* format, ...) {
  if (!stdio_usb_connected()) {
    return;
  }

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

}  // namespace app
