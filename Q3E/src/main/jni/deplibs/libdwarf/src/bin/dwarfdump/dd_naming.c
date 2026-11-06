/*
Copyright (C) 2000-2005 Silicon Graphics, Inc. All Rights Reserved.
Portions Copyright (C) 2007-2012 David Anderson. All Rights Reserved.
Portions Copyright (C) 2010-2012 SN Systems Ltd. All Rights Reserved.

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

/* SGI has moved from the Crittenden Lane address.  */

#include <config.h>
#include <stdio.h> /* FILE decl for dd_esb.h */

#include "dwarf.h"
#include "libdwarf.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_makename.h"
#include "dd_naming.h"
#include "dd_esb.h"

#ifndef TRIVIAL_NAMING
static const char *
skipunder(const char *v)
{
    const char *cp = v;
    int undercount = 0;
    for (; *cp ; ++cp) {
        if (*cp == '_') {
            ++undercount;
            if (undercount == 2) {
                return cp+1;
            }
        }
    }
    return "";
}
#endif /*  TRIVIAL_NAMING */

static const char *
ellipname(int   res,
    int         val_in,
    const char *v,
    const char *ty)
{
#ifndef TRIVIAL_NAMING
    if (glflags.gf_check_dwarf_constants &&
        checking_this_compiler()) {
        DWARF_CHECK_COUNT(dwarf_constants_result,1);
    }
#endif /* TRIVIAL_NAMING */
    if (res != DW_DLV_OK) {
        char buf[100];
        char *n;
        struct esb_s eb;

        esb_constructor_fixed(&eb,buf,sizeof(buf));
        esb_append_printf_s(&eb,
            "<Unknown %s",ty);
        esb_append_printf_u(&eb,
            " value 0x%x>",val_in);
        /* Capture any name error in DWARF constants */
#ifndef TRIVIAL_NAMING
        if (glflags.gf_check_dwarf_constants &&
            checking_this_compiler()) {
            if (glflags.gf_check_verbose_mode) {
                printf("ERROR %s of %d (0x%x) is unknown "
                    "to dwarfdump. "
                    "Continuing. \n",ty,val_in,val_in );
                glflags.gf_count_major_errors++;
            }
            DWARF_ERROR_COUNT(dwarf_constants_result,1);
            DWARF_CHECK_ERROR_PRINT_CU();
        }
#else /* TRIVIAL_NAMING */
        /* This is for the tree-generation, not dwarfdump itself. */
        printf("ERROR %s of %d (0x%x) is unknown to dwarfdump. "
            "Continuing. \n",ty,val_in,val_in );
        glflags.gf_count_major_errors++;
#endif /* TRIVIAL_NAMING */
        n = makename(esb_get_string(&eb));
        if (!n) {
            printf("Out of memory extracting ellipsis name\n");
            esb_destructor(&eb);
            return "";
        }
        esb_destructor(&eb);
        return n;
    }
#ifndef TRIVIAL_NAMING
    if (glflags.ellipsis) {
        return skipunder(v);
    }
#endif
    return v;
}

const char * get_TAG_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_TAG_name(val_in,&v);
    return ellipname(res,val_in,v,"TAG");
}
const char * get_children_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_children_name(val_in,&v);
    return ellipname(res,val_in,v,"children");
}
const char * get_FORM_CLASS_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_FORM_CLASS_name(val_in,&v);
    return ellipname(res,val_in,v,"FORM_CLASS");
}
const char * get_FORM_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_FORM_name(val_in,&v);
    return ellipname(res,val_in,v,"FORM");
}
const char * get_AT_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_AT_name(val_in,&v);
    return ellipname(res,val_in,v,"AT");
}
const char * get_OP_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_OP_name(val_in,&v);
    return ellipname(res,val_in,v,"OP");
}
const char * get_ATE_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_ATE_name(val_in,&v);
    return ellipname(res,val_in,v,"ATE");
}
const char * get_DS_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_DS_name(val_in,&v);
    return ellipname(res,val_in,v,"DS");
}
const char * get_END_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_END_name(val_in,&v);
    return ellipname(res,val_in,v,"END");
}
const char * get_ATCF_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_ATCF_name(val_in,&v);
    return ellipname(res,val_in,v,"ATCF");
}
const char * get_ACCESS_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_ACCESS_name(val_in,&v);
    return ellipname(res,val_in,v,"ACCESS");
}
const char * get_VIS_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_VIS_name(val_in,&v);
    return ellipname(res,val_in,v,"VIS");
}
const char * get_VIRTUALITY_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_VIRTUALITY_name(val_in,&v);
    return ellipname(res,val_in,v,"VIRTUALITY");
}
const char * get_LANG_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_LANG_name(val_in,&v);
    return ellipname(res,val_in,v,"LANG");
}
const char * get_LNAME_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_LNAME_name(val_in,&v);
    return ellipname(res,val_in,v,"LNAME");
}
const char * get_ID_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_ID_name(val_in,&v);
    return ellipname(res,val_in,v,"ID");
}
const char * get_CC_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_CC_name(val_in,&v);
    return ellipname(res,val_in,v,"CC");
}
const char * get_INL_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_INL_name(val_in,&v);
    return ellipname(res,val_in,v,"INL");
}
const char * get_ORD_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_ORD_name(val_in,&v);
    return ellipname(res,val_in,v,"ORD");
}
const char * get_DSC_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_DSC_name(val_in,&v);
    return ellipname(res,val_in,v,"DSC");
}
const char * get_LNS_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_LNS_name(val_in,&v);
    return ellipname(res,val_in,v,"LNS");
}
const char * get_LNE_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_LNE_name(val_in,&v);
    return ellipname(res,val_in,v,"LNE");
}
const char * get_MACINFO_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_MACINFO_name(val_in,&v);
    return ellipname(res,val_in,v,"MACINFO");
}
const char * get_MACRO_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_MACRO_name(val_in,&v);
    return ellipname(res,val_in,v,"MACRO");
}
const char * get_CFA_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_CFA_name(val_in,&v);
    return ellipname(res,val_in,v,"CFA");
}
const char * get_EH_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_EH_name(val_in,&v);
    return ellipname(res,val_in,v,"EH");
}
const char * get_FRAME_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_FRAME_name(val_in,&v);
    return ellipname(res,val_in,v,"FRAME");
}
const char * get_CHILDREN_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_CHILDREN_name(val_in,&v);
    return ellipname(res,val_in,v,"CHILDREN");
}
const char * get_ADDR_name(unsigned int val_in)
{
    const char *v = 0;
    int res = dwarf_get_ADDR_name(val_in,&v);
    return ellipname(res,val_in,v,"ADDR");
}
