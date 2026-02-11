/* sha512.c - SHA384 and SHA512 hash functions
 * Copyright (C) 2003, 2008, 2009 Free Software Foundation, Inc.
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser general Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


/*  Test vectors from FIPS-180-2:
 *
 *  "abc"
 * 384:
 *  CB00753F 45A35E8B B5A03D69 9AC65007 272C32AB 0EDED163
 *  1A8B605A 43FF5BED 8086072B A1E7CC23 58BAECA1 34C825A7
 * 512:
 *  DDAF35A1 93617ABA CC417349 AE204131 12E6FA4E 89A97EA2 0A9EEEE6 4B55D39A
 *  2192992A 274FC1A8 36BA3C23 A3FEEBBD 454D4423 643CE80E 2A9AC94F A54CA49F
 *
 *  "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
 * 384:
 *  09330C33 F71147E8 3D192FC7 82CD1B47 53111B17 3B3B05D2
 *  2FA08086 E3B0F712 FCC7C71A 557E2DB9 66C3E9FA 91746039
 * 512:
 *  8E959B75 DAE313DA 8CF4F728 14FC143F 8F7779C6 EB9F7FA1 7299AEAD B6889018
 *  501D289E 4900F7E4 331B99DE C4B5433A C7D329EE B6DD2654 5E96E55B 874BE909
 *
 *  "a" x 1000000
 * 384:
 *  9D0E1809 716474CB 086E834E 310A4A1C ED149E9C 00F24852
 *  7972CEC5 704C2A5B 07B8B3DC 38ECC4EB AE97DDD8 7F3D8985
 * 512:
 *  E718483D 0CE76964 4E2E42C7 BC15B463 8E1F98B1 3B204428 5632A803 AFA973EB
 *  DE0FF244 877EA60A 4CB0432C E577C31B EB009C5C 2C49AA2E 4EADB217 AD8CC09B
 */

#include "quakedef.h"

#ifndef SHA2
#define SHA2 256
#include "sha2.c"
#undef SHA2
#define SHA2 512
#endif

#undef U64_C
#undef U64_C_LOW
#undef u64
#undef ROUNDS
#undef SHA2_CONTEXT
#undef sha2trunc_init
#undef sha2_init
#if SHA2==256
	#define U64_C(n) (n##ull>>32)
	#define U64_C_LOW(n) (u64)(n##ull)
	#define u64 quint32_t
	#define ROUNDS 64
	#define SHA2_CONTEXT SHA256_CONTEXT
	#define sha2trunc_init sha224_init
	#define sha2_init sha256_init
#else
	#define U64_C(n) n##ull
	#define U64_C_LOW(n) n##ull
	#define u64 quint64_t
	#define ROUNDS 80
	#define SHA2_CONTEXT SHA512_CONTEXT
	#define ROTR ROTR64
	#define Ch Ch64
	#define Maj Maj64
	#define Sum0 Sum0_64
	#define Sum1 Sum1_64
	#define transform transform_64
	#define sha2trunc_init sha384_init
	#define sha2_init sha512_init
	#define sha2_write sha512_write
	#define sha2_final sha512_final
#endif
#define BLOCKBYTES (16*sizeof(u64))
#define byte qbyte

typedef struct
{
	  u64 h0, h1, h2, h3, h4, h5, h6, h7;
	  u64 nblocks;
	  byte buf[BLOCKBYTES];
	  int count;
} SHA2_CONTEXT;

static void
sha2_init (void *context)
{
  SHA2_CONTEXT *hd = context;

  hd->h0 = U64_C(0x6a09e667f3bcc908);
  hd->h1 = U64_C(0xbb67ae8584caa73b);
  hd->h2 = U64_C(0x3c6ef372fe94f82b);
  hd->h3 = U64_C(0xa54ff53a5f1d36f1);
  hd->h4 = U64_C(0x510e527fade682d1);
  hd->h5 = U64_C(0x9b05688c2b3e6c1f);
  hd->h6 = U64_C(0x1f83d9abfb41bd6b);
  hd->h7 = U64_C(0x5be0cd19137e2179);

  hd->nblocks = 0;
  hd->count = 0;
}
static void sha2trunc_init (void *context)
{
  SHA2_CONTEXT *hd = context;

  //sha224 uses only the low parts.
  hd->h0 = U64_C_LOW(0xcbbb9d5dc1059ed8);
  hd->h1 = U64_C_LOW(0x629a292a367cd507);
  hd->h2 = U64_C_LOW(0x9159015a3070dd17);
  hd->h3 = U64_C_LOW(0x152fecd8f70e5939);
  hd->h4 = U64_C_LOW(0x67332667ffc00b31);
  hd->h5 = U64_C_LOW(0x8eb44a8768581511);
  hd->h6 = U64_C_LOW(0xdb0c2e0d64f98fa7);
  hd->h7 = U64_C_LOW(0x47b5481dbefa4fa4);

  hd->nblocks = 0;
  hd->count = 0;
}

fte_inlinestatic u64
ROTR (u64 x, u64 n)
{
  return ((x >> n) | (x << (sizeof(u64)*8 - n)));
}

fte_inlinestatic u64
Ch (u64 x, u64 y, u64 z)
{
  return ((x & y) ^ ( ~x & z));
}

fte_inlinestatic u64
Maj (u64 x, u64 y, u64 z)
{
  return ((x & y) ^ (x & z) ^ (y & z));
}

#undef S0
#undef S1
#if SHA2==256
#define S0(x) (ROTR((x),7) ^ ROTR((x),18) ^ ((x)>>3))
#define S1(x) (ROTR((x),17) ^ ROTR((x),19) ^ ((x)>>10))

fte_inlinestatic u64
Sum0 (u64 x)
{
  return (ROTR (x, 2) ^ ROTR (x, 13) ^ ROTR (x, 22));
}

fte_inlinestatic u64
Sum1 (u64 x)
{
  return (ROTR (x, 6) ^ ROTR (x, 11) ^ ROTR (x, 25));
}
#else
#define S0(x) (ROTR((x),1) ^ ROTR((x),8) ^ ((x)>>7))
#define S1(x) (ROTR((x),19) ^ ROTR((x),61) ^ ((x)>>6))

fte_inlinestatic u64
Sum0 (u64 x)
{
  return (ROTR (x, 28) ^ ROTR (x, 34) ^ ROTR (x, 39));
}

fte_inlinestatic u64
Sum1 (u64 x)
{
  return (ROTR (x, 14) ^ ROTR (x, 18) ^ ROTR (x, 41));
}
#endif

/****************
 * Transform the message W which consists of 16 64-bit-words
 */
static void
transform (SHA2_CONTEXT *hd, const unsigned char *data)
{
  u64 a, b, c, d, e, f, g, h;
  u64 w[ROUNDS];
  int t;
  static const u64 k[] =
    {
      U64_C(0x428a2f98d728ae22), U64_C(0x7137449123ef65cd),
      U64_C(0xb5c0fbcfec4d3b2f), U64_C(0xe9b5dba58189dbbc),
      U64_C(0x3956c25bf348b538), U64_C(0x59f111f1b605d019),
      U64_C(0x923f82a4af194f9b), U64_C(0xab1c5ed5da6d8118),
      U64_C(0xd807aa98a3030242), U64_C(0x12835b0145706fbe),
      U64_C(0x243185be4ee4b28c), U64_C(0x550c7dc3d5ffb4e2),
      U64_C(0x72be5d74f27b896f), U64_C(0x80deb1fe3b1696b1),
      U64_C(0x9bdc06a725c71235), U64_C(0xc19bf174cf692694),
      U64_C(0xe49b69c19ef14ad2), U64_C(0xefbe4786384f25e3),
      U64_C(0x0fc19dc68b8cd5b5), U64_C(0x240ca1cc77ac9c65),
      U64_C(0x2de92c6f592b0275), U64_C(0x4a7484aa6ea6e483),
      U64_C(0x5cb0a9dcbd41fbd4), U64_C(0x76f988da831153b5),
      U64_C(0x983e5152ee66dfab), U64_C(0xa831c66d2db43210),
      U64_C(0xb00327c898fb213f), U64_C(0xbf597fc7beef0ee4),
      U64_C(0xc6e00bf33da88fc2), U64_C(0xd5a79147930aa725),
      U64_C(0x06ca6351e003826f), U64_C(0x142929670a0e6e70),
      U64_C(0x27b70a8546d22ffc), U64_C(0x2e1b21385c26c926),
      U64_C(0x4d2c6dfc5ac42aed), U64_C(0x53380d139d95b3df),
      U64_C(0x650a73548baf63de), U64_C(0x766a0abb3c77b2a8),
      U64_C(0x81c2c92e47edaee6), U64_C(0x92722c851482353b),
      U64_C(0xa2bfe8a14cf10364), U64_C(0xa81a664bbc423001),
      U64_C(0xc24b8b70d0f89791), U64_C(0xc76c51a30654be30),
      U64_C(0xd192e819d6ef5218), U64_C(0xd69906245565a910),
      U64_C(0xf40e35855771202a), U64_C(0x106aa07032bbd1b8),
      U64_C(0x19a4c116b8d2d0c8), U64_C(0x1e376c085141ab53),
      U64_C(0x2748774cdf8eeb99), U64_C(0x34b0bcb5e19b48a8),
      U64_C(0x391c0cb3c5c95a63), U64_C(0x4ed8aa4ae3418acb),
      U64_C(0x5b9cca4f7763e373), U64_C(0x682e6ff3d6b2b8a3),
      U64_C(0x748f82ee5defb2fc), U64_C(0x78a5636f43172f60),
      U64_C(0x84c87814a1f0ab72), U64_C(0x8cc702081a6439ec),
      U64_C(0x90befffa23631e28), U64_C(0xa4506cebde82bde9),
      U64_C(0xbef9a3f7b2c67915), U64_C(0xc67178f2e372532b),
      U64_C(0xca273eceea26619c), U64_C(0xd186b8c721c0c207),
      U64_C(0xeada7dd6cde0eb1e), U64_C(0xf57d4f7fee6ed178),
      U64_C(0x06f067aa72176fba), U64_C(0x0a637dc5a2c898a6),
      U64_C(0x113f9804bef90dae), U64_C(0x1b710b35131c471b),
      U64_C(0x28db77f523047d84), U64_C(0x32caab7b40c72493),
      U64_C(0x3c9ebe0a15c9bebc), U64_C(0x431d67c49c100d4c),
      U64_C(0x4cc5d4becb3e42b6), U64_C(0x597f299cfc657e2a),
      U64_C(0x5fcb6fab3ad6faec), U64_C(0x6c44198c4a475817)
    };

  /* get values from the chaining vars */
  a = hd->h0;
  b = hd->h1;
  c = hd->h2;
  d = hd->h3;
  e = hd->h4;
  f = hd->h5;
  g = hd->h6;
  h = hd->h7;

#ifdef WORDS_BIGENDIAN
  memcpy (w, data, BLOCKBYTES);
#else
  {
    int i;
    byte *p2;

    for (i = 0, p2 = (byte *) w; i < 16; i++, p2 += sizeof(*w))
      {
#if SHA2==512
	p2[7] = *data++;
	p2[6] = *data++;
	p2[5] = *data++;
	p2[4] = *data++;
#endif
	p2[3] = *data++;
	p2[2] = *data++;
	p2[1] = *data++;
	p2[0] = *data++;
      }
  }
#endif

  for (t = 16; t < ROUNDS; t++)
    w[t] = S1 (w[t - 2]) + w[t - 7] + S0 (w[t - 15]) + w[t - 16];


  for (t = 0; t < ROUNDS; )
    {
      u64 t1, t2;

      /* Performance on a AMD Athlon(tm) Dual Core Processor 4050e
         with gcc 4.3.3 using gcry_md_hash_buffer of each 10000 bytes
         initialized to 0,1,2,3...255,0,... and 1000 iterations:
         Not unrolled with macros:  440ms
         Unrolled with macros:      350ms
         Unrolled with inline:      330ms
      */
#if 0 /* Not unrolled.  */
      t1 = h + Sum1 (e) + Ch (e, f, g) + k[t] + w[t];
      t2 = Sum0 (a) + Maj (a, b, c);
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
      t++;
#else /* Unrolled to interweave the chain variables.  */
      t1 = h + Sum1 (e) + Ch (e, f, g) + k[t] + w[t];
      t2 = Sum0 (a) + Maj (a, b, c);
      d += t1;
      h  = t1 + t2;

      t1 = g + Sum1 (d) + Ch (d, e, f) + k[t+1] + w[t+1];
      t2 = Sum0 (h) + Maj (h, a, b);
      c += t1;
      g  = t1 + t2;

      t1 = f + Sum1 (c) + Ch (c, d, e) + k[t+2] + w[t+2];
      t2 = Sum0 (g) + Maj (g, h, a);
      b += t1;
      f  = t1 + t2;

      t1 = e + Sum1 (b) + Ch (b, c, d) + k[t+3] + w[t+3];
      t2 = Sum0 (f) + Maj (f, g, h);
      a += t1;
      e  = t1 + t2;

      t1 = d + Sum1 (a) + Ch (a, b, c) + k[t+4] + w[t+4];
      t2 = Sum0 (e) + Maj (e, f, g);
      h += t1;
      d  = t1 + t2;

      t1 = c + Sum1 (h) + Ch (h, a, b) + k[t+5] + w[t+5];
      t2 = Sum0 (d) + Maj (d, e, f);
      g += t1;
      c  = t1 + t2;

      t1 = b + Sum1 (g) + Ch (g, h, a) + k[t+6] + w[t+6];
      t2 = Sum0 (c) + Maj (c, d, e);
      f += t1;
      b  = t1 + t2;

      t1 = a + Sum1 (f) + Ch (f, g, h) + k[t+7] + w[t+7];
      t2 = Sum0 (b) + Maj (b, c, d);
      e += t1;
      a  = t1 + t2;

      t += 8;
#endif
    }

  /* Update chaining vars.  */
  hd->h0 += a;
  hd->h1 += b;
  hd->h2 += c;
  hd->h3 += d;
  hd->h4 += e;
  hd->h5 += f;
  hd->h6 += g;
  hd->h7 += h;
}


/* Update the message digest with the contents
 * of INBUF with length INLEN.
 */
static void
sha2_write (void *context, const void *inbuf_arg, size_t inlen)
{
  const unsigned char *inbuf = inbuf_arg;
  SHA2_CONTEXT *hd = context;

  if (hd->count == BLOCKBYTES)
    {				/* flush the buffer */
      transform (hd, hd->buf);
      hd->count = 0;
      hd->nblocks++;
    }
  if (!inbuf)
    return;
  if (hd->count)
    {
      for (; inlen && hd->count < BLOCKBYTES; inlen--)
	    hd->buf[hd->count++] = *inbuf++;
      sha2_write (context, NULL, 0);
      if (!inlen)
	    return;
    }

  while (inlen >= BLOCKBYTES)
    {
      transform (hd, inbuf);
      hd->count = 0;
      hd->nblocks++;
      inlen -= BLOCKBYTES;
      inbuf += BLOCKBYTES;
    }
  for (; inlen && hd->count < BLOCKBYTES; inlen--)
    hd->buf[hd->count++] = *inbuf++;
}


/* The routine final terminates the computation and
 * returns the digest.
 * The handle is prepared for a new cycle, but adding bytes to the
 * handle will the destroy the returned buffer.
 * Returns: 64 bytes representing the digest.  When used for sha384,
 * we take the leftmost 48 of those bytes.
 */

static void
sha2_final (void *context)
{
  SHA2_CONTEXT *hd = context;
  u64 t, msb, lsb;
  byte *p;

  sha2_write (context, NULL, 0); /* flush */ ;

  t = hd->nblocks;
  /* multiply by 128 to make a byte count */
  lsb = t * BLOCKBYTES;
  msb = t >> (sizeof(u64)*8-((BLOCKBYTES==128)?7:6));
  /* add the count */
  t = lsb;
  if ((lsb += hd->count) < t)
    msb++;
  /* multiply by 8 to make a bit count */
  t = lsb;
  lsb <<= 3;
  msb <<= 3;
  msb |= t >> (sizeof(u64)*8-3);

  if (hd->count < BLOCKBYTES-sizeof(u64)*2)
    {				/* enough room */
      hd->buf[hd->count++] = 0x80;	/* pad */
      while (hd->count < BLOCKBYTES-sizeof(u64)*2)
	    hd->buf[hd->count++] = 0;	/* pad */
    }
  else
    {				/* need one extra block */
      hd->buf[hd->count++] = 0x80;	/* pad character */
      while (hd->count < BLOCKBYTES)
        hd->buf[hd->count++] = 0;
      sha2_write (context, NULL, 0); /* flush */ ;
      memset (hd->buf, 0, BLOCKBYTES-sizeof(u64)*2);	/* fill next block with zeroes */
    }

#if SHA2==256
#define X(a) do { *p++ = hd->h##a >> 24; *p++ = hd->h##a >> 16;	      \
                  *p++ = hd->h##a >> 8;  *p++ = hd->h##a; } while (0)

  /* append the 128 bit count */
  hd->buf[56] = msb >> 24;
  hd->buf[57] = msb >> 16;
  hd->buf[58] = msb >> 8;
  hd->buf[59] = msb;

  hd->buf[60] = lsb >> 24;
  hd->buf[61] = lsb >> 16;
  hd->buf[62] = lsb >> 8;
  hd->buf[63] = lsb;
#else
#define X(a) do { *p++ = hd->h##a >> 56; *p++ = hd->h##a >> 48;	      \
                  *p++ = hd->h##a >> 40; *p++ = hd->h##a >> 32;	      \
                  *p++ = hd->h##a >> 24; *p++ = hd->h##a >> 16;	      \
                  *p++ = hd->h##a >> 8;  *p++ = hd->h##a; } while (0)

  /* append the 128 bit count */
  hd->buf[112] = msb >> 56;
  hd->buf[113] = msb >> 48;
  hd->buf[114] = msb >> 40;
  hd->buf[115] = msb >> 32;
  hd->buf[116] = msb >> 24;
  hd->buf[117] = msb >> 16;
  hd->buf[118] = msb >> 8;
  hd->buf[119] = msb;

  hd->buf[120] = lsb >> 56;
  hd->buf[121] = lsb >> 48;
  hd->buf[122] = lsb >> 40;
  hd->buf[123] = lsb >> 32;
  hd->buf[124] = lsb >> 24;
  hd->buf[125] = lsb >> 16;
  hd->buf[126] = lsb >> 8;
  hd->buf[127] = lsb;
#endif
  transform (hd, hd->buf);

  p = hd->buf;
#ifdef WORDS_BIGENDIAN
#undef X
#define X(a) do { *(u64*)p = hd->h##a ; p += sizeof(u64); } while (0)
#endif
  X (0);
  X (1);
  X (2);
  X (3);
  X (4);
  X (5);
  /* Note that these last two chunks are included even for SHA384.
     We just ignore them. */
  X (6);
  X (7);
#undef X
}

#if SHA2==256
static void sha224_finish (qbyte *digest, void *context)
{
	SHA2_CONTEXT *hd = (SHA2_CONTEXT *) context;
	sha2_final(context);
	memcpy(digest, hd->buf, 224/8);	//only the first 224 bits of the result...
}
static void sha256_finish (qbyte *digest, void *context)
{
	SHA2_CONTEXT *hd = (SHA2_CONTEXT *) context;
	sha2_final(context);
	memcpy(digest, hd->buf, 256/8);
}

hashfunc_t hash_sha2_224 =
{
	224/8,
	sizeof(SHA2_CONTEXT),
	sha224_init,
	sha2_write,
	sha224_finish
};
hashfunc_t hash_sha2_256 =
{
	256/8,
	sizeof(SHA2_CONTEXT),
	sha256_init,
	sha2_write,
	sha256_finish
};

/*#if defined(HAVE_SERVER) && !defined(HAVE_CLIENT)
__attribute__((constructor)) void sha2_256_unit_test(void)
{
	qbyte digest[256/8];
	qbyte need[sizeof(digest)];

	CalcHash(&hash_sha2_256, digest, sizeof(digest), (qbyte*)(volatile qbyte*)"", 0);
	Base16_DecodeBlock("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", need, sizeof(need));
	if (memcmp(digest, need, sizeof(digest)))
		printf("%s %i fail\n", __func__, __LINE__), abort();

	CalcHash(&hash_sha2_256, digest, sizeof(digest), (qbyte*)(volatile qbyte*)"abc", 3);
	Base16_DecodeBlock("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", need, sizeof(need));
	if (memcmp(digest, need, sizeof(digest)))
		printf("%s %i fail\n", __func__, __LINE__), abort();
}
#endif*/
#endif
#if SHA2==512
static void sha384_finish (qbyte *digest, void *context)
{
	SHA2_CONTEXT *hd = (SHA2_CONTEXT *) context;
	sha2_final(context);
	memcpy(digest, hd->buf, 384/8);
}

static void sha512_finish (qbyte *digest, void *context)
{
	SHA2_CONTEXT *hd = (SHA2_CONTEXT *) context;
	sha2_final(context);
	memcpy(digest, hd->buf, 512/8);
}

hashfunc_t hash_sha2_384 =
{
	384/8,
	sizeof(SHA2_CONTEXT),
	sha384_init,
	sha2_write,
	sha384_finish
};
hashfunc_t hash_sha2_512 =
{
	512/8,
	sizeof(SHA2_CONTEXT),
	sha512_init,
	sha2_write,
	sha512_finish
};
#endif
