//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once


// helper class for reading wave header
template <bool LittleEndian =
#if defined(CR_ARCH_CPU_BIG_ENDIAN)
  false
#else
  true
#endif
>
class WaveHelper final {
public:
   struct Header {
      char riff[4];
      uint32_t chunkSize;
      char wave[4];
      char fmt[4];
      uint32_t subchunk1Size;
      uint16_t audioFormat;
      uint16_t numChannels;
      uint32_t sampleRate;
      uint32_t byteRate;
      uint16_t blockAlign;
      uint16_t bitsPerSample;
      char dataChunkId[4];
      uint32_t dataChunkLength;
   };

public:
   WaveHelper () = default;
   ~WaveHelper () = default;

public:
   template <typename U> U read16 (uint16_t value) {
      if constexpr (LittleEndian) {
         return static_cast <U> (value);
      }
      else {
         return static_cast <U> (static_cast <uint16_t> ((value >> 8) | (value << 8)));
      }
   }

   template <typename U> U read32 (uint32_t value) {
      if constexpr (LittleEndian) {
         return static_cast <U> (value);
      }
      else {
         return static_cast <U> ((((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) | ((value & 0x00ff0000) >> 8) | ((value & 0xff000000) >> 24)));
      }
   }

   bool isWave (char *format) const {
      if constexpr (LittleEndian) {
         return memcmp (format, "WAVE", 4) == 0;
      }
      else {
         return *reinterpret_cast <uint32_t *> (format) == 0x57415645;
      }
   }
};
