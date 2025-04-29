#include "GeneratorRegister.hpp"

static char *test_generate(const char *text, int state)
{
    static std::vector<std::string>::iterator it;
    static std::vector<std::string> completeList = {
        "hello",
        "world",
        "complete"
    };

    if ( state == 0 ) it = std::begin(completeList);

    while ( it != std::end(completeList) ) {
        auto &str = *(it++);
        if ( str.find(text) != std::string::npos ) {
            return strdup(str.c_str());
        }
    }

    return nullptr;
}

BEGIN_REGISTER_GENERATOR()
    REGISTER_GENERATOR("t", false, test_generate);
END_REGISTER_GENERATOR()