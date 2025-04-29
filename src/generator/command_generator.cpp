#include "GeneratorRegister.hpp"

static char *command_generate(const char *text, int state)
{
    static std::vector<std::string>::iterator it;
    static auto commands = CommandLine::getCurrentCommandLine()->getRegisteredCommands();

    if ( state == 0 ) it = commands.begin();

    while ( it != std::end(commands) ) {
        auto &command = *(it++);
        if ( command.find(text) != std::string::npos ) {
            return strdup(command.c_str());
        }
    }

    return nullptr;
}

static char *alert_command_generate(const char *text, int state)
{
    static std::vector<std::string>::iterator it;
    static auto commands = CommandLine::getCurrentCommandLine()->getRegisteredCommands();

    if ( *text == '\0' ) {
        std::string alert;

        if ( state == 0 ) {
            alert = CommandLine::getCurrentCommandLine()->getAlert();
            alert = "alert: " + alert;
            return strdup(alert.c_str());
        }

        return nullptr;
    }

    if ( state == 0 ) it = commands.begin();

    while ( it != std::end(commands) ) {
        auto &command = *(it++);
        if ( command.find(text) != std::string::npos ) {
            return strdup(command.c_str());
        }
    }

    return nullptr;
}

BEGIN_REGISTER_GENERATOR()
    REGISTER_GENERATOR("c", true, command_generate);
    REGISTER_GENERATOR("m", true, alert_command_generate);
END_REGISTER_GENERATOR()