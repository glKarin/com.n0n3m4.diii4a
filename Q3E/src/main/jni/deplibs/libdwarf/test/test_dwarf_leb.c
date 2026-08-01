/*
  Copyright (C) 2000,2004 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2011-2020 David Anderson. All Rights Reserved.

  This program is free software; you can redistribute it
  and/or modify it under the terms of version 2.1 of the
  GNU Lesser General Public License as published by the Free
  Software Foundation.

  This program is distributed in the hope that it would be
  useful, but WITHOUT ANY WARRANTY; without even the implied
  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  Further, this software is distributed without any warranty
  that it is free of the rightful claim of any third person
  regarding infringement or the like.  Any license provided
  herein, whether implied or otherwise, applies only to this
  software file.  Patent licenses, if any, provided herein
  do not apply to combinations of this program with other
  software, or any other product whatsoever.

  You should have received a copy of the GNU Lesser General
  Public License along with this program; if not, write the
  Free Software Foundation, Inc., 51 Franklin Street - Fifth
  Floor, Boston MA 02110-1301, USA.

*/

#include <config.h>

#include <stddef.h> /* size_t */
#include <stdio.h>  /* printf() */

#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dwarf_base_types.h"
#include "dwarf_opaque.h"
#include "dwarf_error.h"

static void
printinteresting(void)
{
    return;
}

static Dwarf_Signed stest[] = {
0,0xff,
0x800000000000002f,
0x800000000000003f,
0x800000000000004f,
0x8000000000000070,
0x800000000000007f,
0x8000000000000080,
0x8000000000000000,
0x800000ffffffffff,
0x80000000ffffffff,
0x800000ffffffffff,
0x8000ffffffffffff,
0xffffffffffffffff,
-1703944 /*18446744073707847672 as signed*/,
562949951588368,
-1,
-127,
-100000,
-2000000000,
-4000000000,
-8000000000,
-800000000000,
-9223372036854775807LL,
-4611686018427387904LL
};
static Dwarf_Unsigned utest[] = {
0,0xff,0x7f,0x80,
0x800000000000002f,
0x800000000000003f,
0x800000000000004f,
0x8000000000000070,
0x800000000000007f,
0x8000000000000080,
0x800000ffffffffff,
0x80000000ffffffff,
0x800000ffffffffff,
0x8000ffffffffffff,
9223372036854775808ULL,
-1703944 /*18446744073707847672 as signed*/,
562949951588368,
0xffff,
0xffffff,
0xffffffff,
0xffffffffff,
0xffffffffffff,
0xffffffffffffff,
0xffffffffffffffff
};

#define BUFFERLEN 100

static void
dump_encoded(char *space,int len)
{
    int t;

    printf("encode len %2d: ",len);
    for ( t = 0; t < len; ++t) {
        printf("%02x",space[t] & 0xff);
    }
    printf("\n");
}

static void
printsigned(Dwarf_Signed v, char *buf, int buflen, int line)
{
    printf("Signed val   %20lld 0x%16llx (line %3d) ",
        v,(Dwarf_Unsigned)v,
        line);
    dump_encoded(buf,buflen);
}
static void
printunsigned(Dwarf_Unsigned v, char *buf, int buflen, int line)
{
    printf("Unsigned val %20llu 0x%16llx "
        "(line %3d) ",
        v,v,line);
    dump_encoded(buf,buflen);
}

static unsigned
signedtest(unsigned len)
{
    unsigned errcnt = 0;
    unsigned t = 0;
    char bufferspace[BUFFERLEN];

    for ( ; t < len; ++t) {
        int res = 0;
        int encodelen = 0;
        Dwarf_Unsigned skiplen = 0;
        Dwarf_Unsigned decodelen = 0;
        Dwarf_Signed decodeval = 0;

        res = dwarf_encode_signed_leb128(
            stest[t],&encodelen,bufferspace,BUFFERLEN);
        if (res != DW_DLV_OK) {
            printf("FAIL signed encode "
                "DW_DLV_ERROR, index %u expected 0x%llx"
                " line:%d\n",
                t,stest[t],__LINE__);
            ++errcnt;
        }
        res = dwarf_decode_signed_leb128(
            (char *)bufferspace,
            &decodelen,
            &decodeval,
            (char *)(&bufferspace[BUFFERLEN-1]));
        if (res != DW_DLV_OK) {
            printf("FAIL public DW_DLV_ERROR "
                "signed decode index %u "
                "val 0x%llx line:%d\n",
                t,stest[t],__LINE__);
            ++errcnt;
        }
        if (stest[t] != decodeval) {
            printf("FAIL public signed decode val index %u "
                "expected 0x%llx vs got 0x%llx line:%d\n",
                t,stest[t],decodeval,__LINE__);
            ++errcnt;
        } else {
            printsigned(stest[t],bufferspace,encodelen,__LINE__);
        }
        if ((Dwarf_Unsigned)encodelen != decodelen) {
            printf("FAIL public signed decodelen val "
                "index %u val 0x%llx "
                " encodelen %u decodelen %u   line:%d\n",
                t,stest[t],(unsigned)encodelen,
                (unsigned)decodelen,__LINE__);
            ++errcnt;
        }

#if 0
        res = dwarf_decode_signed_leb128(
            (char *)bufferspace,
            &decodelen,
            &decodeval,
            (char *)(&bufferspace[BUFFERLEN-1]));
        if (res != DW_DLV_OK) {
            printf("FAIL got DW_DLV_ERRROR signed decode"
                " index %u "
                "expected val 0x%llx line:%d\n",
                t,stest[t],__LINE__);
            ++errcnt;
        }
        if (stest[t] != decodeval) {
            printf("FAIL signed decode val index %u "
                "val 0x%llx vs 0x%llx line:%d\n",
                t,stest[t],decodeval,__LINE__);
            ++errcnt;
        }
        if ((Dwarf_Unsigned)encodelen != decodelen) {
            printf("FAIL signed decodelen val index %u val 0x%llx "
                " encodelen %u decodelen %u   line:%d\n",
                t,stest[t],(unsigned)encodelen,
                (unsigned)decodelen,__LINE__);
            ++errcnt;
        }
#endif
        res = _dwarf_skip_leb128(bufferspace,
            &skiplen,(char *)(&bufferspace[BUFFERLEN-1]));
        if (res != DW_DLV_OK) {
            printf("FAIL got DW_DLV_ERRROR signed skip"
                " index %u "
                "expected val 0x%llx line:%d\n",
                t,stest[t],__LINE__);
            ++errcnt;
        }
        if (skiplen != decodelen) {
            printf("FAIL signed skip val index %u val 0x%llx "
                " encodelen %u decodelen %u   line:%d\n",
                t,stest[t],(unsigned)encodelen,
                (unsigned)decodelen,__LINE__);
            ++errcnt;
        }
    }
    return errcnt;
}

static  unsigned
unsignedtest(unsigned len)
{
    unsigned errcnt = 0;
    unsigned t = 0;
    char bufferspace[BUFFERLEN];

    for ( ; t < len; ++t) {
        int res = 0;
        int encodelen = 0;
        Dwarf_Unsigned skiplen = 0;
        Dwarf_Unsigned decodelen = 0;
        Dwarf_Unsigned decodeval = 0;

        res = dwarf_encode_leb128(
            utest[t],&encodelen,bufferspace,BUFFERLEN);
        if (res != DW_DLV_OK) {
            printf("FAIL signed encode index %u val 0x%llx line:%d\n",
                t,utest[t],__LINE__);
            ++errcnt;
        }
        res = dwarf_decode_leb128(
            (char *)bufferspace,
            &decodelen,
            &decodeval,
            (char *)(&bufferspace[BUFFERLEN-1]));
        if (res != DW_DLV_OK) {
            printf("FAIL public unsigned decode index %u "
                "val 0x%llx line:%d\n", t,utest[t],__LINE__);
            ++errcnt;
        }
        if (utest[t] != decodeval) {
            printf("FAIL public unsigned decode val index %u "
                "expected 0x%llx vs received 0x%llx line:%d\n",
                t,utest[t],decodeval,__LINE__);
            ++errcnt;
        } else {
            printunsigned(utest[t],bufferspace,encodelen,__LINE__);
        }

        if ((Dwarf_Unsigned)encodelen != decodelen) {
            printf("FAIL public unsigned decodelen val index %u "
                "val 0x%llx line:%d\n", t,utest[t],__LINE__);
            ++errcnt;
        }

#if 0
        res = dwarf_decode_leb128(
            (char *)bufferspace,
            &decodelen,
            &decodeval,
            (char *)(&bufferspace[BUFFERLEN-1]));
        if (res != DW_DLV_OK) {
            printf("FAIL unsigned decode index %u "
                "val 0x%llx line:%d\n",
                t,utest[t],__LINE__);
            ++errcnt;
        }
        if (utest[t] != decodeval) {
            printf("FAIL unsigned decode val index %u "
                "val 0x%llx vs 0x%llx line:%d\n",
                t,utest[t],decodeval,__LINE__);
            ++errcnt;
        }
        if ((Dwarf_Unsigned)encodelen != decodelen) {
            printf("FAIL unsigned decodelen val index %u "
                "val 0x%llx line:%d\n",
                t,utest[t],__LINE__);
            ++errcnt;
        }
#endif
        res = _dwarf_skip_leb128(bufferspace,
            &skiplen,(char *)(&bufferspace[BUFFERLEN-1]));
        if (res != DW_DLV_OK) {
            printf("FAIL got DW_DLV_ERRROR signed skip"
                " index %u "
                "expected val 0x%llx line:%d\n",
                t,stest[t],__LINE__);
            ++errcnt;
        }
        if (skiplen != decodelen) {
            printf("FAIL signed skip val index %u val 0x%llx "
                " encodelen %u decodelen %u   line:%d\n",
                t,stest[t],(unsigned)encodelen,
                (unsigned)decodelen,__LINE__);
            ++errcnt;
        }
    }
    return errcnt;
}
static unsigned char v1[] = {
0x90, 0x90, 0x90,
0x90, 0x90, 0x90,
0x90, 0x90, 0x90,
0x90, 0x90, 0x90,
0x90 };

static unsigned char v2[] = {
0xf4,0xff,
0xff,
0xff,
0x0f,
0x4c,
0x00,
0x00,
0x00};

/*   9223372036854775808 == -9223372036854775808 */
static unsigned char v3[] = {
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x41 };

/*  This warning with --enable-sanitize is fixed
    as of November 11, 2016 when decoding test v4.
    dwarf_leb.c: runtime error: negation of
    -9223372036854775808 cannot be
    represented in type 'Dwarf_Signed' (aka 'long long');
    cast to an unsigned type to negate this value to itself.
    The actual value here is
    -4611686018427387904 0xc000000000000000,
    for a 64bit signed int target */
static unsigned char v4[] = {
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0x40 }; /* the 0x40 on bit
    plays the role of incorporating the bit and
    also requesting a sign bit. */

/*  sort of v4 with zero padding on end.
    Here with target signed 64bit twos complement */
static unsigned char v5[] = {
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0xc0, /* the 0xc0 plays the role of
    incorporating a bit and continuing input. */
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0x40  /* The 0x40  cannot be or-d in
    as it is shifted off the end of the target
    64bit int area, but it plays the role of
    requesting sign bit. */
};

/*  If we had a target of 32bit signed int we would, to
    to get 0xc0000000 with trailing padding:
0x80, 0x80, 0x80,
0x80, 0x84, The 4 gets into the int., 8 continues
    the input any higher bits in the bottom
    7 bits get shifted off and vanish,
    so 0xf6 would get the same value.
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0x40     The 0x40  gets the sign set

Simlarly, but without any padding bytes:
0x80, 0x80, 0x80,
0x80, 0x44, The  second 4 (and 2 zero bits) gets into the int,
    first 4 gets the sign bit set
*/

/*  Error, too long  due to the non-zero past any valid
    Dwarf_Signed*/
static unsigned char v6[] = {
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0xc0,
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0x41, /* The 0x40  cannot be or-d in
    as we are off the end of shiftable area,
    but it plays the role of requesting sign bit. */

};

/*  unsigned decode with padding */
static unsigned char v7[] = {
/* This is missing the terminator of
    a byte without top bit set. intended ERROR */
0x81, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80
};
static unsigned char sv7[] = {
/* This is missing the terminator of
    a byte without top bit set. intended ERROR */
0x81, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0xc0
};
static unsigned char v7b[] = {
0x81, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,0
};
static unsigned char negv7b[] = {
0x81, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,0x40
};
/* padding exceeds our chosen max */
static unsigned char v8[] = {
0x81, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x81, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80,
0x80
};

static unsigned char v62[] = {
0xff, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80, /* 56 bits so far */
0x3f          /* 62 bits */
};
static unsigned char v63[] = {
0xff, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80, /* 56 bits so far */
0x7f         /* 63 bits */
};

static unsigned char v64[] = {
0xff, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80, /* 56 bits so far */
0xff,                   /* 63 bits so far */
0x01         /* 64 bits */
};

static unsigned char v65[] = {
0xff, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80, /* 56 bits so far */
0xff,                   /* 63 bits so far */
0x03         /* 65 bits, only one of the last 2 bits counts */
};

static unsigned char v63s[] = {
0xff, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80, /* 56 bits so far */
0xff,0x40  /* 63 bits and sign */
};

static unsigned char v64s[] = {
0xff, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80, /* 56 bits so far */
0xff,                   /* 63 bits so far */
0x40         /* 64 bits */
};

static unsigned char v65s[] = {
0xff, 0x80, 0x80, 0x80,
0x80, 0x80, 0x80, 0x80, /* 56 bits so far */
0xff,                   /* 63 bits so far */
0x43         /* 65 bits, the two low bits here are dropped */
};

static unsigned
specialtests(void)
{
    unsigned errcnt = 0;
    unsigned vlen = 0;
    Dwarf_Unsigned decodelen = 0;
    Dwarf_Signed sdecodeval = 0;
    Dwarf_Unsigned udecodeval = 0;
    int res;

    vlen = sizeof(v1)/sizeof(char);
    res = dwarf_decode_signed_leb128(
        (char *)v1,
        &decodelen,
        &sdecodeval,
        (char *)(&v1[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v1  line:%d\n",__LINE__);
        ++errcnt;
    }
    res = dwarf_decode_leb128(
        (char *)v1,
        &decodelen,
        &udecodeval,
        (char *)(&v1[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v1  line:%d\n",__LINE__);
        ++errcnt;
    }

    vlen = sizeof(v2)/sizeof(char);
    res = dwarf_decode_signed_leb128(
        (char *)v2,
        &decodelen,
        &sdecodeval,
        (char *)(&v2[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL signed decode special v2  line:%d\n",__LINE__);
        ++errcnt;
    }
    /*  If you just do (byte & 0x7f) << shift
        and byte is (or is promoted to) a signed type
        on the following decode you get the wrong value.
        Undefined effect in C leads to error.  */
    res = dwarf_decode_leb128(
        (char *)v2,
        &decodelen,
        &udecodeval,
        (char *)(&v2[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL unsigned decode special v2  line:%d\n",__LINE__);
        ++errcnt;
    }

    vlen = sizeof(v3)/sizeof(char);
    res = dwarf_decode_signed_leb128(
        (char *)v3,
        &decodelen,
        &sdecodeval,
        (char *)(&v3[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL signed decode special v3  line:%d\n",__LINE__);
        ++errcnt;
    }
    if ((Dwarf_Unsigned)sdecodeval !=
        (Dwarf_Unsigned)0x8000000000000000) {
        printf("FAIL signed decode special v3 value check %lld "
            "vs %lld  line:%d\n",
            sdecodeval,(Dwarf_Signed)0x8000000000000000,__LINE__);
        ++errcnt;
    }

    vlen = sizeof(v4)/sizeof(char);
    res = dwarf_decode_signed_leb128(
        (char *)v4,
        &decodelen,
        &sdecodeval,
        (char *)(&v4[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL signed decode special v4  line:%d\n",__LINE__);
        ++errcnt;
    }
    if (sdecodeval != -4611686018427387904) {
        printf("FAIL signed decode special v4 value check %lld "
            "vs %lld  line:%d\n",
            sdecodeval,-4611686018427387904LL,__LINE__);
        printf("FAIL signed decode special v4 value check 0x%llx "
            "vs 0x%llx  line:%d\n",
            sdecodeval,-4611686018427387904LL,__LINE__);
        ++errcnt;
    }
    vlen = sizeof(v5)/sizeof(char);
    res = dwarf_decode_signed_leb128(
        (char *)v5,
        &decodelen,
        &sdecodeval,
        (char *)(&v5[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL signed decode special v5  line:%d\n",__LINE__);
        ++errcnt;
    }
    if (sdecodeval != -4611686018427387904) {
        printf("FAIL signed decode special v5 value check got %lld "
            "vs expected %lld  line:%d\n",
            sdecodeval,-4611686018427387904LL,__LINE__);
        printf("FAIL signed decode special v5 value check got %llx "
            "vs expected %llx  line:%d\n",
            sdecodeval,-4611686018427387904LL,__LINE__);
        ++errcnt;
    }
    if (decodelen != vlen) {
        printf("FAIL signed decode special v5 decode len ck"
            "Expected decode len %u"
            "got decode len  %u  line %d\n",
            (unsigned)vlen,(unsigned)decodelen,__LINE__);
        ++errcnt;
    }
    vlen = sizeof(v6)/sizeof(char);
    res = dwarf_decode_signed_leb128((char *)v6,
        &decodelen,
        &sdecodeval,
        (char *)(&v6[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL signed decode special v6 "
            "did not get expected error output %d\n",
            __LINE__);
        ++errcnt;
    }

    vlen = sizeof(v7)/sizeof(char);
    res = dwarf_decode_leb128((char *)v7,
        &decodelen,
        &udecodeval,
        (char *)(&v7[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v7 "
            "unexpected that it passed. line %d\n",
            __LINE__);
        ++errcnt;
    }

    vlen = sizeof(v7)/sizeof(char);
    res = dwarf_decode_signed_leb128((char *)v7,
        &decodelen,
        &sdecodeval,
        (char *)(&v7[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL signed decode special v7 "
            "unexpected that it passed. line %d\n",
            __LINE__);
        ++errcnt;
    }

    vlen = sizeof(v7b)/sizeof(char);
    res = dwarf_decode_leb128((char *)v7b,
        &decodelen,
        &udecodeval,
        (char *)(&v7b[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v7b "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    if (udecodeval != 1) {
        printf("FAIL unsigned decode special v7b value "
            "check got %llu "
            "vs expected %u  line:%d\n",
            udecodeval,1,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL unsigned decode special v7b decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }

    vlen = sizeof(negv7b)/sizeof(char);
    res = dwarf_decode_signed_leb128((char *)negv7b,
        &decodelen,
        &sdecodeval,
        (char *)(&negv7b[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL unsigned decode special negv7b "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    if (sdecodeval != -9223372036854775807LL) {
        printf("FAIL unsigned decode special v7b value check got"
            " %lld "
            "vs expected %lld  line:%d\n",
            sdecodeval,-9223372036854775807LL,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL unsigned decode special v7b decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }

    vlen = sizeof(sv7)/sizeof(char);
    res = dwarf_decode_signed_leb128((char *)sv7,
        &decodelen,
        &sdecodeval,
        (char *)(&negv7b[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL signed decode special sv7 "
            "unexpected that it passed. line %d\n",
            __LINE__);
        ++errcnt;
    }

    vlen = sizeof(v8)/sizeof(char);
    res = dwarf_decode_signed_leb128((char *)v8,
        &decodelen,
        &sdecodeval,
        (char *)(&v8[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v8 "
            "unexpected pass expected DW_DLV_ERROR line:%d\n",
            __LINE__);
        ++errcnt;
    }
    vlen = sizeof(v8)/sizeof(char);
    res = dwarf_decode_leb128((char *)v8,
        &decodelen,
        &udecodeval,
        (char *)(&v8[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v8 "
            "unexpected pass expected DW_DLV_ERROR line:%d\n",
            __LINE__);
        ++errcnt;
    }

    return errcnt;
}

static int
testatmaxlimit(void)
{
    unsigned errcnt = 0;
    unsigned vlen = 0;
    Dwarf_Unsigned decodelen = 0;
    Dwarf_Unsigned udecodeval = 0;
    Dwarf_Signed sdecodeval = 0;
    int res = 0;

    vlen = sizeof(v62);
    res = dwarf_decode_leb128((char *)v62,
        &decodelen,
        &udecodeval,
        (char *)(&v62[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v62 "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    printunsigned(udecodeval,(char *)v62 , vlen, __LINE__);
    if (udecodeval != 4539628424389460095ULL) {
        printf("FAIL unsigned decode special v62 value "
            "check got %llu "
            "vs expected %u  line:%d\n",
            udecodeval,1,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL unsigned decode special v62 decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }
    vlen = sizeof(v63);
    res = dwarf_decode_leb128((char *)v63,
        &decodelen,
        &udecodeval,
        (char *)(&v63[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v63 "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    printunsigned(udecodeval,(char *)v63 , vlen, __LINE__);
    if (udecodeval != 9151314442816847999ULL) {
        printf("FAIL unsigned decode special v63 value "
            "check got %llu "
            "vs expected %u  line:%d\n",
            udecodeval,1,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL unsigned decode special v63 decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }

    vlen = sizeof(v64);
    res = dwarf_decode_leb128((char *)v64,
        &decodelen,
        &udecodeval,
        (char *)(&v64[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v64 "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    printunsigned(udecodeval,(char *)v64 , vlen, __LINE__);
    if (udecodeval != 18374686479671623807ULL) {
        printf("FAIL unsigned decode special v64 value "
            "check got %llu "
            "vs expected %u  line:%d\n",
            udecodeval,1,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL unsigned decode special v64 decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }

    /*  Here we discard a bit from v65, the most significant has
        nowhere to go and we presently do not call that an error. */
    vlen = sizeof(v65);
    res = dwarf_decode_leb128((char *)v65,
        &decodelen,
        &udecodeval,
        (char *)(&v65[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v65 "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    printunsigned(udecodeval,(char *)v65 , vlen, __LINE__);
    if (udecodeval != 18374686479671623807ULL) {
        printf("FAIL unsigned decode special v65 "
            "value check got %llu "
            "vs expected %u  line:%d\n",
            udecodeval,1,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL unsigned decode special v65 decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }

    vlen = sizeof(v63s);
    res = dwarf_decode_signed_leb128((char *)v63s,
        &decodelen,
        &sdecodeval,
        (char *)(&v63s[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL signed decode special v63s "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    printsigned(sdecodeval,(char *)v63s , vlen, __LINE__);
    if (sdecodeval != -72057594037927809) {
        printf("FAIL signed decode special v63s value check got %llu "
            "vs expected %u  line:%d\n",
            udecodeval,1,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL signed decode special v63s decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }

    /*  Here we discard a bit from v65, the most significant has
        nowhere to go and we presently do not call that an error. */
    vlen = sizeof(v64s);
    res = dwarf_decode_signed_leb128((char *)v64s,
        &decodelen,
        &sdecodeval,
        (char *)(&v64s[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL signed decode special v64s "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    printsigned(sdecodeval,(char *)v64s , vlen, __LINE__);
    if (sdecodeval != -72057594037927809) {
        printf("FAIL signed decode special v64s value check got %llu "
            "vs expected %u  line:%d\n",
            udecodeval,1,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL signed decode special v64s decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }

    /*  Here we discard bits from v65s, the most significant pair has
        nowhere to go and we presently do not call that an error. */
    vlen = sizeof(v65s);
    res = dwarf_decode_signed_leb128((char *)v65s,
        &decodelen,
        &sdecodeval,
        (char *)(&v65s[vlen]));
    if (res == DW_DLV_ERROR) {
        printf("FAIL signed decode special v65s "
            "unexpected error output %d\n",
            __LINE__);
        ++errcnt;
    }
    printsigned(sdecodeval,(char *)v65s , vlen, __LINE__);
    if (sdecodeval != -72057594037927809) {
        printf("FAIL signed decode special v65s value check got %llu "
            "vs expected %u  line:%d\n",
            udecodeval,1,__LINE__);
        ++errcnt;
    }
    if (vlen != decodelen) {
        printf("FAIL signed decode special v65s decode len got %u "
            "vs expected %u  line:%d\n",
            (unsigned)decodelen,vlen,__LINE__);
        ++errcnt;
    }

    return errcnt;
}

int main(void)
{
    unsigned slen = sizeof(stest)/sizeof(Dwarf_Signed);
    unsigned ulen = sizeof(utest)/sizeof(Dwarf_Unsigned);
    int errs = 0;

    printinteresting();
    errs += signedtest(slen);

    errs += unsignedtest(ulen);

    errs += specialtests();

    errs += testatmaxlimit();

    if (errs) {
        printf("FAIL. leb encode/decode errors\n");
        return 1;
    }
    printf("PASS leb tests\n");
    return 0;
}
