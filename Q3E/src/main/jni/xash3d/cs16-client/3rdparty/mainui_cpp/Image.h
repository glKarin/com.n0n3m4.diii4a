/*
Image.h -- image class
Copyright (C) 2019 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef IMAGE_H
#define IMAGE_H

#include "enginecallback_menu.h"

class CImage
{
public:
	CImage() : m_szPath( NULL ), m_hPic( 0 ) { }
	CImage( HIMAGE hPic ) : m_szPath( NULL ), m_hPic( hPic ) { }
	CImage( const char *szPath, int flags = 0 )
	{
		Load( szPath, flags );
	}
	CImage( const char *szPath, const byte *rgbdata, int size, int flags = 0 )
	{
		Load( szPath, rgbdata, size, flags );
	}

	void Set( HIMAGE handle )
	{
		m_hPic = handle;
		m_szPath = NULL;
	}

	void Load( const char *szPath, int flags = 0 )
	{
		Set( EngFuncs::PIC_Load( szPath, flags ) );
		m_szPath = szPath;
	}

	void Load( const char *szPath, const byte *rgbdata, int size, int flags = 0 )
	{
		Set( EngFuncs::PIC_Load( szPath, rgbdata, size, flags ));
		m_szPath = szPath;
	}

	void Reset( void )
	{
		m_szPath = NULL;
		m_hPic   = 0;
	}

	// a1ba: why there is no destructor?
	// Engine doesn't track the reference count of texture
	// so unloading texture may behave not as you may expect
	// Moreover, you can't unload texture by it's handle, only by name
	// If you still want to unload image, there is a function for you
	void ForceUnload()
	{
		if( m_szPath )
			EngFuncs::PIC_Free( m_szPath );
		m_hPic = 0;
	}

	const char * operator =( const char *path )
	{
		// don't pass NULL to engine
		if( path ) Load( path );
		else Reset();
		return path;
	}

	HIMAGE operator = ( HIMAGE handle )
	{
		Set( handle );
		return handle;
	}

	operator HIMAGE()
	{
		return m_hPic;
	}

	friend inline bool operator == ( CImage &a, CImage &b )
	{
		return a.m_hPic == b.m_hPic;
	}

	bool IsValid() const     { return m_hPic != 0; }
	const char *Path() const { return m_szPath; }
	HIMAGE Handle() const    { return m_hPic; }
private:
	const char *m_szPath;
	HIMAGE m_hPic;
};

#endif // IMAGE_H
