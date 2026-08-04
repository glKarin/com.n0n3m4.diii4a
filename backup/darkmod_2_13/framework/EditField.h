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

#ifndef __EDITFIELD_H__
#define __EDITFIELD_H__

/*
===============================================================================

	Edit field

===============================================================================
*/

const int MAX_EDIT_LINE = 256;

typedef struct autoComplete_s {
	bool			valid;
	int				length;
	char			completionString[MAX_EDIT_LINE];
	char			currentMatch[MAX_EDIT_LINE];
	int				matchCount;
	int				matchIndex;
	int				findMatchIndex;
} autoComplete_t;

class idEditField {
public:
					idEditField();
					~idEditField();

	void			Clear( void );
	void			SetWidthInChars( int w );
	void			SetCursor( int c );
	int				GetCursor( void ) const;
	void			ClearAutoComplete( void );
	int				GetAutoCompleteLength( void ) const;
	void			AutoComplete( void );
	void			CharEvent( int c );
	void			KeyDownEvent( int key );
	void			Paste( void );
	char *			GetBuffer( void );
	void			Draw( int x, int y, int width, bool showCursor, const idMaterial *material );
	void			SetBuffer( const char *buffer );

private:
	int				cursor;
	int				scroll;
	int				widthInChars;
	char			buffer[MAX_EDIT_LINE];
	autoComplete_t	autoComplete;
};

#endif /* !__EDITFIELD_H__ */
