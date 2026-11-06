/*
    Copyright 2009-2010 SN Systems Ltd. All rights reserved.
    Portions Copyright 2009-2020 David Anderson. All rights reserved.

    This program is free software; you can redistribute it
    and/or modify it under the terms of version 2.1 of the
    GNU Lesser General Public License as published by the
    Free Software Foundation.

    This program is distributed in the hope that it would
    be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A
    PARTICULAR PURPOSE.

    Further, this software is distributed without any warranty
    that it is free of the rightful claim of any third person
    regarding infringement or the like.  Any license provided
    herein, whether implied or otherwise, applies only to
    this software file.  Patent licenses, if any, provided
    herein do not apply to combinations of this program with
    other software, or any other product whatsoever.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write
    the Free Software Foundation, Inc., 51 Franklin Street -
    Fifth Floor, Boston MA 02110-1301, USA.

*/
/*   Used by scripts/libbuild.sh. Not in libdwarf itself. */

#include <config.h>
#include <stddef.h> /* NULL size_t */
#include <ctype.h>  /* isspace() */
#include <errno.h>  /* errno */
#include <stdio.h>  /* fgets() fprintf() printf() sscanf() */
#include <stdlib.h> /* exit() qsort() strtoul() */
#include <string.h> /* strchr() strcmp() strcpy() strlen()
    strncmp() */
#include "libdwarf_private.h"
#include "dd_getopt.h"
#include "dwarf_safe_strcpy.h"
#include "dd_minimal.h"

/* works around ref to this via dd_getopt */
void dd_minimal_count_global_error(void) {}

/*  gennames.c
    Prints routines to return constant name for the associated value
    (such as the TAG name string for a particular tag).

    The input is dwarf.h
    For each set of names with a common prefix, we create a routine
    to return the name given the value.
    Also print header file that gives prototypes of routines.
    To handle cases where there are multiple names for a single
    value (DW_AT_* has some due to ambiguities in the DWARF2 spec)
    we take the first of a given value as the definitive name.
    TAGs, Attributes, etc are given distinct checks.

    The doprinting argument is so that when used by tag_tree.c,
    and tag_attr.c that we don't get irritating messages on stderr
    when those dwarfdump build-time applications are run.

    ======== For consistency across the library we
    ======== now only use the #define macro version,
    ======== never the enum version.
    There are multiple output files as some people find one
    form more pleasant than the other.

    ======== It's difficult to believe compilers still
    ======== generate poor switch() code.
    Some compilers generate better code for switch statements than
    others, so the -s and -t options let the user decide which
    is better for their compiler (when building dwarfdump):
    a simple switch or code doing binary search.
    This choice affects the runtime speed of dwarfdump.

*/

static void OpenAllFiles(void);
static void WriteFileTrailers(void);
static void CloseAllFiles(void);
static void GenerateInitialFileLines(void);
static void GenerateOneSet(void);
static void WriteNameDeclarations(void);
#ifdef TRACE_ARRAY
static void PrintArray(void);
#endif /* TRACE_ARRAY */
static int is_skippable_line(char *pLine);
static void ParseDefinitionsAndWriteOutput(void);

/* We don't need really long lines: the input file is simple. */
#define MAX_LINE_SIZE 1000
/*  We don't need a variable array size,
    it just has to be big enough. */
#define ARRAY_SIZE 350

#define MAX_NAME_LEN 64

/* To store entries from dwarf.h */
typedef struct {
    char     ad_prefixname[MAX_NAME_LEN];  /* short name */
    char     ad_name[MAX_NAME_LEN];  /* short name */
    unsigned ad_value;              /* value */
    /* Original spot in array.   Lets us guarantee a stable sort. */
    unsigned ad_original_position;
} array_data;

/*  A group_array is a grouping from dwarf.h.
    All the TAGs are one group, all the
    FORMs are another group, and so on. */
static array_data group_array[ARRAY_SIZE];
static unsigned array_count = 0;

typedef int (*compfn)(const void *,const void *);
static int Compare(array_data *,array_data *);

static const char *prefix_root = "DW_";
static const unsigned prefix_root_len = 3;

/* f_dwarf_in is the input dwarf.h. The others are output files. */
static FILE *f_dwarf_in;
static FILE *f_names_h;
static FILE *f_names_c;

/* Size unchecked, but large enough. */
static char prefix[200] = "";

static const char *usage[] = {
    "Usage: gennames <options>",
    "    -i  <path/src/lib/libdwarf",
    "    -o output-table-path",
    "",
};

static void
print_args(int argc, char *argv[])
{
    int index;
    printf("Arguments: ");
    for (index = 1; index < argc; ++index) {
        printf("%s ",argv[index]);
    }
    printf("\n");
}

static char *program_name = 0;
static char *input_name = 0;
static char *output_name = 0;

static void
print_version(const char * name)
{
#ifdef _DEBUG
    const char *acType = "Debug";
#else
    const char *acType = "Release";
#endif /* _DEBUG */

    printf("%s [%s %s]\n",name,PACKAGE_VERSION,acType);
}

static void
print_usage_message(const char *options[])
{
    int index;
    for (index = 0; *options[index]; ++index) {
        printf("%s\n",options[index]);
    }
}

/* process arguments */
static void
process_args(int argc, char *argv[])
{
    int c = 0;
    int usage_error = FALSE;

    program_name = argv[0];

    while ((c = dwgetopt(argc, argv, "i:o:")) != EOF) {
        switch (c) {
        case 'i':
            input_name = dwoptarg;
            break;
        case 'o':
            output_name = dwoptarg;
            break;
        default:
            usage_error = TRUE;
            break;
        }
    }

    if (usage_error || 1 == dwoptind || dwoptind != argc) {
        print_usage_message(usage);
        exit(EXIT_FAILURE);
    }
}

int
main(int argc,char **argv)
{
    print_version(argv[0]);
    print_args(argc,argv);
    process_args(argc,argv);
    OpenAllFiles();
    GenerateInitialFileLines();
    ParseDefinitionsAndWriteOutput();
    WriteNameDeclarations();
    WriteFileTrailers();
    CloseAllFiles();
    return 0;
}

/* Print the array used to hold the tags, attributes values */
#ifdef TRACE_ARRAY
static void
PrintArray(void)
{
    int i;
    for (i = 0; i < array_count; ++i) {
        printf("%d: Name %s_%s, Value 0x%04x\n",
            i,grouparray[i].ad_prefixname,
            grouparray[i].ad_name,
            grouparray[i].ad_value);
    }
}
#endif /* TRACE_ARRAY */

/* By including original position we force a stable sort */
static int
Compare(array_data *elem1,array_data *elem2)
{
    if (elem1->ad_value < elem2->ad_value) {
        return -1;
    }
    if (elem1->ad_value > elem2->ad_value) {
        return 1;
    }
    if (elem1->ad_original_position < elem2->ad_original_position) {
        return -1;
    }
    if (elem1->ad_original_position > elem2->ad_original_position) {
        return 1;
    }
    return 0;
}

static FILE *
open_path(const char *base, const char *file, const char *direction)
{
    FILE *f = 0;
    /*  POSIX PATH_MAX  would suffice, normally stdio
        BUFSIZ is larger than PATH_MAX */
    static char path_name[BUFSIZ];

    /* 2 == space for / and NUL */
    size_t baselen = strlen(base) +1;
    size_t filelen = strlen(file) +1;
    size_t netlen = baselen + filelen;

    if (netlen >= BUFSIZ) {
        printf("Error opening '%s/%s', name too long\n",base,file);
        exit(EXIT_FAILURE);
    }
    _dwarf_safe_strcpy(path_name,BUFSIZ,
        base,baselen-1);
    _dwarf_safe_strcpy(path_name+baselen-1,BUFSIZ -baselen,
        "/",1);
    _dwarf_safe_strcpy(path_name+baselen,BUFSIZ -baselen -1,
        file,filelen-1);
    f = fopen(path_name,direction);
    if (!f) {
        printf("Error opening '%s'\n",path_name);
        exit(EXIT_FAILURE);
    }
    return f;
}

/* Open files and write the basic headers */
static void
OpenAllFiles(void)
{
    const char *dwarf_h      = "dwarf.h";
    const char *names_h      = "dwarf_names.h";
    const char *names_c      = "dwarf_names.c";

    f_dwarf_in = open_path(input_name,dwarf_h,"r");
    f_names_h = open_path(output_name,names_h,"w");
    f_names_c = open_path(output_name,names_c,"w");
}

static void
GenerateInitialFileLines(void)
{
    /* Generate entries for 'dwarf_names.c' */
    fprintf(f_names_c,"/* Generated routines, do not edit. */\n");
    fprintf(f_names_c,"/* Generated for source version %s */\n",
        PACKAGE_VERSION);
    fprintf(f_names_c,"\n/* BEGIN FILE */\n\n");
    fprintf(f_names_c,"#include \"dwarf.h\"\n\n");
    fprintf(f_names_c,"#include \"libdwarf.h\"\n\n");
}

/* Close files and write basic trailers */
static void
WriteFileTrailers(void)
{
    /* Generate entries for 'dwarf_names.c' */
    fprintf(f_names_c,"\n/* END FILE */\n");
}

static void
CloseAllFiles(void)
{
    fclose(f_dwarf_in);
    fclose(f_names_h);
    fclose(f_names_c);
}

struct NameEntry {
    char ne_name[MAX_NAME_LEN];
};

/*  Sort these by name, then write */
#define MAX_NAMES 200
static struct NameEntry nameentries[MAX_NAMES];
static int  curnameentry;

/*  We compare as capitals for sorting purposes.
    This does not do right by UTF8, but the strings
    are from in dwarf.h and are plain ASCII.  */
static int
CompareName(struct NameEntry *elem1,struct NameEntry *elem2)
{
    char *cpl = elem1->ne_name;
    char *cpr = elem2->ne_name;
    for ( ; *cpl && *cpr; ++cpl,++cpr) {
        unsigned char l = *cpl;
        unsigned char r = *cpr;
        unsigned char l1 = 0;
        unsigned char r1 = 0;

        if (l == r) {
            continue;
        }
        if (l <= 'z' && l >= 'a') {
            l1 = l - 'a' + 'A';
            l = l1;
        }
        if (r <= 'z' && r >= 'a') {
            r1 = r -'a'+ 'A';
            r = r1;
        }
        if (l < r) {
            return -1;
        }
        if (l > r) {
            return 1;
        }
        continue;
    }
    if (*cpl < *cpr) {
        return -1;
    }
    if (*cpl > *cpr) {
        return 1;
    }
    return 0; /* should NEVER happen */
}

/*  Sort into name order for readability of the declarations,
    then print the declarations. */
static void
WriteNameDeclarations(void)
{
    int i = 0;

    qsort((void *)&nameentries,curnameentry,
        sizeof(struct NameEntry),(compfn)CompareName);
    /* Generate entries for 'dwarf_names.h' and libdwarf.h */
    for ( ; i < curnameentry;++i) {
        fprintf(f_names_h,"DW_API int dwarf_get_%s_name"
            "(unsigned int /*val_in*/,\n",nameentries[i].ne_name);
        fprintf(f_names_h,"    const char ** /*s_out */);\n");
    }
}
static void
SaveNameDeclaration(char *prefix_id)
{
    unsigned long length = 0;

    if (curnameentry >= MAX_NAMES) {
        printf("FAIL gennames. Exceeded limit of declarations %d "
            "when given %s\n",curnameentry,prefix_id);
        exit(EXIT_FAILURE);
    }
    length = strlen(prefix_id);
    if (length >= MAX_NAME_LEN) {
        printf("FAIL gennames. Exceeded limit of declaration "
            "name length at %ul "
            "when given %s\n",curnameentry,prefix_id);
        exit(EXIT_FAILURE);
    }
    strcpy(nameentries[curnameentry].ne_name,prefix_id);
    ++curnameentry;
}

/* Write the table and code for a common set of names */
static void
GenerateOneSet(void)
{
    unsigned u;
    unsigned prev_value = 0;
    char * prev_name = "";
    char *prefix_id = prefix + prefix_root_len;

#ifdef TRACE_ARRAY
    printf("List before sorting:\n");
    PrintArray();
#endif /* TRACE_ARRAY */

    /*  Sort the array, because the values in 'libdwarf.h' are not in
        ascending order; if we use '-t' we must be sure the values are
        sorted, for the binary search to work properly.
        We want a stable sort, hence mergesort.  */
    qsort((void *)&group_array,array_count,
        sizeof(array_data),(compfn)Compare);

#ifdef TRACE_ARRAY
    printf("\nList after sorting:\n");
    PrintArray();
#endif /* TRACE_ARRAY */

    SaveNameDeclaration(prefix_id);
    /* Generate code for 'dwarf_names.c' */
    fprintf(f_names_c,"/* ARGSUSED */\n");
    fprintf(f_names_c,"int\n");
    fprintf(f_names_c,"dwarf_get_%s_name (unsigned int val,\n",
        prefix_id);
    fprintf(f_names_c,"    const char ** s_out)\n");
    fprintf(f_names_c,"{\n");
    fprintf(f_names_c,"    switch (val) {\n");

    for (u = 0; u < array_count; ++u) {
        /* Check if value already dumped */
        if (u > 0 && group_array[u].ad_value == prev_value) {
            fprintf(f_names_c,
                "    /*  Skipping alternate spelling of value\n");
            fprintf(f_names_c,
                "        0x%x. %s_%s */\n",
                (unsigned)prev_value,
                prefix,
                group_array[u].ad_name);
            printf("0x%04x: Skip "
                "%s_%s Keep %s_%s\n",
                (unsigned)prev_value,
                prefix,
                group_array[u].ad_name,
                prefix,prev_name);
            continue;
        }
        prev_value = group_array[u].ad_value;
        prev_name = group_array[u].ad_name;

        /* Generate entries for 'dwarf_names.c' */
        fprintf(f_names_c,"    case %s_%s:\n",
            prefix,group_array[u].ad_name);
        fprintf(f_names_c,"        *s_out = \"%s_%s\";\n",
            prefix,group_array[u].ad_name);
        fprintf(f_names_c,"        return DW_DLV_OK;\n");
    }

    /* Closing entries for 'dwarf_names.h' */
    fprintf(f_names_c,"    default: break;\n");
    fprintf(f_names_c,"    }\n");
    fprintf(f_names_c,"    return DW_DLV_NO_ENTRY;\n");
    fprintf(f_names_c,"}\n");
    /* Mark the group_array as empty */
    array_count = 0;
}

/*  Detect empty lines (and other lines we do not want to read) */
static int
is_skippable_line(char *pLine)
{
    int empty = TRUE;

    for (; *pLine && empty; ++pLine) {
        empty = isspace(*pLine);
    }
    return empty;
}

/* Parse the 'dwarf.h' file and generate the tables */
static void
ParseDefinitionsAndWriteOutput(void)
{
    char new_prefix[64];
    char *second_underscore = NULL;
    char type[1000];
    char name[1000];
    char value[1000];
    char extra[1000];
    char line_in[MAX_LINE_SIZE];
    int pending = FALSE;
    int prefix_len = 0;

    /* Process each line from 'dwarf.h' */
    while (!feof(f_dwarf_in)) {
        /*  errno is cleared here so printing errno after
            the fgets is showing errno as set by fgets. */
        char *fgbad = 0;
        errno = 0;
        fgbad = fgets(line_in,sizeof(line_in),f_dwarf_in);
        if (!fgbad) {
            if (feof(f_dwarf_in)) {
                break;
            }
            /*  Is error. errno must be set. */
            fprintf(stderr,"Error reading dwarf.h!. Errno %d\n",
                errno);
            exit(EXIT_FAILURE);
        }
        if (is_skippable_line(line_in)) {
            continue;
        }
        sscanf(line_in,"%s %s %s %s",type,name,value,extra);
        if (strcmp(type,"#define") ||
            strncmp(name,prefix_root,prefix_root_len)) {
            continue;
        }

        second_underscore = strchr(name + prefix_root_len,'_');
        prefix_len = (int)(second_underscore - name);
        _dwarf_safe_strcpy(new_prefix,sizeof(new_prefix),
            name,prefix_len);

        /* Check for new prefix set */
        if (strcmp(prefix,new_prefix)) {
            if (pending) {
                /* Generate current prefix set */
                GenerateOneSet();
            }
            pending = TRUE;
            _dwarf_safe_strcpy(prefix,sizeof(prefix),
                new_prefix,strlen(new_prefix));
        }

        /* Be sure we have a valid entry */
        if (array_count >= ARRAY_SIZE) {
            printf("Too many entries for current "
                "group_array size of %d",ARRAY_SIZE);
            exit(EXIT_FAILURE);
        }
        if (!second_underscore) {
            printf("Line has no underscore %s\n",line_in);
            continue;
        }

        /* Move past the second underscore */
        ++second_underscore;

        {
            unsigned long v = strtoul(value,NULL,16);
            /*  Some values are duplicated, that is ok.
                After the sort we will weed out the duplicate values,
                see GenerateOneSet(). */
            /*  Record current entry */
            if (strlen(second_underscore) >= MAX_NAME_LEN) {
                printf("Too long a name %s for max len %d\n",
                    second_underscore,MAX_NAME_LEN);
                exit(EXIT_FAILURE);
            }
            _dwarf_safe_strcpy(group_array[array_count].ad_name,
                MAX_NAME_LEN,second_underscore,
                strlen(second_underscore));
            group_array[array_count].ad_value = v;
            group_array[array_count].ad_original_position =
                array_count;
            ++array_count;
        }
    }
    if (pending) {
        /* Generate final prefix set */
        GenerateOneSet();
    }
}
