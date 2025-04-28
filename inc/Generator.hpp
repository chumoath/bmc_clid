#pragma once

#include <readline/readline.h>

struct Generator
{
    bool isCompletion;
    rl_compentry_func_t *generate;
    std::string arg_type;
};