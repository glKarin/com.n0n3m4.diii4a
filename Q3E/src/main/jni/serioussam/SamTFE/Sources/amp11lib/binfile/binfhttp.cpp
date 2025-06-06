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

// binfile - class library for files/streams - http receiver

#include <stdio.h>
#include <string.h>
#include "binfhttp.h"

httpbinfile::httpbinfile()
{
}

httpbinfile::~httpbinfile()
{
  close();
}

errstat httpbinfile::open(const char *url, const char *proxy, binfilepos off, binfilepos len)
{
  if (proxy)
    if (!strncmp(proxy, "http://", 7))
      proxy+=7;
  char buf[100];
  while (1)
  {
    if (!strncmp(url, "http://", 7))
      url+=7;
    int r=file.open(proxy?proxy:url, 80);
    if (r<0)
      return r;

    writestr(file, "GET ");
    if (proxy)
      writestr(file, "http://");
    const char *fname;
    for (fname=proxy?url:strchr(url,'/')?strchr(url,'/'):"/"; *fname; fname++)
      if (*fname==' ')
        writestr(file, "%20");
      else
      if (*fname=='%')
        writestr(file, "%25");
      else
        putch(file, *fname);
    writestr(file, " HTTP/1.0\n");
    if (off||len)
    {
      if (!len)
        sprintf(buf, "Range: bytes=%i-\n", off);
      else
        sprintf(buf, "Range: bytes=%i-%i\n", off, off+len-1);
      writestr(file, buf);
    }
    putch(file, '\n');

    int clen=0;
    int rfrom=0;
    int rto,rtot;
    while (1)
    {
      if (file.ioctl(ioctlreof))
        return -1;
      readline(file, buf, 100, '\n');
      while (strlen(buf)&&(buf[strlen(buf)-1]=='\r'))
        buf[strlen(buf)-1]=0;
      if (!strncmp(buf, "Content-Length:", 15))
        sscanf(buf, "Content-Length: %i", &clen);
      if (!strncmp(buf, "Content-Range:", 14))
        sscanf(buf, "Content-Range: bytes %i-%i/%i", &rfrom, &rto, &rtot);
      if (!strncmp(buf, "Location:", 9))
      {
        url=buf+10;
        file.close();
        break;
      }
      if (!*buf)
      {
        openpipe(file,0,modewrite,rfrom,0,rfrom+clen);
        return 0;
      }
    }
  }
}
