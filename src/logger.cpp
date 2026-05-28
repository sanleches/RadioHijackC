#include "logger.hpp"

#include <cstdarg>
#include <cstdio>

namespace app {

void Logger::info(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

}  // namespace app
