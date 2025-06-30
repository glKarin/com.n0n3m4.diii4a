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

#ifndef SE_INCL_SOUNDDECODER_H
#define SE_INCL_SOUNDDECODER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

class CSoundDecoder {
public:
  class CDecodeData_MPEG *sdc_pmpeg;
  class CDecodeData_OGG  *sdc_pogg ;

  // initialize/end the decoding support engine(s)
  static void InitPlugins(void);
  static void EndPlugins(void);

  // create a decoder that streams from file
  CSoundDecoder(const CTFileName &fnmStream);
  ~CSoundDecoder(void);
  void Clear(void);

  // check if a decoder is succefully opened
  BOOL IsOpen(void);
  // get wave format of the decoder (invaid if it is not open)
  void GetFormat(WAVEFORMATEX &wfe);

  // decode a block of bytes
  INDEX Decode(void *pvDestBuffer, INDEX ctBytesToDecode);
  // reset decoder to start of sample
  void Reset(void);
};

#endif  /* include-once check. */

