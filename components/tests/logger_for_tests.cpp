#include "logger_for_tests.h"

#include <stdarg.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
const char* LOG_TAG{"LOG_TEST"};

void test_log(const char* format, ...) {
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, LOG_TAG, format, args);
  va_end(args);
}
