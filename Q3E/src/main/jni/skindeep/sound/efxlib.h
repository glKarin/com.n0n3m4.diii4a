/*
*/

#ifndef __EFXLIBH
#define __EFXLIBH

#include "idlib/containers/List.h"
#include "idlib/Str.h"
#include "idlib/Lexer.h"
#include "idlib/Heap.h"
#include "framework/Common.h"
#include "sound/sound.h"

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

	bool FindEffect( idStr &name, ALuint *effect );
	bool LoadFile( const char *filename, bool OSPath = false );
	void Clear( void );

	//DARKMOD
	bool Reload();
	bool IsAfterReload();

private:
	bool ReadEffect( idLexer &lexer, idSoundEffect *effect );
	idList<idSoundEffect *>effects;

	//DARKMOD
	idStr efxFilename;
	bool isAfterReload;
};

#endif // __EFXLIBH
