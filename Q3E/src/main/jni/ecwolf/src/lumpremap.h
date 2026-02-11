/*
** lumpremap.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#ifndef __LUMPREMAP_H__
#define __LUMPREMAP_H__

#include "resourcefiles/resourcefile.h"
#include "tarray.h"
#include "zstring.h"

class LumpRemapper
{
	public:
		enum Type
		{
			AUDIOT,
			VGAGRAPH,
			VSWAP
		};

		enum PSpriteType
		{
			PSPR_NONE,
			PSPR_NORMAL,
			PSPR_BLAKE
		};

		LumpRemapper(const char* extension);

		void		AddFile(FResourceFile *file, Type type);
		void		DoRemap();

		static void	AddFile(const char* extension, FResourceFile *file, Type type);
		static void ClearRemaps();
		static void	LoadMap(const char* extension, const char* name, const char* data, unsigned int length);
		static unsigned int LumpSampleRate(FResourceFile *Owner);
		static PSpriteType IsPSprite(int lumpnum);
		static void	RemapAll();
	protected:
		bool		LoadMap();
		void		LoadMap(const char* name, const char* data, unsigned int length);
		void		ParseMap(class Scanner &sc);
	private:
		struct RemapFile
		{
			FResourceFile	*file;
			Type			type;
		};

		unsigned int		digiTimerValue;
		bool				loaded;
		FString				mapLumpName;
		TArray<FString>		graphics, sprites, sounds, digitalsounds, music, textures;
		TArray<RemapFile>	files;
};

#endif
