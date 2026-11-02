// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../neo/framework/async/RetailHuffman.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
    using namespace etqwRetail;
    // Captured from the original 1.5 dedicated server on loopback.
    const uint8_t packet[] = {0xc9,0xff,0x2a,0xff,0xb7,0x74,0xd2,0xbc,0x79,0x98,0x2e,0x00};
    const uint8_t expected[] = {1,0,0,0,0x14,0x28,0,0,0x94,0x2a,5,0,4,0,0x80,0x20,0x76};
    uint8_t decoded[32768], encoded[65536], input[32768];
    size_t used = 0, written = 0;
    assert(CommandCodec().Decode(packet, sizeof(packet), decoded, sizeof(expected), used));
    assert(used == sizeof(packet) && !std::memcmp(decoded, expected, sizeof(expected)));
    assert(CommandCodec().Encode(expected, sizeof(expected), encoded, sizeof(encoded), written));
    assert(written == sizeof(packet) && !std::memcmp(encoded, packet, written));
    for (size_t cut = 0; cut < sizeof(packet); ++cut)
        assert(!CommandCodec().Decode(packet, cut, decoded, sizeof(expected), used));
    const uint8_t zero[] = {0};
    assert(ReliableCodec().Decode(zero, 1, decoded, 4, used));
    for (int i = 0; i < 4; ++i) assert(decoded[i] == 0);
    for (size_t i = 0; i < sizeof(input); ++i) input[i] = (i * 179 + i / 256) & 255;
    for (const Huffman* codec : {&ReliableCodec(), &EntityCodec(), &CommandCodec()}) {
        assert(codec->Encode(input, sizeof(input), encoded, sizeof(encoded), written));
        assert(codec->Decode(encoded, written, decoded, sizeof(input), used));
        assert(used == written && !std::memcmp(input, decoded, sizeof(input)));
        assert(!codec->Encode(input, sizeof(input), encoded, 1, written));
    }
    std::puts("Retail Huffman: captured packet, all symbols, capacity and truncation checks passed");
}
