#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include "Command.hpp"

int execPopen(const std::string &cmdline, std::string &output);
template<typename T>
std::string intToHexStr(T num)
{
    std::stringstream ss;
    ss << "0x" << std::hex << num;
    return ss.str();
}
int strToNum(const std::string &numStr);