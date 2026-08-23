#include "ClientInstance.h"

void ClientInstance::BaseTick()
{
    while (!m_is_disconnect) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (m_is_disconnect) break;

        if (TickHandel != nullptr && m_source_auth_input) {
            TickHandel(this);
        }
        ++Tick;
    }
}
void ClientInstance::StartTick()
{
    if (m_baseTick.joinable()) {
        LOG(LOG_WARN, "[ClientInstance] StartTick called but tick already running");
        return;
    }
    m_is_disconnect = false;   // 确保新线程能正常跑
    m_baseTick = std::thread(&ClientInstance::BaseTick, this);
}
void ClientInstance::SetTickHandle(std::function<void(ClientInstance*)> handel)
{
    TickHandel = handel;
}