/*
Copyright (C) 2010-2018 David Anderson.  All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

*   Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

* Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
NO EVENT SHALL David Anderson BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

//
// irepform.h
//
//
class IRCUdata;
class IRDie;
class IRAttr;
class IRFormInterface;

// An Abstract class.
class IRForm {
public:
    //virtual void emitvalue() = 0;
    //IRForm & operator=(const IRForm &r);
    virtual IRForm * clone() const  =0;
    virtual ~IRForm() {};
    IRForm() {};
    virtual enum Dwarf_Form_Class getFormClass() const = 0;
private:
};

class IRFormUnknown : public IRForm {
public:
    IRFormUnknown():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_UNKNOWN)
        {}
    ~IRFormUnknown() {};
    IRFormUnknown(IRFormInterface *);
    IRFormUnknown(const IRFormUnknown &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
    }
    virtual IRFormUnknown * clone() const {
        return new IRFormUnknown(*this);
    }
    IRFormUnknown & operator=(const IRFormUnknown &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        return *this;
    };
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
};
// An address class entry refers to some part
// (normally a loadable) section of the object file.
// Not to DWARF info. Typically into .text or .data for example.
// We therefore want a section number and offset
// (generally useless for us)
// or preferably  an elf symbol as that has a value
// and an elf section number.
// We often/usually know neither here so we do not even try.
// Later we will make one up if we have to.
class IRFormAddress : public IRForm {
public:
    IRFormAddress():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_ADDRESS),
        address_(0)
        {};
    IRFormAddress(IRFormInterface *);
    ~IRFormAddress() {};
    IRFormAddress & operator=(const IRFormAddress &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        address_ = r.address_;
        return *this;
    };
    IRFormAddress(const IRFormAddress &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        address_ = r.address_;
    }
    virtual IRFormAddress * clone() const {
        return new IRFormAddress(*this);
    };
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    Dwarf_Addr  getAddress() { return address_;};
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
private:
    void setAddress(Dwarf_Addr addr) { address_ = addr; };
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    Dwarf_Addr address_;
};
class IRFormBlock : public IRForm {
public:
    IRFormBlock():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_BLOCK),
        fromloclist_(0),sectionoffset_(0)
        {}
    IRFormBlock(IRFormInterface *);
    ~IRFormBlock() {};
    IRFormBlock & operator=(const IRFormBlock &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        blockdata_ = r.blockdata_;
        fromloclist_ = r.fromloclist_;
        sectionoffset_ = r.sectionoffset_;
        return *this;
    };
    IRFormBlock(const IRFormBlock &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        blockdata_ = r.blockdata_;
        fromloclist_ = r.fromloclist_;
        sectionoffset_ = r.sectionoffset_;
    }
    virtual IRFormBlock * clone() const {
        return new IRFormBlock(*this);
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    Dwarf_Unsigned getBlockLen() {return blockdata_.size();}
    Dwarf_Small* getBlockBytes() {
        // Standard guarantees vector content is simply array.
        return &blockdata_[0];
    };
    enum Dwarf_Form_Class getFormClass() const {
        return formclass_;
    };
    void insertBlock(Dwarf_Block *bl) {
        Dwarf_Small *d = static_cast<Dwarf_Small *>(bl->bl_data);
        Dwarf_Unsigned len = bl->bl_len;
        blockdata_.clear();
        blockdata_.insert(blockdata_.end(),d+0,d+len);
        fromloclist_ = bl->bl_from_loclist;
        sectionoffset_ = bl->bl_section_offset;
    };
    std::vector<Dwarf_Small> getBlockData(){ return blockdata_;};
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    std::vector<Dwarf_Small> blockdata_;
    Dwarf_Small fromloclist_;
    Dwarf_Unsigned sectionoffset_;
};
class IRFormConstant : public IRForm {
public:
    enum Signedness {SIGN_NOT_SET,SIGN_UNKNOWN,UNSIGNED, SIGNED };
    IRFormConstant():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_CONSTANT),
        signedness_(SIGN_NOT_SET),
        uval_(0), sval_(0)
        { memset(&data16_,0,sizeof(data16_)); };
    IRFormConstant(IRFormInterface *);
    ~IRFormConstant() {};
    IRFormConstant(Dwarf_Half finalform,
        Dwarf_Half initialform,
        enum Dwarf_Form_Class formclass,
        IRFormConstant::Signedness signedness,
        Dwarf_Unsigned uval,
        Dwarf_Signed sval) {
        finalform_ = finalform;
        initialform_ = initialform;
        formclass_ = formclass;
        signedness_ = signedness;
        uval_ = uval;
        sval_ = sval;
    };
    IRFormConstant(Dwarf_Half finalform,
        Dwarf_Half initialform,
        enum Dwarf_Form_Class formclass,
        Dwarf_Form_Data16 & data16) {

        (void)formclass;
        finalform_ = finalform;
        initialform_ = initialform;
        formclass_ = DW_FORM_CLASS_CONSTANT;
        signedness_ = SIGN_UNKNOWN;
        uval_ = 0;
        sval_ = 0;
        data16_ = data16;
    };
    IRFormConstant & operator=(const IRFormConstant &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        signedness_ = r.signedness_;
        uval_ = r.uval_;
        sval_ = r.sval_;
        data16_ = r.data16_;
        return *this;
    };
    IRFormConstant(const IRFormConstant &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        signedness_ = r.signedness_;
        uval_ = r.uval_;
        sval_ = r.sval_;
        data16_ = r.data16_;
    }
    virtual IRFormConstant * clone() const {
        return new IRFormConstant(*this);
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    Dwarf_Form_Class getFormClass() const { return formclass_; };
    Signedness getSignedness() const {return signedness_; };
    Dwarf_Signed getSignedVal() const {return sval_;};
    Dwarf_Unsigned getUnsignedVal() const {return uval_;};
    Dwarf_Form_Data16 getData16Val() const {return data16_;};
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    // Starts at SIGN_NOT_SET.
    // SIGN_UNKNOWN means it was a DW_FORM_data* of some
    // kind so we do not really know.
    Signedness signedness_;
    // Both uval_ and sval_ are always set to the same bits.
    Dwarf_Unsigned uval_;
    Dwarf_Signed sval_;
    Dwarf_Form_Data16 data16_;

    void setValues16(Dwarf_Form_Data16 *v,
        enum Signedness s) {
        (void)s;
        uval_ = 0;
        sval_ = 0;
        data16_ = *v;
    }
    void setValues(Dwarf_Signed sval, Dwarf_Unsigned uval,
        enum Signedness s) {
        signedness_ = s;
        uval_ = uval;
        sval_ = sval;
    }
};

class IRFormExprloc : public IRForm {
public:
    IRFormExprloc():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_EXPRLOC)
        {};
    IRFormExprloc(IRFormInterface *);
    ~IRFormExprloc() {};
    IRFormExprloc & operator=(const IRFormExprloc &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        exprlocdata_ = r.exprlocdata_;
        return *this;
    };
    IRFormExprloc(const IRFormExprloc &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        exprlocdata_ = r.exprlocdata_;

    }
    virtual IRFormExprloc * clone() const {
        return new IRFormExprloc(*this);
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
    std::vector<char> getexprlocdata() const {return exprlocdata_; };
    void insertBlock(Dwarf_Unsigned len, Dwarf_Ptr data) {
        char *d = static_cast<char *>(data);
        exprlocdata_.clear();
        exprlocdata_.insert(exprlocdata_.end(),d+0,d+len);
    };
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    std::vector<char> exprlocdata_;
};

class IRFormFlag : public IRForm {
public:
    IRFormFlag():
        initialform_(0),
        finalform_(0),
        formclass_(DW_FORM_CLASS_FLAG),
        flagval_(0)
        {};
    IRFormFlag(IRFormInterface*);
    ~IRFormFlag() {};
    IRFormFlag & operator=(const IRFormFlag &r) {
        if (this == &r) return *this;
        initialform_ = r.initialform_;
        finalform_ = r.finalform_;
        formclass_ = r.formclass_;
        flagval_ = r.flagval_;
        return *this;
    };
    IRFormFlag(const IRFormFlag &r) {
        initialform_ = r.initialform_;
        finalform_ = r.finalform_;
        formclass_ = r.formclass_;
        flagval_ = r.flagval_;
    }
    virtual IRFormFlag * clone() const {
        return new IRFormFlag(*this);
    }
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    void setFlagVal(Dwarf_Bool v) { flagval_ = v;}
    Dwarf_Bool getFlagVal() { return flagval_; }
private:
    Dwarf_Half initialform_;
    // In most cases initialform_ == finalform_.
    // Otherwise, initialform == DW_FORM_indirect.
    Dwarf_Half finalform_;
    enum Dwarf_Form_Class formclass_;
    Dwarf_Bool flagval_;
};

class IRFormLinePtr : public IRForm {
public:
    IRFormLinePtr():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_LINEPTR),
        debug_line_offset_(0)
        {};
    IRFormLinePtr(IRFormInterface *);
    ~IRFormLinePtr() {};
    IRFormLinePtr & operator=(const IRFormLinePtr &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        debug_line_offset_ = r.debug_line_offset_;
        return *this;
    };
    IRFormLinePtr(const IRFormLinePtr &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        debug_line_offset_ = r.debug_line_offset_;
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm(void) {return finalform_; }
    virtual IRFormLinePtr * clone() const {
        return new IRFormLinePtr(*this);
    }
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    Dwarf_Off debug_line_offset_;

    void setOffset(Dwarf_Unsigned uval) {
        debug_line_offset_ = uval;
    };
};

class IRFormLoclistPtr : public IRForm {
public:
    IRFormLoclistPtr():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_LOCLISTPTR),
        loclist_offset_(0)
        {};
    IRFormLoclistPtr(IRFormInterface *);
    ~IRFormLoclistPtr() {};
    IRFormLoclistPtr & operator=(const IRFormLoclistPtr &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        loclist_offset_ = r.loclist_offset_;
        return *this;
    };
    IRFormLoclistPtr(const IRFormLoclistPtr &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        loclist_offset_ = r.loclist_offset_;
    }
    virtual IRFormLoclistPtr * clone() const {
        return new IRFormLoclistPtr(*this);
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    Dwarf_Off loclist_offset_;

    void setOffset(Dwarf_Unsigned uval) {
        loclist_offset_ = uval;
    };
};

class IRFormMacPtr : public IRForm {
public:
    IRFormMacPtr():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_MACPTR),
        macro_offset_(0)
        {};
    IRFormMacPtr(IRFormInterface *);
    ~IRFormMacPtr() {};
    IRFormMacPtr & operator=(const IRFormMacPtr &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        macro_offset_ = r.macro_offset_;
        return *this;
    };
    IRFormMacPtr(const IRFormMacPtr &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        macro_offset_ = r.macro_offset_;
    }
    virtual IRFormMacPtr * clone() const {
        return new IRFormMacPtr(*this);
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    Dwarf_Off macro_offset_;

    void setOffset(Dwarf_Unsigned uval) {
        macro_offset_ = uval;
    };
};

class IRFormRangelistPtr : public IRForm {
public:
    IRFormRangelistPtr():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_RANGELISTPTR),
        rangelist_offset_(0)
        {};
    IRFormRangelistPtr(IRFormInterface *);
    ~IRFormRangelistPtr() {};
    IRFormRangelistPtr & operator=(const IRFormRangelistPtr &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        rangelist_offset_ = r.rangelist_offset_;
        return *this;
    };
    IRFormRangelistPtr(const IRFormRangelistPtr &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        rangelist_offset_ = r.rangelist_offset_;
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    virtual IRFormRangelistPtr * clone() const {
        return new IRFormRangelistPtr(*this);
    }
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    Dwarf_Off rangelist_offset_;

    void setOffset(Dwarf_Unsigned uval) {
        rangelist_offset_ = uval;
    };
};

class IRFormFramePtr : public IRForm {
public:
    IRFormFramePtr():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_FRAMEPTR),
        frame_offset_(0)
        {};
    IRFormFramePtr(IRFormInterface *);
    ~IRFormFramePtr() {};
    IRFormFramePtr & operator=(const IRFormFramePtr &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        frame_offset_ = r.frame_offset_;
        return *this;
    };
    IRFormFramePtr(const IRFormFramePtr &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        frame_offset_ = r.frame_offset_;
    }
    virtual IRFormFramePtr * clone() const {
        return new IRFormFramePtr(*this);
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    Dwarf_Off frame_offset_;

    void setOffset(Dwarf_Unsigned uval) {
        frame_offset_ = uval;
    };
};

class IRFormReference : public IRForm {
public:
    IRFormReference():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_REFERENCE),
        reftype_(RT_NONE),
        globalOffset_(0),cuRelativeOffset_(0),
        targetInputDie_(0),
        target_die_(0)
        {initSig8();};
    IRFormReference(IRFormInterface *);
    ~IRFormReference() {};
    IRFormReference & operator=(const IRFormReference &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        reftype_ = r.reftype_;
        globalOffset_ = r.globalOffset_;
        cuRelativeOffset_ = r.cuRelativeOffset_;
        typeSig8_ = r.typeSig8_;
        targetInputDie_ = r.targetInputDie_;
        target_die_ = r.target_die_;
        return *this;
    };
    IRFormReference(const IRFormReference &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        reftype_ = r.reftype_;
        globalOffset_ = r.globalOffset_;
        cuRelativeOffset_ = r.cuRelativeOffset_;
        typeSig8_ = r.typeSig8_;
        targetInputDie_ = r.targetInputDie_;
        target_die_ = r.target_die_;
    }
    virtual IRFormReference * clone() const {
        return new IRFormReference(*this);
    }
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    void setOffset(Dwarf_Off off) { globalOffset_ = off;
        reftype_ = RT_GLOBAL;};
    void setCUOffset(Dwarf_Off off) { cuRelativeOffset_= off;
        reftype_ = RT_CUREL;};
    void setSignature(Dwarf_Sig8 * sig) { typeSig8_ = *sig;
        reftype_ = RT_SIG;};
    const Dwarf_Sig8 *getSignature() { return &typeSig8_;};
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
    enum RefType { RT_NONE,RT_GLOBAL, RT_CUREL,RT_SIG };
    enum RefType getReferenceType() { return reftype_;};
    Dwarf_P_Die getTargetGenDie() { return target_die_;};
    IRDie * getTargetInDie() { return targetInputDie_;};
    void setTargetGenDie(Dwarf_P_Die targ) { target_die_ = targ; };
    void setTargetInDie(IRDie* targ) { targetInputDie_ = targ; };

private:
    void initSig8();

    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    enum RefType reftype_;

    // gobalOffset_ on input target set if and only if RT_GLOBAL
    Dwarf_Off globalOffset_;
    // cuRelativeOffset_  on input targetset if and only if RT_CUREL
    Dwarf_Off cuRelativeOffset_;
    // typeSig8_ on input target set if and only if RT_SIG
    Dwarf_Sig8 typeSig8_;

    // For RT_SIG we do not need extra data.
    // For RT_CUREL and RT_GLOBAL we do.

    // For RT_CUREL.  Points at the target input DIE
    // after all input DIEs set up for a CU .
    IRDie * targetInputDie_;
    // FIXME
    Dwarf_P_Die target_die_; //for RT_CUREL, this is known
        // for sure only after all target DIEs generated!

    // RT_GLOBAL. FIXME
};

class IRFormString: public IRForm {
public:
    IRFormString():
        finalform_(0), initialform_(0),
        formclass_(DW_FORM_CLASS_STRING),
        strpoffset_(0) {};
    ~IRFormString() {};
    IRFormString(IRFormInterface *);
    IRFormString(const IRFormString &r) {
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        formdata_= r.formdata_;
        strpoffset_= r.strpoffset_;
    }
    virtual IRFormString * clone() const {
        return new IRFormString(*this);
    }
    IRFormString & operator=(const IRFormString &r) {
        if (this == &r) return *this;
        finalform_ = r.finalform_;
        initialform_ = r.initialform_;
        formclass_ = r.formclass_;
        formdata_ = r.formdata_;
        strpoffset_ = r.strpoffset_;
        return *this;
    };
    void setInitialForm(Dwarf_Half v) { initialform_ = v;}
    void setFinalForm(Dwarf_Half v) { finalform_ = v;}
    Dwarf_Half getInitialForm() { return initialform_;}
    Dwarf_Half getFinalForm() {return finalform_;}
    void setString(const char *s) {formdata_ = s; };
    const std::string & getString() const {return formdata_; };
    enum Dwarf_Form_Class getFormClass() const { return formclass_; };
private:
    Dwarf_Half finalform_;
    // In most cases directform == indirect form.
    // Otherwise, directform == DW_FORM_indirect.
    Dwarf_Half initialform_;
    enum Dwarf_Form_Class formclass_;
    std::string formdata_;
    Dwarf_Unsigned strpoffset_;
};

// Factory Method.
IRForm *formFactory(Dwarf_Debug dbg, Dwarf_Attribute attr,
    IRCUdata &cudata,IRAttr & irattr);
