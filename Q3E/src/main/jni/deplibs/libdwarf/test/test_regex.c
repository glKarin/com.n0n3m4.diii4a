/*   This test code is hereby placed in the public domain. */

#include <config.h>
#include <stdio.h> /* printf() */
#include "dwarf.h"
#include "libdwarf.h"
#include "dd_regex.h"
#include "dd_safe_strcpy.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"

struct glflags_s glflags;

#define DW_DLV_NO_ENTRY -1
#define DW_DLV_OK        0
#define DW_DLV_ERROR     1

static const char *
ename(int v)
{
    switch(v) {
    case DW_DLV_OK:
        return "DW_DLV_OK";
    case DW_DLV_NO_ENTRY:
        return "DW_DLV_NO_ENTRY";
    case DW_DLV_ERROR:
        return "DW_DLV_ERROR";
    }
    return "Unknown return value!!!";
}

static int errcount = 0;
static void
checkres(int res, const char *func,
    const char *re,
    const char *test,int expres,int line)
{
    if (res != expres) {
        printf("FAIL re %s input %s %s=%s exp %s line %d\n",
            re,test,
            func,
            ename(res), ename(expres),
            line);
        ++errcount;
    }
    return;
}

/* Simpler testing */
static void
testx(const char *expr,
    int expv1,
    const char*check,
    int intexpv2,
    int line)
{
    int res = 0;
    if (!expr || !check) {
        printf("Null expression or string. Considered pass.\n");
        return;
    }
    printf("Test %s %s expect=%s expect=%s line %d\n",
        expr,check,ename(expv1),ename(intexpv2),line);
    res = dd_re_comp((char *)expr);
    checkres(res,"dd_re_comp",expr,check,expv1,line);
    if (res) {
        return;
    }
    res = dd_re_exec((char *)check);
    checkres(res,"dd_re_exec",expr,check,intexpv2,line);
}

int
main(void)
{
    testx("u.leb",DW_DLV_OK,"local_dwarf_decode_u",
        DW_DLV_NO_ENTRY,__LINE__);
    testx("u.*leb",DW_DLV_OK,"local_dwarf_decode_u",
        DW_DLV_NO_ENTRY,__LINE__);
    testx("x+",DW_DLV_OK,"x",DW_DLV_OK,__LINE__);
    testx("x*",DW_DLV_OK,"x",DW_DLV_OK,__LINE__);
    testx("x+",DW_DLV_OK,"xx",DW_DLV_OK,__LINE__);
    testx("[fx]*j[0-9]",DW_DLV_OK,"yxxffi123a",
        DW_DLV_NO_ENTRY,__LINE__);
    testx("[fx]*[0-9]",DW_DLV_OK,"yxxffi123a",
        DW_DLV_NO_ENTRY,__LINE__);
    testx("sim",DW_DLV_OK,"simple",DW_DLV_OK,__LINE__);
    testx("simple",DW_DLV_OK,"sim",DW_DLV_NO_ENTRY,__LINE__);
    testx("sim",DW_DLV_OK,"sim",DW_DLV_OK,__LINE__);
    testx("^X.*k$",DW_DLV_OK,"X12345k",DW_DLV_OK,__LINE__);
    testx("X*Y+ijk",DW_DLV_OK,"yxxffi123a",DW_DLV_NO_ENTRY,__LINE__);
    testx("[fx]+i[^0-9a-eA-E]*",DW_DLV_OK,"yxxffiKM",
        DW_DLV_OK,__LINE__);
    testx("[fx]+i[^0-9]*",DW_DLV_OK,"yxxffixyza",DW_DLV_OK,__LINE__);
    testx("[fx]*[0-9]",DW_DLV_OK,"yxxff123a",DW_DLV_OK,__LINE__);
    testx("[fx]*[0-9]+",DW_DLV_OK,"yxxffi123a",
        DW_DLV_NO_ENTRY,__LINE__);
    testx("[fx][0-9]",DW_DLV_OK,"yxxffi123a",
        DW_DLV_NO_ENTRY,__LINE__);
    testx("[fx]+[0-9]",DW_DLV_OK,"yxxffi123a",
        DW_DLV_NO_ENTRY,__LINE__);
    testx("[fx]+i[0-9]*",DW_DLV_OK,"yxxffi123a",DW_DLV_OK,__LINE__);
    testx("u.*leb",DW_DLV_OK,"local_dwarf_decode_u_leb128",
        DW_DLV_OK,__LINE__);
    testx("u.*leb",DW_DLV_OK,"uleblen",DW_DLV_OK,__LINE__);
    testx("u.leb",DW_DLV_OK,"local_dwarf_decode_u_leb128",
        DW_DLV_OK,__LINE__);
    testx("uleb",DW_DLV_OK,"uleblen",DW_DLV_OK,__LINE__);

    testx(".*",DW_DLV_OK,  "abc",DW_DLV_OK,__LINE__);
    testx("abc",DW_DLV_OK, "ab",DW_DLV_NO_ENTRY,__LINE__);
    testx("a[^xy]j",DW_DLV_OK,"ab",DW_DLV_NO_ENTRY,__LINE__);
    testx("a[^xy]j",DW_DLV_OK,"abj",DW_DLV_OK,__LINE__);
    testx("a[xy]j",DW_DLV_OK,"ab",DW_DLV_NO_ENTRY,__LINE__);
    testx( "a[xy]j",DW_DLV_OK,"ayj",DW_DLV_OK,__LINE__);

    testx("a[xy]j",DW_DLV_OK,"afj",DW_DLV_NO_ENTRY,__LINE__);
    testx("a[xyj",DW_DLV_ERROR,"afj",DW_DLV_ERROR,__LINE__);

    /*  A trailing] with no preceeding [ is not
        an error */
    testx("axy]j",DW_DLV_OK,"abc",DW_DLV_NO_ENTRY,__LINE__);
    testx("axy]j",DW_DLV_OK,"axy]j",DW_DLV_OK,__LINE__);
    testx("xy]",DW_DLV_OK,"axy]j",DW_DLV_OK,__LINE__);
    testx("+x",DW_DLV_ERROR,"x",DW_DLV_ERROR,__LINE__);
    testx("x+",DW_DLV_OK,"x",DW_DLV_OK,__LINE__);
    testx("^ax",DW_DLV_OK,"yaxfoo",DW_DLV_NO_ENTRY,__LINE__);
    testx("^ax",DW_DLV_OK,"axfoob",DW_DLV_OK,__LINE__);
    testx(0,DW_DLV_NO_ENTRY,"axfoob",DW_DLV_OK,__LINE__);
    testx("",DW_DLV_NO_ENTRY,"axfoob",DW_DLV_OK,__LINE__);
    testx("+",DW_DLV_ERROR,"axfoob",DW_DLV_NO_ENTRY,__LINE__);
    testx("(+",DW_DLV_OK,"axfoob",DW_DLV_NO_ENTRY,__LINE__);
    testx("(+",DW_DLV_OK,"((",DW_DLV_OK,__LINE__);
    testx("(+)",DW_DLV_OK,"x((()",DW_DLV_OK,__LINE__);
    testx("yy+(",DW_DLV_OK,"yyy(",DW_DLV_OK,__LINE__);
    testx("[fx]+i[0-9]*",DW_DLV_OK,"yxxffi123",DW_DLV_OK,__LINE__);
    testx("a\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\"
        "(\\(\\()))))))))))))))))))))",DW_DLV_OK,"yxxffi123",
        DW_DLV_NO_ENTRY,__LINE__); /* () not special, no groups
        or references to same allowed. */
    testx("[fx]+[0-9]*a",DW_DLV_OK,"yxxff123a",DW_DLV_OK,__LINE__);
    testx("[fx]+i[0-9]*",DW_DLV_OK,"yxxffi123a",DW_DLV_OK,__LINE__);

    testx("[fx]+",DW_DLV_OK,"yxxffi123a",DW_DLV_OK,__LINE__);
    testx("[fx]+i[0-9]+",DW_DLV_OK,"yxxffi123",DW_DLV_OK,__LINE__);
    testx("a[fx]+b[cd]",DW_DLV_OK,"afffbdddy",DW_DLV_OK,__LINE__);
    testx("a[fx]+b[cd]",DW_DLV_OK,"afffdddy",
        DW_DLV_NO_ENTRY,__LINE__);
    if (errcount > 0) {
        printf("\n\nFAIL test_regex errcount %d\n",errcount);
        return 1;
    }
    printf("PASS  test_regex\n");
    return 0;
}
