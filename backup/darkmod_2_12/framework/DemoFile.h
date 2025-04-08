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

#ifndef __DEMOFILE_H__
#define __DEMOFILE_H__

/*
===============================================================================

	Demo file

===============================================================================
*/

typedef enum {
	DS_FINISHED,
	DS_RENDER,
	DS_SOUND,
	DS_VERSION
} demoSystem_t;

class idDemoFile : public idFile {
public:
					idDemoFile();
	virtual			~idDemoFile() override;

	virtual const char *	GetName( void ) override { return (f?f->GetName():""); }
	virtual const char *	GetFullPath( void ) override { return (f?f->GetFullPath():""); }

	void			SetLog( bool b, const char *p );
	void			Log( const char *p );
	bool			OpenForReading( const char *fileName );
	bool			OpenForWriting( const char *fileName );
	void			Close();

	const char *	ReadHashString();
	void			WriteHashString( const char *str );

	void			ReadDict( idDict &dict );
	void			WriteDict( const idDict &dict );

	virtual int		Read( void *buffer, int len ) override;
	virtual int		Write( const void *buffer, int len ) override;

private:
	static idCompressor *AllocCompressor( int type );

	bool			writing;
	byte *			fileImage;
	idFile *		f;
	idCompressor *	compressor;

	idList<idStr*>	demoStrings;
	idFile *		fLog;
	bool			log;
	idStr			logStr;

	static idCVar	com_logDemos;
	static idCVar	com_compressDemos;
	static idCVar	com_preloadDemos;
};

#endif /* !__DEMOFILE_H__ */
