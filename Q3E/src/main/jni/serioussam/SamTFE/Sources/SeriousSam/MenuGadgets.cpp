/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#include "SeriousSam/StdH.h"
#include <Engine/Base/KeyNames.h>
#include "MenuPrinting.h"
#include <GameMP/LCDDrawing.h>
#include <Engine/CurrentVersion.h>
#include "LevelInfo.h"
#include "VarList.h"

extern CFontData _fdBig;
extern CFontData _fdMedium;
extern CFontData _fdSmall;
extern CSoundData *_psdSelect;

CMenuGadget *_pmgLastActivatedGadget = NULL;

//##############################################################################################################################3
//##############################################################################################################################3
//##############################################################################################################################3
__extern FLOAT _fGlobalTopAdjuster = 0.15f;
__extern FLOAT _fGlobalInfoAdjuster = 0.05f;
__extern FLOAT _fGlobalProfileFOVAdjuster = 90.0f;
ENGINE_API extern FLOAT _fWeaponFOVAdjuster;
//##############################################################################################################################3
//##############################################################################################################################3
//##############################################################################################################################3

extern CSoundData *_psdPress;
extern PIX  _pixCursorPosI;
extern PIX  _pixCursorPosJ;
extern INDEX sam_bWideScreen;

BOOL _bDefiningKey = FALSE;
BOOL _bEditingString = FALSE;


CMenuGadget::CMenuGadget( void)
{
  mg_pmgLeft = NULL;
  mg_pmgRight = NULL;
  mg_pmgUp = NULL;
  mg_pmgDown = NULL;

  mg_bVisible = TRUE;
  mg_bEnabled = TRUE;
  mg_bLabel = FALSE;
  mg_bFocused = FALSE;
  mg_iInList = -1;    // not in list
}

void CMenuGadget::OnActivate( void)
{
  NOTHING;
}

// return TRUE if handled
BOOL CMenuGadget::OnKeyDown( int iVKey)
{
  // if return pressed
  if( iVKey==VK_RETURN || iVKey==VK_LBUTTON) {
    // activate
    OnActivate();
    // key is handled
    return TRUE;
  }
  // key is not handled
  return FALSE;
}


BOOL CMenuGadget::OnChar(MSG msg)
{
  // key is not handled
  return FALSE;
}


void CMenuGadget::OnSetFocus( void)
{
  mg_bFocused = TRUE;
  if( !IsSeparator())
  {
    PlayMenuSound(_psdSelect);
    IFeel_PlayEffect("Menu_select");
  }
}

void CMenuGadget::OnKillFocus( void)
{
  mg_bFocused = FALSE;
}

void CMenuGadget::Appear( void)
{
  mg_bVisible = TRUE;
}

void CMenuGadget::Disappear( void)
{
  mg_bVisible = FALSE;
  mg_bFocused = FALSE;
}

void CMenuGadget::Think( void)
{
}
void CMenuGadget::OnMouseOver(PIX pixI, PIX pixJ)
{
}

// get current color for the gadget
COLOR CMenuGadget::GetCurrentColor(void)
{
  // use normal colors
  COLOR colUnselected = _pGame->LCDGetColor(C_GREEN, "unselected");
  COLOR colSelected   = _pGame->LCDGetColor(C_WHITE, "selected");
  // if disabled
  if (!mg_bEnabled) {
    // use a bit darker colors
    colUnselected = _pGame->LCDGetColor(C_dGREEN, "disabled unselected");
    colSelected   = _pGame->LCDGetColor(C_GRAY, "disabled selected");
    // if label
    if (mg_bLabel) {
      // use white
      colUnselected = colSelected = _pGame->LCDGetColor(C_WHITE, "label");
    }
  }
  // use unselected color
  COLOR colRet = colUnselected;
  // if selected
  if( mg_bFocused) {
    // oscilate towards selected color
    FLOAT tmNow = _pTimer->GetHighPrecisionTimer().GetSeconds();
    colRet = LerpColor( (colUnselected>>1)&0x7F7F7F7F, colSelected, sin(tmNow*10.0f)*0.5f+0.5f);
  }

  return colRet|CT_OPAQUE;
}

void CMenuGadget::Render( CDrawPort *pdp)
{
}

void CMGTitle::Render( CDrawPort *pdp)
{
  SetFontTitle(pdp);

  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixI = box.Center()(1);
  PIX pixJ = box.Min()(2);

  pdp->PutTextC( mg_strText, pixI, pixJ, _pGame->LCDGetColor(C_WHITE|CT_OPAQUE, "title"));
}

CMGButton::CMGButton( void)
{
  mg_pActivatedFunction = NULL;
  mg_iIndex = 0;
  mg_iCenterI = 0;
  mg_iTextMode = 1;
  mg_bfsFontSize = BFS_MEDIUM;
  mg_iCursorPos = -1;
  mg_bRectangle = FALSE;
  mg_bMental = FALSE;
}


void CMGButton::SetText( CTString strNew)
{
  mg_strText = strNew;
}


void CMGButton::OnActivate( void)
{
  if( mg_pActivatedFunction!=NULL && mg_bEnabled)
  {
    PlayMenuSound(_psdPress);
    IFeel_PlayEffect("Menu_press");
    _pmgLastActivatedGadget = this;
    (*mg_pActivatedFunction)();
  }
}


BOOL CMGVarButton::IsSeparator(void)
{
  if( mg_pvsVar==NULL) return FALSE;
  return mg_pvsVar->vs_bSeparator;
}


BOOL CMGVarButton::IsEnabled(void)
{
  return( _gmRunningGameMode==GM_NONE
       || mg_pvsVar==NULL
       || mg_pvsVar->vs_bCanChangeInGame);
}

 
void CMGButton::Render( CDrawPort *pdp)
{
  if (mg_bfsFontSize==BFS_LARGE) {
    SetFontBig(pdp);
  } else if (mg_bfsFontSize==BFS_MEDIUM) {
    SetFontMedium(pdp);
  } else {
    ASSERT(mg_bfsFontSize==BFS_SMALL);
    SetFontSmall(pdp);
  }
  pdp->SetTextMode(mg_iTextMode);

  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  COLOR col = GetCurrentColor();
  if(mg_bEditing) {
    col = _pGame->LCDGetColor(C_GREEN|0xFF, "editing");
  }
  
  COLOR colRectangle = col;
  if( mg_bHighlighted) {
    col = _pGame->LCDGetColor(C_WHITE|0xFF, "hilited");
    if( !mg_bFocused) {
      colRectangle = _pGame->LCDGetColor(C_WHITE|0xFF, "hilited rectangle");
    }
  }
  if (mg_bMental) {
    FLOAT tmIn   = 0.2f;
    FLOAT tmOut  = 1.0f;
    FLOAT tmFade = 0.1f;
    FLOAT tmExist = tmFade+tmIn+tmFade;
    FLOAT tmTotal = tmFade+tmIn+tmFade+tmOut;
    
    FLOAT tmTime = _pTimer->GetHighPrecisionTimer().GetSeconds();
    FLOAT fFactor = 1;
    if (tmTime>0.1f) {
      tmTime = fmod(tmTime, tmTotal);
      fFactor = CalculateRatio(tmTime, 0, tmExist, tmFade/tmExist, tmFade/tmExist);
    }
    col = (col&~0xFF)|INDEX(0xFF*fFactor);
  }

  if( mg_bRectangle) {
    // put border
    const PIX pixLeft   = box.Min()(1);
    const PIX pixUp     = box.Min()(2)-3;
    const PIX pixWidth  = box.Size()(1)+1;
    const PIX pixHeight = box.Size()(2);
    pdp->DrawBorder( pixLeft, pixUp, pixWidth, pixHeight, colRectangle);
  }

  if( mg_bEditing) {
    // put border
    PIX pixLeft   = box.Min()(1);
    PIX pixUp     = box.Min()(2)-3;
    PIX pixWidth  = box.Size()(1)+1;
    PIX pixHeight = box.Size()(2);
    if (mg_strLabel!="") {
      pixLeft = (PIX) (box.Min()(1)+box.Size()(1)*0.55f);
      pixWidth = (PIX) (box.Size()(1)*0.45f+1);
    }
    pdp->Fill( pixLeft, pixUp, pixWidth, pixHeight, _pGame->LCDGetColor(C_dGREEN|0x40, "edit fill"));
  }


  INDEX iCursor = mg_iCursorPos;

  // print text
  if (mg_strLabel!="") {
    PIX pixIL = (PIX) (box.Min()(1)+box.Size()(1)*0.45f);
    PIX pixIR = (PIX) (box.Min()(1)+box.Size()(1)*0.55f);
    PIX pixJ  = (PIX) (box.Min()(2));

    pdp->PutTextR( mg_strLabel, pixIL, pixJ, col);
    pdp->PutText( mg_strText, pixIR, pixJ, col);
  } else {
    CTString str = mg_strText;
    if (pdp->dp_FontData->fd_bFixedWidth) {
      str = str.Undecorated();
      //INDEX iLen = str.Length();
      INDEX iMaxLen = ClampDn(box.Size()(1)/(pdp->dp_pixTextCharSpacing+pdp->dp_FontData->fd_pixCharWidth), (PIX)1);
      if (iCursor>=iMaxLen) {
        str.TrimRight(iCursor);
        str.TrimLeft(iMaxLen);
        iCursor = iMaxLen;
      } else {
        str.TrimRight(iMaxLen);
      }
    }
         if( mg_iCenterI==-1) pdp->PutText(  str, box.Min()(1),    box.Min()(2), col);
    else if( mg_iCenterI==+1) pdp->PutTextR( str, box.Max()(1),    box.Min()(2), col);
    else                      pdp->PutTextC( str, box.Center()(1), box.Min()(2), col);
  }

  // put cursor if editing
  if( mg_bEditing && (((ULONG)(_pTimer->GetRealTimeTick()*2))&1)) {
    PIX pixX = box.Min()(1) + GetCharOffset( pdp, iCursor);
    if (mg_strLabel!="") {
      pixX += (PIX) (box.Size()(1)*0.55f);
    }
    PIX pixY = (PIX) (box.Min()(2));
    if (!pdp->dp_FontData->fd_bFixedWidth) {
      pixY -= (PIX) (pdp->dp_fTextScaling *2);
    }
    pdp->PutText( "|", pixX, pixY, _pGame->LCDGetColor(C_WHITE|0xFF, "editing cursor"));
  }
}


PIX CMGButton::GetCharOffset( CDrawPort *pdp, INDEX iCharNo)
{
  if (pdp->dp_FontData->fd_bFixedWidth) {
    return ((PIX) ((pdp->dp_FontData->fd_pixCharWidth+pdp->dp_pixTextCharSpacing)*(iCharNo-0.5f)));
  }
  CTString strCut(mg_strText);
  strCut.TrimLeft( strlen(mg_strText)-iCharNo);
  PIX pixFullWidth = pdp->GetTextWidth(mg_strText);
  PIX pixCutWidth  = pdp->GetTextWidth(strCut);
  // !!!! not implemented for different centering
  return pixFullWidth-pixCutWidth;
}

CMGModel::CMGModel(void)
{
  mg_fFloorY = 0;
}

void CMGModel::Render( CDrawPort *pdp)
{
  // if no model
  if (mg_moModel.GetData()==NULL) {
    // just render text
    mg_strText = TRANS("No model");
    CMGButton::Render(pdp);
    return;
  }

  // get position on screen
  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  CDrawPort dpModel(pdp, box);
  dpModel.Lock();
  dpModel.FillZBuffer(1.0f);

  _pGame->LCDSetDrawport(&dpModel);
  // clear menu here
  dpModel.Fill(C_BLACK|255);
  _pGame->LCDRenderClouds1();
  _pGame->LCDRenderClouds2();

  // prepare projection
  CRenderModel rmRenderModel;
  CPerspectiveProjection3D pr;
  //pr.FOVL() = sam_bWideScreen ? AngleDeg(45.0f) : AngleDeg(30.0f);
  pr.FOVL() = AngleDeg(_fGlobalProfileFOVAdjuster);

  pr.ScreenBBoxL() = FLOATaabbox2D(
    FLOAT2D(0.0f, 0.0f),
    FLOAT2D((float)dpModel.GetWidth(), (float)dpModel.GetHeight())
  );
  pr.AspectRatioL() = 1.0f;
  pr.FrontClipDistanceL() = 0.3f;
  pr.ViewerPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
  
  // initialize remdering
  CAnyProjection3D apr;
  apr = pr;
  BeginModelRenderingView(apr, &dpModel);
  rmRenderModel.rm_vLightDirection = FLOAT3D( 0.2f, -0.2f, -0.2f);

  // if model needs floor
  if (mg_moFloor.GetData()!=NULL) {
    // set floor's position
    CPlacement3D pl = mg_plModel;
    pl.pl_OrientationAngle = ANGLE3D(0,0,0);
    pl.pl_PositionVector = mg_plModel.pl_PositionVector;
    pl.pl_PositionVector(2) += mg_fFloorY;
    rmRenderModel.SetObjectPlacement(pl);

    // render the floor
    rmRenderModel.rm_colLight   = C_WHITE;
    rmRenderModel.rm_colAmbient = C_WHITE;
    mg_moFloor.SetupModelRendering( rmRenderModel);
    mg_moFloor.RenderModel( rmRenderModel);
  }

  // set model's position
  CPlacement3D pl;
  pl.pl_OrientationAngle = mg_plModel.pl_OrientationAngle;
  pl.pl_PositionVector = mg_plModel.pl_PositionVector;
  extern FLOAT sam_fPlayerOffset;
  pl.pl_PositionVector(3) += sam_fPlayerOffset;
  rmRenderModel.SetObjectPlacement(pl);

  // render the model
  rmRenderModel.rm_colLight   = LerpColor(C_BLACK, C_WHITE, 0.4f)|CT_OPAQUE;
  rmRenderModel.rm_colAmbient = LerpColor(C_BLACK, C_WHITE, 0.2f)|CT_OPAQUE;
  mg_moModel.SetupModelRendering( rmRenderModel);
  FLOATplane3D plFloorPlane = FLOATplane3D( FLOAT3D( 0.0f, 1.0f, 0.0f), 
    mg_plModel.pl_PositionVector(2)+mg_fFloorY);
  FLOAT3D vShadowLightDir = FLOAT3D( -0.2f, -0.4f, -0.6f);
  CPlacement3D plLightPlacement = CPlacement3D( 
    mg_plModel.pl_PositionVector+
      vShadowLightDir*mg_plModel.pl_PositionVector(3)*5, 
    ANGLE3D(0,0,0));
  mg_moModel.RenderShadow( rmRenderModel, plLightPlacement, 200.0f, 200.0f, 1.0f, plFloorPlane);
  mg_moModel.RenderModel( rmRenderModel);
  EndModelRenderingView();

  _pGame->LCDScreenBox(_pGame->LCDGetColor(C_GREEN, "model box")|GetCurrentColor());

  dpModel.Unlock();

  pdp->Unlock();
  pdp->Lock();
  _pGame->LCDSetDrawport(pdp);

  // print the model name
  {
    PIXaabbox2D box = FloatBoxToPixBox(pdp, BoxPlayerModelName());
    COLOR col = GetCurrentColor();

    PIX pixI = box.Min()(1);
    PIX pixJ = box.Max()(2);
    pdp->PutText( mg_strText, pixI, pixJ, col);
  }
}


// ------- Edit gadget implementation
CMGEdit::CMGEdit(void)
{
  mg_pstrToChange   = NULL;
  mg_ctMaxStringLen = 70;
  Clear();
}


void CMGEdit::Clear(void)
{
  mg_iCursorPos = 0;
  mg_bEditing   = FALSE;
  _bEditingString = FALSE;
}


void CMGEdit::OnActivate(void)
{
  if (!mg_bEnabled) {
    return;
  }
  ASSERT( mg_pstrToChange != NULL);
  PlayMenuSound( _psdPress);
  IFeel_PlayEffect("Menu_press");
  SetText( mg_strText);
  mg_iCursorPos = strlen(mg_strText);
  mg_bEditing = TRUE;
  _bEditingString = TRUE;
}


// focus lost
void CMGEdit::OnKillFocus(void)
{
  // go out of editing mode
  if( mg_bEditing) {
    OnKeyDown(VK_RETURN);
    Clear();
  }
  // proceed
  CMenuGadget::OnKillFocus();
}


// helper function for deleting char(s) from string
static void Key_BackDel( CTString &str, INDEX &iPos, BOOL bShift, BOOL bRight)
{
  // do nothing if string is empty
  INDEX ctChars = strlen(str);
  if( ctChars==0) return;
  if( bRight && iPos<ctChars) {  // DELETE key
    if( bShift) {
      // delete to end of line
      str.TrimRight(iPos);
    } else {
      // delete only one char
      str.DeleteChar(iPos);
    }  
  }
  if( !bRight && iPos>0) {       // BACKSPACE key
    if( bShift) {
      // delete to start of line
      str.TrimLeft(ctChars-iPos);
      iPos=0;
    } else {
      // delete only one char
      str.DeleteChar(iPos-1);
      iPos--;
    }  
  }
}

// key/mouse button pressed
BOOL CMGEdit::OnKeyDown( int iVKey)
{
  // if not in edit mode
  if( !mg_bEditing) {
    // behave like normal gadget
    return CMenuGadget::OnKeyDown(iVKey);
  }

  // finish editing?
  BOOL bShift = GetKeyState(VK_SHIFT) & 0x8000;
  switch( iVKey) {
  case VK_UP: case VK_DOWN:        
  case VK_RETURN:  case VK_LBUTTON: *mg_pstrToChange = mg_strText;  Clear(); OnStringChanged();  break;
  case VK_ESCAPE:  case VK_RBUTTON:  mg_strText = *mg_pstrToChange; Clear(); OnStringCanceled(); break;
  case VK_LEFT:    if( mg_iCursorPos > 0)                  mg_iCursorPos--;  break;
  case VK_RIGHT:   if( mg_iCursorPos < static_cast<INDEX>(strlen(mg_strText))) mg_iCursorPos++;  break;
  case VK_HOME:    mg_iCursorPos = 0;                   break;
  case VK_END:     mg_iCursorPos = strlen(mg_strText);  break;
  case VK_BACK:    Key_BackDel( mg_strText, mg_iCursorPos, bShift, FALSE);  break;
  case VK_DELETE:  Key_BackDel( mg_strText, mg_iCursorPos, bShift, TRUE);   break;
  default:  break; // ignore all other special keys
  }

  // key is handled
  return TRUE;
}


// char typed
BOOL CMGEdit::OnChar( MSG msg)
{
  // if not in edit mode
  if( !mg_bEditing) {
    // behave like normal gadget
    return CMenuGadget::OnChar(msg);
  }
  // only chars are allowed
  const INDEX ctFullLen  = mg_strText.Length();
  const INDEX ctNakedLen = mg_strText.LengthNaked();
  mg_iCursorPos = Clamp( mg_iCursorPos, (INDEX)0, ctFullLen);
  int iVKey = msg.wParam;
  if( isprint(iVKey) && ctNakedLen<=mg_ctMaxStringLen) {
    mg_strText.InsertChar( mg_iCursorPos, (char)iVKey);
    mg_iCursorPos++;
  }
  // key is handled
  return TRUE;
}


void CMGEdit::Render( CDrawPort *pdp)
{
  if( mg_bEditing) {
    mg_iTextMode = -1;
  } else if( mg_bFocused) {
    mg_iTextMode = 0;
  } else {
    mg_iTextMode = 1;
  }
  if (mg_strText=="" && !mg_bEditing) {
    if (mg_bfsFontSize==BFS_SMALL) {
      mg_strText="*";
    } else {
      mg_strText=TRANS("<none>");
    }
    CMGButton::Render(pdp);
    mg_strText="";
  } else {
    CMGButton::Render(pdp);
  }
}
void CMGEdit::OnStringChanged(void)
{
}
void CMGEdit::OnStringCanceled(void)
{
}


void CMGArrow::Render( CDrawPort *pdp)
{
  SetFontMedium(pdp);

  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  COLOR col = GetCurrentColor();

  CTString str;
  if (mg_adDirection==AD_NONE) {
    str = "???";
  } else if (mg_adDirection==AD_UP) {
    str = TRANS("Page Up");
  } else if (mg_adDirection==AD_DOWN) {
    str = TRANS("Page Down");
  } else {
    ASSERT(FALSE);
  }
  PIX pixI = box.Min()(1);
  PIX pixJ = box.Min()(2);
  pdp->PutText( str, pixI, pixJ, col);
}

void CMGArrow::OnActivate(void)
{
  if (mg_adDirection==AD_UP) {
    pgmCurrentMenu->ScrollList(-3);
  } else if (mg_adDirection==AD_DOWN) {
    pgmCurrentMenu->ScrollList(+3);
  }
}

#define HSCOLUMNS 6
CTString strHighScores[HIGHSCORE_COUNT+1][HSCOLUMNS];
FLOAT afI[HSCOLUMNS] = {
  0.12f, 0.15f, 0.6f, 0.7f, 0.78f, 0.9f
};

void CMGHighScore::Render( CDrawPort *pdp)
{
  SetFontMedium(pdp);

  COLOR colHeader = _pGame->LCDGetColor(C_GREEN|255, "hiscore header");
  COLOR colData = _pGame->LCDGetColor(C_mdGREEN|255, "hiscore data");
  COLOR colLastSet = _pGame->LCDGetColor(C_mlGREEN|255, "hiscore last set");
  INDEX iLastSet = _pGame->gm_iLastSetHighScore;

  CTString strText;
  
  strHighScores[0][0] = TRANS("No.");
  strHighScores[0][1] = TRANS("Player Name");
  strHighScores[0][2] = TRANS("Difficulty");
  strHighScores[0][3] = TRANS("Time");
  strHighScores[0][4] = TRANS("Kills");
  strHighScores[0][5] = TRANS("Score");

  {for (INDEX i=0; i<HIGHSCORE_COUNT; i++) {
    switch(_pGame->gm_ahseHighScores[i].hse_gdDifficulty) {
    default:
      ASSERT(FALSE);
#ifdef PLATFORM_FREEBSD
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
    case (CSessionProperties::GameDifficulty)-100:
#pragma clang diagnostic pop
#else
	case (CSessionProperties::GameDifficulty)-100:
#endif
      strHighScores[i+1][1] = "---";
      continue;
      break;
    case CSessionProperties::GD_TOURIST:
      strHighScores[i+1][2] = TRANS("Tourist");
      break;
    case CSessionProperties::GD_EASY:
      strHighScores[i+1][2] = TRANS("Easy");
      break;
    case CSessionProperties::GD_NORMAL:
      strHighScores[i+1][2] = TRANS("Normal");
      break;
    case CSessionProperties::GD_HARD:
      strHighScores[i+1][2] = TRANS("Hard");
      break;
    case CSessionProperties::GD_EXTREME:
      strHighScores[i+1][2] = TRANS("Serious");
      break;
    case CSessionProperties::GD_EXTREME+1:
      strHighScores[i+1][2] = TRANS("Mental");
      break;
    }
    strHighScores[i+1][0].PrintF("%d", i+1);
    strHighScores[i+1][1] = _pGame->gm_ahseHighScores[i].hse_strPlayer;
    strHighScores[i+1][3] = TimeToString(_pGame->gm_ahseHighScores[i].hse_tmTime);
    strHighScores[i+1][4].PrintF("%03d", _pGame->gm_ahseHighScores[i].hse_ctKills);
    strHighScores[i+1][5].PrintF("%9d", _pGame->gm_ahseHighScores[i].hse_ctScore);
  }}

  PIX pixJ = (PIX) (pdp->GetHeight()*0.25f);
  {for (INDEX iRow=0; iRow<HIGHSCORE_COUNT+1; iRow++) {
    COLOR col = (iRow==0) ? colHeader : colData;
    if (iLastSet!=-1 && iRow-1==iLastSet) {
      col = colLastSet;
    }
    {for (INDEX iColumn=0; iColumn<HSCOLUMNS; iColumn++) {
      PIX pixI = (PIX) (pdp->GetWidth()*afI[iColumn]);
      if (iColumn==1) {
        pdp->PutText(strHighScores[iRow][iColumn], pixI, pixJ, col);
      } else {
        pdp->PutTextR(strHighScores[iRow][iColumn], pixI, pixJ, col);
      }
    }}
    if (iRow==0) {
      pixJ+=(PIX) (pdp->GetHeight()*0.06f);
    } else {
      pixJ+=(PIX) (pdp->GetHeight()*0.04f);
    }
  }}
}

// ------- Trigger button implementation
INDEX GetNewLoopValue( int iVKey, INDEX iCurrent, INDEX ctMembers)
{
  INDEX iPrev = (iCurrent+ctMembers-1)%ctMembers;
  INDEX iNext = (iCurrent+1)%ctMembers;
  // return and right arrow set new text
  if( iVKey == VK_RETURN || iVKey==VK_LBUTTON || iVKey==VK_RIGHT )
  {
    return iNext;
  }
  // left arrow and backspace sets prev text
  else if( (iVKey == VK_BACK || iVKey==VK_RBUTTON) || (iVKey == VK_LEFT) )
  {
    return iPrev;
  }
  return iCurrent;
}

CMGTrigger::CMGTrigger( void) 
{
  mg_pOnTriggerChange = NULL;
  mg_iCenterI = 0;
  mg_bVisual = FALSE;
}

void CMGTrigger::ApplyCurrentSelection(void)
{
  mg_iSelected = Clamp(mg_iSelected, (INDEX)0, mg_ctTexts-1);
  mg_strValue = mg_astrTexts[ mg_iSelected];
}


void CMGTrigger::OnSetNextInList(int iVKey)
{
  if( mg_pPreTriggerChange != NULL) {
    mg_pPreTriggerChange(mg_iSelected);
  }

  mg_iSelected = GetNewLoopValue( iVKey, mg_iSelected, mg_ctTexts);
  mg_strValue = mg_astrTexts[ mg_iSelected];

  if( mg_pOnTriggerChange != NULL) {
    (*mg_pOnTriggerChange)(mg_iSelected);
  }
}


BOOL CMGTrigger::OnKeyDown( int iVKey)
{
  if( (iVKey == VK_RETURN || iVKey==VK_LBUTTON) ||
      (iVKey == VK_LEFT) ||
      (iVKey == VK_BACK || iVKey==VK_RBUTTON) ||
      (iVKey == VK_RIGHT) )
  {
    // key is handled
    if( mg_bEnabled) OnSetNextInList(iVKey);
    return TRUE;
  }
  // key is not handled
  return FALSE;
}


void CMGTrigger::Render( CDrawPort *pdp)
{
  SetFontMedium(pdp);

  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixIL = (PIX) (box.Min()(1)+box.Size()(1)*0.45f);
  PIX pixIR = (PIX) (box.Min()(1)+box.Size()(1)*0.55f);
  PIX pixJ  = (PIX) (box.Min()(2));

  COLOR col = GetCurrentColor();
  if (!mg_bVisual || mg_strValue=="") {
    CTString strValue = mg_strValue;
    if (mg_bVisual) {
      strValue = TRANS("none");
    }
    if (mg_iCenterI==-1) {
      pdp->PutText( mg_strLabel, box.Min()(1), pixJ, col);
      pdp->PutTextR( strValue, box.Max()(1), pixJ, col);
    } else {
      pdp->PutTextR( mg_strLabel, pixIL, pixJ, col);
      pdp->PutText( strValue, pixIR, pixJ, col);
    }
  } else {
    CTString strLabel = mg_strLabel+": ";
    pdp->PutText(strLabel, box.Min()(1), pixJ, col);
    CTextureObject to;
    try {
      to.SetData_t(mg_strValue);
      //CTextureData *ptd = (CTextureData *)to.GetData();
      PIX pixSize = box.Size()(2);
      PIX pixCX = box.Max()(1)-pixSize/2;
      PIX pixCY = box.Center()(2);
      pdp->PutTexture( &to, PIXaabbox2D( 
        PIX2D(pixCX-pixSize/2, pixCY-pixSize/2), 
        PIX2D(pixCX-pixSize/2+pixSize, pixCY-pixSize/2+pixSize)), C_WHITE|255);
    } catch (const char *strError) {
      CPrintF("%s\n", (const char *)strError);
    }
    to.SetData(NULL);
  }
}

CMGSlider::CMGSlider()
{
  mg_iMinPos = 0;
  mg_iMaxPos = 16;
  mg_iCurPos = 8;
  mg_pOnSliderChange = NULL;
  mg_fFactor = 1.0f;
}

void CMGSlider::ApplyCurrentPosition( void)
{
  mg_iCurPos = Clamp(mg_iCurPos, mg_iMinPos, mg_iMaxPos);
  FLOAT fStretch = FLOAT(mg_iCurPos)/(mg_iMaxPos-mg_iMinPos);
  mg_fFactor = fStretch;

  if (mg_pOnSliderChange!=NULL) {
    mg_pOnSliderChange(mg_iCurPos);
  }
}

void CMGSlider::ApplyGivenPosition( INDEX iMin, INDEX iMax, INDEX iCur)
{
  mg_iMinPos = iMin;
  mg_iMaxPos = iMax;
  mg_iCurPos = iCur;
  ApplyCurrentPosition();
}


BOOL CMGSlider::OnKeyDown( int iVKey)
{
  // if scrolling left
  if( (iVKey==VK_BACK || iVKey==VK_LEFT) && mg_iCurPos>mg_iMinPos) {
    mg_iCurPos --;
    ApplyCurrentPosition();
    return TRUE;
  // if scrolling right
  } else if( (iVKey==VK_RETURN || iVKey==VK_RIGHT) && mg_iCurPos<mg_iMaxPos) {
    mg_iCurPos++;
    ApplyCurrentPosition();
    return TRUE;
  // if lmb pressed
  } else if (iVKey==VK_LBUTTON) {
    // get position of slider box on screen
    PIXaabbox2D boxSlider = GetSliderBox();
    // if mouse is within
    if (boxSlider>=PIX2D(_pixCursorPosI, _pixCursorPosJ)) {
      // set new position exactly where mouse pointer is
      FLOAT fRatio = FLOAT(_pixCursorPosI-boxSlider.Min()(1))/boxSlider.Size()(1);
      fRatio = (fRatio-0.01f)/(0.99f-0.01f);
      fRatio = Clamp(fRatio, 0.0f, 1.0f);
      mg_iCurPos = (INDEX) (fRatio*(mg_iMaxPos-mg_iMinPos) + mg_iMinPos);
      ApplyCurrentPosition();
      return TRUE;
    }
  }
  return CMenuGadget::OnKeyDown( iVKey);
}


PIXaabbox2D CMGSlider::GetSliderBox(void)
{
  extern CDrawPort *pdp;
  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixIR = (PIX) (box.Min()(1)+box.Size()(1)*0.55f);
  PIX pixJ  = (PIX) (box.Min()(2));
  PIX pixJSize  = (PIX) (box.Size()(2)*0.95f);
  PIX pixISizeR = (PIX) (box.Size()(1)*0.45f);
  if( sam_bWideScreen) pixJSize++;
  return PIXaabbox2D( PIX2D(pixIR+1, pixJ+1), PIX2D(pixIR+pixISizeR-2, pixJ+pixJSize-2));
}


void CMGSlider::Render( CDrawPort *pdp)
{
  SetFontMedium(pdp);

  // get geometry
  COLOR col = GetCurrentColor();
  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixIL = (PIX) (box.Min()(1)+box.Size()(1)*0.45f);
  PIX pixIR = (PIX) (box.Min()(1)+box.Size()(1)*0.55f);
  PIX pixJ  = (PIX) (box.Min()(2));
  PIX pixJSize  = (PIX) (box.Size()(2)*0.95f);
  PIX pixISizeR = (PIX) (box.Size()(1)*0.45f);
  if( sam_bWideScreen) pixJSize++;

  // print text left of slider
  pdp->PutTextR( mg_strText, pixIL, pixJ, col);

  // draw box around slider
  PIXaabbox2D aabbox( PIX2D(pixIR+1, pixJ), PIX2D(pixIR+pixISizeR-2, pixJ+pixJSize-2));
  _pGame->LCDDrawBox(0, -1, aabbox, _pGame->LCDGetColor(C_GREEN|255, "slider box"));
    
  // draw filled part of slider
  pdp->Fill( pixIR+2, pixJ+1, (pixISizeR-5)*mg_fFactor, (pixJSize-4), col);

  // print percentage text
  CTString strPercentage;
  strPercentage.PrintF("%d%%", (int)floor(mg_fFactor*100+0.5f) );
  pdp->PutTextC( strPercentage, pixIR+pixISizeR/2, pixJ+1, col);
}


void CMGLevelButton::OnActivate(void)
{
  PlayMenuSound(_psdPress);
  IFeel_PlayEffect("Menu_press");
  _pGame->gam_strCustomLevel = mg_fnmLevel;
  extern void (*_pAfterLevelChosen)(void);
  _pAfterLevelChosen();
}


void CMGLevelButton::OnSetFocus(void)
{
  SetThumbnail(mg_fnmLevel);
  CMGButton::OnSetFocus();
}



// return slider position on scren
PIXaabbox2D CMGVarButton::GetSliderBox(void)
{ 
  extern CDrawPort *pdp;
  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixIR = (PIX) (box.Min()(1)+box.Size()(1)*0.55f);
  PIX pixJ  = (PIX) (box.Min()(2));
  PIX pixISize = (PIX) (box.Size()(1)*0.13f);
  PIX pixJSize = (PIX) (box.Size()(2));
  return PIXaabbox2D( PIX2D(pixIR, pixJ+1), PIX2D(pixIR+pixISize-4, pixJ+pixJSize-6));
}

extern BOOL _bVarChanged;
BOOL CMGVarButton::OnKeyDown(int iVKey)
{
  if (mg_pvsVar==NULL || mg_pvsVar->vs_bSeparator || !mg_pvsVar->Validate() || !mg_bEnabled) {
    return CMenuGadget::OnKeyDown(iVKey);
  }

  // handle slider
  if( mg_pvsVar->vs_iSlider && !mg_pvsVar->vs_bCustom) {
    // ignore RMB
    if( iVKey==VK_RBUTTON) return TRUE;
    // handle LMB
    if( iVKey==VK_LBUTTON) {
      // get position of slider box on screen
      PIXaabbox2D boxSlider = GetSliderBox();
      // if mouse is within
      if( boxSlider>=PIX2D(_pixCursorPosI, _pixCursorPosJ)) {
        // set new position exactly where mouse pointer is
        mg_pvsVar->vs_iValue = (INDEX) ((FLOAT)(_pixCursorPosI-boxSlider.Min()(1))/boxSlider.Size()(1) * (mg_pvsVar->vs_ctValues));
        _bVarChanged = TRUE;
      } 
      // handled
      return TRUE;
    }
  }

  if( iVKey==VK_RETURN) {
    FlushVarSettings(TRUE);
    void MenuGoToParent(void);
    MenuGoToParent();
    return TRUE;
  }

  if( iVKey==VK_LBUTTON || iVKey==VK_RIGHT) {
    if (mg_pvsVar!=NULL) {
      INDEX iOldValue = mg_pvsVar->vs_iValue;
      mg_pvsVar->vs_iValue++;
      if( mg_pvsVar->vs_iValue>=mg_pvsVar->vs_ctValues) {
        // wrap non-sliders, clamp sliders
        if( mg_pvsVar->vs_iSlider) mg_pvsVar->vs_iValue = mg_pvsVar->vs_ctValues-1L;
        else mg_pvsVar->vs_iValue = 0;
      }
      if( iOldValue != mg_pvsVar->vs_iValue) {
        _bVarChanged = TRUE;
        mg_pvsVar->vs_bCustom = FALSE;
        mg_pvsVar->Validate();
      }
    }
    return TRUE;
  }
  
  if( iVKey==VK_LEFT || iVKey==VK_RBUTTON) {
    if (mg_pvsVar!=NULL) {
      INDEX iOldValue = mg_pvsVar->vs_iValue;
      mg_pvsVar->vs_iValue--;
      if( mg_pvsVar->vs_iValue<0) {
        // wrap non-sliders, clamp sliders
        if( mg_pvsVar->vs_iSlider) mg_pvsVar->vs_iValue = 0;
        else mg_pvsVar->vs_iValue = mg_pvsVar->vs_ctValues-1L;
      }
      if( iOldValue != mg_pvsVar->vs_iValue) {
        _bVarChanged = TRUE;
        mg_pvsVar->vs_bCustom = FALSE;
        mg_pvsVar->Validate();
      }
    }
    return TRUE;
  }
  
  // not handled
  return CMenuGadget::OnKeyDown(iVKey);
}


void CMGVarButton::Render( CDrawPort *pdp)
{
  if (mg_pvsVar==NULL) {
    return;
  }

  SetFontMedium(pdp);

  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixIL = (PIX) (box.Min()(1)+box.Size()(1)*0.45f);
  PIX pixIR = (PIX) (box.Min()(1)+box.Size()(1)*0.55f);
  PIX pixIC = (PIX) (box.Center()(1));
  PIX pixJ  = (PIX) (box.Min()(2));

  if (mg_pvsVar->vs_bSeparator)
  {
    mg_bEnabled = FALSE;
    COLOR col = _pGame->LCDGetColor(C_WHITE|255, "separator");
    CTString strText = mg_pvsVar->vs_strName;
    pdp->PutTextC(strText, pixIC, pixJ, col);
  }
  else if (mg_pvsVar->Validate())
  {
    // check whether the variable is disabled
    if( mg_pvsVar->vs_strFilter!="") mg_bEnabled = _pShell->GetINDEX(mg_pvsVar->vs_strFilter);
    COLOR col = GetCurrentColor();
    pdp->PutTextR( mg_pvsVar->vs_strName, pixIL, pixJ, col);
    // custom is by default
    CTString strText = TRANS("Custom");
    if( !mg_pvsVar->vs_bCustom)
    { // not custom!
      strText = mg_pvsVar->vs_astrTexts[mg_pvsVar->vs_iValue];
      // need slider?
      if( mg_pvsVar->vs_iSlider>0) {
        // draw box around slider
        PIX pixISize = (PIX) (box.Size()(1)*0.13f);
        PIX pixJSize = (PIX) (box.Size()(2));
        PIXaabbox2D aabbox( PIX2D(pixIR, pixJ+1), PIX2D(pixIR+pixISize-4, pixJ+pixJSize-6));
        _pGame->LCDDrawBox( 0, -1, aabbox, _pGame->LCDGetColor(C_GREEN|255, "slider box"));
        // draw filled part of slider
        if( mg_pvsVar->vs_iSlider==1) {
          // fill slider
          FLOAT fFactor = (FLOAT)(mg_pvsVar->vs_iValue+1) / mg_pvsVar->vs_ctValues;
          pdp->Fill( pixIR+1, pixJ+2, (pixISize-6)*fFactor, pixJSize-9, col);
        } else {
          // ratio slider
          ASSERT( mg_pvsVar->vs_iSlider==2);
          FLOAT fUnitWidth = (FLOAT)(pixISize-5) / mg_pvsVar->vs_ctValues;
          pdp->Fill( pixIR+1+(mg_pvsVar->vs_iValue*fUnitWidth), pixJ+2, fUnitWidth, pixJSize-9, col);
        }
        // move text printout to the right of slider
        pixIR += (PIX) (box.Size()(1)*0.15f);
      }
    }
    // write right text
    pdp->PutText(strText, pixIR, pixJ, col);
  }
}


CMGFileButton::CMGFileButton(void)
{
  mg_iState = FBS_NORMAL;
}

// refresh current text from description
void CMGFileButton::RefreshText(void)
{
  mg_strText = mg_strDes;
  mg_strText.OnlyFirstLine();
  mg_strInfo = mg_strDes;
  mg_strInfo.RemovePrefix(mg_strText);
  mg_strInfo.DeleteChar(0);
}

void CMGFileButton::SaveDescription(void)
{
  CTFileName fnFileNameDescription = mg_fnm.NoExt()+".des";
  try {
    mg_strDes.Save_t(fnFileNameDescription);
  } catch (const char *strError) {
    CPrintF("%s\n", (const char *)strError);
  }
}

CMGFileButton *_pmgFileToSave = NULL;
void OnFileSaveOK(void)
{
  if (_pmgFileToSave!=NULL) {
    _pmgFileToSave->SaveYes();
  }
}

void CMGFileButton::DoSave(void)
{
  if (FileExistsForWriting(mg_fnm)) {
    _pmgFileToSave = this;
    extern void SaveConfirm(void);
    SaveConfirm();
  } else {
    SaveYes();
  }
}

void CMGFileButton::SaveYes(void)
{
  ASSERT(gmLoadSaveMenu.gm_bSave);
  // call saving function
  BOOL bSucceeded = gmLoadSaveMenu.gm_pAfterFileChosen(mg_fnm);
  // if saved
  if (bSucceeded) {
    // save the description too
    SaveDescription();
  }
}

void CMGFileButton::DoLoad(void)
{
  ASSERT(!gmLoadSaveMenu.gm_bSave);
  // if no file
  if(!FileExists(mg_fnm)) {
    // do nothing
    return;
  }
  if (gmLoadSaveMenu.gm_pgmNextMenu!=NULL) {
    gmLoadSaveMenu.gm_pgmParentMenu = gmLoadSaveMenu.gm_pgmNextMenu;
  }
  // call loading function
  BOOL bSucceeded = gmLoadSaveMenu.gm_pAfterFileChosen(mg_fnm);
  ASSERT(bSucceeded);
}

static CTString _strTmpDescription;
static CTString _strOrgDescription;

void CMGFileButton::StartEdit(void)
{
  CMGEdit::OnActivate();
}

void CMGFileButton::OnActivate(void)
{
  if (mg_fnm=="") {
    return;
  }

  PlayMenuSound(_psdPress);
  IFeel_PlayEffect("Menu_press");

  // if loading
  if (!gmLoadSaveMenu.gm_bSave) {
    // load now
    DoLoad();
  // if saving
  } else {
    // switch to editing mode
    BOOL bWasEmpty = mg_strText==EMPTYSLOTSTRING;
    mg_strDes = gmLoadSaveMenu.gm_strSaveDes;
    RefreshText();
    _strOrgDescription = _strTmpDescription = mg_strText;
    if (bWasEmpty) {
      _strOrgDescription = EMPTYSLOTSTRING;
    }
    mg_pstrToChange = &_strTmpDescription;
    StartEdit();
    mg_iState = FBS_SAVENAME;
  }
}
BOOL CMGFileButton::OnKeyDown(int iVKey)
{
  if (mg_iState == FBS_NORMAL) {
    if (gmLoadSaveMenu.gm_bSave || gmLoadSaveMenu.gm_bManage) {
      if (iVKey == VK_F2) {
        if (FileExistsForWriting(mg_fnm)) {
          // switch to renaming mode
          _strOrgDescription = mg_strText;
          _strTmpDescription = mg_strText;
          mg_pstrToChange = &_strTmpDescription;
          StartEdit();
          mg_iState = FBS_RENAME;
        }
        return TRUE;
      } else if (iVKey == VK_DELETE) {
        if (FileExistsForWriting(mg_fnm)) {
          // delete the file, its description and thumbnail
          RemoveFile(mg_fnm);
          RemoveFile(mg_fnm.NoExt()+".des");
          RemoveFile(mg_fnm.NoExt()+"Tbn.tex");
          // refresh menu
          gmLoadSaveMenu.EndMenu();
          gmLoadSaveMenu.StartMenu();
          OnSetFocus();
        }
        return TRUE;
      }
    }
    return CMenuGadget::OnKeyDown(iVKey);
  } else {
    // go out of editing mode
    if(mg_bEditing) {
      if (iVKey==VK_UP || iVKey==VK_DOWN) {
        CMGEdit::OnKeyDown(VK_ESCAPE);
      }
    }
    return CMGEdit::OnKeyDown(iVKey);
  }
}

void CMGFileButton::OnSetFocus(void)
{
  mg_iState = FBS_NORMAL;

  if (gmLoadSaveMenu.gm_bAllowThumbnails && mg_bEnabled) {
    SetThumbnail(mg_fnm);
  } else {
    ClearThumbnail();
  }
  pgmCurrentMenu->KillAllFocuses();
  CMGButton::OnSetFocus();
}

void CMGFileButton::OnKillFocus(void)
{
  // go out of editing mode
  if(mg_bEditing) {
    OnKeyDown(VK_ESCAPE);
  }
  CMGEdit::OnKillFocus();
}

// override from edit gadget
void CMGFileButton::OnStringChanged(void)
{
  // if saving
  if (mg_iState == FBS_SAVENAME) {
    // do the save
    mg_strDes = _strTmpDescription+"\n"+mg_strInfo;
    DoSave();
  // if renaming
  } else if (mg_iState == FBS_RENAME) {
    // do the rename
    mg_strDes = _strTmpDescription+"\n"+mg_strInfo;
    SaveDescription();
    // refresh menu
    gmLoadSaveMenu.EndMenu();
    gmLoadSaveMenu.StartMenu();
    OnSetFocus();
  }
}
void CMGFileButton::OnStringCanceled(void)
{
  mg_strText = _strOrgDescription;
}

void CMGFileButton::Render( CDrawPort *pdp)
{
  // render original gadget first
  CMGEdit::Render(pdp);

  // if currently selected
  if (mg_bFocused && mg_bEnabled) {
    // add info at the bottom if screen
    SetFontMedium(pdp);

    PIXaabbox2D box = FloatBoxToPixBox(pdp, BoxSaveLoad(15.0, 1.0));
    PIX pixI = box.Min()(1);
    PIX pixJ = box.Min()(2);

    COLOR col = _pGame->LCDGetColor(C_mlGREEN|255, "file info");
    pdp->PutText( mg_strInfo, pixI, pixJ * _fGlobalInfoAdjuster, col);
  }
}

FLOATaabbox2D GetBoxPartHoriz(const FLOATaabbox2D &box, FLOAT fMin, FLOAT fMax)
{
  FLOAT fBoxMin = box.Min()(1);
  FLOAT fBoxSize = box.Size()(1);

  return FLOATaabbox2D(
    FLOAT2D(fBoxMin+fBoxSize*fMin, box.Min()(2)),
    FLOAT2D(fBoxMin+fBoxSize*fMax, box.Max()(2)));
}

void PrintInBox(CDrawPort *pdp, PIX pixI, PIX pixJ, PIX pixSizeI, CTString str, COLOR col)
{
  str = str.Undecorated();
  PIX pixCharSize = pdp->dp_pixTextCharSpacing+pdp->dp_FontData->fd_pixCharWidth;
  str.TrimRight(pixSizeI/pixCharSize);

  // print text
  pdp->PutText(str, pixI, pixJ, col);
}

CMGServerList::CMGServerList()
{
  mg_iSelected = 0;
  mg_iFirstOnScreen = 0;
  mg_ctOnScreen = 10;
  mg_pixMinI = 0;
  mg_pixMaxI = 0;
  mg_pixListMinJ = 0;
  mg_pixListStepJ = 0;
  mg_pixDragJ = -1;
  mg_iDragLine = -1;
  mg_pixMouseDrag = -1;
  // by default, sort by ping, best on top
  mg_iSort = 2;
  mg_bSortDown = FALSE;
}
void CMGServerList::AdjustFirstOnScreen(void)
{
  INDEX ctSessions = _lhServers.Count();
  mg_iSelected = Clamp(mg_iSelected, (INDEX)0, ClampDn(ctSessions-1, (INDEX)0));
  mg_iFirstOnScreen = Clamp(mg_iFirstOnScreen, (INDEX)0, ClampDn(ctSessions-mg_ctOnScreen, (INDEX)0));

  if (mg_iSelected<mg_iFirstOnScreen) {
    mg_iFirstOnScreen = ClampUp(mg_iSelected, ClampDn(ctSessions-mg_ctOnScreen-1, (INDEX)0));
  } else if (mg_iSelected>=mg_iFirstOnScreen+mg_ctOnScreen) {
    mg_iFirstOnScreen = ClampDn(mg_iSelected-mg_ctOnScreen+1, (INDEX)0);
  }
}

BOOL _iSort = 0;
BOOL _bSortDown = FALSE;

int CompareSessions(const void *pv0, const void *pv1)
{
  const CNetworkSession &ns0 = **(const CNetworkSession **)pv0;
  const CNetworkSession &ns1 = **(const CNetworkSession **)pv1;

  int iResult = 0;
  switch(_iSort) {
  case 0: iResult = stricmp(ns0.ns_strSession, ns1.ns_strSession); break;
  case 1: iResult = stricmp(ns0.ns_strWorld, ns1.ns_strWorld); break;
  case 2: iResult = (int) (Sgn(ns0.ns_tmPing-ns1.ns_tmPing)); break;
  case 3: iResult = Sgn(ns0.ns_ctPlayers-ns1.ns_ctPlayers); break;
  case 4: iResult = stricmp(ns0.ns_strGameType, ns1.ns_strGameType); break;
  case 5: iResult = stricmp(ns0.ns_strMod,  ns1.ns_strMod ); break;
  case 6: iResult = stricmp(ns0.ns_strVer,  ns1.ns_strVer ); break;
  }

  if (iResult==0) { // make sure we always have unique order when resorting
    return stricmp(ns0.ns_strAddress, ns1.ns_strAddress);;
  }

  return _bSortDown?-iResult:iResult;
}

extern CMGButton mgServerColumn[7];
extern CMGEdit mgServerFilter[7];

void SortAndFilterServers(void)
{
  {FORDELETELIST(CNetworkSession, ns_lnNode, _lhServers, itns) {
    delete &*itns;
  }}
  {FOREACHINLIST(CNetworkSession, ns_lnNode, _pNetwork->ga_lhEnumeratedSessions, itns) {
    CNetworkSession &ns = *itns;
    extern CTString _strServerFilter[7];
    if (_strServerFilter[0]!="" && !ns.ns_strSession.Matches("*"+_strServerFilter[0]+"*")) continue;
    if (_strServerFilter[1]!="" && !ns.ns_strWorld.Matches("*"+_strServerFilter[1]+"*")) continue;
    if (_strServerFilter[2]!="") {
      char strCompare[3] = {0,0,0};
      int iPing = 0;
      _strServerFilter[2].ScanF("%2[<>=]%d", strCompare, &iPing);
      if (strcmp(strCompare, "<" )==0 && !(int(ns.ns_tmPing*1000)< iPing)) continue;
      if (strcmp(strCompare, "<=")==0 && !(int(ns.ns_tmPing*1000)<=iPing)) continue;
      if (strcmp(strCompare, ">" )==0 && !(int(ns.ns_tmPing*1000)> iPing)) continue;
      if (strcmp(strCompare, ">=")==0 && !(int(ns.ns_tmPing*1000)>=iPing)) continue;
      if (strcmp(strCompare, "=" )==0 && !(int(ns.ns_tmPing*1000)==iPing)) continue;
    }
    if (_strServerFilter[3]!="") {
      char strCompare[3] = {0,0,0};
      int iPlayers = 0;
      _strServerFilter[3].ScanF("%2[<>=]%d", strCompare, &iPlayers);
      if (strcmp(strCompare, "<" )==0 && !(ns.ns_ctPlayers< iPlayers)) continue;
      if (strcmp(strCompare, "<=")==0 && !(ns.ns_ctPlayers<=iPlayers)) continue;
      if (strcmp(strCompare, ">" )==0 && !(ns.ns_ctPlayers> iPlayers)) continue;
      if (strcmp(strCompare, ">=")==0 && !(ns.ns_ctPlayers>=iPlayers)) continue;
      if (strcmp(strCompare, "=" )==0 && !(ns.ns_ctPlayers==iPlayers)) continue;
    }
    if (_strServerFilter[4]!="" && !ns.ns_strGameType.Matches("*"+_strServerFilter[4]+"*")) continue;
    if (_strServerFilter[5]!="" && !ns.ns_strMod.Matches("*"+_strServerFilter[5]+"*")) continue;
    if (_strServerFilter[6]!="" && !ns.ns_strVer.Matches("*"+_strServerFilter[6]+"*")) continue;

    CNetworkSession *pnsNew = new CNetworkSession;
    pnsNew->Copy(*itns);
    _lhServers.AddTail(pnsNew->ns_lnNode);
  }}

  _lhServers.Sort(CompareSessions, _offsetof(CNetworkSession, ns_lnNode));
}

void CMGServerList::Render(CDrawPort *pdp)
{
  _iSort     = mg_iSort    ; 
  _bSortDown = mg_bSortDown;
  SortAndFilterServers();

  SetFontSmall(pdp);
  BOOL bFocusedBefore = mg_bFocused;
  mg_bFocused = FALSE;

  //PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  COLOR col = GetCurrentColor();

  PIX pixDPSizeI = pdp->GetWidth();
  PIX pixDPSizeJ = pdp->GetHeight();
  PIX pixCharSizeI = pdp->dp_pixTextCharSpacing+pdp->dp_FontData->fd_pixCharWidth;
  PIX pixCharSizeJ = pdp->dp_pixTextLineSpacing+pdp->dp_FontData->fd_pixCharHeight+1;
  PIX pixLineSize = 1;
  PIX pixSliderSizeI = 10;
  PIX pixOuterMargin = 20;
  
  //INDEX ctSessions = _lhServers.Count();
  INDEX iSession=0;

  INDEX ctColumns[7];
  {for (INDEX i=0; i < static_cast<INDEX>(ARRAYCOUNT(ctColumns)); i++) {
    ctColumns[i] = mgServerColumn[i].mg_strText.Length()+1;
  }}

  PIX pixSizePing     = Max(PIX(pixCharSizeI*5), pixCharSizeI*ctColumns[2])+pixLineSize*2;
  PIX pixSizePlayerCt = Max(PIX(pixCharSizeI*5),pixCharSizeI*ctColumns[3])+pixLineSize*2;
  PIX pixSizeGameType = Max(Min(PIX(pixCharSizeI*20), PIX(pixDPSizeI*0.2f)), pixCharSizeI*ctColumns[4])+pixLineSize*2;
  PIX pixSizeMapName  = Max(PIX(pixDPSizeI*0.25f), pixCharSizeI*ctColumns[1])+pixLineSize*2;
  PIX pixSizeMod      = Max(Min(PIX(pixCharSizeI*11), PIX(pixDPSizeI*0.2f)), pixCharSizeI*ctColumns[5])+pixLineSize*2;
  PIX pixSizeVer      = Max(PIX(pixCharSizeI*7), pixCharSizeI*ctColumns[6])+pixLineSize*2;

  PIX apixSeparatorI[9];
  apixSeparatorI[0] = pixOuterMargin;
  apixSeparatorI[8] = pixDPSizeI-pixOuterMargin-pixLineSize;
  apixSeparatorI[7] = apixSeparatorI[8]-pixSliderSizeI-pixLineSize;
  apixSeparatorI[6] = apixSeparatorI[7]-pixSizeVer-pixLineSize;
  apixSeparatorI[5] = apixSeparatorI[6]-pixSizeMod-pixLineSize;
  apixSeparatorI[4] = apixSeparatorI[5]-pixSizeGameType-pixLineSize;
  apixSeparatorI[3] = apixSeparatorI[4]-pixSizePlayerCt-pixLineSize;
  apixSeparatorI[2] = apixSeparatorI[3]-pixSizePing-pixLineSize;
  apixSeparatorI[1] = apixSeparatorI[2]-pixSizeMapName-pixLineSize;
  apixSeparatorI[1] = apixSeparatorI[2]-pixSizeMapName-pixLineSize;
  //PIX pixSizeServerName = apixSeparatorI[1]-apixSeparatorI[0]-pixLineSize;

  PIX pixTopJ = (PIX) (pixDPSizeJ * _fGlobalTopAdjuster);
  PIX pixBottomJ = (PIX) (pixDPSizeJ*0.82f);

  PIX pixFilterTopJ = (PIX) (pixTopJ+pixLineSize*3+pixCharSizeJ+pixLineSize*3);
  PIX pixListTopJ = (PIX) (pixFilterTopJ+pixLineSize+pixCharSizeJ+pixLineSize);
  INDEX ctSessionsOnScreen = (pixBottomJ-pixListTopJ)/pixCharSizeJ;
  pixBottomJ = pixListTopJ + pixCharSizeJ*ctSessionsOnScreen + pixLineSize*2;

  mg_pixMinI = apixSeparatorI[0];
  mg_pixMaxI = apixSeparatorI[5];
  mg_pixListMinJ = pixListTopJ;
  mg_pixListStepJ = pixCharSizeJ;
  mg_pixSBMinI = apixSeparatorI[7];
  mg_pixSBMaxI = apixSeparatorI[8];
  mg_pixSBMinJ = pixListTopJ;
  mg_pixSBMaxJ = pixBottomJ;
  mg_pixHeaderMinJ = pixTopJ;
  mg_pixHeaderMidJ = pixTopJ+pixLineSize+pixCharSizeJ;
  mg_pixHeaderMaxJ = pixTopJ+(pixLineSize+pixCharSizeJ)*2;
  memcpy(mg_pixHeaderI, apixSeparatorI, sizeof(mg_pixHeaderI));

  {for (INDEX i=0; i < static_cast<INDEX>(ARRAYCOUNT(mgServerFilter)); i++) {
    mgServerColumn[i].mg_boxOnScreen = PixBoxToFloatBox(pdp, 
      PIXaabbox2D( PIX2D(apixSeparatorI[i]+pixCharSizeI/2,pixTopJ+pixLineSize*4), PIX2D(apixSeparatorI[i+1]-pixCharSizeI/2,pixTopJ+pixLineSize*4+pixCharSizeJ) ));
    mgServerFilter[i].mg_boxOnScreen = PixBoxToFloatBox(pdp, 
      PIXaabbox2D( PIX2D(apixSeparatorI[i]+pixCharSizeI/2,pixFilterTopJ), PIX2D(apixSeparatorI[i+1]-pixCharSizeI/2,pixFilterTopJ+pixCharSizeJ) ));
  }}

  for (INDEX i=0; i < static_cast<INDEX>(ARRAYCOUNT(apixSeparatorI)); i++) {
    pdp->DrawLine(apixSeparatorI[i], pixTopJ, apixSeparatorI[i], pixBottomJ, col|CT_OPAQUE);
  }
  pdp->DrawLine(apixSeparatorI[0], pixTopJ, apixSeparatorI[8], pixTopJ, col|CT_OPAQUE);
  pdp->DrawLine(apixSeparatorI[0], pixListTopJ-pixLineSize, apixSeparatorI[8], pixListTopJ-pixLineSize, col|CT_OPAQUE);
  pdp->DrawLine(apixSeparatorI[0], pixBottomJ, apixSeparatorI[8], pixBottomJ, col|CT_OPAQUE);

  PIXaabbox2D boxHandle = GetScrollBarHandleBox();
  pdp->Fill(boxHandle.Min()(1)+2, boxHandle.Min()(2)+2, boxHandle.Size()(1)-3, boxHandle.Size()(2)-3, col|CT_OPAQUE);

  //PIX pixJ = pixTopJ+pixLineSize*2+1;

  mg_ctOnScreen = ctSessionsOnScreen;
  AdjustFirstOnScreen();

  if (_lhServers.Count()==0) {
    if (_pNetwork->ga_strEnumerationStatus!="") {
      mg_bFocused = TRUE;
      COLOR colItem = GetCurrentColor();
      PrintInBox(pdp, apixSeparatorI[0]+pixCharSizeI, pixListTopJ+pixCharSizeJ+pixLineSize+1, apixSeparatorI[1]-apixSeparatorI[0], 
        TRANS("searching..."), colItem);
    }
  } else {
    FOREACHINLIST(CNetworkSession, ns_lnNode, _lhServers, itns) {
      CNetworkSession &ns = *itns;

      if (iSession<mg_iFirstOnScreen || iSession>=mg_iFirstOnScreen+ctSessionsOnScreen) {
        iSession++;
        continue;
      }

      PIX pixJ = pixListTopJ+(iSession-mg_iFirstOnScreen)*pixCharSizeJ+pixLineSize+1;

      mg_bFocused = bFocusedBefore&&iSession==mg_iSelected;
      COLOR colItem = GetCurrentColor();

      if (ns.ns_strVer!=_SE_VER_STRING) {
        colItem = MulColors(colItem, 0xA0A0A0FF);
      }

      CTString strPing(0,"%4d", INDEX(ns.ns_tmPing*1000));
      CTString strPlayersCt(0, "%2d/%2d", ns.ns_ctPlayers, ns.ns_ctMaxPlayers);
      CTString strMod = ns.ns_strMod;
      if (strMod=="") {
        strMod = "SeriousSam";
      }
      PrintInBox(pdp, apixSeparatorI[0]+pixCharSizeI/2, pixJ, apixSeparatorI[1]-apixSeparatorI[0]-pixCharSizeI, ns.ns_strSession, colItem);
      PrintInBox(pdp, apixSeparatorI[1]+pixCharSizeI/2, pixJ, apixSeparatorI[2]-apixSeparatorI[1]-pixCharSizeI, TranslateConst(ns.ns_strWorld), colItem);
      PrintInBox(pdp, apixSeparatorI[2]+pixCharSizeI/2, pixJ, apixSeparatorI[3]-apixSeparatorI[2]-pixCharSizeI, strPing, colItem);
      PrintInBox(pdp, apixSeparatorI[3]+pixCharSizeI/2, pixJ, apixSeparatorI[4]-apixSeparatorI[3]-pixCharSizeI, strPlayersCt, colItem);
      PrintInBox(pdp, apixSeparatorI[4]+pixCharSizeI/2, pixJ, apixSeparatorI[5]-apixSeparatorI[4]-pixCharSizeI, TranslateConst(ns.ns_strGameType), colItem);
      PrintInBox(pdp, apixSeparatorI[5]+pixCharSizeI/2, pixJ, apixSeparatorI[6]-apixSeparatorI[5]-pixCharSizeI, TranslateConst(strMod), colItem);
      PrintInBox(pdp, apixSeparatorI[6]+pixCharSizeI/2, pixJ, apixSeparatorI[7]-apixSeparatorI[6]-pixCharSizeI, ns.ns_strVer, colItem);

      iSession++;
    }
  }

  mg_bFocused = bFocusedBefore;
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

static PIXaabbox2D GetSliderBox(INDEX iFirst, INDEX iVisible, INDEX iTotal,
  PIXaabbox2D boxFull)
{
  if (iTotal<=0) {
    return boxFull;
  }
  FLOAT fSize = ClampUp(FLOAT(iVisible)/iTotal, 1.0f);
  PIX pixFull = (PIX) (boxFull.Size()(2));
  PIX pixSize = (PIX) (pixFull*fSize);
  pixSize = ClampDn(pixSize, boxFull.Size()(1));
  PIX pixTop = (PIX) (pixFull*(FLOAT(iFirst)/iTotal)+boxFull.Min()(2));
  PIX pixI0 = (PIX) (boxFull.Min()(1));
  PIX pixI1 = (PIX) (boxFull.Max()(1));
  return PIXaabbox2D(PIX2D(pixI0, pixTop), PIX2D(pixI1, pixTop+pixSize));
}

PIXaabbox2D CMGServerList::GetScrollBarFullBox(void)
{
  return PIXaabbox2D(PIX2D(mg_pixSBMinI, mg_pixSBMinJ), PIX2D(mg_pixSBMaxI, mg_pixSBMaxJ));
}
PIXaabbox2D CMGServerList::GetScrollBarHandleBox(void)
{
  return GetSliderBox(mg_iFirstOnScreen, mg_ctOnScreen, _lhServers.Count(), GetScrollBarFullBox());
}

void CMGServerList::OnMouseOver(PIX pixI, PIX pixJ)
{
  mg_pixMouseI = pixI;
  mg_pixMouseJ = pixJ;

  if (!(GetKeyState(VK_LBUTTON)&0x8000)) {
    mg_pixDragJ = -1;
  }

  BOOL bInSlider = (pixI>=mg_pixSBMinI && pixI<=mg_pixSBMaxI && pixJ>=mg_pixSBMinJ && pixJ<=mg_pixSBMaxJ);
  if (mg_pixDragJ>=0 && bInSlider) {
    PIX pixDelta = pixJ-mg_pixDragJ;
    INDEX ctSessions = _lhServers.Count();
    INDEX iWantedLine = mg_iDragLine+
      SliderPixToIndex(pixDelta, mg_ctOnScreen, ctSessions, GetScrollBarFullBox());
    mg_iFirstOnScreen = Clamp(iWantedLine, (INDEX)0, ClampDn(ctSessions-mg_ctOnScreen, (INDEX)0));
    mg_iSelected = Clamp(mg_iSelected, mg_iFirstOnScreen, mg_iFirstOnScreen+mg_ctOnScreen-1);
//    AdjustFirstOnScreen();
    return;
  }

  // if some server is selected
  if (pixI>=mg_pixMinI && pixI<=mg_pixMaxI) {
    INDEX iOnScreen = (pixJ-mg_pixListMinJ)/mg_pixListStepJ;
    if (iOnScreen>=0 && iOnScreen<mg_ctOnScreen) {
      // put focus on it
      mg_iSelected = mg_iFirstOnScreen+iOnScreen;
      AdjustFirstOnScreen();
      mg_pixMouseDrag = -1;
    }
  } else if (bInSlider) {
    mg_pixMouseDrag = pixJ;
  }
}

BOOL CMGServerList::OnKeyDown(int iVKey)
{
  switch(iVKey) {
  case VK_UP:
    mg_iSelected-=1;
    AdjustFirstOnScreen();
    break;
  case VK_DOWN:
    mg_iSelected+=1;
    AdjustFirstOnScreen();
    break;
  case VK_PRIOR:
    mg_iSelected-=mg_ctOnScreen-1;
    mg_iFirstOnScreen-=mg_ctOnScreen-1;
    AdjustFirstOnScreen();
    break;
  case VK_NEXT:
    mg_iSelected+=mg_ctOnScreen-1;
    mg_iFirstOnScreen+=mg_ctOnScreen-1;
    AdjustFirstOnScreen();
    break;
  case 11:
    mg_iSelected-=3;
    mg_iFirstOnScreen-=3;
    AdjustFirstOnScreen();
    break;
  case 10:
    mg_iSelected+=3;
    mg_iFirstOnScreen+=3;
    AdjustFirstOnScreen();
    break;
  case VK_LBUTTON:
/*    if (mg_pixMouseJ>=mg_pixHeaderMinJ && mg_pixMouseJ<=mg_pixHeaderMidJ
      && mg_pixMouseI>=mg_pixHeaderI[0] && mg_pixMouseI<=mg_pixHeaderI[7]) {
      INDEX iNewSort = mg_iSort;
      if (mg_pixMouseI<=mg_pixHeaderI[1]) {
        iNewSort = 0;
      } else if (mg_pixMouseI<=mg_pixHeaderI[2]) {
        iNewSort = 1;
      } else if (mg_pixMouseI<=mg_pixHeaderI[3]) {
        iNewSort = 2;
      } else if (mg_pixMouseI<=mg_pixHeaderI[4]) {
        iNewSort = 3;
      } else if (mg_pixMouseI<=mg_pixHeaderI[5]) {
        iNewSort = 4;
      } else if (mg_pixMouseI<=mg_pixHeaderI[6]) {
        iNewSort = 5;
      } else if (mg_pixMouseI<=mg_pixHeaderI[7]) {
        iNewSort = 6;
      }
      if (iNewSort==mg_iSort) {
        mg_bSortDown = !mg_bSortDown;
      } else {
        mg_bSortDown = FALSE;
      }
      mg_iSort = iNewSort;
      break;
    } else */if (mg_pixMouseDrag>=0) {
      mg_pixDragJ = mg_pixMouseDrag;
      mg_iDragLine = mg_iFirstOnScreen;
      break;
    } 
  case VK_RETURN:
    PlayMenuSound(_psdPress);
    IFeel_PlayEffect("Menu_press");
    {INDEX i=0;
    FOREACHINLIST(CNetworkSession, ns_lnNode, _lhServers, itns) {
      if (i==mg_iSelected) {

        char strAddress[256];
        int iPort;
        itns->ns_strAddress.ScanF("%200[^:]:%d", &strAddress, &iPort);
        _pGame->gam_strJoinAddress = strAddress;
        _pShell->SetINDEX("net_iPort", iPort);
        extern void StartSelectPlayersMenuFromServers(void );
        StartSelectPlayersMenuFromServers();
        return TRUE;
      }
      i++;
    }}
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void CMGServerList::OnSetFocus(void)
{
  mg_bFocused = TRUE;
}
void CMGServerList::OnKillFocus(void)
{
  mg_bFocused = FALSE;
}

// -------------------------------- Buttons for player selecting implementation
void CMGChangePlayer::OnActivate(void)
{
  PlayMenuSound(_psdPress);
  IFeel_PlayEffect("Menu_press");
  _iLocalPlayer = mg_iLocalPlayer;
  if( _pGame->gm_aiMenuLocalPlayers[ mg_iLocalPlayer] < 0) 
    _pGame->gm_aiMenuLocalPlayers[ mg_iLocalPlayer] = 0;
  gmPlayerProfile.gm_piCurrentPlayer = &_pGame->gm_aiMenuLocalPlayers[ mg_iLocalPlayer];
  gmPlayerProfile.gm_pgmParentMenu = &gmSelectPlayersMenu;
  extern BOOL _bPlayerMenuFromSinglePlayer;
  _bPlayerMenuFromSinglePlayer = FALSE;
  ChangeToMenu( &gmPlayerProfile);
}
  
void CMGChangePlayer::SetPlayerText(void)
{
  INDEX iPlayer = _pGame->gm_aiMenuLocalPlayers[ mg_iLocalPlayer];
  CPlayerCharacter &pc = _pGame->gm_apcPlayers[ iPlayer];
  if (iPlayer<0 || iPlayer>7) {
    mg_strText = "????";
  } else {
    mg_strText.PrintF(TRANSV("Player %d: %s\n"), mg_iLocalPlayer+1, (const char *) pc.GetNameForPrinting());
  }
}

// ------- Key (from customize keyboard) implementation

CMGKeyDefinition::CMGKeyDefinition( void)
{
  mg_iState = DOING_NOTHING;
}


void CMGKeyDefinition::OnActivate(void)
{
  PlayMenuSound(_psdPress);
  IFeel_PlayEffect("Menu_press");
  SetBindingNames(/*bDefining=*/TRUE);
  mg_iState = RELEASE_RETURN_WAITING;
}


BOOL CMGKeyDefinition::OnKeyDown( int iVKey)
{
  // if waiting for a key definition
  if( mg_iState == PRESS_KEY_WAITING) {
    // do nothing
    return TRUE;
  }

  // if backspace pressed
  if(iVKey == VK_BACK) {
    // clear both keys
    DefineKey(KID_NONE);
    // message is processed
    return TRUE;
  }

  return CMenuGadget::OnKeyDown( iVKey);
}

// set names for both key bindings
void CMGKeyDefinition::SetBindingNames(BOOL bDefining)
{
  // find the button
  INDEX ict=0;
  //INDEX iDik=0;
  FOREACHINLIST( CButtonAction, ba_lnNode, _pGame->gm_ctrlControlsExtra->ctrl_lhButtonActions, itba) {
    if( ict == mg_iControlNumber) {
      CButtonAction &ba = *itba;
      // get the current bindings and names
      INDEX iKey1 = ba.ba_iFirstKey;
      INDEX iKey2 = ba.ba_iSecondKey;
      BOOL bKey1Bound = iKey1!=KID_NONE;
      BOOL bKey2Bound = iKey2!=KID_NONE;
      CTString strKey1 = _pInput->GetButtonTransName(iKey1);
      CTString strKey2 = _pInput->GetButtonTransName(iKey2);

      // if defining
      if (bDefining) {
        // if only first key is defined
        if (bKey1Bound && !bKey2Bound) {
          // put question mark for second key
          mg_strBinding = strKey1+TRANS(" or ")+"?";
        // otherwise
        } else {
          // put question mark only
          mg_strBinding = "?";
        }
      // if not defining
      } else {
        // if second key is defined
        if (bKey2Bound) {
          // add both
          mg_strBinding = strKey1+TRANS(" or ")+strKey2;
        // if second key is undefined
        } else {
          // display only first one
          mg_strBinding = strKey1;
        }
      }
      return ;
    }
    ict++;
  }

  // if not found, put errorneous string
  mg_strBinding = "???";
}

void CMGKeyDefinition::Appear(void)
{
  SetBindingNames(/*bDefining=*/FALSE);
  CMenuGadget::Appear();
}

void CMGKeyDefinition::Disappear(void)
{
  CMenuGadget::Disappear();
}

void CMGKeyDefinition::DefineKey(INDEX iDik)
{
  // for each button in controls
  INDEX ict=0;
  FOREACHINLIST(CButtonAction, ba_lnNode, _pGame->gm_ctrlControlsExtra->ctrl_lhButtonActions, itba) {
    CButtonAction &ba = *itba;
    // if it is this one
    if (ict == mg_iControlNumber) {
      // if should clear
      if (iDik == KID_NONE) {
        // unbind both
        ba.ba_iFirstKey = KID_NONE;
        ba.ba_iSecondKey = KID_NONE;
      }
      // if first key is unbound, or both keys are bound
      if (ba.ba_iFirstKey==KID_NONE || ba.ba_iSecondKey!=KID_NONE) {
        // bind first key
        ba.ba_iFirstKey = iDik;
        // clear second key
        ba.ba_iSecondKey = KID_NONE;
      // if only first key bound
      } else {
        // bind second key
        ba.ba_iSecondKey = iDik;
      }
    // if it is not this one
    } else {
      // clear bindings that contain this key
      if (ba.ba_iFirstKey == iDik) {
        ba.ba_iFirstKey = KID_NONE;
      }
      if (ba.ba_iSecondKey == iDik) {
        ba.ba_iSecondKey = KID_NONE;
      }
    }
    ict++;
  }

  SetBindingNames(/*bDefining=*/FALSE);
}

void CMGKeyDefinition::Think( void)
{
  if( mg_iState == RELEASE_RETURN_WAITING)
  {
    _bDefiningKey = TRUE;
    extern BOOL _bMouseUsedLast;
    _bMouseUsedLast = FALSE;
    _pInput->SetJoyPolling(TRUE);
    _pInput->GetInput(FALSE);
    if( _pInput->IsInputEnabled() &&
        !_pInput->GetButtonState( KID_ENTER) &&
        !_pInput->GetButtonState( KID_MOUSE1 ) )
    {
      mg_iState = PRESS_KEY_WAITING;
    }
  }  
  else if( mg_iState == PRESS_KEY_WAITING)
  {
    _pInput->SetJoyPolling(TRUE);
    _pInput->GetInput(FALSE);
    for( INDEX iDik = 0; iDik<MAX_OVERALL_BUTTONS; iDik++)
    {
      if( _pInput->GetButtonState( iDik))
      {
        // skip keys that cannot be defined
        if (iDik == KID_TILDE) {
          continue;
        }
        // if escape not pressed
        if (iDik != KID_ESCAPE) {
          // define the new key
          DefineKey(iDik);
        // if escape pressed
        } else {
          // undefine the key
          DefineKey(KID_NONE);
        }

        // end defining loop
        mg_iState = DOING_NOTHING;
        _bDefiningKey = FALSE;
        // refresh all buttons
        pgmCurrentMenu->FillListItems();
        break;
      }
    }
  }
}

void CMGKeyDefinition::Render( CDrawPort *pdp)
{
  SetFontMedium(pdp);

  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixIL = (PIX) (box.Min()(1)+box.Size()(1)*0.45f);
  PIX pixIR = (PIX) (box.Min()(1)+box.Size()(1)*0.55f);
  PIX pixJ = (PIX) (box.Min()(2));

  COLOR col = GetCurrentColor();
  pdp->PutTextR( mg_strLabel, pixIL, pixJ, col);
  pdp->PutText( mg_strBinding, pixIR, pixJ, col);
}
