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

// just a timer for WC/DOS4G

#include <string.h>
#include <conio.h>
#include <i86.h>
#include "timer.h"

static void (__far __interrupt *tmOldTimer)();
static void (*tmTimerRoutine)();
static unsigned __int64 tmTicker;

static void far *getvect(unsigned char intno)
{
  REGS r;
  SREGS sr;
  r.h.ah=0x35;
  r.h.al=intno;
  sr.ds=sr.es=0;
  int386x(0x21, &r, &r, &sr);
  return MK_FP(sr.es, r.x.ebx);
}

static void setvect(unsigned char intno, void far *vect)
{
  REGS r;
  SREGS sr;
  r.h.ah=0x25;
  r.h.al=intno;
  r.x.edx=FP_OFF(vect);
  sr.ds=FP_SEG(vect);
  sr.es=0;
  int386x(0x21, &r, &r, &sr);
}

static void __interrupt tmTimerHandler()
{
  tmTicker+=65536;
  tmOldTimer();
}

static unsigned __int64 get()
{
  unsigned long tm=tmTicker;
  outp(0x43,0);
  tm-=inp(0x40);
  tm-=inp(0x40)<<8;
  return tm;
}

void tmInit()
{
  tmOldTimer=(void (__far __interrupt *)())getvect(0x08);
  setvect(0x08, tmTimerHandler);

  outp(0x43, 0x34);
  outp(0x40, 0x00);
  outp(0x40, 0x00);

  get();
  tmTicker-=get();
}

unsigned long tmGetTimer()
{
  return (get()*28125)>>25;
}

void tmClose()
{
  setvect(0x08, tmOldTimer);
  outp(0x43, 0x34);
  outp(0x40, 0x00);
  outp(0x40, 0x00);
}
