#ifndef TML_LOG_TEST_H
#define TML_LOG_TEST_H

// This function can be used instead of regular ESP-IDF macros for logging
// It can be useful ex. in case of debugging of templates, when code is placed in header
// and you can not include additional files and declare defines for logging
void test_log(const char *format, ...);

#endif  // TML_LOG_TEST_H
