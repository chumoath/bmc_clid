#include "Command.hpp"
#include <utility>

Command::Command(std::string name, CommandFunction func, std::string info, std::string arg_type, std::vector<std::string> arg_alert)
    : name(std::move(name)), func(std::move(func)), arg_type(std::move(arg_type)), info(std::move(info)), arg_alert(std::move(arg_alert))
{
}