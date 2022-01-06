
#ifndef ILOGGER_HASHMACRO_H
#define ILOGGER_HASHMACRO_H

#include <cstdint>
#include <cstring>

// From https://gist.github.com/Lee-R/3839813 , Public Domain from Lee-R: "Consider it public domain. "
namespace compiletime_string_hash
{
    // FNV-1a 32bit hashing algorithm.
    constexpr std::uint32_t fnv1a_32(char const* s, std::size_t count)
    {
        return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
    }
}

constexpr std::uint32_t LOG_SA_HASH(const char* string) {
    return compiletime_string_hash::fnv1a_32(string, std::strlen(string));
}

static_assert(LOG_SA_HASH("0123456789ABCDEF") == 141695047u);

#endif // ILOGGER_HASHMACRO_H
