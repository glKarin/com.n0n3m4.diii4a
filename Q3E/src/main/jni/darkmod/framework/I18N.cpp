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



#include "I18N.h"

// uncomment to have debug printouts
//#define M_DEBUG 1
// uncomment to have each Translate() call printed
//#define T_DEBUG 1

class I18NLocal :
	public I18N
{
public:
	I18NLocal();

	virtual ~I18NLocal() override;

	/**
	* Called by idCommon at game init time.
	*/
	virtual void			Init() override;

	// Called at game shutdown time
	virtual void			Shutdown() override;

	/**
	* Attempt to translate a string template in the form of "#str_12345" into
	* the current user selected language, using the FM specific dict first.
	*/
	virtual const char*		Translate( const idStr &in ) override;
	/**
	* The same, but with a const char*
	*/
	virtual const char*		Translate( const char* in ) override;

	/**
	* Returns the current active language.
	*/
	virtual const idStr&	GetCurrentLanguage() const override;

	/**
	* Returns the path to the fonts for the current active language.
	*/
	virtual const idStr&	GetCurrentFontPath() const override;

	/**
	* Print memory usage info.
    */
	virtual void			Print() const override;

	/**
	* Load a new character mapping based on the new language. Returns the
	* number of characters that should be remapped upon dictionary and
	* readable load time.
	*/
	virtual int				LoadCharacterMapping( idStr& lang ) override;

	/**
	* Set a new laguage (example: "english").
	*/
	virtual bool			SetLanguage( const char* lang, bool firstTime = false ) override;

	/**
	* Given an English string like "Maps", returns the "#str_xxxxx" template
	* string that would result back in "Maps" under English. Can be used to
	* make translation work even for hard-coded English strings.
	*/
	virtual const char*		TemplateFromEnglish( const char* in) override;
	virtual const char*		TemplateFromEnglish( const idStr &in) override;

	/**
	* Changes the given string from "A Little House" to "Little House, A",
	* supporting multiple languages like English, German, French etc.
	*/
	virtual void			MoveArticlesToBack(const idStr& title, idStr& prefix, idStr& suffix) override; // grayman #3110

private:
	// current language
	idStr				m_lang;
	
	// current font path (depends on language)
	idStr				m_fontPath;

	// depending on current language, move articles to back of Fm name for display?
	bool				m_bMoveArticles;

	// A dictionary consisting of the current language + the current FM dict.
	idLangDict			m_Dict;

	// reverse dictionary for TemplateFromEnglish
	idDict				m_ReverseDict;
	// dictionary to map "A ..." to "..., A" for MoveArticlesToBack()
	idDict				m_ArticlesDict;

	// A table remapping between characters. The string contains two bytes
	// for each remapped character, Length()/2 is the count.
	idStr				m_Remap;
};

I18NLocal	i18nLocal;
I18N*		i18n = &i18nLocal;

/*
===============
I18NLocal::I18NLocal
===============
*/
I18NLocal::I18NLocal()
{}

I18NLocal::~I18NLocal()
{}

/*
===============
I18NLocal::Init
===============
*/
void I18NLocal::Init()
{
	// some default values before we initialize everything in SetLanguage()
	m_lang = cvarSystem->GetCVarString( "sys_lang" );
	m_fontPath = "fonts/english";
	m_bMoveArticles = (m_lang != "polish" && m_lang != "italian") ? true : false;

	m_Dict.Clear();

	// build the reverse dictionary for TemplateFromEnglish
	// TODO: Do this by looking them up in lang/english.lang?
	// inventory categories
	m_ReverseDict.Set( "Lockpicks",	"#str_02389" );
	m_ReverseDict.Set( "Maps", 		"#str_02390" );
	m_ReverseDict.Set( "Readables",	"#str_02391" );
	m_ReverseDict.Set( "Keys",		"#str_02392" );
	m_ReverseDict.Set( "Potions",	"#str_02393" );
	m_ReverseDict.Set( "Tools",		"#str_02394" );
	// Do not add "Weapons", these do not have an inv_category (as it is invisible anyway). FIXME later?
	// m_ReverseDict.Set( "Weapons",		"#str_02411" );

	// inventory item names used in keybindings
	m_ReverseDict.Set( "Mine",			"#str_02202" );
	m_ReverseDict.Set( "Lantern",		"#str_02395" );
	m_ReverseDict.Set( "Spyglass",		"#str_02396" );
	m_ReverseDict.Set( "Compass",		"#str_02397" );
	m_ReverseDict.Set( "Health Potion",	"#str_02398" );
	m_ReverseDict.Set( "Breath Potion",	"#str_02399" );
	m_ReverseDict.Set( "Holy Water",	"#str_02400" );
	m_ReverseDict.Set( "Corpse",		"#str_02409" );
	m_ReverseDict.Set( "Body",			"#str_02410" );
	m_ReverseDict.Set( "Flashbomb",		"#str_02438" );
	m_ReverseDict.Set( "Flashmine",		"#str_02439" );
	m_ReverseDict.Set( "Explosive Mine","#str_02440" );
	
	// difficulty names
	m_ReverseDict.Set( "Easy",				"#str_03000" );
	m_ReverseDict.Set( "Casual",			"#str_03001" );
	m_ReverseDict.Set( "Novice",			"#str_03002" );
	m_ReverseDict.Set( "Beginner",			"#str_03003" );
	m_ReverseDict.Set( "Medium",			"#str_03004" );
	m_ReverseDict.Set( "Normal",			"#str_03005" );
	m_ReverseDict.Set( "Challenging",		"#str_03006" );
	m_ReverseDict.Set( "Expert",			"#str_03007" );
	m_ReverseDict.Set( "Master",			"#str_03008" );
	m_ReverseDict.Set( "Veteran",			"#str_03009" );
	m_ReverseDict.Set( "Hardcore",			"#str_03010" );
	m_ReverseDict.Set( "Difficult",			"#str_03011" );
	m_ReverseDict.Set( "Hard",				"#str_03012" );
	m_ReverseDict.Set( "Apprentice",		"#str_03013" );
	m_ReverseDict.Set( "Professional",		"#str_03014" );
	m_ReverseDict.Set( "Braggard",			"#str_03015" );
    // Monster names (also used as difficulty names)
	m_ReverseDict.Set( "Ghost",				"#str_08003" );
	m_ReverseDict.Set( "Spirit",			"#str_08009" );
	m_ReverseDict.Set( "Zombie",			"#str_08015" );
	m_ReverseDict.Set( "Ghoul",				"#str_08016" );
	m_ReverseDict.Set( "Wraith",			"#str_08017" );
	m_ReverseDict.Set( "Werebeast",			"#str_08018" );
    // Other names (also used as difficulty names)
	m_ReverseDict.Set( "Rogue",				"#str_08336" );
	m_ReverseDict.Set( "Thief",				"#str_08340" );
	m_ReverseDict.Set( "Thug",				"#str_08341" );
	m_ReverseDict.Set( "Smuggler",			"#str_08352" );
	m_ReverseDict.Set( "Shadow",			"#str_08353" );
	m_ReverseDict.Set( "Swindler",			"#str_08354" );
	m_ReverseDict.Set( "Taffer",			"#str_08355" );

	// The article prefixes, with the suffix to use instead
	m_ArticlesDict.Set( "A ",	", A" );	// English, Portuguese
	m_ArticlesDict.Set( "An ",	", An" );	// English
	m_ArticlesDict.Set( "Der ",	", Der" );	// German
	m_ArticlesDict.Set( "Die ",	", Die" );	// German
	m_ArticlesDict.Set( "Das ",	", Das" );	// German
	m_ArticlesDict.Set( "De ",	", De" );	// Dutch, Danish
	m_ArticlesDict.Set( "El ",	", El" );	// Spanish
	m_ArticlesDict.Set( "Het ",	", Het" );	// Dutch
	m_ArticlesDict.Set( "Il ",	", Il" );	// Italian
	m_ArticlesDict.Set( "La ",	", La" );	// French, Italian
	m_ArticlesDict.Set( "Las ",	", Las" );	// Spanish
	m_ArticlesDict.Set( "Le ",	", Le" );	// French
	m_ArticlesDict.Set( "Les ",	", Les" );	// French
	m_ArticlesDict.Set( "Los ",	", Los" );	// Spanish
	m_ArticlesDict.Set( "Os ",	", Os" );	// Portuguese
	m_ArticlesDict.Set( "The ",	", The" );	// English

	m_Remap.Clear();						// by default, no remaps

	// Create the correct dictionary and set fontLang
	SetLanguage( cvarSystem->GetCVarString( "sys_lang" ), true );
}

/*
===============
I18NLocal::Shutdown
===============
*/
void I18NLocal::Shutdown()
{
	common->Printf( "I18NLocal: Shutdown.\n" );
	m_lang = "";
	m_fontPath = "";
	m_ReverseDict.ClearFree();
	m_ArticlesDict.ClearFree();
	m_Dict.Clear();
}

/*
===============
I18NLocal::Print
===============
*/
void I18NLocal::Print() const
{
	common->Printf("I18N: Current language: %s\n", m_lang.c_str() );
	common->Printf("I18N: Current font path: %s\n", m_fontPath.c_str() );
	common->Printf("I18N: Move articles to back: %s\n", m_bMoveArticles ? "Yes" : "No");
	common->Printf(" Main " );
	m_Dict.Print();
	common->Printf(" Reverse dict   : " );
	m_ReverseDict.PrintMemory();
	common->Printf(" Articles dict  : " );
	m_ArticlesDict.PrintMemory();
	common->Printf(" Remapped chars : %i\n", m_Remap.Length() / 2 );
}

/*
===============
I18NLocal::Translate
===============
*/
const char* I18NLocal::Translate( const char* in )
{
#ifdef T_DEBUG
	common->Printf("I18NLocal: Translating '%s'.\n", in == NULL ? "(NULL)" : in);
#endif
	return m_Dict.GetString( in );				// if not found here, do warn
}

/*
===============
I18NLocal::Translate
===============
*/
const char* I18NLocal::Translate( const idStr &in ) {
#ifdef T_DEBUG
	common->Printf("I18NLocal: Translating '%s'.\n", in == NULL ? "(NULL)" : in.c_str());
#endif
	return m_Dict.GetString( in.c_str() );			// if not found here, do warn
}

/*
===============
I18NLocal::TemplateFromEnglish

If the string is not a template, but an English string, returns a template
like "#str_01234" from the input. Works only for a limited number of strings
that appear in the reverse dict and is used mainly to make inventory categories
for entities with hard-coded category names work. Returns the original string
if it cannot be found in the reverse dict.
===============
*/
const char* I18NLocal::TemplateFromEnglish( const char* in ) {
	return m_ReverseDict.GetString( in, in );
}

const char* I18NLocal::TemplateFromEnglish( const idStr &in ) {
	return m_ReverseDict.GetString( in.c_str(), in.c_str() );
}

/*
===============
I18NLocal::GetCurrentLanguage
===============
*/
const idStr& I18NLocal::GetCurrentLanguage() const
{
	return m_lang;
}

/*
===============
I18NLocal::GetCurrentFontPath
===============
*/
const idStr& I18NLocal::GetCurrentFontPath() const
{
	return m_fontPath;
}

/*
===============
I18NLocal::LoadCharacterMapping

Loads the character remap table, defaulting to "default.map" if "LANGUAGE.map"
is not found. This is used to fix bug #2812.
===============
*/
int I18NLocal::LoadCharacterMapping( idStr& lang ) {

	m_Remap.Clear();		// clear the old mapping

	const char *buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	idStr file = "strings/"; file += lang + ".map";
	int len = fileSystem->ReadFile( file, (void**)&buffer );

	// does not exist
	if (len <= 0)
	{
		file = "strings/default.map";
		len = fileSystem->ReadFile( file, (void**)&buffer );
	}
	if (len <= 0)
	{
		common->Printf("I18N: Found no character remapping for %s.\n", lang.c_str() );
		return 0;
	}
	
    src.LoadMemory(buffer, static_cast<int>(strlen(buffer)), file);
	if ( !src.IsLoaded() ) {
		common->Warning("I18N: Cannot load character remapping %s.", file.c_str() );
		return 0;
	}

	idToken tok, tok2;
	// format:
	/*
	{
		"ff"	"b2"			// remap character 0xff to 0xb2
	}
	*/
	
	src.ExpectTokenString( "{" );
	while ( src.ReadToken( &tok ) ) {
		if ( tok == "}" ) {
			break;
		}
		if ( src.ReadToken( &tok2 ) ) {
			if ( tok2 == "}" ) {
				common->Warning("I18N: Expected a byte, but got } while parsing %s", file.c_str() );
				break;
			}
			// add the two numbers
			//	common->Warning("got '%s' '%s'", tok.c_str(), tok2.c_str() );
			m_Remap.Append( (char) tok.GetIntValue() );
			m_Remap.Append( (char) tok2.GetIntValue() );
//			common->Printf("I18N: Mapping %i (0x%02x) to %i (0x%02x)\n", tok.GetIntValue(), tok.GetIntValue(), tok2.GetIntValue(), tok2.GetIntValue() );
		}
	}

	common->Printf("I18N: Loaded %i character remapping entries.\n", m_Remap.Length() / 2 );

	return m_Remap.Length() / 2;
}

/*
===============
I18NLocal::SetLanguage

Change the language. Does not check the language here, as to not restrict
ourselves to a limited support of languages. Returns true if the language
changed, false if it was not modified.
===============
*/
bool I18NLocal::SetLanguage( const char* lang, bool firstTime ) {
	if (lang == NULL)
	{
		return false;
	}
	common->Printf("I18N: SetLanguage: '%s'.\n", lang);
#ifdef M_DEBUG
#endif

	// store the new setting
	idStr oldLang = m_lang; 
	m_lang = lang;

	// set sys_lang
	cvarSystem->SetCVarString("sys_lang", lang);
	m_bMoveArticles = (m_lang != "polish" && m_lang != "italian") ? true : false;

	// If we need to remap some characters upon loading one of these languages:
	LoadCharacterMapping(m_lang);

	// build our combined dictionary, first the TDM base dict
	idStr file = "strings/"; file += m_lang + ".lang";
	m_Dict.Load( file, true, m_Remap.Length() / 2, m_Remap.c_str() );				// true => clear before load

	idLangDict* fmDict = new idLangDict;
	file = "strings/fm/"; file += m_lang + ".lang";

	if (!fmDict->Load(file, false, m_Remap.Length() / 2, m_Remap.c_str()))
	{
		common->Printf("I18N: '%s' not found.\n", file.c_str() );
	}
	else
	{
		// else fold the newly loaded strings into the system dict
		int num = fmDict->GetNumKeyVals( );
		const idLangKeyValue*  kv;
		for (int i = 0; i < num; i++)
		{	
			kv = fmDict->GetKeyVal( i );
			if (kv != NULL)
			{
#ifdef M_DEBUG
				common->Printf("I18NLocal: Folding '%s' ('%s') into main dictionary.\n", kv->key.c_str(), kv->value.c_str() );
#endif
				m_Dict.AddKeyVal( kv->key.c_str(), kv->value.c_str() );
			}
		}
	}

	// With FM strings it can happen that one translation is missing or incomplete,
	// so fall back to the english version by folding these in, too:
	if (m_lang != "english")
	{
		file = "strings/fm/english.lang";
		if (!fmDict->Load(file, true, m_Remap.Length() / 2, m_Remap.c_str()))
		{
			common->Printf("I18NLocal: '%s' not found, skipping it.\n", file.c_str() );
		}
		else
		{
			// else fold the newly loaded strings into the system dict unless they exist already
			int num = fmDict->GetNumKeyVals( );
			const idLangKeyValue*  kv;
			for (int i = 0; i < num; i++)
			{	
				kv = fmDict->GetKeyVal( i );
				if (kv != NULL)
				{
					const char *oldEntry = m_Dict.GetString( kv->key.c_str(), false);
					// if equal, the entry was not found
					if (oldEntry == kv->key.c_str())
					{
#ifdef M_DEBUG
						common->Printf("I18NLocal: Folding '%s' ('%s') into main dictionary as fallback.\n", kv->key.c_str(), kv->value.c_str() );
#endif
						m_Dict.AddKeyVal( kv->key.c_str(), kv->value.c_str() );
					}
				}
			}
		}	
	}

	// Now set the path to where to load fonts from
	m_fontPath = idStr( Translate( "#str_04127" ) );
    if (m_fontPath == "#str_04127")
	{
		// by default it is the language
		m_fontPath = "fonts/" + m_lang;
	}

	idUserInterface *gui = session->GetGui(idSession::gtMainMenu);

	if (gui && !firstTime) {
		// Recreate main menu GUI
		session->ResetMainMenu();
		session->StartMenu();
		gui = session->GetGui(idSession::gtMainMenu);
	}

	if ( gui && (!firstTime) && (oldLang != m_lang && (oldLang == "russian" || m_lang == "russian")))
	{
		// Restarting the game does not really work, the fonts are still broken
		// (for some reason) and if the user was in a game, this would destroy his session.
	    // this does not reload the fonts, either: cmdSystem->BufferCommandText( CMD_EXEC_NOW, "ReloadImages" );

		// So instead just pop-up a message box:
		gui->SetStateBool("MsgBoxVisible", true);

		// TODO: Switching to Russian will show these strings in Russian, but the font is not yet there
		//		 So translate these before loading the new dictionary, and the display them?
		gui->SetStateString("MsgBoxTitle", Translate("#str_02206") );	// Language changed
		gui->SetStateString("MsgBoxText", Translate("#str_02207") );	// You might need to manually restart the game to see the right characters.

		gui->SetStateBool("MsgBoxLeftButtonVisible", false);
		gui->SetStateBool("MsgBoxRightButtonVisible", false);
		gui->SetStateBool("MsgBoxMiddleButtonVisible", true);
		gui->SetStateString("MsgBoxMiddleButtonText", Translate("#str_07188"));

		gui->SetStateString("MsgBoxMiddleButtonCmd", "close_msg_box");
	}

	return (oldLang != m_lang) ? true : false;
}

/*
===============
I18NLocal::MoveArticlesToBack

Changes "A Little House" to "Little House, A", supporting multiple languages
like English, German, French etc.
===============
*/

// grayman #3110 - rewritten to avoid crashes from freeing memory in the caller

void I18NLocal::MoveArticlesToBack(const idStr& title, idStr& prefix, idStr& suffix)
{
	prefix = "";
	suffix = "";

	// Do not move articles if the language is italian or polish:
	if ( !m_bMoveArticles )
	{
		return;
	}

	// find index of first " "
	int spaceIdx = title.Find(' ');
	// no space, nothing to do
	if ( spaceIdx == -1 )
	{
		return;
	}

	idStr prfx = title.Left( spaceIdx + 1 );

	// see if we have prfx in the dictionary
	const char* sfx = m_ArticlesDict.GetString( prfx.c_str(), NULL );

	if ( sfx != NULL )
	{
		prefix = prfx;
		suffix = sfx;
	}

	// return prefix and suffix to caller
	return;
}

