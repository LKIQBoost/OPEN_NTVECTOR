#pragma once

#include "BinaryWriter.h"
#include "PlayerAuthInput.h"
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <vector>

struct LocalPlayerSnapshot {
    bool available = false;
    bool positionAvailable = false;
    bool rotationAvailable = false;
    int64_t entityRuntimeID = 0;
    Vec2 headRotation{};
    Vec3 position{};
    Vec2 moveVector{};
    float headYaw = 0;
    std::vector<PlayerAuthInputData> inputData;
    bool readyPosDeltaDirty = false;
};

class LocalPlayer
{
public:
    LocalPlayer() = default;

    void Reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_available = false;
        m_positionAvailable = false;
        m_rotationAvailable = false;
        m_entityRuntimeID = 0;
        m_headRotation = {};
        m_position = {};
        m_moveVector = {};
        m_headYaw = 0;
        m_inputData.clear();
        m_readyPosDeltaDirty = false;
    }

    void SetServerState(int64_t runtimeID, const Vec3& value, float pitch = 0, float yaw = 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entityRuntimeID = runtimeID;
        m_position = value;
        m_headRotation = Vec2{ pitch, yaw };
        m_headYaw = yaw;
        m_available = true;
        m_positionAvailable = true;
        m_rotationAvailable = true;
    }

    LocalPlayerSnapshot GetSnapshot() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return { m_available, m_positionAvailable, m_rotationAvailable, m_entityRuntimeID,
                 m_headRotation, m_position, m_moveVector, m_headYaw, m_inputData,
                 m_readyPosDeltaDirty };
    }

    void SetPosition(const Vec3& value, bool serverConfirmed = true) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_position = value;
        m_positionAvailable = serverConfirmed;
    }

    void SetRotation(float pitch, float yaw, float headYaw, bool serverConfirmed = true) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_headRotation = Vec2{ pitch, yaw };
        m_headYaw = headYaw;
        m_rotationAvailable = serverConfirmed;
    }

    void AddPosition(float x, float y, float z) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_position.x += x;
        m_position.y += y;
        m_position.z += z;
        m_positionAvailable = false;
    }

    void AddHeadRotation(float pitch, float yaw) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_headRotation.x += pitch;
        m_headRotation.y += yaw;
        m_headYaw += yaw;
        m_rotationAvailable = false;
    }

    void AddInput(PlayerAuthInputData value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_inputData.push_back(value);
    }

    void RemoveInput(PlayerAuthInputData value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_inputData.erase(std::remove(m_inputData.begin(), m_inputData.end(), value), m_inputData.end());
    }

private:
    mutable std::mutex m_mutex;
    bool m_available = false;
    bool m_positionAvailable = false;
    bool m_rotationAvailable = false;
    int64_t m_entityRuntimeID = 0;
    Vec2 m_headRotation{};
    Vec3 m_position{};
    Vec2 m_moveVector{};
    float m_headYaw = 0;
    std::vector<PlayerAuthInputData> m_inputData;
    bool m_readyPosDeltaDirty = false;
};
