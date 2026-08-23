#include "ConnectInstance.h"
#include "RakPeer.h"
#include <fstream>

extern unsigned int MinecraftBedrockProtocolVersion;
ConnectInstance::ConnectInstance(std::string chain, std::string skindata, std::string ip, int port, EVP_PKEY* ec_key, ClientInstance* Instance) : chain(std::move(chain)),
    skindata(std::move(skindata)),
    ip(std::move(ip)),
    port(port),
    EC_KEY(ec_key),
    m_instance(Instance),
    m_callback(std::make_unique<SocketCallback>(this)),
    m_event_wrapper(std::make_unique<PythonWrapperCallback>()),
    peer(nullptr),
    m_is_disconnect(true),
    m_sendnum(0),
    m_recvnum(0)
{ // Use initializer list to initialize m_callback
   //memset(this, 0, sizeof(*this)); // Correct sizeof(this) to sizeof(*this)
   //this->chain = std::string(chain);
   //this->m_instance = Instance; // Initialize m_instance to nullptr
   //this->m_callback = std::make_unique<SocketCallback>(this); // Initialize m_callback with a new SocketCallback instance
   //this->m_event_wrapper = std::make_unique<PythonWrapperCallback>();
   //this->skindata = std::string(skindata);
   //this->ip = std::string(ip);
   //this->port = port;
   //this->EC_KEY = ec_key;
   //this->m_is_disconnect = true;
   //this->peer = NULL;
   //this->m_recvnum = 0;
   //this->m_sendnum = 0;
}
ConnectInstance::~ConnectInstance() {
    Disconnect();
    if (m_recv_thread.joinable()) {
        m_recv_thread.join();
    }
    if (m_send_thread.joinable()) {
        m_send_thread.join();
    }
}
void ConnectInstance::Disconnect() {

    m_is_disconnect = true;
}

void ConnectInstance::SendPacket()
{
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        if (m_is_disconnect)
            break;
        if (m_buffer.hasDataInCurrentBuffer()) {
            BufferSlice Buffer = m_buffer.swapAndGetBuffer();
            std::vector<uint8_t> data(Buffer.data, Buffer.data + Buffer.size);

            if (is_networksetting)
                data = std::move(m_algorithm->Compress(data));
            if (is_servertoclient) {
                unsigned char hash[32];
                unsigned char ulong[8];
                memcpy(ulong, &m_sendnum, 8);
                vector<uint8_t> buf(ulong, ulong + 8);
                buf.insert(buf.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
                buf.insert(buf.end(), std::make_move_iterator(SESSING_KEY.begin()), std::make_move_iterator(SESSING_KEY.end()));
                calculate_sha256((char*)buf.data(), buf.size(), hash);
                m_sendnum++;
                data.insert(data.end(), hash, hash + 8);
                //cout << Easy::StringToHex_s((char*)buf.data(),buf.size())<<'\n';



                SendKey->encrypt(data);
            }
            BinaryWriter br2(data.size() + 1);
            br2.Write(&header, 1);
            br2.Write(data.data(), data.size());
            peer->Send(
                (const char*)br2.data(),
                (int)br2.size(),
                MEDIUM_PRIORITY,
                UNRELIABLE,
                0,
                this->m_address, // 目标地址
                false
            );
            m_buffer.finishSending();
        }
    }
}

void ConnectInstance::ThreadHandle() {


    // 创建套接字描述符
    SLNet::SocketDescriptor sd;

    // 启动客户端
    peer->Startup(1, &sd, 1);

    // 连接到服务器
    //std::cout << "Connecting to server..." << std::endl;
    SLNet::ConnectionAttemptResult res = peer->Connect(ip.data(), port, 0, 0);

    if (res != SLNet::CONNECTION_ATTEMPT_STARTED)
    {
        //std::cout << "Unable to start connection, error number: " << res << std::endl;
        return;
    }

    SLNet::Packet* packet;
    // 处理收到的数据包
    for (packet = peer->Receive(); true; peer->DeallocatePacket(packet), packet = peer->Receive())
    {
        if (m_is_disconnect)
            break;

        // 如果没有收到数据包，等待一帧避免忙等
        if (!packet) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        RequestNetworkSettings r;
        if (packet) {
            switch (packet->data[0])
            {
            case ID_CONNECTION_REQUEST_ACCEPTED:
                this->m_address = packet->systemAddress;
                Logger::getInstance().log_("ID_CONNECTION_REQUEST_ACCEPTED", LOG_NETWORK);
                r.ProtocolVersion = MinecraftBedrockProtocolVersion;
                WritePacket(r);
                //std::cout << "Our connection request has been accepted." << std::endl;
                break;

            case ID_DISCONNECTION_NOTIFICATION:
                Logger::getInstance().log_("ID_DISCONNECTION_NOTIFICATION", LOG_NETWORK);
                //std::cout << "We have been disconnected." << std::endl;
                if (is_disconnect_exit)
                    exit(0);
                break;

            case ID_CONNECTION_LOST:
                Logger::getInstance().log_("ID_CONNECTION_LOST", LOG_NETWORK);
                if (is_disconnect_exit)
                    exit(0);
                break;

            case ID_CONNECTION_ATTEMPT_FAILED:     // 连接尝试失败
                Logger::getInstance().log_("ID_CONNECTION_ATTEMPT_FAILED", LOG_NETWORK);
                if (is_disconnect_exit)
                    exit(0);
                break;
            case RakNetID:
            {
                // 收到普通消息
                HandlePakcet((unsigned char*)packet->data + 1, packet->length - 1);
            }
            default:
                break;
            }
        }
    }


    // 清理
    delete peer;
}
vector<uint8_t> ConnectInstance::computeGcmSign(vector<uint8_t> vec, vector<uint8_t> aeskey, uint64_t num)
{
    unsigned char hash[32];
    unsigned char ulong[8];
    memcpy(ulong, &num, 8);
    vector<uint8_t> buf(ulong, ulong + 8);
    buf.insert(buf.end(), vec.begin(), vec.end());
    buf.insert(buf.end(), aeskey.begin(), aeskey.end());
    calculate_sha256((char*)buf.data(), buf.size(), hash);
    //cout << Easy::StringToHex_s((char*)buf.data(),buf.size())<<'\n';
    return vector<uint8_t>(hash, hash + 8);
}
void ConnectInstance::AESInitialize(std::vector<unsigned char> key256)
{
    std::vector<unsigned char> key = key256;
    std::vector<unsigned char> iv = std::vector<unsigned char>(key.data(), key.data() + 16);
    SendKey = std::make_unique<AesGcm>(key, iv, false);
    RecvKey = std::make_unique<AesGcm>(key, iv, true);
    is_servertoclient = true;
}
int ConnectInstance::WritePacket(PacketBase& Packet) {
    vector<uint8_t> data = Packet.Serializ();
    m_buffer.appendData(data);

    //cout << Easy::StringToHex_s((char*)data.data(), data.size()) << '\n';
    return 0;
}
int ConnectInstance::WritePacket(std::string Packet)
{
    m_buffer.appendData(std::vector<uint8_t>(Packet.data(), Packet.data() + Packet.size()));
    return 0;
}
int ConnectInstance::WritePacket(std::vector<uint8_t> Packet)
{
    m_buffer.appendData(Packet);
    return 0;
}
void ConnectInstance::Connect() {
    if (m_is_disconnect) {
        m_is_disconnect = false;
        peer = new SLNet::RakPeer();

        //peer->ApplyNetworkSimulator(0.2f, 0, 0);
        m_recv_thread = thread(&ConnectInstance::ThreadHandle, this);
        m_send_thread = thread(&ConnectInstance::SendPacket, this);
        m_recv_thread.detach();
        m_send_thread.detach();
    }
    return;
}
void ConnectInstance::HandlePakcet(unsigned char* data, size_t length) {
    vector<uint8_t> packetdata(data, data + length);
    if (is_servertoclient) {
        //vector<unsigned char> packetdata2;
        RecvKey->decrypt(packetdata);
        unsigned char gcmsign[8];
        uint8_t* tp = packetdata.data();
        size_t datal = packetdata.size();
        memcpy(gcmsign, tp + datal - 8, 8);
        packetdata.resize(datal - 8);
        vector<unsigned char> sign = computeGcmSign(packetdata, SESSING_KEY, m_recvnum);
        m_recvnum++;
        if (memcmp(gcmsign, sign.data(), 8)) {
            Logger::getInstance().log(LOG_ERROR, "Message verify sign failed!");
            throw std::runtime_error("Message verify sign failed!");
        }
    }
    if (is_networksetting)
        packetdata = m_algorithm->Decompress(packetdata);
    if (!packetdata.empty()) {
        BinaryReader br(packetdata.data(), packetdata.size());
        while (true) {
            uint32_t packetlength = br.ReadVarUInt();
            char* packet = (char*)br.Read(packetlength);

            BinaryReader br2(packet, packetlength);
            uint32_t packetid;
            uint32_t length = br2.ReadVarUInt32(packetid);
            std::string payload = std::string((const char*)(br2.Read(packetlength - length)), packetlength - length);
            // ID=5 的内核事件触发时写入 dis.log
            if (packetid == 5) {
                static std::mutex dis_log_mutex;
                std::lock_guard<std::mutex> lock(dis_log_mutex);
                std::ofstream dis_log("dis.log", std::ios::app);
                if (dis_log.is_open()) {
                    auto now = std::chrono::system_clock::now();
                    auto now_time = std::chrono::system_clock::to_time_t(now);
                    std::tm time_info;
#ifdef _WIN32
                    localtime_s(&time_info, &now_time);
#else
                    localtime_r(&now_time, &time_info);
#endif
                    dis_log << std::put_time(&time_info, "[%Y-%m-%d %H:%M:%S]")
                            << " [KernelEvent ID=5] payload_size=" << payload.size()
                            << " payload=" << payload << std::endl;
                    dis_log.close();
                }
            }
            m_event_wrapper->invokeEventHandlers(packetid, payload);
            m_callback->invokeCallback(packetid, (const unsigned char*)(payload.c_str()), payload.size());
            size_t d = br.m_pointer;
            if (d == packetdata.size())
                break;
        }
    }
}

void ConnectInstance::ClearUserEvent() {
    m_event_wrapper->clearUserEvent();
}
void ConnectInstance::ClearKernelEvent() {
    m_event_wrapper->clearKernelEvent();
}
void ConnectInstance::ClearAllEvent() {
    m_event_wrapper->clearAllEvent();
}


void ConnectInstance::RegisterReceiveCallBack(uint32_t id, ReceiveCallback callback)
{
    m_callback->registerReceiveCallback(id, callback);
}
void ConnectInstance::RegisterReceiveCallBack(uint32_t id, PyObject* callback, bool is_kernel)
{
    m_event_wrapper->registerEventHandler(id, PyEventHandler(callback, is_kernel));
}
