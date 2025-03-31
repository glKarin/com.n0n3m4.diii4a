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

#ifndef __CVARSYSTEM_H__
#define __CVARSYSTEM_H__

/*
===============================================================================

	Console Variables (CVars) are used to hold scalar or string variables
	that can be changed or displayed at the console as well as accessed
	directly in code.

	CVars are mostly used to hold settings that can be changed from the
	console or saved to and loaded from configuration files. CVars are also
	occasionally used to communicate information between different modules
	of the program.

	CVars are restricted from having the same names as console commands to
	keep the console interface from being ambiguous.

	CVars can be accessed from the console in three ways:
	cvarName			prints the current value
	cvarName X			sets the value to X if the variable exists
	set cvarName X		as above, but creates the CVar if not present

	stgatilov #5600: CVars must be declared as global variable or as static class member.
	Unlike original Doom 3, a cvar can only have one C++ variable connected to it.

	stgatilov #5600: Standard thread-safety principles apply:
	 * It is OK to read/write different cvars in parallel.
	 * It is OK to read the same cvar in parallel.
	 * It is WRONG to write cvar in parallel with reading/writing the same cvar.
	 * Some methods are only called during initialization, they are never in parallel to anything.

	CVars should be contructed only through one of the constructors with name,
	value, flags and description. The name, value and description parameters
	to the constructor have to be static strings, do not use va() or the like
	functions returning a string.

	CVars should always be declared with the correct type flag: CVAR_BOOL,
	CVAR_INTEGER or CVAR_FLOAT. If no such flag is specified the CVar
	defaults to type string. If the CVAR_BOOL flag is used there is no need
	to specify an argument auto-completion function because the CVar gets
	one assigned automatically.

	CVars are automatically range checked based on their type and any min/max
	or valid string set specified in the constructor.

	CVars are always considered cheats except when CVAR_NOCHEAT, CVAR_INIT,
	CVAR_ROM, CVAR_ARCHIVE, CVAR_USERINFO, CVAR_SERVERINFO, CVAR_NETWORKSYNC
	is set.

===============================================================================
*/

typedef enum {
	CVAR_ALL				= -1,		// all flags
	CVAR_BOOL				= BIT(0),	// variable is a boolean
	CVAR_INTEGER			= BIT(1),	// variable is an integer
	CVAR_FLOAT				= BIT(2),	// variable is a float
	CVAR_SYSTEM				= BIT(3),	// system variable
	CVAR_RENDERER			= BIT(4),	// renderer variable
	CVAR_SOUND				= BIT(5),	// sound variable
	CVAR_GUI				= BIT(6),	// gui variable
	CVAR_GAME				= BIT(7),	// game variable
	CVAR_TOOL				= BIT(8),	// tool variable
	CVAR_USERINFO			= BIT(9),	// sent to servers, available to menu
	CVAR_SERVERINFO			= BIT(10),	// sent from servers, available to menu
	CVAR_NETWORKSYNC		= BIT(11),	// cvar is synced from the server to clients
	CVAR_STATIC				= BIT(12),	// statically declared, not user created
	CVAR_NOCHEAT			= BIT(14),	// variable is not considered a cheat
	CVAR_INIT				= BIT(15),	// can only be set from the command-line
	CVAR_ROM				= BIT(16),	// display only, cannot be set by user at all
	CVAR_ARCHIVE			= BIT(17),	// set to cause it to be saved to a config file
	CVAR_MODIFIED			= BIT(18)	// set when the variable is modified
} cvarFlags_t;


/*
===============================================================================

	idCVar

===============================================================================
*/

class idCVar {
public:
							// Always use one of the following constructors.
							idCVar( const char *name, const char *value, int flags, const char *description,
									argCompletion_t valueCompletion = NULL );
							idCVar( const char *name, const char *value, int flags, const char *description,
									float valueMin, float valueMax, argCompletion_t valueCompletion = NULL );
							idCVar( const char *name, const char *value, int flags, const char *description,
									const char **valueStrings, argCompletion_t valueCompletion = NULL );

	ID_FORCE_INLINE const char *		GetName( void ) const { return name; }
	ID_FORCE_INLINE int					GetFlags( void ) const { return flags; }
	ID_FORCE_INLINE const char *		GetDescription( void ) const { return description; }
	ID_FORCE_INLINE float				GetMinValue( void ) const { return valueMin; }
	ID_FORCE_INLINE float				GetMaxValue( void ) const { return valueMax; }
	ID_FORCE_INLINE const char **		GetValueStrings( void ) const { return valueStrings; }
	ID_FORCE_INLINE argCompletion_t		GetValueCompletion( void ) const { return valueCompletion; }

	ID_FORCE_INLINE bool				IsModified( void ) const { return ( flags & CVAR_MODIFIED ) != 0; }
	void								SetModified( void ) { flags |= CVAR_MODIFIED; }
	void								ClearModified( void ) { flags &= ~CVAR_MODIFIED; }

	ID_FORCE_INLINE const char *		GetString( void ) const { return value; }
	ID_FORCE_INLINE bool				GetBool( void ) const { return ( integerValue != 0 ); }
	ID_FORCE_INLINE int					GetInteger( void ) const { return integerValue; }
	ID_FORCE_INLINE float				GetFloat( void ) const { return floatValue; }

	void					SetString( const char *value ) { InternalSetString( value ); }
	void					SetBool( const bool value ) { InternalSetBool( value ); }
	void					SetInteger( const int value ) { InternalSetInteger( value ); }
	void					SetFloat( const float value ) { InternalSetFloat( value ); }

	static void				RegisterStaticVars( void );

private:
	const char *			name;					// name
	const char *			value;					// value
	const char *			description;			// description
	int						flags;					// CVAR_? flags
	float					valueMin;				// minimum value
	float					valueMax;				// maximum value
	const char **			valueStrings;			// valid value strings
	argCompletion_t			valueCompletion;		// value auto-completion function
	int						integerValue;			// atoi( string )
	float					floatValue;				// atof( value )
	idCVar *				next;					// next statically declared cvar

	// from idInternalCVar:
	idStr					nameString;				// name
	idStr					resetString;			// default value (resetting will change to this)
	idStr					valueString;			// main value
	bool					missionOverride;		// if true, then "missionString" overrides main value,
	idStr					missionString;
	idStr					descriptionString;		// description
	friend class idCVarSystemLocal;

private:
	idCVar( void ) = default;

	void					Init( const char *name, const char *value, int flags, const char *description,
									float valueMin, float valueMax, const char **valueStrings, argCompletion_t valueCompletion );

	void					UpdateValue( void );
	void					Set( const char *newValue, bool force, bool fromServer, bool mission );
	void					Reset( void );

	// from idInternalCVar:
	void					InternalSetString( const char *newValue );
	void					InternalServerSetString( const char *newValue );
	void					InternalMissionSetString( const char *newValue );
	void					InternalSetBool( const bool newValue );
	void					InternalSetInteger( const int newValue );
	void					InternalSetFloat( const float newValue );
	static idCVar *			InternalCreate( const char *newName, int newFlags );
	void					InternalRegister( void );

	static idCVar *			staticVars;
};

class idCVarInt : public idCVar {
public:
	idCVarInt( const char *name, const char *value, int flags, const char *description )
		: idCVar( name, value, flags | CVAR_INTEGER, description ) {}
	operator				int() { return GetInteger(); }
	void operator			= ( int newValue ) { SetInteger( newValue ); }
};

class idCVarBool : public idCVar {
public:
	idCVarBool( const char *name, const char *value, int flags, const char *description )
		: idCVar( name, value, flags | CVAR_BOOL, description ) {}
	operator				bool() { return GetBool(); }
	void operator			= ( bool newValue ) { SetBool( newValue ); }
};

ID_INLINE idCVar::idCVar( const char *name, const char *value, int flags, const char *description,
							argCompletion_t valueCompletion ) {
	if ( !valueCompletion && ( flags & CVAR_BOOL ) ) {
		valueCompletion = idCmdSystem::ArgCompletion_Boolean;
	}
	Init( name, value, flags, description, 1, -1, NULL, valueCompletion );
}

ID_INLINE idCVar::idCVar( const char *name, const char *value, int flags, const char *description,
							float valueMin, float valueMax, argCompletion_t valueCompletion ) {
	Init( name, value, flags, description, valueMin, valueMax, NULL, valueCompletion );
}

ID_INLINE idCVar::idCVar( const char *name, const char *value, int flags, const char *description,
							const char **valueStrings, argCompletion_t valueCompletion ) {
	Init( name, value, flags, description, 1, -1, valueStrings, valueCompletion );
}


/*
===============================================================================

	idCVarSystem

===============================================================================
*/

class idCVarSystem {
public:
	virtual					~idCVarSystem( void ) {}

	virtual void			Init( void ) = 0;
	virtual void			Shutdown( void ) = 0;
	virtual bool			IsInitialized( void ) const = 0;

							// Registers a CVar.
							// INTERNAL USE ONLY!
	virtual void			Register( idCVar *cvar ) = 0;

							// Finds the CVar with the given name.
							// Returns NULL if there is no CVar with the given name.
	virtual idCVar *		Find( const char *name ) = 0;

							// Sets the value of a CVar by name.
	virtual void			SetCVarString( const char *name, const char *value, int flags = 0 ) = 0;
	virtual void			SetCVarBool( const char *name, const bool value, int flags = 0 ) = 0;
	virtual void			SetCVarInteger( const char *name, const int value, int flags = 0 ) = 0;
	virtual void			SetCVarFloat( const char *name, const float value, int flags = 0 ) = 0;
							// stgatilov #5453: override cvar value by mission
							// value = NULL means: drop mission override
	virtual void			SetCVarMissionString( const char *name, const char *value ) = 0;

							// Gets the value of a CVar by name.
	virtual const char *	GetCVarString( const char *name ) const = 0;
	virtual bool			GetCVarBool( const char *name ) const = 0;
	virtual int				GetCVarInteger( const char *name ) const = 0;
	virtual float			GetCVarFloat( const char *name ) const = 0;
							// stgatilov #5453: returns mission override or NULL if there is none
	virtual const char *	GetCVarMissionString( const char *name ) const = 0;

							// Called by the command system when argv(0) doesn't match a known command.
							// Returns true if argv(0) is a variable reference and prints or changes the CVar.
	virtual bool			Command( const idCmdArgs &args ) = 0;

							// Command and argument completion using callback for each valid string.
	virtual void			CommandCompletion( void(*callback)( const char *s ) ) = 0;
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) = 0;

							// Moves CVars to and from dictionaries.
	virtual idDict			MoveCVarsToDict( int flags ) const = 0;
	virtual void			SetCVarsFromDict( const idDict &dict ) = 0;

	virtual bool			WasArchivedCVarModifiedAfterLastWrite() = 0;
	virtual void			WriteArchivedCVars( idFile *f ) = 0;

	virtual idDict			GetMissionOverrides() const = 0;
	virtual void			SetMissionOverrides( const idDict &dict = {} ) = 0;
};

extern idCVarSystem *		cvarSystem;

#endif /* !__CVARSYSTEM_H__ */
