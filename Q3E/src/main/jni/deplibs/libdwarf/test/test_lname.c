/*
Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*  Usage:  ./test_LNAME
    where an env var gives path to source tree
    or
    ./test_LNAM -f <path to source tree>
*/

#include <config.h>

#include <stddef.h> /* size_t */
#include <stdio.h>  /* FILE fclose() fgets() fopen() printf() */
#include <stdlib.h> /* atol() exit() getenv() */
#include <string.h> /* strcmp() strlen() strncmp() */

#define TRUE  1
#define FALSE 0

/* We don't allow arbitrary DW_DLE line length. */
#define MAXDEFINELINE 1000
static char buffer[MAXDEFINELINE];

/* Arbitrary. A much smaller max length value would work. */
#define MAX_NUM_LENGTH 12

/*  Increase this when the actual number of DW_LNAME_name
    instances approaches this number. */
#define MAX_LNAME_LEN 30
#define LNAME_INSTANCE_MAX  100
struct instance_s {
    char name[MAX_LNAME_LEN];
    int  value;
    int  count;
};

struct instance_s header[LNAME_INSTANCE_MAX];
int              header_count;
struct instance_s csource[LNAME_INSTANCE_MAX];
int              csource_count;

static void
_dwarf_safe_strcpy(char *out,
    size_t outlen,
    const char *in_s,
    size_t inlen)
{
    size_t      full_inlen = inlen+1;
    char       *cpo = 0;
    const char *cpi= 0;
    const char *cpiend = 0;

    if (full_inlen >= outlen) {
        if (!outlen) {
            return;
        }
        cpo = out;
        cpi= in_s;
        cpiend = in_s +(outlen-1);
    } else {
        /*  If outlen is very large
            strncpy is very wasteful. */
        cpo = out;
        cpi= in_s;
        cpiend = in_s +inlen;
    }
    for ( ; *cpi && cpi < cpiend ; ++cpo, ++cpi) {
        *cpo = *cpi;
    }
    *cpo = 0;
}

static void
check_for_dup(struct instance_s *ary,int count,
    char *name, const char *msg)
{
    struct instance_s *cur = ary;
    int k = 0;

    for ( ; k < count; ++k,++cur) {
        if (!strcmp(cur->name,name)) {
            printf("FAIL found %s duplicated %s\n",
                name,msg);
            exit(EXIT_FAILURE);
        }
    }
}

/* return TRUE on error */
static void
check_hdr_match(void)
{
    int k = 0;
    int l = 0;
    int foundh = FALSE;
    int foundc = FALSE;

    if (header_count != csource_count) {
        printf("FAIL LNAME Counts mismatch %d %d\n",
            header_count,csource_count);
        exit(EXIT_FAILURE);
    }
    for ( ; k < header_count; ++k) {
        struct instance_s * curh = &header[k];
        foundh = FALSE;
        foundc = FALSE;

        for (l = 0 ; l < csource_count; ++l) {
            struct instance_s * curc = &csource[l];

            if (strcmp(curh->name,curc->name)) {
                continue;
            }
            foundc= TRUE;
            foundh= TRUE;
            curc->count++;
            if (curc->count > 1) {
                printf("FAIL DUP %s in csource count %d\n",
                    curc->name,curc->count);
                exit(EXIT_FAILURE);
            }
            break;
        }
        if (!foundc) {
            printf("FAIL to find curc %s in csource\n",
                curh->name);
            exit(EXIT_FAILURE);
        }
        if (!foundh) {
            printf("FAIL to find curh %s in csource\n",
                curh->name);
            exit(EXIT_FAILURE);
        }
    }
}

/*  Fill in struct instance_s csource[LNAME_INSTANCE_MAX] */

static void
read_lname_csrc(char *path)
{

    /*  The format should be
        case<space>name<colon> */
    unsigned linenum = 0;
    FILE*fin = 0;

    fin = fopen(path, "r");
    if (!fin) {
        printf("Unable to open define csource to read %s\n",path);
        exit(EXIT_FAILURE);
    }
    for ( ;;++linenum) {
        char   *line = 0;
        size_t  linelen = 0;
        char   *curdefname = 0;
        char   *pastname = 0;
        unsigned curdefname_len = 0;
        int    name_ok = FALSE;

        line = fgets(buffer,MAXDEFINELINE,fin);
        if (!line) {
            break;
        }
        linelen = strlen(line);
        /*  Turn the newline into a NUL */
        line[linelen-1] = 0;
        --linelen;
        if (linelen >= (unsigned)(MAXDEFINELINE-1)) {
            printf("case line %u is too long!\n",linenum);
            exit(EXIT_FAILURE);
        }
        if (strncmp(line,"    case DW_LNAME_",18)) {
            /* Skip the non- case DW_LNAME lines */
            continue;
        }
        curdefname = line+9;
        /* ASSERT: line ends with NUL byte. */
        for ( ; ; curdefname_len++) {
            pastname = curdefname +curdefname_len;
            if (!*pastname) {
                /* At end of line. Missing value. */
                printf("WARNING: csrc case name has no comment"
                    "%d %s\n", linenum,path);
                continue;
            }
            if (*pastname == ' ' || *pastname == ':') {
                /* Ok. Now insert into table. */
                name_ok = TRUE;
                --curdefname_len;
                *pastname = 0;
                break;
            }
        }
        if (!name_ok) {
            printf(" Fail checking case  %d src line %d\n",
                csource_count,__LINE__);
            exit(EXIT_FAILURE);
        }
        if (strlen(curdefname)+1 >=  MAX_LNAME_LEN) {
            printf("Name %s is too long for table at %u\n",
                curdefname,
                (unsigned)strlen(curdefname));
        }
        strcpy(csource[csource_count].name,curdefname);
        csource[csource_count].value = 0;
        csource[csource_count].count = 0;
        check_for_dup(csource,csource_count,curdefname,"in C");
        ++csource_count;
        if (csource_count >= LNAME_INSTANCE_MAX) {
            printf("FAIL. No more room in table, line %d\n",__LINE__);
            exit(EXIT_FAILURE);
        }
        /* Ignoring rest of line */
    }
    fclose(fin);
}
/* Fill in  header[LNAME_INSTANCE_MAX] */
static void
read_lname_hdr(char *path)
{
    /*  The format should be
        #define<space>name<spaces>number<spaces>optional-c-comment
        and we are intentionally quite rigid about it all except
        that the number of spaces before any comment is allowed. */
    unsigned linenum = 0;
    FILE*fin = 0;

    fin = fopen(path, "r");
    if (!fin) {
        printf("Unable to open define dwarf.h to read %s\n",path);
        exit(EXIT_FAILURE);
    }
    for ( ;;++linenum) {
        char   *line = 0;
        size_t  linelen = 0;
        char   *curdefname = 0;
        char   *pastname = 0;
        unsigned curdefname_len = 0;
        char   *numstart = 0;
        char   *endptr = 0;
        unsigned long v = 0;
        int name_ok = FALSE;

        line = fgets(buffer,MAXDEFINELINE,fin);
        if (!line) {
            break;
        }
        linelen = strlen(line);
        /*  Turn the newline into a NUL */
        line[linelen-1] = 0;
        --linelen;
        if (linelen >= (unsigned)(MAXDEFINELINE-1)) {
            printf("define line %u is too long!\n",linenum);
            exit(EXIT_FAILURE);
        }
        if (strncmp(line,"#define DW_LNAME_",17)) {
            /* Skip the non- DW_DLE_ lines */
            continue;
        }
        curdefname = line+8;
        /* ASSERT: line ends with NUL byte. */
        for ( ; ; curdefname_len++) {
            pastname = curdefname +curdefname_len;
            if (!*pastname) {
                /* At end of line. Missing value. */
                printf("define line %u of %s: has no number value!\n",
                    linenum,path);
                exit(EXIT_FAILURE);
            }
            if (*pastname == ' ') {
                /* Ok. Now look for value. */
                numstart = pastname + 1;
                *pastname = 0;
                name_ok = TRUE;
                break;
            }
        }
        if (!name_ok) {
            printf("Name not terminated. error line %d\n",__LINE__);
        }
        /* strtoul skips leading whitespace. */
        endptr = 0;
        v = strtoul(numstart,&endptr,0);
        /*  This test is a bit odd. But is valid till
            we decide it is inappropriate. */
        if (v > LNAME_INSTANCE_MAX) {
            printf("define line %u: number value unreasonable. %lu\n",
                linenum,v);
            exit(EXIT_FAILURE);
        }
        if (v == 0 && endptr == numstart) {
            printf("define line %u of %s: number value missing.\n",
                linenum,path);
            printf("Leaving a space as in #define A B 3"
                " in libdwarf.h.in will cause this.\n");
            exit(EXIT_FAILURE);
        }
        if (*endptr != ' ' && *endptr != 0) {
            unsigned char e = *endptr;
            printf("define line %u: number value terminates oddly "
                "char: %u 0x%x line %s\n",
                linenum,e,e,line);
            exit(EXIT_FAILURE);
        }
        if ((header_count +1) != (int)v) {
            printf(" Fail checking defines: count %d v %d line %d\n",
                header_count,(int)v, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (strlen(curdefname)+1 >= MAX_LNAME_LEN) {
            printf("Name %s is too long for table at %d\n",
                curdefname,
                (int)strlen(curdefname));
        }
        strcpy(header[header_count].name,curdefname);
        check_for_dup(header,header_count,curdefname,"in dwarf.h");
        header[header_count].value = v;
        ++header_count;
        if (header_count >= LNAME_INSTANCE_MAX) {
            printf("FAIL. No more room in header table, line %d\n",
                __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Ignoring rest of line */
    }
    fclose(fin);
}

static char pathbufhdr[2000];
static char pathbufc[2000];
static void
local_safe_strcpy(char *targ,char *src,
    unsigned targlen, unsigned srclen)
{
    if (srclen > targlen) {
        printf("Target name does not fit in buffer.\n"
            "In test_errmsg_list.c increase buffer size "
            " from %u \n",(unsigned int)sizeof(pathbufhdr));
        exit(EXIT_FAILURE);
    }
    _dwarf_safe_strcpy(targ,targlen,src,srclen);
}

/*
    ./test_LNAME -f $top_srcdir
*/
int
main(int argc, char **argv)
{
    char        *path    = 0;
    size_t       len     = 0;
    const char  *libpath="/src/lib/libdwarf/dwarf.h";
    const char  *srchdr="/src/lib/libdwarf/dwarf_lname_version.c";
    int argn = 0;

    if (argc > 1) {
        for (argn = 1; argn < argc; ++argn){
            if (!strcmp(argv[argn],"-f")) {
                argn += 1;
                if (argn >= argc) {
                    printf("test_errmsglist: -f missing file path");
                    exit(EXIT_FAILURE);
                }
                path=argv[argn];
            } else {
                printf("Expected -f \n");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        /* env var should be set with base path of code */
        path = getenv("DWTOPSRCDIR");
    }
    if (!path) {
        printf("Expected environment variable "
            " DWTOPSRCDIR or -f arg with path of "
            "base directory (usually called 'code')\n");
        exit(EXIT_FAILURE);
    }
    len = strlen(path);
    local_safe_strcpy(pathbufhdr,path,sizeof(pathbufhdr),
        (unsigned)len);
    local_safe_strcpy(pathbufhdr+len,(char *)libpath,
        (unsigned)(sizeof(pathbufhdr) -len -1),
        (unsigned)strlen(libpath));

    local_safe_strcpy(pathbufc,path,sizeof(pathbufc),len);
    local_safe_strcpy(pathbufc+len,(char *)srchdr,
        (unsigned)(sizeof(pathbufc) -len -1),
        (unsigned)strlen(srchdr));

    read_lname_hdr(pathbufhdr);
    read_lname_csrc(pathbufc);
    check_hdr_match();
    /* OK. */
    return 0;
}
