// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __WLEXER_H__
#define __WLEXER_H__

struct wpunctuation_t {
	wchar_t* p;						// punctuation character(s)
	int n;							// punctuation id
};

class idWLexer {
public:
					idWLexer();
					idWLexer( const wchar_t* ptr, int length, const char* name, int startLine = 1 );
					// destructor
					~idWLexer();

					// load a script from the given memory with the given length and a specified line offset,
					// so source strings extracted from a file can still refer to proper line numbers in the file
					// NOTE: the ptr is expected to point at a valid C string: ptr[length] == L'\0'
	bool			LoadMemory( const wchar_t* ptr, int length, const char* name, int startLine = 1 );
					// free the script
	void			FreeSource( void );
					// returns true if a script is loaded
	bool			IsLoaded( void ) const { return loaded; }
					// read a token
	bool			ReadToken( idWToken* token );
					// expect a certain token, reads the token when available
	bool			ExpectTokenString( const wchar_t* string );
					// expect a token
	bool			ExpectAnyToken( idWToken* token );
					// skip the rest of the current line
	bool			SkipRestOfLine( void );
					// skip the braced section
	bool			SkipBracedSection( bool parseFirstBrace = true );
					// skip the braced section character-by-character
	bool			SkipBracedSectionExact( bool parseFirstBrace = true );
					// skips spaces, tabs, C-like comments etc.
	bool			SkipWhiteSpace( bool currentLine );
					// set an array with punctuations, NULL restores default C/C++ set, see default_punctuations for an example
	void			SetPunctuations( const wpunctuation_t* p );
					// returns true if any errors were printed
	bool			HadError( void ) const;
					// returns true if any warnings were printed
	bool			HadWarning( void ) const;
					// Prints an error message
	void			Error( const char *str, ... );
					
					// Prints a warning message
	void			Warning( const char *str, ... );

					// get offset in script
	int				GetFileOffset( void ) const;

private:

	void			CreatePunctuationTable( const wpunctuation_t* punctuations );
	bool			ReadEscapeCharacter( wchar_t* ch );
	bool			ReadString( idWToken* token, wchar_t quote );
	bool			ReadName( idWToken *token );
	bool			ReadNumber( idWToken *token );
	bool			ReadPunctuation( idWToken* token );
	bool			CheckString( const wchar_t *str ) const;

private:
	bool			loaded;
	idStr			filename;				// file name of the script
	bool			allocated;
	const wchar_t*	buffer;					// buffer containing the script
	const wchar_t*	script_p;				// current pointer in the script
	const wchar_t*	end_p;					// pointer to the end of the script
	const wchar_t*	lastScript_p;			// script pointer before reading token
	const wchar_t*	whiteSpaceStart_p;		// start of last white space
	const wchar_t*	whiteSpaceEnd_p;		// end of last white space
	int				line;					// current line in script
	int				lastline;				// line before reading token
	const wpunctuation_t* punctuations;		// the punctuations used in the script
	int*			punctuationtable;		// ASCII table with punctuations
	int*			nextpunctuation;		// next punctuation in chain
	bool			hadError;				// set by Error, even if the error is supressed
	bool			hadWarning;				// set by Warning, even if the warning is supressed
};

ID_INLINE bool idWLexer::HadError( void ) const {
	return hadError;
}

ID_INLINE bool idWLexer::HadWarning( void ) const {
	return hadWarning;
}

ID_INLINE int idWLexer::GetFileOffset( void ) const {
	return this->script_p - this->buffer;
}

#endif /* __WLEXER_H__ */
