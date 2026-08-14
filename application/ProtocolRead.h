#pragma once
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "CommandBlockUpdate.h"   // BlockPos
#include <string>

// Small wire-format read helpers shared by the receive-side packet parsers,
// matching the minecraft-data bedrock 1.21.120 definitions:
//   zigzag32 / zigzag64 -> signed varint with zigzag encoding
//   varint              -> plain unsigned LEB128
//   BlockCoordinates    -> x: zigzag32, y: varint, z: zigzag32
//
// NOTE: BinaryReader::ReadVarInt() accumulates the raw varint without
// un-zigzagging it, so signed (zigzag) fields are decoded here explicitly.
namespace pkt_io {

    static inline int32_t ReadZigZag32(BinaryReader& br) {
        uint64_t raw = br.ReadVarUInt();
        return static_cast<int32_t>((raw >> 1) ^ static_cast<uint64_t>(-static_cast<int64_t>(raw & 1)));
    }

    static inline int64_t ReadZigZag64(BinaryReader& br) {
        uint64_t raw = 0;
        size_t len = BinaryWriter::varint_to_uint(
            reinterpret_cast<const uint8_t*>(br.data() + br.m_pointer), &raw);
        br.m_pointer += len;
        return static_cast<int64_t>((raw >> 1) ^ static_cast<uint64_t>(-static_cast<int64_t>(raw & 1)));
    }

    // BlockCoordinates: x zigzag32, y varint (unsigned), z zigzag32
    static inline BlockPos ReadBlockPos(BinaryReader& br) {
        BlockPos p;
        p.x = ReadZigZag32(br);
        p.y = static_cast<int32_t>(br.ReadVarUInt());
        p.z = ReadZigZag32(br);
        return p;
    }

}
