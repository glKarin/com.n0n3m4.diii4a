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

#ifndef __BINFMEM_H
#define __BINFMEM_H

#include "binfile.h"

class mbinfile : public binfile
{
private:
  uint1 *filebuf;

  binfilepos *flen;
  void **fbuf;
  binfilepos rawlen;
  binfilepos rawpos;
  binfilepos fbuflen;
  intm fleninc;
  boolm freemem;
  binfilepos chsize(binfilepos);
  boolm trunc;

protected:
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawpeek(void *buf, binfilepos len);
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawseek(binfilepos pos);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  mbinfile();
  virtual ~mbinfile();

  enum { openro=0, openrw=1, openfree=2 };

  errstat open(void *buf, binfilepos len, intm type);
  errstat opencs(void *&buf, binfilepos &len, intm inc);
};

#endif
