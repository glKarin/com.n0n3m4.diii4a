/*
Copyright (c) 2020, David Anderson
All rights reserved.

This software file is hereby placed in the public domain.
For use by anyone for any purpose.

This code is not in libdwarf itself, it simply checks
some information in dwarf.h and emits  source file
into the libdwarf source directory (which code
gets compiled into libdwarf)
*/

#include <config.h> /* for PACKAGE_VERSION */
#include <stdio.h>  /* FILE fclose() fopen() fprintf() printf() */
#include <stdlib.h> /* exit() getenv() qsort() */
#include <string.h> /* strcmp() strcpy() strncmp() strlen() */

#include "dwarf.h"
#include "dwarf_lname_data.h"
#include "dwarf_string.h"

#define MAXDEFINELINE 1000
#define LNAMEBUFSIZ   1128
static char buffer[LNAMEBUFSIZ];

#define FALSE 0
#define TRUE  1

static unsigned int tcount = sizeof(lnsix) / sizeof(lnsix[0]);
static unsigned int touchcounts[sizeof(lnsix) / sizeof(lnsix[0])];

static void
validatetouchcount(char *curdefname,unsigned long v)
{
    if (v >= tcount) {
        printf("ERROR: For LNAME id 0x%04lx  there is "
            "No count available, limit is %x. Something wrong!\n",
            v,tcount);
        exit(EXIT_FAILURE);
    }
    if (touchcounts[v]) {
        printf("ERROR: For LNAME id 0x%04lx  we are double "
            "counting at count 0x%x . Something wrong!\n",
            v,touchcounts[v]);
        exit(EXIT_FAILURE);
    }
    ++touchcounts[v];
    if (lnsix[v].ln_value !=  v) {
        printf("ERROR: For LNAME id 0x%04lx  not at the "
            "correct entry vs "
            " ln_value 0x%04x\n",
            v,lnsix[v].ln_value);
        exit(EXIT_FAILURE);
    }
    if (strcmp(curdefname,lnsix[v].ln_name)) {
        printf("ERROR: For LNAME id 0x%04lx we find the"
            " wrong string! name %s lnsixname %s \n",
            v, curdefname,lnsix[v].ln_name);
        exit(EXIT_FAILURE);
    }
}

static void
validatefinaltouchcount(void)
{
    unsigned long i = 1;
    /*  Skip [0], it is not real, just a placeholder.
        0 is not a valid LNAME id */
    for ( ; i < tcount; ++i) {
        if (touchcounts[i] != 1) {
            printf(" ERROR: an entry in the LNAMES list "
                " with DW_LNAME id 0x%04lx has touchcount"
                " %u which indicates an error comparing "
                " dwarf.h and the lnsix table here.\n",
                i,touchcounts[i]);
            exit(EXIT_FAILURE);
        }
    }
}

/*  Writes a complete new function
    int dwarf_language_version_data(..)
    in
    dwarf_lname_version.c

    Replacing what was in dwarf_query.c for
    that function.

    We avoid anything in the new source file
    representing date or time as we want
    re-running this (if no changes in dwarf_lname_data.h)
    outputs bit-for-bit identical dwarf_lname_version.c .
*/

#if 0 /* debug dump function */
static void
dump_table(void)
{
    unsigned i = 0;

    printf("lnsix table\n");
    for ( ; i < tcount; ++i) {
        struct lnsix_s * e = &lnsix[i];
        printf("[ %2u] %-16s, %s,0x%04u,%u,%s;\n",
        i,
        e->ln_informal,
        e->ln_name,
        e->ln_value,
        e->ln_low_bound,
        e->ln_vscheme);
    }
    fflush(stdout);
}
#endif /* debug dump function */

static int
qcompar(const void *lh_i,const void *rh_i)
{
    const struct lnsix_s *lh = 0;
    const struct lnsix_s *rh = 0;
    int res = 0;
    int res1 = 0;
    int res2 = 0;

    lh = lh_i;
    rh = rh_i;
    res=strcmp(lh->ln_vscheme,rh->ln_vscheme);
    if (res) {
        return res;
    }
    res1= lh->ln_low_bound - rh->ln_low_bound;
    if (res1) {
        return res1;
    }
    res2=strcmp(lh->ln_name,rh->ln_name);
    return res2;
}

static int
same_switch_part(struct lnsix_s *lh,struct lnsix_s *rh) {
    if (strcmp(lh->ln_vscheme,rh->ln_vscheme)) {
        return FALSE;
    }
    if (lh->ln_low_bound != rh->ln_low_bound) {
        return FALSE;
    }
    return TRUE;
}

static void
print_return_values(FILE *outfile,struct lnsix_s *cur)
{
    fprintf(outfile,"        *dw_default_lower_bound = %u;\n",
        cur->ln_low_bound);
    if (!strcmp(cur->ln_vscheme,"")) {
        fprintf(outfile,"        *dw_version_scheme = 0;\n");
    } else {
        fprintf(outfile,"        *dw_version_scheme = \"%s\";\n",
            cur->ln_vscheme);
    }
    fprintf(outfile,
        "        return DW_DLV_OK;\n");
}

static struct lnsix_s zerosix;
static void
print_table_entries(FILE *outfile)
{
    /* The zero entry is not relevant, a placeholder. */
    unsigned        i = 1;
    struct lnsix_s  sublist_entry;
    struct lnsix_s *cur = 0;
    signed long     sublist_start = -1;
    signed long     k = 0;

    sublist_entry = zerosix;
    for ( ;i < (unsigned)tcount; ++i) {
        if (sublist_start < 0) {
            sublist_entry = lnsix[i];
            sublist_start = i;
            continue;
        }
        cur = &lnsix[i];
        if (same_switch_part(&sublist_entry,cur)) {
            continue;
        }
        /*  now print switch cases   */
        for (k = sublist_start; k < (signed long)i; ++k) {
            fprintf(outfile,"    case %s:\n",
                lnsix[k].ln_name);
        }
        print_return_values(outfile,&sublist_entry);
        sublist_start = i;
        sublist_entry = lnsix[i];
    }
    for (k = sublist_start;k >= 0 &&  k < (signed long)i; ++k) {
        fprintf(outfile,"    case %s:\n",
            lnsix[k].ln_name);
    }
    print_return_values(outfile,&sublist_entry);
    fprintf(outfile,"    default:\n");
    fprintf(outfile,"        break;\n");
}

static void
write_new_query_function(FILE *outfile)
{
    qsort(&lnsix[0],tcount,sizeof(struct lnsix_s), qcompar);
    fprintf(outfile,"/* Generated code, do not edit. */\n");
    fprintf(outfile,"/* Generated for source version %s */\n",
        PACKAGE_VERSION);
    fprintf(outfile, "\n");
    fprintf(outfile, "#include \"dwarf.h\"\n");
    fprintf(outfile, "#include \"libdwarf.h\"\n");

    fprintf(outfile, "int\n");
    fprintf(outfile, "dwarf_language_version_data(\n");
    fprintf(outfile, "    Dwarf_Unsigned dw_lname,\n");
    fprintf(outfile, "    int *dw_default_lower_bound,\n");
    fprintf(outfile, "    const char   **dw_version_scheme)\n");

    fprintf(outfile, "{\n");
    fprintf(outfile, "    switch(dw_lname) {\n");
    print_table_entries(outfile);
    fprintf(outfile, "    }\n");
    fprintf(outfile, "    return DW_DLV_NO_ENTRY;\n");
    /* insert the details here. */
    fprintf(outfile, "}\n");
}

static void
setup_new_query_function(char *path)
{
    FILE *outfile = 0;
    char *outsuffix = "/src/lib/libdwarf/dwarf_lname_version.c";
    struct dwarfstring_s m;
    dwarfstring_constructor(&m);

    dwarfstring_append(&m,path);
    dwarfstring_append(&m,outsuffix);
    outfile = fopen(dwarfstring_string(&m),"w");
    if (!outfile) {
        printf("ERROR building dwarf_lname_version.c FAILED."
            "Unable to open %s for output\n",
            dwarfstring_string(&m));
        dwarfstring_destructor(&m);
        exit(EXIT_FAILURE);
    }
    dwarfstring_destructor(&m);
    write_new_query_function(outfile);
    fclose(outfile);
}

/*  Issue error message and exit there is a mismatch,
    this should never fail.
    It verifies that dwarf.h DW_LNAME values match
    exactly the LNAME values in dwarf_lname_data.h */

static void
check_if_lname_complete(char *path)
{
    FILE         *fin = 0;
    unsigned int  linenum = 1;
    int           loop_done = FALSE;
    const char   *oldop = "#define DW_LNAME_";
    int           oldoplen = strlen(oldop);
    int           langentry = -1;
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
        ++langentry;
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
        /*  v is the DW_LNAM value, 0x0001 for DW_LNAME_Ada */
        if (!lastvalue) {
            /* Nothing to check yet. Is first DW_LNAME. */
            lastvalue = v;
            validatetouchcount(curdefname,v);
            continue;
        }
        if (lastvalue == v) {
            printf("define line %u: DW_LNAME number value "
                " 0x%lx duplicated.\n",
                linenum,lastvalue);
            exit(EXIT_FAILURE);
        }
        if (v != (lastvalue +1)) {
            printf("define line %u: DW_LNAME number "
                " last value  0x%lx after "
                " expected 0x%lx, not 0x%lx",
                linenum,lastvalue,lastvalue+1,v);
            exit(EXIT_FAILURE);
        }
        validatetouchcount(curdefname,v);
        lastvalue = v;
    }
    validatefinaltouchcount();
    fclose(fin);
}

int main(int argc, char**argv)
{
    const char *tailpath = "/src/lib/libdwarf/dwarf.h";
    char  *path  = 0;
    struct dwarfstring_s m;

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
    dwarfstring_constructor(&m);
    dwarfstring_append(&m,path);
    dwarfstring_append(&m,(char *)tailpath);
    check_if_lname_complete(dwarfstring_string(&m));
    setup_new_query_function(path);
    dwarfstring_destructor(&m);
    return 0;
}
