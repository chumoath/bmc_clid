#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include "Command.hpp"

int execPopen(const std::string &cmdline, std::string &output);
std::string intToHexStr(int num);
int strToNum(const std::string &numStr);