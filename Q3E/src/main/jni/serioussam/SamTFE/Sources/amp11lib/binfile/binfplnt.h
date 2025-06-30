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

#ifndef WIN32
#error must compile for NT
#endif

#ifndef __NTPLAYBINFILE_H
#define __NTPLAYFILEPLAY_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <mmsystem.h>
#include "binfile.h"

class ntplaybinfile : public binfile
{
protected:
  HWAVEOUT wavehnd;
  int blklen;
  int nblk;
  WAVEHDR *hdrs;
  char *playbuf;
  int curbuf;
  int curbuflen;
  int linger;
  int blocking;

  virtual errstat rawclose();
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  ntplaybinfile();
  virtual ~ntplaybinfile();

  // get descriptive name of a wave device
  static int getdevicename(int device, char *namebuffer, int bufferlen);

  int open(int rate, int stereo, int bit16, int blen, int nb, int device=-1);
};

#endif
