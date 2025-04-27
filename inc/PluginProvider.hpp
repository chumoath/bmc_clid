#pragma once

#include <forward_list>
#include <string>

#define COMMAND_LIB_PATH       ""
#define GENERATOR_LIB_PATH     ""

class PluginProvider
{

};

std::forward_list<PluginProvider> loadProvider(const std::string& pathLib);