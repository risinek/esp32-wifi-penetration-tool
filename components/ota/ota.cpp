#include "ota.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_https_ota.h"
#include "esp_log.h"
// #include "esp_http_client.h"
#include "esp_ota_ops.h"
// #include "esp_system.h"
#include <cstring>

#include "wifi_controller.h"

namespace {
static const char *LOG_TAG = "ota";
const std::string gDefaultOtaUrl{"http://192.168.4.10:8070/blink.bin"};
}  // namespace

// These lines are needed in case HTTPS is used for OTA and we need to store server certificate
// Refer to "simple_ota_example"
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info) {
  if (new_app_info == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_app_desc_t running_app_info;
  if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
    ESP_LOGI(LOG_TAG, "Running firmware version: %s", running_app_info.version);
  }

#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
  if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
    ESP_LOGW(LOG_TAG, "Current running version is the same as a new. We will not continue the update.");
    return ESP_FAIL;
  }
#endif

  return ESP_OK;
}

// esp_err_t http_event_handler(esp_http_client_event_t *evt) {
//   switch (evt->event_id) {
//     case HTTP_EVENT_ERROR:
//       ESP_LOGE(LOG_TAG, "HTTP_EVENT_ERROR");
//       break;
//     case HTTP_EVENT_ON_CONNECTED:
//       ESP_LOGE(LOG_TAG, "HTTP_EVENT_ON_CONNECTED");
//       break;
//     case HTTP_EVENT_HEADER_SENT:
//       ESP_LOGE(LOG_TAG, "HTTP_EVENT_HEADER_SENT");
//       break;
//     case HTTP_EVENT_ON_HEADER:
//       ESP_LOGE(LOG_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
//       break;
//     case HTTP_EVENT_ON_DATA:
//       ESP_LOGE(LOG_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//       break;
//     case HTTP_EVENT_ON_FINISH:
//       ESP_LOGE(LOG_TAG, "HTTP_EVENT_ON_FINISH");
//       break;
//     case HTTP_EVENT_DISCONNECTED:
//       ESP_LOGE(LOG_TAG, "HTTP_EVENT_DISCONNECTED");
//       break;
//     case HTTP_EVENT_REDIRECT:
//       ESP_LOGE(LOG_TAG, "HTTP_EVENT_REDIRECT");
//       break;
//   }
//   return ESP_OK;
// }

// static void ota_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
//   if (event_base == ESP_HTTPS_OTA_EVENT) {
//     switch (event_id) {
//       case ESP_HTTPS_OTA_START:
//         ESP_LOGI(LOG_TAG, "OTA started");
//         break;
//       case ESP_HTTPS_OTA_CONNECTED:
//         ESP_LOGI(LOG_TAG, "Connected to server");
//         break;
//       case ESP_HTTPS_OTA_GET_IMG_DESC:
//         ESP_LOGI(LOG_TAG, "Reading Image Description");
//         break;
//       case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
//         ESP_LOGI(LOG_TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
//         break;
//       case ESP_HTTPS_OTA_DECRYPT_CB:
//         ESP_LOGI(LOG_TAG, "Callback to decrypt function");
//         break;
//       case ESP_HTTPS_OTA_WRITE_FLASH:
//         ESP_LOGD(LOG_TAG, "Writing to flash: %d written", *(int *)event_data);
//         break;
//       case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
//         ESP_LOGI(LOG_TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
//         break;
//       case ESP_HTTPS_OTA_FINISH:
//         ESP_LOGI(LOG_TAG, "OTA finish");
//         break;
//       case ESP_HTTPS_OTA_ABORT:
//         ESP_LOGI(LOG_TAG, "OTA abort");
//         break;
//     }
//   }
// }

Ota::OTAConnectionTask::OTAConnectionTask() : Task("OTAConnectionTask") {}

void Ota::OTAConnectionTask::run(void *data) {
  // ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &ota_event_handler, NULL));

  esp_http_client_config_t config{};
  config.cert_pem = NULL;
  // config.event_handler = http_event_handler;
  config.keep_alive_enable = true;

  std::string *url = (std::string *)data;
  const char *otaURL = ((url == nullptr) || (url->empty())) ? gDefaultOtaUrl.c_str() : url->c_str();
  ESP_LOGD(LOG_TAG, "Try to get OTA from URL '%s'", otaURL);

  config.url = otaURL;
  config.timeout_ms = 2000;
  config.skip_cert_common_name_check = true;
  esp_https_ota_config_t ota_config{};
  ota_config.http_config = &config;

  esp_https_ota_handle_t https_ota_handle = NULL;
  esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
  if ((err != ESP_OK) || (https_ota_handle == NULL)) {
    ESP_LOGE(LOG_TAG, "ESP HTTPS OTA Begin failed. err=%d", err);
    vTaskDelete(NULL);
    return;
  }

  esp_app_desc_t app_desc;
  err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
  if (err != ESP_OK) {
    ESP_LOGE(LOG_TAG, "esp_https_ota_read_img_desc failed. err=%d", err);
    esp_https_ota_abort(https_ota_handle);
    vTaskDelete(NULL);
    return;
  }
  err = validate_image_header(&app_desc);
  if (err != ESP_OK) {
    ESP_LOGE(LOG_TAG, "Image header verification failed. err=%d", err);
    esp_https_ota_abort(https_ota_handle);
    vTaskDelete(NULL);
    return;
  }

  // Disabling of powersafe more for WiFi is desirable to improve WiFi speed for OTA.
  wifictl_disable_powersafe();

  while (1) {
    err = esp_https_ota_perform(https_ota_handle);
    if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
      break;
    }
    // TODO(all): Server doesn't send information about binary size. OR at least ESP API doesn't have any way to readit
    // in ESP IDF v4.1. If we would have it, we could show progress in percents
    ESP_LOGI(LOG_TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
  }

  if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
    // the OTA image was not completely received and user can customise the response to this situation.
    ESP_LOGE(LOG_TAG, "Complete data was not received.");
    esp_https_ota_abort(https_ota_handle);
    vTaskDelete(NULL);
    return;
  }

  esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
  if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
    ESP_LOGI(LOG_TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
  } else {
    if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
      ESP_LOGE(LOG_TAG, "Image validation failed, image is corrupted");
    }
    ESP_LOGE(LOG_TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
    vTaskDelete(NULL);
  }
}

Ota::~Ota() { mOTAConnectionTask.stop(); }

void Ota::connectToServer(const std::string &url) {
  mUrl = url;
  mOTAConnectionTask.start(&mUrl);
}
