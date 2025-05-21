#include "PluginCommon.hpp"
#include <vector>
#include <string>
#include "Common.hpp"

static int i2c_read(const std::vector<std::string> &Inputs, std::vector<std::string> &Outputs)
{
    std::ostringstream oss;
    std::string cmdline;
    std::string output;
    int offset_width;
    int offset;
    int ret;

    if ( Inputs.size() != 5 ) {
        Outputs.emplace_back("Invalid number of parameters.");
        return ReturnCode::Error;
    }

    offset = strToNum(Inputs[4]);
    if ( offset < 0 ) {
        Outputs.emplace_back("Invalid argument: offset");
        return ReturnCode::Error;
    }

    if ( offset > 0xFFFF ) {
        Outputs.emplace_back("Invalid Offset: offset should less 0xFFFF");
        return ReturnCode::Error;
    }

    offset_width = offset > 0xFF ? 2 : 1;

    oss << "i2ctransfer -f -y " << Inputs[1] << " w" << offset_width << "@" << Inputs[3] << " ";

    if ( offset > 0xFF ) {
        oss << intToHexStr(offset >> 8) << " ";
    }

    oss << intToHexStr(offset & 0xFF) << " " << "r" << Inputs[2];

    cmdline = oss.str();

    std::cout << "cmdline: " << cmdline << std::endl;

    ret = execPopen(cmdline, output);

    Outputs.emplace_back(output);

    return ret;
}

static int i2c_write(const std::vector<std::string> &Inputs, std::vector<std::string> &Outputs)
{
    std::ostringstream oss;
    std::string cmdline;
    std::string output;
    int offset;
    int wlen;
    int ret;

    if ( Inputs.size() < 6 ) {
        Outputs.emplace_back("Invalid number of parameters.");
        return ReturnCode::Error;
    }

    offset = strToNum(Inputs[4]);
    if ( offset < 0 ) {
        Outputs.emplace_back("Invalid argument: offset");
        return ReturnCode::Error;
    }

    wlen = strToNum(Inputs[2]);
    if ( wlen < 0 ) {
        Outputs.emplace_back("Invalid argument: wlen");
        return ReturnCode::Error;
    }

    if ( offset > 0xFFFF ) {
        Outputs.emplace_back("Invalid Offset: offset should less 0xFFFF");
        return ReturnCode::Error;
    }

    if ( Inputs.size() != 5 + wlen ) {
        Outputs.emplace_back("Invalid number of parameters.");
        return ReturnCode::Error;
    }

    wlen = offset > 0xFF ? 2 + wlen : 1 + wlen;

    oss << "i2ctransfer -f -y " << Inputs[1] << " w" << wlen << "@" << Inputs[3] << " ";

    if ( offset > 0xFF ) {
        oss << intToHexStr(offset >> 8) << " ";
    }

    oss << intToHexStr( offset & 0xFF ) << " ";

    for ( auto it = Inputs.begin() + 5; it != Inputs.end(); it++ ) {
        oss << *it << " ";
    }

    cmdline = oss.str();
    std::cout << "cmdline: " << cmdline << std::endl;
    ret = execPopen(cmdline, output);

    Outputs.emplace_back(output);

    return ret;
}

BEGIN_REGISTER_COMMAND()
    REGISTER_COMMAND("i2c_read", i2c_read, "Read value from i2c device.", "xxxx", { "bus id", "read len", "slave addr", "offset" });
    REGISTER_COMMAND("i2c_write", i2c_write, "Write value to i2c device.", "xlxxa", { "bus id", "write len", "slave addr", "offset", "data" });
END_REGISTER_COMMAND()