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

/*  Usage:  ./test_errmsg_list.c
    where an env var gives path to source tree */

#include <config.h>

#include <stddef.h> /* size_t */
#include <stdio.h>  /* FILE fclose() fgets() fopen() printf() */
#include <stdlib.h> /* atol() exit() getenv() */
#include <string.h> /* strcmp() strlen() strncmp() */

#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dwarf_base_types.h"
#include "dwarf_safe_strcpy.h"
#include "dwarf_opaque.h"
#include "dwarf_errmsg_list.h"

/* We don't allow arbitrary DW_DLE line length. */
#define MAXDEFINELINE 1000
static char buffer[MAXDEFINELINE];
static char buffer2[MAXDEFINELINE];

/* Arbitrary. A much smaller max length value would work. */
#define MAX_NUM_LENGTH 12

/* return TRUE on error */
static int
check_errnum_mismatches(unsigned i)
{
    unsigned nextstop = 0;
    const char *sp = _dwarf_errmsgs[i];
    const char *cp = sp;
    unsigned innit = FALSE;
    unsigned prevchar = 0;
    unsigned value = 0;

    for ( ; *cp; cp++) {
        unsigned c = 0;
        c = 0xff & *cp;
        if ( c >= '0' && c <= '9' && !innit
            && prevchar != '(') {
            /* Skip. number part of macro name. */
            prevchar = c;
            continue;
        }
        if ( c >= '0' && c <= '9') {
            value = value * 10;
            value += (c - '0');
            nextstop++;
            if (nextstop > MAX_NUM_LENGTH) {
                break;
            }
            innit = TRUE;
        } else {
            if (innit) {
                break;
            }
            prevchar= c;
        }
    }
    if (innit) {
        if (i != value) {
            return TRUE;
        }
        return FALSE;
    }
    /* There is no number to check. Ignore it. */
    printf("mismatch value %d has no errnum to check %s\n",
        i,_dwarf_errmsgs[i]);
    return TRUE;
}

static int
splmatches(char *base, unsigned baselen,char *test)
{
    if (baselen != (unsigned)strlen(test) ) {
        return FALSE;
    }
    for ( ; *test; ++test,++base) {
        if (*test != *base) {
            return FALSE;
        }
    }
    return TRUE;
}

static void
check_dle_list(const char *path)
{
    /*  The format should be
        #define<space>name<spaces>number<spaces>optional-c-comment
        and we are intentionally quite rigid about it all except
        that the number of spaces before any comment is allowed. */
    unsigned linenum = 0;
    unsigned long prevdefval = 0;
    unsigned cur_dle_line = 0;
    unsigned foundlast = 0;
    unsigned foundlouser = 0;
    FILE*fin = 0;

    fin = fopen(path, "r");
    if (!fin) {
        printf("Unable to open define list to read %s\n",path);
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
        if (strncmp(line,"#define DW_DLE_",15)) {
            /* Skip the non- DW_DLE_ lines */
            continue;
        }
        curdefname = line+8;
        /* ASSERT: line ends with NUL byte. */
        for ( ; ; curdefname_len++) {
            if (foundlouser) {
                printf("define line %u has  stuff after "
                    "DW_DLE_LO_USER!\n",
                    linenum);
                exit(EXIT_FAILURE);
            }
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
                break;
            }
        }
        /* strtoul skips leading whitespace. */
        endptr = 0;
        v = strtoul(numstart,&endptr,0);
        /*  This test is a bit odd. But is valid till
            we decide it is inappropriate. */
        if (v > DW_DLE_LO_USER) {
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
        if (splmatches(curdefname,curdefname_len,"DW_DLE_LAST")) {
            if (foundlast) {
                printf("duplicated DW_DLE_LAST! line %u\n",linenum);
                exit(EXIT_FAILURE);
            }
            foundlast = 1;
            if (v != prevdefval) {
                printf("Invalid: Last value mismatch! %lu vs %lu\n",
                    v,prevdefval);
            }
        } else if (splmatches(curdefname,curdefname_len,
            "DW_DLE_LO_USER")) {
            if (!foundlast) {
                printf("error:expected DW_DLE_LO_USER after LAST! "
                    "line %u\n", linenum);
                exit(EXIT_FAILURE);
            }
            if (foundlouser) {
                printf("Error:duplicated DW_DLE_LO_USER! line %u\n",
                    linenum);
                exit(EXIT_FAILURE);
            }
            foundlouser = 1;
            continue;
        } else {
            if (cur_dle_line > 0) {
                if (v != prevdefval+1) {
                    printf("Invalid: Missing value! %lu vs %lu\n",
                        prevdefval,v);
                    exit(EXIT_FAILURE);
                }
            }
            prevdefval = v;
        }
        ++cur_dle_line;
        /* Ignoring rest of line for now. */
    }
    fclose(fin);
}

static void
quoted_length(char * line, unsigned *quotedlen)
{
    char *cp = line;
    unsigned pos = 0;
    unsigned inquotes = FALSE;
    unsigned inquotepos = 0;
    unsigned skiprest = FALSE;
    unsigned qlen = 0;

    for ( ; *cp ; ++cp,++pos) {
        if (skiprest) {
            continue;
        }
        if ( *cp == '"') {
            if (inquotes) {
                qlen = pos - inquotepos;
                skiprest = TRUE;
                continue;
            } else {
                inquotes = TRUE;
                inquotepos = pos;
            }
        }
    }
    if (!inquotes) {
        *quotedlen = qlen; /* empty, not a line we care about */
    } else {
        *quotedlen = qlen +1; /*counting trailing NUL byte */
    }
    *quotedlen = qlen +1; /*counting trailing NUL byte */
    return;
}

static void
read_next_line(FILE *fin,unsigned linenum,unsigned int *quotedlen)
{
    char        *line = 0;
    size_t       linelen = 0;
    unsigned int qlen2 = 0;

    line = fgets(buffer2,MAXDEFINELINE,fin);
    if (!line) {
        printf("inner end file line %u is too long!\n",linenum);
        exit(EXIT_FAILURE);
    }
    linelen = strlen(line);
    line[linelen-1] = 0;
    --linelen;
    if (linelen >= (unsigned long)(MAXDEFINELINE-1)) {
        printf("inner line %u is too long!\n",linenum);
        exit(EXIT_FAILURE);
    }
    quoted_length(line,&qlen2);
    *quotedlen = qlen2;
}

static void
check_msg_lengths(const char *path)
{
    /*  The format should be
        {"DW_DLE_ ...  "},
        or a line pair for longer messages
        {"DW_DLE_ ...  "
            "   "},
        and we are intentionally quite rigid about it all. */
    unsigned linenum = 0;
    unsigned long total_quoted_bytes = 0;
    FILE*fin = 0;
    const char *msglenid = "#define DW_MAX_MSG_LEN ";
    long length_of_errstrings = 0;
    int loop_done = FALSE;
    unsigned long longest_string_length = 0;
    unsigned long longest_string_line = 0;

    fin = fopen(path, "r");
    if (!fin) {
        printf("Unable to open define list to read %s\n",path);
        exit(EXIT_FAILURE);
    }
    for ( ;!loop_done;++linenum) {
        char *line = 0;
        size_t   linelen = 0;
        unsigned quotedlen = 0;
        unsigned quotedlen2 = 0;

        line = fgets(buffer,MAXDEFINELINE,fin);
        if (!line) {
            break;
        }
        linelen = strlen(line);
        line[linelen-1] = 0;
        --linelen;
        if (linelen >= (unsigned)(MAXDEFINELINE-1)) {
            printf("define line %u is too long!\n",linenum);
            exit(EXIT_FAILURE);
        }
        if (!strncmp(line,msglenid,strlen(msglenid))) {
            const char  *cp = line+strlen(msglenid);
            length_of_errstrings = atol(cp);
            continue;
        }
        if (strncmp(line,"{\"DW_DLE_",9)) {
            /* Skip the non- DW_DLE_ lines */
            continue;
        }
        quoted_length(line, &quotedlen);
        if (line[linelen-1] == '"') {
            /* continued on next */
            ++linenum;
            read_next_line(fin,linenum,&quotedlen2);
        } else if (line[linelen-1] ==  ',') {
            /* non-final line */
            if (line[linelen-2] != '}') {
                printf("Unexpected Format at comma "
                    "line %u\n",linenum);
                exit(EXIT_FAILURE);
            }
        } else if (line[linelen-1] ==  '}') {
            /* Final line */
            /* ok */
            loop_done = TRUE;
        } else {
            printf("Unexpected Format at end of line %u\n",linenum);
            exit(EXIT_FAILURE);
        }
        if ((quotedlen +quotedlen2) > longest_string_length) {
            longest_string_length = quotedlen +quotedlen2;
            longest_string_line = linenum;
        }
        total_quoted_bytes += quotedlen +quotedlen2;
    }
    {
        unsigned long arysize = sizeof(_dwarf_errmsgs);
        unsigned long expsize = (DW_DLE_LAST+1) * DW_MAX_MSG_LEN;

        printf("Longest string length   %lu\n",longest_string_length);
        printf("Longest string linenum  %lu\n",longest_string_line);
        printf("Length of quoted areas  %lu\n",total_quoted_bytes);
        printf("Length of static array  %lu\n",
            (DW_DLE_LAST+1)*length_of_errstrings);
        printf("Expected length array   %lu\n", expsize);
        printf("Sizeof() static array   %lu\n", arysize);
        if (arysize != expsize) {
            printf("_dwarf_errmsgs size %lu but "
                "expected size is %lu!.  Error",
                arysize,expsize);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fin);
}

static char pathbuf[2000];
static char pathbuferrm[2000];
static void
local_safe_strcpy(char *targ,char *src,
    unsigned targlen, unsigned srclen)
{
    if (srclen > targlen) {
        printf("Target name does not fit in buffer.\n"
            "In test_errmsg_list.c increase buffer size "
            " from %u \n",(unsigned int)sizeof(pathbuf));
        exit(EXIT_FAILURE);
    }
    _dwarf_safe_strcpy(targ,targlen,src,srclen);
}

/*
    ./test_errmsg_list -f /path.../libdwarf.h
    ./test_errmsg_list -t $top_srcdir

*/
int
main(int argc, char **argv)
{
    unsigned     i       = 0;
    char        *path    = 0;
    char        *errpath = 0;
    size_t       len     = 0;
    const char  *libpath="/src/lib/libdwarf/libdwarf.h";
    const char  *srchdr="/src/lib/libdwarf/dwarf_errmsg_list.h";
    int argn = 0;

    pathbuf[0] = 0;
    if (argc > 1) {
        for (argn = 1; argn < argc; ++argn){
            if (!strcmp(argv[1],"-f")) {
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
        if (!path) {
            printf("Expected environment variable "
                " DWTOPSRCDIR with path of "
                "base directory (usually called 'code')\n");
            exit(EXIT_FAILURE);
        }
    }
    len = strlen(path);
    local_safe_strcpy(pathbuf,path,sizeof(pathbuf),(unsigned)len);
    local_safe_strcpy(pathbuf+len,(char *)libpath,
        (unsigned)(sizeof(pathbuf) -len -1),
        (unsigned)strlen(libpath));

    local_safe_strcpy(pathbuferrm,path,sizeof(pathbuferrm),len);
    local_safe_strcpy(pathbuferrm+len,(char *)srchdr,
        (unsigned)(sizeof(pathbuferrm) -len -1),
        (unsigned)strlen(srchdr));
    path = pathbuf;
    errpath = pathbuferrm;
    check_dle_list(path);
    for ( i = 0; i <= DW_DLE_LAST; ++i) {
        if (check_errnum_mismatches(i)) {
            printf("mismatch value %d is: %s\n",i,_dwarf_errmsgs[i]);
            exit(EXIT_FAILURE);
        }
    }
    check_msg_lengths(errpath);
    /* OK. */
    return 0;
}
