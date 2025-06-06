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
#include <Engine/Templates/Stock_CTextureData.h>
 
// global variables used in terrain editing functions
UWORD *_puwBuffer=NULL;
Rect _rect;
PIX _srcExtraW=0;
PIX _srcExtraH=0;
CTextureData *_ptdBrush=NULL;
CTextureData *_ptdDistributionRandomNoise=NULL;
CTextureData *_ptdContinousRandomNoise=NULL;
UWORD *_puwNoiseTarget=NULL;
PIX _pixNoiseTargetW=0;
PIX _pixNoiseTargetH=0;
FLOAT _fStrength=0.0f;

// fbm noise buffer
FLOAT *_pafWhiteNoise=NULL;
#define WNOISE 64

// undo variables
UWORD *_puwUndoTerrain=NULL;
Rect _rectUndo;
BOOL _bUndoStart=FALSE;
BufferType _btUndoBufferType=BT_INVALID;
INDEX _iUndoBufferData=-1;
INDEX _iTerrainEntityID=-1;

// filter matrices
FLOAT _afFilterFineBlur[5][5]=
{
  {0.0f,       0.0f,       0.0f,       0.0f,    0.0f},
  {0.0f,    0.0625f,    0.0625f,    0.0625f,    0.0f},
  {0.0f,    0.0625f,       0.5f,    0.0625f,    0.0f},
  {0.0f,    0.0625f,    0.0625f,    0.0625f,    0.0f},
  {0.0f,       0.0f,       0.0f,       0.0f,    0.0f},
};

FLOAT _afFilterBlurMore[5][5]=
{
  {0.0277777f,    0.0277777f,    0.0277777f,    0.0277777f,    0.0277777f},
  {0.0277777f,    0.0555555f,    0.0555555f,    0.0555555f,    0.0277777f},
  {0.0277777f,    0.0555555f,    0.0111111f,    0.0555555f,    0.0277777f},
  {0.0277777f,    0.0555555f,    0.0555555f,    0.0555555f,    0.0277777f},
  {0.0277777f,    0.0277777f,    0.0277777f,    0.0277777f,    0.0277777f},
};

FLOAT _afFilterEdgeDetect[5][5]=
{
  {0.0f,    0.0f,    0.0f,    0.0f,    0.0f},
  {0.0f,    1.0f,    1.0f,    1.0f,    0.0f},
  {0.0f,    1.0f,   -7.0f,    1.0f,    0.0f},
  {0.0f,    1.0f,    1.0f,    1.0f,    0.0f},
  {0.0f,    0.0f,    0.0f,    0.0f,    0.0f},
};

FLOAT _afFilterEmboss[5][5]=
{
  {0.0f,    0.0f,    0.0f,    0.0f,    0.0f},
  {0.0f,    1.0f,    1.0f,   -1.0f,    0.0f},
  {0.0f,    1.0f,    1.0f,   -1.0f,    0.0f},
  {0.0f,    1.0f,   -1.0f,   -1.0f,    0.0f},
  {0.0f,    0.0f,    0.0f,    0.0f,    0.0f},
};

FLOAT _afFilterSharpen[5][5]=
{
  {0.0f,    0.0f,    0.0f,    0.0f,    0.0f},
  {0.0f,   -1.0f,   -1.0f,   -1.0f,    0.0f},
  {0.0f,   -1.0f,   16.0f,   -1.0f,    0.0f},
  {0.0f,   -1.0f,   -1.0f,   -1.0f,    0.0f},
  {0.0f,    0.0f,    0.0f,    0.0f,    0.0f},
};

FLOAT GetBrushMultiplier(INDEX x, INDEX y)
{
  if(_ptdBrush==NULL) return 1.0f;
  {
    COLOR col=_ptdBrush->GetTexel(x,y);
    FLOAT fResult=FLOAT(col>>24)/255.0f;
    return fResult;
  }
}

void ApplyAddPaint(UWORD uwMin, UWORD uwMax)
{
  for(INDEX y=0; y<_rect.Height(); y++)
  {
    for(INDEX x=0; x<_rect.Width(); x++)
    {
      FLOAT fBrushMultiplier=GetBrushMultiplier(x,y);
      if( fBrushMultiplier==0.0f) continue;
      INDEX iOffset=y*_rect.Width()+x;
      FLOAT fValue=_puwBuffer[iOffset];
      fValue+=fBrushMultiplier*_fStrength*32.0f*theApp.m_fPaintPower;
      fValue=Clamp(fValue,FLOAT(uwMin),FLOAT(uwMax));
      _puwBuffer[iOffset]=fValue;
    }
  }
}

void ApplyRNDNoise(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;
  FLOAT fMaxNoise=theApp.m_fNoiseAltitude/ptrTerrain->tr_vTerrainSize(2)*65535.0f;
  for(INDEX y=0; y<_rect.Height(); y++)
  {
    for(INDEX x=0; x<_rect.Width(); x++)
    {
      FLOAT fBrushMultiplier=GetBrushMultiplier(x,y);
      INDEX iPixSrc=y*_rect.Width()+x;
      FLOAT fInfluence=_puwBuffer[iPixSrc];
      FLOAT fRnd=FLOAT(rand())/(float)(RAND_MAX)-0.5f;
      FLOAT fValue=_puwBuffer[iPixSrc];
      FLOAT fMaxRandomized=fValue+fRnd*fMaxNoise;
      FLOAT fFilterPower=Clamp(fBrushMultiplier*_fStrength,0.0f,1.0f);
      FLOAT fResult=Lerp( fValue, fMaxRandomized, fFilterPower);
      UWORD uwResult=Clamp(UWORD(fResult),MIN_UWORD,MAX_UWORD);
      _puwBuffer[iPixSrc]=uwResult;
    }
  }
}

FLOAT GetDistributionNoise( INDEX x, INDEX y, FLOAT fRandom)
{
  INDEX iw=_ptdDistributionRandomNoise->GetPixWidth();
  INDEX ih=_ptdDistributionRandomNoise->GetPixHeight();
  INDEX ctSize=iw*ih;
  INDEX iOffset=abs(INDEX(iw*y+x+fRandom*ctSize)%ctSize);
  COLOR col=_ptdDistributionRandomNoise->td_pulFrames[iOffset];
  FLOAT fResult=FLOAT(col&0xFF)/255.0f;
  return fResult;
}

FLOAT GetContinousNoise( INDEX x, INDEX y, FLOAT fRandom)
{
  INDEX iw=_ptdContinousRandomNoise->GetPixWidth();
  INDEX ih=_ptdContinousRandomNoise->GetPixHeight();
  INDEX ctSize=iw*ih;
  INDEX iOffset=abs(INDEX(iw*y+x+fRandom*ctSize)%ctSize);
  COLOR col=_ptdContinousRandomNoise->td_pulFrames[iOffset];
  FLOAT fResult=FLOAT(col&0xFF)/255.0f;
  return fResult;
}

void ApplyContinousNoise(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;
  FLOAT fMaxNoise=theApp.m_fNoiseAltitude/ptrTerrain->tr_vTerrainSize(2)*65535.0f;
  for(INDEX y=0; y<_rect.Height(); y++)
  {
    INDEX oy=y+_rect.rc_iTop;
    for(INDEX x=0; x<_rect.Width(); x++)
    {
      INDEX ox=x+_rect.rc_iLeft;
      FLOAT fBrushMultiplier=GetBrushMultiplier(x,y);
      INDEX iPixSrc=y*_rect.Width()+x;
      FLOAT fInfluence=_puwBuffer[iPixSrc];
      FLOAT fRnd=GetContinousNoise( ox, oy, 0.0f)-0.5f;
      FLOAT fValue=_puwBuffer[iPixSrc];
      FLOAT fMaxRandomized=fValue+fRnd*fMaxNoise;
      fMaxRandomized=Clamp(fMaxRandomized,(FLOAT)MIN_UWORD,(FLOAT)MAX_UWORD);
      FLOAT fFilterPower=Clamp(fBrushMultiplier*_fStrength,0.0f,1.0f);
      FLOAT fResult=Lerp( fValue, fMaxRandomized, fFilterPower);
      UWORD uwResult=Clamp((UWORD)fResult,MIN_UWORD,MAX_UWORD);
      _puwBuffer[iPixSrc]=uwResult;
    }
  }
}

void ApplyPosterize(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;
  FLOAT fStepUW=theApp.m_fPosterizeStep/ptrTerrain->tr_vTerrainSize(2)*65535.0f;
  for(INDEX y=0; y<_rect.Height(); y++)
  {
    for(INDEX x=0; x<_rect.Width(); x++)
    {
      FLOAT fBrushMultiplier=GetBrushMultiplier(x,y);
      if(fBrushMultiplier==0.0f) continue;
      INDEX iPixSrc=y*_rect.Width()+x;
      FLOAT fValue=_puwBuffer[iPixSrc];
      FLOAT fPosterized=(INDEX(fValue/fStepUW))*fStepUW+1.0f;
      UWORD uwResult=Clamp(UWORD(fPosterized),MIN_UWORD,MAX_UWORD);
      _puwBuffer[iPixSrc]=uwResult;
    }
  }
}

void ApplyFilterMatrix(FLOAT afFilterMatrix[5][5])
{
  INDEX ctBuffBytes=_rect.Width()*_rect.Height()*sizeof(UWORD);
  UWORD *puwDst=(UWORD*)AllocMemory(ctBuffBytes);
  memcpy(puwDst,_puwBuffer,ctBuffBytes);
  for(INDEX y=0; y<_rect.Height()-_srcExtraH*2; y++)
  {
    for(INDEX x=0; x<_rect.Width()-_srcExtraW*2; x++)
    {
      INDEX iPixDst=(y+_srcExtraH)*_rect.Width()+x+_srcExtraW;
      FLOAT fBrushMultiplier=GetBrushMultiplier(x,y);

      FLOAT fDivSum=0.0f;
      FLOAT fSum=0.0f;
      for(INDEX j=0; j<5; j++)
      {
        for(INDEX i=0; i<5; i++)
        {
          FLOAT fWeight=(afFilterMatrix)[i][j];
          fDivSum+=fWeight;
          INDEX iPixSrc=(y+j)*_rect.Width()+(x+i);
          FLOAT fInfluence=_puwBuffer[iPixSrc];
          fSum+=fInfluence*fWeight;
        }
      }
      UWORD uwMax=Clamp(UWORD(fSum/fDivSum),MIN_UWORD,MAX_UWORD);
      FLOAT fFilterPower=Clamp(fBrushMultiplier*_fStrength/64.0f,0.0f,1.0f);
      UWORD uwResult=Lerp( puwDst[iPixDst], uwMax, fFilterPower);
      puwDst[iPixDst]=uwResult;
    }
  }
  memcpy(_puwBuffer,puwDst,ctBuffBytes);
  FreeMemory( puwDst);
}

static INDEX _iTerrainWidth=0;
static UWORD *_puwHeightMap=NULL;
static INDEX _iRandomDX=0;

void SetHMPixel( UWORD pix, INDEX x, INDEX y)
{
  UWORD *pdest=_puwHeightMap+y*_iTerrainWidth+x;
  if (*pdest==65535) {
    *pdest=pix;
  }
}

UWORD GetHMPixel(INDEX x, INDEX y)
{
  UWORD *pdest=_puwHeightMap+y*_iTerrainWidth+x;
  return *pdest;
}

UWORD RandomizePixel(FLOAT fmid, FLOAT fdmax)
{
  FLOAT fRand=((FLOAT)rand())/(float)(RAND_MAX)-0.5f;
  FLOAT fd=fdmax*fRand;
  FLOAT fres=Clamp(fmid+fd,0.0f,65536.0f);
  return fres;
}

void SubdivideAndDisplace(INDEX x, INDEX y, INDEX idx, FLOAT fdMax)
{
  FLOAT flu=GetHMPixel(x,y);
  FLOAT fru=GetHMPixel(x+idx,y);
  FLOAT frd=GetHMPixel(x+idx,y+idx);
  FLOAT fld=GetHMPixel(x,y+idx);

  if( fdMax<_iRandomDX)
  {
    SetHMPixel(RandomizePixel((flu+fru)/2.0f,fdMax),  x+idx/2, y      );  // middle top
    SetHMPixel(RandomizePixel((fld+frd)/2.0f,fdMax),  x+idx/2, y+idx  );  // middle bottom
    SetHMPixel(RandomizePixel((fru+frd)/2.0f,fdMax),  x+idx,   y+idx/2);  // right  middle 
    SetHMPixel(RandomizePixel((flu+fld)/2.0f,fdMax),  x,       y+idx/2);  // left   middle 
  
    SetHMPixel(RandomizePixel((flu+fru+fld+frd)/4.0f,fdMax),  x+idx/2, y+idx/2);  // middle
  }
  else
  {
    SetHMPixel(RandomizePixel(65536.0f/2.0f,fdMax),  x+idx/2, y      );  // middle top
    SetHMPixel(RandomizePixel(65536.0f/2.0f,fdMax),  x+idx/2, y+idx  );  // middle bottom
    SetHMPixel(RandomizePixel(65536.0f/2.0f,fdMax),  x+idx,   y+idx/2);  // right  middle 
    SetHMPixel(RandomizePixel(65536.0f/2.0f,fdMax),  x,       y+idx/2);  // left   middle 
  
    SetHMPixel(RandomizePixel(65536.0f/2.0f,fdMax),  x+idx/2, y+idx/2);  // middle
  }
  
  fdMax*=0.5f;
  if(idx>1)
  {
    SubdivideAndDisplace(x      ,y      , idx/2, fdMax);
    SubdivideAndDisplace(x+idx/2,y      , idx/2, fdMax);
    SubdivideAndDisplace(x      ,y+idx/2, idx/2, fdMax);
    SubdivideAndDisplace(x+idx/2,y+idx/2, idx/2, fdMax);
  }
}

Rect GetTerrainRect(void)
{
  Rect rect;
  rect.rc_iLeft=0;
  rect.rc_iRight=0;
  rect.rc_iTop=0;
  rect.rc_iBottom=0;

  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return rect;

  rect.rc_iLeft=0;
  rect.rc_iRight=ptrTerrain->tr_pixHeightMapWidth;
  rect.rc_iTop=0;
  rect.rc_iBottom=ptrTerrain->tr_pixHeightMapHeight;
  return rect;
}

FLOAT GetWrappedPixelValue( INDEX x, INDEX y)
{
  INDEX iWrapX=(x+WNOISE)%WNOISE;
  INDEX iWrapY=(y+WNOISE)%WNOISE;
  return _pafWhiteNoise[iWrapY*WNOISE+iWrapX];
}

void RandomizeWhiteNoise(void)
{
  if(_pafWhiteNoise==NULL)
  {
    _pafWhiteNoise=(FLOAT *)AllocMemory(WNOISE*WNOISE*sizeof(FLOAT));
  }

  FLOAT *pfTemp=_pafWhiteNoise;
  for(INDEX i=0; i<WNOISE*WNOISE; i++)
  {
    FLOAT fRnd=FLOAT(rand())/(float)(RAND_MAX)-0.5f;
    *pfTemp=fRnd;
    pfTemp++;
  }    
}

FLOAT *GenerateTerrain_FBMBuffer(PIX pixW, PIX pixH, INDEX ctOctaves, FLOAT fHighFrequencyStep,
                                 FLOAT fStepFactor, FLOAT fMaxAmplitude, FLOAT fAmplitudeDecreaser,
                                 BOOL bAddNegativeValues, BOOL bRandomOffest, FLOAT &fMin, FLOAT &fMax)
{
  if(_pafWhiteNoise==NULL)
  {
    RandomizeWhiteNoise();
  }
  FLOAT *pfTemp=_pafWhiteNoise;

  FLOAT fTmpMaxAmplitude=fMaxAmplitude;
  INDEX ctMemory=pixW*pixH*sizeof(FLOAT);
  FLOAT *pafFBM=(FLOAT *)AllocMemory(ctMemory);
  memset(pafFBM,0,ctMemory);

  FLOAT fPixStep=fHighFrequencyStep/pow(fStepFactor,ctOctaves);
  fMin=1e6;
  fMax=-1e6;
  for(INDEX iOctave=ctOctaves-1; iOctave>=0; iOctave--)
  {
    FLOAT fOctaveOffset=0.0f;
    if( bRandomOffest)
    {
      fOctaveOffset=_pafWhiteNoise[iOctave];
    }
    for(INDEX y=0; y<pixH; y++)
    {
      for(INDEX x=0; x<pixW; x++)
      {
        FLOAT fY=y*fPixStep+fOctaveOffset;
        FLOAT fX=x*fPixStep+fOctaveOffset;
        // calculate bilinear value
        FLOAT fLU=GetWrappedPixelValue( fX  , fY);
        FLOAT fRU=GetWrappedPixelValue( fX+1, fY);
        FLOAT fLD=GetWrappedPixelValue( fX  , fY+1);
        FLOAT fRD=GetWrappedPixelValue( fX+1, fY+1);
        FLOAT fFX=fX-INDEX(fX);
        FLOAT fFY=fY-INDEX(fY);
        FLOAT fBil=Lerp(Lerp(fLU,fRU,fFX),Lerp(fLD,fRD,fFX),fFY);
        INDEX iOffset=pixW*y+x;
        FLOAT fValue=pafFBM[iOffset];
        FLOAT fAdd=fBil*fTmpMaxAmplitude;
        if(bAddNegativeValues || fAdd>0)
        {
          fValue=fValue+fAdd;
        }
        pafFBM[iOffset]=fValue;
        if(fValue>fMax) fMax=fValue;
        if(fValue<fMin) fMin=fValue;
      }
    }
    fPixStep*=fStepFactor;
    fTmpMaxAmplitude*=fAmplitudeDecreaser;
  }
  return pafFBM;
}

void GenerateTerrain_SubdivideAndDisplace(void)
{
  // inside subdivide and displace functions we will use these global variables
  _iTerrainWidth=_rect.Width();
  _puwHeightMap=_puwBuffer;
  UWORD uwScrollValue=8.0f-Clamp(theApp.m_iRNDSubdivideAndDisplaceItterations, INDEX(0), INDEX(8));
  _iRandomDX=(_iTerrainWidth-1)<<uwScrollValue;
  
  UWORD uwrnd;
  FLOAT fdMax=65536.0f;
  for (INDEX i=0; i<_rect.Width()*_rect.Height(); i++) {
    _puwHeightMap[i] = 65535;
  }
  uwrnd=RandomizePixel(fdMax/2.0f,fdMax); SetHMPixel(uwrnd,                0,                0);
  uwrnd=RandomizePixel(fdMax/2.0f,fdMax); SetHMPixel(uwrnd, _iTerrainWidth-1,                0);
  uwrnd=RandomizePixel(fdMax/2.0f,fdMax); SetHMPixel(uwrnd, _iTerrainWidth-1, _iTerrainWidth-1);
  uwrnd=RandomizePixel(fdMax/2.0f,fdMax); SetHMPixel(uwrnd,                0, _iTerrainWidth-1);
  // generate rest of terrain pixels using recursion
  SubdivideAndDisplace(0,0,_iTerrainWidth-1,fdMax/2.0f);
}

void GenerateTerrain(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;

  switch(theApp.m_iTerrainGenerationMethod)
  {
    case 0:
    {
      GenerateTerrain_SubdivideAndDisplace();
      break;
    }
    case 1:
    {
      FLOAT fMin, fMax;
      PIX pixTerrainW=ptrTerrain->tr_pixHeightMapWidth;
      PIX pixTerrainH=ptrTerrain->tr_pixHeightMapHeight;
      FLOAT *pafFBM=GenerateTerrain_FBMBuffer( pixTerrainW, pixTerrainH, theApp.m_iFBMOctaves,
        theApp.m_fFBMHighFrequencyStep,theApp.m_fFBMStepFactor, theApp.m_fFBMMaxAmplitude,
        theApp.m_fFBMfAmplitudeDecreaser, theApp.m_bFBMAddNegativeValues, theApp.m_bFBMRandomOffset, fMin, fMax);
      // convert buffer to height map
      FLOAT fSizeY=ptrTerrain->tr_vTerrainSize(2);
      FLOAT fConvertFactor=(theApp.m_fFBMMaxAmplitude/fSizeY)*MAX_UWORD;
      // set height map
      for(INDEX iPix=0; iPix<pixTerrainW*pixTerrainH; iPix++)
      {
        FLOAT fValue=pafFBM[iPix];
        UWORD uwValue=UWORD(Clamp((fValue-fMin)/(fMax-fMin)*fConvertFactor,0.0f,65535.0f));
        _puwBuffer[iPix]=uwValue;
      }
  
      FreeMemory( pafFBM);
      break;
    }
  }
}

void EqualizeBuffer(void)
{
  UWORD uwHeightMax=0;
  UWORD uwHeightMin=MAX_UWORD;
  INDEX x,y;

  for(y=0; y<_rect.Height(); y++)
  {
    for(x=0; x<_rect.Width(); x++)
    {
      INDEX iOffset = y*_rect.Width()+x;
      UWORD uwHeight = _puwBuffer[iOffset];
      if( uwHeight>uwHeightMax) uwHeightMax=uwHeight;
      if( uwHeight<uwHeightMin) uwHeightMin=uwHeight;
    }
  }
  
  FLOAT fFactor=65535.0f/(uwHeightMax-uwHeightMin);
  // equalize (normalize from 0 to 65535)
  for(y=0; y<_rect.Height(); y++)
  {
    for(x=0; x<_rect.Width(); x++)
    {
      INDEX iOffset = y*_rect.Width()+x;
      UWORD uwHeight = _puwBuffer[iOffset];
      FLOAT fNormalized=(uwHeight-uwHeightMin)*fFactor;
      _puwBuffer[iOffset]=fNormalized;
    }
  }
}

BOOL SetupContinousNoiseTexture( void)
{
  try
  {
    _ptdContinousRandomNoise=_pTextureStock->Obtain_t( theApp.m_fnContinousNoiseTexture);
    _ptdContinousRandomNoise->Force(TEX_STATIC|TEX_CONSTANT);
  }
  catch (const char *strError)
  {
    (void) strError;
    WarningMessage("Unable to obtain continous random noise texture!\nError: %s", strError);
    return FALSE;
  }
  return TRUE;
}

void FreeContinousNoiseTexture( void)
{
  _pTextureStock->Release( _ptdContinousRandomNoise);
}

BOOL SetupDistributionNoiseTexture( void)
{
  try
  {
    _ptdDistributionRandomNoise=_pTextureStock->Obtain_t( theApp.m_fnDistributionNoiseTexture);
    _ptdDistributionRandomNoise->Force(TEX_STATIC|TEX_CONSTANT);
  }
  catch (const char *strError)
  {
    (void) strError;
    WarningMessage("Unable to obtain distribution random noise texture!\nError: %s", strError);
    return FALSE;
  }
  return TRUE;
}

void FreeDistributionNoiseTexture( void)
{
  _pTextureStock->Release( _ptdDistributionRandomNoise);
}

FLOAT StepUp(FLOAT fCur, FLOAT fMin, FLOAT fMax)
{
  if( fCur<=fMin) return 0.0f;
  if( fCur>=fMax) return 1.0f;
  return (fCur-fMin)/(fMax-fMin);
}

FLOAT StepDown(FLOAT fCur, FLOAT fMin, FLOAT fMax)
{
  if( fCur<=fMin) return 1.0f;
  if( fCur>=fMax) return 0.0f;
  return (fMax-fCur)/(fMax-fMin);
}

FLOAT3D NormalFrom4Points(const FLOAT3D &v0, const FLOAT3D &v1, const FLOAT3D &v2, const FLOAT3D &v3,
                          FLOAT fLerpX, FLOAT fLerpZ)
{
  FLOAT fHDeltaX = Lerp(v1(2)-v0(2), v3(2)-v2(2), fLerpZ);
  FLOAT fHDeltaZ = Lerp(v0(2)-v2(2), v1(2)-v3(2), fLerpX);
  FLOAT fDeltaX  = v1(1) - v0(1);
  FLOAT fDeltaZ  = v0(3) - v2(3);

  FLOAT3D vNormal;
  vNormal(2) = sqrt(1 / (((fHDeltaX*fHDeltaX)/(fDeltaX*fDeltaX)) + ((fHDeltaZ*fHDeltaZ)/(fDeltaZ*fDeltaZ)) + 1));
  vNormal(1) = sqrt(vNormal(2)*vNormal(2) * ((fHDeltaX*fHDeltaX) / (fDeltaX*fDeltaX)));
  vNormal(3) = sqrt(vNormal(2)*vNormal(2) * ((fHDeltaZ*fHDeltaZ) / (fDeltaZ*fDeltaZ)));
  if (fHDeltaX>0) {
    vNormal(1) = -vNormal(1);
  }
  if (fHDeltaZ<0) {
    vNormal(3) = -vNormal(3);
  }
  //ASSERT(Abs(vNormal.Length()-1)<0.01);
  return vNormal;
}

FLOAT3D GetPoint(CTerrain *ptrTerrain, INDEX iX, INDEX iY)
{
  const FLOAT3D &vStretch = ptrTerrain->tr_vStretch;
  iX = Clamp(iX, INDEX(0), ptrTerrain->tr_pixHeightMapWidth);
  iY = Clamp(iY, INDEX(0), ptrTerrain->tr_pixHeightMapHeight);

  return FLOAT3D(
    FLOAT(iX)*vStretch(1), 
    (FLOAT)ptrTerrain->tr_auwHeightMap[iX+iY*ptrTerrain->tr_pixHeightMapWidth] * vStretch(2),
    FLOAT(iY)*vStretch(3));
}

UWORD GetSlope(CTerrain *ptrTerrain, INDEX iX, INDEX iY)
{
  FLOAT3D av[9];
  INDEX iHMapWidth = ptrTerrain->tr_pixHeightMapWidth;
  FLOAT3D vStretch = ptrTerrain->tr_vStretch;

  av[0] = GetPoint(ptrTerrain, iX-1, iY-1);
  av[1] = GetPoint(ptrTerrain, iX  , iY-1);
  av[2] = GetPoint(ptrTerrain, iX+1, iY-1);
  av[3] = GetPoint(ptrTerrain, iX-1, iY  );
  av[4] = GetPoint(ptrTerrain, iX  , iY  );
  av[5] = GetPoint(ptrTerrain, iX+1, iY  );
  av[6] = GetPoint(ptrTerrain, iX-1, iY+1);
  av[7] = GetPoint(ptrTerrain, iX  , iY+1);
  av[8] = GetPoint(ptrTerrain, iX+1, iY+1);

  FLOAT3D avN[4];
  FLOAT3D vNormal;
  avN[0] = NormalFrom4Points(av[0], av[1], av[3], av[4], 1, 1);
  avN[1] = NormalFrom4Points(av[1], av[2], av[4], av[5], 0, 1);
  avN[2] = NormalFrom4Points(av[3], av[4], av[6], av[7], 1, 0);
  avN[3] = NormalFrom4Points(av[4], av[5], av[7], av[8], 0, 0);

  vNormal = avN[0]+avN[1]+avN[2]+avN[3];
  vNormal.Normalize();

  return (1-vNormal(2))*65535;
}

void GenerateLayerDistribution(INDEX iForLayer, Rect rect)
{
  if(!SetupDistributionNoiseTexture()) return;

  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;

  // obtain buffer
  UWORD *puwAltitude=GetBufferForEditing(ptrTerrain, rect, BT_HEIGHT_MAP, 0);
  INDEX ctSize=rect.Width()*rect.Height()*sizeof(UWORD);
  UWORD *puwSlope=(UWORD *)AllocMemory(ctSize);

  // prepare slope buffer
  for(INDEX y=0; y<rect.Height(); y++)
  {
    for(INDEX x=0; x<rect.Width(); x++)
    {
      INDEX iOffset = y*rect.Width()+x;
      puwSlope[iOffset] = GetSlope(ptrTerrain, x+rect.rc_iLeft, y+rect.rc_iTop);
    }
  }
  
  // for each layer
  for(INDEX iLayer=0; iLayer<ptrTerrain->tr_atlLayers.Count(); iLayer++)
  {
    if( iForLayer!=-1 && iLayer!=iForLayer) continue;
    CTerrainLayer *ptlLayer=GetLayer(iLayer);
    if(!ptlLayer->tl_bAutoRegenerated) continue;
    // get layer
    UWORD *puwMask=GetBufferForEditing(ptrTerrain, rect, BT_LAYER_MASK, iLayer);

    for(INDEX y=0; y<rect.Height(); y++)
    {
      INDEX oy=y+rect.rc_iTop;
      for(INDEX x=0; x<rect.Width(); x++)
      {
        INDEX ox=x+rect.rc_iLeft;
        INDEX iOffset = y*rect.Width()+x;
        FLOAT fAltitudeRatio = puwAltitude[iOffset]/65535.0f;
        FLOAT fSlopeRatio = puwSlope[iOffset]/65535.0f;
        FLOAT fAltitudeRange=ptlLayer->tl_fMaxAltitude-ptlLayer->tl_fMinAltitude;
        FLOAT fSlopeRange=ptlLayer->tl_fMaxSlope-ptlLayer->tl_fMinSlope;

        // get coverage influence
        FLOAT fCoverageInfluence=1.0f;
        FLOAT fCoverageRandom=GetDistributionNoise( ox, oy, ptlLayer->tl_fCoverageRandom);
        fCoverageInfluence=StepDown(fCoverageRandom, ptlLayer->tl_fCoverage, ptlLayer->tl_fCoverage+ptlLayer->tl_fCoverageNoise);

        // get min altitude influence
        FLOAT fMinAltitudeInfluence=1.0f;
        if(ptlLayer->tl_bApplyMinAltitude)
        {
          FLOAT fMinAltitudeNoise=(GetDistributionNoise( ox, oy, ptlLayer->tl_fMinAltitudeRandom)-0.5f)*ptlLayer->tl_fMinAltitudeNoise;
          FLOAT fAltMinFade1=ptlLayer->tl_fMinAltitude+fAltitudeRange*ptlLayer->tl_fMinAltitudeFade;
          fMinAltitudeInfluence=StepUp(fAltitudeRatio+fMinAltitudeNoise, ptlLayer->tl_fMinAltitude, fAltMinFade1);
        }

        // get max altitude influence
        FLOAT fMaxAltitudeInfluence=1.0f;
        if(ptlLayer->tl_bApplyMaxAltitude)
        {
          FLOAT fMaxAltitudeNoise=(GetDistributionNoise( ox, oy, ptlLayer->tl_fMaxAltitudeRandom)-0.5f)*ptlLayer->tl_fMaxAltitudeNoise;
          FLOAT fAltMaxFade1=ptlLayer->tl_fMaxAltitude-fAltitudeRange*ptlLayer->tl_fMaxAltitudeFade;
          fMaxAltitudeInfluence=StepDown(fAltitudeRatio+fMaxAltitudeNoise, fAltMaxFade1, ptlLayer->tl_fMaxAltitude);
        }

        // get min slope influence
        FLOAT fMinSlopeInfluence=1.0f;
        if(ptlLayer->tl_bApplyMinSlope)
        {
          FLOAT fMinSlopeNoise=(GetDistributionNoise( ox, oy, ptlLayer->tl_fMinSlopeRandom)-0.5f)*ptlLayer->tl_fMinSlopeNoise;
          FLOAT fSlopeMinFade1=ptlLayer->tl_fMinSlope+fSlopeRange*ptlLayer->tl_fMinSlopeFade;
          fMinSlopeInfluence=StepUp(fSlopeRatio+fMinSlopeNoise, ptlLayer->tl_fMinSlope, fSlopeMinFade1);
        }

        // get max slope influence
        FLOAT fMaxSlopeInfluence=1.0f;
        if(ptlLayer->tl_bApplyMaxSlope)
        {
          FLOAT fMaxSlopeNoise=(GetDistributionNoise( ox, oy, ptlLayer->tl_fMaxSlopeRandom)-0.5f)*ptlLayer->tl_fMaxSlopeNoise;
          FLOAT fSlopeMaxFade1=ptlLayer->tl_fMaxSlope-fSlopeRange*ptlLayer->tl_fMaxSlopeFade;
          fMaxSlopeInfluence=StepDown(fSlopeRatio+fMaxSlopeNoise, fSlopeMaxFade1, ptlLayer->tl_fMaxSlope);
        }

        // calculate result of all influences
        puwMask[iOffset]= 65535.0f*
          fCoverageInfluence*
          fMinAltitudeInfluence*
          fMaxAltitudeInfluence*
          fMinSlopeInfluence*
          fMaxSlopeInfluence;
      }
    }
    // apply buffer change
    SetBufferForEditing(ptrTerrain, puwMask, rect, BT_LAYER_MASK, iLayer);
    theApp.GetActiveDocument()->SetModifiedFlag( TRUE);
    FreeMemory(puwMask);
  }
  FreeMemory(puwAltitude);
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  FreeDistributionNoiseTexture();
}

void GenerateLayerDistribution(INDEX iForLayer)
{
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;

  Rect rect;
  rect.rc_iLeft=0;
  rect.rc_iRight=ptrTerrain->tr_pixHeightMapWidth;
  rect.rc_iTop=0;
  rect.rc_iBottom=ptrTerrain->tr_pixHeightMapHeight;
  
  GenerateLayerDistribution(iForLayer, rect);
}
          
void RecalculateShadows(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;
  ptrTerrain->UpdateShadowMap();
}

void OptimizeLayers(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;

  Rect rect;
  rect.rc_iLeft=0;
  rect.rc_iRight=ptrTerrain->tr_pixHeightMapWidth;
  rect.rc_iTop=0;
  rect.rc_iBottom=ptrTerrain->tr_pixHeightMapHeight;

  CStaticArray<UWORD*> apuwLayers;
  // obtain buffer
  INDEX ctLayers=ptrTerrain->tr_atlLayers.Count();
  apuwLayers.New(ctLayers);

  INDEX iLayer, iOffset;
  for( iLayer=0; iLayer<ctLayers; iLayer++)
  {
    UWORD *puw=GetBufferForEditing(ptrTerrain, rect, BT_LAYER_MASK, iLayer);
    apuwLayers[iLayer]=puw;
  }

  // count overdraw before optimisation
  INDEX ctDrawnBefore=0;
  INDEX ctPixels=rect.Width()*rect.Height();
  for(iOffset=0; iOffset<ctPixels; iOffset++)
  {
    for(INDEX i=0; i<ctLayers; i++)
    {
      UWORD *puwCurr=apuwLayers[i]+iOffset;
      if( (*puwCurr)>>8 != 0) ctDrawnBefore++;
    }
  }

  // optimize for overdraw
  for(iOffset=0; iOffset<ctPixels; iOffset++)
  {
    BOOL bOptimize=FALSE;
    for(INDEX i=ctLayers-1; i>=0; i--)
    {
      UWORD *puwCurr=apuwLayers[i]+iOffset;
      // if should optimize
      if(bOptimize)
      {
        // clear mask
        *puwCurr=0;
      }
      else
      {
        if( (*puwCurr)>>8 == 255)
        {
          // clear mask for all layers beneath current one
          bOptimize=TRUE;
        }
      }
    }
  }

  // count overdraw after optimisation
  INDEX ctDrawnAfter=0;
  for(iOffset=0; iOffset<ctPixels; iOffset++)
  {
    for(INDEX i=0; i<ctLayers; i++)
    {
      UWORD *puwCurr=apuwLayers[i]+iOffset;
      if( (*puwCurr)>>8 != 0) ctDrawnAfter++;
    }
  }

  // make a report about optimisation success
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CTString strReport;
  strReport.PrintF("Overdraw before optimization: %g. Overdraw after optimization: %g",
    FLOAT(ctDrawnBefore)/ctPixels, FLOAT(ctDrawnAfter)/ctPixels);
  pMainFrame->SetStatusBarMessage( strReport, STATUS_LINE_PANE, 5.0f);

  // free buffers
  for( iLayer=0; iLayer<ctLayers; iLayer++)
  {
    SetBufferForEditing(ptrTerrain, apuwLayers[iLayer], rect, BT_LAYER_MASK, iLayer);
    FreeMemory(apuwLayers[iLayer]);
  }
  theApp.GetActiveDocument()->SetModifiedFlag( TRUE);
  theApp.m_ctTerrainPageCanvas.MarkChanged();
}

BOOL _bIsUpToDate=TRUE;
Rect _rectDiscarded;
void DiscardLayerDistribution(Rect rect)
{
  if(_bIsUpToDate)
  {
    _rectDiscarded.rc_iLeft=rect.rc_iLeft;    
    _rectDiscarded.rc_iRight=rect.rc_iRight;      
    _rectDiscarded.rc_iTop=rect.rc_iTop;          
    _rectDiscarded.rc_iBottom=rect.rc_iBottom;
  }
  else
  {
    if(rect.rc_iLeft   < _rectDiscarded.rc_iLeft)    _rectDiscarded.rc_iLeft=rect.rc_iLeft;
    if(rect.rc_iRight  > _rectDiscarded.rc_iRight)   _rectDiscarded.rc_iRight=rect.rc_iRight;
    if(rect.rc_iTop    < _rectDiscarded.rc_iTop)     _rectDiscarded.rc_iTop=rect.rc_iTop;
    if(rect.rc_iBottom > _rectDiscarded.rc_iBottom)  _rectDiscarded.rc_iBottom=rect.rc_iBottom;
  }
  _bIsUpToDate=FALSE;
}

void UpdateLayerDistribution(void)
{
  if(_bIsUpToDate || !theApp.m_Preferences.ap_bAutoUpdateTerrainDistribution) return;
  // update layer distribution
  GenerateLayerDistribution(-1, _rectDiscarded);
  _bIsUpToDate=TRUE;
}

void ApplyFilterOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), theApp.m_fFilterPower*16.0f, TE_ALTITUDE_FILTER);
}

void ApplySmoothOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), theApp.m_fSmoothPower*16.0f, TE_ALTITUDE_SMOOTH);
}

void ApplyEqualizeOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), 1.0f, TE_ALTITUDE_EQUALIZE);
}

void ApplyGenerateTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), 1.0f, TE_GENERATE_TERRAIN);
}

void ApplyRndNoiseOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), theApp.m_fNoiseAltitude, TE_ALTITUDE_RND_NOISE);
}

void ApplyContinousNoiseOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), theApp.m_fNoiseAltitude, TE_ALTITUDE_CONTINOUS_NOISE);
}

void ApplyMinimumOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), 1.0f, TE_ALTITUDE_MINIMUM);
}

void ApplyMaximumOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), 1.0f, TE_ALTITUDE_MAXIMUM);
}

void ApplyFlattenOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), 1.0f, TE_ALTITUDE_FLATTEN);
}

void ApplyPosterizeOntoTerrain(void)
{
  EditTerrain(NULL, FLOAT3D(0,0,0), theApp.m_fPosterizeStep, TE_ALTITUDE_POSTERIZE);
}

CEntity *GetEntityForID(ULONG iEntityID)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    if(iten->en_ulID==iEntityID) return &*iten;
  }
  return NULL;
}

// constructor
CTerrainUndo::CTerrainUndo()
{
  tu_puwUndoBuffer=NULL;
  tu_puwRedoBuffer=NULL;
}

void DeleteOneUndo(CTerrainUndo *ptrud)
{
  if(ptrud->tu_puwUndoBuffer!=NULL)    FreeMemory(ptrud->tu_puwUndoBuffer);
  if(ptrud->tu_puwRedoBuffer!=NULL)    FreeMemory(ptrud->tu_puwRedoBuffer);
  delete ptrud;
}

void DeleteTerrainUndo(CWorldEditorDoc* pDoc)
{
  for(INDEX iUndo=0; iUndo<pDoc->m_dcTerrainUndo.Count(); iUndo++)
  {
    CTerrainUndo *ptu=&pDoc->m_dcTerrainUndo[iUndo];
    pDoc->m_dcTerrainUndo.Remove(ptu);
    DeleteOneUndo(ptu);
  }
}

CTerrain *GetUndoTerrain(ULONG ulEntityID)
{
  // obtain terrain entity
  CEntity *penTerrain=GetEntityForID(_iTerrainEntityID);
  if(penTerrain==NULL)
  {
    return NULL;
   }
  // obtain terrain
  CTerrain *ptrTerrain=penTerrain->GetTerrain();
  return ptrTerrain;
}

void ApplyTerrainUndo(CTerrainUndo *ptrud)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  CTerrain *ptrTerrain=GetUndoTerrain(ptrud->tu_ulEntityID);
  if(ptrTerrain==NULL)
  {
    DeleteOneUndo(ptrud);
    return;
  }

  // obtain and store redo buffer
  if(ptrud->tu_puwRedoBuffer==NULL)
  {
    ptrud->tu_puwRedoBuffer=GetBufferForEditing(ptrTerrain, ptrud->tu_rcRect, 
      ptrud->tu_btUndoBufferType, ptrud->tu_iUndoBufferData);
  }

  // apply buffer change (undo)
  SetBufferForEditing(ptrTerrain, ptrud->tu_puwUndoBuffer, ptrud->tu_rcRect,
    ptrud->tu_btUndoBufferType, ptrud->tu_iUndoBufferData);

  pDoc->m_iCurrentTerrainUndo--;

  DiscardLayerDistribution(ptrud->tu_rcRect);
  pDoc->SetModifiedFlag( TRUE);
  theApp.m_ctTerrainPageCanvas.MarkChanged();
}

void ApplyTerrainRedo(CTerrainUndo *ptrud)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  CTerrain *ptrTerrain=GetUndoTerrain(ptrud->tu_ulEntityID);
  if(ptrTerrain==NULL)
  {
    DeleteOneUndo(ptrud);
    return;
  }

  if(ptrud->tu_puwRedoBuffer==NULL) return;

  // apply buffer change (redo)
  SetBufferForEditing(ptrTerrain, ptrud->tu_puwRedoBuffer, ptrud->tu_rcRect,
    ptrud->tu_btUndoBufferType, ptrud->tu_iUndoBufferData);

  pDoc->m_iCurrentTerrainUndo++;

  DiscardLayerDistribution(ptrud->tu_rcRect);
  pDoc->SetModifiedFlag( TRUE);
  theApp.m_ctTerrainPageCanvas.MarkChanged();
}

UWORD *ExtractUndoRect(PIX pixTerrainWidth)
{
  INDEX ctBuffBytes=_rectUndo.Width()*_rectUndo.Height()*sizeof(UWORD);
  UWORD *puwBuff=(UWORD*)AllocMemory(ctBuffBytes);
  if(puwBuff==NULL) return NULL;
  UWORD *puwBuffTemp=puwBuff;
  for(INDEX y=_rectUndo.rc_iTop; y<_rectUndo.rc_iBottom; y++)
  {
    for(INDEX x=_rectUndo.rc_iLeft; x<_rectUndo.rc_iRight; x++)
    {
      INDEX iOffset=y*pixTerrainWidth+x;
      UWORD uwValue=_puwUndoTerrain[iOffset];
      *puwBuffTemp=uwValue;
      puwBuffTemp++;
    }
  }
  return puwBuff;
}

void TerrainEditBegin(void)
{
  if( !(theApp.m_Preferences.ap_iMemoryForTerrainUndo>0))
  {
    return;
  }
  _bUndoStart=TRUE;
}

void RemoveRedoList(void)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  CDynamicContainer<CTerrainUndo> dcTemp;
  for( INDEX itu=0; itu<pDoc->m_iCurrentTerrainUndo+1; itu++)
  {
    dcTemp.Add(&pDoc->m_dcTerrainUndo[itu]);
  }
  for( INDEX ituDel=pDoc->m_iCurrentTerrainUndo+1; ituDel<pDoc->m_dcTerrainUndo.Count(); ituDel++)
  {
    DeleteOneUndo(&pDoc->m_dcTerrainUndo[ituDel]);
  }
  pDoc->m_dcTerrainUndo.MoveContainer(dcTemp);
}

void LimitMemoryConsumption(INDEX iNewConsumption)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  INDEX ctUndos=pDoc->m_dcTerrainUndo.Count();
  INDEX iLastValid=-1;
  INDEX iSum=iNewConsumption;
  for(INDEX iUndo=ctUndos-1; iUndo>=0; iUndo--)
  {
    CTerrainUndo *ptu=&pDoc->m_dcTerrainUndo[iUndo];
    INDEX iMemory=ptu->tu_rcRect.Width()*ptu->tu_rcRect.Height()*sizeof(UWORD);
    if(ptu->tu_puwRedoBuffer!=NULL)
    {
      iSum=iSum+iMemory*2;
    }
    else
    {
      iSum=iSum+iMemory;
    }

    if(iSum>theApp.m_Preferences.ap_iMemoryForTerrainUndo*1024*1024)
    {
      iLastValid=iUndo+1;
      break;
    }
  }
  if( iLastValid!=-1)
  {
    CDynamicContainer<CTerrainUndo> dcTemp;
    for( INDEX itu=iLastValid; itu<ctUndos; itu++)
    {
      dcTemp.Add(&pDoc->m_dcTerrainUndo[itu]);
    }
    for( INDEX ituDel=0; ituDel<iLastValid; ituDel++)
    {
      DeleteOneUndo(&pDoc->m_dcTerrainUndo[ituDel]);
    }
    pDoc->m_dcTerrainUndo.MoveContainer(dcTemp);
    if(pDoc->m_iCurrentTerrainUndo>=iLastValid)
    {
      pDoc->m_iCurrentTerrainUndo=pDoc->m_iCurrentTerrainUndo-iLastValid;
    }
  }
}

void TerrainEditEnd(void)
{
  if( !(theApp.m_Preferences.ap_iMemoryForTerrainUndo>0))
  {
    return;
  }
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  // obtain terrain entity
  CEntity *penTerrain=GetEntityForID(_iTerrainEntityID);
  if(penTerrain==NULL)
  {
    if(_puwUndoTerrain!=NULL)
    {
      FreeMemory(_puwUndoTerrain);
      _puwUndoTerrain=NULL;
    }
    return;
  }
  // obtain terrain
  CTerrain *ptrTerrain=penTerrain->GetTerrain();
  if(ptrTerrain==NULL)
  {
    if(_puwUndoTerrain!=NULL)
    {
      FreeMemory(_puwUndoTerrain);
      _puwUndoTerrain=NULL;
    }
    return;
  }

  // we will add undo, clear all existing redo's
  RemoveRedoList();

  // remember undo
  CTerrainUndo *ptrud=new CTerrainUndo;
  
  INDEX iNewConsumption=_rectUndo.Width()*_rectUndo.Height()*sizeof(UWORD);
  LimitMemoryConsumption(iNewConsumption);
  
  ptrud->tu_ulEntityID=_iTerrainEntityID;
  ptrud->tu_rcRect=_rectUndo;
  ptrud->tu_btUndoBufferType=_btUndoBufferType;
  ptrud->tu_iUndoBufferData=_iUndoBufferData;
  ptrud->tu_puwUndoBuffer=ExtractUndoRect(ptrTerrain->tr_pixHeightMapWidth);
  
  if(ptrud->tu_puwUndoBuffer!=NULL)
  {
    pDoc->m_dcTerrainUndo.Add(ptrud);
  }
  else
  {
    delete ptrud;
  }
  pDoc->m_iCurrentTerrainUndo=pDoc->m_dcTerrainUndo.Count()-1;
  // release obtained terrain buffer
  if(_puwUndoTerrain!=NULL)
  {
    FreeMemory(_puwUndoTerrain);
    _puwUndoTerrain=NULL;
  }
}

CTileInfo::CTileInfo()
{
  ti_ix=-1;
  ti_iy=-1;
  ti_bSwapXY=FALSE;
  ti_bFlipX=FALSE;
  ti_bFlipY=FALSE;
}

void ObtainLayerTileInfo(CDynamicContainer<CTileInfo> *pdcTileInfo, CTextureData *ptdTexture, INDEX &ctTilesPerRow)
{
  CTFileName fnTexture=ptdTexture->GetName();
  CTFileName fnTileInfo=fnTexture.NoExt()+CTString(".tli");

  INDEX ctParsedLines=0;
  try
  {
	  char achrLine[ 256];
    CTFileStream strm;
  	strm.Open_t( fnTileInfo);

	  FOREVER
	  {
      CDynamicContainer<CTString> dcTokens;

      strm.GetLine_t(achrLine, 256);
      ctParsedLines++;
      char achrSeparators[]   = " ,";

      char *pchrToken = strtok( achrLine, achrSeparators);
      while( pchrToken != NULL )
      {
        CTString *pstrToken=new CTString();
        *pstrToken=CTString( pchrToken);
        dcTokens.Add(pstrToken);
        // next token
        pchrToken = strtok( NULL, achrSeparators);
      }

      // if no tokens parsed
      if(dcTokens.Count()==0) continue;

      INDEX iToken=0;
      // analyze parsed tokens
      if(dcTokens[iToken]=="TilesPerRow")
      {
        // there must be at least 1 token for 'TilesPerRow' indentifier
        if(dcTokens.Count()-1-iToken<1)
        {
          throw("You must enter number of tiles per raw.");
        }
        ctTilesPerRow=0;
        INDEX iResultTPR=sscanf(dcTokens[iToken+1], "%d", &ctTilesPerRow);
        if(iResultTPR<=0)
        {
          ctTilesPerRow=0;
          throw("Unable to parse count of tiles per row.");
        }
      }
      else if(dcTokens[iToken]=="Tile")
      {
        // there must be at least 2 tokens for 'Tile' indentifier
        if(dcTokens.Count()-1-iToken<2)
        {
          throw("You must enter 2 coordinates per tile.");
        }
        INDEX x,y;

        INDEX iResultX=sscanf(dcTokens[iToken+1], "%d", &x);
        if(iResultX<=0)
        {
          throw("Unable to parse x coordinate.");
        }
        INDEX iResultY=sscanf(dcTokens[iToken+2], "%d", &y);
        if(iResultY<=0)
        {
          throw("Unable to parse y coordinate.");
        }
        if(x<=0 || y<=0)
        {
          throw("Tile coordinates must be greater than 0.");
        }
        // jump over coordinate tokens
        iToken+=3;

        // add tile info
        CTileInfo *pti=new CTileInfo();
        pti->ti_ix=x-1;
        pti->ti_iy=y-1;

        for( INDEX iFlagToken=iToken; iFlagToken<dcTokens.Count(); iFlagToken++)
        {
          if(dcTokens[iFlagToken]=="SwapXY")
          {
            pti->ti_bSwapXY=TRUE;
          }
          else if(dcTokens[iFlagToken]=="FlipX")
          {
            pti->ti_bFlipX=TRUE;
          }
          else if(dcTokens[iFlagToken]==";")
          {
            break;
          }
          else if(dcTokens[iFlagToken]=="FlipY")
          {
            pti->ti_bFlipY=TRUE;
          }
          else
          {
            throw("Unrecognizable character found.");
          }
        }
        pdcTileInfo->Add(pti);
      }

      // clear allocated tokens
      for(INDEX i=0; i<dcTokens.Count(); i++)
      {
        delete &dcTokens[i];
      }
      dcTokens.Clear();
    }
  }
  catch (const char *strError)
  {
    (void) strError;
  }
}

void TilePaintTool(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  CTerrainLayer *ptlLayer=GetLayer();
  if(ptrTerrain==NULL || ptlLayer==NULL || ptlLayer->tl_ltType!=LT_TILE || ptlLayer->tl_ptdTexture==NULL) return;
  
  CDynamicContainer<CTileInfo> dcTileInfo;
  INDEX ctTilesPerRaw=0;
  ObtainLayerTileInfo( &dcTileInfo, ptlLayer->tl_ptdTexture, ctTilesPerRaw);
  INDEX ctTiles=dcTileInfo.Count();
  if(ctTilesPerRaw==0 || ctTiles==0) return;
  ptlLayer->SetTilesPerRow(ctTilesPerRaw);
  ptlLayer->tl_iSelectedTile= Clamp( ptlLayer->tl_iSelectedTile, (INDEX)0, INDEX(ctTiles-1) );
  if(ptlLayer->tl_iSelectedTile==-1) return;
  CTileInfo &ti=dcTileInfo[ptlLayer->tl_iSelectedTile];

  // _rect holds terrain size
  if(_fStrength>0)
  {
    UWORD uwValue=
      dcTileInfo[ptlLayer->tl_iSelectedTile].ti_iy*ctTilesPerRaw+
      dcTileInfo[ptlLayer->tl_iSelectedTile].ti_ix;
    if(ti.ti_bFlipX) uwValue|=TL_FLIPX;
    if(ti.ti_bFlipY) uwValue|=TL_FLIPY;
    if(ti.ti_bSwapXY) uwValue|=TL_SWAPXY;
    uwValue|=TL_VISIBLE;
    _puwBuffer[0]=uwValue<<8;
  }
  else
  {
    _puwBuffer[0]=0;
  }

  /*
  vidjeti sta ne radi sa brisanjem tile-ova
  ne pogadja se dobro lokacija tile-a ispod misa
  +view layer on/off ne discarda pretender texture
  +ne crta se dobro trenutno selektirani tile i preklapa se animirani (under mouse)
  +rotirane i flipane tileove crtati
  +nesto zapinje kad se prvi put otvara info window
  +ubaciti skrolanje tileova na mouse wheel
  +pick tile
  */


  // free allocated tile info structures
  for(INDEX i=0; i<dcTileInfo.Count(); i++)
  {
    delete &dcTileInfo[i];
  }
  dcTileInfo.Clear();

}

void EditTerrain(CTextureData *ptdBrush, FLOAT3D &vHitPoint, FLOAT fStrength, ETerrainEdit teTool)
{
  _ptdBrush=ptdBrush;
  _fStrength=fStrength;

  CTerrain *ptrTerrain=GetTerrain();
  CTerrainLayer *ptlLayer=GetLayer();
  INDEX iLayer=GetLayerIndex();
  if(ptrTerrain==NULL || ptlLayer==NULL) return;

  // obtain buffer type
  BufferType btBufferType=BT_INVALID;
  INDEX iBufferData=-1;
  if( (teTool>TE_BRUSH_ALTITUDE_START && teTool<TE_BRUSH_ALTITUDE_END) ||
      (teTool>TE_ALTITUDE_START       && teTool<TE_ALTITUDE_END) ||
      (teTool==TE_GENERATE_TERRAIN) ||
      (teTool==TE_ALTITUDE_EQUALIZE) )
  {
    btBufferType=BT_HEIGHT_MAP;
  }
  else if( (teTool>TE_BRUSH_LAYER_START && teTool<TE_BRUSH_LAYER_END) ||
           (teTool>TE_LAYER_START       && teTool<TE_LAYER_END) ||
            teTool==TE_TILE_PAINT)
  {
    btBufferType=BT_LAYER_MASK;
    iBufferData=iLayer;
  }
  else if( teTool>TE_BRUSH_EDGE_START && teTool<TE_BRUSH_EDGE_END)
  {
    btBufferType=BT_EDGE_MAP;
  }
  else
  {
    return;
  }

  _puwBuffer=NULL;
  _srcExtraW=0;
  _srcExtraH=0;

  if( teTool==TE_BRUSH_ALTITUDE_SMOOTH ||
      teTool==TE_BRUSH_ALTITUDE_FILTER ||
      teTool==TE_BRUSH_LAYER_SMOOTH ||
      teTool==TE_ALTITUDE_SMOOTH ||
      teTool==TE_ALTITUDE_FILTER ||
      teTool==TE_LAYER_SMOOTH ||
      teTool==TE_LAYER_FILTER)
  {
    _srcExtraW=2;
    _srcExtraH=2;
  }

  // extract source rectangle
  Point pt=Calculate2dHitPoint(ptrTerrain, vHitPoint);
  // perform operation on brush rect
  if(teTool==TE_TILE_PAINT)
  {
    _rect.rc_iLeft=pt.pt_iX;
    _rect.rc_iRight=_rect.rc_iLeft+1;
    _rect.rc_iTop=pt.pt_iY;
    _rect.rc_iBottom=_rect.rc_iTop+1;
  }
  else if(_ptdBrush!=NULL)
  {
    _rect.rc_iLeft=pt.pt_iX-ptdBrush->GetPixWidth()/2-_srcExtraW;
    _rect.rc_iRight=pt.pt_iX+(ptdBrush->GetPixWidth()-ptdBrush->GetPixWidth()/2)+_srcExtraW;
    _rect.rc_iTop=pt.pt_iY-ptdBrush->GetPixHeight()/2-_srcExtraH;
    _rect.rc_iBottom=pt.pt_iY+(ptdBrush->GetPixHeight()-ptdBrush->GetPixHeight()/2)+_srcExtraH;
  }
  // perform operation on whole terrain area
  else
  {
    _rect.rc_iLeft=0;
    _rect.rc_iRight=ptrTerrain->tr_pixHeightMapWidth;
    _rect.rc_iTop=0;
    _rect.rc_iBottom=ptrTerrain->tr_pixHeightMapHeight;
  }

  Rect rectTerrain;
  rectTerrain.rc_iLeft=0;
  rectTerrain.rc_iTop=0;
  rectTerrain.rc_iRight=ptrTerrain->tr_pixHeightMapWidth;
  rectTerrain.rc_iBottom=ptrTerrain->tr_pixHeightMapHeight;

  BOOL bAutoRememberUndo=FALSE;
  // if should apply undo for whole terrain
  if( (teTool>TE_ALTITUDE_START && teTool<TE_ALTITUDE_END) ||
      (teTool>TE_LAYER_START    && teTool<TE_LAYER_END) ||
      (teTool==TE_GENERATE_TERRAIN) ||
      (teTool==TE_ALTITUDE_EQUALIZE) )
  {
    TerrainEditBegin();
    bAutoRememberUndo=TRUE;
  }

  // edit start, undo starts
  if(_bUndoStart)
  {
    _bUndoStart=FALSE;
    _btUndoBufferType=btBufferType;
    _iUndoBufferData=iBufferData;
    _rectUndo=_rect;
    _iTerrainEntityID=ptrTerrain->tr_penEntity->en_ulID;

    _puwUndoTerrain=GetBufferForEditing(ptrTerrain, rectTerrain, btBufferType, iBufferData);
  }
  // editing in progress, update undo data
  else
  {
    // update undo rect
    if(_rect.rc_iLeft   < _rectUndo.rc_iLeft)    _rectUndo.rc_iLeft=_rect.rc_iLeft;
    if(_rect.rc_iRight  > _rectUndo.rc_iRight)   _rectUndo.rc_iRight=_rect.rc_iRight;
    if(_rect.rc_iTop    < _rectUndo.rc_iTop)     _rectUndo.rc_iTop=_rect.rc_iTop;
    if(_rect.rc_iBottom > _rectUndo.rc_iBottom)  _rectUndo.rc_iBottom=_rect.rc_iBottom;
  }

  // clamp undo rect to terrain size
  _rectUndo.rc_iLeft=Clamp(_rectUndo.rc_iLeft, rectTerrain.rc_iLeft, rectTerrain.rc_iRight);
  _rectUndo.rc_iRight=Clamp(_rectUndo.rc_iRight, rectTerrain.rc_iLeft, rectTerrain.rc_iRight);
  _rectUndo.rc_iTop=Clamp(_rectUndo.rc_iTop, rectTerrain.rc_iTop, rectTerrain.rc_iBottom);
  _rectUndo.rc_iBottom=Clamp(_rectUndo.rc_iBottom, rectTerrain.rc_iTop, rectTerrain.rc_iBottom);

  // obtain buffer
  _puwBuffer=GetBufferForEditing(ptrTerrain, _rect, btBufferType, iBufferData);

  switch( teTool)
  {
    case TE_BRUSH_ALTITUDE_PAINT:
    {
      ApplyAddPaint(MIN_UWORD,MAX_UWORD);
      break;
    }
    case TE_BRUSH_EDGE_ERASE:
    {
      _fStrength=-fStrength;
      ApplyAddPaint(MIN_UWORD,MAX_UWORD);
      break;
    }
    case TE_BRUSH_LAYER_PAINT:
    {
      _fStrength=fStrength*32.0f;
      ApplyAddPaint(MIN_UWORD,MAX_UWORD);
      break;
    }
    case TE_BRUSH_ALTITUDE_SMOOTH:
    case TE_BRUSH_LAYER_SMOOTH:
    case TE_ALTITUDE_SMOOTH:
    case TE_LAYER_SMOOTH:
    {
      _fStrength=fStrength*theApp.m_fSmoothPower;
      ApplyFilterMatrix(_afFilterBlurMore);
      break;
    }
    case TE_BRUSH_ALTITUDE_FILTER:
    case TE_BRUSH_LAYER_FILTER:
    case TE_LAYER_FILTER:
    case TE_ALTITUDE_FILTER:
    {
      _fStrength=fStrength*theApp.m_fFilterPower;
      switch(theApp.m_iFilter)
      {
        case FLT_FINEBLUR:    ApplyFilterMatrix(_afFilterFineBlur    ); break;
        case FLT_SHARPEN:     ApplyFilterMatrix(_afFilterSharpen     ); break;
        case FLT_EMBOSS:      ApplyFilterMatrix(_afFilterEmboss      ); break;
        case FLT_EDGEDETECT:  ApplyFilterMatrix(_afFilterEdgeDetect  ); break;
      }
      break;
    }
    case TE_BRUSH_ALTITUDE_MINIMUM:
    case TE_ALTITUDE_MINIMUM:
    {
      _fStrength=0;
      ApplyAddPaint(theApp.m_uwEditAltitude,MAX_UWORD);
      break;
    }
    case TE_BRUSH_ALTITUDE_MAXIMUM:
    case TE_ALTITUDE_MAXIMUM:
    {
      _fStrength=0;
      ApplyAddPaint(MIN_UWORD, theApp.m_uwEditAltitude);
      break;
    }
    case TE_BRUSH_ALTITUDE_FLATTEN:
    case TE_ALTITUDE_FLATTEN:
    {
      _fStrength=0;
      ApplyAddPaint(theApp.m_uwEditAltitude, theApp.m_uwEditAltitude);
      break;
    }    
    case TE_BRUSH_ALTITUDE_POSTERIZE:
    case TE_ALTITUDE_POSTERIZE:
    {
      ApplyPosterize();
      break;
    }    
    case TE_BRUSH_ALTITUDE_RND_NOISE:
    case TE_BRUSH_LAYER_RND_NOISE:
    case TE_ALTITUDE_RND_NOISE:
    case TE_LAYER_RND_NOISE:
    {
      ApplyRNDNoise();
      break;
    }    
    case TE_BRUSH_ALTITUDE_CONTINOUS_NOISE:
    case TE_BRUSH_LAYER_CONTINOUS_NOISE:
    case TE_ALTITUDE_CONTINOUS_NOISE:
    case TE_LAYER_CONTINOUS_NOISE:
    {
      if(!SetupContinousNoiseTexture()) return;
      ApplyContinousNoise();
      FreeContinousNoiseTexture();
      break;
    }    
    case TE_GENERATE_TERRAIN:
    {
      GenerateTerrain();
      break;
    }
    case TE_ALTITUDE_EQUALIZE:
    {
      EqualizeBuffer();
      break;
    }
    case TE_CLEAR_LAYER_MASK:
    {
      for(INDEX i=0; i<_rect.Width()*_rect.Height(); i++)
      {
        _puwBuffer[i]=0;
      }
      break;
    }
    case TE_FILL_LAYER_MASK:
    {
      for(INDEX i=0; i<_rect.Width()*_rect.Height(); i++)
      {
        _puwBuffer[i]=MAX_UWORD;
      }
      break;
    }
    case TE_TILE_PAINT:
    {
      TilePaintTool();
      break;
    }
  }

  // apply buffer change
  SetBufferForEditing(ptrTerrain, _puwBuffer, _rect, btBufferType, iBufferData);
  theApp.GetActiveDocument()->SetModifiedFlag( TRUE);
  FreeMemory(_puwBuffer);

  // mark rect for layer distribution updating
  if(teTool!=TE_TILE_PAINT)
  {
    DiscardLayerDistribution(_rect);
  }

  theApp.m_ctTerrainPageCanvas.MarkChanged();

  if(bAutoRememberUndo)
  {
    TerrainEditEnd();
  }
}
