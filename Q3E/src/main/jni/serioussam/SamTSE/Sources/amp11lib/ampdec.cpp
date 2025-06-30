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

// amp11 - an Audio-MPEG decoder - decoder

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ampdec.h"

int ampegdecoder::lsftab[4]={2,3,1,0};
int ampegdecoder::freqtab[4]={44100,48000,32000};

int ampegdecoder::ratetab[2][3][16]=
{
  {
    {  0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448,  0},
    {  0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384,  0},
    {  0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,  0},
  },
  {
    {  0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256,  0},
    {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,  0},
    {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,  0},
  },
};

ampegdecoder::ampegdecoder()
{
}

ampegdecoder::~ampegdecoder()
{
}

void ampegdecoder::refillbits()
{
  if (mainbufpos>(8*mainbuflen))
    mainbufpos=mainbuflen*8;
  int p=mainbufpos>>3;
  if ((mainbuflen-p)>mainbuflow)
    return;
  memmove(mainbuf, mainbuf+p, mainbuflen-p);
  mainbufpos-=p*8;
  mainbuflen-=p;
  while (1)
  {
    mainbuflen+=file->read(mainbuf+mainbuflen, mainbufsize-mainbuflen);
    if (file->ioctl(file->ioctlreof)||(mainbuflen>=mainbufmin))
      break;
  }
  memset(mainbuf+mainbuflen, 0, mainbufsize-mainbuflen);
}

void ampegdecoder::setbufsize(int size, int low)
{
  mainbufsize=(size>mainbufmax)?mainbufmax:size;
  mainbuflow=(low>(mainbufsize-16))?(mainbufsize-16):(low<mainbufmin)?mainbufmin:low;
}

int ampegdecoder::openbits()
{
  mainbufsize=mainbufmax;
  mainbuflow=(file->getmode()&file->modeseek)?mainbufmin:mainbufmax;
  mainbufpos=0;
  mainbuflen=0;
  return 1;
}

void ampegdecoder::getbytes(void *buf2, int n)
{
  memcpy(buf2, bitbuf+(*bitbufpos>>3), n);
  *bitbufpos+=n*8;
}

int ampegdecoder::sync7FF()
{
  mainbufpos=(mainbufpos+7)&~7;
  while (1)
  {
    refillbits();
    if (mainbuflen<4)
      return 0;
    while ((((mainbufpos>>3)+1)<mainbuflen)&&((mainbuf[mainbufpos>>3]!=0xFF)||(mainbuf[(mainbufpos>>3)+1]<0xE0)))
    {
      mainbufpos+=8;
    }
    while (1/*(((mainbufpos>>3)+1)<mainbuflen)&&(mainbuf[mainbufpos>>3]==0xFF)&&(mainbuf[(mainbufpos>>3)+1]>=0xE0)*/)
    {
      if (((mainbufpos>>3)+1)>=mainbuflen)
        break;
      if (mainbuf[mainbufpos>>3]!=0xFF)
        break;
      if (mainbuf[(mainbufpos>>3)+1]<0xE0)
        break;
      mainbufpos+=8;
    }
    if ((mainbufpos>>3)<mainbuflen)
    {
      mainbufpos+=3;
      refillbits();
      return 1;
    }
  }
}

int ampegdecoder::decodehdr(int init)
{
  while (1)
  {
    if (!sync7FF())
    {
      hdrbitrate=0;
      return 0;
    }

    bitbuf=mainbuf;
    bitbufpos=&mainbufpos;
    hdrlsf=lsftab[mpgetbits(2)];
    hdrlay=3-mpgetbits(2);
    hdrcrc=!mpgetbit();
    hdrbitrate = mpgetbits(4);
    hdrfreq = mpgetbits(2);
    hdrpadding = mpgetbit();
    mpgetbit(); // extension
    hdrmode = mpgetbits(2);
    hdrmodeext = mpgetbits(2);
    mpgetbit(); // copyright
    mpgetbit(); // original
    mpgetbits(2); // emphasis
    if (init)
    {
      orglsf=hdrlsf;
      orglay=hdrlay;
      orgfreq=hdrfreq;
      orgstereo=(hdrmode==1)?0:hdrmode;
    }
    if ((hdrlsf!=orglsf)||(hdrlay!=orglay)||(hdrbitrate==0)||(hdrbitrate==15)||(hdrfreq!=orgfreq)||(((hdrmode==1)?0:hdrmode)!=orgstereo))
    {
      *bitbufpos-=20;
      continue;
    }
    if (hdrcrc)
      mpgetbits(16);
    return 1;
  }
}

int ampegdecoder::getheader(binfile &in, int &layer, int &lsf, int &freq, int &stereo, int &rate)
{
  int totrate=0;
  int stream=!(in.getmode()&modeseek);
  int i;

  if (!stream)
    in.seek(0);

  int iposaftertag = 0;

  for (i=0; i<8; i++)
  {
    unsigned char hdr[4];
    int nr=in.peek(hdr, 4);

    // if there is an ID3 tag at the beginning
    if (nr==4 && hdr[0]=='I' && hdr[1]=='D' && hdr[2]=='3' && hdr[3]==3) {
      // peek the tag header
      struct ID3TagHdr {
        char acHdr[3];
        unsigned char cVersion;
        unsigned char cRevision;
        unsigned char cFlags;
        unsigned char acSize[4];  // unsynced 32-bit integer
      } taghdr;
      int nr=in.peek(&taghdr, sizeof(taghdr));
      if (nr!=sizeof(taghdr))
        return 0;

      // decode tag size
      int size;
      size = taghdr.acSize[0];
      size = (size<<7) + taghdr.acSize[1];
      size = (size<<7) + taghdr.acSize[2];
      size = (size<<7) + taghdr.acSize[3];
      // skip over the entire tag
      iposaftertag+=sizeof(taghdr)+size;
      in.seek(iposaftertag);
      // re-read the header
      nr=in.peek(hdr, 4);
    }

    if ((nr!=4)&&!i)
      return 0;
    if (hdr[0]!=0xFF)
      return 0;
    if (hdr[1]<0xE0)
      return 0;
    lsf=lsftab[((hdr[1]>>3)&3)];
    if (lsf==3)
      return 0;
    layer=3-((hdr[1]>>1)&3);
    if (layer==3)
      return 0;
    if ((lsf==2)&&(layer!=2))
      return 0;
    int pad=(hdr[2]>>1)&1;
    stereo=((hdr[3]>>6)&3)!=3;
    freq=freqtab[(hdr[2]>>2)&3]>>lsf;
    if (!freq)
      return 0;
    rate=ratetab[lsf?1:0][layer][(hdr[2]>>4)&15]*1000;
    if (!rate)
      return 0;

    if (stream||(layer!=2))
      return 1;
    totrate+=rate;
    in.seekcur((layer==0)?(((12*rate)/freq+pad)*4):(((lsf&&(layer==2))?72:144)*rate/freq+pad));
  }

  rate=totrate/i;
  in.seek(iposaftertag);
  return 1;
}

binfilepos ampegdecoder::rawseek(binfilepos pos)
{
  if (stream)
    return 0;
  if (pos<0)
    pos=0;
  if (pos>=nframes*framesize)
    pos=nframes*framesize;
  int fr=pos/framesize;
  int frpos=pos%framesize;
  if ((curframe-1)==fr)
  {
    framepos=pos%framesize;
    return (curframe-1)*framesize+framepos;
  }
  int discard=0;
  curframe=fr;
  int extra=(seekmode==seekmodeexact)?1:0;
  fr-=seekinitframes+extra;
  if (fr<0)
    discard=-fr;
  fr+=discard;
  file->seek((fr*nslots+(fr*fslots)/slotdiv)*slotsize);
  mainbufpos=0;
  mainbuflen=0;
  atend=0;
  if (orglay==2) {
    seekinit3(discard);
  }
  if (extra) {
    if (discard!=(seekinitframes+extra)) {
      ampegdecoder::decode(0);
    } else {
      resetsynth();
    }
  }
  if (frpos)
  {
    if (decode(framebuf))
    {
      curframe++;
      framepos=frpos;
    }
    else
      framepos=framesize;
  }
  else
    framepos=framesize;
  return (curframe-1)*framesize+framepos;
}

binfilepos ampegdecoder::rawpeek(void *buf, binfilepos len)
{
  if (framepos==framesize)
    if (decode(framebuf))
    {
      framepos=0;
      curframe++;
    }
  int l=framesize-framepos;
  if (l>len)
    l=len;
  memcpy(buf, framebuf+framepos, l);
  return l;
}

binfilepos ampegdecoder::rawread(void *buf, binfilepos len)
{
  long rd=0;
  while (rd<len)
  {
    if ((framepos==framesize)&&((len-rd)>=framesize))
    {
      if (!decode((short*)((char*)buf+rd))) {
        break;
      }
      curframe++;
      rd+=framesize;
      continue;
    }
    if (framepos==framesize) {
      if (decode(framebuf))
      {
        framepos=0;
        curframe++;
      } else {
        break;
      }
    }
    int l=framesize-framepos;
    if (l>(len-rd))
      l=len-rd;
    memcpy((char*)buf+rd, framebuf+framepos, l);
    framepos+=l;
    rd+=l;
  }
  return rd;
}

int ampegdecoder::decode(void *outsamp)
{
  int rate;
  if (init)
  {
    stream=!(file->getmode()&modeseek);
    int layer,lsf,freq,stereo;
    if (!getheader(*file, layer, lsf, freq, stereo, rate)) {
      return 0; 
    }
    if (stream) {
      rate=0; 
    }
    atend=0;
  }
  if (atend) {
    return 0;
  }
  if (!decodehdr(init)) {
    if (init)
     { return 0; }
    else
     { atend=1; }
  }
  if (init)
  {
    seekinitframes=0;
    if (orglay==0)
      openlayer1(rate);
    else
    if (orglay==1)
      openlayer2(rate);
    else
    if (orglay==2)
      openlayer3(rate);
    else
      return 0;
    if (rate)
      nframes=(long)floor((double)(file->length()+slotsize)*slotdiv/((nslots*slotdiv+fslots)*slotsize)+0.5);
    else
      nframes=0;
  }
  if (orglay==0)
    decode1();
  else
  if (orglay==1)
    decode2();
  else
    decode3();

  if (init)
  {
    srcchan=(orgstereo==3)?1:2;
    opensynth();
    framepos=0;
    framesize=(36*32*samplesize*dstchan)>>ratereduce;
    curframe=1;
  }

  synth(outsamp, fraction[0], fraction[1]);
  return 1;
}

int ampegdecoder::open(binfile &in, int &freq, int &stereo, int fmt, int down, int chn)
{
  close();

  init12();
  init3();

  file=&in;
  openbits();

  dstchan=chn;
  ratereduce=(down<0)?0:(down>2)?2:down;
  samplesize=fmt?2:4;

  init=1;
  if (!decode(framebuf))
    return -1;
  init=0;

  freq=freqtab[orgfreq]>>(orglsf+ratereduce);
  stereo=(dstchan==2)?1:0;

  seekmode=0;
  openmode(moderead|(stream?0:modeseek), 0, nframes*framesize);
  return 0;
}

errstat ampegdecoder::rawclose()
{
  closemode();
  return 0;
}

binfilepos ampegdecoder::rawioctl(intm code, void *buf, binfilepos len)
{
  int old;
  switch (code)
  {
  case ioctlseekmode: old=seekmode; seekmode=len?1:0; return old;
  case ioctlseekmodeget: return seekmode;
  case ioctlsetvol: setvol(buf?*(float*)buf:1); return 0;
  case ioctlsetstereo: setstereo((float*)buf); return 0;
  case ioctlsetequal32:
    if (orglay==2)
      setl3equal(0);
    setequal((float*)buf);
    return 0;
  case ioctlsetequal576:
    if (orglay==2)
    {
      setl3equal((float*)buf);
      return 0;
    }
    float eq32[32];
    int i,j;
    for (i=0; i<32; i++)
    {
      eq32[i]=0;
      for (j=0; j<18; j++)
        eq32[i]+=((float*)buf)[i*18+j];
      eq32[i]/=18;
    }
    setequal(eq32);
    return 0;
  case ioctlrreof:
    return (framepos==framesize)&&atend;
  default: return binfile::rawioctl(code,buf,len);
  }
}
