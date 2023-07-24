// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __LOGITECHLCDSYSTEM_H__
#define __LOGITECHLCDSYSTEM_H__

class sdLogitechLCDSystem {
public:
	typedef void*				objectHandle_t;
	enum brushColor_t {
		BC_WHITE,
		BC_BLACK,
	};

	virtual void				Init( void ) = 0;
	virtual void				Shutdown( void ) = 0;
	virtual void				Update( void ) = 0;

	virtual objectHandle_t		GetImageHandle( const char* fileName ) = 0;

	virtual void				DrawImageHandle( objectHandle_t handle, int x, int y ) = 0;
	virtual void				DrawFilledRect( int x, int y, int width, int height, brushColor_t color ) = 0;
	virtual void				DrawText( int x, int y, const wchar_t* text ) = 0;

	virtual void				GetDimensions( int& x, int& y ) = 0;
};

#endif // __LOGITECHLCDSYSTEM_H__