/*
** wolfmapcommon.h
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#ifndef __WOLFMAPCOMMON_H__
#define __WOLFMAPCOMMON_H__

#include "files.h"
#include "resourcefile.h"

#define RTLCONVERTEDPLANES 4
#define PLANES 3
#define HEADERSIZE 34
#define CARMACK_NEARTAG	static_cast<unsigned char>(0xA7)
#define CARMACK_FARTAG	static_cast<unsigned char>(0xA8)

struct FMapLump : public FResourceLump
{
	protected:
		void ExpandCarmack(const unsigned char* in, unsigned char* out);
		void ExpandRLEW(const unsigned char* in, unsigned char* out, const DWORD length, const WORD rlewTag);

		int FillCache();
	public:
		struct
		{
			DWORD	PlaneOffset[PLANES];
			WORD	PlaneLength[PLANES];
			WORD	Width;
			WORD	Height;
			char	Name[24];
		} Header;
		WORD rlewTag;
		bool carmackCompressed;
		bool rtlMap;

		FMapLump() : FResourceLump()
		{
			LumpSize = HEADERSIZE;
			carmackCompressed = true;
			rtlMap = false;
		}
		~FMapLump()
		{
		}
};

#endif
