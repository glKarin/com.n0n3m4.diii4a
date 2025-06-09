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

static CTextureObject atoIcons[13];
static CTextureObject _toPathDot;
static CTextureObject _toMapBcgLD;
static CTextureObject _toMapBcgLU;
static CTextureObject _toMapBcgRD;
static CTextureObject _toMapBcgRU;

PIX aIconCoords[][2] =
{
  {0, 0},      // 00: Last Episode
  {168, 351},  // 01: Palenque 01 
  {42, 345},   // 02: Palenque 02 
  {41, 263},   // 03: Teotihuacan 01      
  {113, 300},  // 04: Teotihuacan 02      
  {334, 328},  // 05: Teotihuacan 03      
  {371, 187},  // 06: Ziggurat	 
  {265, 111},  // 07: Atrium		 
  {119, 172},  // 08: Gilgamesh	 
  {0, 145},    // 09: Babel		   
  {90, 30},    // 10: Citadel		 
  {171, 11},   // 11: Land of Damned		     
  {376, 0},    // 12: Cathedral	 
};

#define LASTEPISODE_BIT 0
#define PALENQUE01_BIT 1
#define PALENQUE02_BIT  2
#define TEOTIHUACAN01_BIT 3
#define TEOTIHUACAN02_BIT 4
#define TEOTIHUACAN03_BIT 5
#define ZIGGURAT_BIT 6
#define ATRIUM_BIT 7
#define GILGAMESH_BIT 8
#define BABEL_BIT 9
#define CITADEL_BIT 10
#define LOD_BIT 11
#define CATHEDRAL_BIT 12

INDEX  aPathPrevNextLevels[][2] = 
{
  {LASTEPISODE_BIT, PALENQUE01_BIT},      // 00
  {PALENQUE01_BIT, PALENQUE02_BIT},       // 01
  {PALENQUE02_BIT, TEOTIHUACAN01_BIT },   // 02
  {TEOTIHUACAN01_BIT, TEOTIHUACAN02_BIT}, // 03
  {TEOTIHUACAN02_BIT, TEOTIHUACAN03_BIT}, // 04
  {TEOTIHUACAN03_BIT, ZIGGURAT_BIT},      // 05
  {ZIGGURAT_BIT, ATRIUM_BIT},             // 06
  {ATRIUM_BIT, GILGAMESH_BIT},            // 07
  {GILGAMESH_BIT, BABEL_BIT},             // 08
  {BABEL_BIT, CITADEL_BIT},               // 09
  {CITADEL_BIT, LOD_BIT},                 // 10
  {LOD_BIT, CATHEDRAL_BIT},               // 11

};

PIX aPathDots[][10][2] = 
{
  // 00: Palenque01 - Palenque02
  {
    {-1,-1},
  },

  // 01: Palenque01 - Palenque02
  {
    {211,440},
    {193,447},
    {175,444},
    {163,434},
    {152,423},
    {139,418},
    {-1,-1},
  },
  
  // 02: Palenque02 - Teotihuacan01
  {
    {100,372},
    {102,363},
    {108,354},
    {113,345},
    {106,338},
    {-1,-1},
  },

  // 03: Teotihuacan01 - Teotihuacan02
  {
    {153,337},
    {166,341},
    {180,346},
    {194,342},
    {207,337},
    {-1,-1},
  },

  // 04: Teotihuacan02 - Teotihuacan03
  {
    {279,339},
    {287,347},
    {296,352},
    {307,365},
    {321,367},
    {335,362},
    {-1,-1},
  },

  // 05: Teotihuacan03 - Ziggurat
  {
    {-1,-1},
  },

  // 06: Ziggurat - Atrium
  {
    {412,285},
    {396,282},
    {383,273},
    {368,266},
    {354,264},
    {-1,-1},
  },

  // 07: Atrium - Gilgamesh
  {
    {276,255},
    {262,258},
    {248,253},
    {235,245},
    {222,240},
    {-1,-1},
  },

  // 08: Gilgamesh - Babel
  {
    {152,245},
    {136,248},
    {118,253},
    {100,251},
    {85,246},
    {69,243},
    {-1,-1},
  },

  // 09: Babel - Citadel
  {
    {-1,-1},
  },

  // 10: Citadel - Lod
  {
    {190,130},
    {204,126},
    {215,119},
    {232,116},
    {241,107},
    {-1,-1},
  },

  // 11: Lod - Cathedral
  {
    {330,108},
    {341,117},
    {353,126},
    {364,136},
    {377,146},
    {395,147},
    {-1,-1},
  },

};

BOOL ObtainMapData(void)
{
  try {
    atoIcons[ 0].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Book.tex"));
    atoIcons[ 1].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level00.tex"));
    atoIcons[ 2].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level01.tex"));
    atoIcons[ 3].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level02.tex"));
    atoIcons[ 4].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level03.tex"));
    atoIcons[ 5].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level04.tex"));
    atoIcons[ 6].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level05.tex"));
    atoIcons[ 7].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level06.tex"));
    atoIcons[ 8].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level07.tex"));
    atoIcons[ 9].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level08.tex"));
    atoIcons[10].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level09.tex"));
    atoIcons[11].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level10.tex"));
    atoIcons[12].SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\Level11.tex"));
    _toPathDot  .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\PathDot.tex"));
    _toMapBcgLD .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\MapBcgLD.tex"));
    _toMapBcgLU .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\MapBcgLU.tex"));
    _toMapBcgRD .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\MapBcgRD.tex"));
    _toMapBcgRU .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\MapBcgRU.tex"));
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

  PIX pixC1S = pixSX;                          // column 1 start pixel
  PIX pixR1S = pixSY;                          // raw 1 start pixel
  PIX pixC1E = (PIX) (pixSX+256*fStretch);     // column 1 end pixel
  PIX pixR1E = (PIX) (pixSY+256*fStretch);     // raw 1 end pixel
  PIX pixC2S = (PIX) (pixC1E-fStretch);        // column 2 start pixel
  PIX pixR2S = (PIX) (pixR1E-fStretch);        // raw 2 start pixel
  PIX pixC2E = (PIX) (pixC2S+256*fStretch);    // column 2 end pixel
  PIX pixR2E = (PIX) (pixR2S+256*fStretch);    // raw 2 end pixel

  if (ulLevelMask == 0x00000001) {

    // render the book
    PIX pixX = (PIX) (aIconCoords[0][0]*fStretch+pixC1S);
    PIX pixY = (PIX) (aIconCoords[0][1]*fStretch+pixR1S);
    CTextureObject *pto = &atoIcons[0];
    // FIXME: DG: or was the line below supposed to use pixX and pixY?
    pdp->PutTexture( pto, PIXaabbox2D( PIX2D(pixC1S,pixR1S), PIX2D(pixC2E,pixR2E)), C_WHITE|255);

  } else {

    // render pale map bcg
    pdp->PutTexture( &_toMapBcgLU, PIXaabbox2D( PIX2D(pixC1S,pixR1S), PIX2D(pixC1E,pixR1E)), C_WHITE|255);
    pdp->PutTexture( &_toMapBcgRU, PIXaabbox2D( PIX2D(pixC2S,pixR1S), PIX2D(pixC2E,pixR1E)), C_WHITE|255);
    pdp->PutTexture( &_toMapBcgLD, PIXaabbox2D( PIX2D(pixC1S,pixR2S), PIX2D(pixC1E,pixR2E)), C_WHITE|255);
    pdp->PutTexture( &_toMapBcgRD, PIXaabbox2D( PIX2D(pixC2S,pixR2S), PIX2D(pixC2E,pixR2E)), C_WHITE|255);
    
    // render icons
    for( INDEX iIcon=1; iIcon<ARRAYCOUNT(aIconCoords); iIcon++)
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
  }

  // render paths
  for( INDEX iPath=0; iPath<ARRAYCOUNT(aPathPrevNextLevels); iPath++)
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
          C_BLACK|255);
      }
    }
  }

  if( pphi != NULL)
  {
    // set font
    pdp->SetFont( _pfdDisplayFont);
    pdp->SetTextScaling( fStretch);
    pdp->SetTextAspect( 1.0f);
    
    INDEX iPosX, iPosY;
    COLOR colText;
    // set coordinates and dot colors
    if (ulLevelMask == 0x00000001) {
      iPosX = 200;
      iPosY = 330;
      colText = 0x5c6a9aff;
    } else {
      iPosX = 395; 
      iPosY = 403;
      colText = 0xc87832ff;
    }

    PIX pixhtcx = (PIX) (pixC1S+iPosX*fStretch);
    PIX pixhtcy = (PIX) (pixR1S+iPosY*fStretch);

    pdp->PutTextC( pphi->phi_strDescription, pixhtcx, pixhtcy, colText);
    for( INDEX iProgresDot=0; iProgresDot<16; iProgresDot+=1)
    {
      PIX pixDotX = (PIX) (pixC1S+((iPosX-68)+iProgresDot*8)*fStretch);
      PIX pixDotY = (PIX) (pixR1S+(iPosY+19)*fStretch);

      COLOR colDot = colText|255;
      if(iProgresDot>pphi->phi_fCompleted*16)
      {
        colDot = C_BLACK|64;
      }
      pdp->PutTexture( &_toPathDot, PIXaabbox2D( PIX2D(pixDotX, pixDotY), 
        PIX2D(pixDotX+2+8*fStretch, pixDotY+2+8*fStretch)), C_BLACK|255);
      pdp->PutTexture( &_toPathDot, PIXaabbox2D( PIX2D(pixDotX, pixDotY), 
        PIX2D(pixDotX+8*fStretch, pixDotY+8*fStretch)), colDot);      
    }
  }

  // free textures used in map rendering
  ReleaseMapData();
}
