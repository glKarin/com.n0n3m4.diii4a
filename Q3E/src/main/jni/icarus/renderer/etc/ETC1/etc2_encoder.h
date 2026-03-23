#ifndef RTC2_ENCODER_H
#define RTC2_ENCODER_H

#include "etc_rg_etc1.h"

// Pack a 4x4 block of 32bpp BGRA pixels to a 16-byte RGBA8_ETC2_EAC block (supports alpha).
unsigned int etc2_rgba8_block_pack(unsigned char *etc2, const unsigned int* rgba, rg_etc1::etc1_pack_params params);

// Pack a 4x4 block of 32bpp BGRA pixels to a 8-byte RGB8_ETC2 block (opaque).
unsigned int etc2_rgb8_block_pack(unsigned char *etc2, const unsigned int* rgba, rg_etc1::etc1_pack_params params);

// ETC2 support: RGB8_ETC2
void rg_etc2_rgb8_decode_block(const unsigned char *etc_block, unsigned int* rgba);

// ETC2 support: RGBA8_ETC2_EAC
void rg_etc2_rgba8_decode_block(const unsigned char *etc_block, unsigned int* rgba);

#endif // RTC2_ENCODER_H

