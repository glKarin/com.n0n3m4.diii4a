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

#ifndef __BINFCON_H
#define __BINFCON_H

#include "binfile.h"

#ifdef UNIX
#include <termios.h>
#endif

class idconsolebinfile : public binfile
{
protected:
#ifdef UNIX
  termios savedtermios;
#endif
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  idconsolebinfile();
  virtual ~idconsolebinfile();
  int open();
};

class consolebinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  consolebinfile();
  virtual ~consolebinfile();
};

class iconsolebinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  iconsolebinfile();
  virtual ~iconsolebinfile();
};

class oconsolebinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  oconsolebinfile();
  virtual ~oconsolebinfile();
};

class econsolebinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  econsolebinfile();
  virtual ~econsolebinfile();
};

#endif
