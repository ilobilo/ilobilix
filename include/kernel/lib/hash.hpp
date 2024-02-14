// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <cstring>
#include <cstdint>

constexpr __int128_t MurmurHash3_128(const void *key, uint64_t len, uint64_t seed, void *out = nullptr)
{
    auto rotl64 = [](uint64_t x, int8_t r)
    {
        return (x << r) | (x >> (64 - r));
    };

    auto fmix64 = [](uint64_t k)
    {
        k ^= k >> 33;
        k *= 0xFF51AFD7ED558CCDLLU;
        k ^= k >> 33;
        k *= 0xC4CEB9FE1A85EC53LLU;
        k ^= k >> 33;

        return k;
    };

    auto data = static_cast<const uint8_t*>(key);
    const uint64_t nblocks = len / 16;

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const uint64_t c1 = 0x87C37B91114253D5LLU;
    const uint64_t c2 = 0x4CF5AD432745937FLLU;

    auto blocks = static_cast<const uint64_t*>(key);

    for (uint64_t i = 0; i < nblocks; i++)
    {
        uint64_t k1 = blocks[i * 2 + 0];
        uint64_t k2 = blocks[i * 2 + 1];

        k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
        h1 = rotl64(h1, 27); h1 += h2; h1 = h1 * 5 + 0x52DCE729;
        k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;
        h2 = rotl64(h2, 31); h2 += h1; h2 = h2 * 5 + 0x38495AB5;
    }

    auto tail = static_cast<const uint8_t*>(static_cast<const void*>(data + nblocks * 16));

    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (len & 15)
    {
        case 15: k2 ^= (static_cast<uint64_t>(tail[14])) << 48;
        case 14: k2 ^= (static_cast<uint64_t>(tail[13])) << 40;
        case 13: k2 ^= (static_cast<uint64_t>(tail[12])) << 32;
        case 12: k2 ^= (static_cast<uint64_t>(tail[11])) << 24;
        case 11: k2 ^= (static_cast<uint64_t>(tail[10])) << 16;
        case 10: k2 ^= (static_cast<uint64_t>(tail[9])) << 8;
        case 9: k2 ^= (static_cast<uint64_t>(tail[8])) << 0;
            k2 *= c2; k2  = rotl64(k2, 33); k2 *= c1; h2 ^= k2;
        case 8: k1 ^= (static_cast<uint64_t>(tail[7])) << 56;
        case 7: k1 ^= (static_cast<uint64_t>(tail[6])) << 48;
        case 6: k1 ^= (static_cast<uint64_t>(tail[5])) << 40;
        case 5: k1 ^= (static_cast<uint64_t>(tail[4])) << 32;
        case 4: k1 ^= (static_cast<uint64_t>(tail[3])) << 24;
        case 3: k1 ^= (static_cast<uint64_t>(tail[2])) << 16;
        case 2: k1 ^= (static_cast<uint64_t>(tail[1])) << 8;
        case 1: k1 ^= (static_cast<uint64_t>(tail[0])) << 0;
            k1 *= c1; k1  = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
    };

    h1 ^= len; h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    if (out != nullptr)
    {
        static_cast<uint64_t*>(out)[0] = h1;
        static_cast<uint64_t*>(out)[1] = h2;
    }

    return (static_cast<__int128>(h1) << 64) | static_cast<__int128>(h2);
}

constexpr uint64_t MurmurHash3_64(const void *key, uint64_t len, uint64_t seed)
{
    auto val = MurmurHash3_128(key, len, seed);
    return static_cast<uint64_t>(val) ^ static_cast<uint64_t>(val >> 64);
}

constexpr uint64_t MurmurHash2_64A(const void *key, uint64_t len, uint64_t seed)
{
    const uint64_t m = 0xC6A4A7935BD1E995;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t *data = static_cast<const uint64_t*>(key);
    const uint64_t *end = data + (len / 8);

    while (data != end)
    {
        uint64_t k = 0;
        memcpy(&k, data++, sizeof(uint64_t));

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    auto data2 = static_cast<const uint8_t*>(static_cast<const void*>(data));

    switch (len & 7)
    {
        case 7: h ^= static_cast<uint64_t>(data2[6]) << 48;
        case 6: h ^= static_cast<uint64_t>(data2[5]) << 40;
        case 5: h ^= static_cast<uint64_t>(data2[4]) << 32;
        case 4: h ^= static_cast<uint64_t>(data2[3]) << 24;
        case 3: h ^= static_cast<uint64_t>(data2[2]) << 16;
        case 2: h ^= static_cast<uint64_t>(data2[1]) << 8;
        case 1: h ^= static_cast<uint64_t>(data2[0]);
            h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

constexpr uint64_t FNV1a(const void *key, uint64_t len, uint64_t seed = 0xCBF29CE484222325)
{
    const uint64_t prime = 0x00000100000001B3;
    uint64_t h = seed;

    const uint8_t *data = static_cast<const uint8_t*>(key);

    for (uint64_t i = 0; i < len; i++)
    {
        h ^= data[i];
        h *= prime;
    }

    return h;
}

inline constexpr size_t _hash(const void *ptr, uint64_t len, uint64_t seed = 0xC70F6907UL)
{
    return MurmurHash2_64A(ptr, len, seed);
}

inline constexpr size_t _hash(const auto &val)
{
    return _hash(&val, sizeof(val));
}

inline constexpr size_t _hash_combine(const auto &val, uint64_t old)
{
    return _hash(&val, sizeof(val), old);
}