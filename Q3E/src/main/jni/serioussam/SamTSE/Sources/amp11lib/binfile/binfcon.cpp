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

// binfile - class library for files/streams - console io

#ifndef NOUNISTD
#if defined(WIN32)||defined(DOS)
#include <conio.h>
#include <io.h>
#endif
#if !defined(WIN32)
  #include <unistd.h>
#endif
#include <fcntl.h>
#include "binfcon.h"

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

idconsolebinfile::idconsolebinfile()
{
}

idconsolebinfile::~idconsolebinfile()
{
  close();
}

int idconsolebinfile::open()
{
  close();
#ifdef UNIX
  termios tattr;
  if (!isatty(STDIN_FILENO))
    return -1;
  tcgetattr(STDIN_FILENO, &savedtermios);
  tcgetattr(STDIN_FILENO, &tattr);
  tattr.c_lflag&=~(ICANON|ECHO);
  tattr.c_cc[VMIN]=0;
  tattr.c_cc[VTIME]=0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
#endif
  openmode(moderead, 0, 0);
  return 0;
}

errstat idconsolebinfile::rawclose()
{
  closemode();
#ifdef UNIX
  tcsetattr(STDIN_FILENO, TCSANOW, &savedtermios);
#endif
  return 0;
}

binfilepos idconsolebinfile::rawread(void *buf, binfilepos len)
{
#if defined(WIN32)||defined(DOS)
  int n=0;
  while ((n<len)&&_kbhit())
    ((char *)buf)[n++]=_getch();
  return n;
#else
#ifdef UNIX
  return ::read(STDIN_FILENO, buf, len);
#endif
  return 0;
#endif
}

binfilepos idconsolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}


consolebinfile::consolebinfile()
{
  openmode(moderead|modewrite, 0, 0);
}

consolebinfile::~consolebinfile()
{
  closemode();
}

errstat consolebinfile::rawclose()
{
  return -1;
}

binfilepos consolebinfile::rawread(void *buf, binfilepos len)
{
  return ::_read(STDIN_FILENO, buf, len);
}

binfilepos consolebinfile::rawwrite(const void *buf, binfilepos len)
{
  return ::_write(STDOUT_FILENO, buf, len);
}

binfilepos consolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

iconsolebinfile::iconsolebinfile()
{
  openmode(moderead, 0, 0);
}

iconsolebinfile::~iconsolebinfile()
{
  closemode();
}

errstat iconsolebinfile::rawclose()
{
  return -1;
}

binfilepos iconsolebinfile::rawread(void *buf, binfilepos len)
{
  return ::_read(STDIN_FILENO, buf, len);
}

binfilepos iconsolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

oconsolebinfile::oconsolebinfile()
{
  openmode(modewrite, 0, 0);
}

oconsolebinfile::~oconsolebinfile()
{
  closemode();
}

errstat oconsolebinfile::rawclose()
{
  return -1;
}

binfilepos oconsolebinfile::rawwrite(const void *buf, binfilepos len)
{
  return ::_write(STDOUT_FILENO, buf, len);
}

binfilepos oconsolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

econsolebinfile::econsolebinfile()
{
  openmode(modewrite, 0, 0);
}

econsolebinfile::~econsolebinfile()
{
  closemode();
}

errstat econsolebinfile::rawclose()
{
  return -1;
}

binfilepos econsolebinfile::rawwrite(const void *buf, binfilepos len)
{
  return ::_write(STDERR_FILENO, buf, len);
}

binfilepos econsolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

#else

#include <stdio.h>
#include "binfcon.h"

idconsolebinfile::idconsolebinfile()
{
  openmode(moderead, 0, 0);
}

idconsolebinfile::~idconsolebinfile()
{
  closemode();
}

errstat idconsolebinfile::rawclose()
{
  return -1;
}

binfilepos idconsolebinfile::rawread(void *buf, binfilepos len)
{
  return 0;
}

binfilepos idconsolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

consolebinfile::consolebinfile()
{
  openmode(moderead|modewrite, 0, 0);
}

consolebinfile::~consolebinfile()
{
  closemode();
}

errstat consolebinfile::rawclose()
{
  return -1;
}

binfilepos consolebinfile::rawread(void *buf, binfilepos len)
{
  return fread(buf, 1, len, stdin);
}

binfilepos consolebinfile::rawwrite(const void *buf, binfilepos len)
{
  return fwrite(buf, 1, len, stdout);
}

binfilepos consolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

iconsolebinfile::iconsolebinfile()
{
  openmode(moderead, 0, 0);
}

iconsolebinfile::~iconsolebinfile()
{
  closemode();
}

errstat iconsolebinfile::rawclose()
{
  return -1;
}

binfilepos iconsolebinfile::rawread(void *buf, binfilepos len)
{
  return fread(buf, 1, len, stdin);
}

binfilepos iconsolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

oconsolebinfile::oconsolebinfile()
{
  openmode(modewrite, 0, 0);
}

oconsolebinfile::~oconsolebinfile()
{
  closemode();
}

errstat oconsolebinfile::rawclose()
{
  return -1;
}

binfilepos oconsolebinfile::rawwrite(const void *buf, binfilepos len)
{
  return fwrite(buf, 1, len, stdout);
}

binfilepos oconsolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

econsolebinfile::econsolebinfile()
{
  openmode(modewrite, 0, 0);
}

econsolebinfile::~econsolebinfile()
{
  closemode();
}

errstat econsolebinfile::rawclose()
{
  return -1;
}

binfilepos econsolebinfile::rawwrite(const void *buf, binfilepos len)
{
  return fwrite(buf, 1, len, stderr);
}

binfilepos econsolebinfile::rawioctl(intm code, void *buf, binfilepos len)
{
//  switch (code)
//  {
//  default:
  return binfile::rawioctl(code, buf, len);
//  }
}

#endif
