#ifndef SERIAL_COMMAND_DISPATCHER_H_
#define SERIAL_COMMAND_DISPATCHER_H_

#include <functional>
#include <map>
#include <string>

class SerialCommandDispatcher {
 public:
  enum class CommandType : uint8_t { kReset, kStartLogs, kStopLogs, kLedOn, kLedOff };
  using CommandHandlerType = std::function<void()>;

  SerialCommandDispatcher();
  void setCommandHandler(CommandType command, CommandHandlerType handler);
  void onNewSymbols(std::string symbols);

 private:
  void process(const std::string& command);

  std::map<CommandType, CommandHandlerType> mHandlers;
  std::string mCurrentlyReceivedSymbols;
};

#endif  // SERIAL_COMMAND_DISPATCHER_H_
