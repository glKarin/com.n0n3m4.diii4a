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

#ifndef __BINFILE_H
#define __BINFILE_H

#include "ptypes.h"

typedef intm binfilepos;

class binfile
{
public:
  enum
  {
    modeopen=1,
    moderead=2,
    modewrite=4,
    modeseek=8,
    modeappend=16,
  };
// possible combinations:
//   0                                        null
//   moderead                                 istream
//   modewrite                                ostream
//   moderead|modewrite                       iostream
//   modeseek|moderead                        ifile
//   modeseek|moderead|modewrite              iofile
//   modeseek|moderead|modewrite|modeappend   appendable iofile

  enum ioctlenum
  {
    ioctlrtell,
    ioctlwtell,
    ioctlreof,
    ioctlweof,
    ioctlrlen,
    ioctlwunderflow,
    ioctlwunderflowclr,
    ioctlroverflow,
    ioctlroverflowclr,
    ioctlrerr,
    ioctlrerrclr,
    ioctlwerr,
    ioctlwerrclr,
    ioctlrmax,
    ioctlwmax,

    ioctlrbufset,
    ioctlrbufgetlen,
    ioctlrbufget,
    ioctlwbufset,
    ioctlwbufgetlen,
    ioctlwbufget,

    ioctlrfill,
    ioctlrfillget,

    ioctlrbo,
    ioctlrboget,
    ioctlwbo,
    ioctlwboget,
    ioctlwbfill,
    ioctlwbfillget,

    ioctlrflush,
    ioctlrflushforce,
    ioctlrcancel,
    ioctlwflush,
    ioctlwflushforce,
    ioctlwcancel,

    ioctlupdlength,

    ioctltrunc,
    ioctltruncget,

    ioctlblocking,
    ioctlblockingget,

    ioctllinger,
    ioctllingerget,

    ioctlrshutdown,
    ioctlwshutdown,
    ioctlwshutdownforce,

    ioctlrrbufset,
    ioctlrrbufgetlen,
    ioctlrwbufset,
    ioctlrwbufgetlen,
    ioctlrreof,
    ioctlrweof,

    ioctlrsetlog,

    ioctluser=4096,
  };

private:
  enum { minibuflen=8 };

  uint1 minibuf[minibuflen];
  uint1 *buffer;
  binfilepos bufmax;
  binfilepos buflen;
  binfilepos bufpos;
  binfilepos bufstart;
  intm bufdirty;

  uint1 wminibuf[minibuflen];
  uint1 *wbuffer;
  binfilepos wbufmax;
  binfilepos wbufpos;

  void reset();
  boolm invalidatebuffer(boolm force);
  boolm invalidatewbuffer(boolm force);
  boolm setbuffer(intm);
  boolm setwbuffer(intm);

  uintm getbyte();
  uintm peekbyte();
  boolm putbyte(uintm v);

  boolm bitmode;
  intm bitpos;
  uintm bitbuf;

  boolm wbitmode;
  intm wbitpos;
  uintm wbitbuf;
  intm wbitfill;

  intm readfill;

  intm readerr;
  intm writeerr;

  binfilepos filepos;
  binfilepos filewpos;
  binfilepos filelen;

  binfile *pipefile;
  boolm deletepipe;

  binfile *logfile;

  binfilepos readunlogged(void *buf, binfilepos len);

protected:
  uintm mode;

  void openmode(uintm m, binfilepos pos, binfilepos len);
  void closemode();
  void openpipe(binfile &pipe, boolm kill, uintm mumask, binfilepos pos, binfilepos wpos, binfilepos len);

  virtual errstat rawclose();
  virtual binfilepos rawread(void *, binfilepos);
  virtual binfilepos rawpeek(void *, binfilepos);
  virtual binfilepos rawwrite(const void *, binfilepos);
  virtual binfilepos rawseek(binfilepos);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  binfile();
  virtual ~binfile();

  errstat close();
  binfilepos ioctl(intm code, void *buf, binfilepos len) { return rawioctl(code, buf, len); }

  binfilepos read(void *buf, binfilepos len); // moderead
  binfilepos peek(void *buf, binfilepos len);
  binfilepos write(const void *buf, binfilepos len); // modewrite
  binfilepos seek(binfilepos pos); // modeseek
  binfilepos seekcur(binfilepos pos);
  binfilepos seekend(binfilepos pos);
  binfilepos tell() { return ioctl(ioctlrtell); }
  binfilepos length() { return ioctl(ioctlrlen); }
  boolm eof() { return ioctl(ioctlreof); }

  uintm getmode();
  intm getrbitpos();
  intm getwbitpos();

// derived methods

  binfilepos ioctl(intm code) { return ioctl(code,0,0); }
  binfilepos ioctl(intm code, binfilepos par) { return ioctl(code,0,par); }
  binfile &operator [](binfilepos p) { seek(p); return *this; }
  operator int() { return getmode(); }

  boolm eread(void *buf, binfilepos len) { return read(buf,len)==len; }
  boolm epeek(void *buf, binfilepos len) { return peek(buf,len)==len; }
  boolm ewrite(const void *buf, binfilepos len) { return write(buf,len)==len; }

  void rflushbits(intm n);
  void rsyncbyte();
  boolm peekbit();
  uintm peekbits(intm n);
  boolm getbit();
  uintm getbits(intm n);
  boolm wsyncbyte();
  boolm putbit(boolm);
  boolm putbits(uintm, intm);
};

intm geti1(binfile &f);
intm getil2(binfile &f);
intm getib2(binfile &f);
intm getil4(binfile &f);
intm getib4(binfile &f);
#ifdef INT8
intm8 getil8(binfile &f);
intm8 getib8(binfile &f);
#endif
intm getu1(binfile &f);
intm getul2(binfile &f);
intm getub2(binfile &f);
uintm getul4(binfile &f);
uintm getub4(binfile &f);
#ifdef INT8
uintm8 getul8(binfile &f);
uintm8 getub8(binfile &f);
#endif

intm peeki1(binfile &f);
intm peekil2(binfile &f);
intm peekib2(binfile &f);
intm peekil4(binfile &f);
intm peekib4(binfile &f);
#ifdef INT8
intm8 peekil8(binfile &f);
intm8 peekib8(binfile &f);
#endif
intm peeku1(binfile &f);
intm peekul2(binfile &f);
intm peekub2(binfile &f);
uintm peekul4(binfile &f);
uintm peekub4(binfile &f);
#ifdef INT8
uintm8 peekul8(binfile &f);
uintm8 peekub8(binfile &f);
#endif

boolm puti1(binfile &f, intm v);
boolm putil2(binfile &f, intm v);
boolm putib2(binfile &f, intm v);
boolm putil4(binfile &f, intm v);
boolm putib4(binfile &f, intm v);
#ifdef INT8
boolm putil8(binfile &f, int8 v);
boolm putib8(binfile &f, int8 v);
#endif

char getch(binfile &f);
char peekch(binfile &f);
boolm putch(binfile &f, char v);

boolm readstrc(binfile &f, char *buf, intm max);
boolm readstrz(binfile &f, char *buf, intm max);
boolm readline(binfile &f, char *buf, intm max, char delim);
boolm writestrz(binfile &f, const char *buf);
boolm writestr(binfile &f, const char *buf);

floatmax getf4(binfile &);
floatmax getf8(binfile &);
floatmax getf10(binfile &);
boolm putf4(binfile &, floatmax);
boolm putf8(binfile &, floatmax);
boolm putf10(binfile &, floatmax);

#endif
