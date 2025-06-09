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

// binfile - class library for files/streams - archive files

#include "binfarc.h"

abinfile::abinfile()
{
}

abinfile::~abinfile()
{
  close();
}

errstat abinfile::open(binfile &fil, binfilepos ofs, binfilepos len)
{
  close();
  int fmode=fil.getmode()&~modeappend;
  if (!fmode)
    return -1;
  f=&fil;
  if (!(fmode&modeseek))
  {
    fmode&=~modewrite;
    if (!(fmode&moderead)||(ofs!=f->ioctl(f->ioctlrtell)))
      return -1;
  }
  else
  {
    fofs=ofs;
    long l=f->length();
    if (fofs>l)
      fofs=l;
    if ((fofs+len)>l)
      len=l-fofs;
  }
  rawpos=0;
  rawlen=len;
  openmode(fmode, rawpos, rawlen);
  return 0;
}

binfilepos abinfile::rawread(void *buf, binfilepos len)
{
  if ((rawpos+len)>rawlen)
    len=rawlen-rawpos;
  if (mode&modeseek)
    f->seek(fofs+rawpos);
  len=f->read(buf, len);
  rawpos+=len;
  return len;
}

binfilepos abinfile::rawpeek(void *buf, binfilepos len)
{
  if ((rawpos+len)>rawlen)
    len=rawlen-rawpos;
  len=f->peek(buf, len);
  return len;
}

binfilepos abinfile::rawwrite(const void *buf, binfilepos len)
{
  if ((rawpos+len)>rawlen)
    len=rawlen-rawpos;
  f->seek(fofs+rawpos);
  len=f->write(buf, len);
  rawpos+=len;
  return len;
}

binfilepos abinfile::rawseek(binfilepos pos)
{
  rawpos=(pos>rawlen)?rawlen:pos;
  return rawpos;
}
