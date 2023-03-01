#include "serial_command_dispatcher.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace {
const char* LOG_TAG = "SCH";
const std::map<std::string, SerialCommandDispatcher::CommandType> kCommandNames = {
    {"reset", SerialCommandDispatcher::CommandType::kReset},
    {"startlogs", SerialCommandDispatcher::CommandType::kStartLogs},
    {"stoplogs", SerialCommandDispatcher::CommandType::kStopLogs},
    {"ledon", SerialCommandDispatcher::CommandType::kLedOn},
    {"ledoff", SerialCommandDispatcher::CommandType::kLedOff},
    {"btterminalconnected", SerialCommandDispatcher::CommandType::kBtTerminalConnected},
};
}  // namespace

SerialCommandDispatcher::SerialCommandDispatcher() {}

void SerialCommandDispatcher::setCommandHandler(CommandType command, CommandHandlerType handler) {
  mHandlers.insert({command, std::move(handler)});
}

void SerialCommandDispatcher::onNewSymbols(std::string symbols) {
  for (char ch : symbols) {
    if (ch == '\r') {
      // Ignore '\r' symbols
      continue;
    }
    mCurrentlyReceivedSymbols += ch;
  }

  if (!mCurrentlyReceivedSymbols.empty()) {
    process(mCurrentlyReceivedSymbols);
    mCurrentlyReceivedSymbols.clear();
  }
}

std::string SerialCommandDispatcher::getSupportedCommands() {
  std::string result{
      "\n\r\n\r\n\r\n\r\n\rWelcome to ESP32 WiFi penetration tool\n\r"
      "Supported commands: ["};
  for (const auto& commandPair : kCommandNames) {
    if (commandPair.first == "btterminalconnected") {
      // Skip "fake" command
      continue;
    }
    result += commandPair.first + ", ";
  }
  result.pop_back();
  result.pop_back();
  result += "]\n\r";
  return result;
}

void SerialCommandDispatcher::process(const std::string& command) {
  auto itType = kCommandNames.find(command);
  if (itType == kCommandNames.end()) {
    ESP_LOGE(LOG_TAG, "Unsupported command '%s'", command.c_str());
    return;
  }

  ESP_LOGI(LOG_TAG, "Received command '%s'", command.c_str());
  auto itHandler = mHandlers.find(itType->second);
  if (itHandler == mHandlers.end()) {
    ESP_LOGE(LOG_TAG, "No handler set for command '%s'", command.c_str());
    return;
  }
  itHandler->second();
}
