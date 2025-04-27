#pragma once

#include "CommandLine.hpp"
#include <iostream>

static void register_command() __attribute__((constructor));

#define BEGIN_REGISTER_COMMAND() void register_command() {

#define REGISTER_COMMAND(_name, _func, _info, _arg_type, ...) \
    do { \
        CommandLine::registerCommand(_name, _info, _arg_type, ##_VA_ARGS__, _func); \
    } while (0)

#define END_REGISTER_COMMAND() }