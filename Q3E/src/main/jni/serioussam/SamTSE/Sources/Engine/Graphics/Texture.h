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

#ifndef SE_INCL_TEXTURE_H
#define SE_INCL_TEXTURE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/Anim.h>
#include <Engine/Graphics/GfxLibrary.h>

#define BYTES_PER_TEXEL 4   // all textures in engine are 4 bytes per pixel

// td_ulFlags bits
// (on disk and in memory)
#define TEX_ALPHACHANNEL (1UL<<0)   // texture has alpha channel (for old version support)
#define TEX_32BIT        (1UL<<1)   // texture needs to be in 32-bit quality uploaded if can
// (only in memory)
#define TEX_STATIC       (1UL<<5)   // remain loaded after being bound (i.e. uploaded - for base textures)
#define TEX_CONSTANT     (1UL<<6)   // cannot be changed (no mip-map disposing, no LOD biasing, no colorizing, nothing!)
#define TEX_TRANSPARENT  (1UL<<7)   // only one bit of alpha channel is enough (internal format GL_RGB5_A1)
#define TEX_EQUALIZED    (1UL<<8)   // texture has 128-gray last mipmap (i.e. can be discarded in shade mode)
#define TEX_GRAY         (1UL<<9)   // grayscale texture
#define TEX_WHITE        (1UL<<10)  // completely white texture (believe me, there are some cases)
#define TEX_KEEPCOLOR    (1UL<<11)  // don't (de)saturate (for heightmaps and such!)
#define TEX_SINGLEMIPMAP (1UL<<18)  // set if last uploading was in single-mipmap
#define TEX_PROBED       (1UL<<19)  // set if last binding was as probe-texture
// (flags that shows if texture mipmaps has been changed)
#define TEX_DISPOSED     (1UL<<20)  // largest mip-map(s) has been left-out
#define TEX_DITHERED     (1UL<<21)  // dithering has been applied on this texture
#define TEX_FILTERED     (1UL<<22)  // flitering has been applied on this texture
#define TEX_SATURATED    (1UL<<23)  // saturation has been adjusted on this texture
#define TEX_COLORIZED    (1UL<<24)  // mipmaps has been colorized on this texture
#define TEX_WASOLD       (1UL<<30)  // loaded from old format (version 3)


/*
 * Bitmap data for a class of texture objects
 */
class ENGINE_API CTextureData : public CAnimData {
public:
// implementation:
  ULONG td_ulFlags;             // see defines
  MEX   td_mexWidth, td_mexHeight; // texture dimensions
  INDEX td_iFirstMipLevel;      // the highest quality mip level
  INDEX td_ctFineMipLevels;     // number of bilineary created mip levels
  SLONG td_slFrameSize;         // sum of sizes of all mip-maps for one frame
  INDEX td_ctFrames;            // number of different frames

  class CTexParams td_tpLocal;  // local texture parameters
  ULONG td_ulInternalFormat;    // format in which texture will be uploaded
  CTimerValue td_tvLastDrawn;   // timer for probing
  ULONG td_ulProbeObject;
  union {
    ULONG  td_ulObject;
    ULONG *td_pulObjects;
  };
  ULONG *td_pulFrames;          // all frames with their mip-maps and private palettes
  UBYTE *td_pubBuffer1, *td_pubBuffer2;       // buffers for effect textures
  PIX td_pixBufferWidth, td_pixBufferHeight;  // effect buffer dimensions
  class CTextureData *td_ptdBaseTexture;      // base texure for effects (if any)
  class CTextureEffectGlobal *td_ptegEffect;  // all data for effect textures

  INDEX td_iRenderFrame; // frame number currently rendering (for profiling)

  // constructor and destructor
	CTextureData();
	~CTextureData();

  // reference counting (override from CAnimData)
  void RemReference_internal(void);

  // converts global mip level to the corresponding one of texture
  INDEX ClampMipLevel( FLOAT fMipFactor) const;

  // gets values from some of texture data members
  inline MEX GetWidth(void)     const { return td_mexWidth;  };
  inline MEX GetHeight(void)    const { return td_mexHeight; };
  inline PIX GetPixWidth(void)  const { return td_mexWidth >>td_iFirstMipLevel; };
  inline PIX GetPixHeight(void) const { return td_mexHeight>>td_iFirstMipLevel; };
  inline ULONG GetFlags(void)   const { return td_ulFlags; };
  inline ULONG GetNoOfMips(void)     const { return GetNoOfMipmaps( GetPixWidth(), GetPixHeight()); };
  inline ULONG GetNoOfFineMips(void) const { return td_ctFineMipLevels; };

  // mark that texture has been used
  inline void MarkDrawn(void) { td_tvLastDrawn = _pTimer->GetLowPrecisionTimer(); };

  // get string description of texture size, mips and parameters
  CTString GetDescription(void);

  // sets new texture mex width and changes height remaining texture's aspect ratio
  inline void ChangeSize( MEX mexNewWidth) {
    td_mexHeight = MEX( ((FLOAT)mexNewWidth)/td_mexWidth * td_mexHeight);
    td_mexWidth  = mexNewWidth;
  };

  // check if texture frame(s) has been somehow altered (dithering, filtering, saturation, colorizing...)
  inline BOOL IsModified(void) {
    return td_ulFlags & (TEX_DISPOSED|TEX_DITHERED|TEX_FILTERED|TEX_SATURATED|TEX_COLORIZED);
  };
  
  // export finest mipmap of one texture's frame to imageinfo
  void Export_t( class CImageInfo &iiExportedImage, INDEX iFrame);

  // set texture frame as current for accelerator (this will upload texture that needs or wants uploading)
  void SetAsCurrent( INDEX iFrameNo=0, BOOL bForceUpload=FALSE);

  // creates new effect texture with one frame
  void CreateEffectTexture( PIX pixWidth, PIX pixHeight, MEX mexWidth,
                            CTextureData *ptdBaseTexture, ULONG ulGlobalEffect);
  // creates new texture with one frame
  void Create_t( const CImageInfo *pII, MEX mexWanted, INDEX ctFineMips, BOOL bForce32bit);
  // adds one frame to created texture
  void AddFrame_t( const CImageInfo *pII);

  // remove texture from gfx API (driver)
  void Unbind(void);
  // free memory allocated for texture
  void Clear(void);

  // read texture from file
  void Read_t( CTStream *inFile);
  // write texture to file
  void Write_t( CTStream *outFile);
  // force texture to be re-loaded (if needed) in corresponding manner
  void Force( ULONG ulTexFlags);

  // get texel from texture's largest mip-map
  COLOR GetTexel( MEX mexU, MEX mexV);
  // copy (and eventually convert to floats) one row from texture to an array (iChannel is 1=R,2=G,3=B,4=A)
  void  FetchRow( PIX pixRow, void *pfDst, INDEX iChannel=4, BOOL bConvertToFloat=TRUE);
  // get pointer to one row of texture
  ULONG *GetRowPointer( PIX pixRow);
  
// overridden from CSerial:

  // check if this kind of objects is auto-freed
  virtual BOOL IsAutoFreed(void);
  // get amount of memory used by this object
  virtual SLONG GetUsedMemory(void);
};


/*
 * An instance of a texture object
 */
class ENGINE_API CTextureObject : public CAnimObject {
// implementation:
public:
// interface:
public:
  CTextureObject(void);
  // copy from another object of same class
  void Copy(CTextureObject &toOther);
  MEX GetWidth(void) const;
  MEX GetHeight(void) const;
  ULONG GetFlags(void) const;
  void Read_t(  CTStream *istrFile); // throw char * //	read and
	void Write_t( CTStream *ostrFile); // throw char * //	write functions

  // obtain texture and set it for this object
  void SetData_t(const CTFileName &fnmTexture); // throw char *
  // get filename of texture or empty string if no texture
  const CTFileName &GetName(void);
};

ENGINE_API extern void CreateTexture_t( const CTFileName &inFileName,
                                        MEX inMex, INDEX inMipmaps, BOOL bForce32bit);
ENGINE_API extern void CreateTexture_t( const CTFileName &inFileName, const CTFileName &outFileName,
                                        MEX inMex, INDEX inMipmaps, BOOL bForce32bit);
ENGINE_API extern void ProcessScript_t( const CTFileName &inFileName);



/*
 * Render-to-texture class
 */
/*
class ENGINE_API CRenderTexture
{
// implementation:
public:
  CListNode rt_lnInGfx          // for linking in list of all renderable textures
  ULONG rt_ulFlags;             // see defines (only alpha and 32bit, for now)
  PIX   tt_pixWidth, rt_pixHeight; // texture dimensions
  ULONG td_ulInternalFormat;    // format in which texture will be uploaded
  ULONG *rt_pulImage;           // image in memory (no mipmaps for now!)
  class CTexParams td_tpLocal;  // local texture parameters

// interface:
public:
  CRenderTexture(void);
  ~CRenderTexture(void);
  // prepare
  BOOL Init( PIX pixWidth, PIX pixHeight, BOOL b32bit, BOOL bAlpha=FALSE);
  // reset (i.e. prepare again - after display mode switch and stuff like that)
  void Reset(void);
  // set texture as current for accelerator
  void SetAsCurrent(void);
  // set texture as target for rendering
  void SetAsTarget(void);
};
*/

#endif  /* include-once check. */

