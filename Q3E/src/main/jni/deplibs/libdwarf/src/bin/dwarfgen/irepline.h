#ifndef IREPLINE_H
#define IREPLINE_H
/*
Copyright (C) 2010-2013 David Anderson.  All rights reserved.

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
// irepline.h
//

class IRCULine {
public:
    IRCULine(Dwarf_Addr addr,Dwarf_Bool isset,
        Dwarf_Unsigned fileno,
        Dwarf_Unsigned lineno,
        Dwarf_Unsigned linecol,
        const std::string & linename,
        Dwarf_Bool is_stmt,
        Dwarf_Bool bb,
        Dwarf_Bool endseq,
        Dwarf_Bool prol_end,
        Dwarf_Bool epil_beg,
        Dwarf_Unsigned isa,
        Dwarf_Unsigned discrim):
        address_(addr),
        isaddrset_(isset),
        srcfileno_(fileno),
        lineno_(lineno),
        linecol_(linecol),
        linesrc_(linename),
        is_stmt_(is_stmt),
        basic_block_(bb),
        end_sequence_(endseq),
        prologue_end_(prol_end),
        epilogue_begin_(epil_beg),
        isa_(isa),
        discriminator_(discrim)
        {};
    const std::string &getpath() { return linesrc_; };
    Dwarf_Addr getaddr() { return address_;};
    bool getaddrset() { return isaddrset_;};
    bool getendsequence() { return end_sequence_; };
    Dwarf_Unsigned getlineno() { return lineno_; };
    Dwarf_Unsigned getlinecol() { return linecol_; };
    Dwarf_Unsigned getsrcfileno() { return srcfileno_; };
    bool getisstmt() { return is_stmt_; };
    bool getisblock() { return basic_block_; };
    bool getepiloguebegin() { return epilogue_begin_; };
    bool getprologueend() { return prologue_end_; };
    Dwarf_Unsigned getisa() { return isa_; };
    Dwarf_Unsigned getdiscriminator() { return discriminator_; };
    ~IRCULine() {};
private:

    // Names taken from the DWARF4 std. document, sec 6.2.2.
    Dwarf_Addr address_;
    bool isaddrset_;
    Dwarf_Unsigned srcfileno_;
    Dwarf_Unsigned lineno_;
    Dwarf_Signed   linecol_; // aka lineoff
    std::string    linesrc_; // Name for the file,
        //constructed by libdwarf.
    bool           is_stmt_;
    bool           basic_block_;
    bool           end_sequence_;
    bool           prologue_end_;
    bool           epilogue_begin_;
    int            isa_;
    int            discriminator_;
};
class IRCUSrcfile {
public:
    IRCUSrcfile(std::string file): cusrcfile_(std::move(file)) {};
    ~IRCUSrcfile() {};
    std::string &getfilepath() {return cusrcfile_;};
private:
    std::string cusrcfile_;
};

class IRCULineData {
public:
    IRCULineData() {};
    ~IRCULineData() {};
    std::vector<IRCULine> &get_cu_lines() { return culinedata_; };
    std::vector<IRCUSrcfile> &get_cu_srcfiles() {
        return cusrcfiledata_; };
private:
    std::vector<IRCUSrcfile> cusrcfiledata_;
    std::vector<IRCULine> culinedata_;
};
#endif // IREPLINE_H
