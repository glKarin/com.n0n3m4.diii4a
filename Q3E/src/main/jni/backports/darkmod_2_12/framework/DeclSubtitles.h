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

#pragma once

class idDeclSubtitles;

typedef struct {
	idStr soundSampleName;				// must match idSoundSample::name
	SubtitleLevel verbosityLevel;		// when to show/hide dependong on player settings
	idDeclSubtitles *owner;				// which decl contains this mapping struct

	// (inline)
	idStr inlineText;					// single-text subtitle over whole sample, written in decl
	float inlineDurationExtend;			// subtitle is extended by X seconds after sound ends, X = -1 means "default"

	// (SRT file)
	idStr srtFileName;					// path to .srt file which should be loaded for subtitles
} subtitleMapping_t;

class idDeclSubtitles : public idDecl {
public:

	virtual bool			Parse( const char *text, const int textLength ) override;
	virtual void			FreeData( void ) override;
	virtual size_t			Size( void ) const override;

	const subtitleMapping_t *FindSubtitleForSound( const char *soundName ) const;

private:
	// subtitle definitions given straight in the decl (may reference .srt file)
	idList<subtitleMapping_t> defs;
	// other decls included from this one
	idList<idDeclSubtitles*> includes;
};
