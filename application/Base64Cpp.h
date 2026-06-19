
#ifndef LIBSUPERCPP4WIN_BASE64_H
#define LIBSUPERCPP4WIN_BASE64_H

#include <string>
#include <cstdint>
class Base64Cpp
{
private:
    inline static
        char const* get_alphabet();

    inline static
        signed char const* get_inverse();

    /// Returns max chars needed to encode a base64 string
    inline static
        std::size_t constexpr
        encoded_size(std::size_t n) { return 4 * ((n + 2) / 3); }

    /// Returns max bytes needed to decode a base64 string
    inline static
        std::size_t constexpr
        decoded_size(std::size_t n) { return n / 4 * 3; }

    /** Encode a series of octets as a padded, base64 string.

    The resulting string will not be null terminated.

    @par Requires

    The memory pointed to by `out` points to valid memory
    of at least `encoded_size(len)` bytes.

    @return The number of characters written to `out`. This
    will exclude any null termination.
*/
    static std::size_t
        encode(void* dest, void const* src, std::size_t len);

    /** Decode a padded base64 string into a series of octets.

    @par Requires

    The memory pointed to by `out` points to valid memory
    of at least `decoded_size(len)` bytes.

    @return The number of octets written to `out`, and
    the number of characters read from the input string,
    expressed as a pair.
*/
    static std::pair<std::size_t, std::size_t>
        decode(void* dest, char const* src, std::size_t len);

public:
    static std::string
        base64_encode(std::uint8_t const* data, std::size_t len);

    inline static
        std::string
        base64_encode(std::string const& s) {
        return base64_encode(reinterpret_cast <std::uint8_t const*> (s.data()), s.size());
    }

    static std::string
        base64_decode(std::string const& data);
};
#endif