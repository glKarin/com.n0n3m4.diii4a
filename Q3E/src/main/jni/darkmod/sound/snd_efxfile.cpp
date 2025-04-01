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
#include "precompiled.h"
#pragma hdrstop



#include "snd_local.h"
#include "../idlib/math/Math.h"
#include "snd_efxpresets.h"

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
	isAfterReload = false;
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
bool idEFXFile::FindEffect(idStr &name, ALuint *effect) {
	int i;

	for (i = 0; i < effects.Num(); i++) {
		if (effects[i]->name.Icmp(name) == 0) {
			*effect = effects[i]->effect;
			return true;
		}
	}

	return false;
}

/*
===============
idEFXFile::GetEffect
===============
*/
bool idEFXFile::GetEffect(idStr& name, idSoundEffect *soundEffect) {
	int i;

	for (i = 0; i < effects.Num(); i++) {
		if (effects[i]->name.Icmp(name) == 0) {
			*soundEffect = *effects[i];
			return true;
		}
	}

	return false;
}

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

/*
===============
idEFXFile::ReadEffectLegacy

Reads EAX effect in Doom 3 original format.
It is then converted to EFX parameters.
===============
*/
bool idEFXFile::ReadEffectLegacy(idLexer &src, idSoundEffect *effect) {
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
	common->Printf("Loading EAX effect '%s' (#%u)\n", name.c_str(), effect->effect);

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
			float size = src.ParseFloat();
			efxf(AL_EAXREVERB_DENSITY, (size < 2.0f) ? (size - 1.0f) : 1.0f);
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
			unsigned int flags = token.GetUnsignedIntValue();

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
idEFXFile::AddOrUpdatePreset

Checks the internal list of location:sound effect mappings to see if a mapping exists for this location.
If it doesn't exist, create a new object and add to the list.
If it does exist, use the existing sound effect mapping.
In both cases, the EFX database gets updated with the current preset for the location
===============
*/
bool idEFXFile::AddOrUpdatePreset(idStr areaName, idStr efxPreset, ALuint* effect) {

	idSoundEffect* soundEffect = new idSoundEffect;

	const bool found = GetEffect(areaName, soundEffect);
	ALenum err{};
	bool ok;

	if (!found) {

		if (!soundEffect->alloc()) {
			delete soundEffect;
			Clear();
			return false;
		}

		soundEffect->name = areaName;
		effects.Append(soundEffect);
	}

	// update EFX database
	ok = AddPreset(efxPreset, soundEffect, err);

	if (!ok) {
		return false;
	}

	// update effect - this is what's checked for change later
	*effect = soundEffect->effect;

	return true;
}

bool idEFXFile::AddPreset(idStr preset, idSoundEffect* effect, ALenum err) {

	const EFXEAXREVERBPROPERTIES* props = NULL;
	int k = 0;
	for (k = 0; efxPresets[k].name[0]; k++)
		if (efxPresets[k].name == preset) {
			props = &efxPresets[k].props;
			break;
		}

	// Reference the preset by index instead of name.
	if (!props && idStr::IsNumeric(preset)) {
		int idx = atoi(preset);
		if (idx >= 0 && idx < k)
			props = &efxPresets[idx].props;
	}
	if (!props) {
		//src.Error("idEFXFile::ReadEffect: Unknown preset name %s", token.c_str());
		return false;
	}

	efxf(AL_EAXREVERB_DENSITY, props->flDensity);
	efxf(AL_EAXREVERB_DIFFUSION, props->flDiffusion);
	efxf(AL_EAXREVERB_GAIN, props->flGain);
	efxf(AL_EAXREVERB_GAINHF, props->flGainHF);
	efxf(AL_EAXREVERB_GAINLF, props->flGainLF);
	efxf(AL_EAXREVERB_DECAY_TIME, props->flDecayTime);
	efxf(AL_EAXREVERB_DECAY_HFRATIO, props->flDecayHFRatio);
	efxf(AL_EAXREVERB_DECAY_LFRATIO, props->flDecayLFRatio);
	efxf(AL_EAXREVERB_REFLECTIONS_GAIN, props->flReflectionsGain);
	efxf(AL_EAXREVERB_REFLECTIONS_DELAY, props->flReflectionsDelay);
	efxfv(AL_EAXREVERB_REFLECTIONS_PAN, props->flReflectionsPan[0], props->flReflectionsPan[1], props->flReflectionsPan[2]);
	efxf(AL_EAXREVERB_LATE_REVERB_GAIN, props->flLateReverbGain);
	efxf(AL_EAXREVERB_LATE_REVERB_DELAY, props->flLateReverbDelay);
	efxfv(AL_EAXREVERB_LATE_REVERB_PAN, props->flLateReverbPan[0], props->flLateReverbPan[1], props->flLateReverbPan[2]);
	efxf(AL_EAXREVERB_ECHO_TIME, props->flEchoTime);
	efxf(AL_EAXREVERB_ECHO_DEPTH, props->flEchoDepth);
	efxf(AL_EAXREVERB_MODULATION_TIME, props->flModulationTime);
	efxf(AL_EAXREVERB_MODULATION_DEPTH, props->flModulationDepth);
	efxf(AL_EAXREVERB_AIR_ABSORPTION_GAINHF, props->flAirAbsorptionGainHF);
	efxf(AL_EAXREVERB_HFREFERENCE, props->flHFReference);
	efxf(AL_EAXREVERB_LFREFERENCE, props->flLFReference);
	efxf(AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, props->flRoomRolloffFactor);
	efxi(AL_EAXREVERB_DECAY_HFLIMIT, props->iDecayHFLimit);

	return true;

}

/*
===============
idEFXFile::ReadEffectOpenAL
===============
*/
bool idEFXFile::ReadEffectOpenAL(idLexer &src, idSoundEffect *effect) {
	idToken token;

	if (!src.ReadToken(&token))
		return false;	// no more effects
	src.UnreadToken(&token);

	//location name
	if (!src.ExpectTokenString("eaxreverb"))
		return false;
	src.ReadTokenOnLine(&token);
	idStr name = token;
	if (!src.ExpectTokenString("{"))
		return false;

	ALenum err{};
	alGetError();
	common->Printf("Loading EFX effect for location '%s' (#%u)\n", name.c_str(), effect->effect);

	while (1) {
		if (!src.ReadToken(&token)) {
			src.Error( "idEFXFile::ReadEffect: EOF without closing brace" );
			return false;
		}
		if ( token == "}" ) {
			effect->name = name;
			break;
		}

		token.ToUpper();

		if ( token == "PRESET" ) {
			if (!src.ExpectAnyToken(&token))
				return false;
			token.ToUpper();

			if (!AddPreset(token, effect, err)) {
				return false;
			}
		} else if ( token == "DENSITY" ) {
			efxf(AL_EAXREVERB_DENSITY, src.ParseFloat());
		} else if ( token == "DIFFUSION" ) {
			efxf(AL_EAXREVERB_DIFFUSION, src.ParseFloat());
		} else if ( token == "GAIN" ) {
			efxf(AL_EAXREVERB_GAIN, src.ParseFloat());
		} else if ( token == "GAINHF" ) {
			efxf(AL_EAXREVERB_GAINHF, src.ParseFloat());
		} else if ( token == "GAINLF" ) {
			efxf(AL_EAXREVERB_GAINLF, src.ParseFloat());
		} else if ( token == "DECAY_TIME" ) {
			efxf(AL_EAXREVERB_DECAY_TIME, src.ParseFloat());
		} else if ( token == "DECAY_HFRATIO" ) {
			efxf(AL_EAXREVERB_DECAY_HFRATIO, src.ParseFloat());
		} else if ( token == "DECAY_LFRATIO" ) {
			efxf(AL_EAXREVERB_DECAY_LFRATIO, src.ParseFloat());
		} else if ( token == "REFLECTIONS_GAIN" ) {
			efxf(AL_EAXREVERB_REFLECTIONS_GAIN, src.ParseFloat());
		} else if ( token == "REFLECTIONS_DELAY" ) {
			efxf(AL_EAXREVERB_REFLECTIONS_DELAY, src.ParseFloat());
		} else if ( token == "REFLECTIONS_PAN" ) {
			float x = src.ParseFloat(), y = src.ParseFloat(), z = src.ParseFloat();
			efxfv(AL_EAXREVERB_REFLECTIONS_PAN, x, y, z);
		} else if ( token == "LATE_REVERB_GAIN" ) {
			efxf(AL_EAXREVERB_LATE_REVERB_GAIN, src.ParseFloat());
		} else if ( token == "LATE_REVERB_DELAY" ) {
			efxf(AL_EAXREVERB_LATE_REVERB_DELAY, src.ParseFloat());
		} else if ( token == "LATE_REVERB_PAN" ) {
			float x = src.ParseFloat(), y = src.ParseFloat(), z = src.ParseFloat();
			efxfv(AL_EAXREVERB_LATE_REVERB_PAN, x, y, z);
		} else if ( token == "ECHO_TIME" ) {
			efxf(AL_EAXREVERB_ECHO_TIME, src.ParseFloat());
		} else if ( token == "ECHO_DEPTH" ) {
			efxf(AL_EAXREVERB_ECHO_DEPTH, src.ParseFloat());
		} else if ( token == "MODULATION_TIME" ) {
			efxf(AL_EAXREVERB_MODULATION_TIME, src.ParseFloat());
		} else if ( token == "MODULATION_DEPTH" ) {
			efxf(AL_EAXREVERB_MODULATION_DEPTH, src.ParseFloat());
		} else if ( token == "AIR_ABSORPTION_GAINHF" ) {
			efxf(AL_EAXREVERB_AIR_ABSORPTION_GAINHF, src.ParseFloat());
		} else if ( token == "HFREFERENCE" ) {
			efxf(AL_EAXREVERB_HFREFERENCE, src.ParseFloat());
		} else if ( token == "LFREFERENCE" ) {
			efxf(AL_EAXREVERB_LFREFERENCE, src.ParseFloat());
		} else if ( token == "ROOM_ROLLOFF_FACTOR" ) {
			efxf(AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, src.ParseFloat());
		} else if ( token == "DECAY_HFLIMIT" || token == "DECAYHF_LIMIT" ) {
			efxi(AL_EAXREVERB_DECAY_HFLIMIT, src.ParseBool());
		} else {
			src.Error("idEFXFile::ReadEffect: Invalid parameter %s in reverb definition", token.c_str());
			src.ParseRestOfLine(token);
		}
	}

	return true;
}

/*
===============
idEFXFile::LoadFile
===============
*/
bool idEFXFile::LoadFile( const char *filename/*, bool OSPath*/ ) {
	idLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOFATALERRORS );
	idToken token;

	efxFilename = filename;
	src.LoadFile( filename/*, OSPath*/ );
	if ( !src.IsLoaded() ) {

		// #6273: Just return true if file doesn't exist, as EFX can now be specified on location entities
		return true;
	}

	if ( !src.ExpectTokenString( "Version" ) ) {
		return false;
	}

	version = src.ParseInt();
	if ( version != 1 && version != 2 ) {
		src.Error( "idEFXFile::LoadFile: Unknown file version %d", version );
		return false;
	}
	
	while (!src.EndOfFile()) {
		idSoundEffect *effect = new idSoundEffect;

		if (!effect->alloc()) {
			delete effect;
			Clear();
			return false;
		}

		bool ok = false;
		if (version == 1)
			ok = ReadEffectLegacy(src, effect);
		else
			ok = ReadEffectOpenAL(src, effect);

		if (!ok) {
			delete effect;
			break;
		}

		effects.Append(effect);
	};

	return true;
}

/*
===============
idEFXFile::Reload
===============
*/
bool idEFXFile::Reload() {
	if (efxFilename.IsEmpty()) {
		common->Warning("EFX file was not loaded, skipping reload");
		return false;
	}
	//drop all idSoundEffect-s, delete all related OpenAL objects
	Clear();
	//mark that we reloaded the EFX file, all effects must be updated
	isAfterReload = true;
	//read all effects from file again
	return LoadFile(efxFilename);
}

/*
===============
idEFXFile::IsAfterReload
===============
*/
bool idEFXFile::IsAfterReload() {
	bool res = isAfterReload;
	isAfterReload = false;
	return res;
}
