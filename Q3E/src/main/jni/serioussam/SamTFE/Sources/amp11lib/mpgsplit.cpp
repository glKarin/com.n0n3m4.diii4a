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

// MPEG stream splitter

#include "mpgsplit.h"

mpegsplitstream::mpegsplitstream()
{
}

mpegsplitstream::~mpegsplitstream()
{
}

errstat mpegsplitstream::open(binfile &f, int s, int scanal)
{
  if (!(f.getmode()&moderead))
    return -1;
  file=&f;
  scanall=scanal;
  sub=s;
  left=0;
  end=0;
  if (!nextblock())
    return -1;
  openmode(moderead, 0, 0);
  return 0;
}

errstat mpegsplitstream::rawclose()
{
  closemode();
  return 0;
}

int mpegsplitstream::nextblock()
{
  while (1)
  {
    if (file->eof())
      return 0;
    if ((peekub4(*file)>>8)!=1)
    {
      getch(*file);
      continue;
    }
    int b=getub4(*file)&0xFF;
    if (b==sub)
      break;
    if (b==0xb9)
    {
      getch(*file);
      return 0;
    }
    if (b==0xba)
    {
      file->read(0,8);
      continue;
    }
    if (b==0xbb)
    {
      int l=getub2(*file)-6;
      if (scanall)
      {
        file->read(0,l);
        continue;
      }
      file->read(0,6);
      while ((l>=3)&&(peeku1(*file)&0x80))
      {
        if (peeku1(*file)==sub)
          break;
        file->read(0,3);
        l-=3;
      }
      if (l<=0)
        return 0;
      file->read(0,l);
      continue;
    }
    if (b>0xbb)
      file->read(0,getub2(*file));
  }
  int l=getub2(*file);
  while (peeku1(*file)==0xFF)
  {
    getu1(*file);
    l--;
  }
  if ((peeku1(*file)&0xC0)==0x40)
  {
    file->read(0,2);
    l-=2;
  }
  if ((peeku1(*file)&0xF0)==0x20)
  {
    file->read(0,5);
    l-=5;
  }
  else
  if ((peeku1(*file)&0xF0)==0x30)
  {
    file->read(0,10);
    l-=10;
  }
  else
  if (peeku1(*file)==0x0F)
  {
    getu1(*file);
    l--;
  }
  else
  {
    file->read(0,l);
    return 0;
  }
  left=l;
  return 1;
}

binfilepos mpegsplitstream::rawread(void *buf, binfilepos len)
{
  binfilepos l=0;
  while (len&&!end)
  {
    if (file->eof())
      end=1;
    binfilepos l0=file->read((char*)buf+l, (len>left)?left:len);
    left-=l0;
    l+=l0;
    len-=l0;
    if (!left)
      if (!nextblock())
        end=1;
  }
  return l;
}

binfilepos mpegsplitstream::rawpeek(void *buf, binfilepos len)
{
  return file->peek(buf, (len>left)?left:len);
}

binfilepos mpegsplitstream::rawioctl(intm code, void *buf, binfilepos len)
{
  switch (code)
  {
  case ioctlrreof: return end;
  default: return binfile::rawioctl(code, buf, len);
  }
}
