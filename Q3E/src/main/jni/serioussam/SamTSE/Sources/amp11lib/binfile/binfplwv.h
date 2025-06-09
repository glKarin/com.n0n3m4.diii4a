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

#ifndef __WAVPLAYBINFILE_H
#define __WAVPLAYBINFILE_H

#include "binfile.h"

class wavplaybinfile : public binfile
{
protected:
  binfile *file;
  virtual errstat rawclose();
  virtual binfilepos rawwrite(const void *buf, binfilepos len);

public:
  wavplaybinfile();
  virtual ~wavplaybinfile();

  int open(binfile &f, int rate, int stereo, int bit16, int len);
};

#endif
