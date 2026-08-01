/*
Copyright (C) 2005 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright (C) 2013-2018 David Anderson. All Rights Reserved.

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

/*  test_sanitized.c
    testing dd_sanitized.c

*/

#include <config.h>

#include <stdlib.h> /* exit() */
#include <string.h> /* strcmp() */
#include <stdio.h> /* FILE printf etc */

#include "libdwarf_private.h"
#include "dd_esb.h"
#include "dd_minimal.h"
#include "dd_sanitized.h"

void dd_minimal_count_global_error(void) {}

/* Needed so dd_sanitized.c compiles here. */
struct glflags_s {
    int gf_no_sanitize_strings;
} glflags;

static int failcount = 0;

static void
validate_san(int line,
    const char *input,
    const char *output,
    const char *expout)
{
    if (strcmp(output,expout)) {
        ++failcount;
        printf("  FAIL line %d  sanitized() "
            " output \"%s\""
            " expect \"%s\""
            " input  \"%s\"\n",
            line,output,expout,input);
    }
}

const char *s1 = "abcd";
const char *s2 = "ab\ncd";
const char *s3 = "\ncd";
const char *s4 = "ab\n";
const char *s5 = "yy\tcd";
#if 0
    Before October 28 2022 0.5.0 the following test
    would have treated the newline inconsistently from
    the above newline treatments!
#endif /* 0 */
const unsigned char s6[] = {'a',0x1,0x2,0xf4,'\n',0};
const char *s7 = "ab\r\n";

/*  We add in non-printable to test timing for that case */
const char *s8 =
"aaaa bbbb cccc dddd eeee ffff gggg"
"aaaa bbbb cccc dddd eeee ffff gggg"
"aaaa bbbb cccc dddd eeee ffff gggg\v"
"aaaa bbbb cccc dddd eeee ffff gggg"
"aaaa bbbb cccc dddd eeee ffff gggg"
"aaaa bbbb cccc dddd eeee ffff gggg"
"aaaa bbbb cccc dddd eeee ffff gggg\f"
"aaaa bbbb cccc dddd eeee ffff gggg"
"aaaa bbbb cccc dddd eeee ffff gggg"
"aaaa bbbb cccc dddd eeee ffff gggg";

int main(void)
{
    const char *out = 0;
    {
        char *exp = "abcd";
        out = sanitized(s1);
        validate_san(__LINE__,
            exp,out,exp);
    }
    {
        char *exp = "ab\ncd";
        out = sanitized(s2);
        validate_san(__LINE__,
            exp,out,exp);
    }
    {
        char *exp = "\ncd";
        out = sanitized(s3);
        validate_san(__LINE__,
            exp,out,exp);
    }
    {
        char *exp = "ab\n";
        out = sanitized(s4);
        validate_san(__LINE__,
            exp,out,exp);
    }
    {
        char *exp = "yy\tcd";
        out = sanitized(s5);
        validate_san(__LINE__,
            exp,out,exp);
    }
    {
        char* exp = "a%01%02%f4\n";
        out = sanitized((const char *)s6);
        validate_san(__LINE__,
            exp,out,exp);
    }
    {
#ifdef _WIN32
        char* exp = "ab\r\n";
#else
        char* exp = "ab%0d\n";
#endif
        out = sanitized((const char *)s7);
        validate_san(__LINE__,
            exp,out,exp);
    }
#ifdef TIMING
    {
        int i = 0;
        for ( ; i < 5000000; ++i ) {
            out = sanitized((const char *)s8);
            if (i == 2) {
                printf("%s\n",out);
            }
        }
    }
#endif /* TIMING */
    sanitized_string_destructor();
    if (failcount) {
        printf("FAIL sanitized() test\n");
        exit(1);
    }
    printf("PASS sanitized() test\n");
    exit(0);
}
