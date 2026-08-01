/*
Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
*/

/*  The code has been reformatted to match libdwarf
    codingstyle */

#include "config.h"
#include <stdio.h>
#include "dwarf.h"
#include "libdwarf.h"
#include "dd_sanitized.h"
#include "dd_utf8.h"

#ifdef HAVE_UTF8

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1
#define TRUE  1
#define FALSE 0

static const unsigned char utf8d[] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3,
0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
/* 256 */
0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1,
1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1,
1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

/*  codep (codepoints) count the number of characters
    (as opposed to the number of bytes)
*/
Dwarf_Unsigned
dd_utf8_decode(Dwarf_Unsigned* state, Dwarf_Unsigned* codep,
    unsigned char byte) {
    unsigned char type = utf8d[byte];

    if (*state == UTF8_ACCEPT) {
        int t = dwarfdump_sanitize_table[byte];
        if (t == 5) {
            /*  Certain control chars are legal utf8
                since legal ASCII, but we
                reject them here so printf won't emit them
                and possibly screw up a live terminal session.
                We do not really care about a string
                with both utf8 *and* bogus control
                characters: let such print uri-style. */
            *state = UTF8_REJECT;
            return *state;
        }
        *codep = (0xff >> type) & (byte);
    } else {
        *codep = (byte & 0x3fu) | (*codep << 6) ;
    }
    /* Documenting: from the original version
        *codep = (*state != UTF8_ACCEPT) ?
        (byte & 0x3fu) | (*codep << 6) :
        (0xff >> type) & (byte); */
    /* ASSERT: values cannot exceed 400 */
    *state = utf8d[256 + *state*16 + type];
    return *state;
}

/*  It is necessary to remember the codepoint
    for the algorithm to work, though
    we ignore the codpoints. */
int
dd_utf8_checkCodePoints(unsigned char* s) {
    Dwarf_Unsigned codepoint;
    Dwarf_Unsigned state = 0;

    for ( ; *s; ++s) {
        dd_utf8_decode(&state, &codepoint, *s);
        if (state == UTF8_REJECT) {
            return DW_DLV_ERROR;
        }
    }
    if ( state == UTF8_ACCEPT) {
        return DW_DLV_OK;
    }
    return DW_DLV_ERROR;
}
#endif /* HAVE_UTF8 */
