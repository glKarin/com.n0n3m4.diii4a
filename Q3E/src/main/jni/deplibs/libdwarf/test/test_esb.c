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

/*  testesb.c
    testing dd_esb.c

*/

#include <config.h>

#include <stdlib.h> /* exit() */
#include <string.h> /* strcmp() */
#include <stdio.h> /* FILE printf, etc */
#include "libdwarf_private.h"
#include "dd_esb.h"
#include "dd_minimal.h"

void dd_minimal_count_global_error(void) {}

static int failcount = 0;

static void
validate_esb(int instance,
    struct esb_s* d,
    size_t explen,
    size_t expalloc,
    const char *expout,
    int line )
{
    (void) explen;
    (void) expalloc;
    if (esb_string_len(d) != explen) {
        ++failcount;
        printf("  FAIL instance %d  esb_string_len()"
            " %u explen %u line %d\n",
            instance,(unsigned)esb_string_len(d),
            (unsigned)explen,line);
    }
    if (strcmp(esb_get_string(d),expout)) {
        ++failcount;
        printf("  FAIL instance %d esb_get_string"
            " %s expstr %s line %d\n",
            instance,esb_get_string(d),expout,line);
    }
}

int main(void)
{
#ifdef _WIN32
    /* Open the null device used during formatting printing */
    if (!esb_open_null_device())
    {
        fprintf(stderr, "esb: Unable to open null device.\n");
        exit(EXIT_FAILURE);
    }
#endif /* _WIN32 */
    {   /*  First lets establish standard sprintf on
        cases of interest. */
        struct esb_s d5;
        char bufs[4];
        esb_int i = -33;

        /* After static alloc we add len, ie, 11 */
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %d bbb",(int)i);
        validate_esb(201,&d5,11,15,"aaa -33 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %6d bbb",(int)i);
        validate_esb(202,&d5,14,18,"aaa    -33 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %6d bbb",6);
        validate_esb(203,&d5,14,18,"aaa      6 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %06d bbb",6);
        validate_esb(204,&d5,14,18,"aaa 000006 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %06u bbb",6);
        validate_esb(205,&d5,14,18,"aaa 000006 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %6u bbb",6);
        validate_esb(206,&d5,14,18,"aaa      6 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %u bbb",12);
        validate_esb(207,&d5,10,14,"aaa 12 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %06x bbb",12);
        validate_esb(208,&d5,14,18,"aaa 00000c bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %6x bbb",12);
        validate_esb(209,&d5,14,18,"aaa      c bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %+d bbb",12);
        validate_esb(210,&d5,11,15,"aaa +12 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %+6d bbb",12);
        validate_esb(211,&d5,14,18,"aaa    +12 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %6d bbb",(int)i);
        validate_esb(212,&d5,14,18,"aaa    -33 bbb",__LINE__);
        esb_destructor(&d5);

    }

    {
        struct esb_s d;
        esb_constructor(&d);
        esb_append(&d,"a");
        validate_esb(1,&d,1,2,"a",__LINE__);
        esb_append(&d,"b");
        validate_esb(2,&d,2,3,"ab",__LINE__);
        esb_append(&d,"c");
        validate_esb(3,&d,3,4,"abc",__LINE__);
        esb_empty_string(&d);
        validate_esb(4,&d,0,4,"",__LINE__);
        esb_destructor(&d);
    }
    {
        struct esb_s d;

        esb_constructor(&d);
        esb_append(&d,"aa");
        validate_esb(6,&d,2,3,"aa",__LINE__);
        esb_append(&d,"bbb");
        validate_esb(7,&d,5,6,"aabbb",__LINE__);
        esb_append(&d,"c");
        validate_esb(8,&d,6,7,"aabbbc",__LINE__);
        esb_empty_string(&d);
        validate_esb(9,&d,0,7,"",__LINE__);
        esb_destructor(&d);
    }
    {
        struct esb_s d;
        static char oddarray[7] = {'a','b',0,'c','c','d',0};

        esb_constructor(&d);
        /*  This used to provoke a msg on stderr. Bad input.
            Now inserts particular string instead. */
        esb_appendn(&d,oddarray,6);
        validate_esb(10,&d,23,24,"ESBERR_appendn bad call",__LINE__);
        /*  The next one just keeps the previous ESBERR*
            and adds a 'c' */
        esb_appendn(&d,"cc",1);
        validate_esb(11,&d,24,25,"ESBERR_appendn bad callc",__LINE__);
        esb_empty_string(&d);
        validate_esb(12,&d,0,25,"",__LINE__);
        esb_destructor(&d);
    }
    {
        struct esb_s d;

        esb_constructor(&d);
        esb_force_allocation(&d,7);
        esb_append(&d,"aaaa i");
        validate_esb(13,&d,6,7,"aaaa i",__LINE__);
        esb_destructor(&d);
    }
    {
        struct esb_s d5;
        const char * s = "insert me %d";

        esb_constructor(&d5);
        esb_force_allocation(&d5,50);
        esb_append(&d5,"aaa ");
        esb_append_printf_i(&d5,s,1);
        esb_append(&d5,"zzz");
        validate_esb(14,&d5,18,50,"aaa insert me 1zzz",__LINE__);
        esb_destructor(&d5);
    }
    {
        struct esb_s d;
        struct esb_s e;
        char* result = NULL;
        esb_constructor(&d);
        esb_constructor(&e);

        esb_append(&d,"abcde fghij klmno pqrst");
        validate_esb(15,&d,23,24,"abcde fghij klmno pqrst",__LINE__);

        result = esb_get_copy(&d);
        esb_append(&e,result);
        validate_esb(16,&e,23,24,"abcde fghij klmno pqrst",__LINE__);
        esb_destructor(&d);
        esb_destructor(&e);
        /*  esb_get_copy returns malloc-d bytes. Must free. */
        free(result);
    }
    {
        struct esb_s d5;
        char bufs[4];
        char bufl[60];
        const char * s = "insert me %d";

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append(&d5,"aaa ");
        esb_append_printf_i(&d5,s,1);
        esb_append(&d5,"zzz");
        validate_esb(17,&d5,18,19,"aaa insert me 1zzz",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufl,sizeof(bufl));
        esb_append(&d5,"aaa ");
        esb_append_printf_i(&d5,s,1);
        esb_append(&d5,"zzz");
        validate_esb(18,&d5,18,60,"aaa insert me 1zzz",__LINE__);
        esb_destructor(&d5);

    }

    {
        struct esb_s d5;
        char bufs[4];
        const char * s = "insert me";

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_s(&d5,"aaa %s bbb",s);
        validate_esb(19,&d5,17,18,"aaa insert me bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_s(&d5,"aaa %12s bbb",s);
        validate_esb(20,&d5,20,21,"aaa    insert me bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_s(&d5,"aaa %-12s bbb",s);
        validate_esb(21,&d5,20,21,"aaa insert me    bbb",__LINE__);
        esb_destructor(&d5);

    }

    {
        struct esb_s d5;
        char bufs[4];
        esb_int i = -33;
        esb_unsigned u = 0;

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %d bbb",i);
        validate_esb(18,&d5,11,12,"aaa -33 bbb",__LINE__);
        esb_destructor(&d5);

        i = -2;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %4d bbb",i);
        validate_esb(19,&d5,12,13,"aaa   -2 bbb",__LINE__);
        esb_destructor(&d5);

        i = -2;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %4d bbb",i);
        validate_esb(20,&d5,12,13,"aaa   -2 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %6d bbb",i);
        validate_esb(21,&d5,14,15,"aaa     -2 bbb",__LINE__);
        esb_destructor(&d5);

        i = -2;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %04d bbb",i);
        validate_esb(22,&d5,12,13,"aaa -002 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %06d bbb",i);
        validate_esb(23,&d5,14,15,"aaa -00002 bbb",__LINE__);
        esb_destructor(&d5);

        i = -200011;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %04d bbb",i);
        validate_esb(24,&d5,15,16,"aaa -200011 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %06d bbb",i);
        validate_esb(25,&d5,15,16,"aaa -200011 bbb",__LINE__);
        esb_destructor(&d5);

        u = 0x80000000;
        u = 0x8000000000000000;
        i = u;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %4d bbb",i);
        validate_esb(26,&d5,28,29,"aaa -9223372036854775808 bbb",
            __LINE__);
        esb_destructor(&d5);

        i = 987665432;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %4d bbb",i);
        validate_esb(27,&d5,17,18,"aaa 987665432 bbb",__LINE__);
        esb_destructor(&d5);

    }
    {
        struct esb_s d5;
        char bufs[4];
        esb_unsigned u = 0;

        u = 37;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %u bbb",u);
        validate_esb(28,&d5,10,11,"aaa 37 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %4u bbb",u);
        validate_esb(29,&d5,12,13,"aaa   37 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %4u bbb",u);
        validate_esb(30,&d5,12,13,"aaa   37 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_u(&d5,"aaa %6u bbb",u);
        validate_esb(31,&d5,14,15,"aaa     37 bbb",__LINE__);
        esb_destructor(&d5);

    }
    {
        struct esb_s d5;
        char bufs[4];
        esb_int i = -33;
        char * middlelength = "0123456789aaaabbbb";

        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %+d bbb",i);
        validate_esb(18,&d5,11,12,"aaa -33 bbb",__LINE__);
        esb_destructor(&d5);

        i = 33;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %+d bbb",i);
        validate_esb(18,&d5,11,12,"aaa +33 bbb",__LINE__);
        esb_destructor(&d5);

        i = -2;
        esb_constructor_fixed(&d5,bufs,sizeof(bufs));
        esb_append_printf_i(&d5,"aaa %+4d bbb",i);
        validate_esb(19,&d5,12,13,"aaa   -2 bbb",__LINE__);
        esb_destructor(&d5);

        esb_constructor(&d5);
        esb_append_printf_s(&d5,"x%-15syyyy",middlelength);
        validate_esb(20,&d5,23,0,"x0123456789aaaabbbbyyyy",__LINE__);
        esb_destructor(&d5);

    }

#ifdef _WIN32
    /* Close the null device used during formatting printing */
    esb_close_null_device();
#endif /* _WIN32 */
    if (failcount) {
        printf("FAIL esb test\n");
        exit(EXIT_FAILURE);
    }
    printf("PASS esb test\n");
    exit(0);
}
