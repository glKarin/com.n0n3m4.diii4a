/*
Copyright 2011-2012 David Anderson. All rights reserved.
Portions Copyright 2012 SN Systems Ltd. All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of version 2 of the GNU General
  Public License as published by the Free Software Foundation.

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

  You should have received a copy of the GNU General Public
  License along with this program; if not, write the Free
  Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
  Boston MA 02110-1301, USA.

*/

#include <config.h>
#include <stdio.h> /* FILE decl for dd_esb.h */

#include "dwarf.h"
#include "libdwarf.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_esb.h"
#include "dd_uri.h"

/* dwarfdump_ctype table. See uritablebuild.c */
static char dwarfdump_ctype_table[256] = {
0, /* NUL 0x00 */
0, /* control 0x01 */
0, /* control 0x02 */
0, /* control 0x03 */
0, /* control 0x04 */
0, /* control 0x05 */
0, /* control 0x06 */
0, /* control 0x07 */
0, /* control 0x08 */
0, /* whitespace 0x09 */
0, /* whitespace 0x0a */
0, /* whitespace 0x0b */
0, /* whitespace 0x0c */
0, /* whitespace 0x0d */
0, /* control 0x0e */
0, /* control 0x0f */
0, /* control 0x10 */
0, /* control 0x11 */
0, /* control 0x12 */
0, /* control 0x13 */
0, /* control 0x14 */
0, /* control 0x15 */
0, /* control 0x16 */
0, /* control 0x17 */
0, /* control 0x18 */
0, /* control 0x19 */
0, /* control 0x1a */
0, /* control 0x1b */
0, /* control 0x1c */
0, /* control 0x1d */
0, /* control 0x1e */
0, /* control 0x1f */
1, /* ' ' 0x20 */
1, /* '!' 0x21 */
0, /* '"' 0x22 */
1, /* '#' 0x23 */
1, /* '$' 0x24 */
0, /* '%' 0x25 */
1, /* '&' 0x26 */
0, /* ''' 0x27 */
1, /* '(' 0x28 */
1, /* ')' 0x29 */
1, /* '*' 0x2a */
1, /* '+' 0x2b */
1, /* ',' 0x2c */
1, /* '-' 0x2d */
1, /* '.' 0x2e */
1, /* '/' 0x2f */
1, /* '0' 0x30 */
1, /* '1' 0x31 */
1, /* '2' 0x32 */
1, /* '3' 0x33 */
1, /* '4' 0x34 */
1, /* '5' 0x35 */
1, /* '6' 0x36 */
1, /* '7' 0x37 */
1, /* '8' 0x38 */
1, /* '9' 0x39 */
1, /* ':' 0x3a */
0, /* ';' 0x3b */
1, /* '<' 0x3c */
1, /* '=' 0x3d */
1, /* '>' 0x3e */
1, /* '?' 0x3f */
1, /* '@' 0x40 */
1, /* 'A' 0x41 */
1, /* 'B' 0x42 */
1, /* 'C' 0x43 */
1, /* 'D' 0x44 */
1, /* 'E' 0x45 */
1, /* 'F' 0x46 */
1, /* 'G' 0x47 */
1, /* 'H' 0x48 */
1, /* 'I' 0x49 */
1, /* 'J' 0x4a */
1, /* 'K' 0x4b */
1, /* 'L' 0x4c */
1, /* 'M' 0x4d */
1, /* 'N' 0x4e */
1, /* 'O' 0x4f */
1, /* 'P' 0x50 */
1, /* 'Q' 0x51 */
1, /* 'R' 0x52 */
1, /* 'S' 0x53 */
1, /* 'T' 0x54 */
1, /* 'U' 0x55 */
1, /* 'V' 0x56 */
1, /* 'W' 0x57 */
1, /* 'X' 0x58 */
1, /* 'Y' 0x59 */
1, /* 'Z' 0x5a */
1, /* '[' 0x5b */
1, /* '\' 0x5c */
1, /* ']' 0x5d */
1, /* '^' 0x5e */
1, /* '_' 0x5f */
0, /* '`' 0x60 */
1, /* 'a' 0x61 */
1, /* 'b' 0x62 */
1, /* 'c' 0x63 */
1, /* 'd' 0x64 */
1, /* 'e' 0x65 */
1, /* 'f' 0x66 */
1, /* 'g' 0x67 */
1, /* 'h' 0x68 */
1, /* 'i' 0x69 */
1, /* 'j' 0x6a */
1, /* 'k' 0x6b */
1, /* 'l' 0x6c */
1, /* 'm' 0x6d */
1, /* 'n' 0x6e */
1, /* 'o' 0x6f */
1, /* 'p' 0x70 */
1, /* 'q' 0x71 */
1, /* 'r' 0x72 */
1, /* 's' 0x73 */
1, /* 't' 0x74 */
1, /* 'u' 0x75 */
1, /* 'v' 0x76 */
1, /* 'w' 0x77 */
1, /* 'x' 0x78 */
1, /* 'y' 0x79 */
1, /* 'z' 0x7a */
1, /* '{' 0x7b */
1, /* '|' 0x7c */
1, /* '}' 0x7d */
1, /* '~' 0x7e */
0, /* DEL 0x7f */
1, /* 0x80 */
1, /* 0x81 */
1, /* 0x82 */
1, /* 0x83 */
1, /* 0x84 */
1, /* 0x85 */
1, /* 0x86 */
1, /* 0x87 */
1, /* 0x88 */
1, /* 0x89 */
1, /* 0x8a */
1, /* 0x8b */
1, /* 0x8c */
1, /* 0x8d */
1, /* 0x8e */
1, /* 0x8f */
1, /* 0x90 */
1, /* 0x91 */
1, /* 0x92 */
1, /* 0x93 */
1, /* 0x94 */
1, /* 0x95 */
1, /* 0x96 */
1, /* 0x97 */
1, /* 0x98 */
1, /* 0x99 */
1, /* 0x9a */
1, /* 0x9b */
1, /* 0x9c */
1, /* 0x9d */
1, /* 0x9e */
1, /* 0x9f */
0, /* other: 0xa0 */
1, /* 0xa1 */
1, /* 0xa2 */
1, /* 0xa3 */
1, /* 0xa4 */
1, /* 0xa5 */
1, /* 0xa6 */
1, /* 0xa7 */
1, /* 0xa8 */
1, /* 0xa9 */
1, /* 0xaa */
1, /* 0xab */
1, /* 0xac */
1, /* 0xad */
1, /* 0xae */
1, /* 0xaf */
1, /* 0xb0 */
1, /* 0xb1 */
1, /* 0xb2 */
1, /* 0xb3 */
1, /* 0xb4 */
1, /* 0xb5 */
1, /* 0xb6 */
1, /* 0xb7 */
1, /* 0xb8 */
1, /* 0xb9 */
1, /* 0xba */
1, /* 0xbb */
1, /* 0xbc */
1, /* 0xbd */
1, /* 0xbe */
1, /* 0xbf */
1, /* 0xc0 */
1, /* 0xc1 */
1, /* 0xc2 */
1, /* 0xc3 */
1, /* 0xc4 */
1, /* 0xc5 */
1, /* 0xc6 */
1, /* 0xc7 */
1, /* 0xc8 */
1, /* 0xc9 */
1, /* 0xca */
1, /* 0xcb */
1, /* 0xcc */
1, /* 0xcd */
1, /* 0xce */
1, /* 0xcf */
1, /* 0xd0 */
1, /* 0xd1 */
1, /* 0xd2 */
1, /* 0xd3 */
1, /* 0xd4 */
1, /* 0xd5 */
1, /* 0xd6 */
1, /* 0xd7 */
1, /* 0xd8 */
1, /* 0xd9 */
1, /* 0xda */
1, /* 0xdb */
1, /* 0xdc */
1, /* 0xdd */
1, /* 0xde */
1, /* 0xdf */
1, /* 0xe0 */
1, /* 0xe1 */
1, /* 0xe2 */
1, /* 0xe3 */
1, /* 0xe4 */
1, /* 0xe5 */
1, /* 0xe6 */
1, /* 0xe7 */
1, /* 0xe8 */
1, /* 0xe9 */
1, /* 0xea */
1, /* 0xeb */
1, /* 0xec */
1, /* 0xed */
1, /* 0xee */
1, /* 0xef */
1, /* 0xf0 */
1, /* 0xf1 */
1, /* 0xf2 */
1, /* 0xf3 */
1, /* 0xf4 */
1, /* 0xf5 */
1, /* 0xf6 */
1, /* 0xf7 */
1, /* 0xf8 */
1, /* 0xf9 */
1, /* 0xfa */
1, /* 0xfb */
1, /* 0xfc */
1, /* 0xfd */
1, /* 0xfe */
0, /* other: 0xff */
};

/* Translate dangerous and some other characters to safe
   %xx form.
*/
void
translate_to_uri(const char * filename, struct esb_s *out)
{
    const char *cp = 0;

    for (cp = filename  ; *cp; ++cp) {
        char v[2];
        int c = 0xff & (unsigned char)*cp;
        if (dwarfdump_ctype_table[c]) {
            v[0] = (char)c;
            v[1] = 0;
            esb_append(out,v);
        } else {
            esb_append(out,"%");
            esb_append_printf_u(out, "%02x",c&0xff);
        }
    }
}

/* This is not very efficient, but it is seldom called. */
static char
hexdig(char c)
{
    char ochar = 0;
    if (c >= '0' && c <= '9') {
        ochar = (c - '0');
        return ochar;
    }
    if (c >= 'a' && c <= 'f') {
        ochar = (c - 'a')+10;
        return ochar;
    }
    if (c >= 'A' && c <= 'F') {
        ochar = (c - 'A')+10;
        return ochar;
    }
    /* We have an input botch here. */
    printf("ERROR Translating from uri: "
        "A supposed hexadecimal input character is "
        "not 0-9 or a-f or A-F, it is (shown as hex here): %x\n",c);
    glflags.gf_count_major_errors++;

    return ochar;
}

static char tohex(char c1, char c2)
{
    char out = (hexdig(c1) << 4) | hexdig(c2);
    return out;
}
static int
hexpairtochar(const char *cp, char*myochar)
{
    char ochar = 0;
    int olen = 0;
    char c = cp[0];
    if (c) {
        char c2 = cp[1];
        if (c2) {
            ochar = tohex(c,c2);
            olen = 2;
        } else {
            printf("ERROR Translating from uri: "
                "A supposed hexadecimal input character pair "
                "runs off the end of the input after 1 hex digit.\n");
            glflags.gf_count_major_errors++;
            /* botched input. */
            ochar = c;
            olen = 1;
        }
    } else {
        /* botched input. */
        printf("ERROR Translating from uri: "
            "A supposed hexadecimal input character pair "
            "runs off the end of the input.\n");
        glflags.gf_count_major_errors++;
        ochar = '%';
        olen = 0;
    }
    *myochar = ochar;
    return olen;
}

void
translate_from_uri(const char * input, struct esb_s* out)
{
    const char *cp = input;
    char tempstr[2];
    for (; *cp; ++cp) {
        char c = *cp;
        if (c == '%') {
            int increment = 0;
            char c2 = cp[1];
            /* hexpairtochar deals with c2 being NUL. */
            if (c2  == '%') {
                tempstr[0] = c;
                tempstr[1] = 0;
                esb_append(out,tempstr);
                ++cp;
                continue;
            }

            increment = hexpairtochar(cp+1,&c);
            tempstr[0] = c;
            tempstr[1] = 0;
            esb_append(out,tempstr);
            cp +=increment;
            continue;
        }
        tempstr[0] = c;
        tempstr[1] = 0;
        esb_append(out,tempstr);
    }
}

#ifdef TEST

unsigned int errcnt = 0;

static void
mytestfrom(const char * in,const char *expected,int testnum)
{
    struct esb_s out;
    esb_constructor(&out);
    translate_from_uri(in, &out);
    if (strcmp(expected, esb_get_string(&out))) {
        printf(" Fail test %d expected \"%s\" got \"%s\"\n",
            testnum,expected,esb_get_string(&out));
        ++errcnt;
    }
    esb_destructor(&out);
}

static void
mytest(char *in,char *expected,int testnum)
{
    struct esb_s out;
    esb_constructor(&out);
    translate_to_uri(in, &out);
    if (strcmp(expected, esb_get_string(&out))) {
        printf(" Fail test %d expected %s got %s\n",
            testnum,expected,esb_get_string(&out));
        ++errcnt;
    }
    esb_destructor(&out);
}

int
main()
{
    /* We no longer translate space to %20, that
    turns out not to help all that much. */
    mytest("aaa","aaa",1);
    mytest(" bc"," bc",2);
    mytest(";bc","%3bbc",3);
    mytest(" bc\n"," bc%0a",4);
    mytest(";bc\n","%3bbc%0a",5);
    mytest(" bc\r"," bc%0d",6);
    mytest(";bc\r","%3bbc%0d",7);
    mytest(" \x01"," %01",8);
    mytest(";\x01","%3b%01",9);
    mytestfrom("abc","abc",10);
    mytestfrom("a%20bc","a bc",11);
    mytestfrom("a%%20bc","a%20bc",12);
    mytestfrom("a%%%20bc","a% bc",13);
    mytestfrom("a%%%%20bc","a%%20bc",14);
    mytestfrom("a%20","a ",15);
    /* The following is mistaken input. */
    mytestfrom("a%2","a2",16);
    mytestfrom("a%","a%",17);
    mytest("%bc","%25bc",18);

    if (errcnt) {
        printf("uri errcount ",errcnt);
    }
    return errcnt? 1:0;
}
#endif
