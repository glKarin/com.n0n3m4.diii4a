/*
Copyright 2016-2018 David Anderson. All rights reserved.

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
#include <string.h> /* strcmp strncmp */

#include "dwarf.h"
#include "libdwarf.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_esb.h"
#ifndef TESTING
#include "dd_glflags.h"
#endif
#ifdef HAVE_UTF8
#include "dd_utf8.h"
#endif /* HAVE_UTF8 */
#include "dd_sanitized.h"

/*  This does a uri-style conversion of control characters.
    So  SOH prints as %01 for example.
    Which stops corrupted or crafted strings from
    doing things to the terminal the string is routed to.

    We do not translate an input % to %% (as in real uri)
    as that would be a bit confusing for most readers.

    The conversion makes it possible to print UTF-8 strings
    reproducibly, sort of (not showing the
    real glyph!).

    Only call this in a printf or sprintf, and
    only call it once in any single printf/sprintf.
    Otherwise you will get bogus results and confusion. */

/* ASCII control codes:
We leave newline as is, NUL is end of string,
the others are translated.
NUL Null             0  00              Ctrl-@ ^@
SOH Start of heading 1  01      Alt-1   Ctrl-A ^A
STX Start of text    2  02      Alt-2   Ctrl-B ^B
ETX End of text      3  03      Alt-3   Ctrl-C ^C
EOT End of transmission 4 04    Alt-4   Ctrl-D ^D
ENQ Enquiry          5    05    Alt-5   Ctrl-E ^E
ACK Acknowledge      6    06    Alt-6   Ctrl-F ^F
BEL Bell             7    07    Alt-7   Ctrl-G ^G
BS  Backspace        8    08    Alt-8   Ctrl-H ^H
HT  Horizontal tab   9    09    Alt-9   Ctrl-I ^I
LF  Line feed       10    0A    Alt-10  Ctrl-J ^J
VT  Vertical tab    11    0B    Alt-11  Ctrl-K ^K
FF  Form feed       12    0C    Alt-12  Ctrl-L ^L
CR  Carriage return 13    0D    Alt-13  Ctrl-M ^M
SO  Shift out       14    0E    Alt-14  Ctrl-N ^N
SI  Shift in        15    0F    Alt-15  Ctrl-O ^O
DLE Data line escape 16   10    Alt-16  Ctrl-P ^P
DC1 Device control 1 17   11    Alt-17  Ctrl-Q ^Q
DC2 Device control 2 18   12    Alt-18  Ctrl-R ^R
DC3 Device control 3 19   13    Alt-19  Ctrl-S ^S
DC4 Device control 4 20   14    Alt-20  Ctrl-T ^T
NAK Negative acknowledge 21 15  Alt-21  Ctrl-U ^U
SYN Synchronous idle 22   16    Alt-22  Ctrl-V ^V
ETB End transmission block 23 17 Alt-23 Ctrl-W ^W
CAN Cancel              24 18   Alt-24  Ctrl-X ^X
EM  End of medium       25 19   Alt-25  Ctrl-Y ^Y
SU  Substitute          26 1A   Alt-26  Ctrl-Z ^Z
ES  Escape              27 1B   Alt-27  Ctrl-[ ^[
FS  File separator      28 1C   Alt-28  Ctrl-\ ^\
GS  Group separator     29 1D   Alt-29  Ctrl-] ^]
RS  Record separator    30 1E   Alt-30  Ctrl-^ ^^
US  Unit separator      31 1F   Alt-31  Ctrl-_ ^_

In addition,  characters decimal 141, 157, 127,128,
129 143,144,157 appear to be questionable too.  Not in
iso-8859-1 nor in html character entities list.

We translate all strings with a % to do sanitizing.
we change a literal ASCII '%' char to %25 so readers
know any % is a sanitized char. We could double up a %
into %% on output, but switching to %25 is simpler and
for readers and prevents ambiguity.  If a % is found then
utf8 is suppressed for the entire string and uri-style %25
is used.  We avoid using the % symbol itself in strings,
so we expect uri-style is not suppressed.

If the dwarfdump runtime environment is UTF-8 and
the necessary locale() and nl_langinfo() functions are
available and the string has no unwanted bytes then utf8
names are printed using utf8 so they look as expected.

DWREGRESSIONTEMP is defined for Windows Msys2 and its
special issues with naming in regression testing and
defined for all platforms when running regression tests.

*/
#define SANBUF_SIZE 400
/*  Allocates as esb_constructor_fixed() would.
    For strings shorter than
    SANBUF_SIZE bytes no malloc needed. */
static char sanbufa[SANBUF_SIZE];
static char sanbufb[SANBUF_SIZE];
static char sanbufc[SANBUF_SIZE];
static int usebufnum = 0;
#ifdef DWREGRESSIONTEMP
static char sanbufhomea[SANBUF_SIZE];
static char sanbufhomeb[SANBUF_SIZE];
static char sanbufhomec[SANBUF_SIZE];
#endif /* DWREGRESSIONTEMP */

static struct esb_s localesba = {sanbufa,
    SANBUF_SIZE,0,1,0};
static struct esb_s localesbb = {sanbufb,
    SANBUF_SIZE,0,1,0};
static struct esb_s localesbc = {sanbufc,
    SANBUF_SIZE,0,1,0};
#ifdef DWREGRESSIONTEMP
static struct esb_s localhomeifya = {sanbufhomea,
    SANBUF_SIZE,0,1,0};
static struct esb_s localhomeifyb = {sanbufhomeb,
    SANBUF_SIZE,0,1,0};
static struct esb_s localhomeifyc = {sanbufhomec,
    SANBUF_SIZE,0,1,0};
static int usehomeifynum = 0;
#endif /* DWREGRESSIONTEMP */

/*  dwarfdump-sanitize table
    1 means printable ASCII character (byte)
    3 means non-printable
    5 means non-printable control character (byte) */
char dwarfdump_sanitize_table[256] = {
0 /*0*/  ,5 /*0x1*/,5 /*0x2*/,5 /*0x3*/,
5 /*0x4*/,5 /*0x5*/,5 /*0x6*/,5 /*0x7*/,
5 /*0x8*/,1 /*0x9*/,1 /*0xa*/,5 /*0xb*/,
5 /*0xc*/,
#ifdef _WIN32
1 /*0x0d*/,
#else
5 /*0x0d*/,
#endif
5 /*0x0e*/,5 /*0xf*/,
5 /*0x10*/,5 /*0x11*/,5 /*0x12*/,5 /*0x13*/,
5 /*0x14*/,5 /*0x15*/,5 /*0x16*/,5 /*0x17*/,
5 /*0x18*/,5 /*0x19*/,5 /*0x1a*/,5 /*0x1b*/,
5 /*0x1c*/,5 /*0x1d*/,5 /*0x1e*/,5 /*0x1f*/,
1 /*   */,1 /* ! */,1 /* " */,1 /* # */,
1 /* $ */,5 /* % */,1 /* & */,1 /* ' */,
1 /* ( */,1 /* ) */,1 /* * */,1 /* + */,
1 /* , */,1 /* - */,1 /* . */,1 /* / */,
1 /* 0 */,1 /* 1 */,1 /* 2 */,1 /* 3 */,
1 /* 4 */,1 /* 5 */,1 /* 6 */,1 /* 7 */,
1 /* 8 */,1 /* 9 */,1 /* : */,1 /* ; */,
1 /* < */,1 /* = */,1 /* > */,1 /* ? */,
1 /* @ */,1 /* A */,1 /* B */,1 /* C */,
1 /* D */,1 /* E */,1 /* F */,1 /* G */,
1 /* H */,1 /* I */,1 /* J */,1 /* K */,
1 /* L */,1 /* M */,1 /* N */,1 /* O */,
1 /* P */,1 /* Q */,1 /* R */,1 /* S */,
1 /* T */,1 /* U */,1 /* V */,1 /* W */,
1 /* X */,1 /* Y */,1 /* Z */,1 /* [ */,
1 /* \ */,1 /* ] */,1 /* ^ */,1 /* _ */,
1 /* ` */,1 /* a */,1 /* b */,1 /* c */,
1 /* d */,1 /* e */,1 /* f */,1 /* g */,
1 /* h */,1 /* i */,1 /* j */,1 /* k */,
1 /* l */,1 /* m */,1 /* n */,1 /* o */,
1 /* p */,1 /* q */,1 /* r */,1 /* s */,
1 /* t */,1 /* u */,1 /* v */,1 /* w */,
1 /* x */,1 /* y */,1 /* z */,1 /* { */,
1 /* | */,1 /* } */,1 /* ~ */,5 /*0x7f*/,
3 /*0x80*/,3 /*0x81*/,3 /*0x82*/,3 /*0x83*/,
3 /*0x84*/,3 /*0x85*/,3 /*0x86*/,3 /*0x87*/,
3 /*0x88*/,3 /*0x89*/,3 /*0x8a*/,3 /*0x8b*/,
3 /*0x8c*/,3 /*0x8d*/,3 /*0x8e*/,3 /*0x8f*/,
3 /*0x90*/,3 /*0x91*/,3 /*0x92*/,3 /*0x93*/,
3 /*0x94*/,3 /*0x95*/,3 /*0x96*/,3 /*0x97*/,
3 /*0x98*/,3 /*0x99*/,3 /*0x9a*/,3 /*0x9b*/,
3 /*0x9c*/,3 /*0x9d*/,3 /*0x9e*/,3 /*0x9f*/,
3 /*0xa0*/,3 /*0xa1*/,3 /*0xa2*/,3 /*0xa3*/,
3 /*0xa4*/,3 /*0xa5*/,3 /*0xa6*/,3 /*0xa7*/,
3 /*0xa8*/,3 /*0xa9*/,3 /*0xaa*/,3 /*0xab*/,
3 /*0xac*/,3 /*0xad*/,3 /*0xae*/,3 /*0xaf*/,
3 /*0xb0*/,3 /*0xb1*/,3 /*0xb2*/,3 /*0xb3*/,
3 /*0xb4*/,3 /*0xb5*/,3 /*0xb6*/,3 /*0xb7*/,
3 /*0xb8*/,3 /*0xb9*/,3 /*0xba*/,3 /*0xbb*/,
3 /*0xbc*/,3 /*0xbd*/,3 /*0xbe*/,3 /*0xbf*/,
3 /*0xc0*/,3 /*0xc1*/,3 /*0xc2*/,3 /*0xc3*/,
3 /*0xc4*/,3 /*0xc5*/,3 /*0xc6*/,3 /*0xc7*/,
3 /*0xc8*/,3 /*0xc9*/,3 /*0xca*/,3 /*0xcb*/,
3 /*0xcc*/,3 /*0xcd*/,3 /*0xce*/,3 /*0xcf*/,
3 /*0xd0*/,3 /*0xd1*/,3 /*0xd2*/,3 /*0xd3*/,
3 /*0xd4*/,3 /*0xd5*/,3 /*0xd6*/,3 /*0xd7*/,
3 /*0xd8*/,3 /*0xd9*/,3 /*0xda*/,3 /*0xdb*/,
3 /*0xdc*/,3 /*0xdd*/,3 /*0xde*/,3 /*0xdf*/,
3 /*0xe0*/,3 /*0xe1*/,3 /*0xe2*/,3 /*0xe3*/,
3 /*0xe4*/,3 /*0xe5*/,3 /*0xe6*/,3 /*0xe7*/,
3 /*0xe8*/,3 /*0xe9*/,3 /*0xea*/,3 /*0xeb*/,
3 /*0xec*/,3 /*0xed*/,3 /*0xee*/,3 /*0xef*/,
3 /*0xf0*/,3 /*0xf1*/,3 /*0xf2*/,3 /*0xf3*/,
3 /*0xf4*/,3 /*0xf5*/,3 /*0xf6*/,3 /*0xf7*/,
3 /*0xf8*/,3 /*0xf9*/,3 /*0xfa*/,3 /*0xfb*/,
3 /*0xfc*/,3 /*0xfd*/,3 /*0xfe*/,3 /*0xff*/,};

/*  do_sanity_insert() and no_questionable_chars()
    absolutely must have the same idea of
    questionable characters.  Be Careful.
    Until 0.5.0 the two did NOT agree on
    handling of newline or tab or carriage return
    which was... a bug.
    no_questionable_chars() said those three ok
    so do_sanity_insert was never called, making
    an inconsistency vs this code as it was
    before 0.5.0.
    */
static void
do_sanity_insert( const char *s,struct esb_s *mesb)
{
    const char *cp = s;

    for ( ; *cp; cp++) {
        unsigned c = *cp & 0xff ;
        int t = dwarfdump_sanitize_table[c];

        if (t == 1) {
            esb_appendn(mesb,cp,1);
            continue;
        }
        esb_appendn(mesb, "%",1);
        esb_append_printf_u(mesb, "%02x",c & 0xff);
    }
}

/*  This routine improves overall dwarfdump
    run times a lot by separating strings
    that might print badly from strings that
    will print fine.
    In one large test case it reduces run time
    from 140 seconds to 13 seconds. */
static int
no_questionable_chars(const char *s) {
    const char *cp = s;

    for ( ; *cp; cp++) {
        unsigned c = *cp & 0xff ;
        int t = dwarfdump_sanitize_table[c];

        if (t == 1) {
            continue;
        }
        return FALSE;
    }
    return TRUE;
}

void
sanitized_string_destructor(void)
{
    esb_destructor(&localesba);
    esb_destructor(&localesbb);
    esb_destructor(&localesbc);
#ifdef DWREGRESSIONTEMP
    esb_destructor(&localhomeifya);
    esb_destructor(&localhomeifyb);
    esb_destructor(&localhomeifyc);
#endif /* DWREGRESSIONTEMP */
}
#ifdef DWREGRESSIONTEMP
static int
look_for_substr(const char *s,const char *match,
    int matchlen)
{
    const char *cp = s;
    int remaining = strlen(s);

    if (remaining < matchlen) {
        return -1;
    }
    for ( ; *cp ; ++cp,--remaining) {
        if (*cp != match[0]) {
            continue;
        }
        if (remaining < matchlen) {
            /*  No match possible */
            return -1;
        }
        if (strncmp(cp,match,matchlen)) {
            continue;
        }
        return (int)(cp - s);
    }
    return -1;
}
/*  This makes the simplifying assumption that
    a Windows path will only appear once in a string,
    but could be generalized if necessary.  */
static Dwarf_Bool
fullpathtohome(const char *s,struct esb_s *out)
{
    int pos = -1;
    const char *match="C:/msys64/davea/home/admin";
    size_t strlenmatch = strlen(match);

    pos = look_for_substr(s,match,(int)strlenmatch);
    if (pos < 0) {
        return FALSE;
    }
    if (pos > 0) {
        esb_appendn(out,s,pos);
    }
    esb_append(out,"$HOME");
    esb_append(out,s+strlenmatch);
    return TRUE;
}
#endif /* DWREGRESSIONTEMP */

/*  Because we reuse static esb's this MUST NOT
    be called a fourth time before printing
    out the initial returns.  It is rarely
    a problem.  But it is up to the caller to
    behave correctly to avoid getting
    incorrect strings.

    We need this to substitute
    $HOME for windows C: etc file names.
*/
const char *
sanitized(const char *s)
{
    const char *sout = 0;
    struct esb_s *lsp = 0;
#ifdef DWREGRESSIONTEMP
    struct esb_s *hsp = 0;
    Dwarf_Bool changed = FALSE;
    switch (usehomeifynum) {
    case 0:
        hsp = &localhomeifya;
        usehomeifynum = 1;
        break;
    case 1:
        hsp = &localhomeifyb;
        usehomeifynum = 2;
        break;
    case 2:
        hsp = &localhomeifyc;
        usehomeifynum = 0;
        break;
    default: /*  Impossible! */
        hsp = &localhomeifya;
        usehomeifynum = 1;
        break;
    }
    esb_empty_string(hsp);
    changed = fullpathtohome(s,hsp);
    if (changed) {
        s = (const char *)esb_get_string(hsp);
    }
#endif /* DWREGRESSIONTEMP */

#ifndef TESTING
    if (glflags.gf_no_sanitize_strings) {
        return s;
    }
#endif
    if (no_questionable_chars(s)) {
        /*  The original string is safe ASCII as is. */
        return s;
    }
    /*  Using esb_destructor is quite expensive in cpu time
        when we build the next sanitized string
        so we just empty the localesb.
        One reason it's expensive is that we do the appends
        in such small batches in do_sanity_insert().
        */

#ifndef TESTING
#ifdef  HAVE_UTF8
    if (glflags.gf_print_utf8_flag) {
        if (DW_DLV_OK == dd_utf8_checkCodePoints(
            (unsigned char *)s)) {
            return s;
        }
    }
#endif /* HAVE_UTF8 */
#endif /* TESTING */
    switch (usebufnum) {
    case 0:
        lsp = &localesba;
        usebufnum = 1;
        break;
    case 1:
        lsp = &localesbb;
        usebufnum = 2;
        break;
    case 2:
        lsp = &localesbc;
        usebufnum = 0;
        break;
    default: /*  Impossible! */
        lsp = &localesba;
        usebufnum = 1;
        break;
    }
    esb_empty_string(lsp);
    do_sanity_insert(s,lsp);
    sout = esb_get_string(lsp);
    return sout;
}
