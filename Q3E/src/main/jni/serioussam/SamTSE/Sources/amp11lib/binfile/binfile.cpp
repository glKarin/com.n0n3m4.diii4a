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

// binfile - class library for files/streams - base class

#include <string.h>
#include "binfile.h"

binfile::binfile()
{
  reset();
}

binfile::~binfile()
{
  close();
}

void binfile::reset()
{
  mode=0;
  filepos=0;
  filelen=0;
  pipefile=0;
}

void binfile::openpipe(binfile &pipe, boolm kill, uintm mumask, binfilepos pos, binfilepos wpos, binfilepos len)
{
  pipefile=&pipe;
  deletepipe=kill;
  if (pos!=-1)
    pipe.filepos=pos;
  if (wpos!=-1)
    pipe.filewpos=wpos;
  if (len!=-1)
    pipe.filelen=len;
  pipe.mode&=~mumask;
}

void binfile::openmode(uintm m, binfilepos pos, binfilepos len)
{
  filepos=pos;
  filelen=len;
  mode=m|modeopen;
  filewpos=0;
  buffer=0;
  wbuffer=0;
  bufmax=0;
  buflen=0;
  bufpos=0;
  bufstart=0;
  bufdirty=0;
  wbufmax=0;
  wbufpos=0;
  bitmode=0;
  bitpos=0;
  bitbuf=0;
  wbitmode=0;
  wbitpos=0;
  wbitbuf=0;
  wbitfill=0;
  readfill=-1;
  readerr=0;
  writeerr=0;
  pipefile=0;
  logfile=0;
// reset pars
}

void binfile::closemode()
{
  wsyncbyte();
  invalidatebuffer(1);
  buflen=0;
  setbuffer(0);
  if (!(mode&modeseek))
  {
    invalidatewbuffer(1);
    setwbuffer(0);
  }
  else
    rawseek(filepos);
}

binfilepos binfile::rawread(void *, binfilepos)
{
  return 0;
}

binfilepos binfile::rawpeek(void *, binfilepos)
{
  return 0;
}

binfilepos binfile::rawwrite(const void *, binfilepos)
{
  return 0;
}

binfilepos binfile::rawseek(binfilepos)
{
  return 0;
}

errstat binfile::rawclose()
{
  closemode();
  return 0;
}

// ** internal buffer methods ***********************************************

boolm binfile::invalidatebuffer(intm force)
{
  boolm ret=1;
  if (bufdirty)
  {
    rawseek(bufstart);
    binfilepos l=rawwrite(buffer,buflen);
    if (l!=buflen)
    {
      if (!force)
      {
        memmove(buffer, buffer+l, buflen-l);
        buflen-=l;
        bufpos-=l;
        bufstart+=l;
        return 0;
      }
      writeerr=1;
      ret=0;
      if (filelen==(bufstart+buflen))
      {
        filelen=bufstart+l;
        if (filepos>filelen)
          filepos=filelen;
      }
    }
    bufdirty=0;
  }
  if (!(mode&modeseek)) {
    if (force==2) {
      ret=!buflen;
    } else {
      return !buflen;
    }
  }
  bufstart=filepos;
  bufpos=0;
  buflen=0;
  return ret;
}

boolm binfile::invalidatewbuffer(intm force)
{
  if (mode&modeseek)
    return invalidatebuffer(force);
  if (!wbufpos)
    return 1;
  boolm ret;
  if (force==2)
  {
    ret=!wbufpos;
    wbufpos=0;
  }
  else
  {
    binfilepos l=rawwrite(wbuffer,wbufpos);
    ret=l==wbufpos;
    wbufpos-=force?wbufpos:l;
    memmove(wbuffer, wbuffer+l, wbufpos);
  }
  return ret;
}

boolm binfile::setbuffer(intm len)
{
  invalidatebuffer(0);
  if (buflen)
    return 0;
  if (bufmax>minibuflen)
    delete buffer;
  buffer=len?(len<=minibuflen)?minibuf:new uint1 [len]:0;
  bufmax=buffer?len:0;
  return buffer||!len;
}

boolm binfile::setwbuffer(intm len)
{
  invalidatewbuffer(0);
  if (wbufpos)
    return 0;
  if (wbufmax>minibuflen)
    delete wbuffer;
  wbuffer=len?(len<=minibuflen)?wminibuf:new uint1 [len]:0;
  wbufmax=wbuffer?len:0;
  return wbuffer||!len;
}

// ** high level io *********************************************************

errstat binfile::close()
{
  if (pipefile)
  {
    errstat s=pipefile->close();
    if (s<0)
      return s;
    if (deletepipe)
      delete pipefile;
    pipefile=0;
    return 0;
  }

  if (!mode)
    return 0;
  intm r=rawclose();
  if (r>=0)
    reset();
  return r;
}

binfilepos binfile::rawioctl(intm code, void *buf, binfilepos len)
{
  if (pipefile)
    return pipefile->ioctl(code,buf,len);
  binfilepos ret=0;
  switch (code)
  {
  case ioctlrtell: return filepos;
  case ioctlrlen: return filelen;
  case ioctlwtell: return (mode&modeseek)?filepos:filewpos;
  case ioctlrfill: ret=readfill; readfill=len; break;
  case ioctlrfillget: return readfill;
  case ioctlrbo: ret=bitmode; bitmode=len?1:0; break;
  case ioctlrboget: return bitmode;
  case ioctlwbo: ret=((mode&modeseek)?bitmode:wbitmode); ((mode&modeseek)?bitmode:wbitmode)=len?1:0; break;
  case ioctlwboget: return ((mode&modeseek)?bitmode:wbitmode);
  case ioctlwbfill: ret=wbitfill; wbitfill=len?-1:0; break;
  case ioctlwbfillget: return wbitfill;
  case ioctlrerr: return readerr;
  case ioctlrerrclr: ret=readerr; readerr=0; break;
  case ioctlwerr: return writeerr;
  case ioctlwerrclr: ret=writeerr; writeerr=0; break;
  case ioctlrbufset: return setbuffer(len);
  case ioctlrbufgetlen: return bufmax;
  case ioctlrbufget: return buflen-bufpos;
  case ioctlwbufset: return (mode&modeseek)?setbuffer(len):setwbuffer(len);
  case ioctlwbufgetlen: return (mode&modeseek)?bufmax:wbufmax;
  case ioctlwbufget: return (mode&modeseek)?bufpos:wbufpos;
  case ioctlrflush: return invalidatebuffer(0);
  case ioctlrflushforce: return invalidatebuffer(1);
  case ioctlrcancel: return invalidatebuffer(2);
  case ioctlwflush: return invalidatewbuffer(0);
  case ioctlwflushforce: return invalidatewbuffer(1);
  case ioctlwcancel: return invalidatewbuffer(2);
  case ioctlrmax: return filelen-filepos;
  case ioctlwmax: return filelen-filepos; // modeappend?
  case ioctlreof:
    if (!(mode&moderead))
      return 1;
    if (ioctl(ioctlrbufget, 0, 0))
      return 0;
    return ioctl(ioctlrreof, 0, 0);
  case ioctlweof:
    if (!(mode&modewrite))
      return 1;
    if (mode&modeseek)
      return (filelen==filepos)&&!(mode&modeappend);
    return ioctl(ioctlrweof, 0, 0);
  case ioctlrreof: return filelen==filepos;
  case ioctlrweof: return 0;
  case ioctlrshutdown:
    if (!(mode&moderead))
      return 1;
    rsyncbyte();
    if (!ioctl(ioctlrcancel))
      return 0;
    if (!(mode&modeseek))
      mode&=~moderead;
    return 1;
  case ioctlwshutdown:
    if (!(mode&modewrite))
      return 1;
    if (!wsyncbyte())
      return 0;
    if (!ioctl(ioctlwflush))
      return 0;
    mode&=~modewrite;
    return 1;
  case ioctlwshutdownforce:
    if (!(mode&modewrite))
      return 1;
    wsyncbyte();
    ioctl(ioctlwflushforce);
    return 1;
  case ioctlrsetlog:
    logfile=(binfile*)buf;
    return 0;
  }
  return ret;
}

binfilepos binfile::readunlogged(void *buf, binfilepos len)
{
  // reading past end of file????

  binfilepos l1,l2;
  if (!buffer)
  {
    l2=rawread(buf,len);
    filepos+=l2;
    if (l2!=len)
    {
      readerr=1;
      if ((readfill!=-1)&&buf)
        memset((int1*)buf+l2, readfill, len-l2);
    }
    return l2;
  }
  l1=len;
  if (l1>(buflen-bufpos))
  {
    if (buflen<bufmax)
    {
      if (bufdirty)
        rawseek(bufstart+buflen);
      buflen+=rawread(buffer+buflen, bufmax-buflen);
    }
    if (l1>(buflen-bufpos))
      l1=buflen-bufpos;
  }
  if (l1)
  {
    if (buf)
      memcpy(buf, buffer+bufpos, l1);
    bufpos+=l1;
    filepos+=l1;
  }
  if (l1==len)
    return l1;
  invalidatebuffer(0);
  if (buf)
    *(int1**)&buf+=l1;
  len-=l1;
  if (bufpos)
    l2=0;
  else
  if (len>=bufmax)
  {
    l2=rawread(buf, len);
    bufstart+=l2;
  }
  else
  {
    l2=len;
    buflen+=rawread(buffer+bufpos, bufmax-buflen);
    if (l2>(buflen-bufpos))
      l2=buflen-bufpos;
    if (buf)
      memcpy(buf, buffer+bufpos, l2);
    bufpos+=l2;
  }
  if (l2!=len)
  {
    readerr=1;
    if ((readfill!=-1)&&buf)
      memset((int1*)buf+l2, readfill, len-l2);
  }
  filepos+=l2;
  return l1+l2;
}

binfilepos binfile::read(void *buf, binfilepos len)
{
  if (pipefile)
    return pipefile->read(buf,len);
  if (!(mode&moderead)||(len<=0))
    return 0;
  if (buf)
  {
    binfilepos l=readunlogged(buf, len);
    if (logfile)
      logfile->write(buf, l);
    return l;
  }
  else
  {
    binfilepos skipped=0;
    const intm skipbuflen=256;
    int1 skipbuf[skipbuflen];
    while (skipped!=len)
    {
      binfilepos l=len-skipped;
      if (l>skipbuflen)
        l=skipbuflen;
      l=readunlogged(skipbuf, l);
      if (logfile)
        logfile->write(buf, l);
      skipped+=l;
      if (l!=skipbuflen)
        break;
    }
    return skipped;
  }
}

binfilepos binfile::peek(void *buf, binfilepos len)
{
  if (pipefile)
    return pipefile->peek(buf,len);
  if (!(mode&moderead)||(len<=0))
    return 0;
  if (mode&modeseek)
  {
    // insert!!
    len=readunlogged(buf,len);
    seekcur(-len);
    return len;
  }
  if (!buffer)
  {
    binfilepos l=rawpeek(buf, len);
    if (readfill!=-1)
      memset((int1*)buf+l, readfill, len-l);
    if (l!=len)
      readerr=1;
    return l;
  }
  else
  {
    if (len>(buflen-bufpos))
    {
      memmove(buffer,buffer+bufpos,buflen-bufpos);
      buflen-=bufpos;
      bufpos=0;
      buflen+=rawread(buffer+buflen,bufmax-buflen);
      if (len>buflen)
      {
        readerr=1;
        if (readfill!=-1)
          memset((int1*)buf+buflen, readfill, len-buflen);
        len=buflen;
      }
    }
    memcpy(buf,buffer+bufpos,len);
    return len;
  }
}

binfilepos binfile::write(const void *buf, binfilepos len)
{
  if (pipefile)
    return pipefile->write(buf,len);
  if (!(mode&modewrite)||(len<=0))
    return 0;
  binfilepos l1,l2;
  if ((!(mode&modeseek)&&!wbuffer)||((mode&modeseek)&&!buffer))
  {
    l2=rawwrite(buf,len);
    if (l2!=len)
      writeerr=1;
    if (mode&modeseek)
    {
      filepos+=l2;
      if (filelen<filepos)
        filelen=filepos;
    }
    else
      filewpos+=l2;
    return l2;
  }
  if (!(mode&modeseek))
  {
    l1=len;
    if (l1>=(wbufmax-wbufpos))
      l1=wbufmax-wbufpos;
    memcpy(wbuffer+wbufpos, buf, l1);
    wbufpos+=l1;
    filewpos+=l1;
    if (l1==len)
      return l1;
    invalidatewbuffer(0);
    len-=l1;
    *(int1**)&buf+=l1;
    if (!wbufpos&&(len>wbufmax))
      l2=rawwrite(buf,len);
    else
    {
      l2=len;
      if (l2>(wbufmax-wbufpos))
        l2=wbufmax-wbufpos;
      memcpy(wbuffer+wbufpos, buf, l2);
      wbufpos+=l2;
    }
    if (l2!=len)
      writeerr=1;
    filewpos+=l2;
    return l1+l2;
  }
  l1=len;
  if (l1>=bufmax) {
    l1=0;
  }
  if (l1>(bufmax-bufpos)) {
    if (!bufdirty) {
      invalidatebuffer(0);
    } else {
      l1=bufmax-bufpos;
    }
  }
  if (l1)
  {
    memcpy(buffer+bufpos, buf, l1);
    bufpos+=l1;
    if (buflen<bufpos)
      buflen=bufpos;
    bufdirty=1;
    filepos+=l1;
    if (filelen<filepos)
      filelen=filepos;
  }
  if (l1==len)
    return l1;
  invalidatebuffer(0);
  *(int1**)&buf+=l1;
  len-=l1;
  if (!bufpos&&(len>=bufmax))
  {
    l2=rawwrite(buf,len);
    bufstart+=l2;
  }
  else
  {
    l2=len;
    memcpy(buffer+bufpos, buf, l2);
    bufdirty=1;
    bufpos+=l2;
    if (bufpos>buflen)
      buflen=bufpos;
  }
  if (l2!=len)
    writeerr=1;
  filepos+=l2;
  if (filelen<filepos)
    filelen=filepos;
  return l1+l2;
}

binfilepos binfile::seek(binfilepos p)
{
  if (pipefile)
    return pipefile->seek(p);
  if (!(mode&modeseek)||(filepos==p))
    return filepos;
  if (p<0)
    p=0;
  if (!buffer)
  {
    filepos=rawseek(p);
    if (logfile)
      logfile->seek(filepos);
    return filepos;
  }
  if ((p>=bufstart)&&(p<(bufstart+buflen)))
  {
    bufpos=p-bufstart;
    filepos=p;
    if (logfile)
      logfile->seek(filepos);
    return filepos;
  }
  invalidatebuffer(1);
  if (p>filelen)
    p=filelen;
  filepos=rawseek(p);
  bufstart=filepos;
  if (logfile)
    logfile->seek(filepos);
  return filepos;
}

binfilepos binfile::seekcur(binfilepos p)
{
  if (pipefile)
    return pipefile->seekcur(p);
  return seek(filepos+p);
}

binfilepos binfile::seekend(binfilepos p)
{
  if (pipefile)
    return pipefile->seekend(p);
  return seek(filelen+p);
}

uintm binfile::getmode()
{
  if (pipefile)
    return pipefile->getmode();
  return mode;
}

// ** bit streams ***********************************************************

uintm binfile::getbyte()
{
  uint1 v=0;
  read(&v,1);
  return v;
}

uintm binfile::peekbyte()
{
  uint1 v=0;
  peek(&v,1);
  return v;
}

boolm binfile::putbyte(uintm v)
{
  uint1 v2=(uint1)v;
  return ewrite(&v2, 1);
}

void binfile::rflushbits(intm n)
{
  if (pipefile)
  {
    pipefile->rflushbits(n);
    return;
  }
  if (mode&modeseek)
  {
    bitpos+=n;
    seekcur(bitpos>>3);
    bitpos&=7;
  }
  else
  {
    intm s=((bitpos+n)>>3);
    if (!bitpos)
      s++;
    bitpos=(bitpos+n)&7;
    if (s)
    {
      read(0,s-1);
      if (bitpos)
        bitbuf=getbyte();
    }
  }
}

void binfile::rsyncbyte()
{
  if (pipefile)
  {
    pipefile->rsyncbyte();
    return;
  }
  rflushbits((-bitpos)&7);
}

boolm binfile::peekbit()
{
  if (pipefile)
    return pipefile->peekbit();
  boolm r;
  if ((mode&modeseek)||!bitpos)
    r=peekbyte();
  else
    r=bitbuf;
  if (!bitmode)
    return (r>>bitpos)&1;
  else
    return (r>>(7-bitpos))&1;
}

uintm binfile::peekbits(intm n)
{
  if (pipefile)
    return pipefile->peekbits(n);
  uintl4 p;
  if ((mode&modeseek)||!bitpos)
  {
    p=0;
    peek(&p, (bitpos+n+7)>>3);
  }
  else
  {
    p=bitbuf;
    peek(((int1*)&p)+1, (bitpos+n-1)>>3);
  }
  if (!bitmode)
    return (p>>bitpos)&((1<<n)-1);
  else
    return ((*(uintb4*)&p)>>(32-bitpos-n))&((1<<n)-1);
}

boolm binfile::getbit()
{
  if (pipefile)
    return pipefile->getbit();
  boolm r;
  if (mode&modeseek)
  {
    if (bitpos!=7)
      r=peekbyte();
    else
      r=getbyte();
  }
  else
  {
    if (!bitpos)
      bitbuf=getbyte();
    r=bitbuf;
  }
  if (!bitmode)
    r>>=bitpos;
  else
    r>>=7-bitpos;
  bitpos=(bitpos+1)&7;
  return r&1;
}

uintm binfile::getbits(intm n)
{
  if (pipefile)
    return pipefile->getbits(n);
  uintl4 p;
  if (mode&modeseek)
  {
    intm s;
    p=0;
    s=eread(&p, (bitpos+n+7)>>3);
    if (((bitpos+n)&7)&&s)
      seekcur(-1);
  }
  else
  {
    p=bitbuf;
    if (bitpos)
      read(((int1*)&p)+1, (bitpos+n-1)>>3);
    else
      read(&p, (bitpos+n+7)>>3);
    bitbuf=p>>((bitpos+n)>>3);
  }
  uintm r;
  if (!bitmode)
    r=p>>bitpos;
  else
    r=(*(uintb4*)&p)>>(32-bitpos-n);
  bitpos=(bitpos+n)&7;
  return r&((1<<n)-1);
}

boolm binfile::putbit(boolm b)
{
  if (pipefile)
    return pipefile->putbit(b);
  if (!(mode&modewrite))
    return 0;
  boolm r;
  b=b?1:0;
  if (mode&modeseek)
  {
    intm rf=readfill;
    readfill=wbitfill&0xFF;
    uint1 v=(uint1)peekbyte();
    readfill=rf;
    if (!bitmode)
      v=(v&~(1<<bitpos))|(b<<bitpos);
    else
      v=(v&~(1<<(7-bitpos)))|(b<<(7-bitpos));
    r=putbyte(v);
    bitpos=(bitpos+1)&7;
    if (bitpos&&r)
      seekcur(-1);
  }
  else
  {
    if (!wbitmode)
      wbitbuf|=b<<wbitpos;
    else
      wbitbuf|=b<<(7-wbitpos);
    wbitpos++;
    if (wbitpos==8)
    {
      r=putbyte(wbitbuf);
      wbitbuf=0;
      wbitpos=0;
    }
    else
      r=1;
  }
  return r;
}

boolm binfile::putbits(uintm b, intm n)
{
  if (pipefile)
    return pipefile->putbits(b,n);
  if (!(mode&modewrite))
    return 0;
  boolm r;
  b&=(1<<n)-1;
  if (mode&modeseek)
  {
    uintl4 p;
    intm rf=readfill;
    readfill=wbitfill&0xFF;
    peek(&p, (bitpos+n+7)>>3);
    readfill=rf;
    if (!bitmode)
      p=(p&~(((1<<n)-1)<<bitpos))|(b<<bitpos);
    else
      (*(uintb4*)&p)=((*(uintb4*)&p)&~(((1<<n)-1)<<(32-bitpos-n)))|(b<<(32-bitpos-n));
    r=ewrite(&p, (bitpos+n+7)>>3);
    bitpos=(bitpos+n)&7;
    if (bitpos&&r)
      seekcur(-1);
  }
  else
  {
    uintl4 p;
    if (!wbitmode)
      p=b<<wbitpos;
    else
    {
      uintb4 t=b<<(32-wbitpos-n);
      p=*(uintl4*)&t;
    }
    p=p|wbitbuf;
    wbitpos+=n;
    r=ewrite(&p, wbitpos>>3);
    wbitbuf=p>>(wbitpos&~7);
    wbitpos&=7;
  }
  return r;
}

boolm binfile::wsyncbyte()
{
  if (pipefile)
    return pipefile->wsyncbyte();
  if (mode&modeseek)
  {
    if (!bitpos)
      return 1;
    seekcur(1);
    bitpos=0;
    return 1;
  }
  else
  {
    if (!wbitpos)
      return 1;
    if (!wbitmode)
      return putbits(wbitfill, 8-wbitpos);
    else
      return putbits(wbitfill, 8-wbitpos);
  }
}

intm binfile::getrbitpos()
{
  if (pipefile)
    return pipefile->getrbitpos();
  return bitpos;
}

intm binfile::getwbitpos()
{
  if (pipefile)
    return pipefile->getwbitpos();
  return (mode&modeseek)?bitpos:wbitpos;
}


// ** common types io *******************************************************

intm geti1(binfile &f) { int1 v=0; f.read(&v,1); return v; }
intm getil2(binfile &f) { intl2 v=0; f.read(&v,2); return v; }
intm getib2(binfile &f) { intb2 v=0; f.read(&v,2); return v; }
intm getil4(binfile &f) { intl4 v=0; f.read(&v,4); return v; }
intm getib4(binfile &f) { intb4 v=0; f.read(&v,4); return v; }
#ifdef INT8
intm8 getil8(binfile &f) { intl8 v=0; f.read(&v,8); return v; }
intm8 getib8(binfile &f) { intb8 v=0; f.read(&v,8); return v; }
#endif
intm getu1(binfile &f) { uint1 v=0; f.read(&v,1); return v; }
intm getul2(binfile &f) { uintl2 v=0; f.read(&v,2); return v; }
intm getub2(binfile &f) { uintb2 v=0; f.read(&v,2); return v; }
uintm getul4(binfile &f) { uintl4 v=0; f.read(&v,4); return v; }
uintm getub4(binfile &f) { uintb4 v=0; f.read(&v,4); return v; }
#ifdef INT8
uintm8 getul8(binfile &f) { uintl8 v=0; f.read(&v,8); return v; }
uintm8 getub8(binfile &f) { uintb8 v=0; f.read(&v,8); return v; }
#endif
intm peeki1(binfile &f) { int1 v=0; f.peek(&v,1); return v; }
intm peekil2(binfile &f) { intl2 v=0; f.peek(&v,2); return v; }
intm peekib2(binfile &f) { intb2 v=0; f.peek(&v,2); return v; }
intm peekil4(binfile &f) { intl4 v=0; f.peek(&v,4); return v; }
intm peekib4(binfile &f) { intb4 v=0; f.peek(&v,4); return v; }
#ifdef INT8
intm8 peekil8(binfile &f) { intl8 v=0; f.peek(&v,8); return v; }
intm8 peekib8(binfile &f) { intb8 v=0; f.peek(&v,8); return v; }
#endif
intm peeku1(binfile &f) { uint1 v=0; f.peek(&v,1); return v; }
intm peekul2(binfile &f) { uintl2 v=0; f.peek(&v,2); return v; }
intm peekub2(binfile &f) { uintb2 v=0; f.peek(&v,2); return v; }
uintm peekul4(binfile &f) { uintl4 v=0; f.peek(&v,4); return v; }
uintm peekub4(binfile &f) { uintb4 v=0; f.peek(&v,4); return v; }
#ifdef INT8
uintm8 peekul8(binfile &f) { uintl8 v=0; f.peek(&v,8); return v; }
uintm8 peekub8(binfile &f) { uintb8 v=0; f.peek(&v,8); return v; }
#endif
boolm puti1(binfile &f, intm v) { int1 v2=v; return f.ewrite(&v2, 1); }
boolm putil2(binfile &f, intm v) { intl2 v2=v; return f.ewrite(&v2, 2); }
boolm putib2(binfile &f, intm v) { intb2 v2=v; return f.ewrite(&v2, 2); }
boolm putil4(binfile &f, intm v) { intl4 v2=v; return f.ewrite(&v2, 4); }
boolm putib4(binfile &f, intm v) { intb4 v2=v; return f.ewrite(&v2, 4); }
#ifdef INT8
boolm putil8(binfile &f, intm8 v) { intl8 v2=v; return f.ewrite(&v2, 8); }
boolm putib8(binfile &f, intm8 v) { intb8 v2=v; return f.ewrite(&v2, 8); }
#endif
char getch(binfile &f) { int1 v=0; f.read(&v,1); return v; }
char peekch(binfile &f) { int1 v=0; f.peek(&v,1); return v; }
boolm putch(binfile &f, char v) { int1 v2=v; return f.ewrite(&v2, 1); }

boolm readline(binfile &f, char *buf, intm max, char delim)
{
  if (max<=0)
    return 0;
  char *bp=buf;
  while (1)
  {
    char c=getch(f);
    *bp=0;
    if (f.ioctl(f.ioctlrerr))
      break;
    if (c==delim)
      return 1;
    if (max==1)
    {
      while ((getch(f)!=delim)&&!f.ioctl(f.ioctlrerr));
      break;
    }
    max--;
    *bp++=c;
  }
  return 0;
}

boolm readstrz(binfile &f, char *buf, intm max)
{
  if (max<=0)
    return 0;
  char *bp=buf;
  while (1)
  {
    char c=getch(f);
    *bp=0;
    if (f.ioctl(f.ioctlrerr))
      break;
    if (!c)
      return 1;
    if (max==1)
    {
      while (getch(f)&&!f.ioctl(f.ioctlrerr));
      break;
    }
    max--;
    *bp++=c;
  }
  return 0;
}

boolm writestrz(binfile &f, const char *buf)
{
  return f.ewrite(buf, strlen(buf)+1);
}

boolm writestr(binfile &f, const char *buf)
{
  return f.ewrite(buf, strlen(buf));
}
