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

#ifndef LINUX
#error must compile for LINUX
#endif

#ifndef __LINUXPLAYBINFILE_H
#define __LINUXPLAYBINFILE_H

#include "binfile.h"

class linuxplaybinfile : public binfile
{
protected:
  int wavehnd;
  virtual errstat rawclose();
  virtual binfilepos rawwrite(const void *buf, binfilepos len);

public:
  linuxplaybinfile();
  virtual ~linuxplaybinfile();

  int open(int rate, int stereo, int bit16);
};

#endif
