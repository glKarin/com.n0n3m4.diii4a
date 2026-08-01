/*
  Copyright (C) 2010-2018 David Anderson.  All rights reserved.

  Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that the
  following conditions are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.
  * Neither the name of the example nor the
    names of its contributors may be used to endorse or
    promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David
  Anderson BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
// general.h
// The following needed in code using this.
#ifndef GENERAL_H
#define GENERAL_H
#include <sstream>
#include <iomanip> // iomanip for setw etc

// Run time flags from command line.
extern struct CmdOptions {
    bool transformHighpcToConst;
    int  defaultInfoStringForm;
    bool showrelocdetails;
    bool adddata16;
    bool addimplicitconst;
    bool addframeadvanceloc;
    bool addSUNfuncoffsets;
    bool adddebugsup;
    bool addskipbranch;
    bool addlanguageversion;
} cmdoptions;

template <typename T >
std::string IToHex(T v,unsigned l=0)
{
    if (v == 0) {
        // For a zero value, ostringstream does not insert 0x.
        // So we do zeroes here.
        std::string out = "0x0";
        if (l > 3)  {
            out.append(l-3,'0');
        }
        return out;
    }
    std::ostringstream s;
    s.setf(std::ios::hex,std::ios::basefield);
    s.setf(std::ios::showbase);
    if (l > 0) {
        s << std::setw(l);
    }
    s << v ;
    return s.str();
}

template <typename T>
std::string  BldName(const std::string & prefix, T v)
{
    std::ostringstream s;
    s << prefix;
    s << v;
    return s.str();
}
#endif /* GENERAL_H */
