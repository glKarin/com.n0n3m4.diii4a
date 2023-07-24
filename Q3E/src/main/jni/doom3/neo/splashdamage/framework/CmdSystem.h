// Copyright (C) 2007 Id Software, Inc.
//

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

#include "../common/common.h"

class idCmdArgs;

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
typedef void ( *cmdFunction_t )( const idCmdArgs& );

typedef void ( *argCompletionCallback_t )( const char* );

// argument completion function
typedef void ( *argCompletion_t )( const idCmdArgs&, argCompletionCallback_t );

typedef sdFunctions::sdCallable< void( const idCmdArgs& ) > frameCommandCallback_t;
typedef sdFunctions::sdBinderMember0< void, const idCmdArgs&, sdFunctions::sdEmptyType, sdFunctions::sdEmptyType > frameCommandBind_t;

class idCmdSystem {
public:
	virtual				~idCmdSystem( void ) {}

	virtual void		Init( void ) = 0;
	virtual void		Shutdown( void ) = 0;

						// Registers a command and the function to call for it.
	virtual void		AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t completion = NULL ) = 0;
						// Removes a command.
	virtual void		RemoveCommand( const char *cmdName ) = 0;
						// Remove all commands with one of the flags set.
	virtual void		RemoveFlaggedCommands( int flags ) = 0;

						// Adds an alias that will be executed
	virtual void		AddAlias( const char* alias, const char* command ) = 0;
						// Removes an alias
	virtual void		RemoveAlias( const char* alias ) = 0;
						// Find the alias by searching for its value, returns "" if nothing matches
	virtual const char*	FindAliasForValue( const char* value ) const = 0;

	virtual void		HandleFrameCommands( frameCommandCallback_t callback ) = 0;
	virtual void		PushFrameCommand( const char* command ) = 0;

						// Command and argument completion using callback for each valid string.
	virtual void		CommandCompletion( argCompletionCallback_t ) = 0;
	virtual void		ArgCompletion( const char *cmdString, argCompletionCallback_t ) = 0;

						// Adds command text to the command buffer, does not add a final \n
	virtual void		BufferCommandText( cmdExecution_t exec, const char *text ) = 0;
						// Pulls off \n \r or ; terminated lines of text from the command buffer and
						// executes the commands. Stops when the buffer is empty.
						// Normally called once per frame, but may be explicitly invoked.
	virtual void		ExecuteCommandBuffer( bool allowWait = true ) = 0;

						// Adds to the command buffer in tokenized form ( CMD_EXEC_NOW or CMD_EXEC_APPEND only )
	virtual void		BufferCommandArgs( cmdExecution_t exec, const idCmdArgs &args ) = 0;

						// Setup a reloadEngine to happen on next command run, and give a command to execute after reload
	virtual void		SetupReloadEngine( const idCmdArgs &args ) = 0;
	virtual bool		PostReloadEngine( void ) = 0;

						// Base for path/file auto-completion.
	virtual void		ArgCompletion_FolderExtension( const idCmdArgs &args, argCompletionCallback_t, const char *folder, bool stripFolder, ... ) = 0;
						// Base for decl name auto-completion.
	virtual void		ArgCompletion_DeclName( const idCmdArgs &args, argCompletionCallback_t, const char* typeName ) = 0;

						// Default argument completion functions.
	static void			ArgCompletion_Boolean( const idCmdArgs &args, argCompletionCallback_t );
	template<int min,int max>
	static void			ArgCompletion_Integer( const idCmdArgs &args, argCompletionCallback_t );
	template<const char **strings>
	static void			ArgCompletion_String( const idCmdArgs &args, argCompletionCallback_t );

	static void			ArgCompletion_StartGame( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_FileName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_EntitiesName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_WorldName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_ModelName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_SoundName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_SoundShader( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_ImageName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_VideoName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_ConfigName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_RenderDemoName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_NetworkDemoName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_JumpStartDemoName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_StringMap( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_StuffGeneratorName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_AtmosphereName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_MaterialName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_ImposterGeneratorName( const idCmdArgs &args, argCompletionCallback_t );
	static void			ArgCompletion_RenderlightName( const idCmdArgs &args, argCompletionCallback_t );

/*	jrad - in-progress work to get rid of the above hardcoded stuff and let plugins register their own types
	this will likely cause a change to the commandDef_t structure, moving away from function pointers and towards
	using callable entities like functors

	This is a rough sketch of how it'd go down...

	class sdCompletion {
	public:
		virtual ~sdCompletion();
		virtual void operator()( const idCmdArgs& args, argCompletionCallback_t callback ) = 0;
	}

	class sdFileCompletion {
	public:
		sdFileCompletion( const char* extension_, const char* folder_ ) {
			extension = static_cast< const char* >( Mem_CopyString( extension_ ));
			folder = static_cast< const char* >( Mem_CopyString( folder_ ));
		}

		~sdFileCompletion() {
			Mem_Free( extension );
			Mem_Free( folder );
		}

		void operator()( const idCmdArgs& args, argCompletionCallback_t callback ) {
			cmdSystem->ArgCompletion_FolderExtension( args, callback, folder, true, extension, NULL );
		}

	private:
		const char* extension;
		const char* folder;
	};
*/
};

extern idCmdSystem *	cmdSystem;

ID_INLINE void idCmdSystem::ArgCompletion_Boolean( const idCmdArgs &args, argCompletionCallback_t callback ) {
	callback( va( "%s 0", args.Argv( 0 ) ) );
	callback( va( "%s 1", args.Argv( 0 ) ) );
}

template<int min,int max> ID_STATIC_TEMPLATE ID_INLINE void idCmdSystem::ArgCompletion_Integer( const idCmdArgs &args, argCompletionCallback_t callback ) {
	for ( int i = min; i <= max; i++ ) {
		callback( va( "%s %d", args.Argv( 0 ), i ) );
	}
}

template<const char **strings> ID_STATIC_TEMPLATE ID_INLINE void idCmdSystem::ArgCompletion_String( const idCmdArgs &args, argCompletionCallback_t callback ) {
	for ( int i = 0; strings[i]; i++ ) {
		callback( va( "%s %s", args.Argv( 0 ), strings[i] ) );
	}
}

ID_INLINE void idCmdSystem::ArgCompletion_FileName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", true, "", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_EntitiesName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "maps/", true, ".entities", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_WorldName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "maps/", true, ".world", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_ModelName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "models/", false, ".lwo", ".ase", ".md5mesh", ".obj", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_SoundName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "sounds/", false, ".wav", ".ogg", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_SoundShader( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, "sound" );
}

ID_INLINE void idCmdSystem::ArgCompletion_ImageName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", false, ".tga", ".dds", ".pcx", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_VideoName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "video/", false, ".roq", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_ConfigName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", true, ".cfg", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_RenderDemoName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "demos/", true, ".demo", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_NetworkDemoName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "demos/", true, ".ndm", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_JumpStartDemoName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "demos/", true, ".jsd", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_StringMap( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, "stringMap" );
}

ID_INLINE void idCmdSystem::ArgCompletion_StuffGeneratorName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "stuff/", false, ".sg", NULL );
}

ID_INLINE void idCmdSystem::ArgCompletion_AtmosphereName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, "atmosphere" );
}

ID_INLINE void idCmdSystem::ArgCompletion_MaterialName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, "material" );
}

ID_INLINE void idCmdSystem::ArgCompletion_ImposterGeneratorName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, "imposterGenerator" );
}

ID_INLINE void idCmdSystem::ArgCompletion_RenderlightName( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "renderlight/", true, ".rlt", NULL );
}

#endif /* !__CMDSYSTEM_H__ */
