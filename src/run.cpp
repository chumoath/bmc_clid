#include "CommandLine.hpp"
#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>

#ifdef NEED_SERVER
extern "C" int run(const char *greeting);
int run(const char *greeting)
#else
int main(int argc, const char **argv)
#endif
{
    CommandLine *c = CommandLine::getCurrentCommandLine();
    int retCode;

    do {
        #ifdef NEED_SERVER
        retCode = c->readLine(greeting);
        #else
        retCode = c->readLine();
        #endif

        if ( retCode == 1 ) {
            std::cout << "Received error code 1" << std::endl;
        } else if ( retCode == 2 ) {
            std::cout << "Received error code 2" << std::endl;
        }
    } while ( retCode != ReturnCode::Quit);

    return 0;
}