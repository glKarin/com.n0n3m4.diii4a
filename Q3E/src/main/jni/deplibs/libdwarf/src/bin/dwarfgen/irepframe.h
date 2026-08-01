/*
Copyright (C) 2010-2013 David Anderson.  All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

* Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

* Neither the name of the example nor the names of its
    contributors may be used to endorse or promote products
    derived from this software without specific prior written
    permission.

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
// irepframe.h
//

class IRCie {
public:
    IRCie(): cie_byte_length_(0), version_(0),
        code_alignment_factor_(1),
        data_alignment_factor_(1),
        return_address_register_rule_(0)
        {};
    IRCie(Dwarf_Unsigned length, Dwarf_Unsigned version,
        const std::string &augmentation, Dwarf_Unsigned code_align,
        Dwarf_Signed data_align, Dwarf_Half return_reg_rule,
        const void * init_instrs, Dwarf_Unsigned instrs_len):
        cie_byte_length_(length), version_(version),
        augmentation_(augmentation),
        code_alignment_factor_(code_align),
        data_alignment_factor_(data_align),
        return_address_register_rule_(return_reg_rule)
        {
            const Dwarf_Small *x =
                reinterpret_cast<const Dwarf_Small *>(init_instrs);
            for (Dwarf_Unsigned i = 0; i < instrs_len; ++i) {
                initial_instructions_.push_back(x[i]);
            }
        }
    void insert_fde_index(unsigned i) { fde_index_.push_back(i); };
    ~IRCie() {};
    void get_basic_cie_data(Dwarf_Unsigned * version,
        std::string  * aug,
        Dwarf_Unsigned * code_align,
        Dwarf_Signed * data_align,
        Dwarf_Half   * ret_addr_reg) {
            *version = version_;
            *aug = augmentation_;
            *code_align = code_alignment_factor_;
            *data_align = data_alignment_factor_;
            *ret_addr_reg = return_address_register_rule_;
        }
    void get_init_instructions(Dwarf_Unsigned *len,
        void **bytes) {
        *len = initial_instructions_.size();
        *bytes = reinterpret_cast<void *>(&initial_instructions_[0]);
        };
    Dwarf_Unsigned get_cie_byte_length() {return cie_byte_length_;}
    Dwarf_Unsigned get_version() {return version_;}
    std::string& get_augmentation() {return augmentation_;}
    Dwarf_Unsigned get_code_aligmnent_factor() {
        return code_alignment_factor_;}
    Dwarf_Unsigned get_data_aligmnent_factor() {
        return data_alignment_factor_;}
    Dwarf_Half     get_return_address_register_rule() {
        return return_address_register_rule_;}
    std::vector<Dwarf_Small>& get_initial_instructions() {
        return initial_instructions_;}
    std::vector<unsigned int>& get_fde_index() {
        return fde_index_;}

private:
    //  Byte length  0 if not known yet.
    Dwarf_Unsigned cie_byte_length_;
    Dwarf_Unsigned version_;
    std::string augmentation_;
    Dwarf_Unsigned code_alignment_factor_;
    Dwarf_Signed data_alignment_factor_;
    Dwarf_Half   return_address_register_rule_;
    std::vector<Dwarf_Small> initial_instructions_;
    // fde_index is the array of indexes into fdedata_
    // that are fdes used by this cie.
    std::vector<unsigned int>  fde_index_;
};
class IRFde {
public:
    IRFde(): low_pc_(0), func_length_(0),
        cie_offset_(0), cie_index_(-1),
        fde_offset_(0)  {};
    IRFde(Dwarf_Addr low_pc,Dwarf_Unsigned func_length,
        Dwarf_Ptr fde_bytes, Dwarf_Unsigned fde_length,
        Dwarf_Off cie_offset,Dwarf_Signed cie_index_in,
        Dwarf_Off fde_offset):
        low_pc_(low_pc),
        func_length_(func_length),
        cie_offset_(cie_offset), cie_index_(cie_index_in),
        fde_offset_(fde_offset)  {
            const Dwarf_Small *x =
                reinterpret_cast<const Dwarf_Small *>(fde_bytes);
            for (Dwarf_Unsigned i = 0; i < fde_length; ++i) {
                fde_bytes_.push_back(x[i]);
            }
        };
    ~IRFde() {};
    Dwarf_Unsigned cie_index() { return cie_index_; };
    void get_fde_base_data(Dwarf_Addr *lowpc,
        Dwarf_Unsigned * funclen,
        Dwarf_Unsigned *cie_index_input) {
            *lowpc = low_pc_;
            *funclen = func_length_;
            *cie_index_input = cie_index_;
        };
    void get_fde_instrs_into_ir(Dwarf_Ptr ip,Dwarf_Unsigned len   )  {
        const Dwarf_Small *x =
        reinterpret_cast<const Dwarf_Small *>(ip);
        for (Dwarf_Unsigned i = 0; i < len; ++i) {
            fde_instrs_.push_back(x[i]);
        }
        };

    void get_fde_instructions(Dwarf_Unsigned *len,
        void **bytes) {
        *len = fde_instrs_.size();
        *bytes = reinterpret_cast<void *>(&fde_instrs_[0]);
        };
    void fde_instrs () {
        };
    Dwarf_Off get_fde_offset() {
        return fde_offset_;}
    Dwarf_Off get_cie_offset() {
        return cie_offset_;}

private:
    Dwarf_Addr low_pc_;
    Dwarf_Unsigned func_length_;
    // fde_bytes_ may be empty if content bytes not yet created.
    std::vector<Dwarf_Small> fde_bytes_;

    // fde_instrs_ is simply a vector of bytes.
    // it might be good to actually parse the
    // instructions.
    std::vector<Dwarf_Small> fde_instrs_;
    // cie_offset may be 0 if not known yet.
    Dwarf_Off  cie_offset_;
    // cie_index is the index in ciedata_  of
    // the applicable CIE. Begins with index 0.
    Dwarf_Signed cie_index_;
    // fde_offset may be 0 if not yet known.
    Dwarf_Off   fde_offset_;
};

class IRFrame {
public:
    IRFrame() {};
    ~IRFrame() {};
    void insert_cie(IRCie &cie) {
        ciedata_.push_back(cie);
    }
    void insert_fde(IRFde &fdedata) {
        fdedata_.push_back(fdedata);
        unsigned findex = fdedata_.size() -1;
        Dwarf_Signed cindex = fdedata.cie_index();
        if (cindex != -1) {
            IRCie & mycie =  ciedata_[cindex];
            mycie.insert_fde_index(findex);
        }
    }
    std::vector<IRCie> &get_cie_vec() { return ciedata_; };
    std::vector<IRFde> &get_fde_vec() { return fdedata_; };
private:
    std::vector<IRCie> ciedata_;
    std::vector<IRFde> fdedata_;
};
