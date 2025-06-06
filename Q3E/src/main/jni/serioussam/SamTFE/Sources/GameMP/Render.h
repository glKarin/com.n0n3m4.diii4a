/* Copyright (c) 2022-2024 Dreamy Cecil
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

#ifndef XGIZMO_INCL_RENDERINTERFACE_H
#define XGIZMO_INCL_RENDERINTERFACE_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Get scaling multiplier based on screen height
#define HEIGHT_SCALING(DrawPort) ((FLOAT)DrawPort->GetHeight() / 480.0f)

// Interface of methods for rendering
namespace IRender {

// Calculate horizontal FOV according to the aspect ratio
// Can be cancelled using one of the following:
//   AdjustVFOV( FLOAT2D( 4, 3 ), fHFOV )
//   AdjustHFOV( FLOAT2D( 1, 1 ), fHFOV )
inline void AdjustHFOV(const FLOAT2D &vScreen, FLOAT &fHFOV) {
  // Get square ratio based off 4:3 resolution (4:3 = 1.0 ratio; 16:9 = 1.333 etc.)
  FLOAT fSquareRatio = (vScreen(1) / vScreen(2)) * (3.0f / 4.0f);

  // Take current FOV angle and apply square ratio to it
  FLOAT fVerticalAngle = Tan(fHFOV * 0.5f) * fSquareRatio;

  // 90 FOV on 16:9 resolution will become 106.26...
  fHFOV = 2.0f * ATan(fVerticalAngle);
};

// Calculate vertical FOV from horizontal FOV according to the aspect ratio
// Can be cancelled using one of the following:
//   AdjustVFOV( FLOAT2D( vScreen(2), vScreen(1) ), fHFOV )
//   AdjustHFOV( FLOAT2D( vScreen(1), vScreen(2) * 0.75f ), fHFOV )
inline void AdjustVFOV(const FLOAT2D &vScreen, FLOAT &fHFOV) {
  // Take current FOV angle and apply inverted aspect ratio to it
  FLOAT fVerticalAngle = Tan(fHFOV * 0.5f) * (vScreen(2) / vScreen(1));

  // 90 FOV on 4:3 or 106.26 FOV on 16:9 will become 73.74...
  fHFOV = 2.0f * ATan(fVerticalAngle);
};

// Optimized function for getting text width in pixels
inline ULONG GetTextWidth(CDrawPort *pdp, const CTString &str) {
  // Prepare scaling factors
  CFontData *pfd = pdp->dp_FontData;
  const PIX pixCellWidth = pfd->fd_pixCharWidth;
  const SLONG fixTextScalingX = FloatToInt(pdp->dp_fTextScaling * pdp->dp_fTextAspect * 65536.0f);

  // Calculate width of the entire text line
  PIX pixStringWidth = 0;
  PIX pixOldWidth = 0;

  PIX pixCharStart = 0;
  PIX pixCharEnd = pixCellWidth;

  const ULONG ct = str.Length();

  for (ULONG i = 0; i < ct; i++) {
    UBYTE ch = str[i];

    // Reset width if new line
    if (ch == '\n') {
      pixOldWidth = ClampDn(pixOldWidth, pixStringWidth);
      pixStringWidth = 0;
      continue;
    }

    // Ignore tab
    if (ch == '\t') continue;

    // Decorative tag
    if (ch == '^' && pdp->dp_iTextMode != -1) {
      ch = str[++i];
      UBYTE *pubTag = (UBYTE *)&str[i];

      switch (ch) {
        // Skip corresponding number of characters
        case 'c': i += FindZero(pubTag, 6); continue;
        case 'a': i += FindZero(pubTag, 2); continue;
        case 'f': i += FindZero(pubTag, 1); continue;

        case 'b': case 'i': case 'r': case 'o':
        case 'C': case 'A': case 'F': case 'B': case 'I':
          continue;

        // Non-tag character
        default: break;
      }
    }

    // Add character width to the result
    if (!pfd->fd_bFixedWidth) {
      // Proportional font case
      pixCharStart = pfd->fd_fcdFontCharData[ch].fcd_pixStart;
      pixCharEnd = pfd->fd_fcdFontCharData[ch].fcd_pixEnd;
    }

    pixStringWidth += (((pixCharEnd - pixCharStart) * fixTextScalingX) >> 16) + pdp->dp_pixTextCharSpacing;
  }

  // Determine largest width
  return ClampDn(pixStringWidth, pixOldWidth);
};

}; // namespace

#endif
