/*
Copyright (C) 2010-2016 David Anderson.  All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

*   Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
*   Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the
    following disclaimer in the
    documentation and/or other materials provided
    with the distribution.
*   Neither the name of the example nor the
    names of its contributors may be used to endorse
    or promote products
    derived from this software without specific prior
    written permission.

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
// irepresentation.h
// The internal (to dwarfgen) representation of debug information.
// All the various components (info, frame, etc)
// will be stored here in an internal-to-dwarfgen form.
//
//
#include "irepform.h"
#include "irepline.h"
#include "irepdie.h"
#include "irepmacro.h"
#include "irepframe.h"
#include "ireppubnames.h"
#include "strtabdata.h"

// The elf symbols are used to tie relocations to values.
// We do relocations ourselves in dwarfgen so the data is not needed
// once the dwarf .debug_* sections created  in elf.
// We don't write the symbols out as an elf section.
// The position in the vector of symbols is the 'elf symbol index'
// we create.
// Symbol 0 is 'no symbol'.
// Symbol 1 is .text
class ElfSymbol {
public:
    ElfSymbol():symbolValue_(0),
        nameIndex_(0) {};
    ElfSymbol(Dwarf_Unsigned val, const std::string&name,
        strtabdata&stab):
        symbolValue_(val),name_(name) {
        nameIndex_ = stab.addString(name);
    };
    ~ElfSymbol() {};
    Dwarf_Unsigned getSymbolValue() const { return symbolValue_;}
private:
    Dwarf_Unsigned symbolValue_;
    std::string    name_;
    // The offset in the string table.
    unsigned   nameIndex_;
};

//  It's very easy to confuse the section number in an elf file
//  with an array index in dwarfgen.
//  So this class hold an elf section number
//  and gives those a recognizable type.
//  As of 2023 the ElfSectIndex is an index into
//  <vector>SectionForDwarf dwsectab
//  whereas the libdwarfp arrays are fixed values
//  (see NUM_DEBUG_SECTIONS)
class ElfSectIndex {
public:
    ElfSectIndex():elfsect_(0) {};
    ~ElfSectIndex() {};
    ElfSectIndex(unsigned v):elfsect_(v) {};
    unsigned getSectIndex() const { return elfsect_; }
    void setSectIndex(unsigned v) { elfsect_ = v; }
private:
    unsigned elfsect_;
};

//  It's very easy to confuse the symbol number in an elf file
//  with a symbol number in dwarfgen.
//  So this class hold an elf symbol number number
//  and gives those a recognizable type.
class ElfSymIndex {
public:
    ElfSymIndex():elfsym_(0) {};
    ~ElfSymIndex() {};
    ElfSymIndex(unsigned v):elfsym_(v) {};
    Dwarf_Unsigned getSymIndex() const { return elfsym_; }
    void setSymIndex(unsigned v) { elfsym_ = v; }
private:
    Dwarf_Unsigned elfsym_;
};

class ElfSymbols {
public:
    ElfSymbols() {
        // The initial symbol is 'no symbol'.
        std::string emptyname("");
        elfSymbols_.push_back(ElfSymbol(0,emptyname,symstrtab_));

        // We arbitrarily make this symbol .text now, though
        // not needed yet.
        std::string textname(".text");
        elfSymbols_.push_back(ElfSymbol(0,textname,symstrtab_));
        baseTextAddressSymbol_.setSymIndex(elfSymbols_.size()-1);
        }
    ~ElfSymbols() {};
    ElfSymIndex getBaseTextSymbol() const {
        return baseTextAddressSymbol_;};
    ElfSymIndex addElfSymbol(Dwarf_Unsigned val,
        const std::string&name) {
        elfSymbols_.push_back(ElfSymbol(val,name,symstrtab_));
        ElfSymIndex indx(elfSymbols_.size()-1);
        return indx;

    };
    ElfSymbol &  getElfSymbol(ElfSymIndex symi) {
        size_t i = symi.getSymIndex();
        if (i >= elfSymbols_.size()) {
            std::cerr << "Error, sym index " << i <<
            "  too big for symtab size " <<
            elfSymbols_.size() << std::endl;
            exit(1);
        }
        return elfSymbols_[i];
    }
private:
    std::vector<ElfSymbol> elfSymbols_;
    strtabdata symstrtab_;
    ElfSymIndex  baseTextAddressSymbol_;
};

class IRepresentation {
public:
    IRepresentation() {};
    ~IRepresentation(){};
    IRFrame &framedata() { return framedata_; };
    IRFrame &ehframedata() { return ehframedata_; };
    IRMacro &macrodata() { return macrodata_; };
    IRDInfo &infodata() { return debuginfodata_; };
    IRPubsData &pubnamedata() {return pubnamedata_;};
    ElfSymbols &getElfSymbols() { return elfSymbols_;};
    unsigned getBaseTextSymbol() {
        return elfSymbols_.getBaseTextSymbol().getSymIndex();};
private:
    // The Elf symbols data to use for relocations
    ElfSymbols elfSymbols_;

    IRFrame  framedata_;
    IRFrame  ehframedata_;
    IRMacro  macrodata_;
    IRPubsData  pubnamedata_;

    // .debug_info and its line data are inside debuginfodata_;
    IRDInfo  debuginfodata_;
    // .debug_types and its line data are in debugtypesdata_.
    IRDInfo  debugtypesdata_;
};
