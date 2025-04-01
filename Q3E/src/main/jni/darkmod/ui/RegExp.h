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

#ifndef __REGEXP_H__
#define __REGEXP_H__

class idWindow;
class idWinVar;

class idRegister {
public:
						idRegister() = default;
						idRegister( const char *p, int t );

	bool				SetVar( idWinVar *var );

	enum REGTYPE { VEC4 = 0, FLOAT, BOOL, INT, STRING, VEC2, VEC3, RECTANGLE, NUMTYPES } ;
	static int REGCOUNT[NUMTYPES];

	static REGTYPE		RegTypeForVar( idWinVar *var );

	bool				enabled;
	short				type;
	idStr				name;
	int					regCount;
	unsigned short		regs[4];
	idWinVar *			var;

	void				SetToRegs( float *registers );
	void				GetFromRegs( float *registers );
	void				CopyRegs( idRegister *src );
	void				Enable( bool b ) { enabled = b; }
	void				ReadFromDemoFile( idDemoFile *f );
	void				WriteToDemoFile( idDemoFile *f );
	void				WriteToSaveGame( idFile *savefile );
	void				ReadFromSaveGame( idFile *savefile );
};

ID_INLINE idRegister::idRegister( const char *p, int t ) {
	name = p;
	type = t;
	assert( t >= 0 && t < NUMTYPES );
	regCount = REGCOUNT[t];
	enabled = ( type == STRING ) ? false : true;
	var = NULL;
};

ID_INLINE void idRegister::CopyRegs( idRegister *src ) {
	regs[0] = src->regs[0];
	regs[1] = src->regs[1];
	regs[2] = src->regs[2];
	regs[3] = src->regs[3];
}

class idRegisterList {
public:

						idRegisterList();
						~idRegisterList();

	void				AddReg( const char *name, int type, const int *expressions, idWinVar *var);
	void				ParseAndAddReg( const char *name, int type, idParser *src, idWindow *win, idWinVar *var );

	idRegister *		FindReg( const char *name );
	void				SetToRegs( float *registers );
	void				GetFromRegs( float *registers );
	void				Reset();
	void				ReadFromDemoFile( idDemoFile *f );
	void				WriteToDemoFile( idDemoFile *f );
	void				WriteToSaveGame( idFile *savefile );
	void				ReadFromSaveGame( idFile *savefile );

private:
	idList<idRegister*>	regs;
	idHashIndex			regHash;
};

ID_INLINE idRegisterList::idRegisterList() {
	regs.SetGranularity( 4 );
	regHash.SetGranularity( 4 );
	regHash.ClearFree( 32, 4 );
}

ID_INLINE idRegisterList::~idRegisterList() {
}

#endif /* !__REGEXP_H__ */
