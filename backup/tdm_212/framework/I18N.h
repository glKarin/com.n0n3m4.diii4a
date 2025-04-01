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

// Copyright (C) 2011-2013 Tels (Donated to The Dark Mod)

#ifndef __DARKMOD_I18N_H__
#define __DARKMOD_I18N_H__

/*
===============================================================================

  I18N (Internationalization) - manages translations of strings, including FM-
  specific translations and secondary dictionaries.

===============================================================================
*/

class I18N
{
public:
	virtual	~I18N() {}

	/**
	* Initialise language and some dictionaries.
	*/
	virtual void				Init() = 0;

	/**
	 * Shutdown the system, free any dictionaries.
	 */
	virtual void				Shutdown() = 0;

	/**
	* Attempt to translate a string template in the form of "#str_12345" into
	* the current user selected language, using the FM specific dict first.
	*/
	virtual const char*			Translate( const idStr& in ) = 0;
	
	/**
	* The same, but with a const char*
	*/
	virtual const char*			Translate( const char* in ) = 0;

	/**
	* Returns the current active language.
	*/
	virtual const idStr&		GetCurrentLanguage() const = 0;

	/**
	* Returns the path to the fonts for the current active language.
	*/
	virtual const idStr&		GetCurrentFontPath() const = 0;

	/**
	* Print memory usage info.
    */
	virtual void				Print() const = 0;

	/**
	* Load a new character mapping based on the new language. Returns the
	* number of characters that should be remapped upon dictionary and
	* readable load time.
	*/
	virtual int					LoadCharacterMapping( idStr& lang ) = 0;

	/**
	* Set a new laguage (example: "english").
	*/
	virtual bool				SetLanguage( const char* lang, bool firstTime = false ) = 0;

	/**
	* Given an English string like "Maps", returns the "#str_xxxxx" template
	* string that would result back in "Maps" under English. Can be used to
	* make translation work even for hard-coded English strings.
	*/
	virtual const char*			TemplateFromEnglish( const char* in) = 0;
	virtual const char*			TemplateFromEnglish( const idStr &in) = 0;

	/**
	* Changes the given string from "A Little House" to "Little House, A",
	* supporting multiple languages like English, German, French etc.
	*/
	virtual void				MoveArticlesToBack(const idStr& title, idStr& prefix, idStr& suffix) = 0; // grayman #3110
};

#endif /* !__DARKMOD_I18N_H__ */

