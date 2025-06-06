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

#ifndef SE_INCL_REPLACEFILE_H
#define SE_INCL_REPLACEFILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

extern BOOL _bFileReplacingApplied;

BOOL GetReplacingFile(CTFileName fnSourceTexture, CTFileName &fnReplacingTexture,
                      const char *pFilter);
void SetTextureWithPossibleReplacing_t(CTextureObject &to, CTFileName &fnmTexture);

// read/write a texture object
void ReadTextureObject_t(CTStream &strm, CTextureObject &to);
void SkipTextureObject_t(CTStream &strm);
void WriteTextureObject_t(CTStream &strm, CTextureObject &to);

// read/write a model and its texture(s) from a file
void ReadModelObject_t(CTStream &strm, CModelObject &mo);
void SkipModelObject_t(CTStream &strm);
void WriteModelObject_t(CTStream &strm, CModelObject &mo);

// read/write a ska model from a file
void WriteModelInstance_t(CTStream &strm, CModelInstance &mi);
void ReadModelInstance_t(CTStream &strm, CModelInstance &mi);
void SkipModelInstance_t(CTStream &strm);


// read/write an anim object from a file
void ReadAnimObject_t(CTStream &strm, CAnimObject &mo);
void SkipAnimObject_t(CTStream &strm);
void WriteAnimObject_t(CTStream &strm, CAnimObject &mo);

// read/write a sound object from a file
void ReadSoundObject_t(CTStream &strm, CSoundObject &mo);
void SkipSoundObject_t(CTStream &strm);
void WriteSoundObject_t(CTStream &strm, CSoundObject &mo);


#endif  /* include-once check. */

