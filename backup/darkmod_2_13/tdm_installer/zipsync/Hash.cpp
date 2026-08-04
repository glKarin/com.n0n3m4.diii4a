#include "Hash.h"
#include <stdio.h>
#include <string.h>
#include "Logging.h"


namespace ZipSync {

bool HashDigest::operator< (const HashDigest &other) const {
    return memcmp(_data, other._data, sizeof(_data)) < 0;
}
bool HashDigest::operator== (const HashDigest &other) const {
    return memcmp(_data, other._data, sizeof(_data)) == 0;
}
std::string HashDigest::Hex() const {
    char text[100];
    for (int i = 0; i < sizeof(_data); i++)
        sprintf(text + 2*i, "%02x", _data[i]);
    return text;
}
void HashDigest::Parse(const char *hex) {
    ZipSyncAssertF(strlen(hex) == 2 * sizeof(_data), "Hex digest has wrong length %d", strlen(hex));
    for (int i = 0; i < sizeof(_data); i++) {
        /*char octet[4] = {0};
        memcpy(octet, hex + 2*i, 2);
        uint32_t value;
        int k = sscanf(octet, "%02x", &value);
        ZipSyncAssertF(k == 1, "Cannot parse hex digest byte %s", octet);
        _data[i] = value;*/

        uint32_t digits[2];
        for (int d = 0; d < 2; d++) {
            char ch = hex[2*i+d];
            if (ch >= '0' && ch <= '9')
                digits[d] = ch - '0';
            else if (ch >= 'a' && ch <= 'f')
                digits[d] = ch - 'a' + 10;
            else
                ZipSyncAssertF(false, "Cannot parse hex digest digit %c", ch);
        }
        _data[i] = (digits[0] << 4) + digits[1];
    }
}
void HashDigest::Clear() {
    memset(_data, 0, sizeof(_data));
}

Hasher::Hasher() {
    blake2s_init(&_state, sizeof(HashDigest::_data));
}
Hasher& Hasher::Update(const void *in, size_t inlen) {
    blake2s_update(&_state, (const uint8_t *)in, inlen);
    return *this;
}
HashDigest Hasher::Finalize() {
    HashDigest res;
    blake2s_final(&_state, res._data, sizeof(res._data));
    return res;
}

}
