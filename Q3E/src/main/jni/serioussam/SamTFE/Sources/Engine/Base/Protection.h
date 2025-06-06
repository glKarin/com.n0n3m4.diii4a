/* Copyright (c) 2002-2012 Croteam Ltd. 
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

#ifndef SE_INCL_PROTECTION_H
#define SE_INCL_PROTECTION_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif
#define COPY_PROTECTION 1

typedef struct {
  unsigned long P[16 + 2];
  unsigned long S[4][256];
} BLOWFISH_CTX;

extern void Blowfish_Init(BLOWFISH_CTX *ctx, unsigned char *key, int keyLen);
extern void Blowfish_Decrypt(BLOWFISH_CTX *ctx, unsigned long *xl, unsigned long *xr);


#endif  /* include-once check. */

