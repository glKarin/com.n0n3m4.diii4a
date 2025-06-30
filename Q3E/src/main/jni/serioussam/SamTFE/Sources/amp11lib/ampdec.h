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

#ifndef __MPDECODE_H
#define __MPDECODE_H

#include "binfile/binfile.h"

#ifdef __WATCOMC__
void __fistp(long &i, long double x);
#pragma aux __fistp parm [eax] [8087] = "fistp dword ptr [eax]"
unsigned int __swap32(unsigned int);
#pragma aux __swap32 parm [eax] value [eax] = "bswap eax"
#endif
#ifdef GNUCI486
static inline unsigned int __swap32(unsigned int x)
{
  asm("bswapl %0\n":"=r"(x):"0"(x));
  return x;
}
#endif

class ampegdecoder : public binfile
{
private:
// bitstream
  unsigned char *bitbuf;
  int *bitbufpos;

  int mpgetbit()
  {
    int val=(bitbuf[*bitbufpos>>3]>>(7-(*bitbufpos&7)))&1;
    (*bitbufpos)++;
    return val;
  }
  unsigned int convbe32(unsigned int x)
  {
  #ifdef BIGENDIAN
    return x;
  #elif defined(__WATCOMC__)||defined(GNUCI486)
    return __swap32(x);
  #else
    return ((x<<24)&0xFF000000)|((x<<8)&0xFF0000)|((x>>8)&0xFF00)|((x>>24)&0xFF);
  #endif
  }
  long mpgetbits(int n)
  {
    if (!n)
      return 0;
#ifdef FASTBITS
    unsigned long val=(convbe32(*(unsigned long*)&bitbuf[*bitbufpos>>3])>>(32-(*bitbufpos&7)-n))&((1<<n)-1);
#else
    unsigned long val=(((((unsigned char)(bitbuf[(*bitbufpos>>3)+0]))<<24)|(((unsigned char)(bitbuf[(*bitbufpos>>3)+1]))<<16)|(((unsigned char)(bitbuf[(*bitbufpos>>3)+2]))<<8))>>(32-(*bitbufpos&7)-n))&((1<<n)-1);
#endif
    *bitbufpos+=n;
    return val;
  }
  void getbytes(void *buf2, int n);

// mainbitstream
  enum
  {
    mainbufmin=2048,
    mainbufmax=16384
  };
  binfile *file;
  unsigned char mainbuf[mainbufmax];
  int mainbufsize;
  int mainbuflow;
  int mainbuflen;
  int mainbufpos;

  void setbufsize(int size, int min);
  int openbits();
  void refillbits();
  int sync7FF();

// decoder
  static int lsftab[4];
  static int freqtab[4];
  static int ratetab[2][3][16];

  int hdrlay;
  int hdrcrc;
  int hdrbitrate;
  int hdrfreq;
  int hdrpadding;
  int hdrmode;
  int hdrmodeext;
  int hdrlsf;

  int init;
  int orglay;
  int orgfreq;
  int orglsf;
  int orgstereo;

  int stream;
  int slotsize;
  int nslots;
  int fslots;
  int slotdiv;
  int seekinitframes;
  int seekmode;
  char framebuf[2*32*36*4];
  int curframe;
  int framepos;
  int nframes;
  int framesize;
  int atend;
  float fraction[2][36][32];

  int decodehdr(int);
  int decode(void *);

// synth
  static float dwin[1024];
  static float dwin2[512];
  static float dwin4[256];
  static float sectab[32];

  int synbufoffset;
  float synbuf[2048];

  inline float muladd16a(float *a, float *b);
  inline float muladd16b(float *a, float *b);

  int convle16(int x)
  {
  #ifdef BIGENDIAN
    return ((x>>8)&0xFF)|((x<<8)&0xFF00);
  #else
    return x;
  #endif
  }
  int cliptoshort(float x)
  {
  #ifdef __WATCOMC__
    long foo;
    __fistp(foo,x);
  #else
    int foo=(int)x;
  #endif
    return (foo<-32768)?convle16(-32768):(foo>32767)?convle16(32767):convle16(foo);
  }
  static void fdctb32(float *out1, float *out2, float *in);
  static void fdctb16(float *out1, float *out2, float *in);
  static void fdctb8(float *out1, float *out2, float *in);

  int tomono;
  int tostereo;
  int dstchan;
  int ratereduce;
  int usevoltab;
  int srcchan;
  int dctstereo;
  int samplesize;
  float stereotab[3][3];
  float equal[32];
  int equalon;
  float volume;

  int opensynth();
  void synth(void *, float (*)[32], float (*)[32]);
  void resetsynth();

  void setstereo(const float *);
  void setvol(float);
  void setequal(const float *);

//layer 1/2
  struct sballoc
  {
    unsigned int steps;
    unsigned int bits;
    int scaleadd;
    float scale;
  };

  static sballoc alloc[17];
  static sballoc *atab0[];
  static sballoc *atab1[];
  static sballoc *atab2[];
  static sballoc *atab3[];
  static sballoc *atab4[];
  static sballoc *atab5[];
  static const int alloctablens[5];
  static sballoc **alloctabs[3][32];
  static const int alloctabbits[3][32];
  static float multiple[64];
  static float rangefac[16];

  float scale1[2][32];
  unsigned int bitalloc1[2][32];
  float scale2[2][3][32];
  int scfsi[2][32];
  sballoc *bitalloc2[2][32];

  static void init12();
  void openlayer1(int);
  void decode1();
  void openlayer2(int);
  void decode2();
//  void decode2mc();

//layer 3
  struct grsistruct
  {
    int gr;
    int ch;

    int blocktype;
    int mixedblock;

    int grstart;
    int tabsel[4];
    int regionend[4];
    int grend;

    int subblockgain[3];
    int preflag;
    int sfshift;
    int globalgain;

    int sfcompress;
    int sfsi[4];

    int ktabsel;
  };

  static int htab00[],htab01[],htab02[],htab03[],htab04[],htab05[],htab06[],htab07[];
  static int htab08[],htab09[],htab10[],htab11[],htab12[],htab13[],htab14[],htab15[];
  static int htab16[],htab24[];
  static int htaba[],htabb[];
  static int *htabs[34];
  static int htablinbits[34];
  static int sfbtab[7][3][5];
  static int slentab[2][16];
  static int sfbands[3][3][14];
  static int sfbandl[3][3][23];
  static float citab[8];
  static float csatab[8][2];
  static float sqrt05;
  static float ktab[3][32][2];
  static float sec12[3];
  static float sec24wins[6];
  static float cos6[3];
  static float sec36[9];
  static float sec72winl[18];
  static float cos18[9];
  static float winsqs[3];
  static float winlql[9];
  static float winsql[12];
  static int pretab[22];
  static float pow2tab[65];
  static float pow43tab[8207];
  static float ggaintab[256];

  int rotab[3][576];
  float l3equall[576];
  float l3equals[192];
  int l3equalon;

  float prevblck[2][32][18];
  unsigned char huffbuf[4096];
  int huffbit;
  int huffoffset;

  int ispos[576];
  int scalefac0[2][39];
  float xr0[2][576];

  static void init3();
  inline int huffmandecoder(int *valt);
  static void imdct(float *out, float *in, float *prev, int blocktype);
  static void fdctd6(float *out, float *in);
  static void fdctd18(float *out, float *in);
  void readgrsi(grsistruct &si, int &bitpos);
  void jointstereo(grsistruct &si, float (*xr)[576], int *scalefacl);
  void readhuffman(grsistruct &si, float *xr);
  void doscale(grsistruct &si, float *xr, int *scalefacl);
  void readscalefac(grsistruct &si, int *scalefacl);
  void hybrid(grsistruct &si, float (*hout)[32], float (*prev)[18], float *xr);
  void readsfsi(grsistruct &si);
  void readmain(grsistruct (*si)[2]);

  void openlayer3(int);
  void decode3();
  void seekinit3(int);
  void setl3equal(const float *);

protected:
  virtual errstat rawclose();
  virtual binfilepos rawseek(binfilepos pos);
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawpeek(void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  enum
  {
    ioctlsetvol=ioctluser, ioctlsetstereo, ioctlsetequal32, ioctlsetequal576,
    ioctlseekmode, ioctlseekmodeget
  };

  enum { seekmodeexact=0, seekmoderelaxed=1 };

  int open(binfile &in, int &freq, int &stereo, int fmt, int down, int chn);
  static int getheader(binfile &in, int &layer, int &lsf, int &freq, int &stereo, int &rate);

  ampegdecoder();
  virtual ~ampegdecoder();
};

#endif
