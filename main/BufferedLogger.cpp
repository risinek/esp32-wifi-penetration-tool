#include "BufferedLogger.h"

namespace Utils {
BufferedLogger& BufferedLogger::instance() {
  static BufferedLogger buffered_logger;
  return buffered_logger;
}

BufferedLogger::BufferedLogger(uint16_t buf_size) : mBufSize{buf_size} { mLog.reserve(buf_size); }

size_t BufferedLogger::write(uint8_t c) {
  mLog += c;
  return (shrink() ? 0 : 1);
}

size_t BufferedLogger::write(uint8_t const* buffer, size_t size) {
  mLog += String{(char const*)buffer};
  auto shrinked = shrink();
  return size - shrinked;
}

String& BufferedLogger::get_log() { return mLog; }

void BufferedLogger::clear() { mLog.clear(); }

size_t BufferedLogger::shrink() {
  if (mLog.length() > mBufSize) {
    size_t result = mLog.length() - mBufSize;
    mLog = mLog.substring(result);
    return result;
  }
  return 0;
}

}  // namespace Utils
