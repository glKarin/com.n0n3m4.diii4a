/*
*/

#ifndef __EFXLIBH
#define __EFXLIBH

#ifdef _OPENAL_EFX

#include "../../idlib/precompiled.h"

#ifdef _DEBUG_AL
#define EFX_VERBOSE 1
#else
#define EFX_VERBOSE 0
#endif

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
	void UnloadFile(void);
	void Clear( void );

private:
	bool ReadEffect( idLexer &lexer, idSoundEffect *effect );

	idList<idSoundEffect *>effects;
};
#else
#include "eax4.h"




///////////////////////////////////////////////////////////
// Class definitions.
class idSoundEffect
{
	public:
		idSoundEffect() {
		};
		~idSoundEffect() {
			if (data && datasize) {
				Mem_Free(data);
				data = NULL;
			}
		}

		idStr name;
		int datasize;
		void *data;
};

class idEFXFile
{
	private:

	protected:
		// Protected data members.

	public:
		// Public data members.

	private:

	public:
		idEFXFile();
		~idEFXFile();

		bool FindEffect(idStr &name, idSoundEffect **effect, int *index);
		bool ReadEffect(idLexer &lexer, idSoundEffect *effect);
		bool LoadFile(const char *filename, bool OSPath = false);
		void UnloadFile(void);
		void Clear(void);

		idList<idSoundEffect *>effects;
};
///////////////////////////////////////////////////////////
#endif



#endif // __EFXLIBH

