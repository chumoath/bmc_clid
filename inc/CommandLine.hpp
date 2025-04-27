#pragma once

#include "Command.hpp"
#include "Generator.hpp"
#include "PluginProvider.hpp"
#include <memory>
#include <map>
#include <cstring>

class CommandLine
{
public:
    CommandLine(const CommandLine &) = delete;
    CommandLine(CommandLine &&) = delete;
    CommandLine &operator=(const CommandLine &) = delete;
    CommandLine &operator=(CommandLine &&) = delete;

    int readLine();
    int readLine(const std::string &greeting);

    void registerCommand(const std::string &name, const std::string &arg_type, const std::string &info, const std::vector<std::string> &arg_alert, const Command::CommandFunction &func);
    void registerCommand(const Command &command);

    void registerParamGenerator(const Generator &generator);
    std::vector<std::string> getRegisteredCommands();

    std::string getArgType();
    std::string getAlert();
    std::string getStatus();
    size_t getDataLength();
    size_t getTotalDataLength();
    static CommandLine *getCurrentCommandLine();

private:
    CommandLine();
    ~CommandLine();

    static rl_completion_func_t completerHelper;
    static rl_command_func_t rl_mytab_completions;

    void processBuffer();
    int executeCommand(const std::string &command);

    using RegisteredCommands = std::map<std::string, std::shared_ptr<Command>>;
    using RegisteredGenerators = std::map<std::string, std::shared_ptr<Generator>>;

    RegisteredCommands allCommands;
    RegisteredGenerators ParamGenerators;

    std::forward_list<PluginProvider> commandProviders;
    std::forward_list<PluginProvider> generatorProviders;

    std::string current_word_type;
    size_t current_words_length;
    std::string current_command;
    std::string current_word;
    std::string current_status;
    size_t current_data_length;
    size_t current_total_data_length;
    static CommandLine *current_commandline;

    class GarbageCollector {
    public:
        ~GarbageCollector() {
            if (CommandLine::current_commandline != nullptr) {
                delete CommandLine::current_commandline;
                CommandLine::current_commandline = nullptr;

                // 成员会自己析构，不必再手动析构
                // current_commandline->commandProviders.clear();
                // current_commandline->generatorProviders.clear();
            }
        }
    };

    static GarbageCollector gc;
};