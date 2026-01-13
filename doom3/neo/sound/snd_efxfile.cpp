/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "snd_local.h"

#ifdef _OPENAL
#ifdef _OPENAL_EFX
#define mB_to_gain(millibels, property) \
	_mB_to_gain(millibels,AL_EAXREVERB_MIN_ ## property, AL_EAXREVERB_MAX_ ## property)

static inline ALfloat _mB_to_gain(ALfloat millibels, ALfloat min, ALfloat max) {
	return idMath::ClampFloat(min, max, idMath::Pow(10.0f, millibels / 2000.0f));
}

idSoundEffect::idSoundEffect() :
	effect(0) {
}

idSoundEffect::~idSoundEffect() {
	if (soundSystemLocal.alIsEffect(effect))
	    soundSystemLocal.alDeleteEffects(1, &effect);
}

bool idSoundEffect::alloc() {
	alGetError();

	ALenum e;

	soundSystemLocal.alGenEffects(1, &effect);
	e = alGetError();
	if (e != AL_NO_ERROR) {
		common->Warning("idSoundEffect::alloc: alGenEffects failed: 0x%x", e);
		return false;
	}

	soundSystemLocal.alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
	e = alGetError();
	if (e != AL_NO_ERROR) {
		common->Warning("idSoundEffect::alloc: alEffecti failed: 0x%x", e);
		return false;
	}

	return true;
}

/*
===============
idEFXFile::idEFXFile
===============
*/
idEFXFile::idEFXFile( void ) {
}

/*
===============
idEFXFile::Clear
===============
*/
void idEFXFile::Clear( void ) {
	effects.DeleteContents( true );
}

/*
===============
idEFXFile::~idEFXFile
===============
*/
idEFXFile::~idEFXFile( void ) {
	Clear();
}

/*
===============
idEFXFile::FindEffect
===============
*/
bool idEFXFile::FindEffect( idStr &name, ALuint *effect ) {
	int i;

	for ( i = 0; i < effects.Num(); i++ ) {
		if ( effects[i]->name == name ) {
			*effect = effects[i]->effect;
			return true;
		}
	}

	return false;
}

/*
===============
idEFXFile::ReadEffect
===============
*/
#define efxi(param, value)													\
	do {																	\
		ALint _v = value;													\
		EFXprintf("alEffecti(" #param ", %d)\n", _v);						\
		soundSystemLocal.alEffecti(effect->effect, param, _v);				\
		err = alGetError();													\
		if (err != AL_NO_ERROR)												\
			common->Warning("alEffecti(" # param ", %d) "					\
							"failed: 0x%x", _v, err);						\
	} while (false)

#define efxf(param, value)													\
	do {																	\
		ALfloat _v = value;													\
		EFXprintf("alEffectf(" #param ", %.3f)\n", _v);						\
		soundSystemLocal.alEffectf(effect->effect, param, _v);				\
		err = alGetError();													\
		if (err != AL_NO_ERROR)												\
			common->Warning("alEffectf(" # param ", %.3f) "					\
							"failed: 0x%x", _v, err);						\
	} while (false)

#define efxfv(param, value0, value1, value2)								\
	do {																	\
		ALfloat _v[3];														\
		_v[0] = value0;														\
		_v[1] = value1;														\
		_v[2] = value2;														\
		EFXprintf("alEffectfv(" #param ", %.3f, %.3f, %.3f)\n",				\
					_v[0], _v[1], _v[2]);									\
		soundSystemLocal.alEffectfv(effect->effect, param, _v);				\
		err = alGetError();													\
		if (err != AL_NO_ERROR)												\
			common->Warning("alEffectfv(" # param ", %.3f, %.3f, %.3f) "	\
							"failed: 0x%x",	_v[0], _v[1], _v[2], err);		\
	} while (false)

bool idEFXFile::ReadEffect( idLexer &src, idSoundEffect *effect ) {
	idToken name, token;

	if ( !src.ReadToken( &token ) )
		return false;

	// reverb effect
	if ( token != "reverb" ) {
		// other effect (not supported at the moment)
		src.Error( "idEFXFile::ReadEffect: Unknown effect definition" );

		return false;
	}

	src.ReadTokenOnLine( &token );
	name = token;

	if ( !src.ReadToken( &token ) )
		return false;

	if ( token != "{" ) {
		src.Error( "idEFXFile::ReadEffect: { not found, found %s", token.c_str() );
		return false;
	}

	ALenum err;
	alGetError();
	EFXprintf("Loading EFX effect '%s' (#%u)\n", name.c_str(), effect->effect);

	do {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "idEFXFile::ReadEffect: EOF without closing brace" );
			return false;
		}

		if ( token == "}" ) {
			effect->name = name;
			break;
		}

		if ( token == "environment" ) {
			// <+KittyCat> the "environment" token should be ignored (efx has nothing equatable to it)
			src.ParseInt();
		} else if ( token == "environment size" ) {
			//
			// The formula for density according to Creative's EFX-Util.lib:
			//    density = clamp(pow(size, 3) / 16, 0, 1)
			//
			float size = src.ParseFloat();
			size = idMath::ClampFloat(0.0f, 1.0f, (size * size * size) / 16.0f);
			efxf(AL_EAXREVERB_DENSITY, size);
		} else if ( token == "environment diffusion" ) {
			efxf(AL_EAXREVERB_DIFFUSION, src.ParseFloat());
		} else if ( token == "room" ) {
			efxf(AL_EAXREVERB_GAIN, mB_to_gain(src.ParseInt(), GAIN));
		} else if ( token == "room hf" ) {
			efxf(AL_EAXREVERB_GAINHF, mB_to_gain(src.ParseInt(), GAINHF));
		} else if ( token == "room lf" ) {
			efxf(AL_EAXREVERB_GAINLF, mB_to_gain(src.ParseInt(), GAINLF));
		} else if ( token == "decay time" ) {
			efxf(AL_EAXREVERB_DECAY_TIME, src.ParseFloat());
		} else if ( token == "decay hf ratio" ) {
			efxf(AL_EAXREVERB_DECAY_HFRATIO, src.ParseFloat());
		} else if ( token == "decay lf ratio" ) {
			efxf(AL_EAXREVERB_DECAY_LFRATIO, src.ParseFloat());
		} else if ( token == "reflections" ) {
			efxf(AL_EAXREVERB_REFLECTIONS_GAIN, mB_to_gain(src.ParseInt(), REFLECTIONS_GAIN));
		} else if ( token == "reflections delay" ) {
			efxf(AL_EAXREVERB_REFLECTIONS_DELAY, src.ParseFloat());
		} else if ( token == "reflections pan" ) {
			efxfv(AL_EAXREVERB_REFLECTIONS_PAN, src.ParseFloat(), src.ParseFloat(), src.ParseFloat());
		} else if ( token == "reverb" ) {
			efxf(AL_EAXREVERB_LATE_REVERB_GAIN, mB_to_gain(src.ParseInt(), LATE_REVERB_GAIN));
		} else if ( token == "reverb delay" ) {
			efxf(AL_EAXREVERB_LATE_REVERB_DELAY, src.ParseFloat());
		} else if ( token == "reverb pan" ) {
			efxfv(AL_EAXREVERB_LATE_REVERB_PAN, src.ParseFloat(), src.ParseFloat(), src.ParseFloat());
		} else if ( token == "echo time" ) {
			efxf(AL_EAXREVERB_ECHO_TIME, src.ParseFloat());
		} else if ( token == "echo depth" ) {
			efxf(AL_EAXREVERB_ECHO_DEPTH, src.ParseFloat());
		} else if ( token == "modulation time" ) {
			efxf(AL_EAXREVERB_MODULATION_TIME, src.ParseFloat());
		} else if ( token == "modulation depth" ) {
			efxf(AL_EAXREVERB_MODULATION_DEPTH, src.ParseFloat());
		} else if ( token == "air absorption hf" ) {
			efxf(AL_EAXREVERB_AIR_ABSORPTION_GAINHF, mB_to_gain(src.ParseFloat(), AIR_ABSORPTION_GAINHF));
		} else if ( token == "hf reference" ) {
			efxf(AL_EAXREVERB_HFREFERENCE, src.ParseFloat());
		} else if ( token == "lf reference" ) {
			efxf(AL_EAXREVERB_LFREFERENCE, src.ParseFloat());
		} else if ( token == "room rolloff factor" ) {
			efxf(AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, src.ParseFloat());
		} else if ( token == "flags" ) {
			src.ReadTokenOnLine( &token );
			unsigned int flags = token.GetUnsignedLongValue();

			efxi(AL_EAXREVERB_DECAY_HFLIMIT, (flags & 0x20) ? AL_TRUE : AL_FALSE);
			// the other SCALE flags have no equivalent in efx
		} else {
			src.ReadTokenOnLine( &token );
			src.Error( "idEFXFile::ReadEffect: Invalid parameter in reverb definition" );
		}
	} while ( 1 );

	return true;
}

/*
===============
idEFXFile::LoadFile
===============
*/
bool idEFXFile::LoadFile( const char *filename, bool OSPath ) {
	idLexer src( LEXFL_NOSTRINGCONCAT );
	idToken token;

	src.LoadFile( filename, OSPath );
	if ( !src.IsLoaded() ) {
		return false;
	}

	if ( !src.ExpectTokenString( "Version" ) ) {
		return false;
	}

	if ( src.ParseInt() != 1 ) {
		src.Error( "idEFXFile::LoadFile: Unknown file version" );
		return false;
	}

	while ( !src.EndOfFile() ) {
		idSoundEffect *effect = new idSoundEffect;

		if (!effect->alloc()) {
			delete effect;
			Clear();
			return false;
		}

		if (ReadEffect(src, effect))
			effects.Append(effect);
		else
			delete effect;
	};

	return true;
}


/*
===============
idEFXFile::UnloadFile
===============
*/
void idEFXFile::UnloadFile(void)
{
	Clear();
}

#else
/*
===============
idEFXFile::idEFXFile
===============
*/
idEFXFile::idEFXFile(void) { }

/*
===============
idEFXFile::Clear
===============
*/
void idEFXFile::Clear(void)
{
	effects.DeleteContents(true);
}

/*
===============
idEFXFile::~idEFXFile
===============
*/
idEFXFile::~idEFXFile(void)
{
	Clear();
}

/*
===============
idEFXFile::FindEffect
===============
*/
bool idEFXFile::FindEffect(idStr &name, idSoundEffect **effect, int *index)
{
	int i;

	for (i = 0; i < effects.Num(); i++) {
		if ((effects[i]) && (effects[i]->name == name)) {
			*effect = effects[i];
			*index = i;
			return true;
		}
	}

	return false;
}

/*
===============
idEFXFile::ReadEffect
===============
*/
bool idEFXFile::ReadEffect(idLexer &src, idSoundEffect *effect)
{
	idToken name, token;

	if (!src.ReadToken(&token))
		return false;

	// reverb effect
	if (token == "reverb") {
		EAXREVERBPROPERTIES *reverb = (EAXREVERBPROPERTIES *)Mem_Alloc(sizeof(EAXREVERBPROPERTIES));

		if (reverb) {
			src.ReadTokenOnLine(&token);
			name = token;

			if (!src.ReadToken(&token)) {
				Mem_Free(reverb);
				return false;
			}

			if (token != "{") {
				src.Error("idEFXFile::ReadEffect: { not found, found %s", token.c_str());
				Mem_Free(reverb);
				return false;
			}

			do {
				if (!src.ReadToken(&token)) {
					src.Error("idEFXFile::ReadEffect: EOF without closing brace");
					Mem_Free(reverb);
					return false;
				}

				if (token == "}") {
					effect->name = name;
					effect->data = (void *)reverb;
					effect->datasize = sizeof(EAXREVERBPROPERTIES);
					break;
				}

				if (token == "environment") {
					src.ReadTokenOnLine(&token);
					reverb->ulEnvironment = token.GetUnsignedLongValue();
				} else if (token == "environment size") {
					reverb->flEnvironmentSize = src.ParseFloat();
				} else if (token == "environment diffusion") {
					reverb->flEnvironmentDiffusion = src.ParseFloat();
				} else if (token == "room") {
					reverb->lRoom = src.ParseInt();
				} else if (token == "room hf") {
					reverb->lRoomHF = src.ParseInt();
				} else if (token == "room lf") {
					reverb->lRoomLF = src.ParseInt();
				} else if (token == "decay time") {
					reverb->flDecayTime = src.ParseFloat();
				} else if (token == "decay hf ratio") {
					reverb->flDecayHFRatio = src.ParseFloat();
				} else if (token == "decay lf ratio") {
					reverb->flDecayLFRatio = src.ParseFloat();
				} else if (token == "reflections") {
					reverb->lReflections = src.ParseInt();
				} else if (token == "reflections delay") {
					reverb->flReflectionsDelay = src.ParseFloat();
				} else if (token == "reflections pan") {
					reverb->vReflectionsPan.x = src.ParseFloat();
					reverb->vReflectionsPan.y = src.ParseFloat();
					reverb->vReflectionsPan.z = src.ParseFloat();
				} else if (token == "reverb") {
					reverb->lReverb = src.ParseInt();
				} else if (token == "reverb delay") {
					reverb->flReverbDelay = src.ParseFloat();
				} else if (token == "reverb pan") {
					reverb->vReverbPan.x = src.ParseFloat();
					reverb->vReverbPan.y = src.ParseFloat();
					reverb->vReverbPan.z = src.ParseFloat();
				} else if (token == "echo time") {
					reverb->flEchoTime = src.ParseFloat();
				} else if (token == "echo depth") {
					reverb->flEchoDepth = src.ParseFloat();
				} else if (token == "modulation time") {
					reverb->flModulationTime = src.ParseFloat();
				} else if (token == "modulation depth") {
					reverb->flModulationDepth = src.ParseFloat();
				} else if (token == "air absorption hf") {
					reverb->flAirAbsorptionHF = src.ParseFloat();
				} else if (token == "hf reference") {
					reverb->flHFReference = src.ParseFloat();
				} else if (token == "lf reference") {
					reverb->flLFReference = src.ParseFloat();
				} else if (token == "room rolloff factor") {
					reverb->flRoomRolloffFactor = src.ParseFloat();
				} else if (token == "flags") {
					src.ReadTokenOnLine(&token);
					reverb->ulFlags = token.GetUnsignedLongValue();
				} else {
					src.ReadTokenOnLine(&token);
					src.Error("idEFXFile::ReadEffect: Invalid parameter in reverb definition");
					Mem_Free(reverb);
				}
			} while (1);

			return true;
		}
	} else {
		// other effect (not supported at the moment)
		src.Error("idEFXFile::ReadEffect: Unknown effect definition");
	}

	return false;
}


/*
===============
idEFXFile::LoadFile
===============
*/
bool idEFXFile::LoadFile(const char *filename, bool OSPath)
{
	idLexer src(LEXFL_NOSTRINGCONCAT);
	idToken token;

	src.LoadFile(filename, OSPath);

	if (!src.IsLoaded()) {
		return false;
	}

	if (!src.ExpectTokenString("Version")) {
		return NULL;
	}

	if (src.ParseInt() != 1) {
		src.Error("idEFXFile::LoadFile: Unknown file version");
		return false;
	}

	while (!src.EndOfFile()) {
		idSoundEffect *effect = new idSoundEffect;

		if (ReadEffect(src, effect)) {
			effects.Append(effect);
		}
	};

	return true;
}


/*
===============
idEFXFile::UnloadFile
===============
*/
void idEFXFile::UnloadFile(void)
{
	Clear();
}

const GUID EAXPROPERTYID_EAX40_FXSlot0 =
{
    0xC4D79F1E,
    0xF1AC,
    0x436B,
    {0xA8, 0x1D, 0xA7, 0x38, 0xE7, 0x04, 0x54, 0x69}
};

const GUID EAXPROPERTYID_EAX40_Source =
{
    0x1B86B823,
    0x22DF,
    0x4EAE,
    {0x8B, 0x3C, 0x12, 0x78, 0xCE, 0x54, 0x42, 0x27}
};
#endif
#endif
