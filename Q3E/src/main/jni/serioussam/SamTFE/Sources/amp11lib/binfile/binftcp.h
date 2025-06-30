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

#ifndef __BINFTCP_H
#define __BINFTCP_H

#include "binfile.h"

class tcpbinfile;
class tcplistener;

class tcplistener
{
  friend class tcpbinfile;

private:
  intm handle;
  intm blocking;
public:
  tcplistener();
  ~tcplistener();
  errstat open(uint2 port, intm conbuf);
  errstat close();
  boolm setblocking(boolm);
  boolm getblocking();
};

class tcpbinfile : public binfile
{
  friend class tcplistener;

private:
  static intm initcount;
  intm handle;
  boolm closed;
  boolm wclosed;
  intm blocking;

  static boolm globalinit();
  static void globaldone();

protected:
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawpeek(void *buf, binfilepos len);
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  tcpbinfile();
  virtual ~tcpbinfile();

  errstat open(uint4 addr, uint2 port);
  errstat open(const char *addr, uint2 port);
  errstat open(tcplistener &l);

  uint4 getremoteaddr();
  uint4 getlocaladdr();
  uint2 getremoteport();
  uint2 getlocalport();

  static char *addrtoa(char *buf, uint4 addr);
  static char *addrtoa(char *buf, uint4 addr, uint2 port);
  static uint4 atoaddr(const char *buf);
  static uint2 atoport(const char *buf, uint2 port);
};

#endif
