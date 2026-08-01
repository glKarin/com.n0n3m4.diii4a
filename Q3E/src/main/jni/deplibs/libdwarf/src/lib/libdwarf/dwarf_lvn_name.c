/*
Copyright (c) 2025, David Anderson
All rights reserved.
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

#include <config.h>

#include <stddef.h> /* NULL size_t */
#include <stdio.h> /* debugging printf */

#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "dwarf_local_malloc.h"
#include "libdwarf_private.h"
#include "dwarf_base_types.h"
#include "dwarf_opaque.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"
#include "dwarf_die_deliv.h"
#include "dwarf_string.h"

struct Dwarf_LVN_s {
    Dwarf_Unsigned lvn_lang;
    Dwarf_Unsigned lvn_version;
    const char    *lvn_version_name;
};
typedef const struct Dwarf_LVN_s * Dwarf_LVN;

static const struct Dwarf_LVN_s _dwarf_lvn_data[] =
{
{DW_LNAME_C,000000UL,"K&R"},
{DW_LNAME_C,198812UL,"C89"},
{DW_LNAME_C,199901UL,"C99"},
{DW_LNAME_C,201112UL,"C11"},
{DW_LNAME_C,201710UL,"C17"},
{DW_LNAME_C,202311UL,"C23"},

{DW_LNAME_C_plus_plus,199711UL,"C++98"},
{DW_LNAME_C_plus_plus,201103UL,"C++11"},
{DW_LNAME_C_plus_plus,201402UL,"C++14"},
{DW_LNAME_C_plus_plus,201703UL,"C++17"},
{DW_LNAME_C_plus_plus,202002UL,"C++20"},
{DW_LNAME_C_plus_plus,202302UL,"C++23"}
};
/*  Actual instances not yet available.
    Nim    0 VVMMPP
    Erlang 1 VVMMPP
    Elixir 1 VVMMPP
    Gleam  0 VVMMPP */
static Dwarf_Unsigned lvn_indexmax =
    (Dwarf_Unsigned)(sizeof(_dwarf_lvn_data)/
    sizeof(struct Dwarf_LVN_s));

/*  Never returns DW_DLV_ERROR
    Callers must have found DW_AT_language_version
    attribute and pass in its value
    as dw_lv_ver for this to return a meaningful value.
    For K&R C, cc_language_version zero is legitimate. */
int
dwarf_lvn_name_direct(Dwarf_Unsigned dw_lv_lang,
    Dwarf_Unsigned dw_lv_ver,
    const char   **dw_ret_name /* C99 for example */,
    const char   **dw_ret_scheme /* YYYYMM for example */)
{
    Dwarf_LVN lvn = _dwarf_lvn_data;
    Dwarf_Unsigned i = 0;
    int          lowerbound = 0;

    /*For K&R C, lvn->lvn_version zero is legitimate, */
    for ( ; i < lvn_indexmax ; ++lvn,++i) {
        if (dw_lv_lang != lvn->lvn_lang) {
            continue;
        }
        if (dw_lv_ver != lvn->lvn_version) {
            continue;
        }
        if (!lvn->lvn_version_name) {
            continue;
        }
        if (dw_ret_name) {
            *dw_ret_name = lvn->lvn_version_name;
        }
        /*  Ignoring return value */
        if (dw_ret_scheme) {
            dwarf_language_version_data(dw_lv_lang,
                &lowerbound,
                dw_ret_scheme);
        }
        return DW_DLV_OK;
    }
    return DW_DLV_NO_ENTRY;
}

/*  Never returns DW_DLV_ERROR
    On success returns a language version name
    such as "C++11" and a language version scheme
*/
int
dwarf_lvn_name(Dwarf_Die dw_die,
    const char **dw_ret_name,
    const char **dw_ret_version_scheme)
{
    Dwarf_CU_Context cucon = 0;

    if (!dw_die || !dw_die->di_cu_context) {
        return DW_DLV_NO_ENTRY;
    }
    cucon = dw_die->di_cu_context;
    if (!cucon->cc_have_language_version) {

        return DW_DLV_NO_ENTRY;
    }
    if (!cucon->cc_language_version_name) {
        return DW_DLV_NO_ENTRY;
    }
    if (dw_ret_name) {
        *dw_ret_name = cucon->cc_language_version_name;
    }
    if (dw_ret_version_scheme) {
        *dw_ret_version_scheme = cucon->cc_language_version_scheme;
    }
    return DW_DLV_OK;
}

int
dwarf_lvn_table_entry(Dwarf_Unsigned dw_lvn_index,
    Dwarf_Unsigned *dw_lvn_language_name,
    Dwarf_Unsigned *dw_lvn_language_version,
    const char     **dw_lvn_language_version_scheme,
    const char     **dw_lvn_language_version_name)
{
    Dwarf_LVN lvn = 0;
    if (dw_lvn_index >= lvn_indexmax) {
        return DW_DLV_NO_ENTRY;
    }
    lvn = &_dwarf_lvn_data[dw_lvn_index];
    if (dw_lvn_language_name) {
        *dw_lvn_language_name  = lvn->lvn_lang;
    }
    if (dw_lvn_language_version) {
        *dw_lvn_language_version  = lvn->lvn_version;
    }
    if (dw_lvn_language_version_name) {
        *dw_lvn_language_version_name  = lvn->lvn_version_name;
    }
    if (dw_lvn_language_version_scheme) {
        int lowerbound = 0;
        const char *verscheme = 0;
        dwarf_language_version_data(lvn->lvn_lang,
            &lowerbound,
            &verscheme);
        *dw_lvn_language_version_scheme = verscheme;
    }
    return DW_DLV_OK;
}
