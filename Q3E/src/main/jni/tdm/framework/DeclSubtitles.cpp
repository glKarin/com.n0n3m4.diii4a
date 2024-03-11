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
#include "DeclSubtitles.h"


bool idDeclSubtitles::Parse( const char *text, const int textLength ) {
	idLexer src;
	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS & ~LEXFL_NOSTRINGESCAPECHARS );
	src.SkipUntilString( "{" );

	SubtitleLevel verbosity = SUBL_MISSING;

	idToken	tokenCommand, tokenSound, tokenValue;
	while ( src.ReadToken( &tokenCommand ) ) {
		if ( !tokenCommand.Icmp("}") ) {
			break;

		} else if ( !tokenCommand.Icmp("verbosity") ) {
			if ( !src.ReadToken( &tokenValue ) ) {
				src.Warning( "Missing subtitle verbosity level" );
				return false;
			}

			if ( !tokenValue.Icmp("story") ) {
				verbosity = SUBL_STORY;
			} else if ( !tokenValue.Icmp("speech") ) {
				verbosity = SUBL_SPEECH;
			} else if ( !tokenValue.Icmp("effect") ) {
				verbosity = SUBL_EFFECT;
			} else {
				src.Warning( "Subtitle verbosity must be one of: effect, speech, story" );
				return false;
			}

		} else if ( !tokenCommand.Icmp( "inline" ) || !tokenCommand.Icmp( "srt" ) ) {
			if ( !src.ReadToken( &tokenSound ) ) {
				src.Warning( "Missing sound sample name" );
				return false;
			}
			if ( verbosity == SUBL_MISSING ) {
				src.Warning( "Verbosity level not set for subtitle" );
				return false;
			}
			if ( !src.ReadToken( &tokenValue ) ) {
				src.Warning( "Missing subtitle value" );
				return false;
			}

			subtitleMapping_t mapping;
			mapping.owner = this;
			mapping.soundSampleName = tokenSound;
			mapping.verbosityLevel = verbosity;
			bool isInline = false;
			if ( !tokenCommand.Icmp( "inline" ) ) {
				mapping.inlineText = tokenValue;
				isInline = true;
			} else if ( !tokenCommand.Icmp( "srt" ) ) {
				mapping.srtFileName = tokenValue;
			}
			mapping.inlineDurationExtend = -1.0f;

			// parse optional key arguments
			idToken tokenOption;
			while ( src.CheckTokenString( "-" ) ) {
				src.ReadToken( &tokenOption );
				if ( isInline && ( !tokenOption.Icmp( "dx" ) || !tokenOption.Icmp( "durationExtend" ) ) ) {
					mapping.inlineDurationExtend = src.ParseFloat();
					if ( mapping.inlineDurationExtend < 0.0f) {
						src.Warning( "durationExtend value can't be negative (%f)", mapping.inlineDurationExtend );
						return false;
					}
				}
				else {
					src.Warning( "Unknown option -%s on %s subtitle", tokenOption.c_str(), tokenCommand.c_str() );
					return false;
				}
			}

			defs.Append( mapping );

		} else if ( !tokenCommand.Icmp( "include" ) ) {
			if ( !src.ReadToken( &tokenValue ) ) {
				src.Warning( "Missing name of included subtitle decl" );
				return false;
			}

			const idDecl *decl = declManager->FindType( DECL_SUBTITLES, tokenValue.c_str() );
			includes.Append( (idDeclSubtitles*)decl );

		} else {
			src.Warning( "Unknown command '%s'", tokenCommand.c_str() );
			return false;
		}
	}

	return true;
}

static size_t SizeOfMapping( const subtitleMapping_t &mapping ) {
	return (
		sizeof(subtitleMapping_t) + 
		mapping.soundSampleName.Allocated() +
		mapping.inlineText.Allocated() + 
		mapping.srtFileName.Allocated()
	);
}

size_t idDeclSubtitles::Size( void ) const {
	size_t total = 0;
	for (int i = 0; i < defs.Num(); i++)
		total += SizeOfMapping(defs[i]);
	total += includes.MemoryUsed();
	return total;
}

void idDeclSubtitles::FreeData( void ) {
	defs.ClearFree();
	includes.ClearFree();
}

const subtitleMapping_t *idDeclSubtitles::FindSubtitleForSound( const char *soundName ) const {
	for (int i = 0; i < defs.Num(); i++)
		if (defs[i].soundSampleName.Icmp(soundName) == 0)
			return &defs[i];
	for (int i = 0; i < includes.Num(); i++)
		if (const subtitleMapping_t *res = includes[i]->FindSubtitleForSound(soundName))
			return res;
	return nullptr;
}
