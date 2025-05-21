#include "Common.hpp"

int execPopen(const std::string &cmdline, std::string &outputs)
{
    std::ostringstream oss;
    int status;
    char buf[256];
    auto fp = popen(cmdline.c_str(), "r");

    if ( fp == nullptr ) {
        outputs = "Popen failed";
        return ReturnCode::Error;
    }

    while ( fgets(buf, 256, fp) != nullptr ) {
        oss << buf;
    }

    status = pclose(fp);
    outputs = oss.str();

    return status;
}

int strToNum(const std::string &numStr)
{
    std::istringstream iss(numStr);
    int ret;

    if ( numStr.substr(0, 2) == "0x" ) {
        iss >> std::hex >> ret;
    } else {
        iss >> ret;
    }

    if ( iss.fail() ) {
        ret = -ReturnCode::Error;
    }

    return ret;
}