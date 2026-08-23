#include "SettingsCommand.h"

unsigned char SettingsCommand::ID()
{
    return IDSettingsCommand;
}

std::vector<unsigned char> SettingsCommand::Serializ()
{
    int chainsize = command.size();
    BinaryWriter bw(chainsize + 4);
    bw.WriteUInt8(ID());
    bw.WriteUInt8(1);
    bw.WriteVarUInt(chainsize);
    bw.Write(command.data(), chainsize);
    bw.WriteUInt8(SuppressOutput);
    return bw.vect();
}
