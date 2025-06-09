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

#ifndef DOS
#error must compile for DOS4G or DJGPP
#endif

#ifndef __SBPLAYBINFILE_H
#define __SBPLAYBINFILE_H

#include "binfile.h"

class sbplaybinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawwrite(const void *buf, binfilepos len);

public:
  sbplaybinfile();
  virtual ~sbplaybinfile();

  int open(int rate, int stereo, int bit16, int buflen);
};

#endif
