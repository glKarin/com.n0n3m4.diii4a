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


/*
===============
idSoundShader::Init
===============
*/
void idSoundShader::Init( void ) {
	desc = "<no description>";
	errorDuringParse = false;
	numEntries = 0;
	numLeadins = 0;
	leadinVolume = 0;
	altSound = NULL;
}

/*
===============
idSoundShader::idSoundShader
===============
*/
idSoundShader::idSoundShader( void ) {
	Init();
}

/*
===============
idSoundShader::~idSoundShader
===============
*/
idSoundShader::~idSoundShader( void ) {
}

/*
=================
idSoundShader::Size
=================
*/
size_t idSoundShader::Size( void ) const {
	return sizeof( idSoundShader );
}

/*
===============
idSoundShader::idSoundShader::FreeData
===============
*/
void idSoundShader::FreeData() {
	numEntries = 0;
	numLeadins = 0;
}

/*
===================
idSoundShader::SetDefaultText
===================
*/
bool idSoundShader::SetDefaultText( void ) {
	idStr wavname = GetName();

	if (wavname.IcmpPrefix("__testvideo") == 0) {
		//stgatilov #4847: this case is only used for testVideo command
		//see R_TestVideo_f in RenderSystem_init.cpp
		char generated[2048];
		idStr::snPrintf(generated, sizeof(generated),
			"sound %s // IMPLICITLY GENERATED\n"
			"{\n"
			"fromVideo %s\n"
			"}\n"
		, GetName(), wavname.c_str());
		SetText(generated);
		return true;
	}

	wavname.DefaultFileExtension( ".wav" );		// if the name has .ogg in it, that will stay

	// if there exists a wav file with the same name
	if ( 1 ) { //fileSystem->ReadFile( wavname, NULL ) != -1 ) {
		char generated[2048];
		idStr::snPrintf( generated, sizeof( generated ), 
						"sound %s // IMPLICITLY GENERATED\n"
						"{\n"
						"%s\n"
						"}\n", GetName(), wavname.c_str() );
		SetText( generated );
		return true;
	} else {
		return false;
	}
}

/*
===================
DefaultDefinition
===================
*/
const char *idSoundShader::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"_default.wav\n"
		"}";
}

/*
===============
idSoundShader::Parse

  this is called by the declManager
===============
*/
bool idSoundShader::Parse( const char *text, const int textLength ) {
	idLexer	src;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	// deeper functions can set this, which will cause MakeDefault() to be called at the end
	errorDuringParse = false;

	if ( !ParseShader( src ) || errorDuringParse ) {
		MakeDefault();
		return false;
	}
	return true;
}

/*
===============
idSoundShader::ParseShader
===============
*/
bool idSoundShader::ParseShader( idLexer &src ) {
	//stgatilov: if soundCache is not created yet, then shader will be loaded without sound samples
	//and since sound shader is a decl and decls are cached, they will never be loaded in there future either
	//this will results in missing sounds later, when soundCache is finally available
	assert( soundSystemLocal.soundCache || soundSystemLocal.s_noSound.GetBool() );

	memset( &parms, 0, sizeof(parms) );
	parms.minDistance = 1;
	parms.maxDistance = 10;
	parms.volume = 1;
	parms.shakes = 0;
	parms.soundShaderFlags = 0;
	parms.soundClass = 0;

	speakerMask = 0;
	altSound = NULL;

	for( int i = 0; i < SOUND_MAX_LIST_WAVS; i++ ) {
		leadins[i] = NULL;
		entries[i] = NULL;
	}
	numEntries = 0;
	numLeadins = 0;

	int	maxSamples = idSoundSystemLocal::s_maxSoundsPerShader.GetInteger();
	if ( com_makingBuild.GetBool() || maxSamples <= 0 || maxSamples > SOUND_MAX_LIST_WAVS ) {
		maxSamples = SOUND_MAX_LIST_WAVS;
	}

	idToken token;
	while ( 1 ) {
		if ( !src.ExpectAnyToken( &token ) ) {
			return false;
		}
		// end of definition
		else if ( token == "}" ) {
			break;
		}
		// minimum number of sounds
		else if ( !token.Icmp( "minSamples" ) ) {
			maxSamples = idMath::ClampInt( src.ParseInt(), SOUND_MAX_LIST_WAVS, maxSamples );
		}
		// description
		else if ( !token.Icmp( "description" ) ) {
			src.ReadTokenOnLine( &token );
			desc = token.c_str();
		}
		else if ( !token.Icmp( "editor_displayFolder" ) ) {
			// angua: the display folder for sorting the sounds in the editor, this can be ignored here
			src.SkipRestOfLine();
		}
		// mindistance
		else if ( !token.Icmp( "mindistance" ) ) {
			parms.minDistance = src.ParseFloat();
		}
		// maxdistance
		else if ( !token.Icmp( "maxdistance" ) ) {
			parms.maxDistance = src.ParseFloat();
		}
		// shakes screen
		else if ( !token.Icmp( "shakes" ) ) {
			src.ExpectAnyToken( &token );
			if ( token.type == TT_NUMBER ) {
				parms.shakes = token.GetFloatValue();
			} else {
				src.UnreadToken( &token );
				parms.shakes = 1.0f;
			}
		}
		// reverb
		else if ( !token.Icmp( "reverb" ) ) {
			src.ParseFloat();
			if (!src.ExpectTokenString(",")) {
				src.FreeSource();
				return false;
			}
			src.ParseFloat();
			// no longer supported
			src.Warning( "reverb is no longer supported on sound shaders" );
		}
		// volume
		else if ( !token.Icmp( "volume" ) ) {
			parms.volume = idMath::Fmin(1.5f, src.ParseFloat());
		}
		// leadinVolume is used to allow light breaking leadin sounds to be much louder than the broken loop
		else if ( !token.Icmp( "leadinVolume" ) ) {
			leadinVolume = src.ParseFloat();
		}
		// speaker mask
		else if ( !token.Icmp( "mask_center" ) ) {
			speakerMask |= 1<<SPEAKER_CENTER;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_left" ) ) {
			speakerMask |= 1<<SPEAKER_LEFT;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_right" ) ) {
			speakerMask |= 1<<SPEAKER_RIGHT;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_backright" ) ) {
			speakerMask |= 1<<SPEAKER_BACKRIGHT;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_backleft" ) ) {
			speakerMask |= 1<<SPEAKER_BACKLEFT;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_lfe" ) ) {
			speakerMask |= 1<<SPEAKER_LFE;
		}
		// soundClass
		else if ( !token.Icmp( "soundClass" ) ) {
			parms.soundClass = src.ParseInt();
			if ( parms.soundClass < 0 || parms.soundClass >= SOUND_MAX_CLASSES ) {
				src.Warning( "SoundClass out of range" );
				return false;
			}
		}
		// altSound
		else if ( !token.Icmp( "altSound" ) ) {
			if ( !src.ExpectAnyToken( &token ) ) {
				return false;
			}
			altSound = declManager->FindSound( token.c_str() );
		}
		// ordered
		else if ( !token.Icmp( "ordered" ) ) {
			// no longer supported
		}
		// no_dups
		else if ( !token.Icmp( "no_dups" ) ) {
			parms.soundShaderFlags |= SSF_NO_DUPS;
		}
		// no_flicker
		else if ( !token.Icmp( "no_flicker" ) ) {
			parms.soundShaderFlags |= SSF_NO_FLICKER;
		}
		// no_efx
		else if ( !token.Icmp( "no_efx" ) ) {
			parms.soundShaderFlags |= SSF_NO_EFX;
		}
		// plain
		else if ( !token.Icmp( "plain" ) ) {	
			// no longer supported
		}
		// looping
		else if ( !token.Icmp( "looping" ) ) {
			parms.soundShaderFlags |= SSF_LOOPING;
		}
		// no occlusion
		else if ( !token.Icmp( "no_occlusion" ) ) {
			parms.soundShaderFlags |= SSF_NO_OCCLUSION;
		}
		// private
		else if ( !token.Icmp( "private" ) ) {
			parms.soundShaderFlags |= SSF_PRIVATE_SOUND;
		}
		// antiPrivate
		else if ( !token.Icmp( "antiPrivate" ) ) {
			parms.soundShaderFlags |= SSF_ANTI_PRIVATE_SOUND;
		}
		// once
		else if ( !token.Icmp( "playonce" ) ) {
			parms.soundShaderFlags |= SSF_PLAY_ONCE;
		}
		// global
		else if ( !token.Icmp( "global" ) ) {
			parms.soundShaderFlags |= SSF_GLOBAL;
		}
		// unclamped
		else if ( !token.Icmp( "unclamped" ) ) {
			parms.soundShaderFlags |= SSF_UNCLAMPED;
		}
		// omnidirectional
		else if ( !token.Icmp( "omnidirectional" ) ) {
			parms.soundShaderFlags |= SSF_OMNIDIRECTIONAL;
		}

		// the wave files
		else if ( !token.Icmp( "leadin" ) ) {
			// add to the leadin list
			if ( !src.ReadToken( &token ) ) {
				src.Warning( "Expected sound after leadin" );
				return false;
			}
			if ( soundSystemLocal.soundCache && numLeadins < maxSamples ) {
				leadins[ numLeadins ] = soundSystemLocal.soundCache->FindSound( token.c_str() );
				numLeadins++;
			}
		} else if ( !token.Icmp( "fromVideo" ) ) {
			//#4534: instead of specifying a sound sample file,
			//the name of a material may be specified with "fromVideo" keyword
			//this material must have cinematic in it, declared with audio enabled
			//then the sound stream from the cinematic's video would be used as base sound sample for the shader
			if ( !src.ReadToken( &token ) ) {
				src.Warning( "Expected material name after fromVideo" );
				return false;
			}
			token.BackSlashesToSlashes();
			if ( soundSystemLocal.soundCache ) {
				idStr soundName = "fromVideo " + token;
				entries[ numEntries ] = soundSystemLocal.soundCache->FindSound( soundName.c_str() );
				numEntries++;
			}
		} else if ( token.Find( ".wav", false ) != -1 || token.Find( ".ogg", false ) != -1 ) {
			// add to the wav list
			if ( soundSystemLocal.soundCache && numEntries < maxSamples ) {
				token.BackSlashesToSlashes();
				idStr lang = cvarSystem->GetCVarString( "sys_lang" );
				if ( lang.Icmp( "english" ) != 0 && token.Find( "sound/vo/", false ) >= 0 ) {
					idStr work = token;
					work.ToLower();
					work.StripLeading( "sound/vo/" );
					work = va( "sound/vo/%s/%s", lang.c_str(), work.c_str() );
					if ( fileSystem->ReadFile( work, NULL, NULL ) > 0 ) {
						token = work;
					} else {
						// also try to find it with the .ogg extension
						work.SetFileExtension( ".ogg" );
						if ( fileSystem->ReadFile( work, NULL, NULL ) > 0 ) {
							token = work;
						}
					}
				} 					
				entries[ numEntries ] = soundSystemLocal.soundCache->FindSound( token.c_str() );
				numEntries++;
			}
		} else {
			src.Warning( "unknown token '%s'", token.c_str() );
			return false;
		}
	}

	if ( parms.shakes > 0.0f ) {
		CheckShakesAndOgg();
	}

	return true;
}

/*
===============
idSoundShader::CheckShakesAndOgg
===============
*/
bool idSoundShader::CheckShakesAndOgg( void ) const {
	int i;
	bool ret = false;

	for ( i = 0; i < numLeadins; i++ ) {
		if ( leadins[ i ]->objectInfo.wFormatTag == WAVE_FORMAT_TAG_OGG ) {
			common->Warning( "sound shader '%s' has shakes and uses OGG file '%s'",
								GetName(), leadins[ i ]->name.c_str() );
			ret = true;
		}
	}
	for ( i = 0; i < numEntries; i++ ) {
		if ( entries[ i ]->objectInfo.wFormatTag == WAVE_FORMAT_TAG_OGG ) {
			common->Warning( "sound shader '%s' has shakes and uses OGG file '%s'",
								GetName(), entries[ i ]->name.c_str() );
			ret = true;
		}
	}
	return ret;
}

/*
===============
idSoundShader::List
===============
*/
void idSoundShader::List() const {
	idStrList	shaders;

	common->Printf( "%4i: %s\n", Index(), GetName() );
	if ( idStr::Icmp( GetDescription(), "<no description>" ) != 0 ) {
		common->Printf( "      description: %s\n", GetDescription() );
	}
	for( int k = 0; k < numLeadins ; k++ ) {
		const idSoundSample *objectp = leadins[k];
		if ( objectp ) {
			common->Printf( "      %5dms %4dKb %s (LEADIN)\n", soundSystemLocal.SamplesToMilliseconds(objectp->DurationIn44kHzSamples()), (objectp->objectMemSize/1024)
				,objectp->name.c_str() );
		}
	}
	for( int k = 0; k < numEntries; k++ ) {
		const idSoundSample *objectp = entries[k];
		if ( objectp ) {
			common->Printf( "      %5dms %4dKb %s\n", soundSystemLocal.SamplesToMilliseconds(objectp->DurationIn44kHzSamples()), (objectp->objectMemSize/1024)
				,objectp->name.c_str() );
		}
	}
}

/*
===============
idSoundShader::GetAltSound
===============
*/
const idSoundShader *idSoundShader::GetAltSound( void ) const {
	return altSound;
}

/*
===============
idSoundShader::GetMinDistance
===============
*/
float idSoundShader::GetMinDistance() const {
	return parms.minDistance;
}

/*
===============
idSoundShader::GetMaxDistance
===============
*/
float idSoundShader::GetMaxDistance() const {
	return parms.maxDistance;
}

/*
===============
idSoundShader::GetDescription
===============
*/
const char *idSoundShader::GetDescription() const {
	return desc;
}

/*
===============
idSoundShader::HasDefaultSound
===============
*/
bool idSoundShader::HasDefaultSound() const {
	for ( int i = 0; i < numEntries; i++ ) {
		if ( entries[i] && entries[i]->defaultSound ) {
			return true;
		}
	}
	return false;
}

/*
===============
idSoundShader::GetParms
===============
*/
const soundShaderParms_t *idSoundShader::GetParms() const {
	return &parms;
}

/*
===============
idSoundShader::GetNumSounds
===============
*/
int idSoundShader::GetNumSounds() const {
	return numLeadins + numEntries;
}

/*
===============
idSoundShader::GetSound
===============
*/
const char *idSoundShader::GetSound( int index ) const {
	if ( index >= 0 ) {
		if ( index < numLeadins ) {
			return leadins[index]->name.c_str();
		}
		index -= numLeadins;
		if ( index < numEntries ) {
			return entries[index]->name.c_str();
		}
	}
	return "";
}
