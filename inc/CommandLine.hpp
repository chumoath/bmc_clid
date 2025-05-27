#pragma once

#include "Command.hpp"
#include "Generator.hpp"
#include "PluginProvider.hpp"
#include <memory>
#include <map>
#include <cstring>
#include <sdbusplus/bus.hpp>

class CommandLine
{
public:
    CommandLine();
    ~CommandLine() = default;
    CommandLine(const CommandLine &) = delete;
    CommandLine(CommandLine &&) = delete;
    CommandLine &operator=(const CommandLine &) = delete;
    CommandLine &operator=(CommandLine &&) = delete;

    int readLine();
    int readLine(const std::string &greeting);

    void registerCommand(const std::string &name, const std::string &info, const std::string &arg_type, const std::vector<std::string> &arg_alert, const Command::CommandFunction &func);
    void registerCommand(const Command &command);

    void registerParamGenerator(const Generator &generator);
    std::vector<std::string> getRegisteredCommands();

    std::string getArgType();
    std::string getAlert();
    std::string getStatus();
    size_t getDataLength();
    size_t getTotalDataLength();
    static std::shared_ptr<CommandLine> getCurrentCommandLine();
    std::shared_ptr<sdbusplus::bus_t> systemBus;

private:
    static rl_completion_func_t completerHelper;
    static rl_command_func_t rl_mytab_completions;

    void processBuffer();
    int executeCommand(const std::string &command);

    using RegisteredCommands = std::map<std::string, std::shared_ptr<Command>>;
    using RegisteredGenerators = std::map<std::string, std::shared_ptr<Generator>>;

    std::forward_list<PluginProvider> commandProviders;
    std::forward_list<PluginProvider> generatorProviders;

    // 先析构Command，再 dlclose 动态库；否则析构时会导致Segmentation Fault
    // current_commandline 使用智能指针，确保CommandLine对象被析构；否则 new出的对象没有手动delete，不会析构对象，也不会触发该问题
    RegisteredCommands allCommands;
    RegisteredGenerators ParamGenerators;

    std::string current_word_type;
    size_t current_words_length;
    std::string current_command;
    std::string current_word;
    std::string current_status;
    size_t current_data_length;
    size_t current_total_data_length;
    static std::shared_ptr<CommandLine> current_commandline;
};