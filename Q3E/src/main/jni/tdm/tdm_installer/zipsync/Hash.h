#pragma once

#include <stdint.h>
#include <string>

#pragma warning(push)
#pragma warning(disable:4804)  //warning C4804: '/': unsafe use of type 'bool' in operation
#include <blake2.h>
#pragma warning(pop)


namespace ZipSync {

/**
 * The hash digest used for all files.
 * If two files have same hash value, then they are considered equal (no check required).
 * Thus, a reliable cryptographic hash must be used.
 */
class HashDigest {
    //256-bit hash (see Hasher below)
    uint8_t _data[32];

    friend class Hasher;
public:
    bool operator< (const HashDigest &other) const;
    bool operator== (const HashDigest &other) const;
    std::string Hex() const;
    void Parse(const char *hex);
    void Clear();
};

/**
 * Wrapper around the chosen hash function.
 * Currently it is BLAKE2s (TODO: use [p]arallel flavor?)
 */
class Hasher {
    blake2s_state _state;
public:
    Hasher();
    Hasher& Update(const void *in, size_t inlen);
    HashDigest Finalize();
};

}
