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

#ifndef __BINFSTD_H
#define __BINFSTD_H

#include "binfile.h"

class sbinfile : public binfile
{
private:
#ifndef NOUNISTD
  intm handle;
  boolm trunc;
#else
  FILE *file;
#endif

protected:
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawpeek(void *buf, binfilepos len);
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawseek(binfilepos pos);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  sbinfile();
  virtual ~sbinfile();

  enum
  {
    openis=0, openos=1, openro=2, openrw=3,
    openex=0, opencr=4, opentr=8, opencn=12,
    openiomode=3,
    opencrmode=12,
  };
  errstat open(const char *name, intm type);
};

#endif
