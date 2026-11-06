/*
Copyright (C) 2022 David Anderson. All Rights Reserved.

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

/*  To print .debug_gnu_pubnames, .debug_gnu_typenames */

#include <config.h>
#include <stdio.h> /* FILE decl for dd_esb.h */

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

static void
print_sec_name(Dwarf_Debug dbg)
{
    struct esb_s truename;
    char buf[DWARF_SECNAME_BUFFER_SIZE];

    esb_constructor_fixed(&truename,buf,sizeof(buf));
    get_true_section_name(dbg,".debug_addr",
        &truename,TRUE);
    printf("\n%s\n\n",sanitized(esb_get_string(&truename)));
    esb_destructor(&truename);
}

static void
print_table_header(Dwarf_Unsigned tabnum,
    Dwarf_Unsigned cur_secoff,
    Dwarf_Unsigned length,
    Dwarf_Half     version,
    Dwarf_Small    address_size,
    Dwarf_Unsigned at_addr_base,
    Dwarf_Unsigned entry_count,
    Dwarf_Unsigned next_table_offset)
{
    printf("Debug Addr table       : %" DW_PR_DUu "\n",tabnum);
    printf(" table length          : %" DW_PR_DUu "\n",length);
    printf(" table version         : %u\n",version);
    printf(" address size          : %u\n",address_size);
    printf(" segment selector size : 0\n");
    printf(" table entry count     : %" DW_PR_DUu "\n",entry_count);
    printf(" table section offset  : 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
        cur_secoff);
    printf(" addr base             : 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
        at_addr_base);
    printf(" addr base  will match some "
        "DW_AT_addr_base attribute\n");
    printf(" next table offset     : 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
        next_table_offset);
}

int
print_debug_addr(Dwarf_Debug dbg,
    Dwarf_Error *error)
{
    int res = DW_DLV_OK;
    Dwarf_Unsigned tabnum = 0;
    Dwarf_Unsigned cur_secoff = 0;
    Dwarf_Debug_Addr_Table tab = 0;
    Dwarf_Unsigned   length = 0;
    Dwarf_Half       version = 0;
    Dwarf_Small      address_size = 0;
    Dwarf_Unsigned   at_addr_base = 0;
    Dwarf_Unsigned   entry_count = 0;
    Dwarf_Unsigned   next_table_offset = 0;

    print_sec_name(dbg);
    for ( ;; tabnum++,cur_secoff = next_table_offset) {
        Dwarf_Unsigned curindex = 0;

        res = dwarf_debug_addr_table(dbg,cur_secoff,
            &tab,&length,&version,&address_size,
            &at_addr_base,&entry_count,&next_table_offset,
            error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* End of section. */
            break;
        }
        print_table_header(tabnum,cur_secoff,length,version,
            address_size, at_addr_base,entry_count,
            next_table_offset);
        for ( ;curindex < entry_count;++curindex) {
            int eres = 0;
            Dwarf_Unsigned addr = 0;
            eres = dwarf_debug_addr_by_index(tab,curindex,
                &addr,error);
            if (eres == DW_DLV_ERROR) {
                dwarf_dealloc_debug_addr_table(tab);
                return eres;
            }
            if (eres == DW_DLV_NO_ENTRY) {
                printf("ERROR: Unexpected NO entry on "
                    ".debug_addr table %" DW_PR_DUu
                    " offset 0x" DW_PR_XZEROS DW_PR_DUx
                    " entry index %" DW_PR_DUu "\n",
                    tabnum,curindex);
                    glflags.gf_count_major_errors++;
                break;
            }
            if (!curindex) {
                printf(" [index] address\n");
            }

            printf(" [%3" DW_PR_DUu "]"
                " 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                curindex,addr);
        }
        dwarf_dealloc_debug_addr_table(tab);
    }
    printf("\n");
    return DW_DLV_OK;
}
