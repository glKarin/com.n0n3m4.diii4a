/* Copyright (c) 1997-2001 Niklas Beisert, Alen Ladavac. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// binfile - class library for files/streams - float functions

#include "binfile.h"

floatmax getf4(binfile &f) { float874 v; if (f.eread(&v,4)) return v; else return 0; }
floatmax getf8(binfile &f) { float878 v; if (f.eread(&v,8)) return v; else return 0; }
floatmax getf10(binfile &f) { float8710 v; if (f.eread(&v,10)) return v; else return 0; }
boolm putf4(binfile &f, floatmax v) { float874 v2=v; return f.ewrite(&v2, 4); }
boolm putf8(binfile &f, floatmax v) { float878 v2=v; return f.ewrite(&v2, 8); }
boolm putf10(binfile &f, floatmax v) { float8710 v2=v; return f.ewrite(&v2, 10); }
