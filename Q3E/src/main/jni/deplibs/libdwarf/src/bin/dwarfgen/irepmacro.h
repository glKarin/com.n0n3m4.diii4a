/*
Copyright (C) 2010-2013 David Anderson.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following
conditions are met:

*   Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.
*   Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.
*   Neither the name of the example nor the
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
// irepmacro.h
//

class IRMacroRecord {
public:
    IRMacroRecord() {};
    ~IRMacroRecord() {};
    IRMacroRecord(Dwarf_Off cuDieOffset,Dwarf_Off offset,
        Dwarf_Small type,
        Dwarf_Signed lineno, Dwarf_Signed lineindex,
        const std::string &macro):cuDieOffset_(cuDieOffset),
        offset_(offset),
        type_(type),lineno_(lineno),lineindex_(lineindex),
        macro_(macro) {};
    Dwarf_Off get_cudieoffset() {return cuDieOffset_;};
    Dwarf_Off get_offset()    {return offset_;};
    Dwarf_Off get_type()      {return type_;};
    Dwarf_Off get_lineno()    {return lineno_;};
    Dwarf_Off get_lineindex() {return lineindex_;};
    std::string& get_macro()  {return macro_; }
private:
    Dwarf_Off    cuDieOffset_;
    Dwarf_Off    offset_;
    Dwarf_Small  type_;
    Dwarf_Signed lineno_;
    Dwarf_Signed lineindex_;
    std::string  macro_;
};
class IRMacro {
public:
    IRMacro() {};
    ~IRMacro() {};
    std::vector<IRMacroRecord> &getMacroVec() { return macrorec_; };
private:
    std::vector<IRMacroRecord> macrorec_;
};
