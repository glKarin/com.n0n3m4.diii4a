/* md5.c - MD5 Message-Digest Algorithm
 * Copyright (C) 1995,1996,1998,1999,2001,2002,
 *               2003  Free Software Foundation, Inc.
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * According to the definition of MD5 in RFC 1321 from April 1992.
 * NOTE: This is *not* the same file as the one from glibc.
 * Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.
 * heavily modified for GnuPG by Werner Koch <wk@gnupg.org>
   further de-libgcrypted to make it a bit more standalone.
 */

/* Test values:
 * ""                  D4 1D 8C D9 8F 00 B2 04  E9 80 09 98 EC F8 42 7E
 * "a"                 0C C1 75 B9 C0 F1 B6 A8  31 C3 99 E2 69 77 26 61
 * "abc                90 01 50 98 3C D2 4F B0  D6 96 3F 7D 28 E1 7F 72
 * "message digest"    F9 6B 69 7D 7C B7 93 8D  52 5A 2F 31 AA F1 61 D0
 */

#include "quakedef.h"


typedef struct {
	unsigned char buf[64];
	qint32_t count;	//count in the buffer
	quint64_t totalbytes;
    quint32_t A,B,C,D;	  /* chaining variables */
} MD5_CONTEXT;

static void
md5_init( void *context)
{
	MD5_CONTEXT *ctx = context;

	ctx->A = 0x67452301;
	ctx->B = 0xefcdab89;
	ctx->C = 0x98badcfe;
	ctx->D = 0x10325476;

	ctx->totalbytes = 0;
	ctx->count = 0;
}


/* These are the four functions used in the four steps of the MD5 algorithm
   and defined in the RFC 1321.  The first function is a little bit optimized
   (as found in Colin Plumbs public domain implementation).  */
/* #define FF(b, c, d) ((b & c) | (~b & d)) */
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) FF (d, b, c)
#define FH(b, c, d) (b ^ c ^ d)
#define FI(b, c, d) (c ^ (b | ~d))

#define buf_put_le32(b,d) ((b)[0]=(d)&0xff, (b)[1]=((d)>>8)&0xff, (b)[2]=((d)>>16)&0xff, (b)[3]=((d)>>24)&0xff)
#define buf_get_le32(b) ((b)[0]|((b)[1]<<8)|((b)[2]<<16)|((b)[3]<<24))
#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))


/****************
 * transform 64 bytes
 */
static void
md5_transform_blk ( void *c, const unsigned char *data )
{
	MD5_CONTEXT *ctx = c;
	quint32_t correct_words[16];
	register quint32_t A = ctx->A;
	register quint32_t B = ctx->B;
	register quint32_t C = ctx->C;
	register quint32_t D = ctx->D;
	quint32_t *cwp = correct_words;
	int i;

	for ( i = 0; i < 16; i++ )
		correct_words[i] = buf_get_le32(data + i * 4);

	#define OP(a, b, c, d, s, T)	\
		do							\
		{							\
			a += FF (b, c, d) + (*cwp++) + T;	\
			a = rol(a, s);			\
			a += b;					\
		} while (0)

	/* Before we start, one word about the strange constants.
	They are defined in RFC 1321 as

	T[i] = (int) (4294967296.0 * fabs (sin (i))), i=1..64
	*/

	/* Round 1.  */
	OP (A, B, C, D,  7, 0xd76aa478);
	OP (D, A, B, C, 12, 0xe8c7b756);
	OP (C, D, A, B, 17, 0x242070db);
	OP (B, C, D, A, 22, 0xc1bdceee);
	OP (A, B, C, D,  7, 0xf57c0faf);
	OP (D, A, B, C, 12, 0x4787c62a);
	OP (C, D, A, B, 17, 0xa8304613);
	OP (B, C, D, A, 22, 0xfd469501);
	OP (A, B, C, D,  7, 0x698098d8);
	OP (D, A, B, C, 12, 0x8b44f7af);
	OP (C, D, A, B, 17, 0xffff5bb1);
	OP (B, C, D, A, 22, 0x895cd7be);
	OP (A, B, C, D,  7, 0x6b901122);
	OP (D, A, B, C, 12, 0xfd987193);
	OP (C, D, A, B, 17, 0xa679438e);
	OP (B, C, D, A, 22, 0x49b40821);

	#undef OP
	#define OP(f, a, b, c, d, k, s, T)	\
		do								\
		{								\
			a += f (b, c, d) + correct_words[k] + T;	\
			a = rol(a, s);				\
			a += b;						\
		} while (0)

	/* Round 2.  */
	OP (FG, A, B, C, D,  1,  5, 0xf61e2562);
	OP (FG, D, A, B, C,  6,  9, 0xc040b340);
	OP (FG, C, D, A, B, 11, 14, 0x265e5a51);
	OP (FG, B, C, D, A,  0, 20, 0xe9b6c7aa);
	OP (FG, A, B, C, D,  5,  5, 0xd62f105d);
	OP (FG, D, A, B, C, 10,  9, 0x02441453);
	OP (FG, C, D, A, B, 15, 14, 0xd8a1e681);
	OP (FG, B, C, D, A,  4, 20, 0xe7d3fbc8);
	OP (FG, A, B, C, D,  9,  5, 0x21e1cde6);
	OP (FG, D, A, B, C, 14,  9, 0xc33707d6);
	OP (FG, C, D, A, B,  3, 14, 0xf4d50d87);
	OP (FG, B, C, D, A,  8, 20, 0x455a14ed);
	OP (FG, A, B, C, D, 13,  5, 0xa9e3e905);
	OP (FG, D, A, B, C,  2,  9, 0xfcefa3f8);
	OP (FG, C, D, A, B,  7, 14, 0x676f02d9);
	OP (FG, B, C, D, A, 12, 20, 0x8d2a4c8a);

	/* Round 3.  */
	OP (FH, A, B, C, D,  5,  4, 0xfffa3942);
	OP (FH, D, A, B, C,  8, 11, 0x8771f681);
	OP (FH, C, D, A, B, 11, 16, 0x6d9d6122);
	OP (FH, B, C, D, A, 14, 23, 0xfde5380c);
	OP (FH, A, B, C, D,  1,  4, 0xa4beea44);
	OP (FH, D, A, B, C,  4, 11, 0x4bdecfa9);
	OP (FH, C, D, A, B,  7, 16, 0xf6bb4b60);
	OP (FH, B, C, D, A, 10, 23, 0xbebfbc70);
	OP (FH, A, B, C, D, 13,  4, 0x289b7ec6);
	OP (FH, D, A, B, C,  0, 11, 0xeaa127fa);
	OP (FH, C, D, A, B,  3, 16, 0xd4ef3085);
	OP (FH, B, C, D, A,  6, 23, 0x04881d05);
	OP (FH, A, B, C, D,  9,  4, 0xd9d4d039);
	OP (FH, D, A, B, C, 12, 11, 0xe6db99e5);
	OP (FH, C, D, A, B, 15, 16, 0x1fa27cf8);
	OP (FH, B, C, D, A,  2, 23, 0xc4ac5665);

	/* Round 4.  */
	OP (FI, A, B, C, D,  0,  6, 0xf4292244);
	OP (FI, D, A, B, C,  7, 10, 0x432aff97);
	OP (FI, C, D, A, B, 14, 15, 0xab9423a7);
	OP (FI, B, C, D, A,  5, 21, 0xfc93a039);
	OP (FI, A, B, C, D, 12,  6, 0x655b59c3);
	OP (FI, D, A, B, C,  3, 10, 0x8f0ccc92);
	OP (FI, C, D, A, B, 10, 15, 0xffeff47d);
	OP (FI, B, C, D, A,  1, 21, 0x85845dd1);
	OP (FI, A, B, C, D,  8,  6, 0x6fa87e4f);
	OP (FI, D, A, B, C, 15, 10, 0xfe2ce6e0);
	OP (FI, C, D, A, B,  6, 15, 0xa3014314);
	OP (FI, B, C, D, A, 13, 21, 0x4e0811a1);
	OP (FI, A, B, C, D,  4,  6, 0xf7537e82);
	OP (FI, D, A, B, C, 11, 10, 0xbd3af235);
	OP (FI, C, D, A, B,  2, 15, 0x2ad7d2bb);
	OP (FI, B, C, D, A,  9, 21, 0xeb86d391);

	/* Put checksum in context given as argument.  */
	ctx->A += A;
	ctx->B += B;
	ctx->C += C;
	ctx->D += D;
}

static void md5_update(void *ctx, const void *in, size_t sz)
{
	MD5_CONTEXT *hd = ctx;
	hd->totalbytes += sz;	//need this for later.
	if (hd->count)
	{	//pack any extra data into our temp buffer, if we've already got data there.
		size_t pre = sizeof(hd->buf) - hd->count;
		if (pre > sz)
			pre = sz;
		memcpy(hd->buf + hd->count, in, sz);
		sz -= pre;
		in = (const qbyte*)in + pre;
	}
	if (hd->count == sizeof(hd->buf))
	{	//if we filled it then process it.
		md5_transform_blk(ctx, hd->buf);
		hd->count = 0;
	}
	while (sz >= sizeof(hd->buf))
	{	//and process the bulk of it.
		md5_transform_blk(ctx, in);
		sz -= sizeof(hd->buf);
		in = (const qbyte*)in + sizeof(hd->buf);
	}
	//save off any extra data.
	memcpy(hd->buf, in, sz);
	hd->count = sz;
}

/* The routine final terminates the message-digest computation and
 * ends with the desired message digest in mdContext->digest[0...15].
 * The handle is prepared for a new MD5 cycle.
 * Returns 16 bytes representing the digest.
 */

static void
md5_final(qbyte *digest, void *context)
{
	MD5_CONTEXT *hd = context;
	qbyte *p;
	quint64_t origbigcount = hd->totalbytes<<3;

	hd->buf[hd->count++] = 0x80;	//pad the data with a 1 (md5_update keeps at least one byte of the buffer free).
	if (hd->count > 56)
	{	//not enough space to fix the total count. fill with 0s.
		if (hd->count < sizeof(hd->buf))
			hd->buf[hd->count++] = 0;			//pad any remaining
		md5_transform_blk(context, hd->buf);
		hd->count = 0;
	}
	while(hd->count < 56)	//we need to fill the entire thing, pretty much
		hd->buf[hd->count++] = 0;			//pad any remaining

	buf_put_le32(hd->buf + 56, (origbigcount));
	buf_put_le32(hd->buf + 60, (origbigcount>>32));
	md5_transform_blk(context, hd->buf);

	p = digest;
	#define X(a) do { buf_put_le32(p, hd->a); p += 4; } while(0)
	X(A);
	X(B);
	X(C);
	X(D);
	#undef X

	memset(hd->buf, 0, sizeof(hd->buf));
	hd->count = 0;
}

hashfunc_t hash_md5 =
{
	16,
	sizeof(MD5_CONTEXT),
	md5_init,
	md5_update,
	md5_final,
};

/*
__attribute__((constructor)) void md5_unit_test(void)
{
	qbyte digest[16];

	CalcHash(&hash_md5, digest, sizeof(digest), "", 0);
	if (memcmp(digest, "\xD4\x1D\x8C\xD9\x8F\x00\xB2\x04" "\xE9\x80\x09\x98\xEC\xF8\x42\x7E", 16))
		printf("fail\n");

	CalcHash(&hash_md5, digest, sizeof(digest), "a", 1);
	if (memcmp(digest, "\x0C\xC1\x75\xB9\xC0\xF1\xB6\xA8" "\x31\xC3\x99\xE2\x69\x77\x26\x61", 16))
		printf("fail\n");

	CalcHash(&hash_md5, digest, sizeof(digest), "abc", 3);
	if (memcmp(digest, "\x90\x01\x50\x98\x3C\xD2\x4F\xB0" "\xD6\x96\x3F\x7D\x28\xE1\x7F\x72", 16))
		printf("fail\n");

	CalcHash(&hash_md5, digest, sizeof(digest), "message digest", 14);
	if (memcmp(digest, "\xF9\x6B\x69\x7D\x7C\xB7\x93\x8D" "\x52\x5A\x2F\x31\xAA\xF1\x61\xD0", 16))
		printf("fail\n");
 }*/
