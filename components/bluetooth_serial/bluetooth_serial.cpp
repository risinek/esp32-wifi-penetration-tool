#include "bluetooth_serial.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <map>

#include "device_id.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

namespace {
const char* LOG_TAG{"BT"};
const char* kSppServerName{"ESP32_SPP_SERVER"};
esp_bt_pin_code_t kPpinCode{1, 2, 3, 4};             // Max 16 byte
constexpr size_t kBtTxBufSize{ESP_SPP_MAX_MTU - 1};  // Size of BT buffer for each sending chunk

std::string pinToStr(const esp_bt_pin_code_t& pin) {
  std::string result;
  for (uint8_t digit : kPpinCode) {
    result += (char)('0' + digit);
  }
  return result;
}

void esp_bt_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) {
  switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
      // Called when authentification successfully completed
      if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
        ESP_LOGD(LOG_TAG, "authentication success: %s", param->auth_cmpl.device_name);
        esp_log_buffer_hex(LOG_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
      } else {
        ESP_LOGE(LOG_TAG, "authentication failed, status:%d", (int)param->auth_cmpl.stat);
      }
      break;
    }

    case ESP_BT_GAP_PIN_REQ_EVT: {
      // Called when user entered PIN (authentyification by PIN is enabled in case SSP is disabled)
      ESP_LOGD(LOG_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", (int)param->pin_req.min_16_digit);
      if (param->pin_req.min_16_digit) {
        ESP_LOGD(LOG_TAG, "Input pin code: 0000 0000 0000 0000");
        esp_bt_pin_code_t pin_code = {0};
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
      } else {
        auto pinStr = pinToStr(kPpinCode);
        ESP_LOGD(LOG_TAG, "Input pin code: '%s'", pinStr.c_str());
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, kPpinCode);
      }
      break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    // Executed only if no legacy style of Bluetooth devices pairing (using PIN) is enabled
    case ESP_BT_GAP_CFM_REQ_EVT:
      // Called when start simplified pairing of device
      ESP_LOGD(LOG_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", (int)param->cfm_req.num_val);
      esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
      break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
      ESP_LOGD(LOG_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", (int)param->key_notif.passkey);
      break;
    case ESP_BT_GAP_KEY_REQ_EVT:
      ESP_LOGD(LOG_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
      break;
#endif

    case ESP_BT_GAP_DISC_RES_EVT:
      ESP_LOGI(LOG_TAG, "ESP_BT_GAP_DISC_RES_EVT");
      break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
      ESP_LOGI(LOG_TAG, "ESP_BT_GAP_DISC_STATE_CHANGED_EVT");
      break;
    case ESP_BT_GAP_RMT_SRVCS_EVT:
      ESP_LOGI(LOG_TAG, "ESP_BT_GAP_RMT_SRVCS_EVT");
      break;
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
      ESP_LOGI(LOG_TAG, "ESP_BT_GAP_RMT_SRVC_REC_EVT");
      break;

    default:
      break;
  }
}

// Convert log levels specified in config file of this project to ESP log level
esp_log_level_t config_to_bt_log_level_t(uint8_t config_log_level) {
  switch (config_log_level) {
    case 0:
      return ESP_LOG_NONE;
    case 1:
      return ESP_LOG_ERROR;
    case 2:
      return ESP_LOG_WARN;
    case 3:
    case 4:
      return ESP_LOG_INFO;
    case 5:
      return ESP_LOG_DEBUG;
    case 6:
      return ESP_LOG_VERBOSE;
    default:
      return ESP_LOG_INFO;
  }
}
}  // namespace

void esp_spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  auto& btSerial = BluetoothSerial::instance();
  switch (event) {
    case ESP_SPP_INIT_EVT: {
      // Called when device is started
      ESP_LOGD(LOG_TAG, "ESP_SPP_INIT_EVT");
      esp_bt_dev_set_device_name(gThisDeviceBTDeviceName.c_str());
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
      const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
      const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;
      esp_spp_start_srv(sec_mask, role_slave, 0, kSppServerName);
      break;
    }

    case ESP_SPP_DISCOVERY_COMP_EVT:
      ESP_LOGI(LOG_TAG, "ESP_SPP_DISCOVERY_COMP_EVT status=%d scn_num=%d", param->disc_comp.status,
               param->disc_comp.scn_num);
      break;
    case ESP_SPP_OPEN_EVT:
      ESP_LOGD(LOG_TAG, "ESP_SPP_OPEN_EVT");
      break;

    case ESP_SPP_CLOSE_EVT:
      btSerial.limitBTLogs(false);
      // Called when terminal cvonnection closed
      btSerial.limitedPrint(ESP_LOG_DEBUG, LOG_TAG, "ESP_SPP_CLOSE_EVT");
      {
        std::lock_guard<std::mutex> lock(btSerial.mMutex);
        btSerial.mTerminalConnectionHandle = 0;
        btSerial.mIsTransmissionRequestInProgress = false;
      }
      // Send fake command to SerialCommandDispatcher about terminal disconnection
      btSerial.onBtDataRecevied("btterminalconnected 0");
      break;

    case ESP_SPP_START_EVT:
      // Called when device is started after ESP_SPP_INIT_EVT
      ESP_LOGD(LOG_TAG, "ESP_SPP_START_EVT");
      break;
    case ESP_SPP_CL_INIT_EVT:
      ESP_LOGD(LOG_TAG, "ESP_SPP_CL_INIT_EVT");
      break;

    case ESP_SPP_DATA_IND_EVT:
      btSerial.limitedPrint(ESP_LOG_DEBUG, LOG_TAG, "ESP_SPP_DATA_IND_EVT len=%d\n\r", param->data_ind.len);
      // Called when data is received by ESP32
      btSerial.onBtDataRecevied(std::string{(const char*)param->data_ind.data, param->data_ind.len});
      break;

    case ESP_SPP_SRV_OPEN_EVT:
      // Called when terminal connection opened
      ESP_LOGD(LOG_TAG, "ESP_SPP_SRV_OPEN_EVT");
      {
        std::lock_guard<std::mutex> lock(btSerial.mMutex);
        btSerial.mTerminalConnectionHandle = param->srv_open.handle;
      }
      btSerial.limitBTLogs(true);
      // Send fake command to SerialCommandDispatcher, so that it will return list of supported commands
      btSerial.onBtDataRecevied("btterminalconnected 1");

      btSerial.transmitChunkOfData();
      break;

    case ESP_SPP_CONG_EVT:
      // Called when state of BT write buffer is changed. "Congested" means full.
      // If buffer is full, we should not write anything new. If it was full and not full anymore - we can write more
      ESP_LOGI(LOG_TAG, "ESP_SPP_CONG_EVT cong=%d", param->cong.cong);
      if (param->cong.cong == false) {
        // If BT write buffer is not full anymore, send more data
        btSerial.transmitChunkOfData();
      }
      // Do not set mIsTransmissionRequestInProgress here, because for each transmission request
      // we will have ESP_SPP_WRITE_EVT
      break;

    case ESP_SPP_WRITE_EVT:
      // Called when BT write operation complete. Check if BT write buffer is full and if not - write more
      btSerial.limitedPrint(ESP_LOG_INFO, LOG_TAG, "ESP_SPP_WRITE_EVT len=%d cong=%d, status=%d\n\r", param->write.len,
                            param->write.cong, param->write.status);

      {
        std::unique_lock<std::mutex> lock(btSerial.mMutex);
        btSerial.mIsTransmissionRequestInProgress = false;
        if (param->write.status == ESP_SPP_SUCCESS) {
          btSerial.mCurrentTransmittedChunk.clear();
        } else {
          lock.unlock();
          btSerial.limitedPrint(ESP_LOG_ERROR, LOG_TAG,
                                "Failed to send chunk of data via Bluetooth. Trying to send it again.\n\r");
        }
      }
      if (param->write.cong == 0) {
        // If BT write buffer is not full, send more data
        btSerial.transmitChunkOfData();
      }
      break;

    default:
      break;
  }
}

BluetoothSerial& BluetoothSerial::instance() {
  static BluetoothSerial instance;
  return instance;
}

bool BluetoothSerial::init(BtLogsForwarder* btLogsForwarder, OnBtDataReceviedCallbackType dataReceviedCallback,
                           uint32_t maxTxBufSize) {
  mBtLogsForwarder = btLogsForwarder;
  mDataReceviedCallback = std::move(dataReceviedCallback);
  mMaxTxBufSize = maxTxBufSize;
  mTxData.resize(mMaxTxBufSize);

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
    ESP_LOGE(LOG_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
    return false;
  }

  if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
    ESP_LOGE(LOG_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
    return false;
  }

  if ((ret = esp_bluedroid_init()) != ESP_OK) {
    ESP_LOGE(LOG_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
    return false;
  }

  if ((ret = esp_bluedroid_enable()) != ESP_OK) {
    ESP_LOGE(LOG_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
    return false;
  }

  if ((ret = esp_bt_gap_register_callback(esp_bt_gap_callback)) != ESP_OK) {
    ESP_LOGE(LOG_TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
    return false;
  }

  if ((ret = esp_spp_register_callback(::esp_spp_callback)) != ESP_OK) {
    ESP_LOGE(LOG_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
    return false;
  }

  esp_spp_cfg_t ssp_config{};
  ssp_config.mode = ESP_SPP_MODE_CB;
  if ((ret = esp_spp_enhanced_init(&ssp_config)) != ESP_OK) {
    ESP_LOGE(LOG_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
    return false;
  }

#if (CONFIG_BT_SSP_ENABLED == true)
  // Executed only if no legacy style of Bluetooth devices pairing (using PIN) is enabled
  // Set default parameters for Secure Simple Pairing
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

  /*
   * Set default parameters for Legacy Pairing
   * Use variable pin, input pin code when pairing
   */
  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
  esp_bt_pin_code_t pin_code;
  esp_bt_gap_set_pin(pin_type, 0, pin_code);

  return true;
}

bool BluetoothSerial::send(std::string message) {
  std::vector<char> data{message.begin(), message.end()};
  message.clear();
  return send(std::move(data));
}

bool BluetoothSerial::send(std::vector<char> message) {
  std::unique_lock<std::mutex> lock(mMutex);
  auto doesMessageFitBuffer = mTxData.insert(message.begin(), message.end());

  if (mTerminalConnectionHandle == 0) {
    // We can not start sending logs, because no terminal is connected
    return doesMessageFitBuffer;
  }

  if (mIsTransmissionRequestInProgress == true) {
    // Newly added logs will be transmitted later as part of transmission event sequence
    return doesMessageFitBuffer;
  }

  extractDataChunkToTransmit();
  mIsTransmissionRequestInProgress = true;
  lock.unlock();

  esp_spp_write(mTerminalConnectionHandle, mCurrentTransmittedChunk.size(), (uint8_t*)mCurrentTransmittedChunk.data());

  return doesMessageFitBuffer;
}

void BluetoothSerial::onBtDataRecevied(std::string receivedData) {
  if (nullptr != mDataReceviedCallback) {
    mDataReceviedCallback(std::move(receivedData));
  }
}

void BluetoothSerial::transmitChunkOfData() {
  std::unique_lock<std::mutex> lock(mMutex);
  if (mTerminalConnectionHandle == 0) {
    return;
  }
  if (mIsTransmissionRequestInProgress) {
    // Transmission request is in progress, but this function is called. It could happen ex if this function is called
    // twice (ex. in case BT terminal connection is just established, we print greetings message and then com to this
    // function to check if there are messages, which were nto sent yet).
    // OR we just received write confirmation event (mIsTransmissionRequestInProgress),
    // cleared mIsTransmissionRequestInProgress, unlocked mutex and immediately after that external thread called
    // BluetoothSerial::send(), which again initiated transaction.
    // Because request is already started, we have nothing to do - just return
    return;
  }

  extractDataChunkToTransmit();
  if (mCurrentTransmittedChunk.empty()) {
    return;
  }
  mIsTransmissionRequestInProgress = true;
  lock.unlock();

  esp_spp_write(mTerminalConnectionHandle, mCurrentTransmittedChunk.size(), (uint8_t*)mCurrentTransmittedChunk.data());
}

void BluetoothSerial::extractDataChunkToTransmit() {
  if (!mCurrentTransmittedChunk.empty()) {
    return;
  }
  mCurrentTransmittedChunk = mTxData.extract(kBtTxBufSize);
}

void BluetoothSerial::limitBTLogs(bool isLimited) {
  mIsLimitedLogs = isLimited;

  // Gather all tags from following files:
  //     esp-idf\components\bt\host\bluedroid\common\include\common\bt_trace.h
  //     esp-idf\components\bt\common\include\bt_common.h
  static const std::map<const char*, uint8_t> btLogTags = {
      {"BT_HCI", CONFIG_BT_LOG_HCI_TRACE_LEVEL},     {"BT_BTM", CONFIG_BT_LOG_BTM_TRACE_LEVEL},
      {"BT_L2CAP", CONFIG_BT_LOG_L2CAP_TRACE_LEVEL}, {"BT_RFCOMM", CONFIG_BT_LOG_RFCOMM_TRACE_LEVEL},
      {"BT_SDP", CONFIG_BT_LOG_SDP_TRACE_LEVEL},     {"BT_GAP", CONFIG_BT_LOG_GAP_TRACE_LEVEL},
      {"BT_BNEP", CONFIG_BT_LOG_BNEP_TRACE_LEVEL},   {"BT_PAN", CONFIG_BT_LOG_PAN_TRACE_LEVEL},
      {"BT_A2D", CONFIG_BT_LOG_A2D_TRACE_LEVEL},     {"BT_AVDT", CONFIG_BT_LOG_AVDT_TRACE_LEVEL},
      {"BT_AVCT", CONFIG_BT_LOG_AVCT_TRACE_LEVEL},   {"BT_AVRC", CONFIG_BT_LOG_AVRC_TRACE_LEVEL},
      {"BT_MCA", CONFIG_BT_LOG_MCA_TRACE_LEVEL},     {"BT_HIDH", CONFIG_BT_LOG_HID_TRACE_LEVEL},
      {"BT_APPL", CONFIG_BT_LOG_APPL_TRACE_LEVEL},   {"BT_GATT", CONFIG_BT_LOG_GATT_TRACE_LEVEL},
      {"BT_SMP", CONFIG_BT_LOG_SMP_TRACE_LEVEL},     {"BT_BTIF", CONFIG_BT_LOG_BTIF_TRACE_LEVEL},
      {"BT_BTC", CONFIG_BT_LOG_BTC_TRACE_LEVEL},     {"BT_OSI", CONFIG_BT_LOG_OSI_TRACE_LEVEL},
      {"BT_BLUFI", CONFIG_BT_LOG_BLUFI_TRACE_LEVEL}, {"BT_LOG", 4}};

  if (isLimited) {
    // Increase log level to avoid flood of logs
    for (const auto& tag_pair : btLogTags) {
      esp_log_level_set(tag_pair.first, ESP_LOG_WARN);
    }
  } else {
    for (const auto& tag_pair : btLogTags) {
      esp_log_level_set(tag_pair.first, config_to_bt_log_level_t(tag_pair.second));
    }
  }
}

void BluetoothSerial::limitedPrint(uint8_t logLevel, const char* tag, const char* format, ...) {
  va_list args;
  va_start(args, format);

  if (!mIsLimitedLogs) {
    esp_log_writev((esp_log_level_t)logLevel, tag, format, args);
    va_end(args);
    return;
  }

  if (!mBtLogsForwarder) {
    va_end(args);
    return;
  }

  mBtLogsForwarder->printToSerial(format, args);
  va_end(args);
}
