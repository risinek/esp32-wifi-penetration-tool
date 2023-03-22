#include "serial_command_dispatcher.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <algorithm>

#include "esp_log.h"

namespace {
const char* LOG_TAG = "SCH";
const std::map<std::string, SerialCommandDispatcher::CommandType> kCommandNames = {
    {"reset", SerialCommandDispatcher::CommandType::kReset},
    {"startlogs", SerialCommandDispatcher::CommandType::kStartLogs},
    {"stoplogs", SerialCommandDispatcher::CommandType::kStopLogs},
    {"limitlogs", SerialCommandDispatcher::CommandType::kLimitLogs},
    {"setloglevel", SerialCommandDispatcher::CommandType::kSetLogLevel},
    {"ledon", SerialCommandDispatcher::CommandType::kLedOn},
    {"ledoff", SerialCommandDispatcher::CommandType::kLedOff},
    {"ledblink", SerialCommandDispatcher::CommandType::kLedBlink},
    {"help", SerialCommandDispatcher::CommandType::kHelp},
    {"getattackstatus", SerialCommandDispatcher::CommandType::kGetAttackStatus},
    {"getdosattackstatus", SerialCommandDispatcher::CommandType::kGetDosAttackStatus},
    {"stopattack", SerialCommandDispatcher::CommandType::kStopAttack},
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

std::string SerialCommandDispatcher::getSupportedCommands() const {
  std::string result{"["};
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
  std::string commandName;
  std::string params;
  auto commandStartPos = command.find_first_not_of(' ');
  if (commandStartPos == std::string::npos) {
    return;
  }
  auto commandEndPos = command.find(' ', commandStartPos);
  if (commandEndPos == std::string::npos) {
    commandName = command.substr(commandStartPos);
  } else {
    commandName = command.substr(commandStartPos, commandEndPos - commandStartPos);
    // Trim spaces in param
    auto firstNonSpacePos = command.find_first_not_of(' ', commandEndPos);
    if (firstNonSpacePos != std::string::npos) {
      auto lastNonSpacePos = command.find_last_not_of(' ');
      auto paramLen =
          (lastNonSpacePos == std::string::npos) ? std::string::npos : (lastNonSpacePos - firstNonSpacePos + 1);
      params = command.substr(firstNonSpacePos, paramLen);
      std::transform(params.begin(), params.end(), params.begin(), [](unsigned char c) { return std::tolower(c); });
    }
  }
  std::transform(commandName.begin(), commandName.end(), commandName.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  auto itType = kCommandNames.find(commandName);
  if (itType == kCommandNames.end()) {
    ESP_LOGE(LOG_TAG, "Unsupported command '%s'", commandName.c_str());
    return;
  }
  if (params.empty()) {
    ESP_LOGI(LOG_TAG, "Received command '%s'", commandName.c_str());
  } else {
    ESP_LOGI(LOG_TAG, "Received command '%s' with params '%s'", commandName.c_str(), params.c_str());
  }

  auto itHandler = mHandlers.find(itType->second);
  if (itHandler == mHandlers.end()) {
    ESP_LOGE(LOG_TAG, "No handler set for command '%s'", commandName.c_str());
    return;
  }
  itHandler->second(params);
}
