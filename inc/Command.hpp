#pragma once

#include <string>
#include <functional>
#include <vector>

enum ReturnCode {
    Quit = -1,
    Ok = 0,
    Error = 1
};

struct Command
{
    using Inputs = std::vector<std::string>;
    using Outputs = std::vector<std::string>;
    using CommandFunction = std::function<int(const Inputs &, Outputs &)>;

    Command() = default;
    Command(std::string name, CommandFunction func, std::string info, std::string arg_type, std::vector<std::string> arg_alert);

    std::string name;
    CommandFunction func;
    std::string arg_type;
    std::string info;
    std::vector<std::string> arg_alert;
};