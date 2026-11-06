/*
Copyright (C) 2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright (C) 2011-2018 SN Systems Ltd. All Rights Reserved.
Portions Copyright (C) 2007-2020 David Anderson. All Rights Reserved.

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

/*  From 199x through 2010 print_frames relied on
    the order of the fdes matching the order of the functions
    in the CUs when it came to printing a function name with
    an FDE.   This sometimes worked for SGI/IRIX because of
    the way the compiler often emitted things.
    It always worked poorly
    for gcc and other compilers.

    As of 2010 the addrmap.h addrmap.h code provides help
    in doing a better job when the tsearch functions (part of
    POSIX) are available.  */

#include <config.h>

#include <stdlib.h> /* exit() */
#include <string.h> /* strlen() */
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_dwconf.h"
#include "dd_dwconf_using_functions.h"
#include "dd_esb.h"
#include "dd_esb_using_functions.h"
#include "dd_sanitized.h"
#include "dd_addrmap.h"
#include "dd_naming.h"
#include "dd_safe_strcpy.h"
#include "dd_glflags.h"
#include "print_frames.h"

#define TRUE 1
#define FALSE 0

#undef TESTOLDDW_OFFSET /* define only for a test. */

Dwarf_Sig8 zero_type_signature;
static void
print_one_frame_reg_col(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Unsigned rule_id,
    Dwarf_Small value_type,
    Dwarf_Unsigned reg_used,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    Dwarf_Half version,
    struct dwconf_s *config_data,
    Dwarf_Signed offset_relevant,
    Dwarf_Signed offset,
    Dwarf_Block * block_ptr);

static void
print_frame_inst_bytes(Dwarf_Debug dbg,
    Dwarf_Frame_Instr_Head instr_head,
    Dwarf_Unsigned instr_array_len,
    Dwarf_Die die,
    Dwarf_Signed data_alignment_factor,
    Dwarf_Unsigned code_alignment_factor,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    Dwarf_Half version,
    struct dwconf_s *config_data);

static void
dealloc_local_atlist(Dwarf_Debug dbg,
    Dwarf_Attribute *atlist,
    Dwarf_Signed atcnt)
{
    Dwarf_Signed k = 0;

    for (; k < atcnt; k++) {
        dwarf_dealloc(dbg, atlist[k], DW_DLA_ATTR);
    }
    dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
}

/*  Executing this for a reporting side effect,
    PRINT_CU_INFO() in dwarfdump.c. */
static void
load_CU_error_data(Dwarf_Debug dbg,Dwarf_Die cu_die)
{
    Dwarf_Signed atcnt = 0;
    Dwarf_Attribute *atlist = 0;
    Dwarf_Half tag = 0;
    char **srcfiles = 0;
    Dwarf_Signed srccnt = 0;
    int local_show_form_used = 0;
    int local_verbose = 0;
    int atres = 0;
    Dwarf_Signed i = 0;
    Dwarf_Signed k = 0;
    Dwarf_Error loadcuerr = 0;
    Dwarf_Off cu_die_goff = 0;

    if (!cu_die) {
        return;
    }
    atres = dwarf_attrlist(cu_die, &atlist, &atcnt, &loadcuerr);
    if (atres != DW_DLV_OK) {
        /*  Something is seriously wrong if it is DW_DLV_ERROR. */
        DROP_ERROR_INSTANCE(dbg,atres,loadcuerr);
        return;
    }
    atres = dwarf_tag(cu_die, &tag, &loadcuerr);
    if (atres != DW_DLV_OK) {
        for (k = 0; k < atcnt; k++) {
            dwarf_dealloc(dbg, atlist[k], DW_DLA_ATTR);
        }
        dealloc_local_atlist(dbg,atlist,atcnt);
        /*  Something is seriously wrong if it is DW_DLV_ERROR. */
        DROP_ERROR_INSTANCE(dbg,atres,loadcuerr);
        return;
    }

    /* The offsets will be zero if it fails. Let it pass. */
    atres = dwarf_die_offsets(cu_die,&glflags.DIE_section_offset,
        &glflags.DIE_offset,&loadcuerr);
    cu_die_goff = glflags.DIE_section_offset;
    DROP_ERROR_INSTANCE(dbg,atres,loadcuerr);

    glflags.DIE_CU_overall_offset = glflags.DIE_section_offset;
    glflags.DIE_CU_offset = glflags.DIE_offset;
    for (i = 0; i < atcnt; i++) {
        Dwarf_Half attr = 0;
        int ares = 0;
        Dwarf_Attribute attrib = atlist[i];

        ares = dwarf_whatattr(attrib, &attr, &loadcuerr);
        if (ares != DW_DLV_OK) {
            for (k = 0; k < atcnt; k++) {
                dwarf_dealloc(dbg, atlist[k], DW_DLA_ATTR);
            }
            dealloc_local_atlist(dbg,atlist,atcnt);
            DROP_ERROR_INSTANCE(dbg,ares,loadcuerr);
            return;
        }
        /*  For now we will not fully deal with the complexity of
            DW_AT_high_pc being an offset of low pc. */
        switch(attr) {
        case DW_AT_low_pc:
            {
            ares = dwarf_formaddr(attrib,
                &glflags.CU_base_address, &loadcuerr);
            DROP_ERROR_INSTANCE(dbg,ares,loadcuerr);
            glflags.CU_low_address = glflags.CU_base_address;
            }
            break;
        case DW_AT_high_pc:
            {
            /*  This is wrong for DWARF4 instances where
                the attribute is really an offset.
                It's also useless for CU DIEs that do not
                have the DW_AT_high_pc high so CU_high_address will
                be zero*/
            ares = dwarf_formaddr(attrib,
                &glflags.CU_high_address, &loadcuerr);
            DROP_ERROR_INSTANCE(dbg,ares,loadcuerr);
            }
            break;
        case DW_AT_name:
        case DW_AT_producer:
            {
            const char *name = 0;
            struct esb_s namestr;

            esb_constructor(&namestr);
            ares = get_attr_value(dbg, tag, cu_die,
                cu_die_goff,attrib, srcfiles, srccnt,
                &namestr, local_show_form_used,local_verbose,
                &loadcuerr);
            DROP_ERROR_INSTANCE(dbg,ares,loadcuerr);
            if (esb_string_len(&namestr)) {
                name = esb_get_string(&namestr);
                if (attr == DW_AT_name) {
                    dd_safe_strcpy(glflags.CU_name,
                        sizeof( glflags.CU_name),
                        name, strlen(name));
                } else {
                    dd_safe_strcpy(glflags.CU_producer,
                        sizeof(glflags.CU_producer),
                        name,strlen(name));
                }
            }
            esb_destructor(&namestr);
            }
            break;
        default:
            /* do nothing */
            break;
        }
    }
    dealloc_local_atlist(dbg,atlist,atcnt);
    return;
}

/*  For inlined functions, try to find name.
    If we fail due to error we hide the error. For now.
    Returns DW_DLV_OK or DW_DLV_NO_ENTRY for now. */
static int
get_abstract_origin_funcname(Dwarf_Debug dbg,
    Dwarf_Die die, Dwarf_Attribute attr,
    struct esb_s *name_out)
{
    Dwarf_Off off = 0;
    Dwarf_Die origin_die = 0;
    Dwarf_Attribute *atlist = NULL;
    Dwarf_Signed atcnt = 0;
    Dwarf_Signed i = 0;
    int dres = 0;
    int atres = 0;
    int name_found = 0;
    int res = 0;
    Dwarf_Error err = 0;
    Dwarf_Bool is_info = dwarf_get_die_infotypes_flag(die);

    res = dwarf_global_formref_b(attr,&off,&is_info,&err);
    if (res == DW_DLV_ERROR) {
        dwarf_dealloc(dbg,err,DW_DLA_ERROR);
        return DW_DLV_NO_ENTRY;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return DW_DLV_NO_ENTRY;
    }
    dres = dwarf_offdie_b(dbg,off,is_info,&origin_die,&err);
    if (dres == DW_DLV_ERROR) {
        dwarf_dealloc(dbg,err,DW_DLA_ERROR);
        return DW_DLV_NO_ENTRY;
    }
    if (dres == DW_DLV_NO_ENTRY) {
        return DW_DLV_NO_ENTRY;
    }
    atres = dwarf_attrlist(origin_die, &atlist, &atcnt, &err);
    if (atres == DW_DLV_ERROR) {
        dwarf_dealloc(dbg,origin_die,DW_DLA_DIE);
        dwarf_dealloc(dbg,err,DW_DLA_ERROR);
        return DW_DLV_NO_ENTRY;
    }
    if (atres == DW_DLV_NO_ENTRY) {
        dwarf_dealloc(dbg,origin_die,DW_DLA_DIE);
        return DW_DLV_NO_ENTRY;
    }
    for (i = 0; i < atcnt; i++) {
        Dwarf_Half lattr = 0;
        int ares = 0;

        ares = dwarf_whatattr(atlist[i], &lattr, &err);
        if (ares == DW_DLV_ERROR) {
            break;
        } else if (ares == DW_DLV_OK) {
            if (lattr == DW_AT_name) {
                int sres = 0;
                char* tempsl = 0;

                sres = dwarf_formstring(atlist[i], &tempsl, &err);
                if (sres == DW_DLV_OK) {
                    esb_append(name_out,tempsl);
                    name_found = TRUE;
                    break;
                }
            }
        }
    }
    for (i = 0; i < atcnt; i++) {
        dwarf_dealloc(dbg, atlist[i], DW_DLA_ATTR);
    }
    dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
    dwarf_dealloc(dbg,origin_die,DW_DLA_DIE);
    if (!name_found) {
        return DW_DLV_NO_ENTRY;
    }
    return DW_DLV_OK;
}
/*
    Returns DW_DLV_OK if a proc with this low_pc found.
    Else returns DW_DLV_NO_ENTRY.

    From print_die.c this has no pcMap passed in,
    we do not really have a sensible context, so this
    really just looks at the current attributes for a name.

    From print_frames.c we do have a pcMap.
*/
int
get_proc_name_by_die(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Addr low_pc,
    struct esb_s *proc_name,
    Dwarf_Die * cu_die_for_print_frames,
    void **pcMap,
    Dwarf_Error *err)
{
    Dwarf_Signed atcnt = 0;
    Dwarf_Signed i = 0;
    Dwarf_Attribute *atlist = NULL;
    Dwarf_Addr low_pc_for_die = 0;
    int atres = 0;
    int funcpcfound = 0;
    int funcres = DW_DLV_OK;
    int funcnamefound = 0;
    int loop_ok = TRUE;

    if (pcMap) {
        struct Addr_Map_Entry *ame = 0;
        ame = addr_map_find(low_pc,pcMap);
        if (ame && ame->mp_name) {
            /* mp_name is NULL only if we ran out of heap space. */
            esb_append(proc_name,ame->mp_name);
            return DW_DLV_OK;
        }
    }
    if (glflags.gf_all_cus_seen_search_by_address) {
        return DW_DLV_NO_ENTRY;
    }
    if (glflags.gf_debug_addr_missing) {
        return DW_DLV_NO_ENTRY;
    }
    atres = dwarf_attrlist(die, &atlist, &atcnt, err);
    if (atres == DW_DLV_ERROR) {
        load_CU_error_data(dbg,*cu_die_for_print_frames);
        simple_err_only_return_action(atres,
            "\nERROR: dwarf_attrlist call fails in attempt "
            " to get a procedure/function name");
        return atres;
    }
    if (atres == DW_DLV_NO_ENTRY) {
        return atres;
    }
    for (i = 0; i < atcnt; i++) {
        Dwarf_Half attr = 0;
        int ares = 0;
        char * temps = 0;
        int sres = 0;
        int dres = 0;

        if (!loop_ok) {
            break;
        }
        if (funcnamefound == 1 && funcpcfound == 1) {
            /* stop as soon as both found */
            break;
        }
        ares = dwarf_whatattr(atlist[i], &attr, err);
        if (ares == DW_DLV_ERROR) {
            struct esb_s m;
            esb_constructor(&m);
            load_CU_error_data(dbg,*cu_die_for_print_frames);
            esb_append_printf_s(&m,
                "\nERROR: dwarf_whatattr fails with %s",
                dwarf_errmsg(*err));
            simple_err_only_return_action(ares,
                esb_get_string(&m));
            esb_destructor(&m);
            dealloc_local_atlist(dbg,atlist,atcnt);
            return DW_DLV_ERROR;
        } else if (ares == DW_DLV_OK) {
            Dwarf_Error aterr = 0;
            switch (attr) {
            case DW_AT_specification:
            case DW_AT_abstract_origin:
                if (!funcnamefound) {
                    /*  Only use this if we have not seen DW_AT_name
                        yet .*/
                    int aores = get_abstract_origin_funcname(dbg,
                        die,
                        atlist[i], proc_name);
                    if (aores == DW_DLV_OK) {
                        /* FOUND THE NAME */
                        funcnamefound = 1;
                    }
                }
                break;
            case DW_AT_name:
                /*  Even if we saw DW_AT_abstract_origin, go ahead
                    and take DW_AT_name. */
                sres = dwarf_formstring(atlist[i], &temps, &aterr);
                if (sres == DW_DLV_ERROR) {
                    glflags.gf_count_major_errors++;
                    printf("\nERROR: "
                        "formstring in get_proc_name failed\n");
                    esb_append(proc_name,
                        "ERROR in dwarf_formstring!");
                    dwarf_dealloc(dbg,aterr,DW_DLA_ERROR);
                    aterr = 0;
                } else if (sres == DW_DLV_NO_ENTRY) {
                    esb_append(proc_name,
                        "NO ENTRY on dwarf_formstring?!");
                } else {
                    esb_append(proc_name,temps);
                }
                funcnamefound = 1; /* FOUND THE NAME (sort of,
                    if error) */
                break;
            case DW_AT_low_pc:
                dres = dwarf_formaddr(atlist[i],
                    &low_pc_for_die, &aterr);
                funcpcfound = 1;
                if (dres == DW_DLV_ERROR) {
                    if (DW_DLE_MISSING_NEEDED_DEBUG_ADDR_SECTION ==
                        dwarf_errno(aterr)) {
                        glflags.gf_debug_addr_missing = 1;
                    } else {
                        glflags.gf_count_major_errors++;
                        printf("\nERROR: dwarf_formaddr() failed"
                            " in get_proc_name. %s\n",
                            dwarf_errmsg(aterr));
                        /* the long name is horrible */
                        if (!glflags.gf_error_code_search_by_address){
                            /* error codes never big numbers. */
                            glflags.gf_error_code_search_by_address=
                                (int)dwarf_errno(aterr);
                        }
                    }
                    dwarf_dealloc(dbg,aterr,DW_DLA_ERROR);
                    aterr = 0;
                    funcpcfound = 0;
                    low_pc_for_die = 0;
                    /* low_pc_for_die = ~low_pc; ??? */
                    loop_ok = FALSE;
                    /* ensure no match */
                } else if (dres == DW_DLV_NO_ENTRY) {
                    funcpcfound = 0;
                    loop_ok = FALSE;
                }
                break;
            default:
                break;
            } /* end switch */
        } /* end DW_DLV_OK */
    } /* end for loop on atcnt */
    dealloc_local_atlist(dbg,atlist,atcnt);
    if (funcnamefound && funcpcfound && pcMap ) {
        /*  Insert the name to map even if not
            the low_pc we are looking for.
            This version does extra work in that
            early symbols in a CU will be inserted
            multiple times (the extra times have no
            effect). */
        addr_map_insert(low_pc_for_die,esb_get_string(proc_name),
            pcMap);
    }
    if (funcnamefound == 0 || funcpcfound == 0 ||
        low_pc != low_pc_for_die) {
        funcres = DW_DLV_NO_ENTRY;
    }
    return funcres;
}

/*  Modified Depth First Search looking for the procedure:
    a)  only looks for children of subprogram.
    b)  With subprogram looks at current die *before* looking
        for a child.

    Needed since some languages, including SGI MP Fortran,
    have nested functions.
    Return 0 on failure, 1 on success.
*/
static int
load_nested_proc_name(Dwarf_Debug dbg, Dwarf_Die die,
    Dwarf_Addr low_pc,
    struct esb_s *ret_name,
    Dwarf_Die *cu_die_for_print_frames,
    void **pcMap,
    Dwarf_Error *err)
{
    Dwarf_Die curdie = die;
    int die_locally_gotten = 0;
    Dwarf_Die prev_child = 0;
    Dwarf_Die newchild = 0;
    Dwarf_Die newsibling = 0;
    Dwarf_Half tag;
    int chres = DW_DLV_OK;
    struct esb_s nestname;

    esb_constructor(&nestname);
    while (chres == DW_DLV_OK) {
        int tres = 0;

        esb_empty_string(&nestname);
        tres = dwarf_tag(curdie, &tag, err);
        newchild = 0;
        if (tres == DW_DLV_OK) {
            int lchres = 0;

            if (tag == DW_TAG_subprogram) {
                int gotit = 0;
                Dwarf_Error locerr = 0;
                gotit = get_proc_name_by_die(dbg, curdie, low_pc,
                    &nestname, cu_die_for_print_frames,
                    pcMap,&locerr);
                if (gotit == DW_DLV_OK) {
                    if (die_locally_gotten) {
                        /*  If we got this die from the parent, we do
                            not want to dealloc here! */
                        dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
                    }
                    esb_append(ret_name,esb_get_string(&nestname));
                    esb_destructor(&nestname);
                    return DW_DLV_OK;
                }
                if (gotit == DW_DLV_ERROR) {
                    dwarf_dealloc(dbg,locerr,DW_DLA_ERROR);
                    locerr = 0;
                }
                /* Check children of subprograms recursively should
                    this really be check children of anything,
                    or just children of subprograms? */

                lchres = dwarf_child(curdie, &newchild, err);
                esb_empty_string(&nestname);
                if (lchres == DW_DLV_OK) {
                    int newprog = 0;
                    Dwarf_Error innererr = 0;
                    /* look for inner subprogram */
                    newprog = load_nested_proc_name(dbg,
                        newchild, low_pc,
                        &nestname,
                        cu_die_for_print_frames,
                        pcMap,&innererr);
                    dwarf_dealloc(dbg, newchild, DW_DLA_DIE);
                    if (newprog == DW_DLV_OK) {
                        /*  Found it.  We could just take this
                            name or we could concatenate names
                            together. For now,
                            just take name */
                        if (die_locally_gotten) {
                            /*  If we got this die from the parent,
                                we do not want to dealloc here! */
                            dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
                        }
                        esb_append(ret_name,esb_get_string(
                            &nestname));
                        esb_destructor(&nestname);
                        return DW_DLV_OK;
                    } else if (newprog == DW_DLV_ERROR) {
                        dwarf_dealloc(dbg,innererr,DW_DLA_ERROR);
                        innererr = 0;
                    }
                } else if (lchres == DW_DLV_NO_ENTRY) {
                    /* nothing to do */
                } else {
                    load_CU_error_data(dbg,*cu_die_for_print_frames);
                    simple_err_only_return_action(lchres,
                        "\nERROR:load_nested_proc_name dwarf_child()"
                        " failed.");
                    if (die_locally_gotten) {
                        /*  If we got this die from the parent, we do
                            not want to dealloc here! */
                        dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
                    }
                    esb_destructor(&nestname);
                    return lchres;
                }
            }  /* end if TAG_subprogram */
        } else {
            esb_empty_string(&nestname);
            if (tres == DW_DLV_ERROR)  {
                struct esb_s m;

                load_CU_error_data(dbg,*cu_die_for_print_frames);
                esb_constructor(&m);
                esb_append_printf_s(&m,
                    "\nERROR: load_nested_proc_name dwarf_tag failed:"
                    " trying to get proc name. "
                    "Error is %s.",dwarf_errmsg(*err));
                simple_err_only_return_action(tres,
                    esb_get_string(&m));
                esb_destructor(&m);
                esb_destructor(&nestname);
                return tres;
            }
            if (die_locally_gotten) {
                /*  If we got this die from the parent, we do not want
                    to dealloc here! */
                dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
            }
            esb_destructor(&nestname);
            return DW_DLV_NO_ENTRY;
        }
        /* try next sibling */
        prev_child = curdie;
        esb_empty_string(&nestname);
#ifdef ORIGINAL_HEADER_API
        chres = dwarf_siblingof_b(dbg, curdie,
            dwarf_get_die_infotypes_flag(curdie),
            &newsibling, err);
#else
        chres = dwarf_siblingof_c(curdie,&newsibling,err);
#endif /* ORIGINAL_HEADER_API */
        if (chres == DW_DLV_ERROR) {
            struct esb_s m;

            load_CU_error_data(dbg,*cu_die_for_print_frames);
            esb_constructor(&m);
            esb_append(&m,
                "Looking for function name for "
                "a frame. "
                "dwarf_siblingof failed"
                " trying to get the name. ");
            print_error_and_continue(
                esb_get_string(&m), chres,*err);
            esb_destructor(&m);
            DROP_ERROR_INSTANCE(dbg,chres,*err);
            if (die_locally_gotten) {
                /*  If we got this die from the parent, we do not want
                    to dealloc here! */
                dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
            }
            esb_destructor(&nestname);
            return DW_DLV_NO_ENTRY;
        } else if (chres == DW_DLV_NO_ENTRY) {
            if (die_locally_gotten) {
                /*  If we got this die from the parent, we do not want
                    to dealloc here! */
                dwarf_dealloc(dbg, prev_child, DW_DLA_DIE);
                prev_child = 0;
            }
            /* Not there at this level */
            esb_destructor(&nestname);
            return DW_DLV_NO_ENTRY;
        }
        /* DW_DLV_OK */
        curdie = newsibling;
        if (die_locally_gotten) {
            /*  If we got this die from the parent, we do not want
                to dealloc here! */
            dwarf_dealloc(dbg, prev_child, DW_DLA_DIE);
            /*  Clearing prev_child here gets coverity 330895
                unused value warning.
                prev_child = 0; */
        }
        die_locally_gotten = 1;
    }
    if (die_locally_gotten) {
        /*  If we got this die from the parent, we do not want to
            dealloc here! */
        dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
    }
    esb_destructor(&nestname);
    return DW_DLV_NO_ENTRY;
}

/*  For SGI MP Fortran and other languages, functions
    nest!  As a result, we must dig thru all functions,
    not just the top level.
    This remembers the CU die and restarts each search at the start
    of  the current cu.
    Return DW_DLV_OK means found name.
    Return DW_DLV_NO_ENTRY means not found name.
    Never returns DW_DLV_ERROR
*/
static int
get_fde_proc_name_by_address(Dwarf_Debug dbg, Dwarf_Addr low_pc,
    const char *frame_section_name,
    struct esb_s *name,
    Dwarf_Die *cu_die_for_print_frames,
    void **pcMap,Dwarf_Error *err)
{
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_offset = 0;
    int cures = DW_DLV_OK;
    int dres = DW_DLV_OK;
    int chres = DW_DLV_OK;
    struct Addr_Map_Entry *ame = 0;
    Dwarf_Half length_size = 0;
    Dwarf_Half extension_size = 0;
    Dwarf_Sig8 type_signature;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Half header_cu_type = 0;
    Dwarf_Bool is_info = TRUE; /* An assumption, but
        sensible as functions will not be in .debug_types */

    type_signature = zero_type_signature;
    ame = addr_map_find(low_pc,pcMap);
    if (ame && ame->mp_name) {
        esb_append(name,ame->mp_name);
        return DW_DLV_OK;
    }
    if (glflags.gf_all_cus_seen_search_by_address) {
        return DW_DLV_NO_ENTRY;
    }
    if (glflags.gf_debug_addr_missing) {
        return DW_DLV_NO_ENTRY;
    }
    if (*cu_die_for_print_frames == NULL) {
        /* Call depends on dbg->cu_context to know what to do. */
        cures = dwarf_next_cu_header_e(dbg,
            is_info,
            cu_die_for_print_frames,
            &cu_header_length,
            &version_stamp, &abbrev_offset,
            &address_size,
            &length_size,
            &extension_size,
            &type_signature,
            &typeoffset,
            &next_cu_offset,
            &header_cu_type,
            err);
        if (cures == DW_DLV_ERROR) {
            /*  If there is a serious error in DIE information
                we just skip looking for a procedure name.
                Perhaps we should report something? */
            printf("\nERROR: Error getting next cu header "
                "looking for a subroutine"
                "/procedure name. Section %s. Err is %s\n",
                sanitized(frame_section_name),
                dwarf_errmsg(*err));
                glflags.gf_count_major_errors++;
            glflags.gf_all_cus_seen_search_by_address = 1;
            DROP_ERROR_INSTANCE(dbg,cures,*err);
            return DW_DLV_NO_ENTRY;
        } else if (cures == DW_DLV_NO_ENTRY) {
            /* loop thru the list again */
            *cu_die_for_print_frames = 0;
        } else {                /* DW_DLV_OK */
#ifdef ORIGINAL_HEADER_API
            dres = dwarf_siblingof_b(dbg, NULL,
                is_info,
                cu_die_for_print_frames,
                err);
            if (dres == DW_DLV_ERROR) {
                /*  If there is a serious error in DIE information
                    we just skip looking for a procedure name.
                    Perhaps we should report something? */
                printf("\nERROR: Error getting "
                    "dwarf_siblingof when looking for"
                    " procedure name. "
                    "Section %s. Err is %s\n",
                    sanitized(frame_section_name),
                    dwarf_errmsg(*err));
                glflags.gf_count_major_errors++;
                DROP_ERROR_INSTANCE(dbg,dres,*err);
                glflags.gf_all_cus_seen_search_by_address = 1;
                return DW_DLV_NO_ENTRY;;
            }
            if (dres == DW_DLV_NO_ENTRY) {
                /*  No initial die? Something is wrong! */
                return dres;
            }
#endif /* ORIGINAL_HEADER_API */
        }
    }
    if (dres == DW_DLV_OK) {
        Dwarf_Die child = 0;

        if (*cu_die_for_print_frames == 0) {
            /*  no information. Possibly a stripped file */
            return DW_DLV_NO_ENTRY;
        }
        chres = dwarf_child(*cu_die_for_print_frames,
            &child, err);
        if (chres == DW_DLV_ERROR) {
            printf("\nERROR: Error getting "
                "dwarf_child(). "
                "Section %s. Err is %s\n",
                sanitized(frame_section_name),
                dwarf_errmsg(*err));
            DROP_ERROR_INSTANCE(dbg,chres,*err);
            glflags.gf_count_major_errors++;
            glflags.gf_all_cus_seen_search_by_address = 1;
            return DW_DLV_NO_ENTRY;
        } else if (chres == DW_DLV_NO_ENTRY) {
            /* FALL THROUGH to look for more CU headers  */
        } else { /* DW_DLV_OK */
            int gotname = 0;
            gotname = load_nested_proc_name(dbg, child, low_pc, name,
                cu_die_for_print_frames,
                pcMap,err);
            dwarf_dealloc(dbg, child, DW_DLA_DIE);
            if (gotname == DW_DLV_OK) {
                return DW_DLV_OK;
            }
            if (gotname == DW_DLV_ERROR) {
                glflags.gf_all_cus_seen_search_by_address = 1;
                DROP_ERROR_INSTANCE(dbg,gotname,*err);
                return DW_DLV_NO_ENTRY;
            }
            child = 0;
        }
    }
    for (;;) {
        Dwarf_Die ldie = 0;

        type_signature = zero_type_signature;
#ifdef ORIGINAL_HEADER_API

        cures = dwarf_next_cu_header_d(dbg,
            is_info, &cu_header_length,
            &version_stamp, &abbrev_offset,
            &address_size,
            &length_size,&extension_size,
            &type_signature,&typeoffset,
            &next_cu_offset,
            &header_cu_type,
            err);
#else
        cures = dwarf_next_cu_header_e(dbg,
            is_info, &ldie, &cu_header_length,
            &version_stamp, &abbrev_offset,
            &address_size,
            &length_size,&extension_size,
            &type_signature,&typeoffset,
            &next_cu_offset,
            &header_cu_type,
            err);
#endif
        if (cures != DW_DLV_OK) {
            if (cures == DW_DLV_ERROR) {
                printf("\nERROR: Error getting "
                    "next_cu_header "
                    "Section %s. Err is %s\n",
                    sanitized(frame_section_name),
                    dwarf_errmsg(*err));
                DROP_ERROR_INSTANCE(dbg,cures,*err);
                glflags.gf_count_major_errors++;
                glflags.gf_all_cus_seen_search_by_address = 1;
                return DW_DLV_NO_ENTRY;
            }
            glflags.gf_all_cus_seen_search_by_address = 1;
            break;
        }

#ifdef ORIGINAL_HEADER_API
        dres = dwarf_siblingof_b(dbg,NULL,is_info, &ldie, err);
#else
        dres = DW_DLV_OK;
#endif
        if (*cu_die_for_print_frames) {
            dwarf_dealloc(dbg, *cu_die_for_print_frames,DW_DLA_DIE);
            *cu_die_for_print_frames = 0;
        }
#ifdef ORIGINAL_HEADER_API
        if (dres == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,dres,*err);
            glflags.gf_all_cus_seen_search_by_address = 1;
            return dres;
        } else if (dres == DW_DLV_NO_ENTRY) {
            return dres;
        }
#endif /* ORIGINAL_HEADER_API */
        /*  DW_DLV_OK
            In normal processing (ie, when doing print_info()
            we would call print_attribute for each die
            including cu_die and thus get CU_base_address,
            CU_high_address, PU_base_address, PU_high_address,
            CU_name for PRINT_CU_INFO() in case of error.  */
        *cu_die_for_print_frames = ldie;
        {
            int chpfres = 0;
            Dwarf_Die child = 0;

            chpfres =
                dwarf_child(*cu_die_for_print_frames, &child,
                    err);
            if (chpfres == DW_DLV_ERROR) {
                load_CU_error_data(dbg,*cu_die_for_print_frames);
                glflags.gf_count_major_errors++;
                printf("\nERROR: Getting procedure name "
                    "dwarf_child fails "
                    " %s\n",dwarf_errmsg(*err));
                DROP_ERROR_INSTANCE(dbg,chpfres,*err);
                glflags.gf_all_cus_seen_search_by_address = 1;
                return DW_DLV_NO_ENTRY;
            } else if (chpfres == DW_DLV_NO_ENTRY) {
                /* FALL THROUGH to loop more */
            } else {
                /* DW_DLV_OK) */
                int gotname = 0;

                gotname = load_nested_proc_name(dbg, child,
                    low_pc, name,
                    cu_die_for_print_frames,
                    pcMap,err);
                dwarf_dealloc(dbg, child, DW_DLA_DIE);
                if (gotname == DW_DLV_OK) {
                    return gotname;
                }
                if (gotname == DW_DLV_ERROR) {
                    DROP_ERROR_INSTANCE(dbg,gotname,*err);
                    glflags.gf_all_cus_seen_search_by_address = 1;
                    return DW_DLV_NO_ENTRY;
                }
            }
        }
        reset_overall_CU_error_data();
    }
    return DW_DLV_NO_ENTRY;
}

/*  Attempting to take care of overflows so we
    only accept good as TRUE. */
static Dwarf_Bool
valid_fde_content(Dwarf_Small * fde_start,
    Dwarf_Unsigned fde_length,
    Dwarf_Small * data_ptr,
    Dwarf_Unsigned data_ptr_length)
{
    Dwarf_Small * fde_end = fde_start + fde_length;
    Dwarf_Small * data_end = 0;

    if (data_ptr < fde_start ||
        data_ptr >= fde_end) {
        return FALSE;
    }
    data_end = data_ptr + data_ptr_length;
    if ( data_end < fde_start || data_end < data_ptr) {
        return FALSE;
    }
    if ( data_end  >= fde_end) {
        return FALSE;
    }
    return TRUE;
}

static void
dd_check_fde_cie(Dwarf_Debug dbg,
    Dwarf_Fde fde,
    Dwarf_Signed cie_index,
    Dwarf_Cie cie)
{
    Dwarf_Error  error;
    Dwarf_Cie    ret_cie = 0;
    Dwarf_Signed ret_index = 0;
    int          res = 0;

    if (!glflags.gf_check_functions) {
        return;
    }
    DWARF_CHECK_COUNT(check_functions_result,1);
    res = dwarf_get_cie_of_fde(fde,&ret_cie,&error);
    if (res == DW_DLV_ERROR) {
        DWARF_CHECK_ERROR2(check_functions_result,
            "Error calling dwarf_get_cie_of_fde() "
            "underlying error is:",
            dwarf_errmsg(error));
        DROP_ERROR_INSTANCE(dbg,res,error);
    } else if (res == DW_DLV_NO_ENTRY) {
        DWARF_CHECK_ERROR(check_functions_result,
            "NO_ENTRY calling dwarf_get_cie_of_fde()");
    } else  { /* OK */
        if ( ret_cie != cie) {
            DWARF_CHECK_ERROR(check_functions_result,
                "dwarf_get_cie_of_fde() returns "
                "inconsistent cie");
        }
    }
    res = dwarf_get_cie_index(cie,&ret_index,&error);
    if (res == DW_DLV_ERROR) {
        DWARF_CHECK_ERROR2(check_functions_result,
            "Error calling dwarf_get_index() "
            "underlying error is:",
            dwarf_errmsg(error));
        DROP_ERROR_INSTANCE(dbg,res,error);
    } else if (res == DW_DLV_NO_ENTRY) {
        DWARF_CHECK_ERROR(check_functions_result,
            "NO_ENTRY calling dwarf_get_cie_index()");
    } else  { /* OK */
        if ( ret_index != cie_index) {
            DWARF_CHECK_ERROR(check_functions_result,
                "dwarf_get_cie_index() returns"
                " mismatched cie index");
        }
    }
}

static void
dd_check_retrieved_fde(Dwarf_Debug dbg,
    Dwarf_Fde *fde_data,
    Dwarf_Fde  expected_fde,
    Dwarf_Addr pc)
{
    int         res = 0;
    Dwarf_Error error = 0;
    Dwarf_Fde   returned_fde = 0;
    Dwarf_Addr  lopc = 0;
    Dwarf_Addr  hipc = 0;

    DWARF_CHECK_COUNT(check_functions_result,1);
    res = dwarf_get_fde_at_pc(fde_data,pc,
        &returned_fde,&lopc,&hipc,&error);
    if (res == DW_DLV_ERROR) {
        DWARF_CHECK_ERROR2(check_functions_result,
            "Error calling dwarf_get_fde_at_pc() "
            "underlying error is:",
            dwarf_errmsg(error));
        DROP_ERROR_INSTANCE(dbg,res,error);
    } else if (res == DW_DLV_NO_ENTRY) {
        struct esb_s m;

        esb_constructor(&m);
        esb_append_printf_u(&m,
            "NO_ENTRY calling dwarf_get_fde_at_pc()"
            " got NO ENTRY for pc 0x%"
            DW_PR_XZEROS DW_PR_DUx
            " which indicates corrupt dwarf",pc);
        DWARF_CHECK_ERROR(check_functions_result,
            esb_get_string(&m));
        esb_destructor(&m);
    } else  { /* OK */
        if ( returned_fde != expected_fde) {
            DWARF_CHECK_ERROR(check_functions_result,
                "dwarf_get_fde_at_pc() returns"
                " mismatched fde index");
        }
    }
}
static const Dwarf_Block blockzero;

/*  Gather the fde print logic here so the control logic
    determining what FDE to print is clearer.  */
static int
print_one_fde(Dwarf_Debug dbg,
    const char *frame_section_name,
    Dwarf_Fde  *fde_data,
    Dwarf_Signed fde_index,
    Dwarf_Cie * cie_data,
    Dwarf_Signed cie_element_count,
    Dwarf_Half address_size,
    Dwarf_Half offset_size,
    Dwarf_Half version,
    int        is_eh,
    struct dwconf_s *config_data,
    void    ** pcMap,
    void    ** lowpcSet,
    Dwarf_Die *cu_die_for_print_frames,
    Dwarf_Error *err)
{
    Dwarf_Addr     j = 0;
    Dwarf_Addr     low_pc = 0;
    Dwarf_Unsigned func_length = 0;
    Dwarf_Addr     end_func_addr = 0;
    Dwarf_Small   *fde_bytes = NULL;
    Dwarf_Unsigned fde_bytes_length = 0;
    Dwarf_Off      cie_offset = 0;
    Dwarf_Signed   cie_index = 0;
    Dwarf_Off      fde_offset = 0;
    Dwarf_Signed   eh_table_offset = 0;
    int            fres = 0;
    int            offres = 0;
    struct esb_s   temps;
    int            printed_intro_addr = 0;
    char           local_buf[100];
    char           temps_buf[200];
    Dwarf_Fde      fde = 0;
    unsigned long  fde_checking_count = 0;

    fde = fde_data[fde_index];
    fres = dwarf_get_fde_range(fde,

        &low_pc, &func_length,
        &fde_bytes,
        &fde_bytes_length,
        &cie_offset, &cie_index,
        &fde_offset, err);
    if (fres == DW_DLV_ERROR) {
        glflags.gf_count_major_errors++;
        printf("ERROR: calling dwarf_get_fde_range() on "
            "index %" DW_PR_DUu "gets an error! Error is %s\n",
            fde_index,dwarf_errmsg(*err));
        return fres;
    }
    if (fres == DW_DLV_NO_ENTRY) {
        return DW_DLV_NO_ENTRY;
    }
    dd_check_fde_cie(dbg,fde,cie_index,cie_data[cie_index]);
    if (glflags.gf_cu_name_flag &&
        glflags.fde_offset_for_cu_low != DW_DLV_BADOFFSET &&
        (fde_offset < glflags.fde_offset_for_cu_low ||
        fde_offset > glflags.fde_offset_for_cu_high)) {
        return DW_DLV_NO_ENTRY;
    }
    /* eh_table_offset was IRIX ONLY. */
    fres = dwarf_get_fde_exception_info(fde,
        &eh_table_offset, err);
    if (fres == DW_DLV_ERROR) {
        glflags.gf_count_major_errors++;
        printf("ERROR: Got error looking for SGI-only "
            "exception table "
            "offset from fde! Error is %s\n",
            dwarf_errmsg(*err));
        return fres;
    }
    esb_constructor_fixed(&temps,temps_buf,sizeof(temps_buf));
    if (glflags.gf_suppress_nested_name_search) {
        /* do nothing. */
    } else {
        struct Addr_Map_Entry *mp = 0;

        esb_empty_string(&temps);
        mp = addr_map_find(low_pc,lowpcSet);
        if (glflags.gf_check_frames ||
            glflags.gf_check_frames_extended) {
            DWARF_CHECK_COUNT(fde_duplication,1);
        }
        fres = get_fde_proc_name_by_address(dbg, low_pc,
            frame_section_name,
            &temps,
            cu_die_for_print_frames,
            pcMap,err);
        if (fres == DW_DLV_ERROR) {
            /*  Failing to get the name is not a crucial
                thing. Do not error off.
                We should not get here as the
                invariant for get_fde_proc_name_by_address()
                says it never returns DW_DLV_ERROR; */
            DROP_ERROR_INSTANCE(dbg,fres,*err);
            fres = DW_DLV_NO_ENTRY;
        }
        /*  If found the name is in temps now, or temps
            is the empty string. */
        if (mp) {
            if (glflags.gf_check_frames ||
                glflags.gf_check_frames_extended) {
                struct esb_s msg;

                esb_constructor_fixed(&msg,
                    local_buf,sizeof(local_buf));
                if (esb_string_len(&temps) > 0) {
                    esb_append_printf_u(&msg,
                        "An fde low pc of 0x%"
                        DW_PR_DUx
                        " is not the first fde with that pc. ",
                        low_pc);
                    esb_append_printf_s(&msg,
                        "The first is named \"%s\"",
                        sanitized(esb_get_string(&temps)));
                } else {
                    esb_append_printf_u(&msg,
                        "An fde low pc of 0x%"
                        DW_PR_DUx
                        " is not the first fde with that pc. "
                        "The first is not named.",
                        (Dwarf_Unsigned)low_pc);
                }
                DWARF_CHECK_ERROR(fde_duplication,
                    esb_get_string(&msg));
                esb_destructor(&msg);
            }
        } else if (fres == DW_DLV_OK) {
            addr_map_insert(low_pc,0,lowpcSet);
        } /* Else we just don't know anything, so record nothing. */
    }

    /* Do not print if in check mode */
    if (glflags.gf_do_print_dwarf) {
        /* Printing the FDE header. */
        printf("<%5" DW_PR_DSd ">"
            "<0x%" DW_PR_XZEROS  DW_PR_DUx
            ":0x%" DW_PR_XZEROS DW_PR_DUx
            "><%s>"
            "<cie offset 0x%" DW_PR_XZEROS DW_PR_DUx ":"
            ":cie index %5"  DW_PR_DUu ">"
            "<fde offset 0x%" DW_PR_XZEROS DW_PR_DUx
            " length: 0x%" DW_PR_XZEROS  DW_PR_DUx ">",
            fde_index,
            (Dwarf_Unsigned)low_pc,
            (Dwarf_Unsigned)(low_pc + func_length),
            sanitized(esb_get_string(&temps)),
            (Dwarf_Unsigned)cie_offset,
            (Dwarf_Unsigned)cie_index,
            (Dwarf_Unsigned)fde_offset,
            fde_bytes_length);
    }
    esb_destructor(&temps);
    if (!is_eh) {
        /* IRIX used eh_table_offset. No one else uses it. */
        /* Do not print if in check mode */
        if (glflags.gf_do_print_dwarf) {
            if (eh_table_offset == DW_DLX_NO_EH_OFFSET) {
                printf("<eh offset %s>\n", "none");
            } else if (eh_table_offset ==
                DW_DLX_EH_OFFSET_UNAVAILABLE) {
                printf("<eh offset %s>\n", "unknown");
            } else {
                printf("<eh offset 0x%" DW_PR_XZEROS DW_PR_DUx
                    ">\n", eh_table_offset);
            }
        }
    } else {
        /*  Printing the .eh_frame header augmentation string,
            if any. */
        int ares = 0;
        Dwarf_Small *data = 0;
        Dwarf_Unsigned len = 0;

        ares = dwarf_get_fde_augmentation_data(fde, &data, &len, err);
        if (ares == DW_DLV_ERROR) {
            glflags.gf_count_major_errors++;
            printf("ERROR: on getting augmentation data for an fde."
                " Error is %s\n",dwarf_errmsg(*err));
            return ares;
        }
        if (ares == DW_DLV_NO_ENTRY) {
            /* do nothing. */
        } else if (ares == DW_DLV_OK) {
            if (glflags.gf_do_print_dwarf) {
                printf("\n       <eh aug data len 0x%"
                    DW_PR_DUx, len);
                if (len) {
                    if (!valid_fde_content(fde_bytes,fde_bytes_length,
                        data,len) ) {
                        glflags.gf_count_major_errors++;
                        printf("ERROR:The .eh_frame augmentation "
                            "data "
                            "is too large to print");
                    } else {
                        Dwarf_Unsigned k2 = 0;

                        for (k2 = 0; k2 < len; ++k2) {
                            if (k2 == 0) {
                                printf(" bytes 0x");
                            }
                            printf("%02x ", (unsigned char) data[k2]);
                        }
                    }
                }
                printf(">");
            }
        }
        /* Do not print if in check mode */
        if (glflags.gf_do_print_dwarf) {
            printf("\n");
        }
    }
    end_func_addr = low_pc + func_length;
    for (j = low_pc; j < end_func_addr; j++) {
        Dwarf_Half k = 0;
        Dwarf_Addr cur_pc_in_table = 0;

        cur_pc_in_table = j;
        if (glflags.gf_check_functions &&
            fde_checking_count < 100){ /* 100 is arbitrary */
            ++fde_checking_count;
            dd_check_retrieved_fde(dbg,fde_data,fde,cur_pc_in_table);
        }
        {
            Dwarf_Unsigned reg = 0;
            Dwarf_Unsigned offset_relevant = 0;
            Dwarf_Small  value_type = 0;
            /*  DWARF has always said the offset is signed
                in an offset(n) or val_offset(N) register rule.
                the API functions here are formally
                incorrect. So we cast the value returned
                to us to Dwarf_Signed at point of use. */
            Dwarf_Signed offset = 0;
            Dwarf_Block block;
            Dwarf_Addr   row_pc = 0;
            Dwarf_Bool   has_more_rows = 0;
            Dwarf_Addr   subsequent_pc = 0;
            int fires = 0;

            block = blockzero;
#ifdef TESTOLDDW_OFFSET
            {
            Dwarf_Unsigned s = 0;
            fires = dwarf_get_fde_info_for_cfa_reg3_b(fde,
                j,
                &value_type,
                &offset_relevant,
                &reg,
                &s,
                &block,
                &row_pc,
                &has_more_rows,
                &subsequent_pc,
                err);
            offset = (Dwarf_Signed)s;
            }
#else
            fires = dwarf_get_fde_info_for_cfa_reg3_c(fde,
                j,
                &value_type,
                &offset_relevant,
                &reg,
                &offset,
                &block,
                &row_pc,
                &has_more_rows,
                &subsequent_pc,
                err);
#endif
            if (fires == DW_DLV_ERROR) {
                glflags.gf_count_major_errors++;
                printf("\nERROR: on getting fde details for "
                    "fde row for address 0x%"
                    DW_PR_XZEROS DW_PR_DUx ": %s\n",
                    j,dwarf_errmsg(*err));
                return fires;
            }
            if (fires == DW_DLV_NO_ENTRY) {
                continue;
            }
            if (!has_more_rows) {
                j = low_pc+func_length-1;
            } else {
                if (subsequent_pc > j) {
                    /*  Loop head will increment j to make up
                        for -1 here. */
                    j = subsequent_pc -1;

                }
            }
            /* Do not print if in check mode */
            if (!printed_intro_addr && glflags.gf_do_print_dwarf) {
                printf("        0x%" DW_PR_XZEROS DW_PR_DUx
                    ": ", (Dwarf_Unsigned)cur_pc_in_table);
                printed_intro_addr = 1;
            }
            print_one_frame_reg_col(dbg,
                *cu_die_for_print_frames,
                config_data->cf_cfa_reg,
                value_type,
                reg,
                address_size,
                offset_size,version,
                config_data,
                offset_relevant, offset, &block);
        }
        for (k = 0; k < config_data->cf_table_entry_count; k++) {
            Dwarf_Unsigned reg = 0;
            Dwarf_Unsigned offset_relevant = 0;
            int fires = 0;
            Dwarf_Small value_type = 0;
            Dwarf_Block block;
            Dwarf_Signed offset = 0;
            Dwarf_Addr row_pc = 0;
            Dwarf_Bool has_more_rows = FALSE;
            Dwarf_Addr subsequent_pc = 0;

            block = blockzero;
            {
#if TESTOLDDW_OFFSET
                Dwarf_Unsigned s = 0;
                fires = dwarf_get_fde_info_for_reg3_b(fde,
                    k,
                    cur_pc_in_table,
                    &value_type,
                    &offset_relevant,
                    &reg,
                    &s,
                    &block,
                    &row_pc, &has_more_rows,&subsequent_pc, err);
                offset = (Dwarf_Signed)s;
#else
                fires = dwarf_get_fde_info_for_reg3_c(fde,
                    k,
                    cur_pc_in_table,
                    &value_type,
                    &offset_relevant,
                    &reg,
                    &offset,
                    &block,
                    &row_pc, &has_more_rows,&subsequent_pc, err);
#endif
            }
            if (fires == DW_DLV_ERROR) {
                printf("\n");
                glflags.gf_count_major_errors++;
                printf("\nERROR:  on getting fde details for "
                    "row address 0x%" DW_PR_XZEROS DW_PR_DUx
                    " table column %d.\n",
                    j,k);
                return fires;
            }
            if (fires == DW_DLV_NO_ENTRY) {
                continue;
            }
            if (row_pc != cur_pc_in_table) {
                /*  row_pc < cur_pc_in_table means this pc has no
                    new register value, the last one found still
                    applies hence this is a duplicate row.
                    row_pc > j cannot happen, the libdwarf function
                    will not return such. */
                continue;
            }

            /* Do not print if in check mode */
            if (!printed_intro_addr && glflags.gf_do_print_dwarf) {
                printf("        0x%" DW_PR_XZEROS DW_PR_DUx ": ",
                    (Dwarf_Unsigned)j);
                printed_intro_addr = 1;
            }
            print_one_frame_reg_col(dbg,
                *cu_die_for_print_frames,
                k,
                value_type,
                reg,
                address_size,
                offset_size,version,
                config_data,
                offset_relevant, offset, &block);
        }
        if (printed_intro_addr) {
            printf("\n");
            printed_intro_addr = 0;
        }
    }
    if (glflags.verbose > 1) {
        Dwarf_Off fde_off = 0;
        Dwarf_Off cie_off = 0;
        int fderes = 0;

        /*  Get the fde instructions and print them in
            raw form. */
        Dwarf_Small * fde_instrs = 0;
        Dwarf_Unsigned fdeinstrslen = 0;

        offres = dwarf_fde_section_offset(dbg, fde, &fde_off,
            &cie_off, err);
        if (offres == DW_DLV_ERROR) {
            glflags.gf_count_major_errors++;
            printf("\nERROR:  on getting fde section offset"
                " of this fde\n");
            return offres;
        }
        if (offres == DW_DLV_NO_ENTRY) {
            glflags.gf_count_major_errors++;
            printf("\nERROR:  Impossible, no fde offset or "
                " cie offset for "
                "fde index "
                "%" DW_PR_DUu "?\n",
                fde_index);
            return offres;
        }
        {
            /* Do not print if in check mode */
            if (glflags.gf_do_print_dwarf) {
                printf(" fde section offset %" DW_PR_DUu
                    " 0x%" DW_PR_XZEROS DW_PR_DUx
                    " cie offset for fde: %" DW_PR_DUu
                    " 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                    (Dwarf_Unsigned) fde_off,
                    (Dwarf_Unsigned) fde_off,
                    (Dwarf_Unsigned) cie_off,
                    (Dwarf_Unsigned) cie_off);
            }
        }
        fderes = dwarf_get_fde_instr_bytes(fde,
            &fde_instrs,&fdeinstrslen,err);
        if (fderes == DW_DLV_ERROR) {
            printf("\nERROR:  getting fde instructions "
                " failed %s \n",dwarf_errmsg(*err));
            glflags.gf_count_major_errors++;
            return offres;
        }
        /*  fdeinstrslen will be zero if DW_DLV_NO_ENTRY. */
        {
            int cires = 0;
            Dwarf_Frame_Instr_Head ihead = 0;
            Dwarf_Unsigned         iarraycount = 0;
            Dwarf_Unsigned cie_length = 0;
            Dwarf_Small cie_version = 0;
            char* augmenter = 0;
            Dwarf_Unsigned code_alignment_factor = 0;
            Dwarf_Signed data_alignment_factor = 0;
            Dwarf_Half return_address_register_rule = 0;
            Dwarf_Small * initial_instructions = 0;
            Dwarf_Unsigned initial_instructions_length = 0;
            Dwarf_Half cie_offset_size = 0;

            if (cie_index >= cie_element_count) {
                glflags.gf_count_major_errors++;
                printf("\nERROR: Bad cie index %" DW_PR_DSd
                    " with fde index %" DW_PR_DUu "! "
                    "(table entry max %" DW_PR_DSd ")\n",
                    cie_index, fde_index,
                    cie_element_count);
                return DW_DLV_NO_ENTRY;
            }
            cires = dwarf_get_cie_info_b(cie_data[cie_index],
                &cie_length,
                &cie_version,
                &augmenter,
                &code_alignment_factor,
                &data_alignment_factor,
                &return_address_register_rule,
                &initial_instructions,
                &initial_instructions_length,
                &cie_offset_size,
                err);
            if (cires == DW_DLV_ERROR) {
                glflags.gf_count_major_errors++;
                printf("\nERROR: Bad cie index %" DW_PR_DSd
                    " with fde index %" DW_PR_DUu "!\n",
                    cie_index,  fde_index);
                return cires;
            }
            if (cires == DW_DLV_NO_ENTRY) {
                /*  Impossible. Do nothing. */
                glflags.gf_count_major_errors++;
                    printf("\nERROR: Impossible: no cie info  for "
                    "cie index %" DW_PR_DUu "?\n", cie_index);
                return cires;
            }
            if (glflags.gf_do_print_dwarf) {
                int res = 0;

                res = dwarf_expand_frame_instructions(
                    cie_data[cie_index],
                    fde_instrs,
                    fdeinstrslen,
                    &ihead,
                    &iarraycount,
                    err);
                if (res == DW_DLV_ERROR) {
                    glflags.gf_count_major_errors++;
                    printf("\nERROR: fail expanding  instructions %"
                        DW_PR_DSd
                        " with fde index %" DW_PR_DUu "!\n",
                        cie_index, fde_index);
                    printf("%s\n",dwarf_errmsg(*err));
                    return res;
                }
                if (res == DW_DLV_NO_ENTRY) {
                    glflags.gf_count_major_errors++;
                        printf("\nERROR: Impossible: no entry "
                        "expanding instructions  for "
                        "cie index %" DW_PR_DUu "?\n", cie_index);
                    return res;
                }

                /* Do not print if in check mode */
                print_frame_inst_bytes(dbg,
                    ihead,
                    iarraycount,
                    *cu_die_for_print_frames,
                    data_alignment_factor,
                    code_alignment_factor,
                    address_size,
                    cie_offset_size,
                    cie_version, config_data);
                dwarf_dealloc_frame_instr_head(ihead);
                ihead = 0;
            }
        }
    }
    return DW_DLV_OK;
}  /* end: print_one_fde */

/*  Print a cie.  Gather the print logic here so the
    control logic deciding what to print
    is clearer.
*/
int
print_one_cie(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Cie cie,
    Dwarf_Unsigned cie_index,
    Dwarf_Half address_size,
    struct dwconf_s *config_data,
    Dwarf_Error *err)
{
    int cires = 0;
    Dwarf_Unsigned cie_length = 0;
    Dwarf_Small version = 0;
    char* augmenter = "";
    Dwarf_Unsigned code_alignment_factor = 0;
    Dwarf_Signed data_alignment_factor = 0;
    Dwarf_Half return_address_register_rule = 0;
    Dwarf_Small *cie_initial_instructions = 0;
    Dwarf_Unsigned cie_initial_instructions_length = 0;
    Dwarf_Off cie_off = 0;
    Dwarf_Half offset_size = 0;

    cires = dwarf_get_cie_info_b(cie,
        &cie_length,
        &version,
        &augmenter,
        &code_alignment_factor,
        &data_alignment_factor,
        &return_address_register_rule,
        &cie_initial_instructions,
        &cie_initial_instructions_length,&offset_size, err);
    if (cires == DW_DLV_ERROR) {
        glflags.gf_count_major_errors++;
        printf("ERROR: calling dwarf_get_cie_info_b() fails\n");
        return cires;
    }
    if (cires == DW_DLV_NO_ENTRY) {
        glflags.gf_count_major_errors++;
        printf("ERROR: Impossible DW_DLV_NO_ENTRY on cie %" DW_PR_DUu
            "\n", cie_index);
        return cires;
    }
    {
        if (glflags.gf_do_print_dwarf) {
            printf("<%5" DW_PR_DUu "> version      %d\n",
                cie_index, version);
            cires = dwarf_cie_section_offset(dbg, cie, &cie_off, err);
            if (cires == DW_DLV_OK) {
                printf("  cie section offset    %" DW_PR_DUu
                    " 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                    (Dwarf_Unsigned) cie_off,
                    (Dwarf_Unsigned) cie_off);
            }
            /* This augmentation is from .debug_frame or
                eh_frame of a cie. . A string. */
            /*      ("  bytes of initial instructions %"*/
            printf("  augmentation                  %s\n",
                sanitized(augmenter));
            printf("  code_alignment_factor         %"
                DW_PR_DUu "\n",
                code_alignment_factor);
            printf("  data_alignment_factor         %"
                DW_PR_DSd "\n",
                data_alignment_factor);
            printf("  return_address_register       %d\n",
                return_address_register_rule);
        }

        {
            int ares = 0;
            Dwarf_Small *data = 0;
            Dwarf_Unsigned len = 0;

            /*  This call only returns  DW_DLV_OK if the augmentation
                is cie aug from .eh_frame.  The
                return is DW_DLV_OK only if there is eh_frame
                style augmentation bytes (its not a string). */
            ares = dwarf_get_cie_augmentation_data(cie, &data,
                &len, err);
            if (ares == DW_DLV_NO_ENTRY) {
                /*  No Aug data (len zero) do nothing. */
            } else if (ares == DW_DLV_OK) {
                /*  We have the gnu eh_frame aug data bytes. */
                if (glflags.gf_do_print_dwarf) {
                    unsigned k2 = 0;

                    /*    ("  bytes of initial instructions  %" */
                    printf("  eh aug data len                0x%"
                        DW_PR_DUx , len);
                    for (k2 = 0; data && k2 < len; ++k2) {
                        if (k2 == 0) {
                            printf(" bytes 0x");
                        }
                        printf("%02x ", (unsigned char) data[k2]);
                    }
                    printf("\n");
                }
            }  else { /* DW_DLV_ERROR */
                glflags.gf_count_major_errors++;
                printf("\nERROR: calling "
                    "dwarf_get_cie_augmentation_data()"
                    " fails.\n");
                printf("%s\n",dwarf_errmsg(*err));
                return ares;
            }
        }

        /* Do not print if in check mode */
        if (glflags.gf_do_print_dwarf) {
            Dwarf_Frame_Instr_Head ihead = 0;
            Dwarf_Unsigned         iarraycount = 0;
            int res = 0;

            printf("  bytes of initial instructions %"
                DW_PR_DUu "\n",
                cie_initial_instructions_length);
            printf("  cie length                    %" DW_PR_DUu
                "\n", cie_length);
            /*  For better layout */
            printf("  initial instructions\n");
            res = dwarf_expand_frame_instructions(
                cie,
                cie_initial_instructions,
                cie_initial_instructions_length,
                &ihead,
                &iarraycount,
                err);
            if (res == DW_DLV_ERROR) {
                glflags.gf_count_major_errors++;
                printf("\nERROR: fail expanding  cie index %"
                    DW_PR_DUu " instructions\n",
                    cie_index);
                printf("%s\n",dwarf_errmsg(*err));
                return res;
            }
            if (res == DW_DLV_NO_ENTRY) {
                glflags.gf_count_major_errors++;
                    printf("\nERROR: Impossible: no entry "
                    "expanding instructions  for "
                    "cie index %"DW_PR_DUu "?\n", cie_index);
                return res;
            }

            print_frame_inst_bytes(dbg,
                ihead,
                iarraycount,
                die,
                data_alignment_factor,
                code_alignment_factor,
                address_size,
                offset_size,version,config_data);
            dwarf_dealloc_frame_instr_head(ihead);
            ihead = 0;
        }
    }
    return DW_DLV_OK;
} /* end: print_one_cie */

int
print_expression_operations(Dwarf_Debug dbg,
    Dwarf_Die die,
    int die_indent_level,
    Dwarf_Small *bytes_in,
    Dwarf_Unsigned block_len,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    Dwarf_Half version,
    struct esb_s *out_string,
    Dwarf_Error *err)
{
    Dwarf_Unsigned ulistlen = 0;
    int res2 = 0;
    /* See PRINTING_DIES macro in print_die.c */

    {
        Dwarf_Loc_Head_c head = 0;
        Dwarf_Locdesc_c locentry = 0;
        int lres = 0;
        Dwarf_Unsigned lopc = 0;
        Dwarf_Unsigned hipc = 0;
        Dwarf_Unsigned ulocentry_count = 0;
        Dwarf_Unsigned section_offset = 0;
        Dwarf_Unsigned locdesc_offset = 0;
        Dwarf_Small lle_value = 0;
        Dwarf_Small loclist_source = 0;
        Dwarf_Addr rawlopc = 0;
        Dwarf_Addr rawhipc = 0;
        Dwarf_Bool debug_addr_unavailable = FALSE;

        res2 = dwarf_loclist_from_expr_c(dbg,
            bytes_in,block_len,
            addr_size,
            offset_size,
            version,
            &head,
            &ulistlen,
            err);
        if (res2 == DW_DLV_NO_ENTRY) {
            return res2;
        }
        if (res2 == DW_DLV_ERROR) {
            glflags.gf_count_major_errors++;
            printf("\nERROR: calling dwarf_loclist_from_expr_c()");
            return res2;
        }
        lres = dwarf_get_locdesc_entry_d(head,
            0, /* Data from 0th LocDesc */
            &lle_value,
            &rawlopc, &rawhipc,
            &debug_addr_unavailable,
            &lopc, &hipc,
            &ulocentry_count,
            &locentry,
            &loclist_source,
            &section_offset,
            &locdesc_offset,
            err);
        if (lres == DW_DLV_ERROR) {
            dwarf_dealloc_loc_head_c(head);
            glflags.gf_count_major_errors++;
            printf("\nERROR: calling dwarf_get_locdesc_entry_d()"
                " on LocDesc 0");
            return lres;
        } else if (lres == DW_DLV_NO_ENTRY) {
            dwarf_dealloc_loc_head_c(head);
            return lres;
        }
        /*  ASSERT: loclist_source == DW_LKIND_expression  */
        /*  ASSERT: lle_value == DW_LLE_start_end  */
        lres = dwarfdump_print_expression_operations(dbg,
            die,
            die_indent_level,
            locentry,
            ulocentry_count,
            out_string,err);
        dwarf_dealloc_loc_head_c(head);
        return lres;
    }
}

/*  DW_CFA_nop may be omitted for alignment,
    so we do not flag that one. */
static int
lastop_pointless(int op)
{
    if (op == DW_CFA_remember_state ||
        op == DW_CFA_MIPS_advance_loc8 ||
        op == DW_CFA_advance_loc ||
        op == DW_CFA_advance_loc4 ||
        op == DW_CFA_advance_loc2 ||
        op == DW_CFA_advance_loc1 ||
        op == DW_CFA_set_loc) {
        return TRUE;
    }
    /* The last op is hopefully useful. */
    return FALSE;
}

static char exprstr_buf[1000];
static int
print_expression( Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Block *expression_block,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    int version)
{
    struct esb_s exprstring;
    Dwarf_Error cerr = 0;
    int gres = 0;

    esb_constructor_fixed(&exprstring,
        exprstr_buf, sizeof(exprstr_buf));
    gres = print_expression_operations(dbg,
        die,
        /* indent */ 1,
        expression_block->bl_data,
        expression_block->bl_len,
        addr_size, offset_size,version,
        &exprstring,&cerr);
    if ( gres == DW_DLV_OK) {
        printf("    %s\n",
        sanitized(esb_get_string(&exprstring)));
    } else if (gres == DW_DLV_NO_ENTRY) {
        glflags.gf_count_major_errors++;
        printf("\nERROR: Unable to get string "
            "from"
            " expression block "
            "of length "
            " %" DW_PR_DUu " bytes.\n",
            expression_block->bl_len);
    } else {
        glflags.gf_count_major_errors++;
        printf("\nERROR: No  string from"
            " expression block "
            "of length "
            " %" DW_PR_DSd " bytes.\n",
            expression_block->bl_len);
        printf("Error: %s\n",dwarf_errmsg(cerr));
        dwarf_dealloc(dbg,cerr,DW_DLA_ERROR);
        cerr = 0;
    }
    esb_destructor(&exprstring);
    return DW_DLV_OK;
}

/*  Print the frame instructions in detail
    for a glob of instructions.
    The frame data has not been checked by
    libdwarf as libdwarf has not
    transformed it into simple structs.
    We are reading the raw data.
*/
/*ARGSUSED*/ static void
print_frame_inst_bytes(Dwarf_Debug dbg,
    Dwarf_Frame_Instr_Head instr_head,
    Dwarf_Unsigned instr_array_len,
    Dwarf_Die die,
    Dwarf_Signed data_alignment_factor,
    Dwarf_Unsigned code_alignment_factor,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    Dwarf_Half version,
    struct dwconf_s *config_data)
{
    Dwarf_Error error = 0;
    Dwarf_Half lastop = 0;
    Dwarf_Unsigned i  = 0;

    if (instr_array_len == 0) {
        return;
    }
    if (!instr_head) {
        /*  Impossible */
        return;
    }
    for ( ; i < instr_array_len; ++i) {
        int res = 0;
        Dwarf_Unsigned  instr_offset_in_instrs = 0;
        Dwarf_Small     cfa_operation = 0;
        const char     *fields= 0;
        Dwarf_Unsigned  u0 = 0;
        Dwarf_Unsigned  u1 = 0;
        Dwarf_Unsigned  u2 = 0;
        Dwarf_Signed    s0 = 0;
        Dwarf_Signed    s1 = 0;
        Dwarf_Block     expression_block;
        const char     *op_name = 0;

        if (!i) {
            printf("  [  ] offset name                 operands\n");
        }
        res = dwarf_get_frame_instruction_a(instr_head,
            i,&instr_offset_in_instrs, &cfa_operation,
            &fields, &u0,&u1,&u2,&s0,&s1,
            0,0, /* These alignment factors passed to us. */
            &expression_block,&error);
        if (res != DW_DLV_OK) {
            if (res == DW_DLV_ERROR) {
                dwarf_dealloc_error(dbg,error);
                error = 0;
            }
            return;
        }
        lastop = cfa_operation;
        res = dwarf_get_CFA_name(cfa_operation,&op_name);
        if (res == DW_DLV_NO_ENTRY) {
            op_name = "InstrOpUnknown";
        }
        printf("  [%2" DW_PR_DUu "]  %2" DW_PR_DUu " %-23s ",i,
            instr_offset_in_instrs,op_name);
        switch(fields[0]) {
        case 'u': {
            if (!fields[1]) {
                printf("%" DW_PR_DUu "\n",u0);
                break;
            }
            if (fields[1] == 'c') {
                Dwarf_Unsigned final =
                    u0*code_alignment_factor;
                printf("%" DW_PR_DUu ,final);
                if (glflags.verbose) {
                    printf("  (%" DW_PR_DUu " * %" DW_PR_DUu
                        ")",
                        u0,code_alignment_factor);

                }
                printf("\n");
                break;
            }
        }
        break;
        case 'r': {
            if (!fields[1]) {
                printreg(u0,config_data);
                printf("\n");
                break;
            }
            if (fields[1] == 'u') {
                if (!fields[2]) {
                    printreg(u0,config_data);
                    printf(" %" DW_PR_DUu ,u1);
                    printf("\n");
                    break;
                }
                if (fields[2] == 'd') {
                    Dwarf_Signed final =
                        (Dwarf_Signed)u1 *
                        data_alignment_factor;
                    printreg(u0,config_data);
                    printf(" %" DW_PR_DSd ,final);
                    if (glflags.verbose) {
                        printf("  (%" DW_PR_DUu " * %" DW_PR_DSd
                            ")",
                            u1,data_alignment_factor);
                    }
                    printf("\n");
                    break;
                }
                if (fields[2] == 'a') {
                    printreg(u0,config_data);
                    printf(" %" DW_PR_DUu ,u1);
                    printf("  (%" DW_PR_DUu ", addrspace  %"
                        DW_PR_DUu ")",
                        u1,u2);
                    printf("\n");
                    break;
                }
            }
            if (fields[1] == 'r') {
                printreg(u0,config_data);
                printf(" ");
                printreg(u1,config_data);
                printf("\n");
                break;
            }
            if (fields[1] == 's') {
                if (fields[2] == 'd') {
                    Dwarf_Signed final = s1 * data_alignment_factor;
                    printreg(u0,config_data);
                    printf(" %" DW_PR_DSd , final);
                    if (glflags.verbose) {
                        printf("  (%" DW_PR_DSd " * %" DW_PR_DSd
                            ")",
                            s1,data_alignment_factor);
                    }
                    if (fields[3] == 'a') {
                        printf(", addrspace  %" DW_PR_DUu, u2);
                    }
                    printf("\n");
                    break;
                }
            }
            if (fields[1] == 'b') {
                /* rb */
                printreg(u0,config_data);
                printf(" expr block len %" DW_PR_DUu "\n",
                    expression_block.bl_len);
                dump_block("    ", expression_block.bl_data,
                    (Dwarf_Signed) expression_block.bl_len);
                printf("\n");
                if (glflags.verbose) {
                    print_expression(dbg,die,&expression_block,
                        addr_size,offset_size,
                        version);
                }
                break;
            }
        }
        break;
        case 's': {
            if (fields[1] == 'd') {
                Dwarf_Signed final = s0*data_alignment_factor;

                printf(" %" DW_PR_DSd ,final);
                if (glflags.verbose) {
                    printf("  (%" DW_PR_DSd " * %" DW_PR_DSd
                        ")",
                        s0,data_alignment_factor);
                }
                printf("\n");
                break;
            }
        }
        break;
        case 'b': {
            if (!fields[1]) {
                printf(" expr block len %" DW_PR_DUu "\n",
                    expression_block.bl_len);
                dump_block("    ", expression_block.bl_data,
                    (Dwarf_Signed) expression_block.bl_len);
                printf("\n");
                if (glflags.verbose) {
                    print_expression(dbg,die,&expression_block,
                        addr_size,offset_size,
                        version);
                }
                break;
            }
        }
        break;
        case 0:
            printf("\n");
        break;
        default:
            printf("UNKNOWN FIELD 0x%x\n",fields[0]);
        }
    }
    if (lastop_pointless(lastop)) {
        printf(
            " Warning: Final FDE operator is useless "
            "but not an error. %s\n",
            get_CFA_name(lastop));
    }
}

/* Print our register names for the cases we have a name.
   Delegate to the configure code to actually do the print.
*/
void
printreg(Dwarf_Unsigned reg, struct dwconf_s *config_data)
{
    print_reg_from_config_data(reg, config_data);
}

/*  Actually does the printing of a rule in the table.
    This may print something or may print nothing!  */
static void
print_one_frame_reg_col(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Unsigned rule_id,
    Dwarf_Small value_type,
    Dwarf_Unsigned reg_used,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    Dwarf_Half version,
    struct dwconf_s *config_data,
    Dwarf_Signed offset_relevant,
    Dwarf_Signed offset,
    Dwarf_Block *block)
{
    char *type_title = "";
    int print_type_title = 1;

    if (!glflags.gf_do_print_dwarf) {
        return;
    }
    if (reg_used == config_data->cf_initial_rule_value &&
        (value_type == DW_EXPR_OFFSET ||
        value_type == DW_EXPR_VAL_OFFSET) ) {
        /*  This is really an empty column. Nothing to do.
            Would be great if we could tell the caller
            the *next* column used here or something. */
        return;
    }
    switch (value_type) {
    case DW_EXPR_OFFSET:
        type_title = "off";
        goto preg2;
    case DW_EXPR_VAL_OFFSET:
        type_title = "valoff";

        preg2:
        if (print_type_title)
            printf("<%s ", type_title);
        printreg(rule_id, config_data);
        printf("=");
        if (offset_relevant == 0) {
            printreg(reg_used, config_data);
            printf(" ");
        } else {
            printf("%02" DW_PR_DSd , offset);
            printf("(");
            printreg(reg_used, config_data);
            printf(") ");
        }
        if (print_type_title)
            printf("> ");
        break;
    case DW_EXPR_EXPRESSION:
        type_title = "expr";
        goto pexp2;
    case DW_EXPR_VAL_EXPRESSION:
        type_title = "valexpr";

        pexp2:
        if (print_type_title) {
            printf("<%s ", type_title);
        }
        if (offset_relevant) {
            printf("ERROR: frame register type %s "
                "marked as offset_relevant "
                "but that is impossible, it has no such "
                "field.",type_title);
            glflags.gf_count_major_errors++;
        }
        printreg(rule_id, config_data);
        printf("=");
        printf("expr-block-len=%" DW_PR_DUu , block->bl_len);
        if (print_type_title) {
            printf("> ");
        }
        if (glflags.verbose) {
            printf("<");
            printf("%s",type_title);
            printf("bytes:");
            /*  The data being dumped comes direct from
                libdwarf so libdwarf validated it. */
            dump_block("", block->bl_data, block->bl_len);
            printf("> ");
            {
                struct esb_s exprstring;
                char local_buf[300];
                int gres = 0;
                Dwarf_Error cerr = 0;

                esb_constructor_fixed(&exprstring,local_buf,
                    sizeof(local_buf));
                gres = print_expression_operations(dbg,
                    die,
                    /* indent */ 1,
                    block->bl_data, block->bl_len,
                    addr_size,
                    offset_size,version,
                    &exprstring,&cerr);
                if ( gres == DW_DLV_OK) {
                    printf("<expr:%s>",sanitized(
                        esb_get_string(&exprstring)));
                } else if (gres == DW_DLV_NO_ENTRY) {
                    glflags.gf_count_major_errors++;
                    printf("\nERROR: Unable to get string from"
                        " DW_EXPR_VAL_EXPRESSION block of length "
                        " %" DW_PR_DSd " bytes.\n" ,offset);
                } else {
                    glflags.gf_count_major_errors++;
                    printf("\nERROR: No  string from"
                        " DW_EXPR_VAL_EXPRESSION block of length "
                        " %" DW_PR_DSd " bytes.\n" ,offset);
                    printf("Error: %s\n",dwarf_errmsg(cerr));
                    dwarf_dealloc_error(dbg,cerr);
                    cerr = 0;
                }
                esb_destructor(&exprstring);
            }
        }
        break;
    default:
        printf("Internal error in libdwarf, value type %d\n",
            value_type);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
dd_check_get_fde_n(Dwarf_Debug dbg,
    Dwarf_Fde *fde_data,
    Dwarf_Fde expected_fde,
    Dwarf_Signed index)
{
    int         res = 0;
    Dwarf_Error localerr = 0;
    Dwarf_Fde   ret_fde = 0;

    DWARF_CHECK_COUNT(check_functions_result,1);
    res = dwarf_get_fde_n(fde_data,
        (Dwarf_Unsigned)index,&ret_fde,&localerr);
    if (res == DW_DLV_ERROR) {
        DWARF_CHECK_ERROR2(check_functions_result,
            "dwarf_get_cie_index() returned DW_DLV_ERRORi "
            "underlying error is ",
            dwarf_errmsg(localerr));
        DROP_ERROR_INSTANCE(dbg,res,localerr);
    } else if (res == DW_DLV_NO_ENTRY) {
        DWARF_CHECK_ERROR(check_functions_result,
            "dwarf_get_cie_index() returned DW_DLV_NO_ENTRY ");
    } else {
        if (ret_fde != expected_fde) {
            DWARF_CHECK_ERROR(check_functions_result,
                "dwarf_get_cie_index() does not"
                " return the expected fde!");
        }
    }
}

/*  We do NOT want to free cie/fde data as we will
    use that in print_all_cies() */
static int
print_all_fdes(Dwarf_Debug dbg,
    const char *frame_section_name,
    Dwarf_Fde *fde_data,
    Dwarf_Signed fde_element_count,
    Dwarf_Cie *cie_data,
    Dwarf_Signed cie_element_count,
    Dwarf_Half address_size,
    Dwarf_Half offset_size,
    Dwarf_Half version,
    int is_eh,
    struct dwconf_s *config_data,
    void **map_lowpc_to_name,
    void **lowpcSet,
    Dwarf_Die *cu_die_for_print_frames,
    Dwarf_Error*err)
{
    Dwarf_Signed i = 0;
    int frame_count = 0;

    /* Do not print if in check mode */
    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];
        const char *stdsecname = 0;

        if (!is_eh) {
            stdsecname=".debug_frame";
        } else {
            stdsecname=".eh_frame";
        }
        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,stdsecname,
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
        printf("\nfde:\n");
    }

    for (i = 0; i < fde_element_count; i++) {
        int fdres = 0;
        Dwarf_Fde fde = 0;

        fde = fde_data[i];
        if (glflags.gf_check_functions) {
            dd_check_get_fde_n(dbg,fde_data,
                fde,i);
        }
        fdres = print_one_fde(dbg,
            frame_section_name,
            fde_data,i,
            cie_data, cie_element_count,
            address_size, offset_size, version,
            is_eh, config_data,
            map_lowpc_to_name,
            lowpcSet,
            cu_die_for_print_frames,
            err);
        if (fdres == DW_DLV_ERROR) {
            glflags.gf_count_major_errors++;
            printf("ERROR: Printing fde %" DW_PR_DSd
                " fails. Error %s\n",
                i,dwarf_errmsg(*err));
            return fdres;
        }
        if (fdres == DW_DLV_NO_ENTRY) {
            glflags.gf_count_major_errors++;
            printf("ERROR: Printing fde %" DW_PR_DSd
                " fails saying 'no entry'. Impossible.\n",
                i);
            return fdres;
        }
        ++frame_count;
        if (frame_count >= glflags.break_after_n_units) {
            break;
        }
    }
    return DW_DLV_OK;
}

static int
print_all_cies(Dwarf_Debug dbg,
    Dwarf_Cie *cie_data,
    Dwarf_Signed cie_element_count,
    Dwarf_Half address_size,
    struct dwconf_s *config_data,
    Dwarf_Die *cu_die_for_print_frames,
    Dwarf_Error*err)
{
    /* Print the cie set. */
    /* Do not print if in check mode */
    Dwarf_Signed i = 0;
    Dwarf_Signed cie_count = 0;

    if (glflags.gf_do_print_dwarf) {
        printf("\ncie:\n");
    }
    for (i = 0; i < cie_element_count; i++) {
        int cres = 0;

        cres = print_one_cie(dbg,
            *cu_die_for_print_frames,
            cie_data[i], i, address_size,
            config_data,
            err);
        if (cres == DW_DLV_ERROR) {
            glflags.gf_count_major_errors++;
            printf("\nERROR: Printing cie %" DW_PR_DSd
                " fails. Error %s\n",
                i,dwarf_errmsg(*err));
            return cres;
        } else if (cres == DW_DLV_NO_ENTRY) {
            glflags.gf_count_major_errors++;
            printf("\nERROR: Printing cie %" DW_PR_DSd
                " fails. saying NO_ENTRY!\n",
                i);
            return cres;
        } else {
            ++cie_count;
            if (cie_count >= glflags.break_after_n_units) {
                break;
            }
        }
    }
    return DW_DLV_OK;
}
/*  get all the data in .debug_frame (or .eh_frame).
    The '3' versions mean print using the dwarf3 new interfaces.
    The non-3 mean use the old interfaces.
    All combinations of requests are possible.  */
int
print_frames(Dwarf_Debug dbg,
    int want_eh,
    struct dwconf_s *config_data,
    /*  Pass these next 3 so preserved from .eh_frame
        to .debug_frame */
    Dwarf_Die * cu_die_for_print_frames,
    void ** map_lowpc_to_name,
    void ** lowpcSet,
    Dwarf_Error *err)
{
    int fres = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Half offset_size = 0;
    Dwarf_Half version = 0;

    /*  We only get here if a flag says we want to
        process the section (want_eh is either 0 or 1). */
    glflags.current_section_id = DEBUG_FRAME;
    /*  Only in DWARF4 or later is there a real address
        size known in the frame data itself.  If any DIE
        is known then a real address size can be gotten from
        dwarf_get_die_address_size(). */
    fres = dwarf_get_address_size(dbg, &address_size, err);
    if (fres != DW_DLV_OK) {
        glflags.gf_count_major_errors++;
        printf("ERROR: Unable to print frame section as "
            " we cannot get the address size\n");
        return fres;
    }
    {
        Dwarf_Cie *cie_data = NULL;
        Dwarf_Signed cie_element_count = 0;
        Dwarf_Fde *fde_data = NULL;
        Dwarf_Signed fde_element_count = 0;
        const char *frame_section_name = 0;
        int silent_if_missing = 0;
        int is_eh = 0;

        if (!want_eh) {
            Dwarf_Error localerr = 0;
            glflags.current_section_id = DEBUG_FRAME;
            /*  Do not free frame_section_name. */
            fres = dwarf_get_frame_section_name(dbg,
                &frame_section_name,&localerr);
            if (fres != DW_DLV_OK || !frame_section_name ||
                !strlen(frame_section_name)) {
                frame_section_name = ".debug_frame";
                if (fres == DW_DLV_ERROR) {
                    dwarf_dealloc(dbg,localerr,DW_DLA_ERROR);
                    localerr = 0;
                }
            }

            /*  Big question here is how to print all the info?
                Can print the logical matrix, but that is huge,
                though could skip lines that don't change.
                Either that, or print the instruction statement
                program that describes the changes.  */
            fres = dwarf_get_fde_list(dbg, &cie_data,
                &cie_element_count,
                &fde_data, &fde_element_count, err);
            if (glflags.gf_check_harmless) {
                print_any_harmless_errors(dbg);
            }
        } else {
            /* want_eh */
            Dwarf_Error localerr = 0;
            glflags.current_section_id = DEBUG_FRAME_EH_GNU;
            is_eh = 1;
            /*  This is gnu g++ exceptions in a .eh_frame section.
                Which is just like .debug_frame except that
                the empty, or 'special' CIE_id is 0, not -1
                (to distinguish fde from cie).
                And the augmentation is "eh". As of egcs-1.1.2
                anyway. A non-zero cie_id is in a fde and is the
                difference between the fde address and the
                beginning of the cie it belongs to.
                This makes sense as this is
                intended to be referenced at run time, and
                is part of the running image. For more on
                augmentation strings,
                see libdwarf/dwarf_frame.c.  */
            /*  Big question here is how to print all the info?
                Can print the logical matrix, but that is huge,
                though could skip lines that don't change.
                Either that, or print the instruction statement
                program that describes the changes.  */
            silent_if_missing = 1;
            /*  Do not free frame_section_name. */
            fres = dwarf_get_frame_section_name_eh_gnu(dbg,
                &frame_section_name,&localerr);
            if (fres != DW_DLV_OK || !frame_section_name ||
                !strlen(frame_section_name)) {
                frame_section_name = ".eh_frame";
                if (fres == DW_DLV_ERROR){
                    dwarf_dealloc(dbg,localerr,DW_DLA_ERROR);
                    localerr = 0;
                }
            }
            fres = dwarf_get_fde_list_eh(dbg, &cie_data,
                &cie_element_count, &fde_data,
                &fde_element_count, err);
            if (glflags.gf_check_harmless) {
                print_any_harmless_errors(dbg);
            }
        }
        if (fres == DW_DLV_ERROR) {
            const char *loc = "dwarf_get_fde_list";
            if (is_eh) {
                loc = "dwarf_get_fde_list_eh";
            }
            glflags.gf_count_major_errors++;
            printf("\nERROR: %s not loadable. %s %s\n",
                sanitized(frame_section_name),loc,
                dwarf_errmsg(*err));
            return fres;
        }

        /* Do not print any frame info if in check mode */
        if (glflags.gf_check_frames) {
            if (fres == DW_DLV_OK) {
                dwarf_dealloc_fde_cie_list(dbg, cie_data,
                    cie_element_count,
                    fde_data, fde_element_count);
            }
            return DW_DLV_OK;
        }

        if (fres == DW_DLV_NO_ENTRY) {
            if (!silent_if_missing) {
                printf("\n%s is not present\n",
                    sanitized(frame_section_name));
            }
            /* no frame information */
            return fres;
        } else {                /* DW_DLV_OK */
            int res = 0;

            res = print_all_fdes(dbg,frame_section_name,fde_data,
                fde_element_count,
                cie_data, cie_element_count,
                address_size,offset_size,version,is_eh,
                config_data,map_lowpc_to_name,
                lowpcSet,
                cu_die_for_print_frames,
                err);
            if (res == DW_DLV_ERROR) {
                glflags.gf_count_major_errors++;
                printf("ERROR: printing fdes fails. %s "
                    " Attempting to continue. \n",
                    dwarf_errmsg(*err));
                dwarf_dealloc_error(dbg,*err);
                *err = 0;
            }
            res = print_all_cies(dbg,
                /* frame_section_name, */
                cie_data, cie_element_count,
                address_size,
                /* offset_size,version,is_eh, */
                config_data,
                /* map_lowpc_to_name, lowpcSet, */
                cu_die_for_print_frames,
                err);
            if (res == DW_DLV_ERROR) {
                glflags.gf_count_major_errors++;
                printf("ERROR: printing cies fails. %s "
                    " Attempting to continue. \n",
                    dwarf_errmsg(*err));
                dwarf_dealloc_error(dbg,*err);
                *err = 0;
            }
            /*  Here we do the free. Not earlier. */
            dwarf_dealloc_fde_cie_list(dbg, cie_data,
                cie_element_count,
                fde_data, fde_element_count);
        } /*  End inner scope, not a loop */
    } /*  End inner scope, not a loop */
    return DW_DLV_OK;
}
