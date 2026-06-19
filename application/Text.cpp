#include "Text.h"
#include "Logger.h"

unsigned char Text::ID()
{
    return IDText;
}

void Text::Deserializ(std::vector<unsigned char> pack)
{
    LOG(LOG_SERVER, "[Text] Deserializing text packet, size: ", pack.size());
    BinaryReader br(pack.data(), pack.size());
    type = br.ReadUInt8();
    LOG(LOG_SERVER, "[Text] Message type: ", (int)type);
    if (type == 9) {
        br.ReadUInt8();
        br.ReadStringUTF(_data);
        int lengt = br.ReadVarUInt();
        _data2 = string((char*)br.Read(lengt), lengt);
    }
    if (type == 1) {
        br.ReadUInt8();
        int lengt = br.ReadVarUInt();
        _data = string((char*)br.Read(lengt), lengt);
        lengt = br.ReadVarUInt();
        _data2 = string((char*)br.Read(lengt), lengt);
        br.ReadUInt32();
        lengt = br.ReadVarUInt();
        SysMsg = string((char*)br.Read(lengt), lengt);
    }
    if (type == 2)
    {
        br.ReadUInt8();
        int lengt = br.ReadVarUInt();
        _data = string((char*)br.Read(lengt), lengt);
        br.ReadUInt8();
        lengt = br.ReadVarUInt();
        _data2 = string((char*)br.Read(lengt), lengt);
    }
    if (type == 8)
    {
        br.ReadStringUTF(_data);
        br.ReadStringUTF(_data2);
    }
    //_data = std::string((char*)pack.data(), pack.size());
}

std::vector<unsigned char> Text::Serializ()
{
    int chainsize = _data.size() + _data2.size();
    BinaryWriter bw(chainsize + 10);
    bw.WriteUInt8(ID());
    bw.WriteUInt8(type);
    bw.WriteUInt8(0);
    bw.WriteVarUInt(_data.size());
    bw.Write(_data.data(), _data.size());
    bw.WriteVarUInt(_data2.size());
    bw.Write(_data2.data(), _data2.size());
    bw.WriteUInt32(0);
    return bw.vect();
}
