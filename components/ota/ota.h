#ifndef OTA_H_
#define OTA_H_

#include <string>

#include "Task.h"

// This class enables all logs, printed by ESP32, to be also copied to Bluetooth
class Ota {
 public:
  Ota() = default;
  ~Ota();

  void connectToServer(const std::string& url = "");

 private:
  class OTAConnectionTask : public Task {
   public:
    OTAConnectionTask();
    void run(void* data) override;
  };

  OTAConnectionTask mOTAConnectionTask;
  std::string mUrl;
};

#endif  // OTA_H_
