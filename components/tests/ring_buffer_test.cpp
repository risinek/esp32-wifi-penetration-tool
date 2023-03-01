#include "ring_buffer_test.h"

#include <sstream>
#include <string>

#include "ring_buffer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace {
const char* LOG_TAG{"BT_TEST"};

// Probably it makes sense to move these commonly used functions and macros to separated header
std::string getFileName(std::string const& path) {
  constexpr char separator = '/';
  std::size_t separatorPosition = path.rfind(separator);
  if (separatorPosition != std::string::npos) {
    return path.substr(separatorPosition + 1U, path.size() - 1U);
  }
  return "";
}
}  // namespace

namespace std {
std::string to_string(const std::vector<char>& vec) {
  std::string res("[");
  for (char ch : vec) {
    res += '0' + ch;
    res += ", ";
  }
  if (!vec.empty()) {
    res.pop_back();
    res.pop_back();
  }
  res += "]";
  return res;
}
}  // namespace std

std::ostringstream& operator<<(std::ostringstream& os, const std::vector<char>& vec) {
  os << std::to_string(vec);
  return os;
}

#define MAKE_FORMAT_STR(val, expected)                                                                                \
  static_cast<std::ostringstream const&>(std::ostringstream()                                                         \
                                         << getFileName(__FILE__) << ":" << __LINE__ << " " << __FUNCTION__           \
                                         << "() failed: " << #val << " is " << std::to_string((val)) << ". Expected " \
                                         << std::to_string((expected)) << "\n\r")                                     \
      .str()
#define EXPECT_EQ(val, expected)                                                    \
  if ((val) != (expected)) {                                                        \
    esp_log_write(ESP_LOG_ERROR, "%s", MAKE_FORMAT_STR((val), (expected)).c_str()); \
    return false;                                                                   \
  }

bool test1() {
  // Overlapping after inserting big vector
  RingBuffer rb(5);
  std::vector<char> dataIn1(10, 1);
  rb.insert(dataIn1);
  auto dataOut = rb.extract(5);
  EXPECT_EQ(rb.empty(), true);
  std::vector<char> expectedData{1, 1, 1, 1, 1};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test2() {
  // Simple insert and extract
  RingBuffer rb(5);
  EXPECT_EQ(rb.size(), 0);
  EXPECT_EQ(rb.capacity(), 5);
  EXPECT_EQ(rb.getFreeElements(), 5);

  std::vector<char> dataIn2(2, 1);
  rb.insert(dataIn2);
  rb.extract(1);
  EXPECT_EQ(rb.size(), 1);
  EXPECT_EQ(rb.capacity(), 4);
  EXPECT_EQ(rb.getFreeElements(), 3);
  return true;
}

bool test3() {
  // Overlapping after adding one by one
  RingBuffer rb(5);
  for (char i = 0; i < 10; ++i) {
    rb.insert({i});
  }
  for (char i = 0; i < 5; ++i) {
    auto dataOut = rb.extract(1);
    EXPECT_EQ(dataOut[0], i + 5);
  }
  EXPECT_EQ(rb.size(), 0);
  EXPECT_EQ(rb.capacity(), 5);
  EXPECT_EQ(rb.getFreeElements(), 5);
  return true;
}

bool test4() {
  // [1, 2, 3, X, X] + 2 Elements -> [1, 2, 3, 4, 5]
  RingBuffer rb(5);
  rb.insert({1, 2, 3});
  rb.insert({4, 5});
  EXPECT_EQ(rb.size(), 5);
  EXPECT_EQ(rb.capacity(), 5);
  EXPECT_EQ(rb.getFreeElements(), 0);
  return true;
}

bool test5() {
  // [X, 2, 3, 4, X] + 2 Elements -> [6, X, 3, 4, 5]
  RingBuffer rb(5);
  rb.insert({1, 2, 3, 4});
  rb.extract(1);
  rb.insert({5, 6});
  EXPECT_EQ(rb.size(), 4);
  EXPECT_EQ(rb.capacity(), 4);
  EXPECT_EQ(rb.getFreeElements(), 0);

  auto dataOut = rb.extract(4);
  std::vector<char> expectedData{3, 4, 5, 6};
  EXPECT_EQ(dataOut, expectedData);
  EXPECT_EQ(rb.size(), 0);
  EXPECT_EQ(rb.capacity(), 5);
  EXPECT_EQ(rb.getFreeElements(), 5);
  return true;
}

bool test6() {
  // [1, 2, 3, 4, 5] + Extract more than RB has.
  RingBuffer rb(5);
  rb.insert({1, 2, 3, 4, 5});

  rb.extract(10);
  EXPECT_EQ(rb.size(), 0);
  EXPECT_EQ(rb.capacity(), 5);
  EXPECT_EQ(rb.getFreeElements(), 5);

  // Then try to extract more
  rb.extract(10);
  EXPECT_EQ(rb.size(), 0);
  EXPECT_EQ(rb.capacity(), 5);
  EXPECT_EQ(rb.getFreeElements(), 5);
  return true;
}

bool test7() {
  // [X, X, X, 4, 5, X, X] + 2 El -> [X, X, X, 4, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3, 4, 5});
  rb.extract(3);
  rb.insert({6, 7});
  EXPECT_EQ(rb.size(), 4);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 2);
  auto dataOut = rb.extract(4);
  std::vector<char> expectedData{4, 5, 6, 7};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test8() {
  // [X, X, X, 4, 5, X, X] + 3 El -> [8, X, X, 4, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3, 4, 5});
  rb.extract(3);
  rb.insert({6, 7, 8});
  EXPECT_EQ(rb.size(), 5);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 1);
  auto dataOut = rb.extract(5);
  std::vector<char> expectedData{4, 5, 6, 7, 8};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test9() {
  // [X, X, X, 4, 5, X, X] + 4 El -> [8, 9, X, 4, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3, 4, 5});
  rb.extract(3);
  rb.insert({6, 7, 8, 9});
  EXPECT_EQ(rb.size(), 6);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 0);
  auto dataOut = rb.extract(6);
  std::vector<char> expectedData{4, 5, 6, 7, 8, 9};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test10() {
  // [X, X, X, 4, 5, X, X] + 5 El -> [8, 9, 10, X, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3, 4, 5});
  rb.extract(3);
  rb.insert({6, 7, 8, 9, 10});
  EXPECT_EQ(rb.size(), 6);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 0);
  auto dataOut = rb.extract(17);
  std::vector<char> expectedData{5, 6, 7, 8, 9, 10};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test11() {
  // [X, 2, 3, X, X, X, X] + 4 El -> [X, 2, 3, 4, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3});
  rb.extract(1);
  rb.insert({4, 5, 6, 7});
  EXPECT_EQ(rb.size(), 6);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 0);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{2, 3, 4, 5, 6, 7};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test12() {
  // [X, 2, 3, X, X, X, X] + 5 El -> [8, X, 3, 4, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3});
  rb.extract(1);
  rb.insert({4, 5, 6, 7, 8});
  EXPECT_EQ(rb.size(), 6);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 0);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{3, 4, 5, 6, 7, 8};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test13() {
  // [X, 2, 3, 4, 5, 6, 7] + 1 El -> [8, X, 3, 4, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3, 4, 5, 6, 7});
  rb.extract(1);
  rb.insert({8});
  EXPECT_EQ(rb.size(), 6);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 0);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{3, 4, 5, 6, 7, 8};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test14() {
  // [X, X, X, X, X, 6, 7] + 2 El -> [8, 9, X, X, X, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3, 4, 5, 6, 7});
  rb.extract(5);
  rb.insert({8, 9});
  EXPECT_EQ(rb.size(), 4);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 2);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{6, 7, 8, 9};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test15() {
  // [X, X, X, X, X, 6, 7] + 5 El -> [8, 9, 10, 11, 12, X, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3, 4, 5, 6, 7});
  rb.extract(5);
  rb.insert({8, 9, 10, 11, 12});
  EXPECT_EQ(rb.size(), 6);
  EXPECT_EQ(rb.capacity(), 6);
  EXPECT_EQ(rb.getFreeElements(), 0);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{7, 8, 9, 10, 11, 12};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test16() {
  // [1, 2, X, X, X, X, X] + 5 El -> [1, 2, 3, 4, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2});
  rb.insert({3, 4, 5, 6, 7});
  EXPECT_EQ(rb.size(), 7);
  EXPECT_EQ(rb.capacity(), 7);
  EXPECT_EQ(rb.getFreeElements(), 0);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{1, 2, 3, 4, 5, 6, 7};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test17() {
  // [1, 2, X, X, X, X, X] + 6 El -> NOT!!! [8, X, 3, 4, 5, 6, 7]
  //                              ->        [3, 4, 5, 6, 7, 8, X] (because all
  //                              elements are deleted from  buffer)
  RingBuffer rb(7);
  rb.insert({1, 2});
  rb.insert({3, 4, 5, 6, 7, 8});
  EXPECT_EQ(rb.size(), 6);
  EXPECT_EQ(rb.capacity(), 7);
  EXPECT_EQ(rb.getFreeElements(), 1);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{3, 4, 5, 6, 7, 8};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test18() {
  // [X, X, X, X, X, X, X] + 7 El -> [1, 2, 3, 4, 5, 6, 7]
  RingBuffer rb(7);
  rb.insert({1, 2, 3, 4, 5, 6, 7});
  EXPECT_EQ(rb.size(), 7);
  EXPECT_EQ(rb.capacity(), 7);
  EXPECT_EQ(rb.getFreeElements(), 0);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{1, 2, 3, 4, 5, 6, 7};
  EXPECT_EQ(dataOut, expectedData);
  return true;
}

bool test19() {
  // [3, X, X, X, X, 1, 2] -2 El -> [3, X, X, X, X, X, X]
  RingBuffer rb(7);
  rb.insert({1, 1, 1, 1, 1, 1, 2});
  rb.extract(5);
  rb.insert({3});
  rb.extract(2);
  EXPECT_EQ(rb.size(), 1);
  EXPECT_EQ(rb.capacity(), 7);
  EXPECT_EQ(rb.getFreeElements(), 6);
  auto dataOut = rb.extract(100);
  std::vector<char> expectedData{3};
  EXPECT_EQ(dataOut, expectedData);
  rb.extract(1);
  EXPECT_EQ(rb.size(), 0);
  EXPECT_EQ(rb.capacity(), 7);
  EXPECT_EQ(rb.getFreeElements(), 7);
  return true;
}

void test_ring_buffer() {
  ESP_LOGD(LOG_TAG, "Testing Ring Buffer");

  bool res = test1() && test2() && test3() && test4() && test5() && test6() && test7() && test8() && test9() &&
             test10() && test11() && test12() && test13() && test14() && test15() && test16() && test17() && test18() &&
             test19();

  if (res) {
    ESP_LOGD(LOG_TAG, "All TESTS of Ring Buffer PASSED");
  } else {
    ESP_LOGD(LOG_TAG, "Some TESTS of Ring Buffer FAILED");
  }
}
