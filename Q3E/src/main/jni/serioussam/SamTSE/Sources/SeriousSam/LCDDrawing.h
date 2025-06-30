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

#ifndef SE_INCL_LCDDRAWING_H
#define SE_INCL_LCDDRAWING_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// !!! FIXME : Lose this!
#ifdef PLATFORM_WIN32
extern void LCDInit(void);
extern void LCDEnd(void);
extern void LCDPrepare(FLOAT fFade);
extern void LCDSetDrawport(CDrawPort *pdp);
extern void LCDDrawBox(PIX pixUL, PIX pixDR, PIXaabbox2D &box, COLOR col);
extern void LCDScreenBox(COLOR col);
extern void LCDScreenBoxOpenLeft(COLOR col);
extern void LCDScreenBoxOpenRight(COLOR col);
extern void LCDRenderClouds1(void);
extern void LCDRenderClouds2(void);
extern void LCDRenderGrid(void);
extern void LCDDrawPointer(PIX pixI, PIX pixJ);
extern COLOR LCDGetColor(COLOR colDefault, const char *strName);
extern COLOR LCDFadedColor(COLOR col);
extern COLOR LCDBlinkingColor(COLOR col0, COLOR col1);
#endif

#endif  /* include-once check. */


