//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/array.h>

CR_NAMESPACE_BEGIN

// see https://github.com/encode84/ulz/
class ULZ final : public Singleton <ULZ> {
public:
   enum : int32_t {
      Excess = 16,
      UncompressFailure = -1
   };

private:
   enum : int32_t {
      WindowBits = 17,
      WindowSize = cr::bit (WindowBits),
      WindowMask = WindowSize - 1,

      MinMatch = 4,
      MaxChain = cr::bit (5),

      HashBits = 19,
      HashLength = cr::bit (HashBits),
      EmptyHash = -1,
   };


private:
   SmallArray <int32_t> hashTable_ {};
   SmallArray <int32_t> prevTable_ {};

public:
   explicit ULZ () {
      hashTable_.resize (HashLength);
      prevTable_.resize (WindowSize);
   }

   ~ULZ () = default;

public:
   int32_t compress (uint8_t *in, int32_t inputLength, uint8_t *out) {
      for (auto &htb : hashTable_) {
         htb = EmptyHash;
      }
      auto op = out;

      int32_t anchor = 0;
      int32_t cur = 0;

      while (cur < inputLength) {
         const int32_t maxMatch = inputLength - cur;

         int32_t bestLength = 0;
         int32_t dist = 0;

         if (maxMatch >= MinMatch) {
            const auto limit = cr::max <int32_t> (cur - WindowSize, EmptyHash);

            int32_t chainLength = MaxChain;
            int32_t lookup = hashTable_[hash32 (&in[cur])];

            while (lookup > limit) {
               if (in[lookup + bestLength] == in[cur + bestLength] && load <uint32_t> (&in[lookup]) == load <uint32_t> (&in[cur])) {
                  int32_t length = MinMatch;

                  while (length < maxMatch && in[lookup + length] == in[cur + length]) {
                     ++length;
                  }

                  if (length > bestLength) {
                     bestLength = length;
                     dist = cur - lookup;

                     if (length == maxMatch) {
                        break;
                     }
                  }
               }

               if (--chainLength == 0) {
                  break;
               }
               lookup = prevTable_[lookup & WindowMask];
            }
         }

         if (bestLength == MinMatch && (cur - anchor) >= (7 + 128)) {
            bestLength = 0;
         }

         if (bestLength >= MinMatch && bestLength < maxMatch && (cur - anchor) != 6) {
            const auto next = cur + 1;
            const auto target = bestLength + 1;
            const auto limit = cr::max <int32_t> (next - WindowSize, EmptyHash);

            int32_t chainLength = MaxChain;
            int32_t lookup = hashTable_[hash32 (&in[next])];

            while (lookup > limit) {
               if (in[lookup + bestLength] == in[next + bestLength] && load <uint32_t> (&in[lookup]) == load <uint32_t> (&in[next])) {
                  int32_t length = MinMatch;

                  while (length < target && in[lookup + length] == in[next + length]) {
                     ++length;
                  }

                  if (length == target) {
                     bestLength = 0;
                     break;
                  }
               }

               if (--chainLength == 0) {
                  break;
               }
               lookup = prevTable_[lookup & WindowMask];
            }
         }

         if (bestLength >= MinMatch) {
            const auto length = bestLength - MinMatch;
            const auto token = ((dist >> 12) & 16) + cr::min <int32_t> (length, 15);

            if (anchor != cur) {
               const auto run = cur - anchor;

               if (run >= 7) {
                  add (op, (7 << 5) + token);
                  encode (op, run - 7);
               }
               else {
                  add (op, (run << 5) + token);
               }
               copy (op, &in[anchor], run);
               op += run;
            }
            else {
               add (op, token);
            }

            if (length >= 15) {
               encode (op, length - 15);
            }
            store16 (op, static_cast <uint16_t> (dist));
            op += 2;

            while (bestLength-- != 0) {
               const auto hash = hash32 (&in[cur]);

               prevTable_[cur & WindowMask] = hashTable_[hash];
               hashTable_[hash] = cur++;
            }
            anchor = cur;
         }
         else {
            const auto hash = hash32 (&in[cur]);

            prevTable_[cur & WindowMask] = hashTable_[hash];
            hashTable_[hash] = cur++;
         }
      }

      if (anchor != cur) {
         const auto run = cur - anchor;

         if (run >= 7) {
            add (op, 7 << 5);
            encode (op, run - 7);
         }
         else {
            add (op, run << 5);
         }
         copy (op, &in[anchor], run);
         op += run;
      }
      return static_cast <int32_t> (op - out);
   }

   int32_t uncompress (uint8_t *in, int32_t inputLength, uint8_t *out, int32_t outLength) {
      auto op = out;
      auto ip = in;

      const auto opEnd = op + outLength;
      const auto ipEnd = ip + inputLength;

      while (ip < ipEnd) {
         const auto token = *ip++;

         if (token >= 32) {
            auto run = token >> 5;

            if (run == 7) {
               run += decode (ip);
            }

            if ((opEnd - op) < run || (ipEnd - ip) < run) {
               return UncompressFailure;
            }
            copy (op, ip, run);

            op += run;
            ip += run;

            if (ip >= ipEnd) {
               break;
            }
         }
         auto length = (token & 15) + MinMatch;

         if (length == (15 + MinMatch)) {
            length += decode (ip);
         }

         if ((opEnd - op) < length) {
            return UncompressFailure;
         }
         const auto dist = ((token & 16) << 12) + load <uint16_t> (ip);
         ip += 2;

         auto cp = op - dist;

         if ((op - out) < dist) {
            return UncompressFailure;
         }

         if (dist >= 8) {
            copy (op, cp, length);
            op += length;
         }
         else {
            for (int32_t i = 0; i < 4; ++i) {
               *op++ = *cp++;
            }

            while (length-- != 4) {
               *op++ = *cp++;
            }
         }
      }
      return static_cast <int32_t> (ip == ipEnd) ? static_cast <int32_t> (op - out) : UncompressFailure;
   }

private:
   template <typename U> U load (void *ptr) {
      U ret;
      memcpy (&ret, ptr, sizeof (U));

      return ret;
   }

   void store16 (void *ptr, uint16_t val) {
      memcpy (ptr, &val, sizeof (uint16_t));
   }

   void copy64 (void *dst, void *src) {
      memcpy (dst, src, sizeof (uint64_t));
   }

   uint32_t hash32 (void *ptr) {
      return (load <uint32_t> (ptr) * 0x9e3779b9) >> (32 - HashBits);
   }

   void copy (uint8_t *dst, uint8_t *src, int32_t count) {
      copy64 (dst, src);

      for (int32_t i = 8; i < count; i += 8) {
         copy64 (dst + i, src + i);
      }
   }

   void add (uint8_t *&dst, int32_t val) {
      *dst++ = static_cast <uint8_t> (val);
   }

   void encode (uint8_t *&ptr, uint32_t val) {
      while (val >= 128) {
         val -= 128;

         *ptr++ = 128 + (val & 127);
         val >>= 7;
      }
      *ptr++ = static_cast <uint8_t> (val);
   }

   uint32_t decode (uint8_t *&ptr) {
      uint32_t val = 0;

      for (int32_t i = 0; i <= 21; i += 7) {
         const uint32_t cur = *ptr++;
         val += cur << i;

         if (cur < 128) {
            break;
         }
      }
      return val;
   }
};

CR_NAMESPACE_END
