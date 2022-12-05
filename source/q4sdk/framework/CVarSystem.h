// Copyright (C) 2004 Id Software, Inc.
//

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

	CVars may be declared in the global namespace, in classes and in functions.
	However declarations in classes and functions should always be static to
	save space and time. Making CVars static does not change their
	functionality due to their global nature.

	CVars should be contructed only through one of the constructors with name,
	value, flags and description. The name, value and description parameters
	to the constructor have to be static strings, do not use va() or the like
	functions returning a string.

	CVars may be declared multiple times using the same name string. However,
	they will all reference the same value and changing the value of one CVar
	changes the value of all CVars with the same name.

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
	CVAR_CHEAT				= BIT(13),	// variable is considered a cheat
	CVAR_NOCHEAT			= BIT(14),	// variable is not considered a cheat
	CVAR_INIT				= BIT(15),	// can only be set from the command-line
	CVAR_ROM				= BIT(16),	// display only, cannot be set by user at all
	CVAR_ARCHIVE			= BIT(17),	// set to cause it to be saved to a config file
	CVAR_MODIFIED			= BIT(18),	// set when the variable is modified
	CVAR_INFO				= BIT(19),	// sent as part of the MOTD packet
	CVAR_NORESET			= BIT(20),	// don't reset the contents on cvar system restart
	CVAR_CASE_SENSITIVE		= BIT(21),	// a change in case of the string contents sets the modified flag
	CVAR_SPECIAL_CONCAT		= BIT(22),	// special concatination of the incoming string to the cvar system, will remove space between ^ and the code that is produced by tokenzier
	CVAR_STRIPTRAILING		= BIT(23),	// always strip trailing / on that cvar
	CVAR_REPEATERINFO		= BIT(24),	// sent from repeaters, available to menu
} cvarFlags_t;


/*
===============================================================================

	idCVar

===============================================================================
*/

class idCVar {
public:
							// Never use the default constructor.
							idCVar( void ) { assert( typeid( this ) != typeid( idCVar ) ); }

							// Always use one of the following constructors.
							idCVar( const char *name, const char *value, int flags, const char *description,
									argCompletion_t valueCompletion = NULL );
							idCVar( const char *name, const char *value, int flags, const char *description,
									float valueMin, float valueMax, argCompletion_t valueCompletion = NULL );
							idCVar( const char *name, const char *value, int flags, const char *description,
									const char **valueStrings, argCompletion_t valueCompletion = NULL );

	virtual					~idCVar( void ) {}

	const char *			GetName( void ) const { return internalVar->name; }
	int						GetFlags( void ) const { return internalVar->flags; }
	const char *			GetDescription( void ) const { return internalVar->description; }
	float					GetMinValue( void ) const { return internalVar->valueMin; }
	float					GetMaxValue( void ) const { return internalVar->valueMax; }
	const char **			GetValueStrings( void ) const { return valueStrings; }
	argCompletion_t			GetValueCompletion( void ) const { return valueCompletion; }

	bool					IsModified( void ) const { return ( internalVar->flags & CVAR_MODIFIED ) != 0; }
	void					SetModified( void ) { internalVar->flags |= CVAR_MODIFIED; }
	void					ClearModified( void ) { internalVar->flags &= ~CVAR_MODIFIED; }

	const char *			GetString( void ) const { return internalVar->value; }
	bool					GetBool( void ) const { return ( internalVar->integerValue != 0 ); }
	int						GetInteger( void ) const { return internalVar->integerValue; }
	float					GetFloat( void ) const { return internalVar->floatValue; }

	void					SetString( const char *value ) { internalVar->InternalSetString( value ); }
	void					SetBool( const bool value ) { internalVar->InternalSetBool( value ); }
	void					SetInteger( const int value ) { internalVar->InternalSetInteger( value ); }
	void					SetFloat( const float value ) { internalVar->InternalSetFloat( value ); }

	void					SetInternalVar( idCVar *cvar ) { internalVar = cvar; }

	void					SetFlag( const cvarFlags_t flag ) { internalVar->flags |= flag; }
	void					RemoveFlag( const cvarFlags_t flag ) { internalVar->flags &= ~flag; }

	static void				RegisterStaticVars( void );

protected:
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
	idCVar *				internalVar;			// internal cvar
	idCVar *				next;					// next statically declared cvar

private:
	void					Init( const char *name, const char *value, int flags, const char *description,
									float valueMin, float valueMax, const char **valueStrings, argCompletion_t valueCompletion );

	virtual void			InternalSetString( const char *newValue ) {}
	virtual void			InternalSetBool( const bool newValue ) {}
	virtual void			InternalSetInteger( const int newValue ) {}
	virtual void			InternalSetFloat( const float newValue ) {}

	static idCVar *			staticVars;
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

// RAVEN BEGIN
// rjohnson: new help system for cvar ui

/*
===============================================================================

	idCVarHelp

===============================================================================
*/

typedef enum {
	CVARHELP_ALL			= -1,		// all categories
	CVARHELP_GAME			= BIT(0),	// game menu
	CVARHELP_RENDERER		= BIT(1),	// renderer menu
	CVARHELP_PHYSICS		= BIT(2),	// physics menu
	CVARHELP_SOUND			= BIT(3),	// sound menu
	CVARHELP_AI				= BIT(4),	// AI menu
} cvarHelpCategory_t;


class idCVarHelp {
public:
								// Never use the default constructor.
								idCVarHelp( void ) { assert( typeid(this) != typeid(idCVarHelp) ); }

								// Always use one of the following constructors.
								idCVarHelp( const char *cvarName, const char *friendlyName, const char *choices, const char *values, cvarHelpCategory_t category );

								idCVarHelp( const idCVarHelp &copy );

	const char *				GetCVarName( void ) const { return cvarName; }	
	const char *				GetFriendlyName( void ) const { return friendlyName; }	
	const char *				GetChoices( void ) const { return choices; }	
	const char *				GetValues( void ) const { return values; }	
	const cvarHelpCategory_t	GetCategory( void ) const { return category; }	
	const idCVarHelp *			GetNext( void ) const { return next; }

	void						SetNext( const idCVarHelp *value ) { next = value; }

	static void					RegisterStatics( void );

private:
	const char *				cvarName;				// the cvar this help belongs to
	const char *				friendlyName;			// a textual name for the cvar impaired
	const char *				choices;				// a textual list of choices for the cvar impaired
	const char *				values;					// the list of values that goes with the choices
	cvarHelpCategory_t			category;				// the category(s) this cvar should appear under
	const idCVarHelp *			next;					// next statically declared cvar helper

	static idCVarHelp *			staticCVarHelps;
	static idCVarHelp *			staticCVarHelpsTail;
};

ID_INLINE idCVarHelp::idCVarHelp( const idCVarHelp &copy ) {
	cvarName = copy.cvarName;
	friendlyName = copy.friendlyName;
	choices = copy.choices;
	values = copy.values;
	category = copy.category;
	next = NULL;
}

// RAVEN END

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
	virtual void			Register( idCVar *cvar ) = 0;

// RAVEN BEGIN
// rjohnson: new help system for cvar ui
							// Registers a CVarHelp.
	virtual void			Register( const idCVarHelp *cvarHelp ) = 0;
	virtual idCVarHelp *	GetHelps( cvarHelpCategory_t category ) = 0;
// RAVEN END

							// Finds the CVar with the given name.
							// Returns NULL if there is no CVar with the given name.
	virtual idCVar *		Find( const char *name ) = 0;

							// Sets the value of a CVar by name.
	virtual void			SetCVarString( const char *name, const char *value, int flags = 0 ) = 0;
	virtual void			SetCVarBool( const char *name, const bool value, int flags = 0 ) = 0;
	virtual void			SetCVarInteger( const char *name, const int value, int flags = 0 ) = 0;
	virtual void			SetCVarFloat( const char *name, const float value, int flags = 0 ) = 0;

							// Gets the value of a CVar by name.
	virtual const char *	GetCVarString( const char *name ) const = 0;
	virtual bool			GetCVarBool( const char *name ) const = 0;
	virtual int				GetCVarInteger( const char *name ) const = 0;
	virtual float			GetCVarFloat( const char *name ) const = 0;

							// Called by the command system when argv(0) doesn't match a known command.
							// Returns true if argv(0) is a variable reference and prints or changes the CVar.
	virtual bool			Command( const idCmdArgs &args ) = 0;

							// Command and argument completion using callback for each valid string.
	virtual void			CommandCompletion( void(*callback)( const char *s ) ) = 0;
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) = 0;

							// Sets/gets/clears modified flags that tell what kind of CVars have changed.
	virtual void			SetModifiedFlags( int flags ) = 0;
	virtual int				GetModifiedFlags( void ) const = 0;
	virtual void			ClearModifiedFlags( int flags ) = 0;

							// Resets variables with one of the given flags set.
	virtual void			ResetFlaggedVariables( int flags ) = 0;

							// Removes auto-completion from the flagged variables.
	virtual void			RemoveFlaggedAutoCompletion( int flags ) = 0;

							// Writes variables with one of the given flags set to the given file.
	virtual void			WriteFlaggedVariables( int flags, const char *setCmd, idFile *f ) const = 0;
// RAVEN BEGIN
// nrausch: memory cvar support
	virtual unsigned int	WriteFlaggedVariables( int flags, const char *setCmd, byte *buf, unsigned int bufSize ) const = 0;
	virtual void			ApplyFlaggedVariables( byte *buf, unsigned int bufSize ) = 0;
	virtual idStr			WriteFlaggedVariables( int flags ) const = 0;
// RAVEN END

							// Moves CVars to and from dictionaries.
	virtual const idDict *	MoveCVarsToDict( int flags ) const = 0;
	virtual void			SetCVarsFromDict( const idDict &dict ) = 0;
};

extern idCVarSystem *		cvarSystem;


/*
===============================================================================

	CVar Registration

	Each DLL using CVars has to declare a private copy of the static variable
	idCVar::staticVars like this: idCVar * idCVar::staticVars = NULL;
	Furthermore idCVar::RegisterStaticVars() has to be called after the
	cvarSystem pointer is set when the DLL is first initialized.

===============================================================================
*/

ID_INLINE void idCVar::Init( const char *name, const char *value, int flags, const char *description,
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
	this->internalVar = this;
	if ( staticVars != (idCVar *)0xFFFFFFFF ) {
		this->next = staticVars;
		staticVars = this;
	} else {
		cvarSystem->Register( this );
	}
}

ID_INLINE void idCVar::RegisterStaticVars( void ) {
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_CVAR);
// RAVEN END
	if ( staticVars != (idCVar *)0xFFFFFFFF ) {
		for ( idCVar *cvar = staticVars; cvar; cvar = cvar->next ) {
			cvarSystem->Register( cvar );
		}
		staticVars = (idCVar *)0xFFFFFFFF;
	}
}

// RAVEN BEGIN
// rjohnson: new help system for cvar ui
ID_INLINE idCVarHelp::idCVarHelp( const char *cvarName, const char *friendlyName, const char *choices, const char *values, cvarHelpCategory_t category ) {
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_CVAR);
// RAVEN END
	this->cvarName = cvarName;
	this->friendlyName = friendlyName;
	this->choices = choices;
	this->values = values;
	this->category = category;
	this->next = NULL;

	if ( staticCVarHelps != (idCVarHelp *)0xFFFFFFFF ) {
		if ( !staticCVarHelpsTail ) {
			staticCVarHelps = this;
		} else {
			staticCVarHelpsTail->next = this;
		}
		staticCVarHelpsTail = this;
		staticCVarHelpsTail->next = NULL;
	} else {
		cvarSystem->Register( this );
	}
}

ID_INLINE void idCVarHelp::RegisterStatics( void ) {
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_CVAR);
// RAVEN END
	if ( staticCVarHelps != (idCVarHelp *)0xFFFFFFFF ) {
		for ( const idCVarHelp *cvarHelp = staticCVarHelps; cvarHelp; cvarHelp = cvarHelp->next ) {
			cvarSystem->Register( cvarHelp );
		}
		staticCVarHelps = (idCVarHelp *)0xFFFFFFFF;
	}
}
// RAVEN END

#endif /* !__CVARSYSTEM_H__ */
