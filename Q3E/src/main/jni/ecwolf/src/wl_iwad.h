/*
** wl_iwad.h
**
**---------------------------------------------------------------------------
** Copyright 2012 Braden Obrzut
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

#ifndef __WL_IWAD_H__
#define __WL_IWAD_H__

#include "zstring.h"

// For IWad Pickers so not in namespace
struct WadStuff
{
	WadStuff() : Type(-1), Hidden(false) {}

	TArray<FString> Path;
	FString Extension;
	FString Name;
	int Type;
	bool Hidden;
};

namespace IWad
{
	enum Flags
	{
		REGISTERED = 1, // Enables not-shareware warning
		HELPHACK = 2,   // Fixes helpart art assets
		PREVIEW = 4,    // Only show in picker if user opts in
		RESOURCE = 8    // Used as a component of another option
	};

	struct IWadData
	{
		FString Name;
		FString Autoname;
		FString Mapinfo;
		TArray<FString> Ident;
		TArray<FString> Required;
		FName Game;
		unsigned int Flags;
		bool LevelSet;
	};

	bool CheckGameFilter(FName filter);
	const IWadData &GetGame();
	unsigned int GetNumIWads();
	void SelectGame(TArray<FString> &wadfiles, const char* iwad, const char* datawad, const FString &progdir);
}

#endif
