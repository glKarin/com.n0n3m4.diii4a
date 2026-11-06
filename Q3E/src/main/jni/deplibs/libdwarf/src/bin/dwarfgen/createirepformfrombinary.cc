/*
  Copyright (C) 2010-2018 David Anderson.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following
  conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following
    disclaimer in the
    documentation and/or other materials provided
    with the distribution.
  * Neither the name of the example nor the
    names of its contributors may be used to endorse
    or promote products
    derived from this software without specific
    prior written permission.

  THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS''
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL David Anderson BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.

*/
// createirepformfrombinary.cc
// Reads an object and inserts its dwarf data into
// an object intended to hold all the dwarf data.

#include "config.h"

/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */

#ifdef HAVE_UNITSTD_H
#include <unistd.h>
#endif
#include <stdlib.h> /* for exit() */
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <string.h> // For memset etc
#include "strtabdata.h"
#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarfp.h"
#include "libdwarf_private.h"
#include "irepresentation.h"
#include "createirepfrombinary.h"

// Microsoft preprocessor definition that wreaks havoc
#ifdef interface
#undef interface
#endif

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::list;

// THis should instantiated only locally, not with 'new'.
// It should not be copied.
class IRFormInterface {
public:
    IRFormInterface(Dwarf_Debug dbg,
        Dwarf_Attribute attr,IRCUdata &cudata,IRAttr & irattr):
        dbg_(dbg),attr_(attr),cudata_(cudata),irattr_(irattr) {};
    // Do not free anything we did not create here.
    ~IRFormInterface() {};
    Dwarf_Debug dbg_;
    Dwarf_Attribute attr_;
    IRCUdata &cudata_;
    IRAttr &irattr_;
private:

    // Do not instantiate.
    IRFormInterface();
    IRFormInterface& operator= (const IRFormInterface &ir);

};

IRForm *formFactory(Dwarf_Debug dbg,
    Dwarf_Attribute attr,IRCUdata &cudata,IRAttr & irattr)
{
    IRFormInterface interface(dbg,attr,cudata,irattr);
    enum Dwarf_Form_Class cl = irattr.getFormClass();

    switch(cl) {
    case DW_FORM_CLASS_UNKNOWN:
        break;
    case DW_FORM_CLASS_ADDRESS:
        {
            IRFormAddress *f = new IRFormAddress(&interface);
            // FIXME: do we need to do anything about symbol here?
            // Yes, if it has a relocation.
            return f;
        }
        break;
    case DW_FORM_CLASS_EXPRLOC:
    case DW_FORM_CLASS_BLOCK:
        {
            IRFormBlock *f = new IRFormBlock(&interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_CONSTANT:
        {
            IRFormConstant *f = new IRFormConstant(&interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_FLAG:
        {
            IRFormFlag *f = new IRFormFlag(&interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_LINEPTR:
        {
            IRFormLinePtr *f = new IRFormLinePtr(&interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_LOCLISTPTR:
        {
            IRFormLoclistPtr *f = new IRFormLoclistPtr(&interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_MACPTR:
        {
            IRFormMacPtr *f = new IRFormMacPtr(&interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_RANGELISTPTR:
        {
            IRFormRangelistPtr *f = new IRFormRangelistPtr(
                &interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_REFERENCE:
        {
            IRFormReference *f  = new IRFormReference(&interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_STRING:
        {
            IRFormString *f = new IRFormString(&interface);
            return f;
        }
        break;
    case DW_FORM_CLASS_FRAMEPTR: /* SGI/IRIX extension. */
        {
            IRFormFramePtr *f = new IRFormFramePtr(&interface);
            return f;
        }
        break;
    default:
        break;
    }
    return new IRFormUnknown();
}

static void
extractInterafaceForms(IRFormInterface *interface,
    Dwarf_Half *finalform,
    Dwarf_Half *initialform)
{
    Dwarf_Error error = 0;
    int res = dwarf_whatform(interface->attr_,finalform,&error);
    if (res != DW_DLV_OK) {
        cerr << "Unable to get attr form " << endl;
        exit(1);
    }

    res = dwarf_whatform_direct(interface->attr_,initialform,&error);
    if (res != DW_DLV_OK) {
        cerr << "Unable to get attr direct form " << endl;
        exit(1);
    }
}

IRFormAddress::IRFormAddress(IRFormInterface * interface)
{
    Dwarf_Addr val = 0;
    Dwarf_Error error = 0;
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_ = DW_FORM_CLASS_ADDRESS;
    address_ = 0;

    int res = dwarf_formaddr(interface->attr_,&val, &error);
    if (res != DW_DLV_OK) {
        cerr << "Unable to read flag value. Impossible error.\n"
            << " line " << __LINE__ << " file " <<
            __FILE__ <<  endl;
        exit(1);
    }
    // FIXME do we need to do anything about the symbol here?
    // It might have a relocation and we should determine that.
    address_ = val;
}
IRFormBlock::IRFormBlock(IRFormInterface * interface)
{
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_BLOCK;
    fromloclist_= 0;
    sectionoffset_=0;

    Dwarf_Block *blockptr = 0;
    Dwarf_Error error = 0;
    int res = dwarf_formblock(interface->attr_,&blockptr, &error);
    if (res != DW_DLV_OK) {
        cerr << "Unable to read block . Impossible error.\n"
            << " line " << __LINE__ << " file " <<
            __FILE__ <<  endl;
        exit(1);
    }
    insertBlock(blockptr);
    dwarf_dealloc(interface->dbg_,blockptr,DW_DLA_BLOCK);
}
IRFormConstant::IRFormConstant(IRFormInterface * interface)
{
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_CONSTANT;
    signedness_=SIGN_NOT_SET;
    uval_=0;
    sval_=0;
    Dwarf_Error error = 0;

    if (finalform == DW_FORM_data16) {
        Dwarf_Form_Data16 data16;
        int rd16 = dwarf_formdata16(interface->attr_,&data16,
            &error);
        if (rd16 != DW_DLV_OK) {
            cerr << "Unable to read constant data16  value. "
                "Impossible error.\n" << endl;
            exit(1);
        }
        setValues16(&data16, SIGN_UNKNOWN);
        return;
    }
    enum Signedness oursign = SIGN_NOT_SET;
    Dwarf_Unsigned uval = 0;
    Dwarf_Signed sval = 0;
    int ress = dwarf_formsdata(interface->attr_, &sval,&error);
    if (ress == DW_DLV_ERROR) {
        Dwarf_Debug dbg = interface->dbg_;
        dwarf_dealloc_error(dbg,error);
        error = 0;
        ress = DW_DLV_NO_ENTRY;
    }
    int resu = dwarf_formudata(interface->attr_, &uval,&error);
    if (resu == DW_DLV_ERROR) {
        Dwarf_Debug dbg = interface->dbg_;
        dwarf_dealloc_error(dbg,error);
        error = 0;
        resu = DW_DLV_NO_ENTRY;
    }
    if (resu == DW_DLV_OK ) {
        if (ress == DW_DLV_OK) {
            if (finalform == DW_FORM_sdata) {
                /* sval set */
                oursign = SIGNED;
            } else {
                oursign = SIGN_UNKNOWN;
            }
        } else {
            oursign = UNSIGNED;
            sval = static_cast<Dwarf_Signed>(uval);
        }
    } else {
        if (ress == DW_DLV_OK) {
            oursign = SIGNED;
            uval = static_cast<Dwarf_Unsigned>(sval);
        } else {
            cerr << "Unable to read constant value. "
                "Impossible error."
                << endl;
            exit(1);
        }
    }
    setValues(sval, uval, oursign);
}
IRFormExprloc::IRFormExprloc(IRFormInterface * interface)
{
    Dwarf_Unsigned len = 0;
    Dwarf_Ptr data = 0;
    Dwarf_Error error = 0;
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_EXPRLOC;

    int res = dwarf_formexprloc(interface->attr_,&len, &data, &error);
    if (res != DW_DLV_OK) {
        cerr << "Unable to read flag value. Impossible error.\n"
            << " line " << __LINE__ << " file " <<
            __FILE__ <<  endl;
        exit(1);
    }
    // FIXME: Would be nice to expand to the expression details
    // instead of just a block of bytes.
    insertBlock(len,data);
}
IRFormFlag::IRFormFlag(IRFormInterface * interface)
{
    Dwarf_Bool flagval = 0;
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_FLAG;
    flagval_=0;

    Dwarf_Error error = 0;
    int res = dwarf_formflag(interface->attr_,&flagval, &error);
    if (res != DW_DLV_OK) {
        cerr << "Unable to read flag value. Impossible error.\n"
            << " line " << __LINE__ << " file " <<
            __FILE__ <<  endl;
        exit(1);
    }
    setFlagVal(flagval);
    setFinalForm(finalform);
    setInitialForm(initialform);

}

// We are simply assuming (here) that the value is a global offset
// to some section or other.
// Calling code ensures that is true.
//
static Dwarf_Unsigned
get_section_offset(IRFormInterface * interface)
{
    Dwarf_Unsigned uval = 0;
    Dwarf_Signed sval = 0;
    Dwarf_Error error = 0;
    Dwarf_Off sectionoffset = 0;
    int resu = 0;
    int ress = 0;

    // The following allows more sorts of value than
    // we really want to allow here, but that is
    // harmless, we believe.
    int resgf = dwarf_global_formref(interface->attr_,
        &sectionoffset, &error);
    if (resgf == DW_DLV_OK ) {
        return sectionoffset;
    }
    resu = dwarf_formudata(interface->attr_, &uval,&error);
    if (resu != DW_DLV_OK ) {
        if (resu == DW_DLV_ERROR){
            Dwarf_Debug dbg = interface->dbg_;
            dwarf_dealloc_error(dbg,error);
            error = 0;
            resu = DW_DLV_NO_ENTRY;
        }
        ress = dwarf_formsdata(interface->attr_, &sval,&error);
        if (ress == DW_DLV_OK) {
            uval = static_cast<Dwarf_Unsigned>(sval);
        } else {
            if (ress == DW_DLV_ERROR) {
                Dwarf_Debug dbg = interface->dbg_;
                dwarf_dealloc_error(dbg,error);
                error = 0;
                ress = DW_DLV_NO_ENTRY;
            }
            cerr << "Unable to read constant offset value. "
                "Impossible error.\n"
                << endl;
            exit(1);
        }
    }
    return uval;
}
IRFormLoclistPtr::IRFormLoclistPtr(IRFormInterface * interface)
{
    Dwarf_Unsigned uval = get_section_offset(interface);
    setOffset(uval);
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_LOCLISTPTR;
    loclist_offset_=0;
}

// When emitting line data from the producer, this
// attribute is automatically emitted by the transform_to_disk
// function if there is line data in the generated CU,
// not created by interpreting this input attribute.
IRFormLinePtr::IRFormLinePtr(IRFormInterface * interface)
{
    Dwarf_Unsigned uval = get_section_offset(interface);
    setOffset(uval);
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_LINEPTR;

}
IRFormMacPtr::IRFormMacPtr(IRFormInterface * interface)
{
    Dwarf_Unsigned uval = get_section_offset(interface);
    setOffset(uval);
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_MACPTR;
    macro_offset_=0;

}
IRFormRangelistPtr::IRFormRangelistPtr(IRFormInterface * interface)
{
    Dwarf_Unsigned uval = get_section_offset(interface);
    setOffset(uval);
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_RANGELISTPTR;
    rangelist_offset_=0;

}
IRFormFramePtr::IRFormFramePtr(IRFormInterface * interface)
{
    Dwarf_Unsigned uval = get_section_offset(interface);
    setOffset(uval);
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_FRAMEPTR;
    frame_offset_=0;

}

// This is a .debug_info to .debug_info (or .debug_types
// to .debug_types) reference.

// Since local/global reference target values are not known
// till the DIEs are actually emitted,
// we eventually use libdwarf  (dwarf_get_die_markers) to fix up
// the new target value.  Here we just record the input value.
//
IRFormReference::IRFormReference(IRFormInterface * interface)
{
    Dwarf_Off val = 0;
    Dwarf_Error error = 0;
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    formclass_ = DW_FORM_CLASS_REFERENCE;
    reftype_ = RT_NONE;
    globalOffset_ = 0;
    cuRelativeOffset_ = 0;
    targetInputDie_= 0;
    target_die_=  0;
    initSig8();
    Dwarf_Bool is_info = TRUE;

    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    IRCUdata &cudata = interface->cudata_;

    if (finalform == DW_FORM_ref_sig8) {
        Dwarf_Sig8 val8;
        int res = dwarf_formsig8(interface->attr_,&val8, &error);
        if (res != DW_DLV_OK) {
            cerr << "Unable to read sig8 reference. "
                "Impossible error.\n"
                << endl;
            exit(1);
        }
        setSignature(&val8);
        return;
    }
    if (finalform == DW_FORM_ref_addr ||
        finalform == DW_FORM_data4 ||
        finalform == DW_FORM_data8) {
        // FIXME there might be a relocation record.
        int res = dwarf_global_formref(interface->attr_,&val,
            &error);
        if (res != DW_DLV_OK) {
            cerr << "Unable to read reference. "
                "Impossible error.\n" << endl;
            exit(1);
        }
        setOffset(val);
        return;
    }
    // Otherwise it is (if a correct FORM for a
    // .debug_info reference)
    // a local CU offset, and we record it as such..
    int res = dwarf_formref(interface->attr_,&val,&is_info,&error);
    if (res != DW_DLV_OK) {
        cerr << "Unable to read reference.. "
            "Impossible error. finalform " <<
            finalform << endl;
        exit(1);
    }
    setCUOffset(val);
    cudata.insertLocalReferenceAttrTargetRef(val,this);
    IRDie *targdie = cudata.getLocalDie(val);
    if (targdie) {
        // Record local offset's IRDie when it is already known.
        setTargetInDie(targdie);
    }
}

// Global static data used to initialized a sig8 reliably.
static Dwarf_Sig8  zero_sig8;
// Initialization helper function.
void
IRFormReference::initSig8()
{
    typeSig8_ = zero_sig8;
}

IRFormString::IRFormString(IRFormInterface * interface)
{
    char *str = 0;
    Dwarf_Error error = 0;
    Dwarf_Half finalform = 0;
    Dwarf_Half initialform = 0;
    extractInterafaceForms(interface,&finalform,&initialform);
    setFinalForm(finalform);
    setInitialForm(initialform);
    formclass_=DW_FORM_CLASS_STRING;
    strpoffset_=0;

    int res = dwarf_formstring(interface->attr_,&str, &error);
    if (res != DW_DLV_OK) {
        cerr << "Unable to read string. Impossible error.\n" << endl;
        exit(1);
    }
    setString(str);
}
