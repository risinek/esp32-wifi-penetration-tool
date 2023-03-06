#ifndef DEVICE_ID_H
#define DEVICE_ID_H

#include <string>

extern const std::string gThisDeviceSSID;
extern const std::string gThisDeviceBTDeviceName;
extern const std::string gThisDeviceApIP;
// Must NOT be less than 11. DHCP server leasing addresses in range [10, gThisDeviceApIPRangeStart)
constexpr uint8_t gThisDeviceApIPRangeStart{100};

#endif  // DEVICE_ID_H
