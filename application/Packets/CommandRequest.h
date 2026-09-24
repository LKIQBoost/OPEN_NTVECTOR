#pragma once
#include "PacketBase.h"
#include <utility>

enum class CommandOriginType : char {
    Player = 0x0,
    CommandBlock = 0x1,
    MinecartCommandBlock = 0x2,
    DevConsole = 0x3,
    Test = 0x4,
    AutomationPlayer = 0x5,
    ClientAutomation = 0x6,
    DedicatedServer = 0x7,
    Entity = 0x8,
    Virtual = 0x9,
    GameArgument = 0xA,
    EntityServer = 0xB,
    Precompiled = 0xC,
    GameDirectorEntityServer = 0xD,
    Scripting = 0xE,
    ExecuteContext = 0xF,
};

class CommandRequest :
    public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;

    void SetCommand(std::string value) { m_command = std::move(value); }
    const std::string& GetCommand() const noexcept { return m_command; }

    void SetCommandOrigin(::CommandOriginType value) noexcept { m_commandOrigin = value; }
    ::CommandOriginType GetCommandOrigin() const noexcept { return m_commandOrigin; }

    void SetRandomUUID(std::string value) { m_randomUUID = std::move(value); }
    const std::string& GetRandomUUID() const noexcept { return m_randomUUID; }

    void SetRequestID(std::string value) { m_requestID = std::move(value); }
    const std::string& GetRequestID() const noexcept { return m_requestID; }

    void SetPlayerID(int64_t value) noexcept { m_playerID = value; }
    int64_t GetPlayerID() const noexcept { return m_playerID; }

    void SetInternalSource(bool value) noexcept { m_internalSource = value; }
    bool GetInternalSource() const noexcept { return m_internalSource; }

    void SetCommandVersion(int value) noexcept { m_commandVersion = value; }
    int GetCommandVersion() const noexcept { return m_commandVersion; }

    void SetUnlimit(bool value) noexcept { m_unlimit = value; }
    bool GetUnlimit() const noexcept { return m_unlimit; }

private:
    std::string m_command;
    ::CommandOriginType m_commandOrigin = ::CommandOriginType::Player;
    std::string m_randomUUID;
    std::string m_requestID;
    int64_t m_playerID = -1;
    bool m_internalSource = false;
    int m_commandVersion = 5111808;
    bool m_unlimit = false;
};
