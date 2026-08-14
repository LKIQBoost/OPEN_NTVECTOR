#include "CallbackManager.h"
#include "ClientInstance.h"
#include "SubChunkClient.h"
#include "PlayStatus.h"
#include "ContainerOpen.h"
#include "InventoryContent.h"
#include "BlockActorData.h"
#include "BlockDataStore.h"

extern unsigned int MinecraftBedrockProtocolVersion;
void CallbackManager::onNetworkSetting(ConnectInstance* m_session, std::vector<uint8_t> packet) {
    LOG(LOG_SERVER, "[Callback] onNetworkSetting - Received network settings packet, size: ", packet.size());
    NetworkSettings ns;
    ns.Deserializ(packet);
    Logger::getInstance().log(LOG_INFO, " Compression algorithm: ", ns.CompressionAlgorithm);
    LOG(LOG_NETWORK, "[Callback] Network compression threshold: ", ns.CompressionThreshold);
    if (ns.CompressionAlgorithm == 0) {
        LOG(LOG_NETWORK, "[Callback] Using Zlib compression algorithm");
        m_session->setCompressionAlgorithm(std::make_unique<ZlibCompress>());
    }
    else if (ns.CompressionAlgorithm == 2) {
        LOG(LOG_NETWORK, "[Callback] Using Zlib Stream compression algorithm");
        m_session->setCompressionAlgorithm(std::make_unique<ZlibStreamCompressor>());
    }
    else
    {
        LOG(LOG_WARN, "[Callback] Unknown compression algorithm: ", ns.CompressionAlgorithm, ", defaulting to Zlib");
        m_session->setCompressionAlgorithm(std::make_unique<ZlibCompress>());
        Logger::getInstance().log(LOG_ERROR, " Unknown compression algorithm.");
    }
    m_session->setNetworkSettingEnabled(true);
    LOG(LOG_NETWORK, "[Callback] Network settings applied, compression enabled");

    LoginPacket lp;
    lp.chain = m_session->chain;
    lp.skindata = m_session->skindata;
    m_session->chain.assign("");
    m_session->skindata.assign("");
    lp.ProtocolVersion = MinecraftBedrockProtocolVersion;
    LOG(LOG_SERVER, "[Callback] Sending LoginPacket with protocol version: ", MinecraftBedrockProtocolVersion);
    m_session->WritePacket(lp);
}

void CallbackManager::onServerToClientHandshake(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_SERVER, "[Callback] onServerToClientHandshake - Processing handshake packet, size: ", packet.size());
    size_t size = 0x30;
    ServerToClientHandshake stch;
    stch.Deserializ(packet);
    LOG(LOG_NETWORK, "[Callback] Parsing JWT from handshake");
    string header;
    string payload;
    string signature;
    Easy::split_jwt(stch.JWT, header, payload, signature);
    payload = Easy::base64url_decode_s(payload);
    payload = Base64Cpp::base64_decode(payload);
    header = Easy::base64url_decode_s(header);
    header = Base64Cpp::base64_decode(header);

    Json::Reader reader;
    Json::Value root;
    reader.parse(header, root);
    string x5u = root["x5u"].asString();
    LOG(LOG_NETWORK, "[Callback] Extracted server public key (x5u)");
    reader.parse(payload, root);
    string salt = string(Base64Cpp::base64_decode(root["salt"].asString()).data(), 16);
    LOG(LOG_NETWORK, "[Callback] Extracted encryption salt from payload");
    x5u = Easy::convertToPEM(x5u);
    LOG(LOG_NETWORK, "[Callback] Loading server EC public key");
    EC_KEY* EC_PUBLIC_KEY = ECC::load_ec_public_key_from_pem(x5u);
    EVP_PKEY* EC_PUBLIC_EVP_KEY = EVP_PKEY_new();
    ECDSA::EC_KEY_AS_EVP_PKEY(EC_PUBLIC_EVP_KEY, EC_PUBLIC_KEY);
    LOG(LOG_NETWORK, "[Callback] Deriving shared secret via ECDH");
    string eckey = string((char*)ECC::derive_shared_secret(m_session->EC_KEY, EC_PUBLIC_EVP_KEY, &size), 0x30);
    unsigned char hash[32];
    calculate_sha256((salt + eckey).data(), 0x40, hash);
    m_session->setSessionKey(vector<uint8_t>(hash, hash + 32));
    LOG(LOG_NETWORK, "[Callback] Session key derived (SHA256 of salt + shared secret)");

    m_session->AESInitialize(vector<unsigned char>(hash, hash + 32));
    LOG(LOG_NETWORK, "[Callback] AES encryption initialized");

    ClientToServerHandshake cth;
    LOG(LOG_SERVER, "[Callback] Sending ClientToServerHandshake response");
    m_session->WritePacket(cth);
    LOG(LOG_NETWORK, "[Callback] Handshake completed, secure channel established");

}

void CallbackManager::onPlayStatus(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_SERVER, "[Callback] onPlayStatus - Received play status packet, size: ", packet.size());
    PlayStatus ps;
    ps.Deserializ(packet);
    LOG(LOG_WARN, "[Callback] PlayStatus value: ", ps.Status);
    // Minecraft PlayStatus: 0=LoginSuccess, 1=FailedClientOutdated, 2=FailedServerOutdated,
    // 3=PlayerSpawn, 4=FailedInvalidTenant, 5=FailedVanillaEdu, 6=FailedEduVanilla,
    // 7=FailedServerFullSubClient
    if (ps.Status != 0 && ps.Status != 3) {
        LOG(LOG_ERROR, "[Callback] PlayStatus indicates failure (", ps.Status, "), server will disconnect");
        // 渚濈劧瑙﹀彂浜嬩欢锛岃Python灞傚彲浠ユ劅鐭?
        PythonEventEngine engine;
        engine.trigger("on_play_status", ps.Status);
        return;
    }
    ClientCacheStatus cth;
    cth.Enabled = true;
    cth.Unknow = false;
    LOG(LOG_SERVER, "[Callback] Sending ClientCacheStatus (Enabled=true)");
    m_session->WritePacket(cth);
    PythonEventEngine engine;
    engine.trigger("on_play_status", ps.Status);
}

void CallbackManager::onResourcePacksInfo(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_SERVER, "[Callback] onResourcePacksInfo - Received resource packs info, size: ", packet.size());
    ResourcePackClientResponse cth;
    cth.Response = 0x03;
    LOG(LOG_SERVER, "[Callback] Sending ResourcePackClientResponse (Response=0x03 - Have all packs)");
    m_session->WritePacket(cth);
    ResourcePackClientResponse cth2;
    cth2.Response = 0x04;
    LOG(LOG_SERVER, "[Callback] Sending ResourcePackClientResponse (Response=0x04 - Completed)");
    m_session->WritePacket(cth2);
    PythonEventEngine engine;
    engine.trigger("on_resource_packs_info", 0);
}

void CallbackManager::onStartGame(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_SERVER, "[Callback] onStartGame - Received start game packet, size: ", packet.size());
    StartGame sg;
    sg.Deserializ(packet);
    Params::PlayerEntityID = ~sg.playerEntityID >> 1;
    m_session->getClientInstance()->getLocalPlayer()->position = sg.PlayerPosition;
    m_session->getClientInstance()->getLocalPlayer()->EntityRuntimeID = sg.EntityRuntimeID;
    LOG(LOG_INFO, "[Callback] Player Entity ID assigned: ", sg.playerEntityID);
    NeteaseJson nj;
    nj.Unknow = true;
    string json;
    Json::Value root;
    Json::FastWriter fw;
    root["eventName"] = "LOGIN_UID";
    root["resid"] = "";
    root["uid"] = Params::UserID;
    nj.Json = fw.write(root);

    if (Params::AutoAuthInput) {
        m_session->getClientInstance()->SetSourceAuthInput(true);
    }
    m_session->getClientInstance()->SetTickHandle(runLoopAuthInput);
    m_session->getClientInstance()->StartTick();

    LOG(LOG_SERVER, "[Callback] Sending NeteaseJson LOGIN_UID event");
    m_session->WritePacket(nj);
    RequestChunkRadius rcr;
    rcr.ChunkRadius = 6;
    rcr.MaxChunkRadius = 12;
    LOG(LOG_SERVER, "[Callback] Sending RequestChunkRadius (Chunk=6, Max=12)");
    if (!Params::Params::RequestChunkRadius) {
        m_session->WritePacket(rcr);
    }

    RequestAbility ra;
    ra.unknown = 2;
    ra.unknown2 = 2;
    ra.unknown3 = 0;
    m_session->WritePacket(ra);
    TickSync ts;
    ts.ClientRequestTick = 0;
    ts.ClientRequestTick = 0;
    m_session->WritePacket(ts);
    LOG(LOG_SCRIPTING, "[Callback] Triggering Python 'start_game' event");
    PythonEventEngine engine;
    engine.trigger("start_game", 0);
    LOG(LOG_INFO, "[Callback] Game started successfully!");
}
void CallbackManager::onStartGameEvent(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_SERVER, "[Callback] onStartGame - Received start game packet, size: ", packet.size());
    StartGame sg;
    sg.Deserializ(packet);
    //Params::PlayerEntityID = ~sg.playerEntityID >> 1;
    m_session->getClientInstance()->getLocalPlayer()->position = sg.PlayerPosition;
    m_session->getClientInstance()->getLocalPlayer()->EntityRuntimeID = sg.EntityRuntimeID;
    LOG(LOG_INFO, "[Callback] Player Entity ID assigned: ", sg.playerEntityID);
    NeteaseJson nj;
    nj.Unknow = true;
    string json;
    Json::Value root;
    Json::FastWriter fw;
    root["eventName"] = "LOGIN_UID";
    root["resid"] = "";
    root["uid"] = Params::UserID;
    nj.Json = fw.write(root);

    if (Params::AutoAuthInput) {
        m_session->getClientInstance()->SetSourceAuthInput(true);
    }
    m_session->getClientInstance()->SetTickHandle(runLoopAuthInput);
    m_session->getClientInstance()->StartTick();

    LOG(LOG_SERVER, "[Callback] Sending NeteaseJson LOGIN_UID event");
    m_session->WritePacket(nj);
    RequestChunkRadius rcr;
    rcr.ChunkRadius = 6;
    rcr.MaxChunkRadius = 12;
    LOG(LOG_SERVER, "[Callback] Sending RequestChunkRadius (Chunk=6, Max=12)");
    m_session->WritePacket(rcr);

    RequestAbility ra;
    ra.unknown = 2;
    ra.unknown2 = 2;
    ra.unknown3 = 0;
    m_session->WritePacket(ra);
    TickSync ts;
    ts.ClientRequestTick = 0;
    ts.ClientRequestTick = 0;
    m_session->WritePacket(ts);
    LOG(LOG_SCRIPTING, "[Callback] Triggering Python 'start_game' event");
    LOG(LOG_INFO, "[Callback] Game started successfully!");
}

void CallbackManager::onPyRpc(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_SERVER, "[Callback] onPyRpc - Received PyRpc packet, size: ", packet.size());
    PythonEventEngine engine;
    PyRpc rpc;
    rpc.Deserializ(packet);
    LOG(LOG_SCRIPTING, "[Callback] Triggering Python 'on_rpc' event, data size: ", rpc.rpcdata.size());
    engine.triggerBytes("on_rpc", rpc.rpcdata);
}

void CallbackManager::onDisconnect(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_WARN, "[Callback] onDisconnect - Received disconnect packet, size: ", packet.size());
    Logger::getInstance().log(LOG_NETWORK, "Engine disconnect.");
    Disconnect d;
    d.Deserializ(packet);
    LOG(LOG_ERROR, "[Callback] Disconnect reason: ", d.message);
    cout << "disconnect: " << WindowsEncodingConverter::utf8ToGbk(d.message) << '\n';
    PythonEventEngine engine;
    engine.trigger("on_disconnect", d.message);
    if (Params::disout) {
        LOG(LOG_WARN, "[Callback] disout flag set, exiting application");
        exit(0);
    }
}

void CallbackManager::onDisconnectEvent(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_WARN, "[Callback] onDisconnect - Received disconnect packet, size: ", packet.size());
    Logger::getInstance().log(LOG_NETWORK, "Engine disconnect.");
    Disconnect d;
    d.Deserializ(packet);
    LOG(LOG_ERROR, "[Callback] Disconnect reason: ", d.message);
    cout << "disconnect: " << WindowsEncodingConverter::utf8ToGbk(d.message) << '\n';
    PythonEventEngine engine;
    engine.trigger("on_disconnect_event", d.message);
    m_session->Disconnect();
}

void CallbackManager::onText(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    //cout << Easy::StringToHex(string((char*)packet.data(), packet.size())) << endl;
    LOG(LOG_SERVER, "[Callback] onText - Received text packet, size: ", packet.size());
    PythonEventEngine engine;
    Text rpc;
    rpc.Deserializ(packet);
    LOG(LOG_INFO, "[Callback] Text message type: ", (int)rpc.type);
    LOG(LOG_SCRIPTING, "[Callback] Triggering Python 'on_text' event");
    engine.trigger("on_text", rpc.type, rpc._data, rpc._data2);
}
void CallbackManager::onAddPlayer(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_ENTITY, "[Callback] onAddPlayer - Received add player packet, size: ", packet.size());
}
void CallbackManager::onPlayerList(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    LOG(LOG_ENTITY, "[Callback] onPlayerList - Received player list packet, size: ", packet.size());
    PlayerList pl;
    pl.Deserializ(packet);
    PythonEventEngine engine;
    engine.trigger("on_player_list", pl);

}
void CallbackManager::onDeathInfo(ConnectInstance* m_session, std::vector<uint8_t> packet) {
    LOG(LOG_ENTITY, "[Callback] onDeathInfo - Received death info packet, size: ", packet.size());
}
void CallbackManager::onIDCommandOutput(ConnectInstance* m_session, std::vector<uint8_t> packet) {
    //std::cout << Easy::StringToHex(std::string((char*)packet.data(), packet.size())) << '\n';
    LOG(LOG_SERVER, "[Callback] onIDCommandOutput - Received command output packet, size: ", packet.size());
    CommandOutput co;
    co.Deserializ(packet);
    LOG(LOG_INFO, "[Callback] Command output received, triggering Python event");
    PythonEventEngine engine;
    std::vector<std::string> strings;
    for (size_t i = 0; i < co.OutputMessages.size(); i++)
    {
        strings.push_back(co.OutputMessages[i].Message);
    }
    engine.trigger("on_command_output", co);
}

void CallbackManager::onIDMovePlayer(ConnectInstance* m_session, std::vector<uint8_t> data)
{
    MovePlayer mp;
    mp.Deserializ(data);
    if (mp.EntityRuntimeID == m_session->getClientInstance()->getLocalPlayer()->EntityRuntimeID)
    {
        m_session->getClientInstance()->getLocalPlayer()->position = mp.Position;
        m_session->getClientInstance()->getLocalPlayer()->inputData.push_back((PlayerAuthInputData)37);
        PythonEventEngine engine;
        engine.trigger("on_move_player", mp.Position.x, mp.Position.y, mp.Position.z);
    }
    //std::cout << "onIDMovePlayer" << '\n';
}
void CallbackManager::onIDMoveActorAbsolute(ConnectInstance*, std::vector<uint8_t>)
{
    //std::cout << "onIDMoveActorAbsolute" << '\n';
}
void CallbackManager::onIDCorrectPlayerMovePrediction(ConnectInstance* ctx, std::vector<uint8_t> data)
{
    // MCBE 1.21.120 CorrectPlayerMovePrediction:
    // prediction_type(u8) position(vec3f) delta(vec3f) rotation(vec2f)
    // angular_velocity(option: 0x00 none / 0x01 + lf32) on_ground(bool) tick(varint64)
    BinaryReader br(data.data(), (int)data.size());
    unsigned char predictionType = br.ReadUInt8();   // 0=player, 1=vehicle
    Vec3 pos = br.ReadVec3();
    br.ReadVec3();                                    // delta, ignored
    br.ReadVec2();                                    // rotation, ignored
    if (br.ReadUInt8() != 0) {                        // optional angular velocity
        br.ReadFloat();                               // ignored
    }
    bool onGround;
    br.ReadBool(onGround);                            // ignored
    br.ReadVarInt64();                                // tick, ignored

    if (predictionType == 0) {
        ctx->getClientInstance()->getLocalPlayer()->position = pos;
    }
    LOG(LOG_ENTITY, "[Callback] onIDCorrectPlayerMovePrediction - corrected position: ", pos.x, ", ", pos.y, ", ", pos.z);
}
void CallbackManager::onIDTickSync(ConnectInstance*, std::vector<uint8_t>)
{
    //std::cout << "onIDTickSync" << '\n';
}
void CallbackManager::onIDUpdatePlayerGameType(ConnectInstance*, std::vector<uint8_t>)
{
    //std::cout << "onIDUpdatePlayerGameType" << '\n';
}
void CallbackManager::onIDSetHealth(ConnectInstance*, std::vector<uint8_t>)
{
    //std::cout << "onIDSetHealth" << '\n';
}
void CallbackManager::onIDRespawn(ConnectInstance* ctx, std::vector<uint8_t> s)
{
    //std::cout << "onIDRespawn" << '\n';
    //cout << Easy::StringToHex(std::string((char*)s.data(), s.size())) << '\n';
    Respawn rs;
    rs.Deserializ(s);
    if (rs.State == 1) {
        //unknown set auth input
    }
    ctx->getClientInstance()->respawn();
    //ctx->getClientInstance()->SetSourceAuthInput(false);
    PythonEventEngine engine;
    engine.trigger("on_respawn", rs.State, rs.EntityRuntimeID);
}
void CallbackManager::onContainerOpen(ConnectInstance* m_session, std::vector<uint8_t> packet) {
    ContainerOpen pkt;
    pkt.Deserializ(packet);
    BlockDataStore::StoreContainerOpen(pkt.ToInfo());
    LOG(LOG_INFO, "[Callback] onContainerOpen - window: ", (int)pkt.WindowID, " type: ", (int)pkt.WindowType, " pos: ", pkt.Position.x, ",", pkt.Position.y, ",", pkt.Position.z);
}

void CallbackManager::onInventoryContent(ConnectInstance* m_session, std::vector<uint8_t> packet) {
    InventoryContent pkt;
    pkt.Deserializ(packet);
    BlockDataStore::StoreContainerContent(pkt.WindowID, pkt.Slots);
    LOG(LOG_INFO, "[Callback] onInventoryContent - window: ", pkt.WindowID, " slots: ", pkt.Slots.size());
}

void CallbackManager::onBlockActorData(ConnectInstance* m_session, std::vector<uint8_t> packet) {
    BlockActorData pkt;
    pkt.Deserializ(packet);
    BlockDataStore::StoreBlockActor(pkt.Position, pkt.RawNBT, pkt.NBTFields);
}
void CallbackManager::onSubChunk(ConnectInstance* m_session, std::vector<uint8_t> packet)
{
    SubChunkClient::OnPacket(packet);
}
void CallbackManager::runLoopAuthInput(ClientInstance* ctx)
{
    PlayerAuthInput a;
    a.headRotation = ctx->getLocalPlayer()->headRotation;
    a.position = ctx->getLocalPlayer()->position;
    a.MoveVector.x = 0;
    a.MoveVector.y = 0;
    a.headYaw = ctx->getLocalPlayer()->headYaw;
    a.inputData = ctx->getLocalPlayer()->inputData;
    // 锟节讹拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷珊锟揭伙拷锟斤拷锟缴撅拷锟侥匡拷锟皆拷兀锟斤拷锟斤拷锟斤拷锟斤拷锟叫э拷锟?
    ctx->getLocalPlayer()->inputData.erase(
        std::remove(ctx->getLocalPlayer()->inputData.begin(), ctx->getLocalPlayer()->inputData.end(), (PlayerAuthInputData)37),
        ctx->getLocalPlayer()->inputData.end()
    );
    //a.inputData.push_back((PlayerAuthInputData)0x18);
    //a.inputData.push_back((PlayerAuthInputData)0x31);
    //a.inputData.push_back((PlayerAuthInputData)0x33);
    a.InputMode = MOUSE;
    a.playMode = 0;
    a.playMode = 0;
    a.interactionModel = 0;   // 必须显式赋值：未初始化(Release 下随机)会写入垃圾 varint，导致 auth input 后续字段解析错位、整包被服务器丢弃
    a.interactPitch = 0;
    a.interactYaw = 0;
    a.Tick = ctx->getTick();
    a.delta = Vec3();
    a.bypassMoveVector = false;
    a.analogueMoveVector = Vec2();
    a.cameraOrientation = Vec3();
    a.rawMoveVector = Vec2();
    a.cameraDeparted = false;
    a.thirdPersonPerspective = 0;
    a.playerRotationToCamera = 0;
    a.readyPosDeltaDirty = ctx->getLocalPlayer()->readyPosDeltaDirty;
    a.isOnGround = true;
    a.resetPosition = false;
    std::vector<uint8_t> s = a.Serializ();
    //cout << Easy::StringToHex(std::string((char*)s.data(), s.size())) << '\n';
    ctx->getInstance()->WritePacket(a);
}
