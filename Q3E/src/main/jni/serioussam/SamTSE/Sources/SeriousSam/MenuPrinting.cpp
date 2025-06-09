/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#include "SeriousSam/StdH.h"

#include "MenuPrinting.h"

__extern FLOAT _fBigStartJ	= 0.25f;
__extern FLOAT _fBigSizeJ		= 0.066f;
__extern FLOAT _fMediumSizeJ	= 0.04f;
__extern FLOAT _fNoStartI		= 0.25f;
__extern FLOAT _fNoSizeI		= 0.04f;
__extern FLOAT _fNoSpaceI		= 0.01f;
__extern FLOAT _fNoUpStartJ	= 0.24f;
__extern FLOAT _fNoDownStartJ	= 0.44f;
__extern FLOAT _fNoSizeJ		= 0.04f;

#ifdef SAM_VERSION_FE105
#define _scaler_ 3.5
#else
#define _scaler_ 3.5
#endif

#define _BOXBIGY1 (_fBigSizeJ * _scaler_)+fRow*_fBigSizeJ
#define _BOXBIGY2 (_fBigSizeJ * _scaler_)+(fRow+1)*_fBigSizeJ

#define _BOXMEDIUMY1 (_fBigStartJ * 1.0)+fRow*_fMediumSizeJ
#define _BOXMEDIUMY2 (_fBigStartJ * 1.0)+(fRow+1)*_fMediumSizeJ

FLOATaabbox2D BoxTitle(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0, _fBigSizeJ),
    FLOAT2D(1, _fBigSizeJ));
}
FLOATaabbox2D BoxNoUp(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(_fNoStartI+fRow*(_fNoSizeI+_fNoSpaceI), fOffset * _fNoUpStartJ),
    FLOAT2D(_fNoStartI+fRow*(_fNoSizeI+_fNoSpaceI)+_fNoSizeI, fOffset * (_fNoUpStartJ) + _fNoSizeJ));
}
FLOATaabbox2D BoxNoDown(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(_fNoStartI+fRow*(_fNoSizeI+_fNoSpaceI), _fNoDownStartJ),
    FLOAT2D(_fNoStartI+fRow*(_fNoSizeI+_fNoSpaceI)+_fNoSizeI, _fNoDownStartJ+_fNoSizeJ));
}
FLOATaabbox2D BoxBigRow(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(0.1f, fOffset * _BOXBIGY1),
    FLOAT2D(0.9f, fOffset * _BOXBIGY2)
    );
}
FLOATaabbox2D BoxBigLeft(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.1f, _BOXBIGY1),
    FLOAT2D(0.45f, _BOXBIGY2)
    );
}
FLOATaabbox2D BoxBigLeftBorder(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, _BOXBIGY1),
    FLOAT2D(0.15f, _BOXBIGY2)
    );
}
FLOATaabbox2D BoxBigRight(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.55f, _BOXBIGY1),
    FLOAT2D(0.9f, _BOXBIGY2)
    );
}

FLOATaabbox2D BoxSaveLoad(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(0.20f, fOffset * _BOXMEDIUMY1),
    FLOAT2D(0.95f, fOffset * _BOXMEDIUMY2)
    );
}

FLOATaabbox2D BoxVersion(FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(0.05f, fOffset * _fBigStartJ+-4.5f*_fMediumSizeJ),
    FLOAT2D(0.97f, fOffset * _fBigStartJ+(-4.5f+1)*_fMediumSizeJ));
}
FLOATaabbox2D BoxMediumRow(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(0.05f, fOffset * _BOXMEDIUMY1),
    FLOAT2D(0.95f, fOffset * _BOXMEDIUMY2)
    );
}
FLOATaabbox2D BoxKeyRow(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(0.15f, fOffset * _BOXMEDIUMY1),
    FLOAT2D(0.85f, fOffset * _BOXMEDIUMY2)
    );
}
FLOATaabbox2D BoxMediumLeft(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(0.05f, fOffset * _BOXMEDIUMY1),
    FLOAT2D(0.45f, fOffset * _BOXMEDIUMY2)
    );
}
FLOATaabbox2D BoxPlayerSwitch(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(0.05f, fOffset * _BOXMEDIUMY1),
    FLOAT2D(0.65f, fOffset * _BOXMEDIUMY2)
    );
}
FLOATaabbox2D BoxMediumMiddle(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(_fNoStartI, fOffset * _BOXMEDIUMY1),
    FLOAT2D(0.95f, fOffset * _BOXMEDIUMY2)
    );
}
FLOATaabbox2D BoxPlayerEdit(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(_fNoStartI, fOffset * _BOXMEDIUMY1),
    FLOAT2D(0.65f, fOffset * _BOXMEDIUMY2)
    );
}
FLOATaabbox2D BoxMediumRight(FLOAT fRow, FLOAT fOffset)
{
  return FLOATaabbox2D(
    FLOAT2D(0.55f, fOffset * _BOXMEDIUMY1),
    FLOAT2D(0.95f, fOffset * _BOXMEDIUMY2)
    );
}
FLOATaabbox2D BoxPopup(void)
{
  return FLOATaabbox2D(FLOAT2D(0.2f, 0.4f), FLOAT2D(0.8f, 0.6f));
}

FLOATaabbox2D BoxPopupLabel(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.22f, 0.43f),
    FLOAT2D(0.78f, 0.49f));
}
FLOATaabbox2D BoxPopupYesLarge(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.30f, 0.51f),
    FLOAT2D(0.48f, 0.57f));
}
FLOATaabbox2D BoxPopupNoLarge(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.52f, 0.51f),
    FLOAT2D(0.70f, 0.57f));
}
FLOATaabbox2D BoxPopupYesSmall(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.30f, 0.54f),
    FLOAT2D(0.48f, 0.59f));
}
FLOATaabbox2D BoxPopupNoSmall(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.52f, 0.54f),
    FLOAT2D(0.70f, 0.59f));
}
FLOATaabbox2D BoxChangePlayer(INDEX iTable, INDEX iButton)
{
  return FLOATaabbox2D(
    FLOAT2D(0.5f+0.15f*(iButton-1), _fBigStartJ+_fMediumSizeJ*2.0f*iTable),
    FLOAT2D(0.5f+0.15f*(iButton+0), _fBigStartJ+_fMediumSizeJ*2.0f*(iTable+1)));
}

FLOATaabbox2D BoxInfoTable(INDEX iTable)
{
  switch(iTable) {
  case 0:
  case 1:
  case 2:
  case 3:
    return FLOATaabbox2D(
      FLOAT2D(0.1f, _fBigStartJ+_fMediumSizeJ*2.0f*iTable),
      FLOAT2D(0.5f, _fBigStartJ+_fMediumSizeJ*2.0f*(iTable+1)));
  default:
    ASSERT(FALSE);
  case -1:  // single player table
  return FLOATaabbox2D(
    FLOAT2D(0.1f, 1-0.2f-_fMediumSizeJ*2.0f),
    FLOAT2D(0.5f, 1-0.2f));
  }
}

FLOATaabbox2D BoxArrow(enum ArrowDir ad, FLOAT fOffset)
{
  FLOAT fRow;
  switch(ad) {
  default:
    ASSERT(FALSE);
  case AD_UP:
	fRow = 0.0;
    return FLOATaabbox2D(
      FLOAT2D(0.02f, fOffset * _BOXMEDIUMY1),
      FLOAT2D(0.15f, fOffset * _BOXMEDIUMY2)
      );

  case AD_DOWN:
	fRow = 13.0;
    return FLOATaabbox2D(
      FLOAT2D(0.02f, fOffset * _BOXMEDIUMY1),
      FLOAT2D(0.15f, fOffset * _BOXMEDIUMY2)
      );
  }
}

FLOATaabbox2D BoxBack(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, 0.93f),
    FLOAT2D(0.15f, 0.98f));
}

FLOATaabbox2D BoxRefresh(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, 0.85f),
    FLOAT2D(0.15f, 0.88f));
}

FLOATaabbox2D BoxNext(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.85f, 0.95f),
    FLOAT2D(0.98f, 1.0f));
}

FLOATaabbox2D BoxLeftColumn(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, _BOXMEDIUMY1),
    FLOAT2D(0.15f, _BOXMEDIUMY2)
    );
}
FLOATaabbox2D BoxPlayerModel(void)
{
  extern INDEX sam_bWideScreen;
  if (!sam_bWideScreen) {
    return FLOATaabbox2D(FLOAT2D(0.68f, 0.235f), FLOAT2D(0.965f, 0.78f));
  } else {
    return FLOATaabbox2D(FLOAT2D(0.68f, 0.235f), FLOAT2D(0.68f+(0.965f-0.68f)*9.0f/12.0f, 0.78f));
  }
}
FLOATaabbox2D BoxPlayerModelName(void)
{
  return FLOATaabbox2D(FLOAT2D(0.68f, 0.78f), FLOAT2D(0.965f, 0.82f));
}
PIXaabbox2D FloatBoxToPixBox(const CDrawPort *pdp, const FLOATaabbox2D &boxF)
{
  PIX pixW = pdp->GetWidth();
  PIX pixH = pdp->GetHeight();
  return PIXaabbox2D(
    PIX2D(boxF.Min()(1)*pixW, boxF.Min()(2)*pixH),
    PIX2D(boxF.Max()(1)*pixW, boxF.Max()(2)*pixH));
}

FLOATaabbox2D PixBoxToFloatBox(const CDrawPort *pdp, const PIXaabbox2D &boxP)
{
  FLOAT fpixW = pdp->GetWidth();
  FLOAT fpixH = pdp->GetHeight();
  return FLOATaabbox2D(
    FLOAT2D(boxP.Min()(1)/fpixW, (boxP.Min()(2)/fpixH)),
    FLOAT2D(boxP.Max()(1)/fpixW, boxP.Max()(2)/fpixH));
}

extern CFontData _fdTitle;
void SetFontTitle(CDrawPort *pdp)
{
  pdp->SetFont( &_fdTitle);
  pdp->SetTextScaling( (1.25f * pdp->GetWidth() /640 *pdp->dp_fWideAdjustment)*0.7);
  pdp->SetTextAspect(1.0f);
}
extern CFontData _fdBig;
void SetFontBig(CDrawPort *pdp)
{
  pdp->SetFont( &_fdBig);
  pdp->SetTextScaling( (1.0f * pdp->GetWidth() /640 *pdp->dp_fWideAdjustment)*0.7);
  pdp->SetTextAspect(1.0f);
}
extern CFontData _fdMedium;
void SetFontMedium(CDrawPort *pdp)
{
  pdp->SetFont( &_fdMedium);
  pdp->SetTextScaling( (1.0f * pdp->GetWidth() /640 *pdp->dp_fWideAdjustment)*0.7);
  pdp->SetTextAspect(0.75f);
}
void SetFontSmall(CDrawPort *pdp)
{
  pdp->SetFont( _pfdConsoleFont);
  pdp->SetTextScaling( 1.0f);
  pdp->SetTextAspect(1.0f);
}
