#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <memory>
#include "Command.hpp"

using RegisteredCommands = std::map<std::string, std::shared_ptr<Command>>;

RegisteredCommands &getBmcCommands()
{
    static RegisteredCommands bmcCommands;
    return bmcCommands;
}

void bmcboxRegisterCommand(const std::string &name, const std::string &info, const std::string &arg_type, const std::vector<std::string> &arg_alert, const Command::CommandFunction &func)
{
    getBmcCommands()[name] = std::make_shared<Command>(name, func, info, arg_type, arg_alert);
}

int main(int argc, char *argv[])
{
    std::vector<std::string> Inputs;
    std::vector<std::string> Outputs;

    RegisteredCommands::iterator it;
    int i = 0;

    if ( std::string(argv[0]).find("bmcbox") != std::string::npos ) {
        i = 1;
    }

    for ( ; i < argc; i++ ) {
        Inputs.emplace_back(argv[i]);
    }

    if ( Inputs.empty() ) {
        std::cout << "Usage1: bmcbox [cmd] [arg]..." << std::endl;
        std::cout << "Usage2: bmcbox_[item] [cmd] [arg]..." << std::endl;
        std::cout << "Usage3: cmd[link] [arg]..." << std::endl;
        exit(1);
    }

    if ( (it = getBmcCommands().find(Inputs[0])) != std::end(getBmcCommands()) ) {
        if ( Inputs.size() >= 2 && (Inputs[1] == "--help" || Inputs[1] == "-h" || Inputs[1] == "help") ) {
            Outputs.emplace_back(it->second->info);
        } else {
            (it->second->func)(Inputs, Outputs);
        }
    }

    for ( auto &o: Outputs ) {
        std::cout << o << std::endl;
    }
}