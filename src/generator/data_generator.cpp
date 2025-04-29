#include "GeneratorRegister.hpp"

static char *data_length_generate(const char *text, int state)
{
    std::string alert;

    if ( state == 0 ) {
        alert = CommandLine::getCurrentCommandLine()->getAlert();
        alert = "int: " + alert;
        return strdup(alert.c_str());
    }

    return nullptr;
}

static char *data_generate(const char *text, int state)
{
    std::string alert;
    size_t current_data_length = CommandLine::getCurrentCommandLine()->getDataLength();
    size_t total_data_length = CommandLine::getCurrentCommandLine()->getTotalDataLength();

    if ( state == 0 ) {
        alert = std::string("int: data(") + std::to_string(current_data_length) + "/" + std::to_string(total_data_length) + ")";
        return strdup(alert.c_str());
    }

    return nullptr;
}

BEGIN_REGISTER_GENERATOR()
    REGISTER_GENERATOR("l", false, data_length_generate);
    REGISTER_GENERATOR("a", false, data_generate);
END_REGISTER_GENERATOR()