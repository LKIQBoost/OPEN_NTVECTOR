#pragma once

#include <Python.h>
#include <cstdint>
#include <string>

PyObject* BuildStructuredProtocolEvent(uint32_t packetId, const std::string& payload);
PyObject* DecodeStructuredProtocolData(uint32_t packetId, const std::string& payload,
                                        std::string& error);
const char* GetProtocolPacketName(uint32_t packetId) noexcept;
