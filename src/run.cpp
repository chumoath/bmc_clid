#include "CommandLine.hpp"
#include <iostream>

extern "C" const char *greeting;
const char *greeting = "EXTERNAL> ";

extern "C" int run(void)
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