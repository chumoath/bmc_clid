#include "CommandLine.hpp"
#include "Common.hpp"
#include <iostream>
#include <functional>
#include <iterator>
#include <cstring>
#include <readline/readline.h>
#include <readline/history.h>

// 类的静态成员变量需要定义
std::shared_ptr<CommandLine> CommandLine::current_commandline = nullptr;
std::shared_ptr<sdbusplus::bus_t> CommandLine::systemBus;

std::shared_ptr<CommandLine> CommandLine::getCurrentCommandLine()
{
    if ( current_commandline == nullptr ) {
        current_commandline = std::make_shared<CommandLine>();
        CommandLine::systemBus = std::make_shared<sdbusplus::bus_t>(sdbusplus::bus::new_default());
        // 不能在构造函数中加载so，否则插件中还会访问 current_commandline；
        // 但此时构造函数还未返回，current_commandline仍为nullptr
        current_commandline->commandProviders = loadProviders(COMMAND_LIB_PATH);
        current_commandline->generatorProviders = loadProviders(GENERATOR_LIB_PATH);
    }
    return current_commandline;
}

// Here we set default commands, they do nothing since we quit with them
// Quitting behaviour is hardcoded in readLine()
CommandLine::CommandLine()
{
    rl_bind_key(TAB, CommandLine::rl_mytab_completions); // To disable the default TAB behavior
    rl_attempted_completion_function = &CommandLine::completerHelper;

    registerCommand("lscmd", "Get available command.", "", {}, [this](const Command::Inputs &inputs, Command::Outputs &outputs) {
        auto commands = getRegisteredCommands();
        for ( auto &command: commands )
            outputs.emplace_back("\t" + command);
        return ReturnCode::Ok;
    });
    registerCommand("quit", "", "", {}, [this](const Command::Inputs &inputs, Command::Outputs &outputs) {
       return ReturnCode::Quit;
    });
    registerCommand("exit", "", "", {}, [this](const Command::Inputs &inputs, Command::Outputs &outputs) {
        return ReturnCode::Quit;
    });
    registerCommand("bye", "", "", {}, [this](const Command::Inputs &inputs, Command::Outputs &outputs) {
        return ReturnCode::Quit;
    });
    registerCommand("help", "Get command help.", "m", { "CommandName" }, [this](const Command::Inputs &inputs, Command::Outputs &outputs) {
       if ( inputs.size() != 2 ) {
           return ReturnCode::Error;
       }

       RegisteredCommands::iterator it;
       if ( ( it = allCommands.find(inputs[1])) != std::end(allCommands) ) {
           outputs.emplace_back(it->second->info);
       }

       return ReturnCode::Ok;
    });
}

void CommandLine::processBuffer()
{
    std::vector<std::string> tokens;
    std::string rl_buf(rl_line_buffer);
    std::istringstream iss(rl_buf);
    std::string token;

    // 从输入流读取每个单词，直到流结束
    while ( iss >> token ) {
        tokens.push_back(token);
    }

    if ( rl_buf[rl_buf.length() - 1] == ' ' ) {
        tokens.emplace_back("");
    }

    current_words_length = tokens.size();

    if ( current_words_length != 0 ) {
        current_command = tokens.front();
        current_word = tokens.back();
    }

    if ( current_words_length <= 1 ) {
        current_word_type = "c";
    } else if ( current_words_length >= 2 ) {
        CommandLine::RegisteredCommands::iterator it;
        if ( (it = allCommands.find(current_command)) != std::end(allCommands) ) {
            if ( it->second->arg_type.back() != 'a' ) {
                if ( current_words_length < it->second->arg_type.length() + 2 ) {
                    current_word_type = std::string(1, it->second->arg_type[current_words_length - 2]);
                    return;
                }

                if ( current_words_length == it->second->arg_type.length() + 2 && current_word.empty() ) {
                    current_word_type = "e";
                    current_status = "ok: enter";
                } else {
                    current_word_type = "e";
                    current_status = "alert: parameter out of range";
                }
            } else {
                if ( current_words_length < it->second->arg_type.length() + 1 ) {
                    current_word_type = std::string(1, it->second->arg_type[current_words_length - 2]);
                    return;
                }

                auto length_idx = it->second->arg_type.find('l');
                 current_total_data_length = strToNum(tokens[length_idx + 1]);
                current_data_length = tokens.size() - it->second->arg_type.length();

                if ( current_words_length < it->second->arg_type.length() + current_total_data_length + 1 ) {
                    current_word_type = "a";
                } else if ( current_words_length == it->second->arg_type.length() + current_total_data_length + 1 && current_word.empty() ) {
                    current_word_type = "e";
                    current_status = "ok: enter";
                } else {
                    current_word_type = "e";
                    current_status = "alert: parameter out of range";
                }
            }
        } else {
            current_word_type = "e";
            current_status = "error: unknown command";
        }
    }
}

int CommandLine::rl_mytab_completions(int ignore, int invoking_key)
{
    int ret;
    std::vector<std::string> tokens;

    current_commandline->processBuffer();
    CommandLine::RegisteredGenerators::iterator it;
    if ( (it = current_commandline->ParamGenerators.find(current_commandline->current_word_type)) != std::end(current_commandline->ParamGenerators) ) {
        if ( it->second->isCompletion && !current_commandline->current_word.empty() )
            ret = rl_complete_internal('!');
        else
            ret = rl_possible_completions(ignore, invoking_key);
    }

    return ret;
}

int CommandLine::readLine()
{
    return readLine("TEST> ");
}

int CommandLine::readLine(const std::string &greeting)
{
    char *buffer = readline(greeting.c_str());
    if ( !buffer ) {
        std::cout << std::endl; // EOF doesn't put last endline so we put that so that it looks uniform.
        return ReturnCode::Quit;
    }

    if ( buffer[0] != '\0' ) {
        add_history(buffer);
    }

    std::string line(buffer);
    free(buffer);

    return executeCommand(line);
}

int CommandLine::executeCommand(const std::string &command)
{
    // Convert input to vector
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

    {
        std::istringstream iss(command);
        std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(),
                std::back_inserter(inputs));
    }

    if ( inputs.empty() ) return ReturnCode::Ok;

    RegisteredCommands::iterator it;
    if ( (it = allCommands.find(inputs[0])) != std::end(allCommands) ) {
        auto ret = (it->second->func)(inputs, outputs);

        for ( auto &o: outputs )
            std::cout << o << std::endl;

        return ret;
    }

    std::cout << "Command " << inputs[0] << " not found" << std::endl;
    return ReturnCode::Error;
}

void CommandLine::registerCommand(const std::string &name, const std::string &info, const std::string &arg_type, const std::vector<std::string> &arg_alert, const Command::CommandFunction &func)
{
    allCommands[name] = std::make_shared<Command>(name, func, info, arg_type, arg_alert);
}

void CommandLine::registerCommand(const Command &command)
{
    allCommands[command.name] = std::make_shared<Command>(command);
}

std::vector<std::string> CommandLine::getRegisteredCommands()
{
    std::vector<std::string> commands;

    for ( auto &pair: allCommands )
        commands.push_back(pair.first);

    return commands;
}

char **CommandLine::completerHelper(const char *text, int start, int )
{
    char **completionList = nullptr;
    RegisteredGenerators::iterator it;

    if ( (it = current_commandline->ParamGenerators.find(current_commandline->current_word_type)) != std::end(current_commandline->ParamGenerators) ) {
        completionList = rl_completion_matches(text, it->second->generate);
    }

    return completionList;
}

void CommandLine::registerParamGenerator(const Generator &generator)
{
    ParamGenerators[generator.arg_type] = std::make_shared<Generator>(generator);
}

std::string CommandLine::getArgType()
{
    return current_word_type;
}

std::string CommandLine::getAlert()
{
    return allCommands.find(current_command)->second->arg_alert[current_words_length - 2];
}

std::string CommandLine::getStatus()
{
    return current_status;
}

size_t CommandLine::getDataLength()
{
    return current_data_length;
}

size_t CommandLine::getTotalDataLength()
{
    return current_total_data_length;
}