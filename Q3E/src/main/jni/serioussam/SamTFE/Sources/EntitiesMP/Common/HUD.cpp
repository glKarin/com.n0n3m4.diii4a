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

#include "EntitiesMP/StdH/StdH.h"
#include "GameMP/SEColors.h"

#include <Engine/Graphics/DrawPort.h>

#include <EntitiesMP/Player.h>
#include <EntitiesMP/PlayerWeapons.h>
#include <EntitiesMP/MusicHolder.h>
#include <EntitiesMP/EnemyBase.h>
#include <EntitiesMP/EnemyCounter.h>

#define ENTITY_DEBUG

// armor & health constants 
// NOTE: these _do not_ reflect easy/tourist maxvalue adjustments. that is by design!
#define TOP_ARMOR  100
#define TOP_HEALTH 100

#ifdef PLATFORM_UNIX
extern "C" __attribute__ ((visibility("default"))) FLOAT _fArmorHeightAdjuster;
extern "C" __attribute__ ((visibility("default"))) FLOAT _fFragScorerHeightAdjuster;
#else
extern __declspec(dllimport) FLOAT _fArmorHeightAdjuster;
extern __declspec(dllimport) FLOAT _fFragScorerHeightAdjuster;
#endif

//
extern INDEX hud_bShowPing;
extern INDEX hud_bShowKills;
extern INDEX hud_bShowScore;

// cheats
extern INDEX cht_bEnable;
extern INDEX cht_bGod;
extern INDEX cht_bFly;
extern INDEX cht_bGhost;
extern INDEX cht_bInvisible;
extern FLOAT cht_fTranslationMultiplier;

// interface control
extern INDEX hud_bShowInfo;
extern INDEX hud_bShowLatency;
extern INDEX hud_bShowMessages;
extern INDEX hud_iShowPlayers;
extern INDEX hud_iSortPlayers;
extern FLOAT hud_fOpacity;
extern FLOAT hud_fScaling;
extern FLOAT hud_tmWeaponsOnScreen;
extern INDEX hud_bShowMatchInfo;

// player statistics sorting keys
enum SortKeys {
  PSK_NAME    = 1,
  PSK_HEALTH  = 2,
  PSK_SCORE   = 3,
  PSK_MANA    = 4, 
  PSK_FRAGS   = 5,
  PSK_DEATHS  = 6,
};

// where is the bar lowest value
enum BarOrientations {
  BO_LEFT  = 1,
  BO_RIGHT = 2, 
  BO_UP    = 3,
  BO_DOWN  = 4,
};

extern const INDEX aiWeaponsRemap[19];

// maximal mana for master status
#define MANA_MASTER 10000

// drawing variables
static const CPlayer *_penPlayer;
static CPlayerWeapons *_penWeapons;
static CDrawPort *_pDP;
static PIX   _pixDPWidth, _pixDPHeight;
static FLOAT _fResolutionScaling;
static FLOAT _fCustomScaling;
static ULONG _ulAlphaHUD;
static COLOR _colHUD;
static COLOR _colHUDText;
static TIME  _tmNow = -1.0f;
static TIME  _tmLast = -1.0f;
static CFontData _fdNumbersFont;

static FLOAT _fResolutionScalingY;
static FLOAT _fCustomScalingAdjustment;
// array for pointers of all players
CPlayer *_apenPlayers[NET_MAXGAMEPLAYERS] = {0};

// status bar textures
static CTextureObject _toHealth;
static CTextureObject _toOxygen;
static CTextureObject _toScore;
static CTextureObject _toHiScore;
static CTextureObject _toMessage;
static CTextureObject _toMana;
static CTextureObject _toFrags;
static CTextureObject _toDeaths;
static CTextureObject _toArmorSmall;
static CTextureObject _toArmorMedium;
static CTextureObject _toArmorLarge;

// ammo textures                    
static CTextureObject _toAShells;
static CTextureObject _toABullets;
static CTextureObject _toARockets;
static CTextureObject _toAGrenades;
static CTextureObject _toANapalm;
static CTextureObject _toAElectricity;
static CTextureObject _toAIronBall;
static CTextureObject _toASniperBullets;
static CTextureObject _toASeriousBomb;
// weapon textures
static CTextureObject _toWKnife;
static CTextureObject _toWColt;
static CTextureObject _toWSingleShotgun;
static CTextureObject _toWDoubleShotgun;
static CTextureObject _toWTommygun;
static CTextureObject _toWSniper;
static CTextureObject _toWChainsaw;
static CTextureObject _toWMinigun;
static CTextureObject _toWRocketLauncher;
static CTextureObject _toWGrenadeLauncher;
static CTextureObject _toWFlamer;
static CTextureObject _toWLaser;
static CTextureObject _toWIronCannon;

// powerup textures (ORDER IS THE SAME AS IN PLAYER.ES!)
#define MAX_POWERUPS 4
static CTextureObject _atoPowerups[MAX_POWERUPS];
// tile texture (one has corners, edges and center)
static CTextureObject _toTile;
// sniper mask texture
static CTextureObject _toSniperMask;
static CTextureObject _toSniperWheel;
static CTextureObject _toSniperArrow;
static CTextureObject _toSniperEye;
static CTextureObject _toSniperLed;

// all info about color transitions
struct ColorTransitionTable {
  COLOR ctt_colFine;      // color for values over 1.0
  COLOR ctt_colHigh;      // color for values from 1.0 to 'fMedium'
  COLOR ctt_colMedium;    // color for values from 'fMedium' to 'fLow'
  COLOR ctt_colLow;       // color for values under fLow
  FLOAT ctt_fMediumHigh;  // when to switch to high color   (normalized float!)
  FLOAT ctt_fLowMedium;   // when to switch to medium color (normalized float!)
  BOOL  ctt_bSmooth;      // should colors have smooth transition
};
static struct ColorTransitionTable _cttHUD;


// ammo's info structure
struct AmmoInfo {
  CTextureObject    *ai_ptoAmmo;
  struct WeaponInfo *ai_pwiWeapon1;
  struct WeaponInfo *ai_pwiWeapon2;
  INDEX ai_iAmmoAmmount;
  INDEX ai_iMaxAmmoAmmount;
  INDEX ai_iLastAmmoAmmount;
  TIME  ai_tmAmmoChanged;
  BOOL  ai_bHasWeapon;
};

// weapons' info structure
struct WeaponInfo {
  enum WeaponType  wi_wtWeapon;
  CTextureObject  *wi_ptoWeapon;
  struct AmmoInfo *wi_paiAmmo;
  BOOL wi_bHasWeapon;
};

extern struct WeaponInfo _awiWeapons[18];
static struct AmmoInfo _aaiAmmo[8] = {
  { &_toAShells,        &_awiWeapons[4],  &_awiWeapons[5],  0, 0, 0, -9, FALSE }, //  0
  { &_toABullets,       &_awiWeapons[6],  &_awiWeapons[7],  0, 0, 0, -9, FALSE }, //  1
  { &_toARockets,       &_awiWeapons[8],  NULL,             0, 0, 0, -9, FALSE }, //  2
  { &_toAGrenades,      &_awiWeapons[9],  NULL,             0, 0, 0, -9, FALSE }, //  3
  { &_toANapalm,        &_awiWeapons[11], NULL,             0, 0, 0, -9, FALSE }, //  4
  { &_toAElectricity,   &_awiWeapons[12], NULL,             0, 0, 0, -9, FALSE }, //  5
  { &_toAIronBall,      &_awiWeapons[14], NULL,             0, 0, 0, -9, FALSE }, //  6
  { &_toASniperBullets, &_awiWeapons[13], NULL,             0, 0, 0, -9, FALSE }, //  7
};
static const INDEX aiAmmoRemap[8] = { 0, 1, 2, 3, 4, 7, 5, 6 };

struct WeaponInfo _awiWeapons[18] = {
  { WEAPON_NONE,            NULL,                 NULL,         FALSE },   //  0
  { WEAPON_KNIFE,           &_toWKnife,           NULL,         FALSE },   //  1
  { WEAPON_COLT,            &_toWColt,            NULL,         FALSE },   //  2
  { WEAPON_DOUBLECOLT,      &_toWColt,            NULL,         FALSE },   //  3
  { WEAPON_SINGLESHOTGUN,   &_toWSingleShotgun,   &_aaiAmmo[0], FALSE },   //  4
  { WEAPON_DOUBLESHOTGUN,   &_toWDoubleShotgun,   &_aaiAmmo[0], FALSE },   //  5
  { WEAPON_TOMMYGUN,        &_toWTommygun,        &_aaiAmmo[1], FALSE },   //  6
  { WEAPON_MINIGUN,         &_toWMinigun,         &_aaiAmmo[1], FALSE },   //  7
  { WEAPON_ROCKETLAUNCHER,  &_toWRocketLauncher,  &_aaiAmmo[2], FALSE },   //  8
  { WEAPON_GRENADELAUNCHER, &_toWGrenadeLauncher, &_aaiAmmo[3], FALSE },   //  9
  { WEAPON_CHAINSAW,        &_toWChainsaw,        NULL,         FALSE },   // 10
  { WEAPON_FLAMER,          &_toWFlamer,          &_aaiAmmo[4], FALSE },   // 11
  { WEAPON_LASER,           &_toWLaser,           &_aaiAmmo[5], FALSE },   // 12
  { WEAPON_SNIPER,          &_toWSniper,          &_aaiAmmo[7], FALSE },   // 13
  { WEAPON_IRONCANNON,      &_toWIronCannon,      &_aaiAmmo[6], FALSE },   // 14
//{ WEAPON_PIPEBOMB,        &_toWPipeBomb,        &_aaiAmmo[3], FALSE },   // 15
//{ WEAPON_GHOSTBUSTER,     &_toWGhostBuster,     &_aaiAmmo[5], FALSE },   // 16
//{ WEAPON_NUKECANNON,      &_toWNukeCannon,      &_aaiAmmo[7], FALSE },   // 17
  { WEAPON_NONE,            NULL,                 NULL,         FALSE },   // 15
  { WEAPON_NONE,            NULL,                 NULL,         FALSE },   // 16
  { WEAPON_NONE,            NULL,                 NULL,         FALSE },   // 17
};


// compare functions for qsort()
static int qsort_CompareNames( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  CTString strName0 = en0.GetPlayerName();
  CTString strName1 = en1.GetPlayerName();
  return strnicmp( strName0, strName1, 8);
}

static int qsort_CompareScores( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_psGameStats.ps_iScore;
  SLONG sl1 = en1.m_psGameStats.ps_iScore;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareHealth( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = (SLONG)ceil(en0.GetHealth());
  SLONG sl1 = (SLONG)ceil(en1.GetHealth());
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareManas( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_iMana;
  SLONG sl1 = en1.m_iMana;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareDeaths( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_psGameStats.ps_iDeaths;
  SLONG sl1 = en1.m_psGameStats.ps_iDeaths;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareFrags( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_psGameStats.ps_iKills;
  SLONG sl1 = en1.m_psGameStats.ps_iKills;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return -qsort_CompareDeaths(ppPEN0, ppPEN1);
}

#ifndef NDEBUG
static int qsort_CompareLatencies( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = (SLONG)ceil(en0.m_tmLatency);
  SLONG sl1 = (SLONG)ceil(en1.m_tmLatency);
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}
#endif // NDEBUG

// prepare color transitions
static void PrepareColorTransitions( COLOR colFine, COLOR colHigh, COLOR colMedium, COLOR colLow,
                                     FLOAT fMediumHigh, FLOAT fLowMedium, BOOL bSmooth)
{
  _cttHUD.ctt_colFine     = colFine;
  _cttHUD.ctt_colHigh     = colHigh;   
  _cttHUD.ctt_colMedium   = colMedium;
  _cttHUD.ctt_colLow      = colLow;
  _cttHUD.ctt_fMediumHigh = fMediumHigh;
  _cttHUD.ctt_fLowMedium  = fLowMedium;
  _cttHUD.ctt_bSmooth     = bSmooth;
}



// calculates shake ammount and color value depanding on value change
#define SHAKE_TIME (2.0f)
static COLOR AddShaker( PIX const pixAmmount, INDEX const iCurrentValue, INDEX &iLastValue,
                        TIME &tmChanged, FLOAT &fMoverX, FLOAT &fMoverY)
{
  // update shaking if needed
  fMoverX = fMoverY = 0.0f;
  const TIME tmNow = _pTimer->GetLerpedCurrentTick();
  if( iCurrentValue != iLastValue) {
    iLastValue = iCurrentValue;
    tmChanged  = tmNow;
  } else {
    // in case of loading (timer got reseted)
    tmChanged = ClampUp( tmChanged, tmNow);
  }
  
  // no shaker?
  const TIME tmDelta = tmNow - tmChanged;
  if( tmDelta > SHAKE_TIME) return NONE;
  ASSERT( tmDelta>=0);
  // shake, baby shake!
  //const FLOAT fAmmount    = _fResolutionScaling * _fCustomScaling * pixAmmount;
  const FLOAT fAmmount    = _fResolutionScaling * _fCustomScalingAdjustment * _fCustomScaling * pixAmmount;
  const FLOAT fMultiplier = (SHAKE_TIME-tmDelta)/SHAKE_TIME *fAmmount;
  const INDEX iRandomizer = (INDEX)((tmNow*511.0f)*fAmmount*iCurrentValue);
  const FLOAT fNormRnd1   = (FLOAT)((iRandomizer ^ (iRandomizer>>9)) & 1023) * 0.0009775f;  // 1/1023 - normalized
  const FLOAT fNormRnd2   = (FLOAT)((iRandomizer ^ (iRandomizer>>7)) & 1023) * 0.0009775f;  // 1/1023 - normalized
  fMoverX = (fNormRnd1 -0.5f) * fMultiplier;
  fMoverY = (fNormRnd2 -0.5f) * fMultiplier;
  // clamp to adjusted ammount (pixels relative to resolution and HUD scale
  fMoverX = Clamp( fMoverX, -fAmmount, fAmmount);
  fMoverY = Clamp( fMoverY, -fAmmount, fAmmount);
  if( tmDelta < SHAKE_TIME/3) return C_WHITE;
  else return NONE;
//return FloatToInt(tmDelta*4) & 1 ? C_WHITE : NONE;
}


// get current color from local color transitions table
static COLOR GetCurrentColor( FLOAT fNormalizedValue)
{
  // if value is in 'low' zone just return plain 'low' alert color
  if( fNormalizedValue < _cttHUD.ctt_fLowMedium) return( _cttHUD.ctt_colLow & 0xFFFFFF00);
  // if value is in out of 'extreme' zone just return 'extreme' color
  if( fNormalizedValue > 1.0f) return( _cttHUD.ctt_colFine & 0xFFFFFF00);
 
  COLOR col;
  // should blend colors?
  if( _cttHUD.ctt_bSmooth)
  { // lets do some interpolations
    FLOAT fd, f1, f2;
    COLOR col1, col2;
    UBYTE ubH,ubS,ubV, ubH2,ubS2,ubV2;
    // determine two colors for interpolation
    if( fNormalizedValue > _cttHUD.ctt_fMediumHigh) {
      f1   = 1.0f;
      f2   = _cttHUD.ctt_fMediumHigh;
      col1 = _cttHUD.ctt_colHigh;
      col2 = _cttHUD.ctt_colMedium;
    } else { // fNormalizedValue > _cttHUD.ctt_fLowMedium == TRUE !
      f1   = _cttHUD.ctt_fMediumHigh;
      f2   = _cttHUD.ctt_fLowMedium;
      col1 = _cttHUD.ctt_colMedium;
      col2 = _cttHUD.ctt_colLow;
    }
    // determine interpolation strength
    fd = (fNormalizedValue-f2) / (f1-f2);
    // convert colors to HSV
    ColorToHSV( col1, ubH,  ubS,  ubV);
    ColorToHSV( col2, ubH2, ubS2, ubV2);
    // interpolate H, S and V components
    ubH = (UBYTE)(ubH*fd + ubH2*(1.0f-fd));
    ubS = (UBYTE)(ubS*fd + ubS2*(1.0f-fd));
    ubV = (UBYTE)(ubV*fd + ubV2*(1.0f-fd));
    // convert HSV back to COLOR
    col = HSVToColor( ubH, ubS, ubV);
  }
  else
  { // simple color picker
    col = _cttHUD.ctt_colMedium;
    if( fNormalizedValue > _cttHUD.ctt_fMediumHigh) col = _cttHUD.ctt_colHigh;
  }
  // all done
  return( col & 0xFFFFFF00);
}



// fill array with players' statistics (returns current number of players in game)
extern INDEX SetAllPlayersStats( INDEX iSortKey)
{
  // determine maximum number of players for this session
  INDEX iPlayers    = 0;
  INDEX iMaxPlayers = _penPlayer->GetMaxPlayers();
  CPlayer *penCurrent;
  // loop thru potentional players 
  for( INDEX i=0; i<iMaxPlayers; i++)
  { // ignore non-existent players
    penCurrent = (CPlayer*)&*_penPlayer->GetPlayerEntity(i);
    if( penCurrent==NULL) continue;
    // fill in player parameters
    _apenPlayers[iPlayers] = penCurrent;
    // advance to next real player
    iPlayers++;
  }
  // sort statistics by some key if needed
  switch( iSortKey) {
  case PSK_NAME:    qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareNames);   break;
  case PSK_SCORE:   qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareScores);  break;
  case PSK_HEALTH:  qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareHealth);  break;
  case PSK_MANA:    qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareManas);   break;
  case PSK_FRAGS:   qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareFrags);   break;
  case PSK_DEATHS:  qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareDeaths);  break;
  default:  break;  // invalid or NONE key specified so do nothing
  }
  // all done
  return iPlayers;
}



// ----------------------- drawing functions

// draw border with filter
static void HUD_DrawBorder( FLOAT fCenterX, FLOAT fCenterY, FLOAT fSizeX, FLOAT fSizeY, COLOR colTiles)
{
  // determine location
  const FLOAT fCenterI  = fCenterX*_pixDPWidth  / 640.0f;
  const FLOAT fCenterJ  = fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment);
  const FLOAT fSizeI    = _fResolutionScaling * _fCustomScalingAdjustment * fSizeX;
  const FLOAT fSizeJ    = _fResolutionScaling * _fCustomScalingAdjustment * fSizeY;
  const FLOAT fTileSize = 8*_fResolutionScaling * _fCustomScalingAdjustment * _fCustomScaling;
  // determine exact positions
  const FLOAT fLeft  = fCenterI  - fSizeI/2 -1; 
  const FLOAT fRight = fCenterI  + fSizeI/2 +1; 
  const FLOAT fUp    = fCenterJ  - fSizeJ/2 -1; 
  const FLOAT fDown  = fCenterJ  + fSizeJ/2 +1;
  const FLOAT fLeftEnd  = fLeft  + fTileSize;
  const FLOAT fRightBeg = fRight - fTileSize; 
  const FLOAT fUpEnd    = fUp    + fTileSize; 
  const FLOAT fDownBeg  = fDown  - fTileSize; 
  // prepare texture                 
  colTiles |= _ulAlphaHUD;
  // put corners
  _pDP->InitTexture( &_toTile, TRUE); // clamping on!
  _pDP->AddTexture( fLeft, fUp,   fLeftEnd, fUpEnd,   colTiles);
  _pDP->AddTexture( fRight,fUp,   fRightBeg,fUpEnd,   colTiles);
  _pDP->AddTexture( fRight,fDown, fRightBeg,fDownBeg, colTiles);
  _pDP->AddTexture( fLeft, fDown, fLeftEnd, fDownBeg, colTiles);
  // put edges
  _pDP->AddTexture( fLeftEnd,fUp,    fRightBeg,fUpEnd,   0.4f,0.0f, 0.6f,1.0f, colTiles);
  _pDP->AddTexture( fLeftEnd,fDown,  fRightBeg,fDownBeg, 0.4f,0.0f, 0.6f,1.0f, colTiles);
  _pDP->AddTexture( fLeft,   fUpEnd, fLeftEnd, fDownBeg, 0.0f,0.4f, 1.0f,0.6f, colTiles);
  _pDP->AddTexture( fRight,  fUpEnd, fRightBeg,fDownBeg, 0.0f,0.4f, 1.0f,0.6f, colTiles);
  // put center
  _pDP->AddTexture( fLeftEnd, fUpEnd, fRightBeg, fDownBeg, 0.4f,0.4f, 0.6f,0.6f, colTiles);
  _pDP->FlushRenderingQueue();
}


// draw icon texture (if color = NONE, use colortransitions structure)
static void HUD_DrawIcon( FLOAT fCenterX, FLOAT fCenterY, CTextureObject &toIcon,
                          COLOR colDefault, FLOAT fNormValue, BOOL bBlink)
{
  // determine color
  COLOR col = colDefault;
  if( col==NONE) col = GetCurrentColor( fNormValue);
  // determine blinking state
  if( bBlink && fNormValue<=(_cttHUD.ctt_fLowMedium/2)) {
    // activate blinking only if value is <= half the low edge
    INDEX iCurrentTime = (INDEX)(_tmNow*4);
    if( iCurrentTime&1) col = C_vdGRAY;
  }
  // determine location
  const FLOAT fCenterI = fCenterX*_pixDPWidth  / 640.0f;
  const FLOAT fCenterJ = fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment);
  // determine dimensions
  CTextureData *ptd = (CTextureData*)toIcon.GetData();
  const FLOAT fHalfSizeI = _fResolutionScaling * _fCustomScalingAdjustment * _fCustomScaling * ptd->GetPixWidth()  *0.5f;
  const FLOAT fHalfSizeJ = _fResolutionScaling * _fCustomScalingAdjustment * _fCustomScaling * ptd->GetPixHeight() *0.5f;
  // done
  _pDP->InitTexture( &toIcon);
  _pDP->AddTexture( fCenterI-fHalfSizeI, fCenterJ-fHalfSizeJ,
                    fCenterI+fHalfSizeI, fCenterJ+fHalfSizeJ, col|_ulAlphaHUD);
  _pDP->FlushRenderingQueue();
}


// draw text (or numbers, whatever)
static void HUD_DrawText( FLOAT fCenterX, FLOAT fCenterY, const CTString &strText,
                          COLOR colDefault, FLOAT fNormValue)
{
  // determine color
  COLOR col = colDefault;
  if( col==NONE) col = GetCurrentColor( fNormValue);
  // determine location
  PIX pixCenterI = (PIX)(fCenterX*_pixDPWidth  / 640.0f);
  PIX pixCenterJ = (PIX)(fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment));
  // done
  _pDP->SetTextScaling( _fResolutionScaling * _fCustomScalingAdjustment * _fCustomScaling);
  _pDP->PutTextCXY( strText, pixCenterI, pixCenterJ, col|_ulAlphaHUD);
}


// draw bar
static void HUD_DrawBar( FLOAT fCenterX, FLOAT fCenterY, PIX pixSizeX, PIX pixSizeY,
                         enum BarOrientations eBarOrientation, COLOR colDefault, FLOAT fNormValue)
{
  // determine color
  COLOR col = colDefault;
  if( col==NONE) col = GetCurrentColor( fNormValue);
  // determine location and size
  PIX pixCenterI = (PIX)(fCenterX*_pixDPWidth  / 640.0f);
  PIX pixCenterJ = (PIX)(fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment));
  PIX pixSizeI   = (PIX)(_fResolutionScaling * _fCustomScalingAdjustment * pixSizeX);
  PIX pixSizeJ   = (PIX)(_fResolutionScaling * _fCustomScalingAdjustment * pixSizeY);
  // fill bar background area
  PIX pixLeft  = pixCenterI-pixSizeI/2;
  PIX pixUpper = pixCenterJ-pixSizeJ/2;
  // determine bar position and inner size
  switch( eBarOrientation) {
  case BO_UP:
    pixSizeJ = (PIX) (pixSizeJ*fNormValue);
    break;
  case BO_DOWN:
    pixUpper  = pixUpper + (PIX)ceil(pixSizeJ * (1.0f-fNormValue));
    pixSizeJ = (PIX) (pixSizeJ*fNormValue);
    break;
  case BO_LEFT:
    pixSizeI = (PIX) (pixSizeI*fNormValue);
    break;
  case BO_RIGHT:
    pixLeft   = pixLeft + (PIX)ceil(pixSizeI * (1.0f-fNormValue));
    pixSizeI = (PIX) (pixSizeI*fNormValue);
    break;
  }
  // done
  _pDP->Fill( pixLeft, pixUpper, pixSizeI, pixSizeJ, col|_ulAlphaHUD);
}

static void DrawRotatedQuad( class CTextureObject *_pTO, FLOAT fX, FLOAT fY, FLOAT fSize, ANGLE aAngle, COLOR col)
{
  FLOAT fSinA = Sin(aAngle);
  FLOAT fCosA = Cos(aAngle);
  FLOAT fSinPCos = fCosA*fSize+fSinA*fSize;
  FLOAT fSinMCos = fSinA*fSize-fCosA*fSize;
  FLOAT fI0, fJ0, fI1, fJ1, fI2, fJ2, fI3, fJ3;

  fI0 = fX-fSinPCos;  fJ0 = fY-fSinMCos;
  fI1 = fX+fSinMCos;  fJ1 = fY-fSinPCos;
  fI2 = fX+fSinPCos;  fJ2 = fY+fSinMCos;
  fI3 = fX-fSinMCos;  fJ3 = fY+fSinPCos;
  
  _pDP->InitTexture( _pTO);
  _pDP->AddTexture( fI0, fJ0, 0, 0, col,   fI1, fJ1, 0, 1, col,
                    fI2, fJ2, 1, 1, col,   fI3, fJ3, 1, 0, col);
  _pDP->FlushRenderingQueue();  

}

static void DrawAspectCorrectTextureCentered( class CTextureObject *_pTO, FLOAT fX, FLOAT fY, FLOAT fWidth, COLOR col)
{
  CTextureData *ptd = (CTextureData*)_pTO->GetData();
  FLOAT fTexSizeI = ptd->GetPixWidth();
  FLOAT fTexSizeJ = ptd->GetPixHeight();
  FLOAT fHeight = fWidth*fTexSizeJ/fTexSizeI;
  
  _pDP->InitTexture( _pTO);
  _pDP->AddTexture( fX-fWidth*0.5f, fY-fHeight*0.5f, fX+fWidth*0.5f, fY+fHeight*0.5f, 0, 0, 1, 1, col);
  _pDP->FlushRenderingQueue();
}

// draw sniper mask
static void HUD_DrawSniperMask( void )
{
  // determine location
  const FLOAT fSizeI = _pixDPWidth;
  const FLOAT fSizeJ = _pixDPHeight;
  const FLOAT fCenterI = fSizeI/2;  
  const FLOAT fCenterJ = fSizeJ/2;  
  const FLOAT fBlackStrip = (fSizeI-fSizeJ)/2;

  COLOR colMask = C_WHITE|CT_OPAQUE;
  
  CTextureData *ptd = (CTextureData*)_toSniperMask.GetData();
  //const FLOAT fTexSizeI = ptd->GetPixWidth();
  //const FLOAT fTexSizeJ = ptd->GetPixHeight();

  // main sniper mask
  _pDP->InitTexture( &_toSniperMask);
  _pDP->AddTexture( fBlackStrip, 0, fCenterI, fCenterJ, 0.98f, 0.02f, 0, 1.0f, colMask);
  _pDP->AddTexture( fCenterI, 0, fSizeI-fBlackStrip, fCenterJ, 0, 0.02f, 0.98f, 1.0f, colMask);
  _pDP->AddTexture( fBlackStrip, fCenterJ, fCenterI, fSizeJ, 0.98f, 1.0f, 0, 0.02f, colMask);
  _pDP->AddTexture( fCenterI, fCenterJ, fSizeI-fBlackStrip, fSizeJ, 0, 1, 0.98f, 0.02f, colMask);
  _pDP->FlushRenderingQueue();
  _pDP->Fill( 0, 0, fBlackStrip+1, fSizeJ, C_BLACK|CT_OPAQUE);
  _pDP->Fill( fSizeI-fBlackStrip-1, 0, fBlackStrip+1, fSizeJ, C_BLACK|CT_OPAQUE);

  colMask = LerpColor(SE_COL_BLUE_LIGHT, C_WHITE, 0.25f);

  FLOAT _fYResolutionScaling = (FLOAT)_pixDPHeight/480.0f;

  FLOAT fDistance = _penWeapons->m_fRayHitDistance;
  FLOAT aFOV = Lerp(_penWeapons->m_fSniperFOVlast, _penWeapons->m_fSniperFOV,
                    _pTimer->GetLerpFactor());
  CTString strTmp;
  
  // wheel
  FLOAT fZoom = 1.0f/tan(RadAngle(aFOV)*0.5f);  // 2.0 - 8.0
  
  FLOAT fAFact = (Clamp(aFOV, 14.2f, 53.1f)-14.2f)/(53.1f-14.2f); // only for zooms 2x-4x !!!!!!
  ANGLE aAngle = 314.0f+fAFact*292.0f;

  DrawRotatedQuad(&_toSniperWheel, fCenterI, fCenterJ, 40.0f*_fYResolutionScaling,
                  aAngle, colMask|0x44);
  
  FLOAT fTM = _pTimer->GetLerpedCurrentTick();
  
  COLOR colLED;
  if (_penWeapons->m_tmLastSniperFire+1.25f<fTM) { // blinking
    colLED = 0x44FF22BB;
  } else {
    colLED = 0xFF4422DD;
  }

  // reload indicator
  DrawAspectCorrectTextureCentered(&_toSniperLed, fCenterI-37.0f*_fYResolutionScaling,
    fCenterJ+36.0f*_fYResolutionScaling, 15.0f*_fYResolutionScaling, colLED);
    
  if (_fResolutionScaling>=1.0f)
  {
    FLOAT _fIconSize;
    FLOAT _fLeftX,  _fLeftYU,  _fLeftYD;
    FLOAT _fRightX, _fRightYU, _fRightYD;

    if (_fResolutionScaling<=1.3f) {
      _pDP->SetFont( _pfdConsoleFont);
      _pDP->SetTextAspect( 1.0f);
      _pDP->SetTextScaling(1.0f);
      _fIconSize = 22.8f;
      _fLeftX = 159.0f;
      _fLeftYU = 8.0f;
      _fLeftYD = 6.0f;
      _fRightX = 159.0f;
      _fRightYU = 11.0f;
      _fRightYD = 6.0f;
    } else {
      _pDP->SetFont( _pfdDisplayFont);
      _pDP->SetTextAspect( 1.0f);
      _pDP->SetTextScaling(0.7f*_fYResolutionScaling);
      _fIconSize = 19.0f;
      _fLeftX = 162.0f;
      _fLeftYU = 8.0f;
      _fLeftYD = 6.0f;
      _fRightX = 162.0f;
      _fRightYU = 11.0f;
      _fRightYD = 6.0f;
    }
     
    // arrow + distance
    DrawAspectCorrectTextureCentered(&_toSniperArrow, fCenterI-_fLeftX*_fYResolutionScaling,
      fCenterJ-_fLeftYU*_fYResolutionScaling, _fIconSize*_fYResolutionScaling, 0xFFCC3399 );
    if (fDistance>9999.9f) { strTmp.PrintF("---.-");           }
    else if (TRUE)         { strTmp.PrintF("%.1f", fDistance); }
    _pDP->PutTextC( strTmp, fCenterI-_fLeftX*_fYResolutionScaling,
      fCenterJ+_fLeftYD*_fYResolutionScaling, colMask|0xaa);
    
    // eye + zoom level
    DrawAspectCorrectTextureCentered(&_toSniperEye,   fCenterI+_fRightX*_fYResolutionScaling,
      fCenterJ-_fRightYU*_fYResolutionScaling, _fIconSize*_fYResolutionScaling, 0xFFCC3399 ); //SE_COL_ORANGE_L
    strTmp.PrintF("%.1fx", fZoom);
    _pDP->PutTextC( strTmp, fCenterI+_fRightX*_fYResolutionScaling,
      fCenterJ+_fRightYD*_fYResolutionScaling, colMask|0xaa);
  }
}


// helper functions

// fill weapon and ammo table with current state
static void FillWeaponAmmoTables(void)
{
  // ammo quantities
  _aaiAmmo[0].ai_iAmmoAmmount    = _penWeapons->m_iShells;
  _aaiAmmo[0].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxShells;
  _aaiAmmo[1].ai_iAmmoAmmount    = _penWeapons->m_iBullets;
  _aaiAmmo[1].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxBullets;
  _aaiAmmo[2].ai_iAmmoAmmount    = _penWeapons->m_iRockets;
  _aaiAmmo[2].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxRockets;
  _aaiAmmo[3].ai_iAmmoAmmount    = _penWeapons->m_iGrenades;
  _aaiAmmo[3].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxGrenades;
  _aaiAmmo[4].ai_iAmmoAmmount    = _penWeapons->m_iNapalm;
  _aaiAmmo[4].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxNapalm;
  _aaiAmmo[5].ai_iAmmoAmmount    = _penWeapons->m_iElectricity;
  _aaiAmmo[5].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxElectricity;
  _aaiAmmo[6].ai_iAmmoAmmount    = _penWeapons->m_iIronBalls;
  _aaiAmmo[6].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxIronBalls;
  _aaiAmmo[7].ai_iAmmoAmmount    = _penWeapons->m_iSniperBullets;
  _aaiAmmo[7].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxSniperBullets;

  // prepare ammo table for weapon possesion
  INDEX i, iAvailableWeapons = _penWeapons->m_iAvailableWeapons;
  for( i=0; i<8; i++) _aaiAmmo[i].ai_bHasWeapon = FALSE;
  // weapon possesion
  for( i=WEAPON_NONE+1; i<WEAPON_LAST; i++)
  {
    if( _awiWeapons[i].wi_wtWeapon!=WEAPON_NONE)
    {
      // regular weapons
      _awiWeapons[i].wi_bHasWeapon = (iAvailableWeapons&(1<<(_awiWeapons[i].wi_wtWeapon-1)));
      if( _awiWeapons[i].wi_paiAmmo!=NULL) _awiWeapons[i].wi_paiAmmo->ai_bHasWeapon |= _awiWeapons[i].wi_bHasWeapon;
    }
  }
}


//<<<<<<< DEBUG FUNCTIONS >>>>>>>

#ifdef ENTITY_DEBUG
CRationalEntity *DBG_prenStackOutputEntity = NULL;
#endif
void HUD_SetEntityForStackDisplay(CRationalEntity *pren)
{
#ifdef ENTITY_DEBUG
  DBG_prenStackOutputEntity = pren;
#endif
  return;
}

#ifdef ENTITY_DEBUG
static void HUD_DrawEntityStack()
{
  CTString strTemp;
  PIX pixFontHeight;
  ULONG pixTextBottom;

  if (tmp_ai[9]==12345)
  {
    if (DBG_prenStackOutputEntity!=NULL)
    {
      pixFontHeight = _pfdConsoleFont->fd_pixCharHeight;
      pixTextBottom = (ULONG) (_pixDPHeight*0.83);
      _pDP->SetFont( _pfdConsoleFont);
      _pDP->SetTextScaling( 1.0f);
    
      INDEX ctStates = DBG_prenStackOutputEntity->en_stslStateStack.Count();
      strTemp.PrintF("-- stack of '%s'(%s)@%gs\n", (const char *) DBG_prenStackOutputEntity->GetName(),
        (const char *) DBG_prenStackOutputEntity->en_pecClass->ec_pdecDLLClass->dec_strName,
        _pTimer->CurrentTick());
      _pDP->PutText( strTemp, 1, pixTextBottom-pixFontHeight*(ctStates+1), _colHUD|_ulAlphaHUD);
      
      for(INDEX iState=ctStates-1; iState>=0; iState--) {
        SLONG slState = DBG_prenStackOutputEntity->en_stslStateStack[iState];
        strTemp.PrintF("0x%08x %s\n", slState, 
          DBG_prenStackOutputEntity->en_pecClass->ec_pdecDLLClass->HandlerNameForState(slState));
        _pDP->PutText( strTemp, 1, pixTextBottom-pixFontHeight*(iState+1), _colHUD|_ulAlphaHUD);
      }
    }
  }
}
#endif
//<<<<<<< DEBUG FUNCTIONS >>>>>>>

// main

// render interface (frontend) to drawport
// (units are in pixels for 640x480 resolution - for other res HUD will be scalled automatically)
extern void DrawHUD( const CPlayer *penPlayerCurrent, CDrawPort *pdpCurrent, BOOL bSnooping, const CPlayer *penPlayerOwner)
{
  // no player - no info, sorry
  if( penPlayerCurrent==NULL || (penPlayerCurrent->GetFlags()&ENF_DELETED)) return;
  
  // if snooping and owner player ins NULL, return
  if ( bSnooping && penPlayerOwner==NULL) return;

  // find last values in case of predictor
  CPlayer *penLast = (CPlayer*)penPlayerCurrent;
  if( penPlayerCurrent->IsPredictor()) penLast = (CPlayer*)(((CPlayer*)penPlayerCurrent)->GetPredicted());
  ASSERT( penLast!=NULL);
  if( penLast==NULL) return; // !!!! just in case

  // cache local variables
  hud_fOpacity = Clamp( hud_fOpacity, 0.1f, 1.0f);
  hud_fScaling = Clamp( hud_fScaling, 0.5f, 1.2f);
  _penPlayer  = penPlayerCurrent;
  _penWeapons = (CPlayerWeapons*)&*_penPlayer->m_penWeapons;
  _pDP        = pdpCurrent;
  _pixDPWidth   = _pDP->GetWidth();
  _pixDPHeight  = _pDP->GetHeight();
  _fCustomScaling     = hud_fScaling;
  _fCustomScalingAdjustment = 1.0f;
  _fResolutionScaling = (FLOAT)_pixDPWidth /640.0f;
  _fResolutionScalingY = (FLOAT)_pixDPHeight /480.0f;
  _colHUD     = 0x4C80BB00;
  _colHUDText = SE_COL_ORANGE_LIGHT;
  _ulAlphaHUD = NormFloatToByte(hud_fOpacity);
  _tmNow = _pTimer->CurrentTick();

  // determine hud colorization;
  COLOR colMax = SE_COL_BLUEGREEN_LT;
  COLOR colTop = SE_COL_ORANGE_LIGHT;
  COLOR colMid = LerpColor(colTop, C_RED, 0.5f);

  // adjust borders color in case of spying mode
  COLOR colBorder = _colHUD; 
  
  if( bSnooping) {
    colBorder = SE_COL_ORANGE_NEUTRAL;
    if( ((ULONG)(_tmNow*5))&1) {
      //colBorder = (colBorder>>1) & 0x7F7F7F00; // darken flash and scale
      colBorder = SE_COL_ORANGE_DARK;
      _fCustomScaling *= 0.933f;
    }
  }

  // draw sniper mask (original mask even if snooping)
  if (((CPlayerWeapons*)&*penPlayerOwner->m_penWeapons)->m_iCurrentWeapon==WEAPON_SNIPER
    &&((CPlayerWeapons*)&*penPlayerOwner->m_penWeapons)->m_bSniping) {
    HUD_DrawSniperMask();
  } 
   
  // prepare font and text dimensions
  CTString strValue;
  PIX pixCharWidth;
  FLOAT fValue, fNormValue, fCol, fRow;
  _pDP->SetFont( &_fdNumbersFont);
  pixCharWidth = _fdNumbersFont.GetWidth() + _fdNumbersFont.GetCharSpacing() +1;
  FLOAT fChrUnit = pixCharWidth * _fCustomScaling;

  const PIX pixTopBound    = 6;
  const PIX pixLeftBound   = 6;
  const PIX pixBottomBound = (PIX) ((480 * _pDP->dp_fWideAdjustment) -pixTopBound);
  const PIX pixRightBound  = 640-pixLeftBound;
  FLOAT fOneUnit  = (32+0) * _fCustomScaling;  // unit size
  FLOAT fAdvUnit  = (32+4) * _fCustomScaling;  // unit advancer
  FLOAT fNextUnit = (32+8) * _fCustomScaling;  // unit advancer
  FLOAT fHalfUnit = fOneUnit * 0.5f;
  FLOAT fMoverX, fMoverY;
  COLOR colDefault;
  
  // prepare and draw health info
  fValue = ClampDn( _penPlayer->GetHealth(), 0.0f);  // never show negative health
  fNormValue = fValue/TOP_HEALTH;
  //####
	if (fValue > 300.0f)
	{
		fValue=999.0f;
	}
	strValue.PrintF("%d", (SLONG)ceil(fValue));
  //####
  _fCustomScalingAdjustment = 0.5f;
  PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.5f, 0.25f, FALSE);
  fRow = pixBottomBound-fHalfUnit;
  fCol = pixLeftBound+fHalfUnit;
  colDefault = AddShaker( 5, (INDEX) fValue, penLast->m_iLastHealth, penLast->m_tmHealthChanged, fMoverX, fMoverY);
  _fCustomScalingAdjustment = 0.7f;
  HUD_DrawBorder( fCol+fMoverX, fRow+fMoverY, fOneUnit, fOneUnit, colBorder);
  fCol += fAdvUnit+fChrUnit*3/2 -fHalfUnit;
  HUD_DrawBorder( fCol, fRow, fChrUnit*3, fOneUnit, colBorder);
  _fCustomScalingAdjustment = 0.5f;
  HUD_DrawText( fCol, fRow, strValue, colDefault, fNormValue);
  fCol -= fAdvUnit+fChrUnit*3/2 -fHalfUnit;
  HUD_DrawIcon( fCol+fMoverX, fRow+fMoverY, _toHealth, C_WHITE /*_colHUD*/, fNormValue, TRUE);

  // prepare and draw armor info (eventually)
  fValue = _penPlayer->m_fArmor;
  if( fValue > 0.0f) {
    fNormValue = fValue/TOP_ARMOR;
    strValue.PrintF( "%d", (SLONG)ceil(fValue));
    PrepareColorTransitions( colMax, colTop, colMid, C_lGRAY, 0.5f, 0.25f, FALSE);
    //fRow = pixBottomBound- (fNextUnit+fHalfUnit);//*_pDP->dp_fWideAdjustment;
    fRow = pixBottomBound- (fNextUnit+fHalfUnit) * _fArmorHeightAdjuster;
    fCol = pixLeftBound+    fHalfUnit;
    colDefault = AddShaker( 3, (INDEX) fValue, penLast->m_iLastArmor, penLast->m_tmArmorChanged, fMoverX, fMoverY);
    _fCustomScalingAdjustment = 0.7f;
    HUD_DrawBorder( fCol+fMoverX, fRow+fMoverY, fOneUnit, fOneUnit, colBorder);
    fCol += fAdvUnit+fChrUnit*3/2 -fHalfUnit;
    HUD_DrawBorder( fCol, fRow, fChrUnit*3, fOneUnit, colBorder);
    _fCustomScalingAdjustment = 0.5f;
    HUD_DrawText( fCol, fRow, strValue, NONE, fNormValue);
    fCol -= fAdvUnit+fChrUnit*3/2 -fHalfUnit;
    if (fValue<=50.5f) {
      HUD_DrawIcon( fCol+fMoverX, fRow+fMoverY, _toArmorSmall, C_WHITE /*_colHUD*/, fNormValue, FALSE);
    } else if (fValue<=100.5f) {
      HUD_DrawIcon( fCol+fMoverX, fRow+fMoverY, _toArmorMedium, C_WHITE /*_colHUD*/, fNormValue, FALSE);
    } else {
      HUD_DrawIcon( fCol+fMoverX, fRow+fMoverY, _toArmorLarge, C_WHITE /*_colHUD*/, fNormValue, FALSE);
    }
  }

  // prepare and draw ammo and weapon info
  CTextureObject *ptoCurrentAmmo=NULL, *ptoCurrentWeapon=NULL, *ptoWantedWeapon=NULL;
  INDEX iCurrentWeapon = _penWeapons->m_iCurrentWeapon;
  INDEX iWantedWeapon  = _penWeapons->m_iWantedWeapon;
  // determine corresponding ammo and weapon texture component
  ptoCurrentWeapon = _awiWeapons[iCurrentWeapon].wi_ptoWeapon;
  ptoWantedWeapon  = _awiWeapons[iWantedWeapon].wi_ptoWeapon;

  AmmoInfo *paiCurrent = _awiWeapons[iCurrentWeapon].wi_paiAmmo;
  if( paiCurrent!=NULL) ptoCurrentAmmo = paiCurrent->ai_ptoAmmo;

  // draw complete weapon info if knife isn't current weapon
  if( ptoCurrentAmmo!=NULL && !GetSP()->sp_bInfiniteAmmo) {
    // determine ammo quantities
    FLOAT fMaxValue = _penWeapons->GetMaxAmmo();
    fValue = _penWeapons->GetAmmo();
    fNormValue = fValue / fMaxValue;
    strValue.PrintF( "%d", (SLONG)ceil(fValue));
    PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.30f, 0.15f, FALSE);
    BOOL bDrawAmmoIcon = _fCustomScaling<=1.0f;
    // draw ammo, value and weapon
    fRow = pixBottomBound-fHalfUnit;
    fCol = 175 + fHalfUnit;
    colDefault = AddShaker( 4, (INDEX) fValue, penLast->m_iLastAmmo, penLast->m_tmAmmoChanged, fMoverX, fMoverY);
  	_fCustomScalingAdjustment = 0.7f;
    HUD_DrawBorder( fCol+fMoverX, fRow+fMoverY, fOneUnit, fOneUnit, colBorder);
    fCol += fAdvUnit+fChrUnit*3/2 -fHalfUnit;
    HUD_DrawBorder( fCol, fRow, fChrUnit*3, fOneUnit, colBorder);
    if( bDrawAmmoIcon) {
      fCol += fAdvUnit+fChrUnit*3/2 -fHalfUnit;
      HUD_DrawBorder( fCol, fRow, fOneUnit, fOneUnit, colBorder);
      _fCustomScalingAdjustment = 0.5f;
      HUD_DrawIcon( fCol, fRow, *ptoCurrentAmmo, C_WHITE /*_colHUD*/, fNormValue, TRUE);
      fCol -= fAdvUnit+fChrUnit*3/2 -fHalfUnit;
    }
    _fCustomScalingAdjustment = 0.5f;
    HUD_DrawText( fCol, fRow, strValue, NONE, fNormValue);
    fCol -= fAdvUnit+fChrUnit*3/2 -fHalfUnit;
    HUD_DrawIcon( fCol+fMoverX, fRow+fMoverY, *ptoCurrentWeapon, C_WHITE /*_colHUD*/, fNormValue, !bDrawAmmoIcon);
  } else if( ptoCurrentWeapon!=NULL) {
    // draw only knife or colt icons (ammo is irrelevant)
    fRow = pixBottomBound-fHalfUnit;
    fCol = 205 + fHalfUnit;
    _fCustomScalingAdjustment = 0.7f;
    HUD_DrawBorder( fCol, fRow, fOneUnit, fOneUnit, colBorder);
    _fCustomScalingAdjustment = 0.5f;
    HUD_DrawIcon(   fCol, fRow, *ptoCurrentWeapon, C_WHITE /*_colHUD*/, fNormValue, FALSE);
  }


  // display all ammo infos
  INDEX i;
  FLOAT fAdv;
  COLOR colIcon, colBar;
  PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.5f, 0.25f, FALSE);
  // reduce the size of icon slightly
  _fCustomScaling = ClampDn( _fCustomScaling*0.8f, 0.5f);
  const FLOAT fOneUnitS  = fOneUnit  *0.8f;
  const FLOAT fAdvUnitS  = fAdvUnit  *0.8f;
  //const FLOAT fNextUnitS = fNextUnit *0.8f;
  const FLOAT fHalfUnitS = fHalfUnit *0.8f;

  // prepare postition and ammo quantities
  fRow = pixBottomBound-fHalfUnitS;
  fCol = pixRightBound -fHalfUnitS;
  const FLOAT fBarPos = fHalfUnitS*0.7f;
  FillWeaponAmmoTables();

  FLOAT fBombCount = penPlayerCurrent->m_iSeriousBombCount;
  BOOL  bBombFiring = FALSE;
  // draw serious bomb
#define BOMB_FIRE_TIME 1.5f
  if (penPlayerCurrent->m_tmSeriousBombFired+BOMB_FIRE_TIME>_pTimer->GetLerpedCurrentTick()) {
    fBombCount++;
    if (fBombCount>3) { fBombCount = 3; }
    bBombFiring = TRUE;
  }
  if (fBombCount>0) {
    fNormValue = (FLOAT) fBombCount / 3.0f;
    COLOR colBombBorder = _colHUD;
    COLOR colBombIcon = C_WHITE;
    COLOR colBombBar = _colHUDText; if (fBombCount==1) { colBombBar = C_RED; }
    if (bBombFiring) { 
      FLOAT fFactor = (_pTimer->GetLerpedCurrentTick() - penPlayerCurrent->m_tmSeriousBombFired)/BOMB_FIRE_TIME;
      colBombBorder = LerpColor(colBombBorder, C_RED, fFactor);
      colBombIcon = LerpColor(colBombIcon, C_RED, fFactor);
      colBombBar = LerpColor(colBombBar, C_RED, fFactor);
    }
    _fCustomScalingAdjustment = 0.7f;
    HUD_DrawBorder( fCol,         fRow, fOneUnitS, fOneUnitS, colBombBorder);
    _fCustomScalingAdjustment = 0.5f;
    HUD_DrawIcon(   fCol,         fRow, _toASeriousBomb, colBombIcon, fNormValue, FALSE);
    HUD_DrawBar(    fCol+fBarPos, fRow, (INDEX) (fOneUnitS/5), (INDEX) (fOneUnitS-2), BO_DOWN, colBombBar, fNormValue);
    // make space for serious bomb
    fCol -= fAdvUnitS;
  }

  // loop thru all ammo types
  if (!GetSP()->sp_bInfiniteAmmo) {
    for( INDEX ii=7; ii>=0; ii--) {
      i = aiAmmoRemap[ii];
      // if no ammo and hasn't got that weapon - just skip this ammo
      AmmoInfo &ai = _aaiAmmo[i];
      ASSERT( ai.ai_iAmmoAmmount>=0);
      if( ai.ai_iAmmoAmmount==0 && !ai.ai_bHasWeapon) continue;
      // display ammo info
      colIcon = C_WHITE /*_colHUD*/;
      if( ai.ai_iAmmoAmmount==0) colIcon = C_mdGRAY;
      if( ptoCurrentAmmo == ai.ai_ptoAmmo) colIcon = C_WHITE; 
      fNormValue = (FLOAT)ai.ai_iAmmoAmmount / ai.ai_iMaxAmmoAmmount;
      colBar = AddShaker( 4, ai.ai_iAmmoAmmount, ai.ai_iLastAmmoAmmount, ai.ai_tmAmmoChanged, fMoverX, fMoverY);
	  _fCustomScalingAdjustment = 0.7f;
      HUD_DrawBorder( fCol,         fRow+fMoverY, fOneUnitS, fOneUnitS, colBorder);
      _fCustomScalingAdjustment = 0.5f;
      HUD_DrawIcon(   fCol,         fRow+fMoverY, *_aaiAmmo[i].ai_ptoAmmo, colIcon, fNormValue, FALSE);
      HUD_DrawBar(    fCol+fBarPos, fRow+fMoverY, (INDEX) (fOneUnitS/5), (INDEX) (fOneUnitS-2), BO_DOWN, colBar, fNormValue);
      // advance to next position
      fCol -= fAdvUnitS;  
    }
  }

  // draw powerup(s) if needed
  PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.66f, 0.33f, FALSE);
  TIME *ptmPowerups = (TIME*)&_penPlayer->m_tmInvisibility;
  TIME *ptmPowerupsMax = (TIME*)&_penPlayer->m_tmInvisibilityMax;
  fRow = pixBottomBound-fOneUnitS-fAdvUnitS;
  fCol = pixRightBound -fHalfUnitS;
  for( i=0; i<MAX_POWERUPS; i++)
  {
    // skip if not active
    const TIME tmDelta = ptmPowerups[i] - _tmNow;
    if( tmDelta<=0) continue;
    fNormValue = tmDelta / ptmPowerupsMax[i];
    // draw icon and a little bar
    _fCustomScalingAdjustment = 0.7f;
    HUD_DrawBorder( fCol,         fRow, fOneUnitS, fOneUnitS, colBorder);
    _fCustomScalingAdjustment = 0.5f;
    HUD_DrawIcon(   fCol,         fRow, _atoPowerups[i], C_WHITE /*_colHUD*/, fNormValue, TRUE);
    HUD_DrawBar(    fCol+fBarPos, fRow, (INDEX) (fOneUnitS/5), (INDEX) (fOneUnitS-2), BO_DOWN, NONE, fNormValue);
    // play sound if icon is flashing
    if(fNormValue<=(_cttHUD.ctt_fLowMedium/2)) {
      // activate blinking only if value is <= half the low edge
      INDEX iLastTime = (INDEX)(_tmLast*4);
      INDEX iCurrentTime = (INDEX)(_tmNow*4);
      if(iCurrentTime&1 & !(iLastTime&1)) {
        ((CPlayer *)penPlayerCurrent)->PlayPowerUpSound();
      }
    }
    // advance to next position
    fCol -= fAdvUnitS;
  }

#define TXT_WIDTH_SCORE		5
#define TXT_WIDTH_ARMOR		4
#define TXT_WIDTH_HEALTH	4
#define TXT_WIDTH_DEATHS	4
#define TXT_WIDTH_FRAGS		4
#define TXT_WIDTH_MANA		5
#define TXT_WIDTH_PING		3
#define TXT_WIDTH_KILLS		4

  const FLOAT fTextScale  = (_fResolutionScaling+1) *0.5f;
  const BOOL bSinglePlay  =  GetSP()->sp_bSinglePlayer;
  COLOR colScore  = _colHUD;
  if(bSinglePlay) {
    if (hud_bShowKills)
	{
    // set font and prepare font parameters
    _pfdDisplayFont->SetVariableWidth();
    _pDP->SetFont( _pfdDisplayFont);
    _pDP->SetTextScaling( fTextScale);
    FLOAT fCharHeight = (_pfdDisplayFont->GetHeight()-2)*fTextScale * 0.9;
    INDEX i=0;
    CTString strKillsC, strKillsT;
	strKillsC.PrintF("%d", _penPlayer->m_psLevelStats.ps_iKills);
	strKillsT.PrintF("%d",_penPlayer->m_psLevelTotal.ps_iKills>9999?9999:_penPlayer->m_psLevelTotal.ps_iKills);

    const FLOAT fCharWidth = (PIX)((_pfdDisplayFont->GetWidth()-2) *fTextScale);
    long textoffset = 1;
	textoffset += (TXT_WIDTH_KILLS /2);
   	//_pDP->PutTextC(strKillsT,			_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit / 2, C_MAGENTA |_ulAlphaHUD); textoffset += (TXT_WIDTH_KILLS/2);
	//_pDP->PutText("/",					_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit / 2, C_MAGENTA |_ulAlphaHUD);	textoffset += 2;
	//_pDP->PutTextC(strKillsC,			_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit / 2, C_MAGENTA |_ulAlphaHUD);	textoffset += (TXT_WIDTH_KILLS/2);
   	_pDP->PutTextC(strKillsT,			_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit / 2, colScore |_ulAlphaHUD); textoffset += (TXT_WIDTH_KILLS/2);
	_pDP->PutText("/",					_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit / 2, colScore |_ulAlphaHUD);	textoffset += 2;
	_pDP->PutTextC(strKillsC,			_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit / 2, colScore |_ulAlphaHUD);	textoffset += (TXT_WIDTH_KILLS/2);
    _pDP->PutTextR("Enemies:",	            _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit / 2, colScore |_ulAlphaHUD);
	}
  }

  // if weapon change is in progress
  _fCustomScaling = hud_fScaling;
  hud_tmWeaponsOnScreen = Clamp( hud_tmWeaponsOnScreen, 0.0f, 10.0f);   
  if( (_tmNow - _penWeapons->m_tmWeaponChangeRequired) < hud_tmWeaponsOnScreen) {
    // determine number of weapons that player has
    INDEX ctWeapons = 0;
    for( i=WEAPON_NONE+1; i<WEAPON_LAST; i++) {
      if( _awiWeapons[i].wi_wtWeapon!=WEAPON_NONE && _awiWeapons[i].wi_wtWeapon!=WEAPON_DOUBLECOLT &&
          _awiWeapons[i].wi_bHasWeapon) ctWeapons++;
    }
    // display all available weapons
    fRow = pixBottomBound - fHalfUnit - 3*fNextUnit;
    fCol = 320.0f - (ctWeapons*fAdvUnit-fHalfUnit)/2.0f;
    // display all available weapons
    for( INDEX ii=WEAPON_NONE+1; ii<WEAPON_LAST; ii++) {
      i = aiWeaponsRemap[ii];
      // skip if hasn't got this weapon
      if( _awiWeapons[i].wi_wtWeapon==WEAPON_NONE || _awiWeapons[i].wi_wtWeapon==WEAPON_DOUBLECOLT
         || !_awiWeapons[i].wi_bHasWeapon) continue;
      // display weapon icon
      COLOR colBorder = _colHUD;
      colIcon = 0xccddff00;
      // weapon that is currently selected has different colors
      if( ptoWantedWeapon == _awiWeapons[i].wi_ptoWeapon) {
        colIcon = 0xffcc0000;
        colBorder = 0xffcc0000;
      }
      // no ammo
      if( _awiWeapons[i].wi_paiAmmo!=NULL && _awiWeapons[i].wi_paiAmmo->ai_iAmmoAmmount==0) {
        _fCustomScalingAdjustment = 0.9f; //#### 0.7f - 1.0f
        HUD_DrawBorder( fCol, fRow, fOneUnit, fOneUnit, 0x22334400);
        _fCustomScalingAdjustment = 0.5f;
        HUD_DrawIcon(   fCol, fRow, *_awiWeapons[i].wi_ptoWeapon, 0x22334400, 1.0f, FALSE);
      // yes ammo
      } else {
        _fCustomScalingAdjustment = 1.0f; //#### 0.7f - 1.0f
        HUD_DrawBorder( fCol, fRow, fOneUnit, fOneUnit, colBorder);
        _fCustomScalingAdjustment = 0.5f;
        HUD_DrawIcon(   fCol, fRow, *_awiWeapons[i].wi_ptoWeapon, colIcon, 1.0f, FALSE);
      }
      // advance to next position
      fCol += fAdvUnit;
    }
  }
  _fCustomScalingAdjustment = 1.0f;

  // reduce icon sizes a bit
  const FLOAT fUpperSize = ClampDn(_fCustomScaling*0.5f, 0.5f)/_fCustomScaling;
  _fCustomScaling*=fUpperSize;
  ASSERT( _fCustomScaling>=0.5f);
  fChrUnit  *= fUpperSize;
  fOneUnit  *= fUpperSize;
  fHalfUnit *= fUpperSize;
  fAdvUnit  *= fUpperSize;
  fNextUnit *= fUpperSize;

  // draw oxygen info if needed
  BOOL bOxygenOnScreen = FALSE;
  fValue = _penPlayer->en_tmMaxHoldBreath - (_pTimer->CurrentTick() - _penPlayer->en_tmLastBreathed);
  if( _penPlayer->IsConnected() && (_penPlayer->GetFlags()&ENF_ALIVE) && fValue<30.0f) { 
    // prepare and draw oxygen info
    fRow = pixTopBound + fOneUnit + fNextUnit;
    fCol = 280.0f;
    fAdv = fAdvUnit + fOneUnit*4/2 - fHalfUnit;
    PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.5f, 0.25f, FALSE);
    fNormValue = fValue/30.0f;
    fNormValue = ClampDn(fNormValue, 0.0f);
    HUD_DrawBorder( fCol,      fRow, fOneUnit,         fOneUnit, colBorder);
    HUD_DrawBorder( fCol+fAdv, fRow, fOneUnit*4,       fOneUnit, colBorder);
    HUD_DrawBar(    fCol+fAdv, fRow, (INDEX) (fOneUnit*4*0.975), (INDEX) (fOneUnit*0.9375), BO_LEFT, NONE, fNormValue);
    HUD_DrawIcon(   fCol,      fRow, _toOxygen, C_WHITE /*_colHUD*/, fNormValue, TRUE);
    bOxygenOnScreen = TRUE;
  }

  // draw boss energy if needed
  if( _penPlayer->m_penMainMusicHolder!=NULL) {
    CMusicHolder &mh = (CMusicHolder&)*_penPlayer->m_penMainMusicHolder;
    fNormValue = 0;

    if( mh.m_penBoss!=NULL && (mh.m_penBoss->en_ulFlags&ENF_ALIVE)) {
      CEnemyBase &eb = (CEnemyBase&)*mh.m_penBoss;
      ASSERT( eb.m_fMaxHealth>0);
      fValue = eb.GetHealth();
      fNormValue = fValue/eb.m_fMaxHealth;
    }
    if( mh.m_penCounter!=NULL) {
      CEnemyCounter &ec = (CEnemyCounter&)*mh.m_penCounter;
      if (ec.m_iCount>0) {
        fValue = ec.m_iCount;
        fNormValue = fValue/ec.m_iCountFrom;
      }
    }
    if( fNormValue>0) {
      // prepare and draw boss energy info
      //PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.5f, 0.25f, FALSE);
      PrepareColorTransitions( colMax, colMax, colTop, C_RED, 0.5f, 0.25f, FALSE);
      
      fRow = pixTopBound + fOneUnit + fNextUnit;
      fCol = 184.0f;
      fAdv = fAdvUnit+ fOneUnit*16/2 -fHalfUnit;
      if( bOxygenOnScreen) fRow += fNextUnit;
      HUD_DrawBorder( fCol,      fRow, fOneUnit,          fOneUnit, colBorder);
      HUD_DrawBorder( fCol+fAdv, fRow, fOneUnit*16,       fOneUnit, colBorder);
      HUD_DrawBar(    fCol+fAdv, fRow, (INDEX) (fOneUnit*16*0.995), (INDEX) (fOneUnit*0.9375), BO_LEFT, NONE, fNormValue);
      HUD_DrawIcon(   fCol,      fRow, _toHealth, C_WHITE /*_colHUD*/, fNormValue, FALSE);
    }
  }


  // determine scaling of normal text and play mode
  //const FLOAT fTextScale  = (_fResolutionScaling+1) *0.5f;
  //const BOOL bSinglePlay  =  GetSP()->sp_bSinglePlayer;
  const BOOL bCooperative =  GetSP()->sp_bCooperative && !bSinglePlay;
  const BOOL bScoreMatch  = !GetSP()->sp_bCooperative && !GetSP()->sp_bUseFrags;
  const BOOL bFragMatch   = !GetSP()->sp_bCooperative &&  GetSP()->sp_bUseFrags;
  COLOR colMana, colFrags, colDeaths, colHealth, colArmor;
  //COLOR colScore  = _colHUD;
  INDEX iScoreSum = 0;

  // if not in single player mode, we'll have to calc (and maybe printout) other players' info
  if( !bSinglePlay)
  {
    // set font and prepare font parameters
    _pfdDisplayFont->SetVariableWidth();
    _pDP->SetFont( _pfdDisplayFont);
    _pDP->SetTextScaling( fTextScale);
    FLOAT fCharHeight = (_pfdDisplayFont->GetHeight()-2)*fTextScale * 0.9;
    // generate and sort by mana list of active players
    BOOL bMaxScore=TRUE, bMaxMana=TRUE, bMaxFrags=TRUE, bMaxDeaths=TRUE;
    hud_iSortPlayers = Clamp( hud_iSortPlayers, (INDEX)-1, (INDEX)6);
    SortKeys eKey = (SortKeys)hud_iSortPlayers;
    if (hud_iSortPlayers==-1) {
           if (bCooperative) eKey = PSK_HEALTH;
      else if (bScoreMatch)  eKey = PSK_SCORE;
      else if (bFragMatch)   eKey = PSK_FRAGS;
      else { ASSERT(FALSE);  eKey = PSK_NAME; }
    }
    if( bCooperative) eKey = (SortKeys)Clamp( (INDEX)eKey, (INDEX)0, (INDEX)3);
    if( eKey==PSK_HEALTH && (bScoreMatch || bFragMatch)) { eKey = PSK_NAME; }; // prevent health snooping in deathmatch
    INDEX iPlayers = SetAllPlayersStats(eKey);

    _pDP->SetTextScaling( fTextScale * 0.8);

    // loop thru players 
    for( INDEX i=0; i<iPlayers; i++)
    { // get player name and mana
      CPlayer *penPlayer = _apenPlayers[i];
      const CTString strName = penPlayer->GetPlayerName();
      const INDEX iScore  = penPlayer->m_psGameStats.ps_iScore;
      const INDEX iMana   = penPlayer->m_iMana;
      const INDEX iFrags  = penPlayer->m_psGameStats.ps_iKills;
      const INDEX iDeaths = penPlayer->m_psGameStats.ps_iDeaths;
      const INDEX iHealth = ClampDn( (INDEX)ceil( penPlayer->GetHealth()), (INDEX)0);
      const INDEX iArmor  = ClampDn( (INDEX)ceil( penPlayer->m_fArmor), (INDEX)0);
      CTString strScore, strMana, strFrags, strDeaths, strHealth, strArmor, strPing, strKillsC, strKillsT;
      strScore.PrintF(  "%d", iScore);
      strMana.PrintF(   "%d", iMana);
      strFrags.PrintF(  "%d", iFrags);
      strDeaths.PrintF( "%d", iDeaths);
      strHealth.PrintF( "%d", iHealth);
      strArmor.PrintF(  "%d", iArmor);
      // detemine corresponding colors
      colHealth = C_mlRED;
      colMana = colScore = colFrags = colDeaths = colArmor = C_lGRAY;
      if( iMana   > _penPlayer->m_iMana)                      { bMaxMana   = FALSE; colMana   = C_WHITE; }
      if( iScore  > _penPlayer->m_psGameStats.ps_iScore)      { bMaxScore  = FALSE; colScore  = C_WHITE; }
      if( iFrags  > _penPlayer->m_psGameStats.ps_iKills)      { bMaxFrags  = FALSE; colFrags  = C_WHITE; }
      if( iDeaths > _penPlayer->m_psGameStats.ps_iDeaths)     { bMaxDeaths = FALSE; colDeaths = C_WHITE; }
      if( penPlayer==_penPlayer) colScore = colMana = colFrags = colDeaths = _colHUD; // current player
      //if( iHealth>25) colHealth = _colHUD;
//####
	  //const INDEX iHealth = ClampDn( (INDEX)ceil( penPlayer->GetHealth()), 0L);
	  //strHealth.PrintF( "%d", iHealth);
	  fValue = ClampDn( penPlayer->GetHealth(), 0.0f);  // never show negative health
	  if (fValue > 500.0f)
	  {
		//strValue=" ~";
		fValue=999.0f;
	  }
	  strHealth.PrintF( "%d", (SLONG)ceil(fValue));
	      //if( iHealth>25) colHealth = _colHUD;
	      if( fValue>25.0f) colHealth = _colHUD;
	      const INDEX iPing   = ceil(penPlayer->en_tmPing*1000.0f);
	      strPing.PrintF("%d", iPing);
	      strKillsC.PrintF("%d", penPlayer->m_psLevelStats.ps_iKills);
	      strKillsT.PrintF("%d", penPlayer->m_psLevelTotal.ps_iKills>9999?9999:penPlayer->m_psLevelTotal.ps_iKills);
//####
      if( iArmor >25) colArmor  = _colHUD;
      // eventually print it out
      if( hud_iShowPlayers==1 || (hud_iShowPlayers==-1 && !bSinglePlay)) {
        // printout location and info aren't the same for deathmatch and coop play
        const FLOAT fCharWidth = (PIX)((_pfdDisplayFont->GetWidth()-2) *fTextScale);
//####
	    long textoffset = 1;
        if( bCooperative)
	    {
	        if (hud_bShowKills)
		    {
		       textoffset += (TXT_WIDTH_KILLS /2);
   	           _pDP->PutTextC(strKillsT,			_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_MAGENTA |_ulAlphaHUD); textoffset += (TXT_WIDTH_KILLS/2);
	           _pDP->PutText("/",					_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_MAGENTA |_ulAlphaHUD);	textoffset += 2;
	           _pDP->PutTextC(strKillsC,			_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_MAGENTA |_ulAlphaHUD);	textoffset += (TXT_WIDTH_KILLS/2);
	        }
	        if (hud_bShowPing)
		    {
	           _pDP->PutTextR("ms",				     _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_YELLOW |_ulAlphaHUD);	textoffset += 2;
	           _pDP->PutTextR(strPing,			     _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_YELLOW |_ulAlphaHUD);	textoffset += TXT_WIDTH_PING+1;
		    }
	       	_pDP->PutTextC(strArmor,			     _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, colArmor |_ulAlphaHUD);	textoffset += (TXT_WIDTH_ARMOR/2);
	       	_pDP->PutText("/",					     _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, _colHUD  |_ulAlphaHUD);	textoffset += 2;
	       	_pDP->PutTextC(strHealth,			     _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, colHealth|_ulAlphaHUD);	textoffset += (TXT_WIDTH_HEALTH/2);
	    }else if( bScoreMatch) {
	        if (hud_bShowPing)
			{
	           _pDP->PutTextR("ms",				     _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_YELLOW |_ulAlphaHUD);	textoffset += 2;
	           _pDP->PutTextR(strPing,				 _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_YELLOW |_ulAlphaHUD);	textoffset += TXT_WIDTH_PING+1;
			}
			_pDP->PutTextC(strMana,                  _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, colMana |_ulAlphaHUD);	textoffset += (TXT_WIDTH_MANA/2);
			_pDP->PutText("/",                       _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, _colHUD |_ulAlphaHUD);	textoffset += 2;
			_pDP->PutTextC(strScore,                 _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, colScore|_ulAlphaHUD);	textoffset += (TXT_WIDTH_SCORE/2);
	    }else { // fragmatch!
	        if (hud_bShowPing)
			{
	           _pDP->PutTextR("ms",				     _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_YELLOW |_ulAlphaHUD);	textoffset += 2;
	           _pDP->PutTextR(strPing,		         _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, C_YELLOW |_ulAlphaHUD);	textoffset += TXT_WIDTH_PING+1;
			}
			_pDP->PutTextC(strDeaths,                _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, colDeaths|_ulAlphaHUD);	textoffset += (TXT_WIDTH_DEATHS/2);
			_pDP->PutText("/",                       _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, _colHUD  |_ulAlphaHUD);	textoffset += 2;
			_pDP->PutTextC(strFrags,                 _pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, colFrags |_ulAlphaHUD);	textoffset += (TXT_WIDTH_FRAGS/2);
	    }
		_pDP->PutTextR(strName,	_pixDPWidth - textoffset * fCharWidth, fCharHeight*i+fOneUnit * 2, colScore |_ulAlphaHUD);
	  }
      // calculate summ of scores (for coop mode)
      iScoreSum += iScore;
    }
//####
 
    // prepare color for local player printouts
    bMaxScore  ? colScore  = C_WHITE : colScore  = C_lGRAY;
    bMaxMana   ? colMana   = C_WHITE : colMana   = C_lGRAY;
    bMaxFrags  ? colFrags  = C_WHITE : colFrags  = C_lGRAY;
    bMaxDeaths ? colDeaths = C_WHITE : colDeaths = C_lGRAY;
  }


  // printout player latency if needed
  if( hud_bShowLatency) {
    CTString strLatency;
    strLatency.PrintF( "%4.0fms", _penPlayer->m_tmLatency*1000.0f);
    PIX pixFontHeight = (PIX)(_pfdDisplayFont->GetHeight() *fTextScale +fTextScale+1);
    _pfdDisplayFont->SetFixedWidth();
    _pDP->SetFont( _pfdDisplayFont);
    _pDP->SetTextScaling( fTextScale);
    _pDP->SetTextCharSpacing( -2.0f*fTextScale);
    _pDP->PutTextR( strLatency, _pixDPWidth, _pixDPHeight-pixFontHeight, C_WHITE|CT_OPAQUE);
  }
  // restore font defaults
  _pfdDisplayFont->SetVariableWidth();
  _pDP->SetFont( &_fdNumbersFont);
  _pDP->SetTextCharSpacing(1);

  // prepare output strings and formats depending on game type
  FLOAT fWidthAdj = 8;
  INDEX iScore = _penPlayer->m_psGameStats.ps_iScore;
  INDEX iMana  = _penPlayer->m_iMana;
  if( bFragMatch) {
    if (!hud_bShowMatchInfo) { fWidthAdj = 4; }
    iScore = _penPlayer->m_psGameStats.ps_iKills;
    iMana  = _penPlayer->m_psGameStats.ps_iDeaths;
  } else if( bCooperative) {
    // in case of coop play, show squad (common) score
    iScore = iScoreSum;
  }

  if( hud_bShowScore ) {
    // prepare and draw score or frags info 
    strValue.PrintF( "%d", iScore);
    fRow = pixTopBound  +fHalfUnit;
    fCol = pixLeftBound +fHalfUnit;
    fAdv = fAdvUnit+ fChrUnit*fWidthAdj/2 -fHalfUnit;
    HUD_DrawBorder( fCol,      fRow, fOneUnit,           fOneUnit, colBorder);
    HUD_DrawBorder( fCol+fAdv, fRow, fChrUnit*fWidthAdj, fOneUnit, colBorder);
    HUD_DrawText(   fCol+fAdv, fRow, strValue, colScore, 1.0f);
    HUD_DrawIcon(   fCol,      fRow, _toFrags, C_WHITE /*colScore*/, 1.0f, FALSE);
  }

  // eventually draw mana info 
  if( bScoreMatch || bFragMatch) {
    strValue.PrintF( "%d", iMana);
    fRow = pixTopBound  + fNextUnit+fHalfUnit *_fFragScorerHeightAdjuster;// 0.75f; //1.5f;// 1.17f;// 2.35f;
    fCol = pixLeftBound + fHalfUnit;
    fAdv = fAdvUnit+ fChrUnit*fWidthAdj/2 -fHalfUnit;
    HUD_DrawBorder( fCol,      fRow, fOneUnit,           fOneUnit, colBorder);
    HUD_DrawBorder( fCol+fAdv, fRow, fChrUnit*fWidthAdj, fOneUnit, colBorder);
    HUD_DrawText(   fCol+fAdv, fRow, strValue,  colMana, 1.0f);
    //HUD_DrawIcon(   fCol,      fRow, _toDeaths, C_WHITE /*colMana*/, 1.0f, FALSE);
  }

  // if single player or cooperative mode
  if( (bSinglePlay || bCooperative) && hud_bShowScore)
  {
    // prepare and draw hiscore info 
    strValue.PrintF( "%d", Max(_penPlayer->m_iHighScore, _penPlayer->m_psGameStats.ps_iScore));
    BOOL bBeating = _penPlayer->m_psGameStats.ps_iScore>_penPlayer->m_iHighScore;
    fRow = pixTopBound+fHalfUnit;
    fCol = 320.0f-fOneUnit-fChrUnit*8/2;
    fAdv = fAdvUnit+ fChrUnit*8/2 -fHalfUnit;
    HUD_DrawBorder( fCol,      fRow, fOneUnit,   fOneUnit, colBorder);
    HUD_DrawBorder( fCol+fAdv, fRow, fChrUnit*8, fOneUnit, colBorder);
    HUD_DrawText(   fCol+fAdv, fRow, strValue, NONE, bBeating ? 0.0f : 1.0f);
    HUD_DrawIcon(   fCol,      fRow, _toHiScore, C_WHITE /*_colHUD*/, 1.0f, FALSE);

    // prepare and draw unread messages
    if( hud_bShowMessages && _penPlayer->m_ctUnreadMessages>0) {
      strValue.PrintF( "%d", _penPlayer->m_ctUnreadMessages);
      //fRow = pixTopBound+fHalfUnit;
      fRow = pixBottomBound- (fNextUnit+fHalfUnit) * _fArmorHeightAdjuster - 21.0f;    
      fCol = pixRightBound-fHalfUnit-fAdvUnit-fChrUnit*4;
      const FLOAT tmIn = 0.5f;
      const FLOAT tmOut = 0.5f;
      const FLOAT tmStay = 2.0f;
      FLOAT tmDelta = _pTimer->GetLerpedCurrentTick()-_penPlayer->m_tmAnimateInbox;
      COLOR col = _colHUD;
      if (tmDelta>0 && tmDelta<(tmIn+tmStay+tmOut) && bSinglePlay) {
        FLOAT fRatio = 0.0f;
        if (tmDelta<tmIn) {
          fRatio = tmDelta/tmIn;
        } else if (tmDelta>tmIn+tmStay) {
          fRatio = (tmIn+tmStay+tmOut-tmDelta)/tmOut;
        } else {
          fRatio = 1.0f;
        }
        fRow-=fAdvUnit*5*fRatio;
        fCol-=fAdvUnit*15*fRatio;
        col = LerpColor(_colHUD, C_WHITE|0xFF, fRatio);
      }
      fAdv = fAdvUnit+ fChrUnit*4/2 -fHalfUnit;
      HUD_DrawBorder( fCol,      fRow, fOneUnit,   fOneUnit, col);
      HUD_DrawBorder( fCol+fAdv, fRow, fChrUnit*4, fOneUnit, col);
      HUD_DrawText(   fCol+fAdv, fRow, strValue,   col, 1.0f);
      HUD_DrawIcon(   fCol,      fRow, _toMessage, C_WHITE /*col*/, 0.0f, TRUE);
    }
  }

  #ifdef ENTITY_DEBUG
  // if entity debug is on, draw entity stack
  HUD_DrawEntityStack();
  #endif

  // draw cheat modes
  if( GetSP()->sp_ctMaxPlayers==1) {
    INDEX iLine=1;
    ULONG ulAlpha = (ULONG) (sin(_tmNow*16)*96 +128);
    PIX pixFontHeight = _pfdConsoleFont->fd_pixCharHeight;
    const COLOR colCheat = _colHUDText;
    _pDP->SetFont( _pfdConsoleFont);
    _pDP->SetTextScaling( 1.0f);
    const FLOAT fchtTM = cht_fTranslationMultiplier; // for text formatting sake :)
    if( fchtTM > 1.0f)  { _pDP->PutTextR( "turbo",     _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
    if( cht_bInvisible) { _pDP->PutTextR( "invisible", _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
    if( cht_bGhost)     { _pDP->PutTextR( "ghost",     _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
    if( cht_bFly)       { _pDP->PutTextR( "fly",       _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
    if( cht_bGod)       { _pDP->PutTextR( "god",       _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
  }

  // in the end, remember the current time so it can be used in the next frame
  _tmLast = _tmNow;

}



// initialize all that's needed for drawing the HUD
extern void InitHUD(void)
{
  // try to
  try {
    // initialize and load HUD numbers font
    DECLARE_CTFILENAME( fnFont, "Fonts\\Numbers3.fnt");
    _fdNumbersFont.Load_t( fnFont);
    //_fdNumbersFont.SetCharSpacing(0);

    // initialize status bar textures
    _toHealth.SetData_t(  CTFILENAME("TexturesMP\\Interface\\HSuper.tex"));
    _toOxygen.SetData_t(  CTFILENAME("TexturesMP\\Interface\\Oxygen-2.tex"));
    _toFrags.SetData_t(   CTFILENAME("TexturesMP\\Interface\\IBead.tex"));
    _toDeaths.SetData_t(  CTFILENAME("TexturesMP\\Interface\\ISkull.tex"));
    _toScore.SetData_t(   CTFILENAME("TexturesMP\\Interface\\IScore.tex"));
    _toHiScore.SetData_t( CTFILENAME("TexturesMP\\Interface\\IHiScore.tex"));
    _toMessage.SetData_t( CTFILENAME("TexturesMP\\Interface\\IMessage.tex"));
    _toMana.SetData_t(    CTFILENAME("TexturesMP\\Interface\\IValue.tex"));
    _toArmorSmall.SetData_t(  CTFILENAME("TexturesMP\\Interface\\ArSmall.tex"));
    _toArmorMedium.SetData_t(   CTFILENAME("TexturesMP\\Interface\\ArMedium.tex"));
    _toArmorLarge.SetData_t(   CTFILENAME("TexturesMP\\Interface\\ArStrong.tex"));

    // initialize ammo textures                    
    _toAShells.SetData_t(        CTFILENAME("TexturesMP\\Interface\\AmShells.tex"));
    _toABullets.SetData_t(       CTFILENAME("TexturesMP\\Interface\\AmBullets.tex"));
    _toARockets.SetData_t(       CTFILENAME("TexturesMP\\Interface\\AmRockets.tex"));
    _toAGrenades.SetData_t(      CTFILENAME("TexturesMP\\Interface\\AmGrenades.tex"));
    _toANapalm.SetData_t(        CTFILENAME("TexturesMP\\Interface\\AmFuelReservoir.tex"));
    _toAElectricity.SetData_t(   CTFILENAME("TexturesMP\\Interface\\AmElectricity.tex"));
    _toAIronBall.SetData_t(      CTFILENAME("TexturesMP\\Interface\\AmCannonBall.tex"));
    _toASniperBullets.SetData_t( CTFILENAME("TexturesMP\\Interface\\AmSniperBullets.tex"));
    _toASeriousBomb.SetData_t(   CTFILENAME("TexturesMP\\Interface\\AmSeriousBomb.tex"));
    // initialize weapon textures
    _toWKnife.SetData_t(           CTFILENAME("TexturesMP\\Interface\\WKnife.tex"));
    _toWColt.SetData_t(            CTFILENAME("TexturesMP\\Interface\\WColt.tex"));
    _toWSingleShotgun.SetData_t(   CTFILENAME("TexturesMP\\Interface\\WSingleShotgun.tex"));
    _toWDoubleShotgun.SetData_t(   CTFILENAME("TexturesMP\\Interface\\WDoubleShotgun.tex"));
    _toWTommygun.SetData_t(        CTFILENAME("TexturesMP\\Interface\\WTommygun.tex"));
    _toWMinigun.SetData_t(         CTFILENAME("TexturesMP\\Interface\\WMinigun.tex"));
    _toWRocketLauncher.SetData_t(  CTFILENAME("TexturesMP\\Interface\\WRocketLauncher.tex"));
    _toWGrenadeLauncher.SetData_t( CTFILENAME("TexturesMP\\Interface\\WGrenadeLauncher.tex"));
    _toWLaser.SetData_t(           CTFILENAME("TexturesMP\\Interface\\WLaser.tex"));
    _toWIronCannon.SetData_t(      CTFILENAME("TexturesMP\\Interface\\WCannon.tex"));
    _toWChainsaw.SetData_t(        CTFILENAME("TexturesMP\\Interface\\WChainsaw.tex"));
    _toWSniper.SetData_t(          CTFILENAME("TexturesMP\\Interface\\WSniper.tex"));
    _toWFlamer.SetData_t(          CTFILENAME("TexturesMP\\Interface\\WFlamer.tex"));
        
    // initialize powerup textures (DO NOT CHANGE ORDER!)
    _atoPowerups[0].SetData_t( CTFILENAME("TexturesMP\\Interface\\PInvisibility.tex"));
    _atoPowerups[1].SetData_t( CTFILENAME("TexturesMP\\Interface\\PInvulnerability.tex"));
    _atoPowerups[2].SetData_t( CTFILENAME("TexturesMP\\Interface\\PSeriousDamage.tex"));
    _atoPowerups[3].SetData_t( CTFILENAME("TexturesMP\\Interface\\PSeriousSpeed.tex"));
    // initialize sniper mask texture
    _toSniperMask.SetData_t(       CTFILENAME("TexturesMP\\Interface\\SniperMask.tex"));
    _toSniperWheel.SetData_t(       CTFILENAME("TexturesMP\\Interface\\SniperWheel.tex"));
    _toSniperArrow.SetData_t(       CTFILENAME("TexturesMP\\Interface\\SniperArrow.tex"));
    _toSniperEye.SetData_t(       CTFILENAME("TexturesMP\\Interface\\SniperEye.tex"));
    _toSniperLed.SetData_t(       CTFILENAME("TexturesMP\\Interface\\SniperLed.tex"));

    // initialize tile texture
    _toTile.SetData_t( CTFILENAME("Textures\\Interface\\Tile.tex"));
    
    // set all textures as constant
    ((CTextureData*)_toHealth .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toOxygen .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toFrags  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toDeaths .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toScore  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toHiScore.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMessage.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMana   .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toArmorSmall.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toArmorMedium.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toArmorLarge.GetData())->Force(TEX_CONSTANT);

    ((CTextureData*)_toAShells       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toABullets      .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toARockets      .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toAGrenades     .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toANapalm       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toAElectricity  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toAIronBall     .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toASniperBullets.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toASeriousBomb  .GetData())->Force(TEX_CONSTANT);

    ((CTextureData*)_toWKnife          .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWColt           .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWSingleShotgun  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWDoubleShotgun  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWTommygun       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWRocketLauncher .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWGrenadeLauncher.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWChainsaw       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWLaser          .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWIronCannon     .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWSniper         .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWMinigun        .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWFlamer         .GetData())->Force(TEX_CONSTANT);
    
    ((CTextureData*)_atoPowerups[0].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_atoPowerups[1].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_atoPowerups[2].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_atoPowerups[3].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toTile      .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toSniperMask.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toSniperWheel.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toSniperArrow.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toSniperEye.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toSniperLed.GetData())->Force(TEX_CONSTANT);

  }
  catch (const char *strError) {
    FatalError( strError);
  }

}


// clean up
extern void EndHUD(void)
{

}

