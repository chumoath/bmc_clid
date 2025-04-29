#pragma once

#include "CommandLine.hpp"
#include <iostream>

static void register_generator() __attribute__((constructor));

#define BEGIN_REGISTER_GENERATOR()  void register_generator() { Generator generator;

#define REGISTER_GENERATOR(_arg_type, _is_completion, _generate) \
    do {   \
        generator.arg_type = _arg_type; \
        generator.isCompletion = _is_completion; \
        generator.generate = _generate; \
        CommandLine::getCurrentCommandLine()->registerParamGenerator(generator); \
    } while (0)

#define END_REGISTER_GENERATOR()    }