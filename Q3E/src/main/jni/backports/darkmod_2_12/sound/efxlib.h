/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#ifndef __EFXLIBH
#define __EFXLIBH

#include "../idlib/containers/List.h"
#include "../idlib/Str.h"
#include "../idlib/Lexer.h"
#include "../idlib/Heap.h"
#include "../framework/Common.h"
#include "sound.h"


#define EFX_VERBOSE 0

#if EFX_VERBOSE
#define EFXprintf(...) do { common->Printf(__VA_ARGS__); } while (false)
#else
#define EFXprintf(...) do { } while (false)
#endif

struct idSoundEffect {
	idSoundEffect();
	~idSoundEffect();

	bool alloc();

	idStr name;
	ALuint effect;
};

class idEFXFile {
public:
	idEFXFile();
	~idEFXFile();

	bool FindEffect(idStr &name, ALuint *effect);
	bool LoadFile(const char *filename);
	void Clear(void);

	//reloading with listSounds command
	bool Reload();
	bool IsAfterReload();

private:
	bool ReadEffectLegacy(idLexer &lexer, idSoundEffect *effect);
	bool ReadEffectOpenAL(idLexer &lexer, idSoundEffect *effect);

	//filename initially passed to LoadFile (or empty if LoadFile never called)
	//(used in Reload method)
	idStr efxFilename;
	int version;
	//this flag is raised after reload: idSoundWorldLocal::MixLoop checks for it
	bool isAfterReload;

	idList<idSoundEffect *> effects;
};

#endif // __EFXLIBH
