/* Copyright (c) 2004-2008 Luigi Auriemma. 
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

/*
GSMSALG 0.3.3
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org


INTRODUCTION
============
With the name Gsmsalg I define the challenge-response algorithm needed
to query the master servers that use the Gamespy "secure" protocol (like
master.gamespy.com for example).
This algorithm is not only used for this type of query but also in other
situations like the so called "Gamespy Firewall Probe Packet" and the
master server hearbeat that is the challenge string sent by the master
servers of the games that use the Gamespy SDK when game servers want to
be included in the online servers list (UDP port 27900).


HOW TO USE
==========
The function needs 4 parameters:
- dst:     the destination buffer that will contain the calculated
           response. Its length is 4/3 of the challenge size so if the
           challenge is 6 bytes long, the response will be 8 bytes long
           plus the final NULL byte which is required (to be sure of the
           allocated space use 89 bytes or "((len * 4) / 3) + 3")
           if this parameter is NULL the function will allocate the
           memory for a new one automatically
- src:     the source buffer containing the challenge string received
           from the server.
- key:     the gamekey or any other text string used as algorithm's
           key, usually it is the gamekey but "might" be another thing
           in some cases. Each game has its unique Gamespy gamekey which
           are available here:
           http://aluigi.org/papers/gslist.cfg
- enctype: are supported 0 (plain-text used in old games, heartbeat
           challenge respond, enctypeX and more), 1 (Gamespy3D) and 2
           (old Gamespy Arcade or something else).

The return value is a pointer to the destination buffer.


EXAMPLE
=======
  #include "MSLegacy.h"

  char  *dest;
  dest = gsseckey(
    NULL,       // dest buffer, NULL for auto allocation
    "ABCDEF",   // the challenge received from the server
    "kbeafe",   // kbeafe of Doom 3 and enctype set to 0
    0);         // enctype 0


LICENSE
=======

    http://www.gnu.org/licenses/gpl.txt
*/
#ifndef SE_INCL_MSLEGACY_H
#define SE_INCL_MSLEGACY_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/* function gsvalfunc */
unsigned char gsvalfunc(u_char reg) {
    if(reg < 0x1a) return u_char (reg + 'A');
    if(reg < 0x34) return u_char (reg + 'G');
    if(reg < 0x3e) return u_char (reg - 4);
    if(reg == 0x3e) return u_char('+');
    if(reg == 0x3f) return u_char ('/');
    return u_char(0);
}

/* function gsseckey */
unsigned char *gsseckey(u_char *secure, u_char *key, int enctype) {
    static  u_char          validate[9];
    u_char                  secbuf[7],
                            buff[256],
                            *ptr,
                            *ptrval,
                            *sec,
                            *k,
                            tmp1,
                            tmp2,
                            ebx,
                            i,
                            ecx,
                            edx,
                            edi;

    i = 0;
    ptr = buff;
    do { *ptr++ = i++; } while(i);  /* 256 times */

    ptr = buff;
    k = (unsigned char*) memcpy(secbuf, key, 6); /* good if key is not NULLed */
    k[6] = edx = i = 0;
    do {    /* 256 times */
        if(!*k) k = secbuf;
        edx = *ptr + edx + *k;
            /* don't use the XOR exchange optimization!!! */
            /* ptrval is used only for faster code */
        ptrval  = buff + edx;
        tmp1    = *ptr;
        *ptr    = *ptrval;
        *ptrval = tmp1;
        ptr++; k++; i++;
    } while(i);

    sec = (unsigned char *) memcpy(secbuf, secure, 6);
    sec[6] = edi = ebx = 0;
    do {    /* 6 times */
        edi = edi + *sec + 1;
        ecx = ebx + buff[edi];
        ebx = ecx;
            /* don't use the XOR exchange optimization!!! */
            /* ptr and ptrval are used only for faster code */
        ptr     = buff + edi;
        ptrval  = buff + ebx;
        tmp1    = *ptr;
        *ptr    = *ptrval;
        *ptrval = tmp1;
        ecx = tmp1 + *ptr;
        *sec++ ^= buff[ecx];
    } while(*sec);

    if(enctype == 2) {
        ptr = key;
        sec = secbuf;
        do {    /* 6 times */
            *sec++ ^= *ptr++;
        } while(*sec);
    }

    sec = secbuf;
    ptrval = validate;
    for(i = 0; i < 2; i++) {
        tmp1 = *sec++;
        tmp2 = *sec++;
        *ptrval++ = gsvalfunc(tmp1 >> 2);
        *ptrval++ = gsvalfunc(((tmp1 & 3) << 4) + (tmp2 >> 4));
        tmp1 = *sec++;
        *ptrval++ = gsvalfunc(((tmp2 & 0xf) << 2) + (tmp1 >> 6));
        *ptrval++ = gsvalfunc(tmp1 & 0x3f);
    }
    *ptrval = 0x00;

    return(validate);
}

/* function resolv */
u_int resolv(char *host) {
    struct  hostent *hp;
    u_int   host_ip;

    host_ip = inet_addr(host);
    if(host_ip == INADDR_NONE) {
        hp = gethostbyname(host);
        if(!hp) {
            return 0;
        } else host_ip = *(u_int *)(hp->h_addr);
    }
    return(host_ip);
}

/* end functions  */

#endif // include once check
