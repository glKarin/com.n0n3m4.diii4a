/*
Copyright (C) 2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2011-2019 David Anderson. All Rights Reserved.
Portions Copyright 2012 SN Systems Ltd. All rights reserved.

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

#include <config.h>

#include <ctype.h>  /* isspace() */
#include <stdlib.h> /* exit() free() getenv() realloc() strtoul() */
#include <string.h> /* memset() strcmp() strlen()
    strncmp() strrchr()*/
#include <stdio.h> /* FILE decl for dd_esb.h */

/* Windows specific header files */
#ifdef _WIN32
#ifdef HAVE_STDAFX_H
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#endif /* _WIN32 */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_dwconf.h"
#include "dd_canonical_append.h"
#include "dd_makename.h"
#include "dd_sanitized.h"
#include "dd_esb.h"
#include "dd_safe_strcpy.h"

/* The nesting level is arbitrary,  2 should suffice.
   But at least this prevents an infinite loop.
*/
#define MAX_NEST_LEVEL 3

struct token_s {
    unsigned tk_len;
    char *tk_data;
};
enum linetype_e {
    LT_ERROR,
    LT_COMMENT,
    LT_BLANK,
    LT_BEGINABI,
    LT_REG,
    LT_FRAME_INTERFACE,
    LT_CFA_REG,
    LT_INITIAL_REG_VALUE,
    LT_SAME_VAL_REG,
    LT_UNDEFINED_VAL_REG,
    LT_REG_TABLE_SIZE,
    LT_ADDRESS_SIZE,
    LT_INCLUDEABI,
    LT_ENDABI,
    LT_OPTION
};

struct comtable_s {
    enum linetype_e type;
    char *name;
    size_t namelen;
};

static int errcount = 0; /* Count errors found in this scan of
    the configuration file. */

static char name_begin_abi[] = "beginabi:";
static char name_reg[] = "reg:";
static char name_frame_interface[] = "frame_interface:";
static char name_cfa_reg[] = "cfa_reg:";
static char name_initial_reg_value[] = "initial_reg_value:";
static char name_same_val_reg[] = "same_val_reg:";
static char name_undefined_val_reg[] = "undefined_val_reg:";
static char name_reg_table_size[] = "reg_table_size:";
static char name_address_size[] = "address_size:";
static char name_includeabi[] = "includeabi:";
static char name_endabi[] = "endabi:";
static char name_option[] = "option:";

/*  The namelen field is filled in at runtime with
    the correct value. Filling in a fake '1' avoids
    a compiler warning. */
static struct comtable_s comtable[] = {
    {LT_BEGINABI, name_begin_abi,1},
    {LT_REG, name_reg,1},
    {LT_FRAME_INTERFACE, name_frame_interface,1},
    {LT_CFA_REG, name_cfa_reg,1},
    {LT_INITIAL_REG_VALUE, name_initial_reg_value,1},
    {LT_SAME_VAL_REG, name_same_val_reg,1},
    {LT_UNDEFINED_VAL_REG, name_undefined_val_reg,1},
    {LT_REG_TABLE_SIZE, name_reg_table_size,1},
    {LT_ADDRESS_SIZE, name_address_size,1},
    {LT_INCLUDEABI, name_includeabi,1},
    {LT_ENDABI, name_endabi,1},
    {LT_OPTION, name_option,1},
};

struct conf_internal_s {
    unsigned long beginabi_lineno;
    unsigned long frame_interface_lineno;
    unsigned long initial_reg_value_lineno;
    unsigned long reg_table_size_lineno;
    unsigned long address_size_lineno;
    unsigned long same_val_reg_lineno;
    unsigned long undefined_val_reg_lineno;
    unsigned long cfa_reg_lineno;
    unsigned long regcount;
    struct dwconf_s * conf_out;
    const char * conf_name_used;
    char ** conf_defaults;
};
static void
init_conf_internal(struct conf_internal_s *s,
    struct dwconf_s * conf_out)
{
    s->beginabi_lineno = 0;
    s->frame_interface_lineno = 0;
    s->initial_reg_value_lineno = 0;
    s->reg_table_size_lineno = 0;
    s->address_size_lineno = 0;
    s->same_val_reg_lineno = 0;
    s->undefined_val_reg_lineno = 0;
    s->cfa_reg_lineno = 0;
    s->cfa_reg_lineno = 0;
    s->conf_name_used = 0;
    s->conf_defaults = 0;
    s->regcount = 0;
    s->conf_out = conf_out;
}

static unsigned size_of_comtable = sizeof(comtable) /
    sizeof(comtable[0]);

static FILE *find_a_file(const char *named_file, char **defaults,
    const char** name_used);
static int find_abi_start(FILE * stream, const char *abi_name,
    long *offset,
    unsigned long *lineno_out);
static int parse_abi(FILE * stream, const char *fname,
    const char *abiname,
    struct conf_internal_s *out, unsigned long lineno,
    unsigned nest_level);
static char *get_token(char *cp, struct token_s *outtok);

/*  We insist that the dwarfdump.conf file
    contain ASCII only and reject if non-ascii
    characters are found. */
static int
is_this_text_file(FILE *stream)
{
    unsigned char buf[512];
    size_t bytesread = 0;
    unsigned long i = 0;
    unsigned long curlinelen = 0;
    unsigned long int maxlinelen = 0;

    bytesread = fread(buf,1,sizeof(buf),stream);
    if (bytesread != sizeof(buf)) {
        if (bytesread < 100) {
            printf("ERROR: Found the configure file is too small to "
                "be reasonable. Add a few comment lines "
                "to enlarge it from %lu bytes to 100\n",
                (unsigned long)bytesread);
            glflags.gf_count_major_errors++;
            return FALSE;
        }
    }
    for ( i = 0; i < bytesread; ++i,++curlinelen) {
        unsigned char c = buf[i];

        if (c == '\n') {
            if (curlinelen > maxlinelen) {
                maxlinelen = curlinelen;
            }
            curlinelen = 0;
            continue;
        }
        if (c == '\r' || c == '\t') {
            continue;
        }
        if (c >= ' ' && c <= '}') {
            continue;
        } else {
            printf("Found non-ascii character 0x%02x "
                "and only ascii allowed in dwarfdump.conf\n",
                c);
            return FALSE;
        }
    }
    if (maxlinelen > 100) {
        printf("ERROR: Found maximum configure file"
            " line length of %lu"
            " which seems too long to be reasonable\n",
            maxlinelen);
        glflags.gf_count_major_errors++;
        return FALSE;
    }
    return TRUE;
}

/*  This finds a dwarfdump.conf file and
    then parses it.  It updates
    conf_out as appropriate.

    This finds the first file (looking in a set of places)
    with that name.  It then looks for the right  ABI entry.
    If the first file it finds does not have that ABI entry it
    gives up.

    It would also be reasonable to search every
    'dwarfdump.conf'
    it finds for the abi.
    But we stop at the first dwarfdump.conf
    we find.
    This is the internal call to get the
    conf data to implement
    a crude 'includeabi' feature.

    Each call here starts at offset 0 of
    a new stream.

    Returns 0 if no errors found, else returns > 0.
*/
static int
find_conf_file_and_read_config_inner(const char *named_file,
    const char *named_abi,
    struct conf_internal_s *conf_internal,
    unsigned nest_level)
{

    FILE *conf_stream = 0;
    const char *name_used = 0;
    long offset = 0;
    int res = FALSE;
    unsigned long lineno = 0;

    errcount = 0;

    conf_stream = find_a_file(named_file,
        conf_internal->conf_defaults,
        &name_used);
    if (!conf_stream) {
        ++errcount;
        if (!named_file || !strlen(named_file)) {
            printf("ERROR: dwarfdump found no dwarfdump.conf file "
                "in any of the standard places\n"
                "(precede all arguments "
                "with option --show-dwarfdump-conf to see "
                "what file paths tried)\n"
                "If no print/check arguments also provided "
                "dwarfdump may silently just stop.\n");
            glflags.gf_count_major_errors++;
        } else {
            printf("ERROR: dwarfdump found no dwarfdump.conf "
                "file \"%s\"\n"
                "If no print/check arguments also provided "
                "dwarfdump may silently just stop.\n",
                sanitized(named_file));
            glflags.gf_count_major_errors++;
        }
        return FOUND_ERROR;
    }
    if (glflags.gf_show_dwarfdump_conf) {
        printf("dwarfdump using configuration "
            "file \"%s\"\n", sanitized(name_used));
    }
    conf_internal->conf_name_used = name_used;

    res = is_this_text_file(conf_stream);
    if (res == FALSE) {
        printf("ERROR: dwarfdump configuration "
            "file \"%s\" is not plain text. Error.\n",
            sanitized(name_used));
        glflags.gf_count_major_errors++;
        fclose(conf_stream);
        return FOUND_ERROR;
    }
    res = fseek(conf_stream,0,SEEK_SET);
    if (res != 0) {
        printf("ERROR: dwarfdump configuration "
            "file \"%s\" fseek to 0 failed. Error\n",
            sanitized(name_used));
        glflags.gf_count_major_errors++;
        fclose(conf_stream);
        return FOUND_ERROR;
    }
    /*  And option: lines must come before any abi data. */
    res = find_abi_start(conf_stream, named_abi, &offset, &lineno);
    if (res == FOUND_DONE ||res == FOUND_OPTION) {
        /* nothing more to do */
        fclose(conf_stream);
        return FOUND_DONE;
    }
    if (res == FOUND_ERROR) {
        ++errcount;
        fclose(conf_stream);
        printf("ERROR: dwarfdump found no usable abi \"%s\" "
            "in file \"%s\".\n",
            named_abi?named_abi:"<not looking for abi>",
            name_used);
        glflags.gf_count_major_errors++;
        return FOUND_ERROR;
    }
    /* res == FOUND_ABI_START */
    res = fseek(conf_stream, offset, SEEK_SET);
    if (res != 0) {
        ++errcount;
        fclose(conf_stream);
        printf("ERROR: dwarfdump seek to %ld offset in %s failed!\n",
            offset, name_used);
        glflags.gf_count_major_errors++;
        return FOUND_ERROR;
    }
    parse_abi(conf_stream, name_used, named_abi,
        conf_internal, lineno,
        nest_level);
    fclose(conf_stream);
    if (errcount) {
        return FOUND_ERROR;
    }
    return FOUND_ABI_START;
}

/* This is the external-facing call to get the conf data. */
int
find_conf_file_and_read_config(const char *named_file,
    const char *named_abi, char **defaults,
    struct dwconf_s *conf_out)
{
    int res = 0;
    struct conf_internal_s conf_internal;
    init_conf_file_data(conf_out);
    init_conf_internal(&conf_internal,conf_out);
    conf_internal.conf_defaults = defaults;
    res =  find_conf_file_and_read_config_inner(named_file,
        named_abi,
        &conf_internal,0);
    /*  Nothing in conf_internal need be freed */
    return res;
}

/*  Try to find a file as named and open for read.
    We treat each name as a full name, we are not
    combining separate name and path components.
    This is  an arbitrary choice...

    The defaults are listed in command_options.c in the array
    config_file_defaults[].

    Since named_file comes from an esb_get_string, lname
    may be non-null but pointing to empty string to mean
    no-name.
*/
static FILE *
find_a_file(const char *named_file,
    char **defaults,
    const char ** name_used)
{
    FILE *fin = 0;
    const char *lname = named_file;
    const char *type = "r";
    int i = 0;

    if (lname && (strlen(lname) > 0)) {
        /*  Name given, just assume it is fully correct,
            try no other. */
        printf("dwarfdump looking for"
            " configuration as \"%s\"\n", lname);
        fin = fopen(lname, type);
        if (fin) {
            *name_used = makename(lname);
            return fin;
        }
        return 0;
    }

#ifdef _WIN32
    /*  Open the configuration file, located
        in the directory where the tool is loaded from */
    {
        static char szPath[MAX_PATH];
        if (GetModuleFileName(NULL,szPath,MAX_PATH)) {
            char *pDir = strrchr(szPath,'/');
            size_t len = 0;
            if (!pDir) {
                pDir = strrchr(szPath,'\\');
                if (!pDir) {
                    /* No file was found */
                    goto try_unix_path;
                }
            }
            /* Add the configuration name to the pathname */
            ++pDir;
            len = pDir - szPath;
            dd_safe_strcpy(pDir,sizeof(szPath)-len,
                "dwarfdump.conf",14);
            lname = szPath;

            if (!lname || !strlen(lname)) {
                lname="<Impossible file name string>";
            }
            if (glflags.gf_show_dwarfdump_conf) {
                printf("dwarfdump looking for"
                    " configuration as: \"%s\"\n", lname);
            }
            fin = fopen(lname, type);
            if (fin) {
                *name_used = makename(lname);
                return fin;
            }
            /* No file was found */
            goto try_unix_path;
        }
    }

    try_unix_path:
#endif

    /* No name given, find a default, if we can. */
    for (i = 0; defaults[i]; ++i) {
        char buf[2000];

        buf[0] = 0;
        lname = defaults[i];
        if (strncmp(lname, "HOME/", 5) == 0) {
            /* arbitrary size */
            char *homedir = getenv("HOME");
            if (homedir) {
                char *cp = _dwarf_canonical_append(buf,
                    sizeof(buf), homedir, lname + 5);
                if (!cp) {
                    /* OOps, ignore this one. */
                    continue;
                }
                lname = buf;
            }
        }
        if (glflags.gf_show_dwarfdump_conf) {
            if (!lname || !strlen(lname)) {
                lname="<Impossible name  string>";
            }
            printf("dwarfdump looking for"
                " configuration as: \"%s\"\n", lname);
        }
        fin = fopen(lname, type);
        if (fin) {
            *name_used = makename(lname);
            return fin;
        }
    }
    return 0;
}

/*  Start at a token begin, see how long it is,
    return length. */
static unsigned
find_token_len(char *cp)
{
    unsigned len = 0;

    for (; *cp; ++cp) {
        if (isspace(*cp)) {
            return len;
        }
        if (*cp == '#') {
            return len;         /* begins comment */
        }
        ++len;
    }
    return len;
}

/*
   Skip past all whitespace: the only code that even knows
   what whitespace is.
*/
static char *
skipwhite(char *cp)
{
    for (; *cp; ++cp) {
        if (!isspace(*cp)) {
            return cp;
        }
    }
    return cp;
}

/* Return TRUE if ok. FALSE if find more tokens.
   Emit error message if error.
*/
static int
ensure_has_no_more_tokens(char *cp,
    const char *fname,
    unsigned long lineno)
{
    struct token_s tok;

    get_token(cp, &tok);
    if (tok.tk_len > 0) {
        printf("dwarfdump.conf error: "
            "extra characters after command operands, found "
            "\"%s\" in %s line %lu\n", tok.tk_data, fname, lineno);
        ++errcount;
        return FALSE;
    }
    return TRUE;
}

/*  There may be many  beginabi: lines in a dwarfdump.conf file,
    find the one we want and return its file offset.
*/
static int
find_abi_start(FILE * stream, const char *abi_name,
    long *offset, unsigned long *lineno_out)
{
    char buf[100];
    unsigned long lineno = 0;

    for (; !feof(stream);) {
        struct token_s tok;
        char *line = 0;
        long loffset = ftell(stream);

        line = fgets(buf, sizeof(buf), stream);
        ++lineno;
        if (!line) {
            if (!abi_name) {
                return FOUND_DONE;
            }
            ++errcount;
            return FOUND_ERROR;
        }
        line = get_token(buf, &tok);
        if (!strcmp(tok.tk_data, "option:")) {
            get_token(line, &tok);
            if (tok.tk_data && !strcmp(tok.tk_data,
                "--format-expr-ops-joined")) {
                /*  print expr ops joined onto one line:
                    matching historical behavior. */
                glflags.gf_expr_ops_joined = TRUE;
            } else {
                printf("ERROR: option command %s is not understood"
                    " giving up\n",tok.tk_data);
                ++errcount;
                return FOUND_ERROR;
            }
            continue;
        }
        if (strcmp(tok.tk_data, "beginabi:")) {
            continue;
        }
        /*  Is beginabi: */
        if (!abi_name) {
            continue;
        }
        get_token(line, &tok);
        if (strcmp(tok.tk_data, abi_name)) {
            continue;
        }
        *offset = loffset;
        *lineno_out = lineno;
        return FOUND_ABI_START;
    }
    if (!abi_name) {
        /* All is ok */
        return FOUND_DONE;
    }
    ++errcount;
    return FALSE;
}

/*  Turn a non-delimited
    input char array into a NUL-terminated C string
    (with the help of makename() to get a permanent
    address for the result ing string).
*/
static char *
build_string(unsigned tlen, char *cp)
{
    struct esb_s x;
    char buffer[32];
    char *ret = 0;

    esb_constructor_fixed(&x,buffer,sizeof(buffer));
    esb_appendn(&x,cp,tlen);
    ret = makename(esb_get_string(&x));
    esb_destructor(&x);
    return ret;
}

/*  The tokenizer for our simple parser.
*/
static char *
get_token(char *cp, struct token_s *outtok)
{
    char *lcp = skipwhite(cp);
    unsigned tlen = find_token_len(lcp);
    static int outofmem = FALSE;

    outtok->tk_len = tlen;
    if (tlen > 0) {
        char *src = build_string(tlen, lcp);
        if (!src) {
            if (!outofmem) {
                printf("Dwarfdump out of memory reading "
                    "dwarfdump.conf and will likely not work.\n");
            }
            outofmem = TRUE;
            return "";
        }
        outtok->tk_data = build_string(tlen, lcp);
    } else {
        outtok->tk_data = "";
    }
    return lcp + tlen;

}

/*
    We can't get all the field set up statically very easily,
    so we get the command string length set here.
*/
static void
finish_comtable_setup(void)
{
    unsigned i;

    for (i = 0; i < size_of_comtable; ++i) {
        comtable[i].namelen = strlen(comtable[i].name);
    }
}

/*
    Given  a line of the table, determine if it is a command
    or not, and if a command, which one is it.
    Return LT_ERROR if it's not recognized.
*/
static enum linetype_e
which_command(char *cp, struct comtable_s **tableentry)
{
    unsigned i = 0;
    struct token_s tok;

    if (*cp == '#')
        return LT_COMMENT;
    if (!*cp)
        return LT_BLANK;

    get_token(cp, &tok);

    for (i = 0; i < size_of_comtable; ++i) {
        if (tok.tk_len == comtable[i].namelen &&
            strcmp(comtable[i].name, tok.tk_data) == 0) {

            *tableentry = &comtable[i];
            return comtable[i].type;
        }
    }

    return LT_ERROR;
}

static int
parseoption(char *cp, const char *fname,unsigned long lineno,
    struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;

    cp = cp + clen + 1;
    cp = skipwhite(cp);
    get_token(cp, &tok);
    if (!tok.tk_data) {
        printf("ERROR: empty option: command is ignored");
        return FALSE;
    }
    ensure_has_no_more_tokens(cp + tok.tk_len, fname, lineno);
    if (!strcmp(tok.tk_data,"--format-expr-ops-joined")) {
        glflags.gf_expr_ops_joined = TRUE;
    } else {
        printf("ERROR: option command %s is not understood"
            " and is ignored",tok.tk_data);
        return FALSE;
    }
    return TRUE;

}
/* We are promised it's an abiname: command
   find the name on the line.
*/
static int
parsebeginabi(char *cp, const char *fname, const char *abiname,
    unsigned long lineno, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    size_t abinamelen = strlen(abiname);
    struct token_s tok;

    cp = cp + clen + 1;
    cp = skipwhite(cp);
    get_token(cp, &tok);
    if (tok.tk_len != abinamelen ||
        strncmp(cp, abiname, abinamelen)) {
        printf("dwarfdump internal error: "
            "mismatch \"%s\" with \"%s\"   \"%s\" line %lu\n",
            cp, tok.tk_data, fname, lineno);
        ++errcount;
        return FALSE;
    }
    ensure_has_no_more_tokens(cp + tok.tk_len, fname, lineno);
    return TRUE;
}

/* This expands register names as required, but does not
   ensure no names duplicated.
*/
#define CONF_TABLE_OVERSIZE  100
static void
add_to_reg_table(struct dwconf_s *conf,
    char *rname, unsigned long rval, const char *fname,
    unsigned long lineno)
{
    if (conf->cf_regs_malloced == 0) {
        conf->cf_regs = 0;
        conf->cf_named_regs_table_size = 0;
    }
    if (rval >= conf->cf_named_regs_table_size) {
        char **newregs = 0;
        unsigned long newtablen = rval + CONF_TABLE_OVERSIZE;
        unsigned long newtabsize = newtablen * sizeof(char *);
        unsigned long oldtabsize =
            conf->cf_named_regs_table_size * sizeof(char *);
        newregs = realloc(conf->cf_regs, newtabsize);
        if (!newregs) {
            printf("dwarfdump: unable to malloc table %lu bytes. "
                " %s line %lu\n", newtabsize, fname, lineno);
            exit(EXIT_FAILURE);
        }
        /* Zero out the new entries. */
        memset((char *) newregs + (oldtabsize), 0,
            (newtabsize - oldtabsize));
        conf->cf_named_regs_table_size = (unsigned long) newtablen;
        conf->cf_regs = newregs;
        conf->cf_regs_malloced = 1;
    }
    conf->cf_regs[rval] = rname;
    return;
}

/* Our input is supposed to be a number.
   Determine the value (and return it) or generate an error message.
*/
static int
make_a_number(char *cmd, const char *filename, unsigned long
    lineno, struct token_s *tok, unsigned long *val_out)
{
    char *endnum = 0;
    unsigned long val = 0;

    val = strtoul(tok->tk_data, &endnum, 0);
    if (val == 0 && endnum == (tok->tk_data)) {
        printf("dwarfdump.conf error: "
            "%s missing register number (\"%s\" "
            "not valid)  %s line %lu\n",
            cmd, tok->tk_data, filename, lineno);
        ++errcount;
        return FALSE;
    }
    if (endnum != (tok->tk_data + tok->tk_len)) {
        printf("dwarfdump.conf error: "
            "%s Missing register number (\"%s\" "
            "not valid)  %s line %lu\n",
            cmd, tok->tk_data, filename, lineno);
        ++errcount;
        return FALSE;
    }
    *val_out = val;
    return TRUE;

}

/*  We are guaranteed it's a reg: command, so parse that command
    and record the interesting data.
*/
static int
parsereg(char *cp, const char *fname, unsigned long lineno,
    struct conf_internal_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s regnum;
    struct token_s tokreg;
    unsigned long val = 0;
    int ok = FALSE;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tokreg);
    cp = get_token(cp, &regnum);
    if (tokreg.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "reg: missing register name  %s line %lu",
            fname, lineno);
        ++errcount;
        return FALSE;

    }
    if (regnum.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "reg: missing register number  %s line %lu",
            fname, lineno);
        ++errcount;
        return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &regnum, &val);

    if (!ok) {
        ++errcount;
        return FALSE;
    }

    add_to_reg_table(conf->conf_out, tokreg.tk_data,
        val, fname, lineno);

    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

/*
   We are guaranteed it's an frame_interface: command.
   Parse it and record the value data.
*/
static int
parseframe_interface(char *cp,
    const char *fname,
    unsigned long lineno,
    struct conf_internal_s *conf,
    struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "%s missing interface number %s line %lu",
            comtab->name, fname, lineno);
        ++errcount;
        return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {
        ++errcount;
        return FALSE;
    }
    if (val != 2 && val != 3) {
        printf("dwarfdump.conf error: "
            "%s only interface numbers 2 or 3 are allowed, "
            " not %lu. %s line %lu",
            comtab->name, val, fname, lineno);
        ++errcount;
        return FALSE;
    }

    conf->conf_out->cf_interface_number = (int) val;
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

/*
   We are guaranteed it's a cfa_reg: command. Parse it
   and record the important data.
*/
static int
parsecfa_reg(char *cp, const char *fname, unsigned long lineno,
    struct conf_internal_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "%s missing cfa_reg number %s line %lu",
            comtab->name, fname, lineno);
        ++errcount;
        return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {
        ++errcount;
        return FALSE;
    }
    conf->conf_out->cf_cfa_reg = (int) val;
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

/* We are guaranteed it's an initial_reg_value: command,
   parse it and put the reg value where it will be remembered.
*/
static int
parseinitial_reg_value(char *cp, const char *fname,
    unsigned long lineno,
    struct conf_internal_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "%s missing initial reg value %s line %lu",
            comtab->name, fname, lineno);
        ++errcount;
        return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {

        ++errcount;
        return FALSE;
    }
    conf->conf_out->cf_initial_rule_value = (int) val;
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

static int
parsesame_val_reg(char *cp, const char *fname,
    unsigned long lineno,
    struct conf_internal_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "%s missing same_reg value %s line %lu",
            comtab->name, fname, lineno);
        ++errcount;
        return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {

        ++errcount;
        return FALSE;
    }
    conf->conf_out->cf_same_val = val;
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

static int
parseundefined_val_reg(char *cp, const char *fname,
    unsigned long lineno,
    struct conf_internal_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "%s missing undefined_reg value %s line %lu",
            comtab->name, fname, lineno);
        ++errcount;
        return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {

        ++errcount;
        return FALSE;
    }
    conf->conf_out->cf_undefined_val = (int) val;
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

/* We are guaranteed it's a table size command, parse it
    and record the table size.
*/
static int
parsereg_table_size(char *cp, const char *fname, unsigned long lineno,
    struct conf_internal_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "%s missing reg table size value %s line %lu",
            comtab->name, fname, lineno);
        ++errcount;
        return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {
        ++errcount;
        return FALSE;
    }
    conf->conf_out->cf_table_entry_count = (unsigned long) val;
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

/* We are guaranteed it's a table size command, parse it
    and record the table size.
*/
static int
parseaddress_size(char *cp, const char *fname, unsigned long lineno,
    struct conf_internal_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
        printf("dwarfdump.conf error: "
            "%s missing address size value %s line %lu",
            comtab->name, fname, lineno);
        ++errcount;
        return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {
        ++errcount;
        return FALSE;
    }
    conf->conf_out->cf_address_size = (unsigned long) val;
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

/*  We are guaranteed it's an endabi: command, parse it and
    check we have the right abi.
*/
static int
parseendabi(char *cp, const char *fname,
    const char *abiname, unsigned long lineno,
    struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    int res = 0;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (strcmp(abiname, tok.tk_data) != 0) {
        printf("%s error: "
            "mismatch abi name %s (here) vs. "
            "%s (beginabi:)  %s line %lu\n",
            comtab->name, tok.tk_data, abiname, fname, lineno);
        ++errcount;
        return FALSE;
    }
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}
static int
parseincludeabi(char *cp, const char *fname, unsigned long lineno,
    char **abiname_out,
    struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    char *name = 0;
    int res = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    name = makename(tok.tk_data);
    *abiname_out = name;
    res = ensure_has_no_more_tokens(cp, fname, lineno);
    return res;
}

/*  Return TRUE if we succeeded and filed in *out.
    Return FALSE if we failed (and fill in nothing).
    beginabi:  <abiname>
    reg: <regname> <dwarf regnumber>
    frame_interface: <integer value 2 or 3>
    cfa_reg:  <number>
    initial_reg_value:  <number: normally 1034 or 1035 >
    reg_table_size: <size of table>
    endabi:  <abiname>

    We are positioned at the start of a beginabi: line when
    called.

*/
static int
parse_abi(FILE * stream, const char *fname, const char *abiname,
    struct conf_internal_s *conf_internal,
    unsigned long lineno,
    unsigned int nest_level)
{
    struct dwconf_s *localconf = conf_internal->conf_out;
    char buf[1000];
    int comtype = 0;
    static int first_time_done = 0;
    struct comtable_s *comtabp = 0;
    int inourabi = FALSE;
    int unknowntextcount = 0;

    if (nest_level > MAX_NEST_LEVEL) {
        ++errcount;
        printf("dwarfdump.conf: includeabi nest "
            "too deep in %s at line %lu\n",
            sanitized(fname), lineno);
        return FALSE;
    }

    if (first_time_done == 0) {
        finish_comtable_setup();
        first_time_done = 1;
    }

    for (; !feof(stream);) {
        char *line = 0;

        /* long loffset = ftell(stream); */
        line = fgets(buf, sizeof(buf), stream);
        if (!line) {
            ++errcount;
            printf("dwarfdump: end of file or error"
                " before endabi: in %s, line %lu\n",
                sanitized(fname), lineno);
            return FALSE;
        }
        ++lineno;
        line = skipwhite(line);
        comtype = which_command(line, &comtabp);
        switch (comtype) {
        case LT_OPTION:
            if (!inourabi) break;
            parseoption(line, sanitized(fname), lineno,comtabp);
            break;
        case LT_ERROR:
            ++unknowntextcount;
            ++errcount;
            printf("dwarfdump: Unknown text in %s ",
                sanitized(fname));
            printf("is \"%s\" "
                "at line %lu\n",
                sanitized(line), lineno);
            if (unknowntextcount > 2) {
                printf("Too much unknown text. Giving up "
                    "on the dwarfdump.conf file\n");
                return FALSE;
            }
            break;
        case LT_COMMENT:
            break;
        case LT_BLANK:
            break;
        case LT_BEGINABI:
            if (conf_internal->beginabi_lineno > 0) {
                ++errcount;
                printf("dwarfdump: Encountered beginabi: "
                    "when not expected. "
                    "%s line %lu previous beginabi line %lu\n",
                    sanitized(fname),
                    lineno, conf_internal->beginabi_lineno);
            }
            conf_internal->beginabi_lineno = lineno;
            inourabi = parsebeginabi(line, fname,
                abiname, lineno, comtabp);
            break;

        case LT_REG:
            if (!inourabi) break;
            parsereg(line, fname, lineno, conf_internal, comtabp);
            conf_internal->regcount++;
            break;
        case LT_FRAME_INTERFACE:
            if (!inourabi) break;
            if (conf_internal->frame_interface_lineno > 0) {
                ++errcount;
                printf("dwarfdump: Encountered duplicate "
                    "frame_interface: "
                    "%s line %lu previous frame_interface: "
                    "line %lu\n",
                    sanitized(fname), lineno,
                    conf_internal->frame_interface_lineno);
            }
            conf_internal->frame_interface_lineno = lineno;
            parseframe_interface(line, fname,
                lineno, conf_internal, comtabp);
            break;
        case LT_CFA_REG:
            if (!inourabi) break;
            if (conf_internal->cfa_reg_lineno > 0) {
                printf("dwarfdump: Encountered duplicate cfa_reg: "
                    "%s line %lu previous cfa_reg line %lu\n",
                    sanitized(fname),
                    lineno, conf_internal->cfa_reg_lineno);
                ++errcount;
            }
            conf_internal->cfa_reg_lineno = lineno;
            parsecfa_reg(line, fname, lineno, conf_internal, comtabp);
            break;
        case LT_INITIAL_REG_VALUE:
            if (!inourabi) break;
            if (conf_internal->initial_reg_value_lineno > 0) {
                printf
                    ("dwarfdump: Encountered duplicate "
                    "initial_reg_value: "
                    "%s line %lu previous initial_reg_value: "
                    "line %lu\n",
                    sanitized(fname), lineno,
                    conf_internal->initial_reg_value_lineno);
                ++errcount;
            }
            conf_internal->initial_reg_value_lineno = lineno;
            parseinitial_reg_value(line, fname,
                lineno, conf_internal, comtabp);
            break;
        case LT_SAME_VAL_REG:
            if (!inourabi) break;
            if (conf_internal->same_val_reg_lineno > 0) {
                ++errcount;
                printf
                    ("dwarfdump: Encountered duplicate "
                    "same_val_reg: "
                    "%s line %lu previous initial_reg_value: "
                    "line %lu\n",
                    sanitized(fname), lineno,
                    conf_internal->initial_reg_value_lineno);
            }
            conf_internal->same_val_reg_lineno = lineno;
            parsesame_val_reg(line, fname,
                lineno, conf_internal, comtabp);
            break;
        case LT_UNDEFINED_VAL_REG:
            if (!inourabi) break;
            if (conf_internal->undefined_val_reg_lineno > 0) {
                ++errcount;
                printf
                    ("dwarfdump: Encountered duplicate "
                    "undefined_val_reg: "
                    "%s line %lu previous initial_reg_value: "
                    "line %lu\n",
                    sanitized(fname), lineno,
                    conf_internal->initial_reg_value_lineno);
            }
            conf_internal->undefined_val_reg_lineno = lineno;
            parseundefined_val_reg(line, fname,
                lineno, conf_internal, comtabp);
            break;
        case LT_REG_TABLE_SIZE:
            if (!inourabi) break;
            if (conf_internal->reg_table_size_lineno > 0) {
                printf("dwarfdump: duplicate reg_table_size: "
                    "%s line %lu previous reg_table_size: line %lu\n",
                    sanitized(fname), lineno,
                    conf_internal->reg_table_size_lineno);
                ++errcount;
            }
            conf_internal->reg_table_size_lineno = lineno;
            parsereg_table_size(line, fname,
                lineno, conf_internal, comtabp);
            break;
        case LT_ENDABI:
            if (!inourabi) break;
            parseendabi(line, fname, abiname, lineno, comtabp);
            if (conf_internal->regcount >
                localconf->cf_table_entry_count) {
                printf("dwarfdump: more registers named than "
                    " in  %s  ( %lu named vs  %s %lu)"
                    "  %s line %lu\n",
                    abiname, (unsigned long) conf_internal->regcount,
                    name_reg_table_size,
                    (unsigned long) localconf->cf_table_entry_count,
                    sanitized(fname), (unsigned long) lineno);
                ++errcount;
            }
            inourabi = FALSE;
            return TRUE;
        case LT_ADDRESS_SIZE:
            if (!inourabi) break;
            if (conf_internal->address_size_lineno > 0) {
                printf("dwarfdump: duplicate address_size: "
                    "%s line %lu previous address_size:"
                    " line %lu\n",
                    fname, lineno,
                    conf_internal->address_size_lineno);
                ++errcount;
            }
            conf_internal->address_size_lineno = lineno;
            parseaddress_size(line, fname,
                lineno, conf_internal, comtabp);
            break;
        case LT_INCLUDEABI: {
            char *abiname_inner = 0;
            unsigned long abilno = conf_internal->beginabi_lineno;
            int ires = 0;

            if (!inourabi) break;
            ires = parseincludeabi(line,fname,lineno,
                &abiname_inner,comtabp);
            if (ires == FALSE) {
                return FALSE;
            }
            /*  For the nested abi read, the abi line
                number must be set as if not-yet-read
                and then restored. */
            conf_internal->beginabi_lineno = 0;
            ires  = find_conf_file_and_read_config_inner(
                conf_internal->conf_name_used,
                abiname_inner, conf_internal,nest_level+1);
            if (ires == FOUND_ERROR) {
                return ires;
            }
            conf_internal->beginabi_lineno = abilno;
            }
            break;
        default:
            printf("dwarfdump internal error,"
                " impossible line type %d  %s %lu \n",
                (int) comtype, sanitized(fname), lineno);
            exit(EXIT_FAILURE);
        }
    }
    ++errcount;
    printf("End of file, no endabi: found. %s, line %lu\n",
        sanitized(fname), lineno);
    return FALSE;
}

/*  Naming a few registers makes printing these just
    a little bit faster.
*/
static char *genericregnames[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
    "r10", "r11", "r12", "r13", "r14", "r15", "r16",
    "r17", "r18", "r19", "r20"
};

/*  This is a simple generic set of registers.  The
    table entry count is pretty arbitrary.
    However, some sarubbo-11 *.bin cases have a
    return address register 117. So lets
    the base standard to 150.
    December 2025, v2.3.0
*/
void
init_conf_file_data(struct dwconf_s *config_file_data)
{
    unsigned generic_table_count;
    config_file_data->cf_abi_name = "";
    config_file_data->cf_config_file_path = "";
    config_file_data->cf_interface_number = 3;
    config_file_data->cf_table_entry_count = 150;
    config_file_data->cf_initial_rule_value = DW_FRAME_UNDEFINED_VAL;
    config_file_data->cf_cfa_reg =  DW_FRAME_CFA_COL3;
    config_file_data->cf_address_size =  0;
    config_file_data->cf_same_val = DW_FRAME_SAME_VAL;
    config_file_data->cf_undefined_val = DW_FRAME_UNDEFINED_VAL;
    config_file_data->cf_regs = genericregnames;
    generic_table_count =
        sizeof(genericregnames) / sizeof(genericregnames[0]);
    config_file_data->cf_named_regs_table_size = generic_table_count;
    config_file_data->cf_regs_malloced = 0;
}

/*  A 'generic' ABI. For up to 1200 registers.
    Perhaps cf_initial_rule_value should be d
    UNDEFINED VALUE (1034) instead, but for the purposes of
    getting the dwarfdump output correct
    either will work.
*/
void
init_generic_config_1200_regs(struct dwconf_s *config_file_data)
{
    unsigned long generic_table_count =
        sizeof(genericregnames) / sizeof(genericregnames[0]);
    config_file_data->cf_interface_number = 3;
    config_file_data->cf_table_entry_count = 1200;
    /*  There is no defined name for cf_initial_rule_value,
        cf_same_val, or cf_undefined_val in libdwarf.h,
        these must just be high enough to be higher than
        any real register number.
        DW_FRAME_CFA_COL3 must also be higher than any
        real register number. */
    config_file_data->cf_initial_rule_value = 1235; /* SAME VALUE */
    config_file_data->cf_cfa_reg =  DW_FRAME_CFA_COL3;
    config_file_data->cf_address_size =  0;
    config_file_data->cf_same_val = 1235;
    config_file_data->cf_undefined_val = 1234;
    config_file_data->cf_regs = genericregnames;
    config_file_data->cf_named_regs_table_size = generic_table_count;
    config_file_data->cf_regs_malloced = 0;
}

/*  Print the 'right' string for the register we are given.
    Deal sensibly with the special regs as well as numbers
    we know and those we have not been told about.

    We are not allowing negative register numbers.
*/
void
print_reg_from_config_data(Dwarf_Unsigned reg,
    struct dwconf_s *config_data)
{
    char *name = 0;
    if (reg == config_data->cf_cfa_reg) {
        fputs("cfa",stdout);
        return;
    }
    if (reg == config_data->cf_undefined_val) {
        fputs("u",stdout);
        return;
    }
    if (reg == config_data->cf_same_val) {
        fputs("s",stdout);
        return;
    }

    if (config_data->cf_regs == 0 ||
        reg >= config_data->cf_named_regs_table_size) {
        printf("r%" DW_PR_DUu "",reg);
        return;
    }
    name = config_data->cf_regs[reg];
    if (!name) {
        /* Can happen, the reg names table can be sparse. */
        printf("r%" DW_PR_DUu "", reg);
        return;
    }
    fputs(name,stdout);
    return;
}

void
free_all_dwconf(struct dwconf_s *conf)
{
    if (conf->cf_regs_malloced) {
        free(conf->cf_regs);
    }
    conf->cf_regs = 0;
    conf->cf_named_regs_table_size =0;
    conf->cf_regs_malloced = 0;
}
