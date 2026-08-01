/*
Copyright (C) 2010-2018 David Anderson.  All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:
* Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following
    disclaimer.
* Redistributions in binary form must reproduce the
    above copyright
    notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.
* Neither the name of the example nor the
    names of its contributors may be used to endorse or
    promote products derived from this software without
    specific prior written permission.

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
// ireppubnames.h
//
//

class IRPub{
public:
    IRPub():dieoffset_(0),cudieoffset_(0) {
        };
    IRPub(const std::string &name,
        Dwarf_Unsigned dieoffset,Dwarf_Unsigned cudieoffset):
        name_(name),dieoffset_(dieoffset),cudieoffset_(cudieoffset) {
    };
    IRPub(const IRPub&r) {
        name_ = r.name_;
        dieoffset_ = r.dieoffset_;
        cudieoffset_ = r.cudieoffset_;
    };
    IRPub& operator=( const IRPub&r) {
        if (this == &r) {
            return *this;
        }
        name_ = r.name_;
        dieoffset_ = r.dieoffset_;
        cudieoffset_ = r.cudieoffset_;
        return *this;
    };
    void setBaseData(const std::string &name,
        Dwarf_Unsigned dieoffset,
        Dwarf_Unsigned cudieoffset){
        name_ = name;
        dieoffset_ = dieoffset;
        cudieoffset_ = cudieoffset;
    };
    const std::string& getName() {return name_;};
    Dwarf_Unsigned getDieOffset() {return dieoffset_;};
    Dwarf_Unsigned getCUdieOffset() {return cudieoffset_;}
private:
    std::string name_;
    Dwarf_Unsigned dieoffset_;
    Dwarf_Unsigned cudieoffset_;
};

// For .debug_pubnames and .debug_pubtypes.
// By the Dwarf Std sec 6.1.1 "Lookup by Name"
// these names and offsets refer to the .debug_info section.
class IRPubsData {
public:
    IRPubsData() {};
    ~IRPubsData() {};
    std::list<IRPub>& getPubnames() {return pubnames_; };
    std::list<IRPub>& getPubtypes() {return pubtypes_; };
private:
    std::list<IRPub>  pubnames_;
    std::list<IRPub>  pubtypes_;
};
