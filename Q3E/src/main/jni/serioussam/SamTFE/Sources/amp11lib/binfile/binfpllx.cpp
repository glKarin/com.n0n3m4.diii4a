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

// binfile - class library for files/streams - linux sound playback

#ifndef LINUX
#error must compile for LINUX
#endif
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <string.h>
#include <fcntl.h>
#include "binfpllx.h"

int linuxplaybinfile::open(int rate, int stereo, int bit16)
{
  close();

  wavehnd=::open("/dev/dsp", O_WRONLY);
  if (wavehnd<0)
    return -1;
  ::ioctl(wavehnd, SNDCTL_DSP_RESET, 0);
  ::ioctl(wavehnd, SNDCTL_DSP_STEREO, &stereo);
  ::ioctl(wavehnd, SNDCTL_DSP_SPEED, &rate);
  int sample_size=bit16?16:8;
  ::ioctl(wavehnd, SNDCTL_DSP_SAMPLESIZE,&sample_size);
  int fmts=bit16?AFMT_S16_LE:AFMT_U8;
  ::ioctl(wavehnd, SNDCTL_DSP_SETFMT, &fmts);

  openmode(modewrite,0,0);
  return 0;
}

errstat linuxplaybinfile::rawclose()
{
  closemode();
  ::close(wavehnd);
  return 0;
}

binfilepos linuxplaybinfile::rawwrite(const void *buf, binfilepos len)
{
  return ::write(wavehnd, buf, len);
}

linuxplaybinfile::linuxplaybinfile()
{
}

linuxplaybinfile::~linuxplaybinfile()
{
  close();
}
