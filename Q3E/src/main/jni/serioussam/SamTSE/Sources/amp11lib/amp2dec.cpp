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

// amp11 - an Audio-MPEG decoder - layer 2 decoder

#include <math.h>
#include "ampdec.h"

ampegdecoder::sballoc ampegdecoder::alloc[17];

ampegdecoder::sballoc *ampegdecoder::atab0[]=
{
  0        , &alloc[0], &alloc[3], &alloc[4], &alloc[5], &alloc[6], &alloc[7], &alloc[8],
  &alloc[9], &alloc[10],&alloc[11],&alloc[12],&alloc[13],&alloc[14],&alloc[15],&alloc[16]
};

ampegdecoder::sballoc *ampegdecoder::atab1[]=
{
  0        , &alloc[0], &alloc[1], &alloc[3], &alloc[2], &alloc[4], &alloc[5], &alloc[6],
  &alloc[7], &alloc[8], &alloc[9], &alloc[10],&alloc[11],&alloc[12],&alloc[13],&alloc[16]
};

ampegdecoder::sballoc *ampegdecoder::atab2[]=
{
  0        , &alloc[0], &alloc[1], &alloc[3], &alloc[2], &alloc[4], &alloc[5], &alloc[16]
};

ampegdecoder::sballoc *ampegdecoder::atab3[]=
{
  0        , &alloc[0], &alloc[1], &alloc[16]
};

ampegdecoder::sballoc *ampegdecoder::atab4[]=
{
  0        , &alloc[0], &alloc[1], &alloc[2], &alloc[4], &alloc[5], &alloc[6], &alloc[7],
  &alloc[8], &alloc[9], &alloc[10],&alloc[11],&alloc[12],&alloc[13],&alloc[14],&alloc[15]
};

ampegdecoder::sballoc *ampegdecoder::atab5[]=
{
  0        , &alloc[0], &alloc[1], &alloc[3], &alloc[2], &alloc[4], &alloc[5], &alloc[6],
  &alloc[7], &alloc[8], &alloc[9], &alloc[10],&alloc[11],&alloc[12],&alloc[13],&alloc[14]
};

const int ampegdecoder::alloctablens[5]={27,30,8,12,30};

ampegdecoder::sballoc **ampegdecoder::alloctabs[3][32]=
{
  {
    atab0, atab0, atab0, atab1, atab1, atab1, atab1, atab1, atab1, atab1,
    atab1, atab2, atab2, atab2, atab2, atab2, atab2, atab2, atab2, atab2,
    atab2, atab2, atab2, atab3, atab3, atab3, atab3, atab3, atab3, atab3
  },
  {
    atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4,
    atab4, atab4
  },
  {
    atab5, atab5, atab5, atab5, atab4, atab4, atab4, atab4, atab4, atab4,
    atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4,
    atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4
  }
};

const int ampegdecoder::alloctabbits[3][32]=
{
  {4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2},
  {4,4,3,3,3,3,3,3,3,3,3,3},
  {4,4,4,4,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}
};

float ampegdecoder::multiple[64];
float ampegdecoder::rangefac[16];

void ampegdecoder::init12()
{
  int i;
  for (i=0; i<63; i++)
	  multiple[i] = exp(log(2.0f)*(3 - i) / 3.0);
  multiple[63]=0;
  for (i=0; i<16; i++)
    rangefac[i]=2.0/((2<<i)-1);
  for (i=0; i<3; i++)
  {
    alloc[i].scaleadd=1<<i;
    alloc[i].steps=2*alloc[i].scaleadd+1;
    alloc[i].bits=(10+5*i)>>1;
    alloc[i].scale=2.0/(2*alloc[i].scaleadd+1);
  }
  for (i=3; i<17; i++)
  {
    alloc[i].steps=0;
    alloc[i].bits=i;
    alloc[i].scaleadd=(1<<(i-1))-1;
    alloc[i].scale=2.0/(2*alloc[i].scaleadd+1);
  }
}

void ampegdecoder::openlayer2(int rate)
{
  if (rate)
  {
    slotsize=1;
    slotdiv=freqtab[orgfreq]>>orglsf;
    nslots=(144*rate)/(freqtab[orgfreq]>>orglsf);
    fslots=(144*rate)%slotdiv;
  }
}

void ampegdecoder::decode2()
{
  int i,j,k,q;

  if (!hdrbitrate)
  {
    for (q=0; q<36; q++)
      for (j=0; j<2; j++)
        for (i=0; i<32; i++)
          fraction[j][q][i]=0;
    return;
  }

  int bitend=mainbufpos-32-(hdrcrc?16:0)+144*1000*ratetab[hdrlsf?1:0][1][hdrbitrate]/(freqtab[hdrfreq]>>hdrlsf)*8+(hdrpadding?8:0);

  int stereo=(hdrmode==3)?1:2;
  int brpch=ratetab[0][1][hdrbitrate]/stereo;

  int tabnum;
  if (hdrlsf)
    tabnum=4;
  else
  if (((hdrfreq==1)&&(brpch>=56))||((brpch>=56)&&(brpch<=80)))
    tabnum=0;
  else
  if ((hdrfreq!=1)&&(brpch>=96))
    tabnum=1;
  else
  if ((hdrfreq!=2)&&(brpch<=48))
    tabnum=2;
  else
    tabnum=3;
  sballoc ***alloc=alloctabs[tabnum>>1];
  const int *allocbits=alloctabbits[tabnum>>1];
  int sblimit=alloctablens[tabnum];
  int jsbound;
  if (hdrmode==1)
    jsbound=(hdrmodeext+1)*4;
  else
  if (hdrmode==3)
    jsbound=0;
  else
    jsbound=sblimit;

  for (i=0; i<sblimit; i++)
    for (j=0; j<((i<jsbound)?2:1); j++)
    {
      bitalloc2[j][i]=alloc[i][mpgetbits(allocbits[i])];
      if (i>=jsbound)
        bitalloc2[1][i]=bitalloc2[0][i];
    }

  for (i=0; i<sblimit; i++)
    for (j=0; j<stereo; j++)
      if (bitalloc2[j][i])
        scfsi[j][i]=mpgetbits(2);

  for (i=0;i<sblimit;i++)
    for (j=0;j<stereo;j++)
      if (bitalloc2[j][i])
      {
        int si[3];
        switch (scfsi[j][i])
        {
        case 0:
          si[0]=mpgetbits(6);
          si[1]=mpgetbits(6);
          si[2]=mpgetbits(6);
          break;
        case 1:
          si[0]=si[1]=mpgetbits(6);
          si[2]=mpgetbits(6);
          break;
        case 2:
          si[0]=si[1]=si[2]=mpgetbits(6);
          break;
        case 3:
          si[0]=mpgetbits(6);
          si[1]=si[2]=mpgetbits(6);
          break;
        }
        for (k=0; k<3; k++)
          scale2[j][k][i]=multiple[si[k]]*bitalloc2[j][i]->scale;
      }

  for (q=0;q<12;q++)
  {
    for (i=0; i<sblimit; i++)
      for (j=0; j<((i<jsbound)?2:1); j++)
        if (bitalloc2[j][i])
        {
          int s[3];
          if (!bitalloc2[j][i]->steps)
            for (k=0; k<3; k++)
              s[k]=mpgetbits(bitalloc2[j][i]->bits)-bitalloc2[j][i]->scaleadd;
          else
          {
            int nlevels=bitalloc2[j][i]->steps;
            int c=mpgetbits(bitalloc2[j][i]->bits);
            for (k=0; k<3; k++)
            {
              s[k]=(c%nlevels)-bitalloc2[j][i]->scaleadd;
              c/=nlevels;
            }
          }

          for (k=0; k<3; k++)
            fraction[j][q*3+k][i]=s[k]*scale2[j][q>>2][i];
          if (i>=jsbound)
            for (k=0;k<3;k++)
              fraction[1][q*3+k][i]=s[k]*scale2[1][q>>2][i];
        }
        else
        {
          for (k=0;k<3;k++)
            fraction[j][q*3+k][i]=0;
          if (i>=jsbound)
            for (k=0;k<3;k++)
              fraction[1][q*3+k][i]=0;
        }

    for (i=sblimit; i<32; i++)
      for (k=0; k<3; k++)
        for (j=0; j<stereo; j++)
          fraction[j][q*3+k][i]=0;
  }
//  if ((bitend-mainbufpos)>=16)
//    decode2mc();
  mpgetbits(bitend-mainbufpos);
}
