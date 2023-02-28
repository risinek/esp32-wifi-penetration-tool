#include "bluetooth_serial.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "nvs_flash.h"

#if defined(PRINT_RX_SPEED) || defined(PRINT_TX_SPEED)
// #include "time.h"
#endif

#include <algorithm>
#include <string>

namespace {
const char* LOG_TAG = "BT";
const char* kSppServerName = "ESP32_SPP_SERVER";
const char* kDeviceBLEName = CONFIG_MGMT_AP_SSID;
esp_bt_pin_code_t kPpinCode = {1, 2, 3, 4};  // Max 16 byte
constexpr size_t kBtTxBufSize = ESP_SPP_MAX_MTU - 1;

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
        ESP_LOGE(LOG_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
      }
      break;
    }

    case ESP_BT_GAP_PIN_REQ_EVT: {
      // Called when user entered PIN (authentyification by PIN is enabled in case SSP is disabled)
      ESP_LOGD(LOG_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
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
      ESP_LOGD(LOG_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
      esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
      break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
      ESP_LOGD(LOG_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
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
}  // namespace

void esp_spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  auto& btSerial = BluetoothSerial::instance();
  switch (event) {
    case ESP_SPP_INIT_EVT: {
      // Called when device is started
      ESP_LOGD(LOG_TAG, "ESP_SPP_INIT_EVT");
      esp_bt_dev_set_device_name(kDeviceBLEName);
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
      // Called when terminal cvonnection closed
      ESP_LOGD(LOG_TAG, "ESP_SPP_CLOSE_EVT");
      {
        std::lock_guard<std::shared_timed_mutex> lock(btSerial.mMutex);
        btSerial.mWriteHandle = 0;
      }
      break;

    case ESP_SPP_START_EVT:
      // Called when device is started after ESP_SPP_INIT_EVT
      ESP_LOGD(LOG_TAG, "ESP_SPP_START_EVT");
      break;
    case ESP_SPP_CL_INIT_EVT:
      ESP_LOGD(LOG_TAG, "ESP_SPP_CL_INIT_EVT");
      break;

    case ESP_SPP_DATA_IND_EVT:
      ESP_LOGD(LOG_TAG, "ESP_SPP_DATA_IND_EVT len=%d", param->data_ind.len);
      // Called when data is received by ESP32
      BluetoothSerial::instance().onBtDataRecevied(std::string{(const char*)param->data_ind.data, param->data_ind.len});

#ifdef PRINT_RX_SPEED
      gettimeofday(&btSerial.mReceiveTimeNew, NULL);
      btSerial.mRXDataSizeTransSend += param->data_ind.len;
      if (btSerial.mReceiveTimeNew.tv_sec - btSerial.mReceiveTimeOld.tv_sec >= 3) {
        btSerial.printRXSpeed();
      }
#endif
      break;

    case ESP_SPP_SRV_OPEN_EVT:
      // Called when terminal connection opened
      ESP_LOGD(LOG_TAG, "ESP_SPP_SRV_OPEN_EVT");
      {
        std::lock_guard<std::shared_timed_mutex> lock(btSerial.mMutex);
        btSerial.mWriteHandle = param->srv_open.handle;
      }
      btSerial.transmitChunkOfData();
#ifdef PRINT_RX_SPEED
      gettimeofday(&btSerial.mReceiveTimeOld, NULL);
#endif
      break;

    case ESP_SPP_CONG_EVT:
      // Called when state of BT write buffer is changed. "Congested" means full.
      // If buffer is full, we should not write anything new. If it was full and not full anymore - we can write more
      ESP_LOGI(LOG_TAG, "ESP_SPP_CONG_EVT cong=%d", param->cong.cong);
      if (param->cong.cong == false) {
        // If BT write buffer is not full anymore, send more data
        btSerial.transmitChunkOfData();
      }
      // Do not set mIsTransmissionEvenSequenceInProgress here.
      // Event sequece is still in progress, it is just paused here
      break;

    case ESP_SPP_WRITE_EVT:
      // Called when BT write operation complete. Check if BT write buffer is full and if not - write more
      ESP_LOGI(LOG_TAG, "ESP_SPP_WRITE_EVT len=%d cong=%d, status=%d", param->write.len, param->write.cong,
               param->write.status);
      if (param->write.status == ESP_SPP_SUCCESS) {
        std::lock_guard<std::shared_timed_mutex> lock(btSerial.mMutex);
        btSerial.mCurrentTransmittedChunk = "";
      } else {
        ESP_LOGE(LOG_TAG, "Failed to send chunk of data via Bluetooth. Trying to send it again.");
      }
      if (param->write.cong == 0) {
        // If BT write buffer is not full, send more data
        btSerial.transmitChunkOfData();
      }

#ifdef PRINT_TX_SPEED
      gettimeofday(&mTransmitTimeNew, NULL);
      btSerial.mTXDataSizeTransSend += param->write.len;
      if (btSerial.mTransmitTimeNew.tv_sec - btSerial.mTransmitTimeOld.tv_sec >= 3) {
        printTXSpeed();
      }
#endif
      break;

    default:
      break;
  }
}

BluetoothSerial& BluetoothSerial::instance() {
  static BluetoothSerial instance;
  return instance;
}

bool BluetoothSerial::init(OnBtDataReceviedCallbackType dataReceviedCallback) {
  mDataReceviedCallback = std::move(dataReceviedCallback);

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

  const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
  if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
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

void BluetoothSerial::send(std::string message) {
  std::lock_guard<std::shared_timed_mutex> lock(mMutex);
  mLogs.emplace_back(std::move(message));

  ESP_LOGD(LOG_TAG, "BluetoothSerial::send() log_size = %d", mLogs.size());

  if (mWriteHandle == 0) {
    // We can not start sending logs, because no terminal is connected
    return;
  }

  if (mIsTransmissionEvenSequenceInProgress == true) {
    // Newly added logs will be transmitted later as part of transmission event sequence
    return;
  }

  extractDataChunkToTransmit();
  mIsTransmissionEvenSequenceInProgress = true;
  esp_spp_write(mWriteHandle, mCurrentTransmittedChunk.length(), (uint8_t*)mCurrentTransmittedChunk.c_str());
}

void BluetoothSerial::onBtDataRecevied(std::string receivedData) {
  if (nullptr != mDataReceviedCallback) {
    mDataReceviedCallback(std::move(receivedData));
  }
}

#ifdef PRINT_RX_SPEED
void BluetoothSerial::printRXSpeed() {
  float time_old_s = mReceiveTimeOld.tv_sec + mReceiveTimeOld.tv_usec / 1000000.0;
  float time_new_s = mReceiveTimeNew.tv_sec + mReceiveTimeNew.tv_usec / 1000000.0;
  float time_interval = time_new_s - time_old_s;
  float speed = mRXDataSizeTransSend * 8 / time_interval / 1000.0;
  ESP_LOGI(LOG_TAG, "speed(%fs ~ %fs): %f kbit/s", time_old_s, time_new_s, speed);
  mRXDataSizeTransSend = 0;
  mReceiveTimeOld.tv_sec = mReceiveTimeNew.tv_sec;
  mReceiveTimeOld.tv_usec = mReceiveTimeNew.tv_usec;
}
#endif
#ifdef PRINT_TX_SPEED
void BluetoothSerial::printTXSpeed() {
  float time_old_s = mTransmitTimeOld.tv_sec + mTransmitTimeOld.tv_usec / 1000000.0;
  float time_new_s = mTransmitTimeNew.tv_sec + mTransmitTimeNew.tv_usec / 1000000.0;
  float time_interval = time_new_s - time_old_s;
  float speed = mTXDataSizeTransSend * 8 / time_interval / 1000.0;
  ESP_LOGI(LOG_TAG, "speed(%fs ~ %fs): %f kbit/s", time_old_s, time_new_s, speed);
  mTXDataSizeTransSend = 0;
  mTransmitTimeOld.tv_sec = mTransmitTimeNew.tv_sec;
  mTransmitTimeOld.tv_usec = mTransmitTimeNew.tv_usec;
}
#endif

void BluetoothSerial::transmitChunkOfData() {
  ESP_LOGI(LOG_TAG, "BluetoothSerial::transmitChunkOfData()");

  std::lock_guard<std::shared_timed_mutex> lock(mMutex);
  if (mWriteHandle == 0) {
    return;
  }

  extractDataChunkToTransmit();
  if (mCurrentTransmittedChunk.empty()) {
    mIsTransmissionEvenSequenceInProgress = false;
    return;
  }
  mIsTransmissionEvenSequenceInProgress = true;
  esp_spp_write(mWriteHandle, mCurrentTransmittedChunk.length(), (uint8_t*)mCurrentTransmittedChunk.c_str());
}

void BluetoothSerial::extractDataChunkToTransmit() {
  if (!mCurrentTransmittedChunk.empty()) {
    return;
  }

  size_t resultLen{0};
  for (auto& curLine : mLogs) {
    if (curLine.length() <= (kBtTxBufSize - resultLen)) {
      // Current log line entirely fits buffer
      mCurrentTransmittedChunk += curLine;
      resultLen += curLine.length();
      curLine = "";
    } else {
      // Only part of current log line fits buffer
      auto symbolsToCopy = kBtTxBufSize - resultLen;
      mCurrentTransmittedChunk += curLine.substr(0, symbolsToCopy);
      auto remainStr = curLine.substr(symbolsToCopy);
      curLine = std::move(remainStr);
      break;
    }
  }

  // Remove extracted lines
  mLogs.erase(std::remove_if(mLogs.begin(), mLogs.end(), [&](const std::string& line) { return line.empty(); }),
              mLogs.end());
}
