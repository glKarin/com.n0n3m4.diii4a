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



idCVar * idCVar::staticVars = NULL;


/*
===============================================================================

CVar Registration

Each DLL using CVars has to declare a private copy of the static variable
idCVar::staticVars like this: idCVar * idCVar::staticVars = NULL;
Furthermore idCVar::RegisterStaticVars() has to be called after the
cvarSystem pointer is set when the DLL is first initialized.

===============================================================================
*/

#define BAD_CVAR ((idCVar *)(size_t)(-1LL))

void idCVar::Init( const char *name, const char *value, int flags, const char *description,
	float valueMin, float valueMax, const char **valueStrings, argCompletion_t valueCompletion ) {
	this->name = name;
	this->value = value;
	this->flags = flags;
	this->description = description;
	this->flags = flags | CVAR_STATIC;
	this->valueMin = valueMin;
	this->valueMax = valueMax;
	this->valueStrings = valueStrings;
	this->valueCompletion = valueCompletion;
	this->integerValue = 0;
	this->floatValue = 0.0f;
	if ( staticVars == BAD_CVAR ) {
		common->Error( "Late attempt to register cvar %s", name );
	}
	this->next = staticVars;
	staticVars = this;
}

void idCVar::RegisterStaticVars( void ) {
	if ( staticVars != BAD_CVAR ) {
		for ( idCVar *cvar = staticVars; cvar; cvar = cvar->next ) {
			cvarSystem->Register( cvar );
		}
		staticVars = BAD_CVAR;
	}
}

/*
===============================================================================

	idInternalCVar

===============================================================================
*/

/*
============
idInternalCVar::idInternalCVar
============
*/
idCVar *idCVar::InternalCreate( const char *newName, int newFlags ) {
	idCVar *self = new idCVar();
	self->nameString = newName;
	self->name = self->nameString.c_str();
	self->valueString = "";
	self->resetString = "";
	self->missionString = "";
	self->missionOverride = false;
	self->descriptionString = "[created dynamically]";
	self->description = self->descriptionString.c_str();
	self->flags = ( newFlags & ~CVAR_STATIC ) | CVAR_MODIFIED;
	self->valueMin = 1;
	self->valueMax = -1;
	self->valueStrings = NULL;
	self->valueCompletion = 0;
	self->UpdateValue();
	return self;
}

/*
============
idInternalCVar::idInternalCVar
============
*/
void idCVar::InternalRegister( void ) {
	nameString = name;
	name = nameString.c_str();
	valueString = value;
	resetString = value;
	missionString = "";
	missionOverride = false;
	descriptionString = GetDescription();
	description = descriptionString.c_str();
	flags |= CVAR_MODIFIED;
	UpdateValue();
}

/*
============
idInternalCVar::UpdateValue
============
*/
void idCVar::UpdateValue( void ) {
	bool clamped = false;

	idStr *pActiveValue = nullptr;
	if ( missionOverride ) {
		pActiveValue = &missionString;
	} else {
		pActiveValue = &valueString;
	}
	value = pActiveValue->c_str();

	if ( flags & CVAR_BOOL ) {
		integerValue = ( atoi( value ) != 0 );
		floatValue = integerValue;
		if ( idStr::Icmp( value, "0" ) != 0 && idStr::Icmp( value, "1" ) != 0 ) {
			*pActiveValue = idStr( (bool)( integerValue != 0 ) );
			value = pActiveValue->c_str();
		}
	} else if ( flags & CVAR_INTEGER ) {
		integerValue = (int)atoi( value );
		if ( valueMin < valueMax ) {
			if ( integerValue < valueMin ) {
				integerValue = (int)valueMin;
				clamped = true;
			} else if ( integerValue > valueMax ) {
				integerValue = (int)valueMax;
				clamped = true;
			}
		}
		if ( clamped || !idStr::IsNumeric( value ) || idStr::FindChar( value, '.' ) ) {
			*pActiveValue = idStr( integerValue );
			value = pActiveValue->c_str();
		}
		floatValue = (float)integerValue;
	} else if ( flags & CVAR_FLOAT ) {
		floatValue = (float)atof( value );
		if ( valueMin < valueMax ) {
			if ( floatValue < valueMin ) {
				floatValue = valueMin;
				clamped = true;
			} else if ( floatValue > valueMax ) {
				floatValue = valueMax;
				clamped = true;
			}
		}
		if ( clamped || !idStr::IsNumeric( value ) ) {
			*pActiveValue = idStr( floatValue );
			value = pActiveValue->c_str();
		}
		integerValue = (int)floatValue;
	} else {
		if ( valueStrings && valueStrings[0] ) {
			integerValue = 0;
			for ( int i = 0; valueStrings[i]; i++ ) {
				if ( idStr::Icmp( value, valueStrings[i] ) == 0 ) {
					integerValue = i;
					break;
				}
			}
			*pActiveValue = valueStrings[integerValue];
			value = pActiveValue->c_str();
			floatValue = (float)integerValue;
		} else if ( idStr::Length( value ) < 32 ) {
			floatValue = (float)atof( value );
			integerValue = (int)floatValue;
		} else {
			floatValue = 0.0f;
			integerValue = 0;
		}
	}
}

/*
============
idInternalCVar::Set
============
*/
void idCVar::Set( const char *newValue, bool force, bool fromServer, bool mission ) {
	if ( !force ) {
		if ( flags & CVAR_ROM ) {
			common->Printf( "%s is read only.\n", nameString.c_str() );
			return;
		}

		if ( flags & CVAR_INIT ) {
			common->Printf( "%s is write protected.\n", nameString.c_str() );
			return;
		}
	}

	idStr *pModifiedValue = nullptr;
	if ( mission ) {
		if ( !newValue ) {
			// NULL -> unset mission override
			missionOverride = false;
			goto DoUpdate;
		}
		missionOverride = true;
		pModifiedValue = &missionString;
	} else {
		if ( !newValue ) {
			newValue = resetString.c_str();
		}
		missionOverride = false;
		missionString.Clear();
		pModifiedValue = &valueString;
	}

	*pModifiedValue = newValue;
	value = pModifiedValue->c_str();

DoUpdate:
	UpdateValue();

	SetModified();

	extern void SetArchivedCVarModified();
	if ( ( flags & CVAR_ARCHIVE ) && !mission )
		SetArchivedCVarModified();
}

/*
============
idInternalCVar::Reset
============
*/
void idCVar::Reset( void ) {
	valueString = resetString;
	missionOverride = false;
	UpdateValue();
}

/*
============
idInternalCVar::InternalSetString
============
*/
void idCVar::InternalSetString( const char *newValue ) {
	Set( newValue, true, false, false );
}

/*
===============
idInternalCVar::InternalServerSetString
===============
*/
void idCVar::InternalServerSetString( const char *newValue ) {
	Set( newValue, true, true, false );
}

/*
===============
idInternalCVar::InternalMissionSetString
===============
*/
void idCVar::InternalMissionSetString( const char *newValue ) {
	Set( newValue, true, false, true );
}

/*
============
idInternalCVar::InternalSetBool
============
*/
void idCVar::InternalSetBool( const bool newValue ) {
	Set( idStr( newValue ), true, false, false );
}

/*
============
idInternalCVar::InternalSetInteger
============
*/
void idCVar::InternalSetInteger( const int newValue ) {
	Set( idStr( newValue ), true, false, false );
}

/*
============
idInternalCVar::InternalSetFloat
============
*/
void idCVar::InternalSetFloat( const float newValue ) {
	Set( idStr( newValue ), true, false, false );
}

/*
===============================================================================

	idCVarSystemLocal

===============================================================================
*/

class idCVarSystemLocal : public idCVarSystem {
public:
							idCVarSystemLocal( void );

	virtual					~idCVarSystemLocal( void ) override {}

	virtual void			Init( void ) override;
	virtual void			Shutdown( void ) override;
	virtual bool			IsInitialized( void ) const override;

	virtual void			Register( idCVar *cvar ) override;

	virtual idCVar *		Find( const char *name ) override;

	virtual void			SetCVarString( const char *name, const char *value, int flags = 0 ) override;
	virtual void			SetCVarBool( const char *name, const bool value, int flags = 0 ) override;
	virtual void			SetCVarInteger( const char *name, const int value, int flags = 0 ) override;
	virtual void			SetCVarFloat( const char *name, const float value, int flags = 0 ) override;
	virtual void			SetCVarMissionString( const char *name, const char *value ) override;

	virtual const char *	GetCVarString( const char *name ) const override;
	virtual bool			GetCVarBool( const char *name ) const override;
	virtual int				GetCVarInteger( const char *name ) const override;
	virtual float			GetCVarFloat( const char *name ) const override;
	virtual const char *	GetCVarMissionString( const char *name ) const override;

	virtual bool			Command( const idCmdArgs &args ) override;

	virtual void			CommandCompletion( void(*callback)( const char *s ) ) override;
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) override;

	virtual idDict			MoveCVarsToDict( int flags ) const override;
	virtual void			SetCVarsFromDict( const idDict &dict ) override;

	friend void				SetArchivedCVarModified();
	virtual bool			WasArchivedCVarModifiedAfterLastWrite() override;
	virtual void			WriteArchivedCVars( idFile *f ) override;

	virtual idDict			GetMissionOverrides() const override;
	virtual void			SetMissionOverrides( const idDict &dict = {} ) override;

	void					RegisterInternal( idCVar *cvar );
	idCVar *				FindInternal( const char *name ) const;
	void					SetInternal( const char *name, const char *value, int flags, bool mission );

private:
	bool					initialized;

	idList<idCVar*>			cvars;
	idHashIndex				cvarHash;
	mutable idSysMutex		mutex;			// protects list and hash table of cvars, but not contents of cvars

	std::atomic<bool>		wasArchivedCvarModifiedAfterLastWrite;

private:
	static void				Toggle_f( const idCmdArgs &args );
	static void				Set_f( const idCmdArgs &args );
	static void				SetS_f( const idCmdArgs &args );
	static void				SetU_f( const idCmdArgs &args );
	static void				SetT_f( const idCmdArgs &args );
	static void				SetA_f( const idCmdArgs &args );
	static void				Reset_f( const idCmdArgs &args );
	static void				SetM_f( const idCmdArgs &args );
	static void				UnSetM_f( const idCmdArgs &args );
	static void				ListByFlags( const idCmdArgs &args, cvarFlags_t flags );
	static void				List_f( const idCmdArgs &args );
	static void				Restart_f( const idCmdArgs &args );
};

idCVarSystemLocal			localCVarSystem;
idCVarSystem *				cvarSystem = &localCVarSystem;

#define NUM_COLUMNS				77		// 78 - 1
#define NUM_NAME_CHARS			33
#define NUM_DESCRIPTION_CHARS	( NUM_COLUMNS - NUM_NAME_CHARS )
#define FORMAT_STRING			"%-32s "

const char *CreateColumn( const char *text, int columnWidth, const char *indent, idStr &string ) {
	int i, lastLine;

	string.Clear();
	for ( lastLine = i = 0; text[i] != '\0'; i++ ) {
		if ( i - lastLine >= columnWidth || text[i] == '\n' ) {
			while( i > 0 && text[i] > ' ' && text[i] != '/' && text[i] != ',' && text[i] != '\\' ) {
				i--;
			}
			while( lastLine < i ) {
				string.Append( text[lastLine++] );
			}
			string.Append( indent );
			lastLine++;
		}
	}
	while( lastLine < i ) {
		string.Append( text[lastLine++] );
	}
	return string.c_str();
}

/*
============
idCVarSystemLocal::FindInternal
============
*/
idCVar *idCVarSystemLocal::FindInternal( const char *name ) const {
	// note: mutex must be locked outside
	int hash = cvarHash.GenerateKey( name, false );
	for ( int i = cvarHash.First( hash ); i != -1; i = cvarHash.Next( i ) ) {
		if ( cvars[i]->nameString.Icmp( name ) == 0 ) {
			return cvars[i];
		}
	}
	return NULL;
}

/*
============
idCVarSystemLocal::SetInternal
============
*/
void idCVarSystemLocal::SetInternal( const char *name, const char *value, int flags, bool mission ) {
	idScopedCriticalSection lock( mutex );

	idCVar *internal = FindInternal( name );

	if ( !internal ) {
		// cvars can be created dynamically by game scripts or mods, although this is a rarely used feature...
		// also this happens after switching versions/platforms because .cfg files contains unknown cvars
		// perhaps we want to make unknown cvars created via "seta" as archivable?
		internal = idCVar::InternalCreate( name, flags );
		int hash = cvarHash.GenerateKey( internal->nameString.c_str(), false );
		cvarHash.Add( hash, cvars.Append( internal ) );
	}

	// if a cvar is marked read-only and init, it should not be possible to modify 
	// it using commandline arguments
	int cvro = internal->flags & CVAR_ROM;
	int cvinit = internal->flags & CVAR_INIT;
	if ( !( cvro && cvinit ) ) {
		if ( mission ) {
			internal->InternalMissionSetString( value );
		} else {
			internal->InternalSetString( value );
			internal->flags |= flags & ~CVAR_STATIC;
		}
	} else {
		common->Warning("Attempt to modify read-only %s CVAR failed.", name);
	}
}

/*
============
idCVarSystemLocal::idCVarSystemLocal
============
*/
idCVarSystemLocal::idCVarSystemLocal( void ) {
	initialized = false;
}

/*
============
idCVarSystemLocal::Init
============
*/
void idCVarSystemLocal::Init( void ) {

	cmdSystem->AddCommand( "toggle", Toggle_f, CMD_FL_SYSTEM, "toggles a cvar" );
	cmdSystem->AddCommand( "set", Set_f, CMD_FL_SYSTEM, "sets a cvar" );
	cmdSystem->AddCommand( "sets", SetS_f, CMD_FL_SYSTEM, "sets a cvar and flags it as server info" );
	cmdSystem->AddCommand( "setu", SetU_f, CMD_FL_SYSTEM, "sets a cvar and flags it as user info" );
	cmdSystem->AddCommand( "sett", SetT_f, CMD_FL_SYSTEM, "sets a cvar and flags it as tool" );
	cmdSystem->AddCommand( "seta", SetA_f, CMD_FL_SYSTEM, "sets a cvar and flags it as archive" );
	cmdSystem->AddCommand( "setm", SetM_f, CMD_FL_SYSTEM, "sets a mission override value for cvar" );
	cmdSystem->AddCommand( "unsetm", UnSetM_f, CMD_FL_SYSTEM, "unsets a mission override for cvar" );
	cmdSystem->AddCommand( "reset", Reset_f, CMD_FL_SYSTEM, "resets a cvar" );
	cmdSystem->AddCommand( "listCvars", List_f, CMD_FL_SYSTEM, "lists cvars" );
	cmdSystem->AddCommand( "cvar_restart", Restart_f, CMD_FL_SYSTEM, "restart the cvar system" );

	initialized = true;
	wasArchivedCvarModifiedAfterLastWrite.store( false );
}

/*
============
idCVarSystemLocal::Shutdown
============
*/
void idCVarSystemLocal::Shutdown( void ) {
	for ( idCVar *cvar : cvars ) {
		if ( cvar->flags | CVAR_STATIC )
			continue;
		delete cvar;
	}
	cvars.ClearFree();
	cvarHash.ClearFree();
	initialized = false;
}

/*
============
idCVarSystemLocal::IsInitialized
============
*/
bool idCVarSystemLocal::IsInitialized( void ) const {
	return initialized;
}

/*
============
idCVarSystemLocal::Register
============
*/
void idCVarSystemLocal::Register( idCVar *cvar ) {
	assert( idCVar::staticVars != BAD_CVAR );
	idCVar *internal = FindInternal( cvar->GetName() );

	if ( internal ) {
		//internal->Update( cvar );
		common->Error( "Cvar %s registered twice", cvar->GetName() );
	} else {
		cvar->InternalRegister();
		int hash = cvarHash.GenerateKey( cvar->nameString.c_str(), false );
		cvarHash.Add( hash, cvars.Append( cvar ) );
	}
}

/*
============
idCVarSystemLocal::Find
============
*/
idCVar *idCVarSystemLocal::Find( const char *name ) {
	idScopedCriticalSection lock( mutex );
	return FindInternal( name );
}

/*
============
idCVarSystemLocal::SetCVarString
============
*/
void idCVarSystemLocal::SetCVarString( const char *name, const char *value, int flags ) {
	SetInternal( name, value, flags, false );
}

/*
============
idCVarSystemLocal::SetCVarMissionString
============
*/
void idCVarSystemLocal::SetCVarMissionString( const char *name, const char *value ) {
	SetInternal( name, value, 0, true );
}

/*
============
idCVarSystemLocal::SetCVarBool
============
*/
void idCVarSystemLocal::SetCVarBool( const char *name, const bool value, int flags ) {
	SetInternal( name, idStr( value ), flags, false );
}

/*
============
idCVarSystemLocal::SetCVarInteger
============
*/
void idCVarSystemLocal::SetCVarInteger( const char *name, const int value, int flags ) {
	SetInternal( name, idStr( value ), flags, false );
}

/*
============
idCVarSystemLocal::SetCVarFloat
============
*/
void idCVarSystemLocal::SetCVarFloat( const char *name, const float value, int flags ) {
	SetInternal( name, idStr( value ), flags, false );
}

/*
============
idCVarSystemLocal::GetCVarString
============
*/
const char *idCVarSystemLocal::GetCVarString( const char *name ) const {
	idScopedCriticalSection lock( mutex );

	idCVar *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetString();
	}
	return "";
}

/*
============
idCVarSystemLocal::GetCVarMissionString
============
*/
const char *idCVarSystemLocal::GetCVarMissionString( const char *name ) const {
	idScopedCriticalSection lock( mutex );

	idCVar *internal = FindInternal( name );
	if ( internal && internal->missionOverride ) {
		return internal->missionString.c_str();
	}
	return nullptr;
}

/*
============
idCVarSystemLocal::GetCVarBool
============
*/
bool idCVarSystemLocal::GetCVarBool( const char *name ) const {
	idScopedCriticalSection lock( mutex );

	idCVar *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetBool();
	}
	return false;
}

/*
============
idCVarSystemLocal::GetCVarInteger
============
*/
int idCVarSystemLocal::GetCVarInteger( const char *name ) const {
	idScopedCriticalSection lock( mutex );

	idCVar *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetInteger();
	}
	return 0;
}

/*
============
idCVarSystemLocal::GetCVarFloat
============
*/
float idCVarSystemLocal::GetCVarFloat( const char *name ) const {
	idScopedCriticalSection lock( mutex );

	idCVar *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetFloat();
	}
	return 0.0f;
}

/*
============
idCVarSystemLocal::Command
============
*/
bool idCVarSystemLocal::Command( const idCmdArgs &args ) {
	idScopedCriticalSection lock( mutex );

	idCVar *internal = FindInternal( args.Argv( 0 ) );

	if ( internal == NULL ) {
		return false;
	}

	if ( args.Argc() == 1 ) {
		// print the variable
		common->Printf( "\"%s\" is:\"%s\"" S_COLOR_WHITE " default:\"%s\"",
					internal->nameString.c_str(), internal->valueString.c_str(), internal->resetString.c_str() );
		if ( internal->missionOverride ) {
			common->Printf( S_COLOR_MAGENTA " mission:\"%s\"", internal->missionString.c_str() );
		}
		common->Printf( "\n" );

		if ( idStr::Length( internal->GetDescription() ) > 0 ) {
			common->Printf( S_COLOR_WHITE "%s\n", internal->GetDescription() );
		}
	} else {
		// set the value
		internal->Set( args.Args(), false, false, false );
	}
	return true;
}

/*
============
idCVarSystemLocal::CommandCompletion
============
*/
void idCVarSystemLocal::CommandCompletion( void(*callback)( const char *s ) ) {
	idScopedCriticalSection lock( mutex );

	for( int i = 0; i < cvars.Num(); i++ ) {
		callback( cvars[i]->GetName() );
	}
}

/*
============
idCVarSystemLocal::ArgCompletion
============
*/
void idCVarSystemLocal::ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) {
	idScopedCriticalSection lock( mutex );

	idCmdArgs args;

	args.TokenizeString( cmdString, false );

	for( int i = 0; i < cvars.Num(); i++ ) {
		if ( !cvars[i]->valueCompletion ) {
			continue;
		}
		if ( idStr::Icmp( args.Argv( 0 ), cvars[i]->nameString.c_str() ) == 0 ) {
			cvars[i]->valueCompletion( args, callback );
			break;
		}
	}
}

/*
============
idCVarSystemLocal::WasArchivedCVarModifiedAfterLastWrite
============
*/
bool idCVarSystemLocal::WasArchivedCVarModifiedAfterLastWrite() {
	return wasArchivedCvarModifiedAfterLastWrite.load( std::memory_order_acquire );
}
void SetArchivedCVarModified() {
	localCVarSystem.wasArchivedCvarModifiedAfterLastWrite.store( true, std::memory_order_release );
}

/*
============
idCVarSystemLocal::WriteArchivedCVars
============
*/
void idCVarSystemLocal::WriteArchivedCVars( idFile *f ) {
	idScopedCriticalSection lock( mutex );

	for( int i = 0; i < cvars.Num(); i++ ) {
		idCVar *cvar = cvars[i];
		if ( cvar->GetFlags() & CVAR_ARCHIVE ) {
			f->Printf( "seta %s \"%s\"\n", cvar->GetName(), cvar->valueString.c_str() );
		}
	}

	wasArchivedCvarModifiedAfterLastWrite.store( false );
}


/*
============
idCVarSystemLocal::MoveCVarsToDict
============
*/
idDict idCVarSystemLocal::MoveCVarsToDict( int flags ) const {
	idScopedCriticalSection lock( mutex );

	idDict dict;
	for( int i = 0; i < cvars.Num(); i++ ) {
		idCVar *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			dict.Set( cvar->GetName(), cvar->GetString() );
		}
	}
	return dict;
}

/*
============
idCVarSystemLocal::SetCVarsFromDict
============
*/
void idCVarSystemLocal::SetCVarsFromDict( const idDict &dict ) {
	idScopedCriticalSection lock( mutex );

	for( int i = 0; i < dict.GetNumKeyVals(); i++ ) {
		const idKeyValue *kv = dict.GetKeyVal( i );
		idCVar *internal = FindInternal( kv->GetKey() );
		if ( internal ) {
			internal->InternalServerSetString( kv->GetValue() );
		}
	}
}

/*
============
idCVarSystemLocal::GetMissionOverrides
============
*/
idDict idCVarSystemLocal::GetMissionOverrides() const {
	idScopedCriticalSection lock( mutex );

	idDict dict;
	for ( int i = 0; i < cvars.Num(); i++ ) {
		idCVar *cvar = cvars[i];
		if ( cvar->missionOverride ) {
			dict.Set( cvar->GetName(), cvar->missionString );
		}
	}
	return dict;
}

/*
============
idCVarSystemLocal::SetMissionOverrides
============
*/
void idCVarSystemLocal::SetMissionOverrides( const idDict &dict ) {
	idScopedCriticalSection lock( mutex );

	for ( int i = 0; i < cvars.Num(); i++ ) {
		idCVar *cvar = cvars[i];
		if ( cvar->missionOverride ) {
			cvar->InternalMissionSetString( nullptr );
		}
	}

	for ( int i = 0; i < dict.GetNumKeyVals(); i++ ) {
		const idKeyValue *kv = dict.GetKeyVal( i );
		idCVar *internal = FindInternal( kv->GetKey() );
		if ( internal ) {
			internal->InternalMissionSetString( kv->GetValue() );
		}
	}
}

/*
============
idCVarSystemLocal::Toggle_f
============
*/
void idCVarSystemLocal::Toggle_f( const idCmdArgs &args ) {
	int argc, i;
	float current, set;
	const char *text;

	argc = args.Argc();
	if ( argc < 2 ) {
		common->Printf ("usage:\n"
			"   toggle <variable>  - toggles between 0 and 1\n"
			"   toggle <variable> <value> - toggles between 0 and <value>\n"
			"   toggle <variable> [string 1] [string 2]...[string n] - cycles through all strings\n");
		return;
	}

	idScopedCriticalSection lock( localCVarSystem.mutex );

	idCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );

	if ( cvar == NULL ) {
		common->Warning( "Toggle_f: cvar \"%s\" not found", args.Argv( 1 ) );
		return;
	}

	if ( argc > 3 ) {
		// cycle through multiple values
		text = cvar->GetString();
		for( i = 2; i < argc; i++ ) {
			if ( !idStr::Icmp( text, args.Argv( i ) ) ) {
				// point to next value
				i++;
				break;
			}
		}
		if ( i >= argc ) {
			i = 2;
		}

		common->Printf( "set %s = %s\n", args.Argv(1), args.Argv( i ) );
		cvar->Set( va("%s", args.Argv( i ) ), false, false, false );
	} else {
		// toggle between 0 and 1
		current = cvar->GetFloat();
		if ( argc == 3 ) {
			set = atof( args.Argv( 2 ) );
		} else {
			set = 1.0f;
		}
		if ( current == 0.0f ) {
			current = set;
		} else {
			current = 0.0f;
		}
		common->Printf( "set %s = %f\n", args.Argv(1), current );
		cvar->Set( idStr( current ), false, false, false );
	}
}

/*
============
idCVarSystemLocal::Set_f
============
*/
void idCVarSystemLocal::Set_f( const idCmdArgs &args ) {
	const char *str = args.Args( 2, args.Argc() - 1 );
	localCVarSystem.SetCVarString( args.Argv(1), str );
}

/*
============
idCVarSystemLocal::SetS_f
============
*/
void idCVarSystemLocal::SetS_f( const idCmdArgs &args ) {
	Set_f( args );
	idCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
	if ( !cvar ) {
		return;
	}
	cvar->flags |= CVAR_SERVERINFO | CVAR_ARCHIVE;
}

/*
============
idCVarSystemLocal::SetU_f
============
*/
void idCVarSystemLocal::SetU_f( const idCmdArgs &args ) {
	Set_f( args );
	idCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
	if ( !cvar ) {
		return;
	}
	cvar->flags |= CVAR_USERINFO | CVAR_ARCHIVE;
}

/*
============
idCVarSystemLocal::SetT_f
============
*/
void idCVarSystemLocal::SetT_f( const idCmdArgs &args ) {
	Set_f( args );
	idCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
	if ( !cvar ) {
		return;
	}
	cvar->flags |= CVAR_TOOL;
}

/*
============
idCVarSystemLocal::SetA_f
============
*/
void idCVarSystemLocal::SetA_f( const idCmdArgs &args ) {
	Set_f( args );
	idCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
	if ( !cvar ) {
		return;
	}

	// FIXME: enable this for ship, so mods can store extra data
	// but during development we don't want obsolete cvars to continue
	// to be saved
//	cvar->flags |= CVAR_ARCHIVE;
}

/*
============
idCVarSystemLocal::Reset_f
============
*/
void idCVarSystemLocal::Reset_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf ("usage: reset <variable>\n");
		return;
	}
	idScopedCriticalSection lock( localCVarSystem.mutex );
	idCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
	if ( !cvar ) {
		return;
	}

	cvar->Reset();
}

/*
============
idCVarSystemLocal::SetM_f
============
*/
void idCVarSystemLocal::SetM_f( const idCmdArgs &args ) {
	const char *str = args.Args( 2, args.Argc() - 1 );
	localCVarSystem.SetCVarMissionString( args.Argv(1), str );
}

/*
============
idCVarSystemLocal::UnSetM_f
============
*/
void idCVarSystemLocal::UnSetM_f( const idCmdArgs &args ) {
	localCVarSystem.SetCVarMissionString( args.Argv(1), nullptr );
}


/*
============
idCVarSystemLocal::ListByFlags
============
*/
// NOTE: the const wonkyness is required to make msvc happy
template<>
ID_INLINE int idListSortCompare( const idCVar * const *a, const idCVar * const *b ) {
	return idStr::Icmp( (*a)->GetName(), (*b)->GetName() );
}

void idCVarSystemLocal::ListByFlags( const idCmdArgs &args, cvarFlags_t flags ) {
	int i, argNum;
	idStr match, indent, string;

	enum {
		SHOW_VALUE,
		SHOW_DESCRIPTION,
		SHOW_TYPE,
		SHOW_FLAGS
	} show;

	argNum = 1;
	show = SHOW_VALUE;

	if ( idStr::Icmp( args.Argv( argNum ), "-" ) == 0 || idStr::Icmp( args.Argv( argNum ), "/" ) == 0 ) {
		if ( idStr::Icmp( args.Argv( argNum + 1 ), "help" ) == 0 || idStr::Icmp( args.Argv( argNum + 1 ), "?" ) == 0 ) {
			argNum = 3;
			show = SHOW_DESCRIPTION;
		} else if ( idStr::Icmp( args.Argv( argNum + 1 ), "type" ) == 0 || idStr::Icmp( args.Argv( argNum + 1 ), "range" ) == 0 ) {
			argNum = 3;
			show = SHOW_TYPE;
		} else if ( idStr::Icmp( args.Argv( argNum + 1 ), "flags" ) == 0 ) {
			argNum = 3;
			show = SHOW_FLAGS;
		}
	}

	if ( args.Argc() > argNum ) {
		match = args.Args( argNum, -1 );
		match.Remove( ' ' );
	} else {
		match = "";
	}

	idScopedCriticalSection lock( localCVarSystem.mutex );
	idList<const idCVar *>cvarList;
	for ( i = 0; i < localCVarSystem.cvars.Num(); i++ ) {
		const idCVar *cvar = localCVarSystem.cvars[i];

		if ( !( cvar->GetFlags() & flags ) ) {
			continue;
		}

		if ( match.Length() && !cvar->nameString.Filter( match, false ) ) {
			continue;
		}

		cvarList.Append( cvar );
	}

	cvarList.Sort();

	switch( show ) {
		case SHOW_VALUE: {
			for ( i = 0; i < cvarList.Num(); i++ ) {
				const idCVar *cvar = cvarList[i];
				common->Printf( FORMAT_STRING , cvar->nameString.c_str() );
				if ( cvar->missionOverride ) {
					assert( cvar->value == cvar->missionString.c_str() );
					common->Printf( S_COLOR_MAGENTA "\"%s\"\n", cvar->value );
				} else {
					assert( cvar->value == cvar->valueString.c_str() );
					common->Printf( S_COLOR_WHITE "\"%s\"\n", cvar->value );
				}
			}
			break;
		}
		case SHOW_DESCRIPTION: {
			indent.Fill( ' ', NUM_NAME_CHARS );
			indent.Insert( "\n", 0 );

			for ( i = 0; i < cvarList.Num(); i++ ) {
				const idCVar *cvar = cvarList[i];
				common->Printf( FORMAT_STRING S_COLOR_WHITE "%s\n", cvar->nameString.c_str(), CreateColumn( cvar->GetDescription(), NUM_DESCRIPTION_CHARS, indent, string ) );
			}
			break;
		}
		case SHOW_TYPE: {
			for ( i = 0; i < cvarList.Num(); i++ ) {
				const idCVar *cvar = cvarList[i];
				if ( cvar->GetFlags() & CVAR_BOOL ) {
					common->Printf( FORMAT_STRING S_COLOR_CYAN "bool\n", cvar->GetName() );
				} else if ( cvar->GetFlags() & CVAR_INTEGER ) {
					if ( cvar->GetMinValue() < cvar->GetMaxValue() ) {
						common->Printf( FORMAT_STRING S_COLOR_GREEN "int " S_COLOR_WHITE "[%d, %d]\n", cvar->GetName(), (int) cvar->GetMinValue(), (int) cvar->GetMaxValue() );
					} else {
						common->Printf( FORMAT_STRING S_COLOR_GREEN "int\n", cvar->GetName() );
					}
				} else if ( cvar->GetFlags() & CVAR_FLOAT ) {
					if ( cvar->GetMinValue() < cvar->GetMaxValue() ) {
						common->Printf( FORMAT_STRING S_COLOR_RED "float " S_COLOR_WHITE "[%s, %s]\n", cvar->GetName(), idStr( cvar->GetMinValue() ).c_str(), idStr( cvar->GetMaxValue() ).c_str() );
					} else {
						common->Printf( FORMAT_STRING S_COLOR_RED "float\n", cvar->GetName() );
					}
				} else if ( cvar->GetValueStrings() ) {
					common->Printf( FORMAT_STRING S_COLOR_WHITE "string " S_COLOR_WHITE "[", cvar->GetName() );
					for ( int j = 0; cvar->GetValueStrings()[j] != NULL; j++ ) {
						if ( j ) {
							common->Printf( S_COLOR_WHITE ", %s", cvar->GetValueStrings()[j] );
						} else {
							common->Printf( S_COLOR_WHITE "%s", cvar->GetValueStrings()[j] );
						}
					}
					common->Printf( S_COLOR_WHITE "]\n" );
				} else {
					common->Printf( FORMAT_STRING S_COLOR_WHITE "string\n", cvar->GetName() );
				}
			}
			break;
		}
		case SHOW_FLAGS: {
			for ( i = 0; i < cvarList.Num(); i++ ) {
				const idCVar *cvar = cvarList[i];
				common->Printf( FORMAT_STRING, cvar->GetName() );
				string = "";
				if ( cvar->GetFlags() & CVAR_BOOL ) {
					string += S_COLOR_CYAN "B ";
				} else if ( cvar->GetFlags() & CVAR_INTEGER ) {
					string += S_COLOR_GREEN "I ";
				} else if ( cvar->GetFlags() & CVAR_FLOAT ) {
					string += S_COLOR_RED "F ";
				} else {
					string += S_COLOR_WHITE "S ";
				}
				if ( cvar->GetFlags() & CVAR_SYSTEM ) {
					string += S_COLOR_WHITE "SYS  ";
				} else if ( cvar->GetFlags() & CVAR_RENDERER ) {
					string += S_COLOR_WHITE "RNDR ";
				} else if ( cvar->GetFlags() & CVAR_SOUND ) {
					string += S_COLOR_WHITE "SND  ";
				} else if ( cvar->GetFlags() & CVAR_GUI ) {
					string += S_COLOR_WHITE "GUI  ";
				} else if ( cvar->GetFlags() & CVAR_GAME ) {
					string += S_COLOR_WHITE "GAME ";
				} else if ( cvar->GetFlags() & CVAR_TOOL ) {
					string += S_COLOR_WHITE "TOOL ";
				} else {
					string += S_COLOR_WHITE "     ";
				}
				string += ( cvar->GetFlags() & CVAR_USERINFO ) ?	"UI "	: "   ";
				string += ( cvar->GetFlags() & CVAR_SERVERINFO ) ?	"SI "	: "   ";
				string += ( cvar->GetFlags() & CVAR_STATIC ) ?		"ST "	: "   ";
				string += ( cvar->GetFlags() & CVAR_INIT ) ?		"IN "	: "   ";
				string += ( cvar->GetFlags() & CVAR_ROM ) ?			"RO "	: "   ";
				string += ( cvar->GetFlags() & CVAR_ARCHIVE ) ?		"AR "	: "   ";
				string += ( cvar->GetFlags() & CVAR_MODIFIED ) ?	"MO "	: "   ";
				string += "\n";
				common->Printf( string );
			}
			break;
		}
	}

	common->Printf( "\n%i cvars listed\n\n", cvarList.Num() );
	common->Printf(	"listCvar [search string]          = list cvar values\n"
				"listCvar -help [search string]    = list cvar descriptions\n"
				"listCvar -type [search string]    = list cvar types\n"
				"listCvar -flags [search string]   = list cvar flags\n" );
}

/*
============
idCVarSystemLocal::List_f
============
*/
void idCVarSystemLocal::List_f( const idCmdArgs &args ) {
	ListByFlags( args, CVAR_ALL );
}

/*
============
idCVarSystemLocal::Restart_f
============
*/
void idCVarSystemLocal::Restart_f( const idCmdArgs &args ) {
	idScopedCriticalSection lock( localCVarSystem.mutex );

	for ( int i = 0; i < localCVarSystem.cvars.Num(); i++ ) {
		idCVar *cvar = localCVarSystem.cvars[i];

		// don't mess with rom values
		if ( cvar->flags & ( CVAR_ROM | CVAR_INIT ) ) {
			continue;
		}

		// throw out any variables the user created
		if ( !( cvar->flags & CVAR_STATIC ) ) {
			int hash = localCVarSystem.cvarHash.GenerateKey( cvar->nameString, false );
			delete cvar;
			localCVarSystem.cvars.RemoveIndex( i );
			localCVarSystem.cvarHash.RemoveIndex( hash, i );
			i--;
			continue;
		}

		cvar->Reset();
	}
}
