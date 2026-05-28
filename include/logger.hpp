/**
 * @file logger.hpp
 * @brief Small formatted logging wrapper used by all firmware modules.
 *
 * The implementation writes to Pico stdio, which is configured for USB serial
 * in CMake. Keeping logging behind one class makes it easy to redirect output
 * later without changing the rest of the codebase.
 */

#pragma once

namespace app {

/**
 * @brief Shared firmware logger.
 */
class Logger {
 public:
  /**
   * @brief Print one formatted log line to USB serial.
   * @param format printf-style format string.
   * @param ... Values referenced by @p format.
   * @return Nothing.
   */
  static void info(const char* format, ...);
};

}  // namespace app
