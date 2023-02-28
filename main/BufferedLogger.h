#ifndef SRC_BUFFEREDLOGGER_H_
#define SRC_BUFFEREDLOGGER_H_

#include <Stream.h>
#include <WString.h>

class BufferedLogger {
 public:
  // Singleton
  static BufferedLogger& instance();
  BufferedLogger(BufferedLogger&) = delete;
  BufferedLogger(BufferedLogger&&) = delete;
  BufferedLogger& operator=(BufferedLogger&) = delete;
  BufferedLogger& operator=(BufferedLogger&&) = delete;

  size_t write(uint8_t c);
  size_t write(uint8_t const* buffer, size_t size);

  String& get_log();
  void clear();

 private:
  explicit BufferedLogger(uint16_t buf_size = 2 * 1024);

  // Reduce size of log string to make it fit in allocated buffer. Consider only mBufSize last bytes of mLog.
  // Return amount of bytes, removed from buffer
  size_t shrink();

  String mLog;
  uint16_t mBufSize;
};

#endif  // SRC_BUFFEREDLOGGER_H_
