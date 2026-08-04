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



#include "RegExp.h"
#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"

int idRegister::REGCOUNT[NUMTYPES] = {4, 1, 1, 1, 0, 2, 3, 4};

/*
====================
idRegister::RegTypeForVar
====================
*/
idRegister::REGTYPE idRegister::RegTypeForVar( idWinVar *var ) {
	if (dynamic_cast<idWinVec4*>(var))
		return VEC4;
	if (dynamic_cast<idWinRectangle*>(var))
		return RECTANGLE;
	if (dynamic_cast<idWinVec2*>(var))
		return VEC2;
	if (dynamic_cast<idWinVec3*>(var))
		return VEC3;
	if (dynamic_cast<idWinFloat*>(var))
		return FLOAT;
	if (dynamic_cast<idWinInt*>(var))
		return INT;
	if (dynamic_cast<idWinBool*>(var))
		return BOOL;
	if (dynamic_cast<idWinStr*>(var))
		return STRING;
	common->FatalError( "idRegister::RegTypeForVar: bad var type" );
	return NUMTYPES;
}

/*
====================
idRegister::SetVar
====================
*/
bool idRegister::SetVar( idWinVar *var ) {
	REGTYPE typeForVar = RegTypeForVar(var);
	if (typeForVar == type) {
		this->var = var;
		return true;
	}
	return false;
}

/*
====================
idRegister::SetToRegs
====================
*/
void idRegister::SetToRegs( float *registers ) {
	int i;
	idVec4 v;
	idVec2 v2;
	idVec3 v3;
	idRectangle rect;

	if ( !enabled || var == NULL || ( var && ( var->GetDict() || !var->GetEval() ) ) ) {
		return;
	}

	switch( type ) {
		case VEC4: {
			v = *static_cast<idWinVec4*>(var);
			break;
		}
		case RECTANGLE: {
			rect = *static_cast<idWinRectangle*>(var);
			v = rect.ToVec4();
			break;
		}
		case VEC2: {
			v2 = *static_cast<idWinVec2*>(var);
			v[0] = v2[0];
			v[1] = v2[1];
			break;
		}
		case VEC3: {
			v3 = *static_cast<idWinVec3*>(var);
			v[0] = v3[0];
			v[1] = v3[1];
			v[2] = v3[2];
			break;
		}
		case FLOAT: {
			v[0] = *static_cast<idWinFloat*>(var);
			break;
		}
		case INT: {
			v[0] = *static_cast<idWinInt*>(var);
			break;
		}
		case BOOL: {
			v[0] = *static_cast<idWinBool*>(var);
			break;
		}
		default: {
			common->FatalError( "idRegister::SetToRegs: bad reg type" );
			break;
		}
	}
	for ( i = 0; i < regCount; i++ ) {
		registers[ regs[ i ] ] = v[i];
	}
}

/*
=================
idRegister::GetFromRegs
=================
*/
void idRegister::GetFromRegs( float *registers ) {
	idVec4 v;
	idRectangle rect;

	if (!enabled || var == NULL || (var && (var->GetDict() || !var->GetEval()))) {
		return;
	}

	for ( int i = 0; i < regCount; i++ ) {
		v[i] = registers[regs[i]];
	}
	
	switch( type ) {
		case VEC4: {
			*static_cast<idWinVec4*>(var) = v;
			break;
		}
		case RECTANGLE: {
			rect.x = v.x;
			rect.y = v.y;
			rect.w = v.z;
			rect.h = v.w;
			*static_cast<idWinRectangle*>(var) = rect;
			break;
		}
		case VEC2: {
			*static_cast<idWinVec2*>(var) = v.ToVec2();
			break;
		}
		case VEC3: {
			*static_cast<idWinVec3*>(var) = v.ToVec3();
			break;
		}
		case FLOAT: {
			*static_cast<idWinFloat*>(var) = v[0];
			break;
		}
		case INT: {
			*static_cast<idWinInt*>(var) = v[0];
			break;
		}
		case BOOL: {
			*static_cast<idWinBool*>(var) = ( v[0] != 0.0f );
			break;
		}
		default: {
			common->FatalError( "idRegister::GetFromRegs: bad reg type" );
			break;
		}
	}
}

/*
=================
idRegister::ReadFromDemoFile
=================
*/
void idRegister::ReadFromDemoFile(idDemoFile *f) {
	f->ReadBool( enabled );
	f->ReadShort( type );
	f->ReadInt( regCount );
	for ( int i = 0; i < 4; i++ )
		f->ReadUnsignedShort( regs[i] );
	name = f->ReadHashString();
}

/*
=================
idRegister::WriteToDemoFile
=================
*/
void idRegister::WriteToDemoFile( idDemoFile *f ) {
	f->WriteBool( enabled );
	f->WriteShort( type );
	f->WriteInt( regCount );
	for (int i = 0; i < 4; i++)
		f->WriteUnsignedShort( regs[i] );
	f->WriteHashString( name );
}

/*
=================
idRegister::WriteToSaveGame
=================
*/
void idRegister::WriteToSaveGame( idFile *savefile ) {
	int len;

	savefile->Write( &enabled, sizeof( enabled ) );
	savefile->Write( &type, sizeof( type ) );
	savefile->Write( &regCount, sizeof( regCount ) );
	savefile->Write( &regs[0], sizeof( regs ) );
	
	len = name.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( name.c_str(), len );

	var->WriteToSaveGame( savefile );
}

/*
================
idRegister::ReadFromSaveGame
================
*/
void idRegister::ReadFromSaveGame( idFile *savefile ) {
	int len;

	savefile->Read( &enabled, sizeof( enabled ) );
	savefile->Read( &type, sizeof( type ) );
	savefile->Read( &regCount, sizeof( regCount ) );
	savefile->Read( &regs[0], sizeof( regs ) );

	savefile->Read( &len, sizeof( len ) );
	name.Fill( ' ', len );
	savefile->Read( &name[0], len );

	var->ReadFromSaveGame( savefile );
}

/*
====================
idRegisterList::AddOrOverwriteReg
====================
*/
void idRegisterList::AddReg( const char *name, int type, const int *expressions, idWinVar *var) {
	
	// check if such register already exists
	idRegister* reg = FindReg( name );
	bool newReg = ( reg == NULL );

	if ( newReg ) {
		// no such register yet: create a new one
		reg = new idRegister( name, type );
	}

	// attach expressions to register
	for ( int i = 0; i < reg->regCount; i++ ) {
		reg->regs[i] = expressions[i];
	}

	// link variable to register
	bool ok = reg->SetVar( var );
	assert(ok);

	if ( newReg ) {
		// add variable to the list
		int index = regs.Append( reg );
		// add to hash table unless it is unnamed
		if ( name ) {
			int hash = regHash.GenerateKey( name, false );
			regHash.Add( hash, index );
		}
	}
}

/*
====================
idRegisterList::AddOrOverwriteReg
====================
*/
void idRegisterList::ParseAndAddReg( const char *name, int type, idParser *src, idWindow *win, idWinVar *var ) {
	// prepare expressions
	int expressions[4] = {0};
	int numRegs = idRegister::REGCOUNT[type];

	if ( type == idRegister::STRING ) {
		// no expression is possible here, just some string constant
		// it can be linked to another variable later in fixup phase
		idToken tok;
		if ( src->ReadToken( &tok ) ) {
			tok = common->Translate( tok );
			var->Init( tok, win );
		}
	} else {
		// this must be several comma-separated components
		for ( int i = 0; i < numRegs; i++ ) {
			expressions[i] = win->ParseExpression(src, NULL);
			if ( i < numRegs-1 ) {
				src->ExpectTokenString(",");
			}
		}
		// stgatilov #5869: warn user if he accidentally wrote more components than necessary
		if ( src->PeekTokenString(",") )
			src->Warning( "Register seems to have more than %d components", numRegs );
	}

	// add/overwrite variable
	AddReg( name, type, expressions, var );
}

/*
====================
idRegisterList::GetFromRegs
====================
*/
void idRegisterList::GetFromRegs(float *registers) {
	for ( int i = 0; i < regs.Num(); i++ ) {
		regs[i]->GetFromRegs( registers );
	}
}

/*
====================
idRegisterList::SetToRegs
====================
*/

void idRegisterList::SetToRegs( float *registers ) {
	int i;
	for ( i = 0; i < regs.Num(); i++ ) {
		regs[i]->SetToRegs( registers );
	}
}

/*
====================
idRegisterList::FindReg
====================
*/
idRegister *idRegisterList::FindReg( const char *name ) {
	if (!name) {
		return NULL;
	}
	int hash = regHash.GenerateKey( name, false );
	for ( int i = regHash.First( hash ); i != -1; i = regHash.Next( i ) ) {
		if ( regs[i]->name.Icmp( name ) == 0 ) {
			return regs[i];
		}
	}
	return NULL;
}

/*
====================
idRegisterList::Reset
====================
*/
void idRegisterList::Reset() {
	regs.DeleteContents( true );
	regHash.ClearFree();
}

/*
====================
idRegisterList::ReadFromSaveGame
====================
*/
void idRegisterList::ReadFromDemoFile(idDemoFile *f) {
	int c;

	f->ReadInt( c );
	regs.DeleteContents( true );
	for ( int i = 0; i < c; i++ ) {
		idRegister *reg = new idRegister;
		reg->ReadFromDemoFile( f );
		regs.Append( reg );
	}
}

/*
====================
idRegisterList::ReadFromSaveGame
====================
*/
void idRegisterList::WriteToDemoFile(idDemoFile *f) {
	int c = regs.Num();

	f->WriteInt( c );
	for ( int i = 0 ; i < c; i++ ) {
		regs[i]->WriteToDemoFile(f);
	}
}

/*
=====================
idRegisterList::WriteToSaveGame
=====================
*/
void idRegisterList::WriteToSaveGame( idFile *savefile ) {
	int i, num;

	num = regs.Num();
	savefile->Write( &num, sizeof( num ) );

	for ( i = 0; i < num; i++ ) {
		regs[i]->WriteToSaveGame( savefile );
	}
}

/*
====================
idRegisterList::ReadFromSaveGame
====================
*/
void idRegisterList::ReadFromSaveGame( idFile *savefile ) {
	int i, num;

	savefile->Read( &num, sizeof( num ) );
	for ( i = 0; i < num; i++ ) {
		regs[i]->ReadFromSaveGame( savefile );
	}
}
