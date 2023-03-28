#include "wifi_controller.h"

#include <stdio.h>
#include <string.h>

#include <string>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "device_id.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "lwip/ip_addr.h"

static const char *LOG_TAG = "wifi_controller";
/**
 * @brief Stores current state of Wi-Fi interface
 */
static bool wifi_init = false;
static uint8_t original_mac_ap[6];

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data) {
  // if (event_id == WIFI_EVENT_AP_STACONNECTED) {
  //   wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
  //   ESP_LOGI(LOG_TAG, "LAMBIN station " MACSTR " join, AID=%d", MAC2STR(event->mac), (int)event->aid);
  // } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
  //   wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
  //   ESP_LOGI(LOG_TAG, "LAMBIN station " MACSTR " leave, AID=%d", MAC2STR(event->mac), (int)event->aid);
  // } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
  //   ESP_LOGI(LOG_TAG, "LAMBIN station event WIFI_EVENT_STA_CONNECTED");
  // } else {
  //   ESP_LOGI(LOG_TAG, "LAMBIN station unhandled event with event_base=%s, ID=%d", event_base, (int)event_id);
  // }
}

static void wifi_event_handler2(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,
                                void *event_data) {
  ESP_LOGI(LOG_TAG, "LAMBIN station event wifi_event_handler2 for event WIFI_EVENT_AP_STACONNECTED");
}
static void wifi_event_handler3(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,
                                void *event_data) {
  ESP_LOGI(LOG_TAG, "LAMBIN station event wifi_event_handler3 for event WIFI_EVENT_AP_STADISCONNECTED");
}

void set_esp_interface_ip(esp_interface_t interface, esp_netif_t *esp_netif, uint32_t local_ip, uint32_t gateway,
                          uint32_t subnet);

void set_esp_interface_ip(esp_interface_t interface, esp_netif_t *esp_netif, uint32_t local_ip, uint32_t gateway,
                          uint32_t subnet);

/**
 * @brief Initializes Wi-Fi interface into APSTA mode and starts it.
 *
 * @attention This function should be called only once.
 */
static void wifi_init_apsta() {
  ESP_ERROR_CHECK(esp_netif_init());

  esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_event_handler2, NULL));

  // save original AP MAC address
  ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, original_mac_ap));

  ESP_ERROR_CHECK(esp_wifi_config_80211_tx_rate(WIFI_IF_AP, WIFI_PHY_RATE_54M));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_country_code("CH", false));
  wifi_init = true;

  ESP_LOGD(LOG_TAG, "THIS_DEVICE_AP_IP = '%s'", gThisDeviceApIP.c_str());
  uint32_t localIP = ipaddr_addr(gThisDeviceApIP.c_str());
  uint32_t ip_255_255_255_0 = ipaddr_addr("255.255.255.0");  // 0xFFFFFF00;
  set_esp_interface_ip(ESP_IF_WIFI_AP, ap_netif, localIP, localIP, ip_255_255_255_0);
}

void wifictl_ap_start(wifi_config_t *wifi_config) {
  ESP_LOGD(LOG_TAG, "Starting AP with SSID '%s' ...", wifi_config->ap.ssid);
  if (!wifi_init) {
    wifi_init_apsta();
  }

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, wifi_config));

  // Looks like there are messages in logs from 2 layers - some default ("event task" or "driver") and user task
  // Below code looks like setup handlers for user task. So, if I caonnect ManagementAP and pass authentification,
  // then default task calls user handler and I can see also logs with "LAMBIN". But if I connect to "Pho Ha Noi 5G"
  // and fail authentification, probably default layer doesn't call user level at all
  //
  // Proove: https://demo-dijiudu.readthedocs.io/en/latest/api-guides/wifi.html#wi-fi-connect-phase
  // "In step 4.2, the Wi-Fi connection may fail because, for example, the password is wrong, the AP is not found, etc.
  // In a case like this, <SYSTEM_EVENT_STA_DISCONNECTED> will arise and the reason for such a failure will be provided"
  //
  // Fuck it! Ignore non-authorised attempts. Just make sure in night logs that someone really can authentificate and
  // callback for them is really called
  //
  // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_event_handler2, NULL));
  // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_event_handler3, NULL));
  // ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, WIFI_EVENT_AP_STACONNECTED, &system_event_handler, NULL));
}

void wifictl_ap_stop() {
  ESP_LOGD(LOG_TAG, "Stopping AP...");
  wifi_config_t wifi_config{};
  wifi_config.ap.max_connection = 0;

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_LOGD(LOG_TAG, "AP stopped");
}

void wifictl_mgmt_ap_start() {
  wifi_config_t mgmt_wifi_config{};
  // Copy SSID
  constexpr uint8_t max_size_of_ssid = sizeof(mgmt_wifi_config.ap.ssid) - 1;
  auto lenSsid = (gThisDeviceSSID.length() > max_size_of_ssid) ? max_size_of_ssid : gThisDeviceSSID.length();
  mempcpy(mgmt_wifi_config.ap.ssid, gThisDeviceSSID.c_str(), lenSsid);
  mgmt_wifi_config.ap.ssid[lenSsid] = 0;
  mgmt_wifi_config.ap.ssid_len = (uint8_t)lenSsid;

  // Copy password
  constexpr uint8_t max_size_of_pass = sizeof(mgmt_wifi_config.ap.password) - 1;
  auto lenPass =
      (strlen(CONFIG_MGMT_AP_PASSWORD) > max_size_of_pass) ? max_size_of_pass : strlen(CONFIG_MGMT_AP_PASSWORD);
  mempcpy(mgmt_wifi_config.ap.password, CONFIG_MGMT_AP_PASSWORD, lenPass);
  mgmt_wifi_config.ap.password[lenPass] = 0;

  mgmt_wifi_config.ap.max_connection = CONFIG_MGMT_AP_MAX_CONNECTIONS;
  mgmt_wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

  wifictl_ap_start(&mgmt_wifi_config);
}

void wifictl_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]) {
  ESP_LOGD(LOG_TAG, "Connecting STA to AP...");
  if (!wifi_init) {
    wifi_init_apsta();
  }

  wifi_config_t sta_wifi_config{};
  sta_wifi_config.sta.channel = ap_record->primary;
  sta_wifi_config.sta.scan_method = WIFI_FAST_SCAN;
  sta_wifi_config.sta.pmf_cfg.capable = false;
  sta_wifi_config.sta.pmf_cfg.required = false;
  mempcpy(sta_wifi_config.sta.ssid, ap_record->ssid, 32);

  if (password != NULL) {
    if (strlen(password) >= 64) {
      ESP_LOGE(LOG_TAG, "Password is too long. Max supported length is 64");
      return;
    }
    memcpy(sta_wifi_config.sta.password, password, strlen(password) + 1);
  }

  ESP_LOGD(LOG_TAG, ".ssid=%s", sta_wifi_config.sta.ssid);

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_wifi_config));
  ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifictl_sta_disconnect() { ESP_ERROR_CHECK(esp_wifi_disconnect()); }

void wifictl_set_ap_mac(const uint8_t *mac_ap) {
  ESP_LOGD(LOG_TAG, "Changing AP MAC address...");
  ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, mac_ap));
}

void wifictl_get_ap_mac(uint8_t *mac_ap) { esp_wifi_get_mac(WIFI_IF_AP, mac_ap); }

void wifictl_restore_ap_mac() {
  ESP_LOGD(LOG_TAG, "Restoring original AP MAC address...");
  ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, original_mac_ap));
}

void wifictl_get_sta_mac(uint8_t *mac_sta) { esp_wifi_get_mac(WIFI_IF_STA, mac_sta); }

void wifictl_set_channel(uint8_t channel) {
  if ((channel == 0) || (channel > 13)) {
    ESP_LOGE(LOG_TAG, "Channel out of range. Expected value from <1,13> but got %u", channel);
    return;
  }
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void configure_dhcp_server(esp_netif_t *esp_netif) {
  // This type definition is missing in ESP IDF v5.0.1 sources by some reason
  typedef struct {
    bool enable;
    ip4_addr_t start_ip;
    ip4_addr_t end_ip;
  } dhcps_lease_t;

  dhcps_lease_t dhcps_poll{};
  dhcps_poll.enable = true;
  uint32_t len = sizeof(dhcps_poll);
  IP4_ADDR(&dhcps_poll.start_ip, 192, 168, 4, 10);
  IP4_ADDR(&dhcps_poll.end_ip, 192, 168, 4, gThisDeviceApIPRangeStart - 1);
  ESP_ERROR_CHECK(
      esp_netif_dhcps_option(esp_netif, ESP_NETIF_OP_SET, ESP_NETIF_REQUESTED_IP_ADDRESS, &dhcps_poll, len));
}

void set_esp_interface_ip(esp_interface_t interface, esp_netif_t *esp_netif, uint32_t local_ip, uint32_t gateway,
                          uint32_t subnet) {
  esp_netif_ip_info_t info{};
  info.ip.addr = local_ip;
  info.gw.addr = gateway;
  info.netmask.addr = subnet;

  ESP_LOGD(LOG_TAG, "Configuring %s static IP: " IPSTR ", MASK: " IPSTR ", GW: " IPSTR,
           interface == ESP_IF_WIFI_STA  ? "STA"
           : interface == ESP_IF_WIFI_AP ? "AP"
                                         : "STA+AP",
           IP2STR(&info.ip), IP2STR(&info.netmask), IP2STR(&info.gw));

  if (interface != ESP_IF_WIFI_AP) {
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(esp_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif, &info));
    if (info.ip.addr == 0) {
      ESP_ERROR_CHECK(esp_netif_dhcpc_start(esp_netif));
    }
  } else {
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(esp_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif, &info));
    configure_dhcp_server(esp_netif);
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif));
  }
}

void wifictl_disable_powersafe() {
#if !CONFIG_BT_ENABLED
  /* Ensure to disable any WiFi power save mode, this allows best throughput
   * and hence timings for overall OTA operation.
   */
  esp_wifi_set_ps(WIFI_PS_NONE);
#else
  /* WIFI_PS_MIN_MODEM is the default mode for WiFi Power saving. When both
   * WiFi and Bluetooth are running, WiFI modem has to go down, hence we
   * need WIFI_PS_MIN_MODEM. And as WiFi modem goes down, OTA download time
   * increases.
   */
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
#endif  // CONFIG_BT_ENABLED
}
