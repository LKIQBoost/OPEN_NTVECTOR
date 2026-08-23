#include "PyRpc.h"
#include "Logger.h"

unsigned char PyRpc::ID()
{
    return IDPyRpc;
}

void PyRpc::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), pack.size());
    //unknow = br.ReadUInt8();
    int lengt = br.ReadVarInt();
    rpcdata = string((char*)br.Read(lengt), lengt);
    RpcHeader = br.ReadUInt32();
}

std::vector<unsigned char> PyRpc::Serializ()
{
    int chainsize = rpcdata.size();
    BinaryWriter bw(chainsize + 10);
    bw.WriteUInt8(ID());
    bw.WriteUInt8(1);
    bw.WriteVarUInt(chainsize);
    bw.Write(rpcdata.data(), chainsize);
    bw.WriteUInt32(PyRpcClientID);
    return bw.vect();
}
