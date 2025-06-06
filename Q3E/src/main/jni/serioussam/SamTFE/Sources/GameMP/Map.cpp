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

static CTextureObject atoIcons[15];
static CTextureObject _toPathDot;
static CTextureObject _toMapBcgLD;
static CTextureObject _toMapBcgLU;
static CTextureObject _toMapBcgRD;
static CTextureObject _toMapBcgRU;

PIX aIconCoords[][2] =
{
  {175,404},  // 00: Hatshepsut
  {60,381},   // 01: Sand Canyon
  {50,300},   // 02: Ramses
  {171,304},  // 03: Canyon
  {190,225},  // 04: Waterfall
  {303,305},  // 05: Oasis
  {361,296},  // 06: Dunes
  {362,222},  // 07: Suburbs
  {321,211},  // 08: Sewers
  {316,156},  // 09: Metropolis
  {194,157},  // 10: Sphynx
  {160,111},  // 11: Karnak
  {167,61},   // 12: Luxor
  {50,53},    // 13: Sacred
  {185,0},    // 14: Pyramid
};

#define HATSHEPSUT_BIT 0
#define SAND_BIT 1
#define RAMSES_BIT 2
#define CANYON_BIT 3
#define WATERFALL_BIT 4
#define OASIS_BIT 5
#define DUNES_BIT 6
#define SUBURBS_BIT 7
#define SEWERS_BIT 8
#define METROPOLIS_BIT 9
#define SPHYNX_BIT 10
#define KARNAK_BIT 11
#define LUXOR_BIT 12
#define SACRED_BIT 13
#define PYRAMID_BIT 14

INDEX  aPathPrevNextLevels[][2] = 
{
  {HATSHEPSUT_BIT, SAND_BIT},     // 00
  {SAND_BIT, RAMSES_BIT},         // 01
  {RAMSES_BIT, CANYON_BIT},       // 02
  {CANYON_BIT, WATERFALL_BIT},    // 03
  {CANYON_BIT, OASIS_BIT},        // 04
  {WATERFALL_BIT, OASIS_BIT},     // 05
  {OASIS_BIT, DUNES_BIT},         // 06
  {DUNES_BIT, SUBURBS_BIT},       // 07
  {SUBURBS_BIT, SEWERS_BIT},      // 08
  {SEWERS_BIT, METROPOLIS_BIT},   // 09
  {METROPOLIS_BIT, SPHYNX_BIT},   // 10
  {SPHYNX_BIT, KARNAK_BIT},       // 11
  {KARNAK_BIT, LUXOR_BIT},        // 12
  {LUXOR_BIT, SACRED_BIT},        // 13
  {SACRED_BIT, PYRAMID_BIT},      // 14
  {LUXOR_BIT, PYRAMID_BIT},       // 15
};

PIX aPathDots[][10][2] = 
{
  // 00: Hatshepsut - Sand
  {
    {207,435},
    {196,440},
    {184,444},
    {172,443},
    {162,439},
    {156,432},
    {-1,-1},
  },
  
  // 01: Sand - Ramses
  {
    {115,388},
    {121,382},
    {128,377},
    {136,371},
    {-1,-1},
  },

  // 02: Ramses - Canyon
  {
    {148,368},
    {159,370},
    {169,374},
    {177,379},
    {187,381},
    {200,380},
    {211,376},
    {-1,-1},
  },

  // 03: Canyon - Waterfall
  {
    {273,339},
    {276,331},
    {278,322},
    {280,313},
    {279,305},
    {273,298},
    {266,293},
    {260,288},
    {-1,-1},
  },

  // 04: Canyon - Oasis
  {
    {288,360},
    {295,355},
    {302,360},
    {310,364},
    {319,367},
    {328,368},
    {-1,-1},
  },

  // 05: Waterfall - Oasis
  {
    {294,279},
    {302,282},
    {310,287},
    {316,294},
    {320,302},
    {323,310},
    {327,318},
    {332,326},
    {337,333},
    {-1,-1},
  },

  // 06: Oasis - Dunes
  {
    {384,360},
    {394,358},
    {405,353},
    {414,347},
    {421,339},
    {426,329},
    {-1,-1},
  },

  // 07: Dunes - Suburbs
  {
    {439,305},
    {434,300},
    {429,293},
    {-1,-1},
  },

  // 08: Suburbs - Sewers
  {
    {403,250},
    {402,244},
    {401,238},
    {398,232},
    {-1,-1},
  },

  // 09: Sewers - Metropolis
  {
    {372,266},
    {371,221},
    {370,216},
    {-1,-1},
  },

  // 10: Metropolis - Alley
  {
    {317,211},
    {310,215},
    {302,219},
    {293,222},
    {283,222},
    {273,221},
    {265,218},
    {-1,-1},
  },

  // 11: Alley - Karnak
  {
    {260,189},
    {259,181},
    {255,174},
    {249,168},
    {241,165},
    {233,164},
    {-1,-1},
  },

  // 12: Karnak - Luxor
  {
    {228,143},
    {228,136},
    {226,129},
    {221,123},
    {-1,-1},
  },

  // 13: Luxor - Sacred
  {
    {175,101},
    {169,106},
    {162,111},
    {154,113},
    {145,113},
    {136,112},
    {-1,-1},
  },

  // 14: Sacred - Pyramid
  {
    {126,59},
    {134,55},
    {142,52},
    {151,49},
    {160,47},
    {170,47},
    {179,48},
    {188,51},
    {-1,-1},
  },

  // 15: Luxor - Pyramid
  {
    {212,71},
    {217,66},
    {225,63},
    {234,63},
    {244,63},
    {253,62},
    {261,59},
    {-1,-1},
  },
};

BOOL ObtainMapData(void)
{
  try {
    atoIcons[ 0].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level00.tex"));
    atoIcons[ 1].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level01.tex"));
    atoIcons[ 2].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level02.tex"));
    atoIcons[ 3].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level03.tex"));
    atoIcons[ 4].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level04.tex"));
    atoIcons[ 5].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level05.tex"));
    atoIcons[ 6].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level06.tex"));
    atoIcons[ 7].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level07.tex"));
    atoIcons[ 8].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level08.tex"));
    atoIcons[ 9].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level09.tex"));
    atoIcons[10].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level10.tex"));
    atoIcons[11].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level11.tex"));
    atoIcons[12].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level12.tex"));
    atoIcons[13].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level13.tex"));
    atoIcons[14].SetData_t(CTFILENAME("Textures\\Computer\\Map\\Level14.tex"));
    _toPathDot  .SetData_t(CTFILENAME("Textures\\Computer\\Map\\PathDot.tex"));
    _toMapBcgLD .SetData_t(CTFILENAME("Textures\\Computer\\Map\\MapBcgLD.tex"));
    _toMapBcgLU .SetData_t(CTFILENAME("Textures\\Computer\\Map\\MapBcgLU.tex"));
    _toMapBcgRD .SetData_t(CTFILENAME("Textures\\Computer\\Map\\MapBcgRD.tex"));
    _toMapBcgRU .SetData_t(CTFILENAME("Textures\\Computer\\Map\\MapBcgRU.tex"));
    // force constant textures
    ((CTextureData*)atoIcons[ 0].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 1].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 2].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 3].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 4].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 5].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 6].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 7].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 8].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[ 9].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[10].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[11].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[12].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[13].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[14].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toPathDot  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMapBcgLD .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMapBcgLU .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMapBcgRD .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMapBcgRU .GetData())->Force(TEX_CONSTANT);
  } 
  catch (char *strError) {
    CPrintF("%s\n", strError);
    return FALSE;
  }
  return TRUE;
}

void ReleaseMapData(void)
{
  atoIcons[0].SetData(NULL);
  atoIcons[1].SetData(NULL);
  atoIcons[2].SetData(NULL);
  atoIcons[3].SetData(NULL);
  atoIcons[4].SetData(NULL);
  atoIcons[5].SetData(NULL);
  atoIcons[6].SetData(NULL);
  atoIcons[7].SetData(NULL);
  atoIcons[8].SetData(NULL);
  atoIcons[9].SetData(NULL);
  atoIcons[10].SetData(NULL);
  atoIcons[11].SetData(NULL);
  atoIcons[12].SetData(NULL);
  atoIcons[13].SetData(NULL);
  atoIcons[14].SetData(NULL);
  _toPathDot.SetData(NULL);
  _toMapBcgLD.SetData(NULL);
  _toMapBcgLU.SetData(NULL);
  _toMapBcgRD.SetData(NULL);
  _toMapBcgRU.SetData(NULL);
}

void RenderMap( CDrawPort *pdp, ULONG ulLevelMask, CProgressHookInfo *pphi)
{
  if( !ObtainMapData())
  {
    ReleaseMapData();
    return;
  }

  PIX pixdpw = (PIX) pdp->GetWidth();
  PIX pixdph = (PIX) pdp->GetHeight();
  PIX imgw = 512;
  PIX imgh = 480;
  FLOAT fStretch = 0.25f;

  // determine max available picture stretch
  if( pixdpw>=imgw*2 && pixdph>=imgh*2) {
    fStretch = 2.0f;
  } else if(pixdpw>=imgw && pixdph>=imgh) {
    fStretch = 1.0f;
  } else if(pixdpw>=imgw/2 && pixdph>=imgh/2) {
    fStretch = 0.5f;
  }

  // calculate LU offset so picture would be centered in dp
  PIX pixSX = (PIX) ((pixdpw-imgw*fStretch)/2);
  PIX pixSY = Max( PIX((pixdph-imgh*fStretch)/2), PIX(0));

  // render pale map bcg
  PIX pixC1S = pixSX;                          // column 1 start pixel
  PIX pixR1S = pixSY;                          // raw 1 start pixel
  PIX pixC1E = (PIX) (pixSX+256*fStretch);     // column 1 end pixel
  PIX pixR1E = (PIX) (pixSY+256*fStretch);     // raw 1 end pixel
  PIX pixC2S = (PIX) (pixC1E-fStretch);        // column 2 start pixel
  PIX pixR2S = (PIX) (pixR1E-fStretch);        // raw 2 start pixel
  PIX pixC2E = (PIX) (pixC2S+256*fStretch);    // column 2 end pixel
  PIX pixR2E = (PIX) (pixR2S+256*fStretch);    // raw 2 end pixel

  // render pale map bcg
  pdp->PutTexture( &_toMapBcgLU, PIXaabbox2D( PIX2D(pixC1S,pixR1S), PIX2D(pixC1E,pixR1E)), C_WHITE|255);
  pdp->PutTexture( &_toMapBcgRU, PIXaabbox2D( PIX2D(pixC2S,pixR1S), PIX2D(pixC2E,pixR1E)), C_WHITE|255);
  pdp->PutTexture( &_toMapBcgLD, PIXaabbox2D( PIX2D(pixC1S,pixR2S), PIX2D(pixC1E,pixR2E)), C_WHITE|255);
  pdp->PutTexture( &_toMapBcgRD, PIXaabbox2D( PIX2D(pixC2S,pixR2S), PIX2D(pixC2E,pixR2E)), C_WHITE|255);

  // render icons
  for( ULONG iIcon=0; iIcon<ARRAYCOUNT(aIconCoords); iIcon++)
  {
    // if level's icon should be rendered
    if( ulLevelMask & (1UL<<iIcon))
    {
      PIX pixX = (PIX) (aIconCoords[iIcon][0]*fStretch+pixC1S);
      PIX pixY = (PIX) (aIconCoords[iIcon][1]*fStretch+pixR1S);
      CTextureObject *pto = &atoIcons[iIcon];
      PIX pixImgW = (PIX) (((CTextureData *)pto->GetData())->GetPixWidth()*fStretch);
      PIX pixImgH = (PIX) (((CTextureData *)pto->GetData())->GetPixHeight()*fStretch);
      pdp->PutTexture( pto, PIXaabbox2D( PIX2D(pixX, pixY), PIX2D(pixX+pixImgW, pixY+pixImgH)), C_WHITE|255);
    }
  }

  // render paths
  for( ULONG iPath=0; iPath<ARRAYCOUNT(aPathPrevNextLevels); iPath++)
  {
    INDEX iPrevLevelBit = aPathPrevNextLevels[iPath][0];
    INDEX iNextLevelBit = aPathPrevNextLevels[iPath][1];

    // if path dots should be rendered:
    // if path src and dst levels were discovered and secret level isn't inbetween or hasn't been discovered
    if( ulLevelMask&(1UL<<iPrevLevelBit) &&
        ulLevelMask&(1UL<<iNextLevelBit) &&
        ((iNextLevelBit-iPrevLevelBit)==1 || !(ulLevelMask&(1UL<<(iNextLevelBit-1)))))
    {
      for( INDEX iDot=0; iDot<10; iDot++)
      {
        PIX pixDotX=(PIX) (pixC1S+aPathDots[iPath][iDot][0]*fStretch);
        PIX pixDotY=(PIX) (pixR1S+aPathDots[iPath][iDot][1]*fStretch);
        if(aPathDots[iPath][iDot][0]==-1) break;
        pdp->PutTexture( &_toPathDot, PIXaabbox2D( PIX2D(pixDotX, pixDotY), PIX2D(pixDotX+8*fStretch, pixDotY+8*fStretch)),
          C_WHITE|255);
      }
    }
  }

  if( pphi != NULL)
  {
    // set font
    pdp->SetFont( _pfdDisplayFont);
    pdp->SetTextScaling( fStretch);
    pdp->SetTextAspect( 1.0f);
    // set coordinates
    PIX pixhtcx = (PIX) (pixC1S+116*fStretch);          // hook text center x
    PIX pixhtcy = (PIX) (pixR1S+220*fStretch);          // hook text center y

    COLOR colText = RGBToColor(200,128,56)|CT_OPAQUE;
    pdp->PutTextC( pphi->phi_strDescription, pixhtcx, pixhtcy, colText);
    for( INDEX iProgresDot=0; iProgresDot<16; iProgresDot+=1)
    {
      PIX pixDotX = (PIX) (pixC1S+(48+iProgresDot*8)*fStretch);
      PIX pixDotY = (PIX) (pixR1S+249*fStretch);

      COLOR colDot = C_WHITE|255;
      if(iProgresDot>pphi->phi_fCompleted*16)
      {
        colDot = C_WHITE|64;
      }
      pdp->PutTexture( &_toPathDot, PIXaabbox2D( PIX2D(pixDotX, pixDotY), 
        PIX2D(pixDotX+8*fStretch, pixDotY+8*fStretch)), colDot);
    }
  }

  // free textures used in map rendering
  ReleaseMapData();
}
