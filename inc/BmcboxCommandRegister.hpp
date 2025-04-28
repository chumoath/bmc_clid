#pragma once

#include <string>
#include <vector>
#include "Command.hpp"

extern void bmcboxRegisterCommand(const std::string &name, const std::string &info, const std::string &arg_type, const std::vector<std::string> &arg_alert, const Command::CommandFunction &func);

static void register_command() __attribute__((constructor));

#define BEGIN_REGISTER_COMMAND()  void register_command() {

#define REGISTER_COMMAND(_name, _func, _info, _arg_type, ...) \
    do { \
        bmcboxRegisterCommand(_name, _info, _arg_type, ##__VA_ARGS__, _func); \
    } while (0)

#define END_REGISTER_COMMAND()   }