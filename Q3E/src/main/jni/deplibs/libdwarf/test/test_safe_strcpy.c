/*
  Copyright 2021-2021 David Anderson. All Rights Reserved.

  This trivial test program is hereby placed in the public domain.
*/

#include <config.h>

#include <stdio.h>  /* printf() */
#include <string.h> /* memset() strcmp() */

#include "dwarf_safe_strcpy.h"

static int
test1(char *buf,unsigned blen,
    const char *inp, unsigned ilen,
    const char *expres,
    int line)
{
    _dwarf_safe_strcpy(buf,blen,inp,ilen);
    if (strcmp(buf,expres)) {
        printf("Line %d expected %s got %s\n",
            line,expres,buf);
        return 1;
    }
    return 0;
}

static int
test_safe_strcpy(void)
{
    char buf[200];
    int failcount = 0;

    printf("Entry test_safe_strcpy\n");

    memset(buf,1,sizeof(buf));
    failcount += test1(buf,4,"abc",3,"abc",__LINE__);
    memset(buf,1,sizeof(buf));
    failcount += test1(buf,3,"abc",3,"ab",__LINE__);
    memset(buf,1,sizeof(buf));
    failcount += test1(buf,4,"abcde",5,"abc",__LINE__);
    memset(buf,'z',sizeof(buf));
    buf[1] = 0;
    failcount += test1(buf,0,"abcde",5,"z",__LINE__);
    memset(buf,'z',sizeof(buf));
    failcount += test1(buf,1,"abcde",5,"",__LINE__);
    memset(buf,'z',sizeof(buf));
    failcount += test1(buf,1,"",5,"",__LINE__);
    memset(buf,'z',sizeof(buf));
    failcount += test1(buf,1,"vv",2,"",__LINE__);
    return failcount;
}

int main(void)
{
    int errs = 0;

    errs += test_safe_strcpy();
    if (errs) {
        printf("FAIL. dwarf_safe_strcpy has errors\n");
        return 1;
    }
    printf("PASS dwarf_safe_strcpy tests\n");
    return 0;
}
