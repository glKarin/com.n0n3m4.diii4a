/*
    Copyright 2011 David Anderson. All rights reserved.

    This program is free software; you can redistribute
    it and/or modify it under the terms of version 2 of
    the GNU General Public License as published by the
    Free Software Foundation.

    This program is distributed in the hope that it would
    be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A
    PARTICULAR PURPOSE.

    Further, this software is distributed without any
    warranty that it is free of the rightful claim of
    any third person regarding infringement or the like.
    Any license provided herein, whether implied or
    otherwise, applies only to this software file.
    Patent licenses, if any, provided herein do not apply
    to combinations of this program with other software,
    or any other product whatsoever.

    You should have received a copy of the GNU General
    Public License along with this program; if not, write
    the Free Software Foundation, Inc., 51 Franklin Street
    - Fifth Floor, Boston MA 02110-1301, USA.

*/

#include <ctype.h> /* isalnum() isspace() */
#include <stdio.h> /* printf() */

/* Generates a table which identifies a few dangerous characters.
   Ones one does not want to appear in output.

   It's a bit arbitrary in that we allow lots of shell-interpreted
   characters through, and most characters generally.

   But not control characters or single or double quotes.
   The quotes would be particularly problematic for post-processing
   dwarfdump output sensibly.

   This was used once to create the table in uri.c and
   not is unlikely to be used again.

*/
static void
print_ctype_entry(int c)
{
    char v[2];
    v[0] = c;
    v[1] = 0;
    if (c == 0) {
        printf("0, /* NUL 0x%02x */\n",c);
        return;
    }
    if (isalnum(c) || c == ' ' ) {
        /*  We let the space character print as space since
            lots of files are named that way in Mac and Windows.
        */
        printf("1, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    if (c == 0x21 || c == 0x23 || c == 0x26) {
        /*  We let the space character print as space since
            lots of files are named that way in Mac and Windows.
        */
        printf("1, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    if (isspace(c)) {
        /* Other white space translated. */
        printf("0, /* whitespace 0x%02x */\n",c);
        return;
    }
    if (c == 0x7f) {
        printf("0, /* DEL 0x%02x */\n",c);
        return;
    }
    if (c >= 0x01 && c <=  0x20 ) {
        /* ASCII control characters. */
        printf("0, /* control 0x%02x */\n",c);
        return;
    }
    if (c == '\'' || c == '\"' || c == '%' || c == ';' ) {
        printf("0, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    if (c >= 0x3a && c <=  0x40 ) {
        /* ASCII */
        printf("1, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    if (c == 0xa0 || c == 0xff ) {
        printf("0, /* other: 0x%02x */\n",c);
        return;
    }
    if (c >= 0x27 && c <=  0x2f ) {
        /* ASCII */
        printf("1, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    if (c >= 0x5b && c <=  0x5f ) {
        /* ASCII */
        printf("1, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    if (c >= 0x60 && c <=  0x60 ) {
        /* ASCII */
        printf("0, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    if (c >= 0x7b && c <=  0x7e ) {
        /* ASCII */
        printf("1, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    if (c < 0x7f) {
        /* ASCII */
        printf("1, /* \'%s\' 0x%02x */\n",v,c);
        return;
    }
    /* We are allowing other iso 8859 characters through unchanged. */
    printf("1, /* 0x%02x */\n",c);
}

/*  This mirrors what dwarfdump has long done via code
    in do_sanity_insert() in dd_sanitized.c.
    We are dropping special treatment of \r here
    as sometimes a pass through (with _WIN32)
    as that was a bug in sanity_insert() given
    we never passed through \t or \n characters.
    The table output here makes dd_sanitized simpler and
    faster.

    The code in dd_sanitized.c was inconsistent on
    treatment of newline carriage-return and tab, but
    now we are consistent. */
static void
print_sanitize_entry(int c_in)
{
    unsigned char c =  c_in & 0xff;

    if (!c_in) {
        printf("0 /*%u*/,",c);
        return;
    }
    if (!(c_in % 4)) {
        printf("\n");
    }
    if (c == '%') {
        printf("3 /* %c */,",c);
        return;
    }
    if (c >= 0x20 && c <=0x7e) {
        printf("1 /* %c */,",c);
        return;
    }
    if (c == 0x0D) {
        printf("\n#ifdef _WIN32\n");
        printf("1 /*0x0d*/,\n");
        printf("#else\n");
        printf("3 /*0x0d*/,\n");
        printf("#endif\n");
        return;
    }
    if (c == 0x0A || c == 0x09 ) {
        printf("1 /*0x%x*/,",c);
        return;
    }
    if (c < 0x20) {
        printf("3 /*0x%x*/,",c);
        return;
    }
    /*  This notices iso-8859 and UTF-8
        data as we don't deal with them
        properly in dwarfdump or allow in uri output. */
    printf("3 /*0x%x*/,",c);
    return;
}

int
main(void)
{
    int i = 0;
    printf("/* dwarfdump-ctype table */\n");
    printf("char dwarfdump_ctype_table[256] = { \n");
    for ( i = 0 ; i <= 255; ++i) {
        print_ctype_entry(i);
    }
    printf("};\n");

    printf("\n");
    printf("/* dwarfdump-sanitize table */\n");
    printf("char dwarfdump_sanitize_table[256] = { \n");
    for ( i = 0 ; i <= 255; ++i) {
        print_sanitize_entry(i);
    }
    printf("};\n");
    return 0;
}
