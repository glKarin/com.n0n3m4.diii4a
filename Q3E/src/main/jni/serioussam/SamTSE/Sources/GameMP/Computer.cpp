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

#include "StdAfx.h"
#include "LCDDrawing.h"
#include "CompMessage.h"
#include "Render.h"

#ifdef PLATFORM_UNIX
#include <Engine/Base/SDL/SDLEvents.h>
#endif

extern CGame *_pGame;

static const FLOAT tmComputerFade   = 1.0f;  // how many seconds it takes computer to fade in/out
static FLOAT fComputerFadeValue     = 0.0f;  // faded value of computer (0..1)
static CTimerValue tvComputerLast;
static CTimerValue _tvMessageAppear;
static CPlayer *_ppenPlayer = NULL;
FLOAT _fMsgAppearFade = 0.0f;
FLOAT _fMsgAppearDelta = 0.0f;

// player statistics are set here
CTString _strStatsDetails = "";

// mouse cursor position
static PIX2D _vpixMouse;
static PIX2D _vpixExternMouse;
static PIX _pixSliderDragJ = -1;
static PIX _iSliderDragLine = -1;
static PIX _bSliderDragText = FALSE;
// font metrics
static PIX _pixCharSizeI = 1;
static PIX _pixCharSizeJ = 1;
static PIX _pixCharSize2I = 1;
static PIX _pixCharSize2J = 1;

static PIX _pixMarginI = 1;
static PIX _pixMarginJ = 1;
// general geometry data
static FLOAT _fScaling = 1;
static FLOAT _fScaling2 = 1;
static PIX _pixSizeI=0;
static PIX _pixSizeJ=0;
static PIXaabbox2D _boxTitle;
static PIXaabbox2D _boxExit;
static PIXaabbox2D _boxMsgList;
static PIXaabbox2D _boxMsgText;
static PIXaabbox2D _boxMsgImage;
static PIXaabbox2D _boxButton[CMT_COUNT];
static INDEX _ctMessagesOnScreen = 5;
static INDEX _ctTextLinesOnScreen = 20;
static INDEX _ctTextCharsPerRow = 20;

// [Cecil] Maximum width for the message text
static PIX _pixMessageTextWidth = 0;

// [Cecil] Slider width multiplier
static FLOAT _fSliderWidthMul = 1.0f;

// [Cecil] Actual slider width
#define SLIDER_WIDTH PIX(_pixMarginI * 2 * _fSliderWidthMul)

// position of the message list
static INDEX _iFirstMessageOnScreen = -1;
static INDEX _iWantedFirstMessageOnScreen = 0;
static INDEX _iLastActiveMessage = -1;
static INDEX _iActiveMessage = 0;

// message type selected in the buttons list
static enum CompMsgType _cmtCurrentType = (enum CompMsgType)-1;
static enum CompMsgType _cmtWantedType = CMT_INFORMATION;

// current scroll position of message text
static INDEX _iTextLineOnScreen = 0;

// message list cache for messages of current type
static CStaticStackArray<CCompMessage> _acmMessages;

// message image data
static CTextureObject _toPicture;

// text/graphics colors
static COLOR _colLight;
static COLOR _colMedium;
static COLOR _colDark;
static COLOR _colBoxes;

// [Cecil] Use bigger font in computer
INDEX cmp_bBigFont = TRUE;
FLOAT cmp_fBigFontScale = 1.0f;

static void SetFont1(CDrawPort *pdp)
{
  pdp->SetFont(_pfdConsoleFont);
  pdp->SetTextScaling(_fScaling);
  pdp->SetTextAspect(1.0f);
}

static void SetFont2(CDrawPort *pdp)
{
  pdp->SetFont(_pfdDisplayFont);
  pdp->SetTextScaling(_fScaling2);
  pdp->SetTextAspect(1.0f);
}

// [Cecil] Check if should use big message font
static inline BOOL UseBigFont(void) {
  return (cmp_bBigFont && _pixSizeJ >= 720);
};

// [Cecil] Set message font
static void SetMessageFont(CDrawPort *pdp) {
  if (UseBigFont()) {
    _pfdDisplayFont->SetFixedWidth();
    pdp->SetFont(_pfdDisplayFont);

    pdp->SetTextScaling(_fScaling2);
    pdp->SetTextCharSpacing(-5.0f * _fScaling2);
    pdp->SetTextAspect(1.0f);

  } else {
    SetFont1(pdp);
  }
};

// [Cecil] Reset message font
static void ResetMessageFont(void) {
  if (UseBigFont()) {
    _pfdDisplayFont->SetVariableWidth();
  }
};

static COLOR MouseOverColor(const PIXaabbox2D &box, COLOR colNone,
                            COLOR colOff, COLOR colOn)
{
  if (box>=_vpixMouse) {
    return _pGame->LCDBlinkingColor(colOff, colOn);
  } else {
    return colNone;
  }
}

static PIXaabbox2D GetMsgListBox(INDEX i)
{
  PIX pixI0 = _boxMsgList.Min()(1)+_pixMarginI;
  PIX pixI1 = _boxMsgList.Max()(1)-_pixMarginI*3;
  PIX pixJ0 = _boxMsgList.Min()(2)+_pixMarginJ;
  PIX pixDJ = _pixCharSizeJ;
  return PIXaabbox2D(
    PIX2D(pixI0, pixJ0+pixDJ*i),
    PIX2D(pixI1, pixJ0+pixDJ*(i+1)-1));
}

static PIXaabbox2D GetSliderBox(INDEX iFirst, INDEX iVisible, INDEX iTotal,
  PIXaabbox2D boxFull)
{
  FLOAT fSize = ClampUp(FLOAT(iVisible)/iTotal, 1.0f);
  PIX pixFull = boxFull.Size()(2);
  PIX pixSize = PIX(pixFull*fSize);
  pixSize = ClampDn(pixSize, boxFull.Size()(1));
  PIX pixTop = (PIX) (pixFull*(FLOAT(iFirst)/iTotal)+boxFull.Min()(2));
  PIX pixI0 = boxFull.Min()(1);
  PIX pixI1 = boxFull.Max()(1);
  return PIXaabbox2D(PIX2D(pixI0, pixTop), PIX2D(pixI1, pixTop+pixSize));
}

static INDEX SliderPixToIndex(PIX pixOffset, INDEX iVisible, INDEX iTotal, PIXaabbox2D boxFull)
{
  FLOAT fSize = ClampUp(FLOAT(iVisible)/iTotal, 1.0f);
  PIX pixFull = boxFull.Size()(2);
  PIX pixSize = PIX(pixFull*fSize);
  if (pixSize>=boxFull.Size()(2)) {
    return 0;
  }
  return (iTotal*pixOffset)/pixFull;
}

static PIXaabbox2D GetTextSliderSpace(void)
{
  PIX pixSizeI = _boxMsgText.Size()(1);
  PIX pixSizeJ = _boxMsgText.Size()(2);

  PIX pixSliderSizeI = ClampDn(SLIDER_WIDTH, (PIX)5);

  return PIXaabbox2D(
    PIX2D(pixSizeI-pixSliderSizeI, _pixMarginJ*4),
    PIX2D(pixSizeI, pixSizeJ));
}

static PIXaabbox2D GetMsgSliderSpace(void)
{
  PIX pixSizeI = _boxMsgList.Size()(1);
  PIX pixSizeJ = _boxMsgList.Size()(2);

  PIX pixSliderSizeI = ClampDn(SLIDER_WIDTH, (PIX)5);

  return PIXaabbox2D(
    PIX2D(pixSizeI-pixSliderSizeI, 0),
    PIX2D(pixSizeI, pixSizeJ));
}

static PIXaabbox2D GetTextSliderBox(void)
{
  if ((_iActiveMessage >= _acmMessages.Count()) || (_iActiveMessage < 0)) {
    return PIXaabbox2D();
  }
  INDEX ctTextLines = _acmMessages[_iActiveMessage].cm_ctFormattedLines;
  PIX pixSizeI = _boxMsgText.Size()(1);
  PIX pixSizeJ = _boxMsgText.Size()(2);
  return GetSliderBox(
    _iTextLineOnScreen, _ctTextLinesOnScreen, ctTextLines, GetTextSliderSpace());
}

static PIXaabbox2D GetMsgSliderBox(void)
{
  INDEX ctLines = _acmMessages.Count();
  PIX pixSizeI = _boxMsgList.Size()(1);
  PIX pixSizeJ = _boxMsgList.Size()(2);
  return GetSliderBox(
    _iFirstMessageOnScreen, _ctMessagesOnScreen, ctLines, GetMsgSliderSpace());
}

// syncronize message list scrolling to show active message
void SyncScrollWithActive(void)
{
  if (_iActiveMessage<_iFirstMessageOnScreen) {
    _iWantedFirstMessageOnScreen = _iActiveMessage;
  }
  if (_iActiveMessage>_iFirstMessageOnScreen+_ctMessagesOnScreen-1) {
    _iWantedFirstMessageOnScreen = _iActiveMessage-_ctMessagesOnScreen+1;
  }
}

// select next unread message
static void NextUnreadMessage(void)
{
  INDEX i=_iActiveMessage;
  FOREVER {
    i++;
    if (i>=_acmMessages.Count()) {
      i = 0;
    }
    if (i==_iActiveMessage) {
      return;
    }
    if (!_acmMessages[i].cm_bRead) {
      _iActiveMessage = i;
      SyncScrollWithActive();
      return;
    }
  }
}

// select last unread message, or last message if all read
void LastUnreadMessage(void)
{
  BOOL bFound = FALSE;
  for(_iActiveMessage=_acmMessages.Count()-1; _iActiveMessage>=0; _iActiveMessage--) {
    if (!_acmMessages[_iActiveMessage].cm_bRead) {
      bFound = TRUE;
      break;
    }
  }
  if (!bFound) {
    _iActiveMessage = ClampDn((long) _acmMessages.Count()-1, (long) 0);
  }
  SyncScrollWithActive();
}

// go to next/previous message
void PrevMessage(void)
{
  if ((_iActiveMessage >= _acmMessages.Count()) || (_iActiveMessage < 0)) {
    return;
  }
  _iActiveMessage--;
  if (_iActiveMessage<0) {
    _iActiveMessage = 0;
  }
  SyncScrollWithActive();
}

void NextMessage(void)
{
  if ((_iActiveMessage >= _acmMessages.Count()) || (_iActiveMessage < 0)) {
    return;
  }
  _iActiveMessage++;
  if (_iActiveMessage>=_acmMessages.Count()) {
    _iActiveMessage = _acmMessages.Count()-1;
  }
  SyncScrollWithActive();
}

void MessagesUpDn(INDEX ctLines)
{
  INDEX ctMessages = _acmMessages.Count();
  _iWantedFirstMessageOnScreen += ctLines;
  INDEX iMaxFirst = ClampDn((INDEX)0, ctMessages-_ctMessagesOnScreen);
  _iWantedFirstMessageOnScreen = Clamp(_iWantedFirstMessageOnScreen, (INDEX)0, iMaxFirst);
  _iActiveMessage = Clamp(_iActiveMessage, 
    _iWantedFirstMessageOnScreen,
    _iWantedFirstMessageOnScreen+_ctMessagesOnScreen-1);
}

void SelectMessage(INDEX i)
{
  if (_acmMessages.Count()==0) {
    return;
  }
  _iActiveMessage = i;
  if (_iActiveMessage<0) {
    _iActiveMessage = 0;
  }
  if (_iActiveMessage>=_acmMessages.Count()) {
    _iActiveMessage = _acmMessages.Count()-1;
  }
  SyncScrollWithActive();
}

// scroll message text
void MessageTextUp(INDEX ctLines)
{
  _iTextLineOnScreen-=ctLines;
  if (_iTextLineOnScreen<0) {
    _iTextLineOnScreen = 0;
  }
}
void MessageTextDn(INDEX ctLines)
{
  // if no message do nothing
  if ((_iActiveMessage >= _acmMessages.Count()) || (_iActiveMessage < 0)) {
    return;
  }
  // find text lines count
  _acmMessages[_iActiveMessage].PrepareMessage();
  INDEX ctTextLines = _acmMessages[_iActiveMessage].cm_ctFormattedLines;
  // calculate maximum value for first visible line
  INDEX iFirstLine = ctTextLines-_ctTextLinesOnScreen;
  if (iFirstLine<0) {
    iFirstLine = 0;
  }

  // increment
  _iTextLineOnScreen+=ctLines;
  if (_iTextLineOnScreen>iFirstLine) {
    _iTextLineOnScreen = iFirstLine;
  }
}

void MessageTextUpDn(INDEX ctLines)
{
  if (ctLines>0) {
    MessageTextDn(ctLines);
  } else if (ctLines<0) {
    MessageTextUp(-ctLines);
  }
}

// mark current message as read
void MarkCurrentRead(void)
{
  if ((_iActiveMessage >= _acmMessages.Count()) || (_iActiveMessage < 0)) {
    return;
  }
  // if running in background
  if (_pGame->gm_csComputerState == CS_ONINBACKGROUND) {
    // do nothing
    return;
  }
  ASSERT(_ppenPlayer!=NULL);
  if (_ppenPlayer==NULL) {
    return;
  }
  // if already read
  if (_acmMessages[_iActiveMessage].cm_bRead) {
    // do nothing
    return;
  }
  // mark as read
  _ppenPlayer->m_ctUnreadMessages--;
  _acmMessages[_iActiveMessage].MarkRead();
}

// update scroll position for message list
static void UpdateFirstOnScreen(void)
{
  if (_iFirstMessageOnScreen==_iWantedFirstMessageOnScreen) {
    return;
  }
  _iFirstMessageOnScreen=_iWantedFirstMessageOnScreen;
  ASSERT(
    _iFirstMessageOnScreen>=0&&
    _iFirstMessageOnScreen<=_acmMessages.Count());
  _iFirstMessageOnScreen = Clamp(_iFirstMessageOnScreen, INDEX(0), _acmMessages.Count());

  // for each message
  for(INDEX i=0; i<_acmMessages.Count(); i++) {
    CCompMessage &cm = _acmMessages[i];
    // if on screen
    if (i>=_iWantedFirstMessageOnScreen
      &&i<_iWantedFirstMessageOnScreen+_ctMessagesOnScreen) {
      // load
      cm.PrepareMessage();
    // if not on screen
    } else {
      // unload
      cm.UnprepareMessage();
    }
  }
}

// update current active message category
static void UpdateType(BOOL bForce=FALSE)
{
  if (_cmtCurrentType==_cmtWantedType && !bForce) {
    return;
  }

  // cleare message cache
  _acmMessages.Clear();
  // for each player's message
  CDynamicStackArray<CCompMessageID> &acmiMsgs = _ppenPlayer->m_acmiMessages;
  for(INDEX i=0; i<acmiMsgs.Count(); i++) {
    CCompMessageID &cmi = acmiMsgs[i];
    // if it is of given type
    if (cmi.cmi_cmtType == _cmtWantedType) {
      // add it to cache
      CCompMessage &cm = _acmMessages.Push();
      cm.SetMessage(&cmi);
    }
  }
  if (!bForce) {
    _cmtCurrentType=_cmtWantedType;
    _iFirstMessageOnScreen = -1;
    _iWantedFirstMessageOnScreen = 0;
    _iActiveMessage = 0;
    _iLastActiveMessage = -2;
    _iTextLineOnScreen = 0;
    LastUnreadMessage();
    UpdateFirstOnScreen();
  }
}

static void UpdateMessageAppearing(void)
{
  if (_iLastActiveMessage!=_iActiveMessage) {
    _pShell->Execute("FreeUnusedStock();");   // make sure user doesn't overflow memory
    _iTextLineOnScreen = 0;
    _iLastActiveMessage=_iActiveMessage;
    _tvMessageAppear = _pTimer->GetHighPrecisionTimer();
  }
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  _fMsgAppearDelta = (tvNow-_tvMessageAppear).GetSeconds();

  if (fComputerFadeValue<0.99f) {
    _tvMessageAppear = _pTimer->GetHighPrecisionTimer();
    _fMsgAppearDelta = 0.0f;
  }
  _fMsgAppearFade = Clamp(_fMsgAppearDelta/0.5f, 0.0f,1.0f);
}

// update screen geometry
static void UpdateSize(CDrawPort *pdp)
{
  // get screen size
  PIX pixSizeI = pdp->GetWidth();
  PIX pixSizeJ = pdp->GetHeight();

  // remember new size
  _pixSizeI = pixSizeI;
  _pixSizeJ = pixSizeJ;

  // determine scaling
  _fScaling = 1.0f;
  _fScaling2 = 1.0f;
  _fSliderWidthMul = 1.0f;

  CFontData *pfd = _pfdConsoleFont;
  INDEX iSubHeight = 0;

  // Too small
  if (pixSizeJ<384) {
    _fScaling = 1.0f;
    _fScaling2 = pixSizeJ/480.0f;

  // [Cecil] Too big
  } else if (UseBigFont()) {
    pfd = _pfdDisplayFont;
    iSubHeight = 3;

    FLOAT fMul = (pixSizeJ / 720.0f) * Clamp(cmp_fBigFontScale, 0.1f, 3.0f);
    _fScaling *= fMul;
    _fScaling2 *= fMul;
    _fSliderWidthMul = 1.5f;
  }

  // [Cecil] Message font sizes
  _pixCharSizeI = (pfd->fd_pixCharWidth  + pfd->fd_pixCharSpacing) * _fScaling;
  _pixCharSizeJ = (pfd->fd_pixCharHeight + pfd->fd_pixLineSpacing - iSubHeight) * _fScaling;

  // [Cecil] Other computer text sizes
  pfd = _pfdConsoleFont;
  _pixCharSize2I = (pfd->fd_pixCharWidth  + pfd->fd_pixCharSpacing) * _fScaling2;
  _pixCharSize2J = (pfd->fd_pixCharHeight + pfd->fd_pixLineSpacing) * _fScaling2;

  _pixMarginI = (PIX) (5*_fScaling2);
  _pixMarginJ = (PIX) (5*_fScaling2);
  PIX pixBoxMarginI = (PIX) (10*_fScaling2);
  PIX pixBoxMarginJ = (PIX) (10*_fScaling2);

  PIX pixJ0Dn = (PIX) (pixBoxMarginJ);
  PIX pixJ1Up = (PIX) (pixJ0Dn+_pixCharSize2J+_pixMarginI*2);
  PIX pixJ1Dn = (PIX) (pixJ1Up+pixBoxMarginJ);
  PIX pixJ2Up = (PIX) (pixJ1Dn+_pixCharSize2J*6*2+pixBoxMarginJ);
  PIX pixJ2Dn = (PIX) (pixJ2Up+pixBoxMarginJ);
  PIX pixJ3Up = (PIX) (_pixSizeJ-pixBoxMarginJ);

  PIX pixI0Rt = (PIX) (pixBoxMarginI);
  PIX pixI1Lt = (PIX) (pixI0Rt+_pixCharSize2I*20+pixBoxMarginI);
  PIX pixI1Rt = (PIX) (pixI1Lt+pixBoxMarginI);
  PIX pixI2Lt = (PIX) (_pixSizeI/2-pixBoxMarginI/2);
  PIX pixI2Rt = (PIX) (_pixSizeI/2+pixBoxMarginI/2);
  PIX pixI4Lt = (PIX) (_pixSizeI-pixBoxMarginI);
  PIX pixI3Rt = (PIX) (pixI4Lt-pixBoxMarginI*2-_pixCharSize2I*10);
  PIX pixI3Lt = (PIX) (pixI3Rt-pixBoxMarginI);

  // calculate box sizes
  _boxTitle = PIXaabbox2D( PIX2D(0, pixJ0Dn-1), PIX2D(pixI3Lt, pixJ1Up));
  _boxExit  = PIXaabbox2D( PIX2D( pixI3Rt, pixJ0Dn-1), PIX2D(_pixSizeI, pixJ1Up));
  PIX pixD = 5;
  PIX pixH = (pixJ2Up-pixJ1Dn-pixD*(CMT_COUNT-1))/CMT_COUNT;
  INDEX i;
  for( i=0; i<CMT_COUNT; i++) {
    _boxButton[i] = PIXaabbox2D( 
      PIX2D(0,       pixJ1Dn+(pixH+pixD)*i),
      PIX2D(pixI1Lt, pixJ1Dn+(pixH+pixD)*i+pixH));
  }
  _boxMsgList = PIXaabbox2D( PIX2D(pixI1Rt, pixJ1Dn), PIX2D(pixI4Lt, pixJ2Up));

  if (GetSP()->sp_bCooperative) {
    _boxMsgText = PIXaabbox2D( PIX2D(pixI2Rt, pixJ2Dn), PIX2D(pixI4Lt, pixJ3Up));
    _boxMsgImage= PIXaabbox2D( PIX2D(pixI0Rt, pixJ2Dn), PIX2D(pixI2Lt, pixJ3Up));
  } else {
    _boxMsgText = PIXaabbox2D( PIX2D(pixI0Rt, pixJ2Dn), PIX2D(pixI4Lt, pixJ3Up));
    _boxMsgImage= PIXaabbox2D();
  }

  FLOAT fSlideSpeed = Max(_pixSizeI, _pixSizeJ*2);
  FLOAT fGroup0 = ClampDn((1-fComputerFadeValue)*fSlideSpeed-_pixSizeJ, 0.0f);
  FLOAT fGroup1 = (1-fComputerFadeValue)*fSlideSpeed;
  // animate box positions
  _boxTitle -= PIX2D( fGroup1, 0);
  _boxExit  += PIX2D( fGroup1, 0);
  for( i=0; i<CMT_COUNT; i++) {
    FLOAT fOffs = ClampDn(fGroup1-(CMT_COUNT-i)*_pixMarginJ*10, 0.0f);
    _boxButton[i] -= PIX2D(fOffs, 0);
  }
  _boxMsgList -= PIX2D(0, fGroup0);
  _boxMsgText += PIX2D(fGroup0, 0);
  _boxMsgImage+= PIX2D(0, fGroup0);

  // [Cecil] Calculate maximum width for the message text
  _pixMessageTextWidth = (_boxMsgText.Size()(1) - _pixMarginI * 2 - SLIDER_WIDTH);

  _ctMessagesOnScreen  = (_boxMsgList.Size()(2) - _pixMarginJ*2)                 / _pixCharSizeJ;
  _ctTextCharsPerRow   = _pixMessageTextWidth / _pixCharSizeI;
  _ctTextLinesOnScreen = (_boxMsgText.Size()(2) - _pixMarginJ*2 - _pixMarginJ*4) / _pixCharSizeJ;
}


static char *_astrButtonTexts[CMT_COUNT];

// print message type buttons
void PrintButton(CDrawPort *pdp, INDEX iButton)
{
  CDrawPort dpButton(pdp, _boxButton[iButton]);
  if (!dpButton.Lock()) {
    return;
  } 
  _pGame->LCDSetDrawport(&dpButton);
  _pGame->LCDRenderCompGrid();
  _pGame->LCDRenderClouds2();
  _pGame->LCDScreenBoxOpenLeft(_colBoxes);

  SetFont2(&dpButton);

  // count messages
  INDEX ctTotal=0;
  INDEX ctRead=0;
  CDynamicStackArray<CCompMessageID> &acmiMsgs = _ppenPlayer->m_acmiMessages;
  {for(INDEX i=0; i<acmiMsgs.Count(); i++) {
    CCompMessageID &cmi = acmiMsgs[i];
    if (cmi.cmi_cmtType==iButton) {
      ctTotal++;
      if (cmi.cmi_bRead) {
        ctRead++;
      }
    }
  }}

  INDEX ctUnread = ctTotal-ctRead;

  // prepare color
  COLOR col = _colMedium;
  if (iButton==_cmtCurrentType) {
    col = _colLight;
  }
  col = MouseOverColor(_boxButton[iButton], col, _colDark, _colLight);

  // prepare string
  CTString str;
  if (ctUnread==0) {
    str = _astrButtonTexts[iButton];
  } else {
    str.PrintF("%s (%d)", _astrButtonTexts[iButton], ctUnread);
  }

  // print it
  dpButton.PutTextR( str, _boxButton[iButton].Size()(1)-_pixMarginI, _pixCharSize2J/2+1, col);

  dpButton.Unlock();
}

// print title
void PrintTitle(CDrawPort *pdp)
{
  SetFont2(pdp);
  CTString strTitle;
  strTitle.PrintF(TRANSV("NETRICSA v2.01 - personal version for: %s"), 
    (const char *) _ppenPlayer->GetPlayerName());
  pdp->PutText( strTitle, _pixMarginI*3, _pixMarginJ-2*_fScaling2+1, _colMedium);
}

// print exit button
void PrintExit(CDrawPort *pdp)
{
  SetFont2(pdp);
  pdp->PutTextR( TRANS("Exit"), _boxExit.Size()(1)-_pixMarginI*3, _pixMarginJ-2*_fScaling2+1, 
    MouseOverColor(_boxExit, _colMedium, _colDark, _colLight));
}

// print list of messages
void PrintMessageList(CDrawPort *pdp)
{
  PIX pixTextX = _pixMarginI;
  PIX pixYLine = _pixMarginJ;
  SetMessageFont(pdp); // [Cecil]

  INDEX iFirst = _iFirstMessageOnScreen;
  INDEX iLast = Min(INDEX(_iFirstMessageOnScreen+_ctMessagesOnScreen), _acmMessages.Count())-1;
  if (iFirst>iLast) {
    pdp->PutText( TRANS("no messages"), pixTextX, pixYLine, _colDark);
  }
  for(INDEX i=iFirst; i<=iLast; i++) {
    COLOR col = _colMedium;
    if (_acmMessages[i].cm_bRead) {
      col = _colDark;
    }
    if (i==_iActiveMessage) {
      col = _colLight;
    }
    if (GetMsgListBox(i-_iFirstMessageOnScreen)>=_vpixMouse) {
      col = _pGame->LCDBlinkingColor(_colLight, _colMedium);
    }
    pdp->PutText( _acmMessages[i].cm_strSubject, pixTextX, pixYLine, col);
    pixYLine+=_pixCharSizeJ;
  }

  PIXaabbox2D boxSliderSpace = GetMsgSliderSpace();
  _pGame->LCDDrawBox(0,0,boxSliderSpace, _colBoxes);
  PIXaabbox2D boxSlider = GetMsgSliderBox();
  COLOR col = _colBoxes;
  PIXaabbox2D boxSliderTrans = boxSlider;
  boxSliderTrans+=_boxMsgList.Min();
  if (boxSliderTrans>=_vpixMouse) {
    col = _pGame->LCDBlinkingColor(_colLight, _colDark);
  }
  pdp->Fill( boxSlider.Min()(1)+2,  boxSlider.Min()(2)+2,
             boxSlider.Size()(1)-4, boxSlider.Size()(2)-4, col);

  ResetMessageFont(); // [Cecil]
}

// print text of current message
void PrintMessageText(CDrawPort *pdp)
{
  if (_acmMessages.Count()==0 ||
      _iActiveMessage>=_acmMessages.Count()||
      fComputerFadeValue<0.99f) {
    return;
  }

  SetFont2(pdp);

  // print subject
  CTString strSubject0;
  CTString strSubject1;
  CTString strSubject2;
  //strSubject.PrintF("%g", _fMsgAppearFade);
  const char *strSubject = _acmMessages[_iActiveMessage].cm_strSubject;
  INDEX ctSubjectLen = strlen(strSubject);
  INDEX ctToPrint = int(_fMsgAppearDelta*20.0f);
  for (INDEX iChar=0; iChar<ctSubjectLen; iChar++) {
    char strChar[2];
    strChar[0] = strSubject[iChar];
    strChar[1] = 0;
    if (iChar>ctToPrint) {
      NOTHING;
    } else if (iChar==ctToPrint) {
      strSubject2+=strChar;
    } else if (iChar==ctToPrint-1) {
      strSubject1+=strChar;
    } else {
      strSubject0+=strChar;
    }
  }
  // [Cecil] Better text width calculation
  PIX pixWidth0 = IRender::GetTextWidth(pdp, strSubject0);
  PIX pixWidth1 = IRender::GetTextWidth(pdp, strSubject1);
  pdp->PutText(strSubject0, _pixMarginI, _pixMarginJ-1, _colMedium);
  pdp->PutText(strSubject1, _pixMarginI+pixWidth0, _pixMarginJ-1, LerpColor( _colLight, _colMedium, 0.5f));
  pdp->PutText(strSubject2, _pixMarginI+pixWidth0+pixWidth1, _pixMarginJ-1, _colLight);

  pdp->DrawLine(0, PIX(_pixMarginJ*4), _boxMsgText.Size()(1), PIX(_pixMarginJ*4), _colBoxes);

  // fill in fresh player statistics
  if (strncmp(_acmMessages[_iActiveMessage].cm_strText, "$STAT", 5)==0) {
    _ppenPlayer->GetStats(_strStatsDetails, CST_DETAIL, _ctTextCharsPerRow);
  }

  // [Cecil] Set font before formatting
  SetMessageFont(pdp);

  // [Cecil] Load and format text based on the box width
  _acmMessages[_iActiveMessage].PrepareMessage();
  _acmMessages[_iActiveMessage].Format(pdp, _pixMessageTextWidth);

  INDEX ctLineToPrint = int(_fMsgAppearDelta*20.0f);
  // print it
  PIX pixJ = _pixMarginJ*4;
  for (INDEX iLine = _iTextLineOnScreen; 
    iLine<_iTextLineOnScreen+_ctTextLinesOnScreen;
    iLine++) {
    INDEX iPrintLine = iLine-_iTextLineOnScreen;
    if (iPrintLine>ctLineToPrint) {
      continue;
    }
    COLOR col = LerpColor( _colLight, _colMedium, Clamp( FLOAT(ctLineToPrint-iPrintLine)/3, 0.0f, 1.0f));
    pdp->PutText(_acmMessages[_iActiveMessage].GetLine(iLine),
      _pixMarginI, pixJ, col);
    pixJ+=_pixCharSizeJ;
  }

  PIXaabbox2D boxSliderSpace = GetTextSliderSpace();
  _pGame->LCDDrawBox(0,0,boxSliderSpace, _colBoxes);
  PIXaabbox2D boxSlider = GetTextSliderBox();
  COLOR col = _colBoxes;
  PIXaabbox2D boxSliderTrans = boxSlider;
  boxSliderTrans+=_boxMsgText.Min();
  if (boxSliderTrans>=_vpixMouse) {
    col = _pGame->LCDBlinkingColor(_colLight, _colDark);
  }
  pdp->Fill( boxSlider.Min()(1)+2,  boxSlider.Min()(2)+2,
             boxSlider.Size()(1)-4, boxSlider.Size()(2)-4, col);

  ResetMessageFont(); // [Cecil]
}


void RenderMessagePicture(CDrawPort *pdp)
{
  CCompMessage &cm = _acmMessages[_iActiveMessage];
  // try to
  try {
    // load image
    _toPicture.SetData_t(cm.cm_fnmPicture);
    ((CTextureData*)_toPicture.GetData())->Force(TEX_CONSTANT);
  // if failed
  } catch (const char *strError) {
    // report error
    CPrintF("Cannot load '%s':\n%s\n", (const char *) (CTString&)cm.cm_fnmPicture, (const char *)strError);
    // do nothing
    return;
  }

  // get image and box sizes
  PIX pixImgSizeI = _toPicture.GetWidth();
  PIX pixImgSizeJ = _toPicture.GetHeight();
  PIXaabbox2D boxPic(PIX2D(_pixMarginI, _pixMarginJ),
      PIX2D(_boxMsgImage.Size()(1)-_pixMarginI, _boxMsgImage.Size()(2)-_pixMarginJ));
  PIX pixBoxSizeI = boxPic.Size()(1);
  PIX pixBoxSizeJ = boxPic.Size()(2);
  PIX pixCenterI = _boxMsgImage.Size()(1)/2;
  PIX pixCenterJ = _boxMsgImage.Size()(2)/2;
  // find image stretch to fit in box
  FLOAT fStretch = Min(FLOAT(pixBoxSizeI)/pixImgSizeI, FLOAT(pixBoxSizeJ)/pixImgSizeJ);
  // draw the image
  pdp->PutTexture(&_toPicture,
    PIXaabbox2D(
      PIX2D(pixCenterI-pixImgSizeI*fStretch/2, pixCenterJ-pixImgSizeJ*fStretch/2),
      PIX2D(pixCenterI+pixImgSizeI*fStretch/2, pixCenterJ+pixImgSizeJ*fStretch/2)));
}


void RenderMessageStats(CDrawPort *pdp)
{
  CSessionProperties *psp = (CSessionProperties *)_pNetwork->GetSessionProperties();
  ULONG ulLevelMask = psp->sp_ulLevelsMask;
  //INDEX iLevel = -1;
  if (psp->sp_bCooperative) {
    extern void RenderMap( CDrawPort *pdp, ULONG ulLevelMask, CProgressHookInfo *pphi);
    if (pdp->Lock()) {
      // get sizes
      PIX pixSizeI = pdp->GetWidth();
      PIX pixSizeJ = pdp->GetHeight();
      // clear bcg
      pdp->Fill( 1, 1, pixSizeI-2, pixSizeJ-2, C_BLACK|CT_OPAQUE);
      // render the map if not fading
      COLOR colFade = _pGame->LCDFadedColor(C_WHITE|255);
      if( (colFade&255) == 255) {
        RenderMap( pdp, ulLevelMask, NULL);
      }
      pdp->Unlock();
    }
  }
}


extern void RenderMessageModel(CDrawPort *pdp, const CTString &strModel);

// draw image of current message
void RenderMessageImage(CDrawPort *pdp)
{
  if (!GetSP()->sp_bCooperative) {
    return;
  }
  // if no message
  if (_acmMessages.Count()==0 || fComputerFadeValue<0.99f) {
    // render empty
    _pGame->LCDRenderClouds2();
    _pGame->LCDScreenBox(_colBoxes);
    return;
  }
  CCompMessage &cm = _acmMessages[_iActiveMessage];

  if (cm.cm_itImage == CCompMessage::IT_STATISTICS) {
    _pGame->LCDRenderCompGrid();
  }
  _pGame->LCDRenderClouds2();
  _pGame->LCDScreenBox(_colBoxes);

  // if no image 
  if (cm.cm_itImage == CCompMessage::IT_NONE) {
    // do nothing
    return;
  } else if (cm.cm_itImage == CCompMessage::IT_PICTURE) {
    RenderMessagePicture(pdp);
  } else if (cm.cm_itImage == CCompMessage::IT_STATISTICS) {
    RenderMessageStats(pdp);
  } else if (cm.cm_itImage == CCompMessage::IT_MODEL) {
    RenderMessageModel(pdp, cm.cm_strModel);
  } else {
    ASSERT(FALSE);
  }
}

// find first group with some unread message
static BOOL FindGroupWithUnread(void)
{
  CDynamicStackArray<CCompMessageID> &acmiMsgs = _ppenPlayer->m_acmiMessages;
  for(INDEX i=acmiMsgs.Count()-1; i>=0; i--) {
    CCompMessageID &cmi = acmiMsgs[i];
    // if it unread
    if (!cmi.cmi_bRead) {
      _cmtWantedType = cmi.cmi_cmtType;
      return TRUE;
    }
  }
  // if none found, select statistics
  _cmtWantedType = CMT_STATISTICS;
  return FALSE;
}

static void ComputerOn(void)
{
  // init button names
  _astrButtonTexts[CMT_INFORMATION ] = TRANS("tactical data");
  _astrButtonTexts[CMT_BACKGROUND  ] = TRANS("strategic data");
  _astrButtonTexts[CMT_WEAPONS     ] = TRANS("weapons");
  _astrButtonTexts[CMT_ENEMIES     ] = TRANS("enemies");
  _astrButtonTexts[CMT_STATISTICS  ] = TRANS("statistics");

  _iFirstMessageOnScreen = -1;
  _iWantedFirstMessageOnScreen = 0;
  _iActiveMessage = 0;
  _cmtCurrentType = (enum CompMsgType)-1;
  _cmtWantedType = CMT_INFORMATION;
  _acmMessages.Clear();

  ASSERT(_ppenPlayer!=NULL);
  if (_ppenPlayer==NULL) {
    return;
  }

  // fill in player statistics
  _ppenPlayer->GetStats(_strStatsDetails, CST_DETAIL, _ctTextCharsPerRow);

  // if end of level
  if (_ppenPlayer->m_bEndOfLevel || _pNetwork->IsGameFinished()) {
    // select statistics
    _cmtWantedType = CMT_STATISTICS;
  // if not end of level
  } else {
    // find group with some unread messages
    FindGroupWithUnread();
  }
}

static void ComputerOff(void)
{
  _acmMessages.Clear();
  _pShell->Execute("FreeUnusedStock();");
}

static void ExitRequested(void)
{
  // if end of game
  if ((_ppenPlayer!=NULL && _ppenPlayer->m_bEndOfGame) || _pNetwork->IsGameFinished()) {
    // if in single player
    if (GetSP()->sp_bSinglePlayer) {
      // request app to show high score
      _pShell->Execute("sam_bMenuHiScore=1;");
    }
    // hard turn off
    _pGame->gm_csComputerState = CS_OFF;
    fComputerFadeValue = 0.0f;
    ComputerOff();
    cmp_ppenPlayer = NULL;
    // stop current game
    _pGame->StopGame();
  // if not end of level
  } else {
    // if can be rendered on second display
    if (cmp_ppenDHPlayer!=NULL) {
      // clear pressed keys
      _pInput->ClearInput();
      // just switch to background fast
      _pGame->gm_csComputerState = CS_ONINBACKGROUND;
      cmp_ppenPlayer = NULL;
    // if no second display
    } else {
      // start turning off
      _pGame->gm_csComputerState = CS_TURNINGOFF;
    }
  }
  // turn off end of level for player
  if (_ppenPlayer!=NULL) {
    _ppenPlayer->m_bEndOfLevel = FALSE;
  }
}

void CGame::ComputerMouseMove(PIX pixX, PIX pixY)
{
  _vpixMouse(1) += pixX-_vpixExternMouse(1);
  _vpixMouse(2) += pixY-_vpixExternMouse(2);
  _vpixExternMouse(1) = pixX;
  _vpixExternMouse(2) = pixY;

  // if dragging
  if (_pixSliderDragJ>=0) {
    PIX pixDelta = _vpixMouse(2)-_pixSliderDragJ;

    if (_bSliderDragText) {
      if (_iActiveMessage<_acmMessages.Count()) {
        INDEX ctTextLines = _acmMessages[_iActiveMessage].cm_ctFormattedLines;
        INDEX iWantedLine = _iSliderDragLine+
          SliderPixToIndex(pixDelta, _ctTextLinesOnScreen, ctTextLines, GetTextSliderSpace());
        MessageTextUpDn(iWantedLine-_iTextLineOnScreen);
      }
    } else {
      INDEX ctLines = _acmMessages.Count();
      INDEX iWantedLine = _iSliderDragLine+
        SliderPixToIndex(pixDelta, _ctMessagesOnScreen, ctLines, GetMsgSliderSpace());
      MessagesUpDn(iWantedLine-_iFirstMessageOnScreen);
    }
  }
}


void CGame::ComputerKeyDown(MSG msg)
{
  static BOOL bRDown = FALSE;
  // if computer is not active
  if (_pGame->gm_csComputerState!=CS_ON && _pGame->gm_csComputerState!=CS_TURNINGON) {
    // do nothing
    return;
  }

  // if escape pressed
  if (msg.message==WM_KEYDOWN && msg.wParam==VK_ESCAPE) {
    ExitRequested();
  }

  // if right mouse pressed
  if (msg.message==WM_RBUTTONDOWN || msg.message==WM_RBUTTONDBLCLK) {
    bRDown = TRUE;
  }
  // if right mouse released
  if (bRDown && msg.message==WM_RBUTTONUP) {
    bRDown = FALSE;
    // mark current message as read
    MarkCurrentRead();
    // find a group with first unread message
    BOOL bHasUnread = FindGroupWithUnread();
    // if some
    if (bHasUnread) {
      // select first unread message in it
      NextUnreadMessage();
    } else {
      ExitRequested();
    }
  }
  
  if (msg.message==WM_KEYDOWN) {
    switch (msg.wParam) {
    // change message types on number keys
    case '1': _cmtWantedType = CMT_INFORMATION ; return;
    case '2': _cmtWantedType = CMT_WEAPONS     ; return;
    case '3': _cmtWantedType = CMT_ENEMIES     ; return;
    case '4': _cmtWantedType = CMT_BACKGROUND  ; return;
    case '5': _cmtWantedType = CMT_STATISTICS  ; return;
    // go to next unread
    case 'U':
    case VK_SPACE:
      NextUnreadMessage(); return;
    // scroll message list
    case 219: PrevMessage(); return;
    case 221: NextMessage(); return;
    // mark current message as read and go to next
    case VK_RETURN: MarkCurrentRead(); NextUnreadMessage(); return;
    // scroll message text
    case VK_UP:   MessageTextUp(1); return;
    case VK_DOWN: MessageTextDn(1); return;
    case VK_PRIOR:MessageTextUp(_ctTextLinesOnScreen-1); return;
    case VK_NEXT: MessageTextDn(_ctTextLinesOnScreen-1); return;
    };
  }

  // if left mouse pressed
  if (msg.message==WM_LBUTTONDOWN || msg.message==WM_LBUTTONDBLCLK) {
    BOOL bOverMsgSlider = FALSE;
    // if over slider
    {PIXaabbox2D boxSlider = GetTextSliderBox();
    PIXaabbox2D boxSliderTrans = boxSlider;
    boxSliderTrans+=_boxMsgText.Min();
    if (boxSliderTrans>=_vpixMouse) {
      bOverMsgSlider = TRUE;
      // start dragging
      _bSliderDragText = TRUE;
      _pixSliderDragJ=_vpixMouse(2);
      _iSliderDragLine = _iTextLineOnScreen;
    }}

    // if over slider
    {PIXaabbox2D boxSlider = GetMsgSliderBox();
    PIXaabbox2D boxSliderTrans = boxSlider;
    boxSliderTrans+=_boxMsgList.Min();
    if (boxSliderTrans>=_vpixMouse) {
      // start dragging
      _bSliderDragText = FALSE;
      _pixSliderDragJ=_vpixMouse(2);
      _iSliderDragLine = _iFirstMessageOnScreen;
    }}
    // if over some button
    {for(INDEX i=0; i<CMT_COUNT; i++) {
      if (_boxButton[i]>=_vpixMouse) {
        // switch to that message type
        _cmtWantedType = (CompMsgType)i;
      }
    }}
    // if over some message
    {for(INDEX i=0; i<_ctMessagesOnScreen; i++) {
      if (GetMsgListBox(i)>=_vpixMouse && !bOverMsgSlider) {
        // switch to that message
        SelectMessage(_iFirstMessageOnScreen+i);
      }
    }}
  }

  // if left mouse released
  if (msg.message==WM_LBUTTONUP) {
    // stop dragging
    _pixSliderDragJ=-1;
    // if over exit
    if (_boxExit>=_vpixMouse) {
      // exit
      ExitRequested();
    }
  }
}

void CGame::ComputerRender(CDrawPort *pdp)
{
  // if playing a demo
  if (_pNetwork->IsPlayingDemo()) {
    // never call computer
    cmp_ppenPlayer = NULL;
  }

  // disable netricsa for non-local players
  if (cmp_ppenPlayer!=NULL && !_pNetwork->IsPlayerLocal(cmp_ppenPlayer)) {
    cmp_ppenPlayer = NULL;
  }
  if (cmp_ppenDHPlayer!=NULL && !_pNetwork->IsPlayerLocal(cmp_ppenDHPlayer)) {
    cmp_ppenDHPlayer = NULL;
  }
  if (cmp_ppenDHPlayer!=NULL && !pdp->IsDualHead()) {
    cmp_ppenDHPlayer = NULL;
  }

  // initially - no player
  _ppenPlayer=NULL;

  // if player calls computer
  if (cmp_ppenPlayer!=NULL) {
    // use that player
    _ppenPlayer = cmp_ppenPlayer;
    // if computer is on in background
    if (_pGame->gm_csComputerState==CS_ONINBACKGROUND) {
      // just toggle to on
      _pGame->gm_csComputerState=CS_ON;
      // find group with some unread messages
      FindGroupWithUnread();
      // force reinit
      _cmtCurrentType = (enum CompMsgType)-1;
    }
  // if using dualhead to render computer on second display
  } else if (cmp_ppenDHPlayer!=NULL) {
    // use that player
    _ppenPlayer = cmp_ppenDHPlayer;
    // clear dualhead request - it has to be reinitialized every frame
    cmp_ppenDHPlayer = NULL;

    // if viewing statistics
    if (_cmtWantedType == CMT_STATISTICS) {
      // fill in fresh player statistics
      _ppenPlayer->GetStats(_strStatsDetails, CST_DETAIL, _ctTextCharsPerRow);
      // force updating
      UpdateType(TRUE);
    }

    // if computer is not on or on in background
    if (_pGame->gm_csComputerState!=CS_ON && _pGame->gm_csComputerState!=CS_ONINBACKGROUND) {
      // switch on fast
      ComputerOn();
      fComputerFadeValue = 1.0f;
      _pGame->gm_csComputerState = CS_ONINBACKGROUND;
      cmp_bInitialStart = FALSE; // end of eventual initial start
    }

    // if should update to new message
    if (cmp_bUpdateInBackground) {
      cmp_bUpdateInBackground = FALSE;
      FindGroupWithUnread();
      // force reinit
      _cmtCurrentType = (enum CompMsgType)-1;
    }
  }

  // if no player
  if (_ppenPlayer==NULL) {
    // make sure computer is off
    _pGame->gm_csComputerState=CS_OFF;
    // do nothing more
    return;
  }

  // if computer is not active
  if (_pGame->gm_csComputerState==CS_OFF) {
    // just remember time
    tvComputerLast = _pTimer->GetHighPrecisionTimer();
    // if a player wants computer
    if (_ppenPlayer!=NULL) {
      // start turning on
      _pGame->gm_csComputerState=CS_TURNINGON;
      ComputerOn();
    } else {
      return;
    }
  }
 
  // calculate up-down speed to be independent of refresh speed
  CTimerValue tvNow   = _pTimer->GetHighPrecisionTimer();
  CTimerValue tvDelta = tvNow - tvComputerLast;
  tvComputerLast      = tvNow;
  FLOAT fFadeSpeed    = (FLOAT)(tvDelta.GetSeconds() / tmComputerFade);

  // if computer is dropping down
  if( _pGame->gm_csComputerState==CS_TURNINGON) {
    // move it down
    fComputerFadeValue += fFadeSpeed;
    // if finished moving
    if( fComputerFadeValue>1.0f) {
      // stop
      fComputerFadeValue = 1.0f;
      _pGame->gm_csComputerState   = CS_ON;
      cmp_bInitialStart  = FALSE; // end of eventual initial start
    }
  }
  // if computer is pulling up
  if( _pGame->gm_csComputerState==CS_TURNINGOFF) {
    // move it up
    fComputerFadeValue -= fFadeSpeed;
    // if finished moving
    if( fComputerFadeValue<0.0f) {
      // stop
      fComputerFadeValue = 0.0f;
      _pGame->gm_csComputerState   = CS_OFF;
      ComputerOff();
#ifdef PLATFORM_UNIX
      if (_pInput != NULL) // rcg02042003 hack for SDL vs. Win32.
        _pInput->ClearRelativeMouseMotion();
#endif
      cmp_ppenPlayer = NULL;
      // exit computer
      return;
    }
  }

  // safety check -> do not proceed if no player
  if (_ppenPlayer==NULL) {
    return;
  }

  // lock drawport
  CDrawPort dpComp(pdp, FALSE);
  if(!dpComp.Lock()) {
    // do nothing
    return;
  }

  // if in fullscreen
  CDisplayMode dmCurrent;
  _pGfx->GetCurrentDisplayMode(dmCurrent);
  if (dmCurrent.IsFullScreen() && dmCurrent.IsDualHead()) {
    // clamp mouse pointer
    _vpixMouse(1) = Clamp(_vpixMouse(1), (PIX)0, dpComp.GetWidth());
    _vpixMouse(2) = Clamp(_vpixMouse(2), (PIX)0, dpComp.GetHeight());
  // if in window
  } else {
    // use same mouse pointer as windows
    _vpixMouse = _vpixExternMouse;
    // if dualhead
    if (dpComp.dp_MinI>0) {
      // offset by half screen
      _vpixMouse(1) -= dpComp.GetWidth();
    }
    // if widescreen
    if (dpComp.dp_MinJ>0) {
      // offset by screen top
      _vpixMouse(2) -= dpComp.dp_MinJ;
    }
  }

  TIME tmOld     = _pTimer->CurrentTick();
  FLOAT fLerpOld = _pTimer->GetLerpFactor();

  FLOAT fSec = tvNow.GetSeconds();
  TIME tmTick = floor(fSec/_pTimer->TickQuantum)*_pTimer->TickQuantum;
  FLOAT fLerp = (fSec-tmTick)/_pTimer->TickQuantum;
  _pTimer->SetCurrentTick(tmTick);
  _pTimer->SetLerp(fLerp);

  LCDPrepare(1.0f);//ClampUp(fComputerFadeValue*10,1.0f));
  LCDSetDrawport(&dpComp);
  // if initial start
  if (cmp_bInitialStart) {
    // do not allow game to show through
    dpComp.Fill(C_BLACK|255);
  // if normal start
  } else {
    // fade over game view
    dpComp.Fill(LCDFadedColor(C_BLACK|255));
  }
  dpComp.FillZBuffer(1.0f);

  // update screen geometry
  UpdateSize(&dpComp);
  // update scroll positions
  UpdateType();
  UpdateFirstOnScreen();
  // check for message change
  UpdateMessageAppearing();
  // mark current message as read
  MarkCurrentRead();

  // get current time and alpha value
  //FLOAT tmNow = (FLOAT)tvNow.GetSeconds();
  //ULONG ulA   = NormFloatToByte(fComputerFadeValue);

  _colLight  = LCDFadedColor(C_WHITE|255);
  #ifdef FIRST_ENCOUNTER  // First Encounter
  _colMedium = LCDFadedColor(SE_COL_GREEN_LIGHT|255);
  _colDark   = LCDFadedColor(LerpColor(SE_COL_GREEN_DARK, SE_COL_GREEN_LIGHT, 0.5f)|255);
  _colBoxes  = LCDFadedColor(LerpColor(SE_COL_GREEN_DARK, SE_COL_GREEN_LIGHT, 0.5f)|255);    
  #else // Second Encounter
  _colMedium = LCDFadedColor(SE_COL_BLUE_LIGHT|255);
  _colDark   = LCDFadedColor(LerpColor(SE_COL_BLUE_DARK, SE_COL_BLUE_LIGHT, 0.5f)|255);
  _colBoxes  = LCDFadedColor(LerpColor(SE_COL_BLUE_DARK, SE_COL_BLUE_LIGHT, 0.5f)|255);    
  #endif

  // background
  LCDRenderCloudsForComp();
//  dpComp.DrawLine( 0, pixSizeJ-1, pixSizeI, pixSizeJ-1, C_GREEN|ulA);

  // all done
  dpComp.Unlock();

  // print title
  CDrawPort dpTitle(&dpComp, _boxTitle);
  if (dpTitle.Lock()) {
    LCDSetDrawport(&dpTitle);
    LCDRenderCompGrid();
    LCDRenderClouds2();
    LCDScreenBoxOpenLeft(_colBoxes);
    PrintTitle(&dpTitle);
    dpTitle.Unlock();
  }

  // print exit button
  CDrawPort dpExit(&dpComp, _boxExit);
  if (dpExit.Lock()) {
    LCDSetDrawport(&dpExit);
    LCDRenderCompGrid();
    LCDRenderClouds2();
    LCDScreenBoxOpenRight(_colBoxes);
    PrintExit(&dpExit);
    dpExit.Unlock();
  }

  // print buttons
  for (INDEX i=0; i<CMT_COUNT; i++) {
    PrintButton(&dpComp, i);
  }
  // print list of messages
  CDrawPort dpMsgList(&dpComp, _boxMsgList);
  if (dpMsgList.Lock()) {
    LCDSetDrawport(&dpMsgList);
    LCDRenderCompGrid();
    LCDRenderClouds2();
    LCDScreenBox(_colBoxes);
    PrintMessageList(&dpMsgList);
    dpMsgList.Unlock();
  }
  // print text of current message
  CDrawPort dpMsgText(&dpComp, _boxMsgText);
  if (dpMsgText.Lock()) {
    LCDSetDrawport(&dpMsgText);
    LCDRenderCompGrid();
    LCDRenderClouds2();
    LCDScreenBox(_colBoxes);
    PrintMessageText(&dpMsgText);
    dpMsgText.Unlock();
  }

  // [Cecil] Skip if invalid box size
  if (!_boxMsgImage.IsEmpty()) {
    // draw image of current message
    CDrawPort dpMsgImage(&dpComp, _boxMsgImage);

    if (dpMsgImage.Lock()) {
      LCDSetDrawport(&dpMsgImage);
      RenderMessageImage(&dpMsgImage);
      dpMsgImage.Unlock();
    }
  }

  // render mouse pointer on top of everything else
  if (_pGame->gm_csComputerState != CS_ONINBACKGROUND) {
    if (dpComp.Lock()) {
      LCDSetDrawport(&dpComp);
      LCDDrawPointer(_vpixMouse(1), _vpixMouse(2));
      dpComp.Unlock();
    }
  }

  _pTimer->SetCurrentTick(tmOld);
  _pTimer->SetLerp(fLerpOld);
}

void CGame::ComputerForceOff()
{
  cmp_ppenPlayer=NULL;
  cmp_ppenDHPlayer=NULL;
  _pGame->gm_csComputerState = CS_OFF;
  fComputerFadeValue = 0.0f;
  _ppenPlayer = NULL;
#ifdef PLATFORM_UNIX
  if (_pInput != NULL) // rcg02042003 hack for SDL vs. Win32.
    _pInput->ClearRelativeMouseMotion();
#endif
}

