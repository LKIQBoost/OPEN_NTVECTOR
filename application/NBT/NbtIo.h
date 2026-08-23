#pragma once

#include "Tag.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nbt {

enum class Encoding {
    BigEndian,
    LittleEndian,
    NetworkLittleEndian,
};

enum class ParseErrorCode {
    None,
    UnexpectedEnd,
    InvalidType,
    InvalidLength,
    InvalidListType,
    DepthLimit,
    ElementLimit,
    StringLimit,
    TrailingData,
};

struct ParseLimits {
    size_t maxDepth = 64;
    size_t maxElements = 1 << 20;
    size_t maxCollectionLength = 1 << 20;
    size_t maxStringBytes = 1 << 20;
};

struct ParseOptions {
    Encoding encoding = Encoding::NetworkLittleEndian;
    ParseLimits limits;
    bool allowTrailingData = false;
};

struct ParseError {
    ParseErrorCode code = ParseErrorCode::None;
    size_t offset = 0;
    std::string message;
};

struct ParseResult {
    bool success = false;
    Tag root;
    std::string rootName;
    size_t bytesRead = 0;
    ParseError error;

    explicit operator bool() const noexcept { return success; }
};

struct WriteResult {
    bool success = false;
    std::vector<uint8_t> data;
    std::string error;

    explicit operator bool() const noexcept { return success; }
};

ParseResult Parse(const uint8_t* data, size_t size, const ParseOptions& options = {});
ParseResult Parse(const std::vector<uint8_t>& data, const ParseOptions& options = {});
WriteResult Write(const Tag& root, const std::string& rootName = {},
                  Encoding encoding = Encoding::NetworkLittleEndian);

} // namespace nbt
