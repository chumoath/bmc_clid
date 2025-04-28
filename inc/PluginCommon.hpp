#pragma once

#if defined(NEED_PLUGIN)
#include "CommandRegister.hpp"
#elif defined(NEED_BMCBOX)
#include "BmcboxCommandRegister.hpp"
#elif defined(NEED_SPLIT)
#include "BmcboxCommandRegister.hpp"
#include "bmcbox.cpp"
#include "Command.cpp"
#else
#error "Either NEED_PLUGIN or NEED_BMCBOX or NEED_SPLIT must be defined"
#endif