/*
Copyright (C) 2020 David Anderson. All Rights Reserved.

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

/*  To print .debug_sup */

#include <config.h>
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_naming.h"
#include "dd_esb.h"                /* For flexible string buffer. */
#include "dd_esb_using_functions.h"
#include "dd_sanitized.h"

int
print_debug_sup(Dwarf_Debug dbg,
    Dwarf_Error *error)
{
    int res = 0;
    const char *stdname = 0;
    char buf[DWARF_SECNAME_BUFFER_SIZE];
    struct esb_s truename;
    Dwarf_Half version = 0;
    Dwarf_Small is_supplementary = 0;
    char *      filename = 0;
    Dwarf_Unsigned checksum_len = 0;
    Dwarf_Unsigned i =0;
    Dwarf_Small *checksum_ptr = 0;
    Dwarf_Small *curptr = 0;

    esb_constructor_fixed(&truename,buf,
        DWARF_SECNAME_BUFFER_SIZE);
    glflags.current_section_id = DEBUG_SUP;
    stdname =  ".debug_sup";
    get_true_section_name(dbg,stdname, &truename,TRUE);
    res = dwarf_get_debug_sup(dbg,
        &version,&is_supplementary,
        &filename,&checksum_len,
        &checksum_ptr,error);
    if (res == DW_DLV_ERROR) {
        glflags.gf_count_major_errors++;
        printf("ERROR: problem reading %s. %s\n",
            sanitized(esb_get_string(&truename)),
            dwarf_errmsg(*error));
        dwarf_dealloc_error(dbg,*error);
        *error = 0;
        esb_destructor(&truename);
        return DW_DLV_OK;
    }
    if (res == DW_DLV_NO_ENTRY) {
        esb_destructor(&truename);
        return res;
    }
    printf("\n%s\n",sanitized(esb_get_string(&truename)));
    printf("  Version              : %u\n",version);
    if (version != 2) {
        glflags.gf_count_major_errors++;
        printf("ERROR: the %s version is %u but "
            "only 2 is currently valid\n",
            sanitized(esb_get_string(&truename)),
            version);
    }
    printf("  Supplementary file   : %u (%s}\n",is_supplementary,
        is_supplementary?"yes":"no");
    if (is_supplementary > 1) {
        glflags.gf_count_major_errors++;
        printf("ERROR: the %s is_supplementary field is %u but "
            "only 0 or 1 is currently valid\n",
            sanitized(esb_get_string(&truename)),
            is_supplementary);
    }
    printf("  Filename             : %s\n",sanitized(filename));
    printf("  Checksum Length      : %" DW_PR_DUu "\n",checksum_len);
    printf("  Checksum bytes in hex:\n");
    curptr = checksum_ptr;
    if (checksum_len > 0) {
        printf("    ");
    }
    for (i = 0 ; i < checksum_len; ++i,++curptr) {
        if (i > 0 && (i%16) == 0) {
            printf("\n    ");
        }
        printf("%02x",*curptr);
    }
    printf("\n");
    esb_destructor(&truename);
    return DW_DLV_OK;
}
