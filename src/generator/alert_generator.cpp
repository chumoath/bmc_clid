#include "GeneratorRegister.hpp"

static char *int_generate(const char *text, int state)
{
    std::string arg_type;
    std::string alert;

    if ( state == 0 ) {
        alert = CommandLine::getCurrentCommandLine()->getAlert();
        if ( CommandLine::getCurrentCommandLine()->getArgType() == "x" ) {
            alert = "int: " + alert;
            return strdup(alert.c_str());
        }
    }

    return nullptr;
}

static char *string_generate(const char *text, int state)
{
    std::string arg_type;
    std::string alert;

    if ( state == 0 ) {
        alert = CommandLine::getCurrentCommandLine()->getAlert();
        if ( CommandLine::getCurrentCommandLine()->getArgType() == "s" ) {
            alert = "string: " + alert;
            return strdup(alert.c_str());
        }
    }

    return nullptr;
}

static char *status_generate(const char *text, int state)
{
    std::string status;

    if ( state == 0 ) {
        status = CommandLine::getCurrentCommandLine()->getStatus();
        return strdup(status.c_str());
    }

    return nullptr;
}

BEGIN_REGISTER_GENERATOR()
    REGISTER_GENERATOR("x", false, int_generate);
    REGISTER_GENERATOR("s", false, string_generate);
    REGISTER_GENERATOR("e", false, status_generate);
END_REGISTER_GENERATOR()