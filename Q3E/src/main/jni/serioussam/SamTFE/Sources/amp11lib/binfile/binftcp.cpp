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

// binfile - class library for files/streams - tcp io

#if defined(WIN32)||defined(UNIX)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
 #include <winsock.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "wsock32.lib")
#endif

#ifdef UNIX
#ifdef ALPHA
extern "C"
{
#endif
 #include <sys/ioctl.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <fcntl.h>
 #include <arpa/inet.h>
 #include <errno.h>
 #include <unistd.h>
#ifdef ALPHA
}
#endif

 #define INVALID_SOCKET -1
 #define SOCKET_ERROR -1
 #define ioctlsocket ::ioctl
 #define closesocket ::close
 #define WSAEWOULDBLOCK EWOULDBLOCK
 #define WSAGetLastError() errno

#ifdef SUNOS4
struct in_addr
{
  u_long s_addr;
};

struct sockaddr_in
{
  short sin_family;
  u_short sin_port;
  in_addr sin_addr;
  char sin_zero[8];
};

extern "C"
{
  int socket(int, int, int);
  int setsockopt(int, int, int, char *, int);
  int getsockopt(int, int, int, char *, int*);
  int connect(int, const sockaddr *, int);
  int getsockname(int, sockaddr *, int *);
  int send(int, const char *, int, int);
  int recv(int, char *, int, int);
  int ioctl(int, int, u_long *);
  int shutdown(int, int);
  int bind(int, const sockaddr *, int);
  int listen(int, int);
  int accept(int, const sockaddr *, int *);
  int getpeername(int, const sockaddr *, int *);
  u_long htonl(u_long);
  u_short htons(u_short);
  u_long ntohl(u_long);
  u_short ntohs(u_short);
}

#ifndef INADDR_ANY
#define INADDR_ANY 0
#endif

#endif
#endif

#ifndef SD_SEND
#define SD_RECEIVE 0
#define SD_SEND 1
#endif


#include "binftcp.h"

intm tcpbinfile::initcount;

tcpbinfile::tcpbinfile()
{
  globalinit();
}

tcpbinfile::~tcpbinfile()
{
  close();
  globaldone();
}

boolm tcpbinfile::globalinit()
{
#ifdef WIN32
  if (!initcount)
  {
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(1,1), &wsadata))
      return 0;
  }
#endif
  initcount++;
  return 1;
}

void tcpbinfile::globaldone()
{
  if (initcount)
    initcount--;
#ifdef WIN32
  if (!initcount)
    WSACleanup();
#endif
}

errstat tcpbinfile::open(const char *addr, uint2 port)
{
  port=atoport(addr, port);
  uint4 a=atoaddr(addr);
  if (!a||!port)
    return -1;
  return open(a, port);
}

errstat tcpbinfile::open(uint4 addr, uint2 port)
{
  close();
  if (!initcount)
    return -1;

  handle=socket(AF_INET, SOCK_STREAM, 0);
  if (handle<0)
    return -1;

  linger lin;
  lin.l_onoff=lin.l_linger=0;
  int opt=1;
  setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
  setsockopt(handle, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));
  setsockopt(handle, SOL_SOCKET, SO_LINGER, (char *)&lin, sizeof(lin));

  sockaddr_in ad;
  ad.sin_family=AF_INET;
  ad.sin_addr.s_addr=htonl(addr);
  ad.sin_port=htons(port);

  if (connect(handle, (sockaddr *)&ad, sizeof(ad))<0)
  {
    closesocket(handle);
    return -1;
  }

  blocking=1;
  unsigned long nb=!blocking;
  ioctlsocket(handle, FIONBIO, &nb);

  openmode(moderead|modewrite, 0, 0);
  closed=0;
  wclosed=0;

  return 0;
}

errstat tcpbinfile::open(tcplistener &l)
{
  close();
  if (!initcount)
    return -1;
  if (l.handle==INVALID_SOCKET)
    return -1;

  handle=accept(l.handle, 0, 0);
  if (handle==INVALID_SOCKET)
    return -2;

  linger lin;
  lin.l_onoff=lin.l_linger=0;

  int opt=1;
  setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
  setsockopt(handle, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));
  setsockopt(handle, SOL_SOCKET, SO_LINGER, (char *)&lin, sizeof(lin));

  blocking=1;
  unsigned long nb=!blocking;
  ioctlsocket(handle, FIONBIO, &nb);

  openmode(moderead|modewrite, 0, 0);
  closed=0;
  wclosed=0;

  return 0;
}

errstat tcpbinfile::rawclose()
{
  closemode();
  return (closesocket(handle)==WSAEWOULDBLOCK)?-1:0;
}

binfilepos tcpbinfile::rawread(void *buf, binfilepos len)
{
  binfilepos l=recv(handle, (char*)buf, len, 0);
  if (!l)
    closed=1;
  if (l==SOCKET_ERROR)
  {
    if (WSAGetLastError()!=WSAEWOULDBLOCK)
      closed=1;
    return 0;
  }
  return l;
}

binfilepos tcpbinfile::rawpeek(void *buf, binfilepos len)
{
  binfilepos l=recv(handle, (char*)buf, len, MSG_PEEK);
  if (!l)
    closed=1;
  if (l==SOCKET_ERROR)
  {
    if (WSAGetLastError()!=WSAEWOULDBLOCK)
      closed=1;
    return 0;
  }
  return l;
}

binfilepos tcpbinfile::rawwrite(const void *buf, binfilepos len)
{
  binfilepos l=send(handle, (char*)buf, len, 0);
  if (l==SOCKET_ERROR)
  {
    if (WSAGetLastError()!=WSAEWOULDBLOCK)
      wclosed=1;
    return 0;
  }
  return l;
}

binfilepos tcpbinfile::rawioctl(intm code, void *buf, binfilepos len)
{
  binfilepos ret;
  int opt;
  switch (code)
  {
  case ioctlblocking:
    ret=blocking;
    blocking=len;
    unsigned long nb;
    nb=!blocking;
    ioctlsocket(handle, FIONBIO, &nb);
    return ret;
  case ioctlblockingget:
    return blocking;
  case ioctlrmax:
    unsigned long readmax;
    ioctlsocket(handle, FIONREAD, &readmax);
    return readmax+binfile::rawioctl(ioctlrbufget, buf, len);
  case ioctlrweof:
    return wclosed;
  case ioctlrreof:
    if (closed)
      return 1;
    int1 eofc;
    intm eofret;
    eofret=recv(handle, (char*)&eofc, 1, MSG_PEEK);
    if (eofret==0)
      closed=1;
    if (eofret==SOCKET_ERROR)
    {
      if (WSAGetLastError()!=WSAEWOULDBLOCK)
        closed=1;
    }
    return closed;
  case ioctlrrbufset:
    getsockopt(handle, SOL_SOCKET, SO_RCVBUF, (char *)&opt, 0);
    ret=opt;
    opt=len;
    setsockopt(handle, SOL_SOCKET, SO_RCVBUF, (char *)&opt, sizeof(opt));
    return ret;
  case ioctlrrbufgetlen:
    getsockopt(handle, SOL_SOCKET, SO_RCVBUF, (char *)&opt, 0);
    return opt;
  case ioctlrwbufset:
    getsockopt(handle, SOL_SOCKET, SO_SNDBUF, (char *)&opt, 0);
    ret=opt;
    opt=len;
    setsockopt(handle, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt));
    return ret;
  case ioctlrwbufgetlen:
    getsockopt(handle, SOL_SOCKET, SO_SNDBUF, (char *)&opt, 0);
    return opt;
  case ioctlrshutdown:
    if (!binfile::rawioctl(ioctlrshutdown, 0, 0))
      return 0;
    shutdown(handle, SD_RECEIVE);
    closed=1;
    return 1;
  case ioctlwshutdown:
    if (!binfile::rawioctl(ioctlwshutdown, 0, 0))
      return 0;
    shutdown(handle, SD_SEND);
    wclosed=1;
    return 1;
  case ioctlwshutdownforce:
    if (!binfile::rawioctl(ioctlwshutdownforce, 0, 0))
      return 0;
    shutdown(handle, SD_SEND);
    wclosed=1;
    return 1;
  case ioctllingerget:
    linger lin;
    getsockopt(handle, SOL_SOCKET, SO_LINGER, (char *)&lin, 0);
    return lin.l_onoff?lin.l_linger*1000:-1;
  case ioctllinger:
    linger lino,linn;
    getsockopt(handle, SOL_SOCKET, SO_LINGER, (char *)&lino, 0);
    linn.l_onoff=(len<0)?0:1;
    linn.l_linger=(len+999)/1000;
    setsockopt(handle, SOL_SOCKET, SO_LINGER, (char *)&linn, sizeof(linn));
    return lino.l_onoff?lino.l_linger*1000:-1;
  default:
    return binfile::rawioctl(code, buf, len);
  }
}

uint4 tcpbinfile::getlocaladdr()
{
  sockaddr_in ad;
  int l=sizeof(ad);
  if (getsockname(handle, (sockaddr *)&ad, &l))
    return 0;
  return ntohl(ad.sin_addr.s_addr);
}

uint2 tcpbinfile::getlocalport()
{
  sockaddr_in ad;
  int l=sizeof(ad);
  if (getsockname(handle, (sockaddr *)&ad, &l))
    return 0;
  return ntohs(ad.sin_port);
}

uint4 tcpbinfile::getremoteaddr()
{
  sockaddr_in ad;
  int l=sizeof(ad);
  if (getpeername(handle, (sockaddr *)&ad, &l))
    return 0;
  return ntohl(ad.sin_addr.s_addr);
}

uint2 tcpbinfile::getremoteport()
{
  sockaddr_in ad;
  int l=sizeof(ad);
  if (getpeername(handle, (sockaddr *)&ad, &l))
    return 0;
  return ntohs(ad.sin_port);
}

char *tcpbinfile::addrtoa(char *buf, uint4 addr)
{
  sprintf(buf, "%i.%i.%i.%i", (int)((addr>>24)&0xFF), (int)((addr>>16)&0xFF), (int)((addr>>8)&0xFF), (int)((addr>>0)&0xFF));
  return buf;
}

char *tcpbinfile::addrtoa(char *buf, uint4 addr, uint2 port)
{
  sprintf(buf, "%i.%i.%i.%i:%i", (int)((addr>>24)&0xFF), (int)((addr>>16)&0xFF), (int)((addr>>8)&0xFF), (int)((addr>>0)&0xFF), (int)port);
  return buf;
}

uint4 tcpbinfile::atoaddr(const char *buf)
{
  char ad[200];
  if (strchr(buf, '/')||strchr(buf, ':'))
  {
    intm l1=strchr(buf, '/')?(strchr(buf, '/')-buf):strlen(buf);
    intm l2=strchr(buf, ':')?(strchr(buf, ':')-buf):strlen(buf);
    intm l=(l1<l2)?l1:l2;
    memcpy(ad, buf, l);
    ad[l]=0;
    buf=ad;
  }

  int a0,a1,a2,a3;
  if (sscanf(buf, "%i.%i.%i.%i", &a3, &a2, &a1, &a0)!=4)
    a0=-1;
  if ((a0>=0)&&(a0<=255)&&(a1>=0)&&(a1<=255)&&(a2>=0)&&(a2<=255)&&(a3>=0)&&(a3<=255))
    return a0|(a1<<8)|(a2<<16)|(a3<<24);
  hostent *hp=gethostbyname(buf);
  if (!hp)
    return 0;
  return ntohl(*(uint4*)hp->h_addr);
}

uint2 tcpbinfile::atoport(const char *buf, uint2 port)
{
  if (!strchr(buf, ':'))
    return port;
  else
    if (strchr(buf, '/')&&(strchr(buf, '/')<strchr(buf, ':')))
      return port;
    else
      return atoi(strchr(buf, ':')+1);
}

tcplistener::tcplistener()
{
  tcpbinfile::globalinit();
  handle=SOCKET_ERROR;
}

tcplistener::~tcplistener()
{
  close();
  tcpbinfile::globaldone();
}

errstat tcplistener::open(uint2 port, intm conbuf)
{
  close();
  if (!tcpbinfile::initcount)
    return -1;

  handle=socket(AF_INET, SOCK_STREAM, 0);
  if (handle==INVALID_SOCKET)
    return -1;

  blocking=1;
  unsigned long nb=!blocking;
  ioctlsocket(handle, FIONBIO, &nb);

  sockaddr_in ad;
  ad.sin_family=AF_INET;
  ad.sin_addr.s_addr=INADDR_ANY;
  ad.sin_port=htons(port);

  if (bind(handle, (sockaddr *)&ad, sizeof(ad))<0)
  {
    closesocket(handle);
    handle=INVALID_SOCKET;
    return -1;
  }

  if (listen(handle, conbuf)<0)
  {
    closesocket(handle);
    handle=INVALID_SOCKET;
    return -1;
  }

  if (!port)
  {
    int l=sizeof(l);
    getsockname(handle, (sockaddr *)&ad, &l);
    port=ntohs(ad.sin_port);
  }

  return port;
}

errstat tcplistener::close()
{
  if (handle==INVALID_SOCKET)
    return 0;
  closesocket(handle);
  handle=INVALID_SOCKET;
  return 0;
}

boolm tcplistener::setblocking(boolm b)
{
  int old=blocking;
  blocking=b;
  unsigned long nb=!blocking;
  ioctlsocket(handle, FIONBIO, &nb);
  return old;
}

boolm tcplistener::getblocking()
{
  return blocking;
}

#else

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "binftcp.h"

intm tcpbinfile::initcount;

tcpbinfile::tcpbinfile()
{
}

tcpbinfile::~tcpbinfile()
{
  close();
}

boolm tcpbinfile::globalinit()
{
  return 1;
}

void tcpbinfile::globaldone()
{
}

errstat tcpbinfile::open(const char *, uint2)
{
  return -1;
}

errstat tcpbinfile::open(uint4, uint2)
{
  return -1;
}

errstat tcpbinfile::open(tcplistener &)
{
  return -1;
}

errstat tcpbinfile::rawclose()
{
  return 0;
}

binfilepos tcpbinfile::rawread(void *, binfilepos)
{
  return 0;
}

binfilepos tcpbinfile::rawpeek(void *, binfilepos)
{
  return 0;
}

binfilepos tcpbinfile::rawwrite(const void *, binfilepos)
{
  return 0;
}

binfilepos tcpbinfile::rawioctl(intm code, void *buf, binfilepos len)
{
  return binfile::rawioctl(code, buf, len);
}

uint4 tcpbinfile::getlocaladdr()
{
  return 0;
}

uint2 tcpbinfile::getlocalport()
{
  return 0;
}

uint4 tcpbinfile::getremoteaddr()
{
  return 0;
}

uint2 tcpbinfile::getremoteport()
{
  return 0;
}

char *tcpbinfile::addrtoa(char *buf, uint4 addr)
{
  sprintf(buf, "%i.%i.%i.%i", (int)((addr>>24)&0xFF), (int)((addr>>16)&0xFF), (int)((addr>>8)&0xFF), (int)((addr>>0)&0xFF));
  return buf;
}

char *tcpbinfile::addrtoa(char *buf, uint4 addr, uint2 port)
{
  sprintf(buf, "%i.%i.%i.%i:%i", (int)((addr>>24)&0xFF), (int)((addr>>16)&0xFF), (int)((addr>>8)&0xFF), (int)((addr>>0)&0xFF), (int)port);
  return buf;
}

uint4 tcpbinfile::atoaddr(const char *buf)
{
  char ad[200];
  if (strchr(buf, '/')||strchr(buf, ':'))
  {
    intm l1=strchr(buf, '/')?(strchr(buf, '/')-buf):strlen(buf);
    intm l2=strchr(buf, ':')?(strchr(buf, ':')-buf):strlen(buf);
    intm l=(l1<l2)?l1:l2;
    memcpy(ad, buf, l);
    ad[l]=0;
    buf=ad;
  }

  int a0,a1,a2,a3;
  if (sscanf(buf, "%i.%i.%i.%i", &a3, &a2, &a1, &a0)!=4)
    a0=-1;
  if ((a0>=0)&&(a0<=255)&&(a1>=0)&&(a1<=255)&&(a2>=0)&&(a2<=255)&&(a3>=0)&&(a3<=255))
    return a0|(a1<<8)|(a2<<16)|(a3<<24);
  return 0;
}

uint2 tcpbinfile::atoport(const char *buf, uint2 port)
{
  if (!strchr(buf, ':'))
    return port;
  else
    if (strchr(buf, '/')&&(strchr(buf, '/')<strchr(buf, ':')))
      return port;
    else
      return atoi(strchr(buf, ':')+1);
}

tcplistener::tcplistener()
{
}

tcplistener::~tcplistener()
{
  close();
}

errstat tcplistener::open(uint2, intm)
{
  return -1;
}

errstat tcplistener::close()
{
  return 0;
}

boolm tcplistener::setblocking(boolm)
{
  return 0;
}

boolm tcplistener::getblocking()
{
  return 0;
}

#endif
