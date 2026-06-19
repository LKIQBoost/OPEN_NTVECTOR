#pragma once
#include <string>
//#include <RakPeerInterface.h>
//#include <MessageIdentifiers.h>
//#include <RakPeerInterface.h>
//#include <MessageIdentifiers.h>
//#include <BitStream.h>
#include <SLikeNet/BitStream.h>
#include <MessageIdentifiers.h>
#include <RakPeerInterface.h>
#include <RakNetTypes.h>
#include <RakSleep.h>

#include <iostream>
#include <future>
#include "ECC.h"
#include "PacketBase.h"
#include "Logger.h"
#include "PacketCommon.h"
#include "PacketBuffer.h"
#include "SocketCallback.h"
#include "ZlibCompress.h"
#include "ZlibStreamCompressor.h"
#include "AES_GCM.h"
#include "Sha256Utils.h"
#include "LoginAuth.h"
#include "PythonWrapperCallback.h"
#include "PyEventHandler.h"

class ClientInstance;
class ConnectInstance
{
public:
	using ReceiveCallback = std::function<void(ConnectInstance*, const std::vector<uint8_t>&)>;
	ConnectInstance(std::string chain, std::string skindata, std::string ip, int port, EVP_PKEY* ec_key, ClientInstance*);
    virtual ~ConnectInstance();                          // �� virtual
    virtual void Connect();                              // �� virtual
    virtual void Disconnect();                           // �� virtual
    virtual int WritePacket(PacketBase& Packet);         // �� virtual
    virtual int WritePacket(std::string Packet);         // �� virtual
    virtual int WritePacket(std::vector<uint8_t> Packet);// �� virtual
	void HandlePakcet(unsigned char* data, size_t length);
    void RegisterReceiveCallBack(uint32_t id, ReceiveCallback callback);
    void RegisterReceiveCallBack(uint32_t id, PyObject* callback, bool);
    void ClearUserEvent();
    void ClearKernelEvent();
    void ClearAllEvent();
    void setSessionKey(std::vector<uint8_t> key) { SESSING_KEY = std::move(key); }
    void setCompressionAlgorithm(std::unique_ptr<ZlibCompress> algo) {
        m_algorithm = std::move(algo);
    }
    void setNetworkSettingEnabled(bool enabled) { is_networksetting = enabled; }
	std::string chain;
	std::string skindata;
	std::string ip;
	int port;
	EVP_PKEY* EC_KEY;
	//ZlibCompress* m_algorithm;
    bool is_disconnect_exit = false;
	void AESInitialize(std::vector<unsigned char> key256);
    ClientInstance* getClientInstance() {
        return m_instance;
    }
    void setDisconnectExit() {
        is_disconnect_exit = true;
    }
protected:
    bool is_networksetting = false;
    bool is_servertoclient = false;
    bool m_is_disconnect;
    PacketBuffer m_buffer;
    std::unique_ptr<AesGcm> SendKey;
    std::unique_ptr<AesGcm> RecvKey;
    std::vector<uint8_t> SESSING_KEY;
    uint64_t m_sendnum;
    std::unique_ptr<SocketCallback> m_callback;
    std::unique_ptr<PythonWrapperCallback> m_event_wrapper;
    std::unique_ptr<ZlibCompress> m_algorithm;
private:
	static size_t calculateVarintSizeFast(int32_t value) {
		uint32_t v = static_cast<uint32_t>(value < 0 ? ~value + 1 : value);
		if (v < (1 << 7)) return 1;
		if (v < (1 << 14)) return 2;
		if (v < (1 << 21)) return 3;
		if (v < (1 << 28)) return 4;
		return 5;  // int32_t ��� 5 �ֽڣ�������λ��
	}
	void SendPacket();
	void ThreadHandle();
	uint64_t m_recvnum;

	std::thread m_recv_thread;
	std::thread m_send_thread;
	SLNet::RakPeerInterface* peer;
	using PacketHandler = void(*)(const char* data, size_t len);
	SLNet::SystemAddress m_address;
    ClientInstance* m_instance;
	static constexpr char header = 0xfe;
	static vector<uint8_t> computeGcmSign(vector<uint8_t> vec, vector<uint8_t> aeskey, uint64_t num);
};

