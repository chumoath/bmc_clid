#include "PluginCommon.hpp"
#include <vector>
#include <string>

static int test_read(const std::vector<std::string> &Inputs, std::vector<std::string> &Outputs)
{
    Outputs.emplace_back("0x90 0x80");
    return ReturnCode::Ok;
}

static int test_write(const std::vector<std::string> &Inputs, std::vector<std::string> &Outputs)
{
    return ReturnCode::Ok;
}

BEGIN_REGISTER_COMMAND()
    REGISTER_COMMAND("test_read", test_read, "Test read value.", "sx", { "ChipName", "Offset" });
    REGISTER_COMMAND("test_write", test_write, "Test write value.", "sxla", { "ChipName", "Offset", "Wlen", "Data" });
END_REGISTER_COMMAND()