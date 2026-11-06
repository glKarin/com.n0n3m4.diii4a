/*
Copyright (c) 2020, David Anderson
All rights reserved.

This software file is hereby placed in the public domain.
For use by anyone for any purpose.

This code is not in libdwarf itself, it simply checks
some information in dwarf.h for correctness.
*/
#include <stdio.h>  /* FILE fclose() fopen() fprintf() printf() */
#include <stdlib.h> /* exit() getenv() */
#include <string.h> /* strcmp() strcpy() strncmp() strlen() */

#define MAXDEFINELINE 1000
static char *input_name = 0;
static char pathbuf[BUFSIZ];
static char buffer[BUFSIZ];
#define FALSE 0
#define TRUE  1

static void
safe_strcpy(char *targ,char *src,unsigned targlen, unsigned srclen)
{
    if (srclen > targlen) {
        printf("Target name does not fit in buffer.\n"
            "In buildopstabcount.c increase buffer size "
            " from %u \n",(unsigned int)sizeof(buffer));
        exit(EXIT_FAILURE);
    }
    strcpy(targ,src);
}

/*  Issue error message and exit there is a mismatch,
    this should never fail.  */
static void
check_if_dw_lang_complete(char *path)
{
    FILE         *fin = 0;
    unsigned int  linenum = 1;
    int           loop_done = FALSE;
    const char   *oldop = "#define DW_LANG_";
    int           oldoplen = strlen(oldop);
    unsigned long lastvalue = 0;

    fin = fopen(path,"r");
    if (!fin) {
        printf("Unable to open dwarf.h to read %s\n",path);
        exit(EXIT_FAILURE);
    }
    for ( ;!loop_done;++linenum) {
        char *line = 0;
        char * pastname  = 0;
        unsigned int linelen = 0;
        char *curdefname = 0;
        unsigned int curdefnamelen = 0;
        char *endptr     = 0;
        char *numstart   = 0;
        unsigned long v  = 0;

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
        if (strncmp(line,oldop,oldoplen)) {
            /* Not ours. */
            continue;
        }
        /* ASSERT: line ends with NUL byte. */
        curdefname = line+8; /* now past #define */
        for ( ; ; curdefnamelen++) {
            pastname = curdefname +curdefnamelen;
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
        *pastname = 0; /* so curdefname points to only the name */
        /*  Skip spaces. */
        for ( ; *numstart == ' '; ++numstart) { }
        endptr = 0;
        v = strtoul(numstart,&endptr,0);
        if (v == 0x8000 ) {
            /*  DW_LANG_lo_user, we are done with this checkable
                part of the list */
            break;
        }
        if (v == 0 && endptr == numstart) {
            printf("define line %u of %s: number value missing.\n",
                linenum,path);
            printf("Leaving a space as in #define A B 3"
                " in dwarf.h.in will cause this.\n");
            exit(EXIT_FAILURE);
        }
        if (*endptr != ' ' && *endptr != 0) {
            unsigned char e = *endptr;
            printf("define line %u: number value terminates oddly "
                "char: %u 0x%x line %s\n",
                linenum,e,e,line);
            exit(EXIT_FAILURE);
        }
        if (!v) {
            printf("define line %u: DW_LANG number value "
                "zero unreasonable.\n",
                linenum);
            exit(EXIT_FAILURE);
        }
        /*  v is the DW_LANG value, 0x0001 for DW_LANG_C89 */
        if (!lastvalue) {
            /* Nothing to check yet. Is first DW_LANG. */
            lastvalue = v;
            continue;
        }
        if (lastvalue == v) {
            printf("define line %u: DW_LANG number value "
                " 0x%lx duplicated.\n",
                linenum,lastvalue);
            exit(EXIT_FAILURE);
        }
        if (v == 0x002a && ((lastvalue +1) == 0x0029)) {
            /* 0x0029 is an unassigned value in DW_LANG table
            The DW_LANG DWARF6 table lists no language for 0x0029.
            The original proposal on 0x0029 was for
            DW_LANG_Nasm March 2021,
            but on email discussion it was agreed the
            name DW_LANG_Assembly
            was more appropriate and DW_LANG_Assembly was
            added as  0x0031.

            Issue 210115.1 requested Nasm.
            Issue 210208.1 requested DW_LANG_C_plus_plus_17
            and was assigned 0x002a.  */
        } else if (v != (lastvalue +1)) {
            printf("define line %u: DW_LANG number "
                " last value  0x%lx after "
                " expected 0x%lx, not 0x%lx",
                linenum,lastvalue,lastvalue+1,v);
            exit(EXIT_FAILURE);
        }
        lastvalue = v;
    }
    fclose(fin);
}

int main(int argc, char**argv)
{
    const char *tailpath = "/src/lib/libdwarf/dwarf.h";
    char  *path  = 0;
    unsigned len = 0;

    if (argc > 1) {
        if (argc != 3) {
            printf("Expected -f <filename> of base code path\n");
            exit(EXIT_FAILURE);
        }
        if (strcmp(argv[1],"-f")) {
            printf("Expected -f\n");
            exit(EXIT_FAILURE);
        }
        path=argv[2];
    } else {
        /* env var should be set with base path of code */
        path = getenv("DWTOPSRCDIR");
        if (!path) {
            printf("Expected environment variable "
                "DWTOPSRCDIR with path of "
                "base directory (usually called 'code')\n");
            exit(EXIT_FAILURE);
        }
    }
    len = strlen(path);
    if (len >= sizeof(pathbuf)) {
        printf(" buildopstab Input path greater length "
            "than makes any sense:"
            " Giving up\n");
        exit(EXIT_FAILURE);

    }
    safe_strcpy(pathbuf,path,sizeof(pathbuf),len);
    {
        size_t remaining =  sizeof(pathbuf) -len -1;
        size_t tailpathlen = strlen(tailpath);
        if (tailpathlen >= remaining) {
            printf(" buildopstab Input tailpath greater "
                "length fits in buf: "
                "Giving up\n");
            exit(EXIT_FAILURE);
        }
        /* Notice tailpath has a leading /  */
        safe_strcpy(pathbuf+len,(char *)tailpath,
            remaining,tailpathlen);
    }
    input_name = pathbuf;

    check_if_dw_lang_complete(pathbuf);

    return 0;
}
