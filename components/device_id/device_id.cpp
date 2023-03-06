#include "device_id.h"

#include "sdkconfig.h"

const std::string gThisDeviceSSID{CONFIG_MGMT_AP_SSID + std::to_string(CONFIG_DEVICE_ID)};
const std::string gThisDeviceBTDeviceName{gThisDeviceSSID};
const std::string gThisDeviceApIP{"192.168.4." + std::to_string(gThisDeviceApIPRangeStart + CONFIG_DEVICE_ID)};