// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __FONTMANAGER_H__
#define __FONTMANAGER_H__

class sdFontManager
{
public:
    virtual void            Init(void) = 0;
    virtual void            Shutdown(void) = 0;

    virtual qhandle_t		FindFont( const char* fontName ) = 0;
    virtual void			FreeFont( const qhandle_t font ) = 0;

    virtual const int		GetFontHeight( const qhandle_t font, const int pointSize ) = 0;

    virtual void			SetFont( const qhandle_t font ) = 0;
    virtual void			SetFontSize( const int pointSize ) = 0;
};

extern sdFontManager* fontManager;

#endif /* !__FONTMANAGER_H__ */
