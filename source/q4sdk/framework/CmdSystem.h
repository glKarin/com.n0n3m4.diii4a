
#ifndef __CMDSYSTEM_H__
#define __CMDSYSTEM_H__

/*
===============================================================================

	Console command execution and command text buffering.

	Any number of commands can be added in a frame from several different
	sources. Most commands come from either key bindings or console line input,
	but entire text files can be execed.

	Command execution takes a null terminated string, breaks it into tokens,
	then searches for a command or variable that matches the first token.

===============================================================================
*/

// command flags
typedef enum {
	CMD_FL_ALL				= -1,
	CMD_FL_CHEAT			= BIT(0),	// command is considered a cheat
	CMD_FL_SYSTEM			= BIT(1),	// system command
	CMD_FL_RENDERER			= BIT(2),	// renderer command
	CMD_FL_SOUND			= BIT(3),	// sound command
	CMD_FL_GAME				= BIT(4),	// game command
	CMD_FL_TOOL				= BIT(5)	// tool command
} cmdFlags_t;

// parameters for command buffer stuffing
typedef enum {
	CMD_EXEC_NOW,						// don't return until completed
	CMD_EXEC_INSERT,					// insert at current position, but don't run yet
	CMD_EXEC_APPEND						// add to end of the command buffer (normal case)
} cmdExecution_t;

// command function
typedef void (*cmdFunction_t)( const idCmdArgs &args );

// argument completion function
typedef void (*argCompletion_t)( const idCmdArgs &args, void(*callback)( const char *s ) );


class idCmdSystem {
public:
	virtual				~idCmdSystem( void ) {}

	virtual void		Init( void ) = 0;
	virtual void		Shutdown( void ) = 0;

						// Registers a command and the function to call for it.
	virtual void		AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t argCompletion = NULL ) = 0;
						// Removes a command.
	virtual void		RemoveCommand( const char *cmdName ) = 0;
						// Remove all commands with one of the flags set.
	virtual void		RemoveFlaggedCommands( int flags ) = 0;

						// Command and argument completion using callback for each valid string.
	virtual void		CommandCompletion( void(*callback)( const char *s ) ) = 0;
	virtual void		ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) = 0;

						// Adds command text to the command buffer, does not add a final \n
	virtual void		BufferCommandText( cmdExecution_t exec, const char *text ) = 0;
						// Pulls off \n \r or ; terminated lines of text from the command buffer and
						// executes the commands. Stops when the buffer is empty.
						// Normally called once per frame, but may be explicitly invoked.
	virtual void		ExecuteCommandBuffer( void ) = 0;

						// Base for path/file auto-completion.
	virtual void		ArgCompletion_FolderExtension( const idCmdArgs &args, void(*callback)( const char *s ), const char *folder, bool stripFolder, ... ) = 0;
	
	virtual void		ArgCompletion_Models( const idCmdArgs &args, void(*callback)( const char *s ), bool strogg, bool marine ) = 0;
	
						// Base for decl name auto-completion.
	virtual void		ArgCompletion_DeclName( const idCmdArgs &args, void(*callback)( const char *s ), int type ) = 0;

						// Adds to the command buffer in tokenized form ( CMD_EXEC_NOW or CMD_EXEC_APPEND only )
	virtual void		BufferCommandArgs( cmdExecution_t exec, const idCmdArgs &args ) = 0;

						// Restore these cvars when the next reloadEngine is done
	virtual void		SetupCVarsForReloadEngine( const idDict &dict ) = 0;
						// Setup a reloadEngine to happen on next command run, and give a command to execute after reload
	virtual void		SetupReloadEngine( const idCmdArgs &args ) = 0;
	virtual bool		PostReloadEngine( void ) = 0;

						// There is a cache of the last completion operation that may need to be cleared sometimes
	virtual void		ClearCompletion( void ) = 0;

						// Default argument completion functions.
	static void			ArgCompletion_Boolean( const idCmdArgs &args, void(*callback)( const char *s ) );
	template<int min,int max>
	static void			ArgCompletion_Integer( const idCmdArgs &args, void(*callback)( const char *s ) );
	template<const char **strings>
	static void			ArgCompletion_String( const idCmdArgs &args, void(*callback)( const char *s ) );
	template<int type>
	static void			ArgCompletion_Decl( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_FileName( const idCmdArgs &args, void(*callback)( const char *s ) );

// RAVEN BEGIN
// mekberg: added
	static void			ArgCompletion_GuiName( const idCmdArgs &args, void(*callback)( const char *s ) );
// RAVEN END

	static void			ArgCompletion_MapName( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_ModelName( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_SoundName( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_ImageName( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_VideoName( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_ForceModel( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_ForceModelStrogg( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_ForceModelMarine( const idCmdArgs &args, void(*callback)( const char *s ) );
// RAVEN BEGIN
// nrausch: standalone video support
	static void			ArgCompletion_StandaloneVideoName( const idCmdArgs &args, void(*callback)( const char *s ) );
// rjohnson: netdemo completion
	static void			ArgCompletion_NetDemoName( const idCmdArgs &args, void(*callback)( const char *s ) );
// RAVEN END
	static void			ArgCompletion_ConfigName( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_SaveGame( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_DemoName( const idCmdArgs &args, void(*callback)( const char *s ) );
};

extern idCmdSystem *	cmdSystem;


ID_INLINE void idCmdSystem::ArgCompletion_Boolean( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	callback( va( "%s 0", args.Argv( 0 ) ) );
	callback( va( "%s 1", args.Argv( 0 ) ) );
}

template<int min,int max> ID_STATIC_TEMPLATE ID_INLINE void idCmdSystem::ArgCompletion_Integer( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	for ( int i = min; i <= max; i++ ) {
		callback( va( "%s %d", args.Argv( 0 ), i ) );
	}
}

template<const char **strings> ID_STATIC_TEMPLATE ID_INLINE void idCmdSystem::ArgCompletion_String( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	for ( int i = 0; strings[i]; i++ ) {
		callback( va( "%s %s", args.Argv( 0 ), strings[i] ) );
	}
}

template<int type> ID_STATIC_TEMPLATE ID_INLINE void idCmdSystem::ArgCompletion_Decl( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, type );
}

ID_INLINE void idCmdSystem::ArgCompletion_FileName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", true, "", NULL );
}

// RAVEN BEGIN
// mekberg: added
ID_INLINE void idCmdSystem::ArgCompletion_GuiName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "guis/", false, ".gui", NULL );
}
// RAVEN END

ID_INLINE void idCmdSystem::ArgCompletion_MapName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "maps/", true, ".map", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_ModelName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "models/", false, ".lwo", ".ase", ".md5mesh", ".ma", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_SoundName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "sound/", false, ".wav", ".ogg", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_ImageName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", false, ".tga", ".dds", ".jpg", ".pcx", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_VideoName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "video/", false, ".roq", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_ForceModel( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_Models( args, callback, true, true );
}

ID_INLINE void idCmdSystem::ArgCompletion_ForceModelStrogg( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_Models( args, callback, true, false );
}

ID_INLINE void idCmdSystem::ArgCompletion_ForceModelMarine( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_Models( args, callback, false, true );
}

// RAVEN BEGIN
// nrausch: standalone video support
ID_INLINE void idCmdSystem::ArgCompletion_StandaloneVideoName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "video/", false, ".wmv", NULL );
}

// rjohnson: netdemo completion
extern char netDemoExtension[16];
ID_INLINE void idCmdSystem::ArgCompletion_NetDemoName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "demos/", true, netDemoExtension, NULL );
}
// RAVEN END

ID_INLINE void idCmdSystem::ArgCompletion_ConfigName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", true, ".cfg", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_SaveGame( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "SaveGames/", true, ".save", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_DemoName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "demos/", true, ".demo", NULL );
}

#endif /* !__CMDSYSTEM_H__ */
