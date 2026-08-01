#ifndef STRTABDATA_H
#define STRTABDATA_H
/*
  Copyright (C) 2010-2013 David Anderson.  All rights reserved.

  Redistribution and use in source and binary forms,
  with or without modification, are permitted provided
  that the following conditions are met:
  * Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.
  * Redistributions in binary form must reproduce the
    above copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.
  * Neither the name of the example nor the
    names of its contributors may be used to endorse or
    promote products derived from this software without
    specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS''
    AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
    SHALL David Anderson BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
    OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// strtabdata.h
// Creates a string table in a way consistent with
// elf string tables. The zero index is a null byte always.

class strtabdata {
public:
    strtabdata(): data_(new char[1000]),
        datalen_(1000), nexttouse_(0) { data_[0] = 0;
        nexttouse_ = 1;};
    ~strtabdata()  { delete[]data_; };
    unsigned addString(const std::string & newstr) {
        // The 1 is for the terminating null byte.
        unsigned nsz = newstr.size() +1;
        unsigned needed = nexttouse_ + nsz;
        if (needed >= datalen_) {
            unsigned baseincr = nsz;
            unsigned altincr = datalen_*2;
            if (altincr> baseincr) {
                baseincr = altincr;
            }
            unsigned newsize = datalen_ + baseincr;
            char *newdata = new char [newsize];
            memcpy(newdata, data_, nexttouse_);
            delete [] data_;
            data_ = newdata;
            datalen_ = newsize;
        }
        memcpy(data_ + nexttouse_, newstr.c_str(),nsz);
        unsigned newstrindex = nexttouse_;
        nexttouse_ += nsz;
        return newstrindex;
    };
    std::string asString(unsigned offset) {
        return std::string(data_+ offset);
    }
    void *exposedata() {return (void *)data_;};
    unsigned exposelen() const {return nexttouse_;};
private:
    char *   data_;

    // datalen_ is the size in bytes pointed to by data_ .
    unsigned datalen_;

    // nexttouse_ is the index of the next (unused) byte in
    // data_ , so it is also the amount of space in data_ that
    // is in use.
    unsigned nexttouse_;
};
#endif /* STRTABDATA_H */
