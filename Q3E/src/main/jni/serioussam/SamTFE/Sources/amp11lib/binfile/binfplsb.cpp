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

// binfile - class library for files/streams - DOS/SB16 sound playback

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "binfplsb.h"

static int dmaCh;
static int dmaLen;
static int dmaAddr;
static unsigned char *dmaPort;
static unsigned char dmaPorts[8][6]=
  {{0x00, 0x01, 0x0A, 0x0B, 0x0C, 0x87},
   {0x02, 0x03, 0x0A, 0x0B, 0x0C, 0x83},
   {0x04, 0x05, 0x0A, 0x0B, 0x0C, 0x81},
   {0x06, 0x07, 0x0A, 0x0B, 0x0C, 0x82},
   {0xC0, 0xC2, 0xD4, 0xD6, 0xD8, 0x8F},
   {0xC4, 0xC6, 0xD4, 0xD6, 0xD8, 0x8B},
   {0xC8, 0xCA, 0xD4, 0xD6, 0xD8, 0x89},
   {0xCC, 0xCE, 0xD4, 0xD6, 0xD8, 0x8A}};
static int irqNum;
static int irqIntNum;
static int irqPort;
static int irqOldMask;
static void (*irqRoutine)();

#ifdef __DOS4G__

#include <i86.h>
static int dmaSel;
int _disableint();
#pragma aux _disableint value [eax] = "pushf" "pop eax" "cli"
void _restoreint(int);
#pragma aux _restoreint parm [eax] = "push eax" "popf"
static void far *irqOldInt;
static char __far *irqStack;
static int irqStackSize;
static int irqStackUsed;
static void __far *irqOldSSESP;

static int dmaDosMalloc(int len)
{
  len=(len+15)>>4;
  REGS r;
  r.w.ax=0x100;
  r.w.bx=len;
  int386(0x31, &r, &r);
  if (r.x.cflag)
    return 0;
  dmaSel=r.w.dx;
  return r.w.ax<<4;
}

static void dmaDosFree()
{
  REGS r;
  r.w.ax=0x101;
  r.w.dx=dmaSel;
  int386(0x31, &r, &r);
}

static void dmaPut(int pos, const void *src, int len)
{
  memcpy((char*)dmaAddr+pos,src,len);
}

static void dmaSet(int pos, int c, int len)
{
  memset((char*)dmaAddr+pos,c,len);
}

static void far *irqGetVect(int intno)
{
  REGS r;
  SREGS sr;
  r.h.ah=0x35;
  r.h.al=intno;
  sr.ds=sr.es=0;
  int386x(0x21, &r, &r, &sr);
  return MK_FP(sr.es, r.x.ebx);
}

static void irqSetVect(int intno, void far *vect)
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

void stackcall(void *);
#pragma aux stackcall parm [eax] = \
  "mov word ptr irqOldSSESP+4,ss" \
  "mov dword ptr irqOldSSESP+0,esp" \
  "lss esp,irqStack" \
  "sti" \
  "call eax" \
  "cli" \
  "lss esp,irqOldSSESP"

void loades();
#pragma aux loades = "push ds" "pop es"

static void __interrupt irqInt()
{
  loades();

  if (!irqStackUsed)
  {
    irqStackUsed++;
    stackcall(irqRoutine);
    irqStackUsed--;
  }
  if (irqNum&8)
    outp(0xA0,0x20);
  outp(0x20,0x20);
}

static int irqSetInt()
{
  irqStackSize=512;
  irqStack=new char [irqStackSize];
  if (!irqStack)
    return 0;
  irqStack+=irqStackSize;
  irqOldInt=irqGetVect(irqIntNum);
  irqSetVect(irqIntNum, irqInt);
  return 1;
}

static void irqResetInt()
{
  irqSetVect(irqIntNum, irqOldInt);
  delete (char near *)(irqStack-irqStackSize);
}

#else

#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#define inp inportb
#define outp outportb

static _go32_dpmi_seginfo dmaSel;
static _go32_dpmi_seginfo irqOldInt;
static _go32_dpmi_seginfo irqNewInt;

static inline int _disableint()
{
  int res;
  asm
  (
    "pushf\n"
    "pop %0\n"
    "cli"
    : "=r"(res)
  );
  return res;
}

static inline void _restoreint(int a)
{
  asm
  (
    "push %0\n"
    "popf"
    : : "r"(a)
  );
}

static int dmaDosMalloc(int len)
{
  dmaSel.size=(len+15)>>4;
  if (_go32_dpmi_allocate_dos_memory(&dmaSel))
    return 0;
  return dmaSel.rm_segment<<4;
}

static void dmaDosFree()
{
  _go32_dpmi_free_dos_memory(&dmaSel);
}

static void dmaPut(int pos, const void *src, int len)
{
  dosmemput(src, len, dmaAddr+pos);
}

static void dmaSet(int pos, int c, int len)
{
  char cc=c;
  int i;
  for (i=0; i<len; i++)
    dmaPut(pos+i, &cc, 1);
}

static void irqInt()
{
  irqRoutine();
  if (irqNum&8)
    outp(0xA0,0x20);
  outp(0x20,0x20);
}

static int irqSetInt()
{
  _go32_dpmi_get_protected_mode_interrupt_vector(irqIntNum, &irqOldInt);
  irqNewInt.pm_offset=(int)irqInt;
  irqNewInt.pm_selector=_go32_my_cs();
  if (_go32_dpmi_allocate_iret_wrapper(&irqNewInt))
    return 0;
  _go32_dpmi_set_protected_mode_interrupt_vector(irqIntNum, &irqNewInt);
  return 1;
}

static void irqResetInt()
{
  _go32_dpmi_set_protected_mode_interrupt_vector(irqIntNum, &irqOldInt);
  _go32_dpmi_free_iret_wrapper(&irqNewInt);
}

#endif



static void dmaStart(int ch, int pos, int buflen, int autoinit)
{
  dmaCh=ch&7;
  dmaPort=dmaPorts[dmaCh];
  int realadr=dmaAddr+pos;
  int page=(dmaAddr+pos)>>16;
  if (dmaCh&4)
  {
    realadr>>=1;
    buflen=(buflen+1)>>1;
  }
  dmaLen=buflen-1;
  int is=_disableint();
  outp(dmaPort[2],dmaCh|4);
  outp(dmaPort[3],autoinit|(ch&3));
  outp(dmaPort[4],0);
  outp(dmaPort[0],realadr);
  outp(dmaPort[0],(realadr>>8));
  outp(dmaPort[5],page);
  outp(dmaPort[5],page);
  outp(dmaPort[1],dmaLen);
  outp(dmaPort[1],(dmaLen>>8));
  outp(dmaPort[2],dmaCh&3);
  _restoreint(is);
}

static void dmaStop()
{
  outp(dmaPort[4],0);
  outp(dmaPort[2],dmaCh|4);
}

static int dmaGetBufPos()
{
  unsigned int a,b;
  int is=_disableint();
  while(1)
  {
    outp(dmaPort[4], 0xFF);
    a=inp(dmaPort[1]);
    a+=inp(dmaPort[1])<<8;
    b=inp(dmaPort[1]);
    b+=inp(dmaPort[1])<<8;
    if (abs(a-b)<=64)
      break;
  }
  _restoreint(is);
  int p=dmaLen-b;
  if (p<0)
    return 0;
  if (p>=dmaLen)
    return 0;
  if (dmaCh&4)
    p<<=1;
  return p;
}

static int dmaAlloc(int buflen)
{
  if (buflen>0x20000)
    buflen=0x20000;
  buflen=(buflen+15)&~15;
  dmaAddr=dmaDosMalloc(buflen);
  if (!dmaAddr)
    return 0;
  int a=0x10000-(dmaAddr&0xFFFF);
  if (a<buflen)
    if (a<(buflen>>1))
    {
      buflen-=a;
      dmaAddr+=a;
    }
    else
      buflen=a;
  if (buflen>0xFF00)
    buflen=0xFF00;
  return buflen;
}

static void dmaFree()
{
  dmaDosFree();
}



static int irqInit(int inum, void (*routine)())
{
  irqRoutine=routine;
  inum&=15;
  irqNum=inum;
  irqIntNum=(inum&8)?(inum+0x68):(inum+8);
  irqPort=(inum&8)?0xA1:0x21;
  irqOldMask=inp(irqPort)&(1<<(inum&7));

  if (!irqSetInt())
    return 0;

  outp(irqPort, inp(irqPort)&~(1<<(inum&7)));
  return 1;
}

static void irqClose()
{
  outp(irqPort, inp(irqPort)|irqOldMask);
  irqResetInt();
}

static int sbPort;
static int sbIRQ;
static int sbDMA;
static int sbDMA16;
static int sbBufPos;
static int sbBufLen;

static int inpSB(int p)
{
  return inp(sbPort+p);
}

static void outpSB(int p, int v)
{
  outp(sbPort+p,v);
}

static void outSB(int v)
{
  while (inpSB(0xC)&0x80);
  outpSB(0xC,v);
}

static int inSB()
{
  while (!(inpSB(0xE)&0x80));
  return inpSB(0xA);
}

static void initSB()
{
  outpSB(0x6,1);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  outpSB(0x6,0);
}

static void setrateSB16(int r)
{
  outSB(0x41);
  outSB(r>>8);
  outSB(r);
}

static void resetSB()
{
  inpSB(0xE);
}

static void resetSB16()
{
  inpSB(0xF);
}

static int testPort()
{
  initSB();
  int i;
  for (i=0; i<1000; i++)
    if (inpSB(0xE)&0x80)
      break;
  if (i==1000)
    return 0;
  outSB(0xE1);
  for (i=0; i<1000; i++)
  {
    int verhi=inSB();
    if (verhi==4)
      break;
    if (verhi!=0xAA)
      return 0;
  }
  if (i==1000)
    return 0;
  return 1;
}

static void sb16Player8()
{
  resetSB();
  outSB(0x45);
}

static void sb16Player16()
{
  outSB(0x47);
  resetSB16();
}

static int sbInit()
{
  if (sbPort)
    return 1;

  char *s=getenv("BLASTER");
  if (!s)
    return 0;
  sbIRQ=-1;
  sbDMA=-1;
  sbDMA16=-1;
  while (1)
  {
    while (*s==' ')
      s++;
    if (!*s)
      break;
    switch (*s++)
    {
    case 'a': case 'A':
      sbPort=strtoul(s, 0, 16);
      break;
    case 'i': case 'I':
      sbIRQ=strtoul(s, 0, 10);
      break;
    case 'd': case 'D':
      sbDMA=strtoul(s, 0, 10);
      break;
    case 'h': case 'H':
      sbDMA16=strtoul(s, 0, 10);
      break;
    }
    while ((*s!=' ')&&*s)
      s++;
  }

  if (!sbPort||(sbIRQ==-1)||(sbDMA16==-1)||(sbDMA==-1))
    return 0;

  if (!testPort())
    return 0;

  resetSB();
  resetSB16();

  return 1;
}

static int sbPlay(int rate, int stereo, int bit16, int len)
{
  if (sbBufLen)
    return 0;

  len=dmaAlloc(len);
  if (!len)
    return 0;
  dmaSet(0, bit16?0:0x80, len);

  initSB();
  resetSB();
  resetSB16();
  setrateSB16(rate);

  irqInit(sbIRQ, bit16?sb16Player16:sb16Player8);
  dmaStart(bit16?sbDMA16:sbDMA, 0, len, 0x58);

  outSB(bit16?0xB6:0xC6);
  outSB((stereo?0x20:0x00)|(bit16?0x10:0x00));
  outSB(0xFC);
  outSB(0xFF);

  sbBufLen=len;
  sbBufPos=0;

  return 1;
}

static void sbStop()
{
  irqClose();
  dmaStop();
  initSB();
  resetSB();
  resetSB16();
  dmaFree();
  sbBufLen=0;
}



static int sbWrite(const void *buf, int len)
{
  int l0=0;
  while (len)
  {
    int l=(dmaGetBufPos()-sbBufPos+sbBufLen)%sbBufLen;
    if (l>len)
      l=len;
    if ((sbBufPos+l)>sbBufLen)
      l=sbBufLen-sbBufPos;
    dmaPut(sbBufPos, buf, l);

    buf=(char*)buf+l;
    sbBufPos=(sbBufPos+l)%sbBufLen;
    len-=l;
    l0+=l;
  }
  return l0;
}

static void sbWait()
{
  int l0=0;
  while (1)
  {
    int l=(dmaGetBufPos()-sbBufPos+sbBufLen)%sbBufLen;
    if (l>l0)
      l0=l;
    if (l<l0)
      break;
  }
}

int sbplaybinfile::open(int rate, int stereo, int bit16, int len)
{
  close();
  if (!sbInit())
    return -1;

  if (!sbPlay(rate, stereo, bit16, len))
    return -1;

  openmode(modewrite,0,0);
  return 0;
}

errstat sbplaybinfile::rawclose()
{
  closemode();
  sbWait();
  sbStop();
  return 0;
}

binfilepos sbplaybinfile::rawwrite(const void *buf, binfilepos len)
{
  return sbWrite(buf, len);
}

sbplaybinfile::sbplaybinfile()
{
}

sbplaybinfile::~sbplaybinfile()
{
  close();
}
