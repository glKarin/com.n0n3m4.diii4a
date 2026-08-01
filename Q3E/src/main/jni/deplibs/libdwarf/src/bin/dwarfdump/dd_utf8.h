/*
Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

*/
#ifndef DD_UTF8_H
#define DD_UTF8_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Dwarf_Unsigned
dd_utf8_decode(Dwarf_Unsigned* state, Dwarf_Unsigned* codep,
    unsigned char byte);

int dd_utf8_checkCodePoints(unsigned char * s);

int dd_utf8_countCodePoints(unsigned char * s,
    Dwarf_Unsigned* count);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DD_UTF8_H */
