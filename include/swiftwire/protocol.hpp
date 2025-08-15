#pragma once
#include <cstdint>

namespace swiftwire::proto {
    // Message types
    inline constexpr uint8_t MSG_HELLO     = 0x01;
    inline constexpr uint8_t MSG_HELLO_ACK = 0x81;

    // Big-endian helpers
    inline void write_u32be(char* p, uint32_t v) {
        p[0] = static_cast<char>((v >> 24) & 0xFF);
        p[1] = static_cast<char>((v >> 16) & 0xFF);
        p[2] = static_cast<char>((v >> 8)  & 0xFF);
        p[3] = static_cast<char>( v        & 0xFF);
    }
    inline uint32_t read_u32be(const char* p) {
        return (uint32_t(uint8_t(p[0])) << 24) |
               (uint32_t(uint8_t(p[1])) << 16) |
               (uint32_t(uint8_t(p[2])) << 8)  |
               uint32_t(uint8_t(p[3]));
    }
    inline void write_u64be(char* p, uint64_t v) {
        for (int i = 7; i >= 0; --i) p[7 - i] = static_cast<char>((v >> (i * 8)) & 0xFF);
    }
    inline uint64_t read_u64be(const char* p) {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) v = (v << 8) | uint8_t(p[i]);
        return v;
    }
}
