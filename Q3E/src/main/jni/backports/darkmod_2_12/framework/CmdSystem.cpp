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



/*
===============================================================================

	idCmdSystemLocal

===============================================================================
*/

typedef struct commandDef_s {
	struct commandDef_s *	next;
	char *					name;
	cmdFunction_t			function;
	argCompletion_t			argCompletion;
	int						flags;
	char *					description;
} commandDef_t;


class idCmdSystemLocal : public idCmdSystem {
public:
	virtual void			Init( void ) override;
	virtual void			Shutdown( void ) override;

	virtual void			AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t argCompletion = NULL ) override;
	virtual void			RemoveCommand( const char *cmdName ) override;
	virtual void			RemoveFlaggedCommands( int flags ) override;

	virtual void			CommandCompletion( void(*callback)( const char *s ) ) override;
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) override;
	void					ExecuteCommandText( const char* text );
	virtual void			AppendCommandText( const char* text ) override;

	virtual void			BufferCommandText( cmdExecution_t exec, const char *text ) override;
	virtual void			ExecuteCommandBuffer( void ) override;

	virtual void			ArgCompletion_FolderExtension( const idCmdArgs &args, void(*callback)( const char *s ), const char *folder, bool stripFolder, ... ) override;
	virtual void			ArgCompletion_DeclName( const idCmdArgs &args, void(*callback)( const char *s ), int type ) override;

	virtual void			BufferCommandArgs( cmdExecution_t exec, const idCmdArgs &args ) override;

	virtual void			SetupReloadEngine( const idCmdArgs &args ) override;
	virtual bool			PostReloadEngine( void ) override;

	void					SetWait( int numFrames ) { wait = numFrames; }
	commandDef_t *			GetCommands( void ) const { return commands; }

private:
	static const int		MAX_CMD_BUFFER = 0x10000;

	commandDef_t *			commands;

	int						wait;
	int						textLength;
	byte					textBuf[MAX_CMD_BUFFER];

	idStr					completionString;
	idStrList				completionParms;

	// piggybacks on the text buffer, avoids tokenize again and screwing it up
	idList<idCmdArgs>		tokenizedCmds;

	// a command stored to be executed after a reloadEngine and all associated commands have been processed
	idCmdArgs				postReload;

private:	
	void					ExecuteTokenizedString( const idCmdArgs &args );
	void					InsertCommandText( const char *text );

	static void				ListByFlags( const idCmdArgs &args, cmdFlags_t flags );
	static void				List_f( const idCmdArgs &args );
	static void				SystemList_f( const idCmdArgs &args );
	static void				RendererList_f( const idCmdArgs &args );
	static void				SoundList_f( const idCmdArgs &args );
	static void				GameList_f( const idCmdArgs &args );
	static void				ToolList_f( const idCmdArgs &args );
	static void				Exec_f( const idCmdArgs &args );
	static void				Vstr_f( const idCmdArgs &args );
	static void				Echo_f( const idCmdArgs &args );
	static void				Parse_f( const idCmdArgs &args );
	static void				Wait_f( const idCmdArgs &args );
	static void				PrintMemInfo_f( const idCmdArgs &args );
};

idCmdSystemLocal			cmdSystemLocal;
idCmdSystem *				cmdSystem = &cmdSystemLocal;


/*
============
idCmdSystemLocal::ListByFlags
============
*/
// NOTE: the const wonkyness is required to make msvc happy
template<>
ID_INLINE int idListSortCompare( const commandDef_t * const *a, const commandDef_t * const *b ) {
	return idStr::Icmp( (*a)->name, (*b)->name );
}

void idCmdSystemLocal::ListByFlags( const idCmdArgs &args, cmdFlags_t flags ) {
	int i;
	idStr match;
	const commandDef_t *cmd;
	idList<const commandDef_t *> cmdList;

	if ( args.Argc() > 1 ) {
		match = args.Args( 1, -1 );
		match.Remove( ' ' );
	} else {
		match = "";
	}

	for ( cmd = cmdSystemLocal.GetCommands(); cmd; cmd = cmd->next ) {
		if ( !( cmd->flags & flags ) ) {
			continue;
		}
		if ( match.Length() && idStr( cmd->name ).Filter( match, false ) == 0 ) {
			continue;
		}

		cmdList.Append( cmd );
	}

	cmdList.Sort();

	for ( i = 0; i < cmdList.Num(); i++ ) {
		cmd = cmdList[i];

		common->Printf( "  %-21s %s\n", cmd->name, cmd->description );
	}

	common->Printf( "%i commands\n", cmdList.Num() );
}

/*
============
idCmdSystemLocal::List_f
============
*/
void idCmdSystemLocal::List_f( const idCmdArgs &args ) {
	idCmdSystemLocal::ListByFlags( args, CMD_FL_ALL );
}

/*
============
idCmdSystemLocal::SystemList_f
============
*/
void idCmdSystemLocal::SystemList_f( const idCmdArgs &args ) {
	idCmdSystemLocal::ListByFlags( args, CMD_FL_SYSTEM );
}

/*
============
idCmdSystemLocal::RendererList_f
============
*/
void idCmdSystemLocal::RendererList_f( const idCmdArgs &args ) {
	idCmdSystemLocal::ListByFlags( args, CMD_FL_RENDERER );
}

/*
============
idCmdSystemLocal::SoundList_f
============
*/
void idCmdSystemLocal::SoundList_f( const idCmdArgs &args ) {
	idCmdSystemLocal::ListByFlags( args, CMD_FL_SOUND );
}

/*
============
idCmdSystemLocal::GameList_f
============
*/
void idCmdSystemLocal::GameList_f( const idCmdArgs &args ) {
	idCmdSystemLocal::ListByFlags( args, CMD_FL_GAME );
}

/*
============
idCmdSystemLocal::ToolList_f
============
*/
void idCmdSystemLocal::ToolList_f( const idCmdArgs &args ) {
	idCmdSystemLocal::ListByFlags( args, CMD_FL_TOOL );
}

/*
===============
idCmdSystemLocal::Exec_f
===============
*/
void idCmdSystemLocal::Exec_f( const idCmdArgs &args ) {
	char *	f;
	idStr	filename;

	if ( args.Argc () != 2 ) {
		common->Printf( "exec <filename> : execute a script file\n" );
		return;
	}

	filename = args.Argv(1);
	filename.DefaultFileExtension( ".cfg" );
	fileSystem->ReadFile( filename, reinterpret_cast<void **>(&f), NULL );
	if ( !f ) {
		common->Printf( "Couldn't exec %s - file does not exist.\n", args.Argv(1) );
		return;
	}
	common->Printf( "execing %s\n", args.Argv(1) );
	
	cmdSystemLocal.BufferCommandText( CMD_EXEC_INSERT, f );

	fileSystem->FreeFile( f );
}

/*
===============
idCmdSystemLocal::Vstr_f

Inserts the current value of a cvar as command text
===============
*/
void idCmdSystemLocal::Vstr_f( const idCmdArgs &args ) {
	const char *v;

	if ( args.Argc () != 2 ) {
		common->Printf( "vstr <variablename> : execute a variable command\n" );
		return;
	}

	v = cvarSystem->GetCVarString( args.Argv( 1 ) );

	cmdSystemLocal.BufferCommandText( CMD_EXEC_APPEND, va( "%s\n", v ) );
}

/*
===============
idCmdSystemLocal::Echo_f

Just prints the rest of the line to the console
===============
*/
void idCmdSystemLocal::Echo_f( const idCmdArgs &args ) {
	int		i;
	
	for ( i = 1; i < args.Argc(); i++ ) {
		common->Printf( "%s ", args.Argv( i ) );
	}
	common->Printf( "\n" );
}

/*
============
idCmdSystemLocal::Wait_f

Causes execution of the remainder of the command buffer to be delayed until next frame.
============
*/
void idCmdSystemLocal::Wait_f( const idCmdArgs &args ) {
	if ( args.Argc() == 2 ) {
		cmdSystemLocal.SetWait( atoi( args.Argv( 1 ) ) );
	} else {
		cmdSystemLocal.SetWait( 1 );
	}
}

/*
============
idCmdSystemLocal::Parse_f

This just prints out how the rest of the line was parsed, as a debugging tool.
============
*/
void idCmdSystemLocal::Parse_f( const idCmdArgs &args ) {
	int		i;

	for ( i = 0; i < args.Argc(); i++ ) {
		common->Printf( "%i: %s\n", i, args.Argv(i) );
	}
}

/*
============
idCmdSystemLocal::Init
============
*/
void idCmdSystemLocal::Init( void ) {

	AddCommand( "listCmds", List_f, CMD_FL_SYSTEM, "lists commands" );
	AddCommand( "listSystemCmds", SystemList_f, CMD_FL_SYSTEM, "lists system commands" );
	AddCommand( "listRendererCmds", RendererList_f, CMD_FL_SYSTEM, "lists renderer commands" );
	AddCommand( "listSoundCmds", SoundList_f, CMD_FL_SYSTEM, "lists sound commands" );
	AddCommand( "listGameCmds", GameList_f, CMD_FL_SYSTEM, "lists game commands" );
	AddCommand( "listToolCmds", ToolList_f, CMD_FL_SYSTEM, "lists tool commands" );
	AddCommand( "exec", Exec_f, CMD_FL_SYSTEM, "executes a config file", ArgCompletion_ConfigName );
	AddCommand( "vstr", Vstr_f, CMD_FL_SYSTEM, "inserts the current value of a cvar as command text" );
	AddCommand( "echo", Echo_f, CMD_FL_SYSTEM, "prints text" );
	AddCommand( "parse", Parse_f, CMD_FL_SYSTEM, "prints tokenized string" );
	AddCommand( "wait", Wait_f, CMD_FL_SYSTEM, "delays remaining buffered commands one or more frames" );

	completionString = "*";

	textLength = 0;
    memset (textBuf, 0, MAX_CMD_BUFFER);
}

/*
============
idCmdSystemLocal::Shutdown
============
*/
void idCmdSystemLocal::Shutdown( void ) {
	commandDef_t *cmd;

	for ( cmd = commands; cmd; cmd = commands ) {
		commands = commands->next;
		Mem_Free( cmd->name );
		Mem_Free( cmd->description );
		delete cmd;
	}

	completionString.ClearFree();
	completionParms.ClearFree();
	tokenizedCmds.ClearFree();
	postReload.Clear();
}

/*
============
idCmdSystemLocal::AddCommand
============
*/
void idCmdSystemLocal::AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t argCompletion ) {
	commandDef_t *cmd;
	
	// fail if the command already exists
	for ( cmd = commands; cmd; cmd = cmd->next ) {
		if ( idStr::Cmp( cmdName, cmd->name ) == 0 ) {
			if ( function != cmd->function ) {
			    common->Printf( "idCmdSystemLocal::AddCommand: %s already defined\n", cmdName );
			}
			return;
		}
	}

	cmd = new commandDef_t;
	cmd->name = Mem_CopyString( cmdName );
	cmd->function = function;
	cmd->argCompletion = argCompletion;
	cmd->flags = flags;
	cmd->description = Mem_CopyString( description );
	cmd->next = commands;
	commands = cmd;
}

/*
============
idCmdSystemLocal::RemoveCommand
============
*/
void idCmdSystemLocal::RemoveCommand( const char *cmdName ) {
	commandDef_t *cmd, **last;

	for ( last = &commands, cmd = *last; cmd; cmd = *last ) {
		if ( idStr::Cmp( cmdName, cmd->name ) == 0 ) {
			*last = cmd->next;
			Mem_Free( cmd->name );
			Mem_Free( cmd->description );
			delete cmd;
			return;
		}
		last = &cmd->next;
	}
}

/*
============
idCmdSystemLocal::RemoveFlaggedCommands
============
*/
void idCmdSystemLocal::RemoveFlaggedCommands( int flags ) {
	commandDef_t *cmd, **last;

	for ( last = &commands, cmd = *last; cmd; cmd = *last ) {
		if ( cmd->flags & flags ) {
			*last = cmd->next;
			Mem_Free( cmd->name );
			Mem_Free( cmd->description );
			delete cmd;
			continue;
		}
		last = &cmd->next;
	}
}

/*
============
idCmdSystemLocal::CommandCompletion
============
*/
void idCmdSystemLocal::CommandCompletion( void(*callback)( const char *s ) ) {
	commandDef_t *cmd;
	
	for ( cmd = commands; cmd; cmd = cmd->next ) {
		callback( cmd->name );
	}
}

/*
============
idCmdSystemLocal::ArgCompletion
============
*/
void idCmdSystemLocal::ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) {
	commandDef_t *cmd;
	idCmdArgs args;

	args.TokenizeString( cmdString, false );

	for ( cmd = commands; cmd; cmd = cmd->next ) {
		if ( !cmd->argCompletion ) {
			continue;
		}
		if ( idStr::Icmp( args.Argv( 0 ), cmd->name ) == 0 ) {
			cmd->argCompletion( args, callback );
			break;
		}
	}
}

/*
============
idCmdSystemLocal::ExecuteTokenizedString
============
*/
void idCmdSystemLocal::ExecuteTokenizedString( const idCmdArgs &args ) {	
	commandDef_t *cmd, **prev;
	
	// execute the command line
	if ( !args.Argc() ) {
		return;		// no tokens
	}

	// check registered command functions	
	for ( prev = &commands; *prev; prev = &cmd->next ) {
		cmd = *prev;
		if ( idStr::Icmp( args.Argv( 0 ), cmd->name ) == 0 ) {
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev = cmd->next;
			cmd->next = commands;
			commands = cmd;

			// perform the action
			if ( !cmd->function ) {
				break;
			} else {
				cmd->function( args );
			}
			return;
		}
	}
	
	// check cvars
	if ( cvarSystem->Command( args ) ) {
		return;
	}

	common->Printf( "Unknown command '%s'\n", args.Argv( 0 ) );
}

/*
============
idCmdSystemLocal::ExecuteCommandText

Tokenizes, then executes.
============
*/
void idCmdSystemLocal::ExecuteCommandText( const char *text ) {	
	ExecuteTokenizedString( idCmdArgs( text, false ) );
}

/*
============
idCmdSystemLocal::InsertCommandText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void idCmdSystemLocal::InsertCommandText( const char *text ) {
	int		i;

	int len = static_cast<int>(strlen( text )) + 1;
	if ( len + textLength > (int)sizeof( textBuf ) ) {
		common->Printf( "idCmdSystemLocal::InsertText: buffer overflow\n" );
		return;
	}

	// move the existing command text
	for ( i = textLength - 1; i >= 0; i-- ) {
		textBuf[ i + len ] = textBuf[ i ];
	}

	// copy the new text in
	memcpy( textBuf, text, len - 1 );

	// add a \n
	textBuf[ len - 1 ] = '\n';

	textLength += len;
}

/*
============
idCmdSystemLocal::AppendCommandText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void idCmdSystemLocal::AppendCommandText( const char *text ) {
		
    int l = static_cast<int>(strlen(text));

	if ( textLength + l >= (int)sizeof( textBuf ) ) {
		common->Printf( "idCmdSystemLocal::AppendText: buffer overflow\n" );
		return;
	}
	memcpy( textBuf + textLength, text, l );
	textLength += l;
}

/*
============
idCmdSystemLocal::BufferCommandText
============
*/
void idCmdSystemLocal::BufferCommandText( cmdExecution_t exec, const char *text ) {
	switch( exec ) {
		case CMD_EXEC_NOW: {
			ExecuteCommandText( text );
			break;
		}
		case CMD_EXEC_INSERT: {
			InsertCommandText( text );
			break;
		}
		case CMD_EXEC_APPEND: {
			AppendCommandText( text );
			break;
		}
		default: {
			common->FatalError( "idCmdSystemLocal::BufferCommandText: bad exec type" );
		}
	}
}

/*
============
idCmdSystemLocal::BufferCommandArgs
============
*/
void idCmdSystemLocal::BufferCommandArgs( cmdExecution_t exec, const idCmdArgs &args ) {
	switch ( exec ) {
		case CMD_EXEC_NOW: {
			ExecuteTokenizedString( args );
			break;
		}
		case CMD_EXEC_APPEND: {
			AppendCommandText( "_execTokenized\n" );
			tokenizedCmds.Append( args );
			break;
		}
		default: {
			common->FatalError( "idCmdSystemLocal::BufferCommandArgs: bad exec type" );
		}
	}
}

/*
============
idCmdSystemLocal::ExecuteCommandBuffer
============
*/
void idCmdSystemLocal::ExecuteCommandBuffer( void ) {
	int			i;
	char *		text;
	int			quotes;
	idCmdArgs	args;

	while( textLength ) {

		if ( wait )	{
			// skip out while text still remains in buffer, leaving it for next frame
			wait--;
			break;
		}

		// find a \n or ; line break
		text = (char *)textBuf;

		quotes = 0;
		for ( i = 0; i < textLength; i++ ) {
			if ( text[i] == '"' ) {
				quotes++;
			}
			if ( !( quotes & 1 ) &&  text[i] == ';' ) {
				break;	// don't break if inside a quoted string
			}
			if ( text[i] == '\n' || text[i] == '\r' || text[i] == 0 ) {
				break;
			}
		}
			
		text[i] = 0;

		if ( !idStr::Cmp( text, "_execTokenized" ) ) {
			args = tokenizedCmds[ 0 ];
			tokenizedCmds.RemoveIndex( 0 );
		} else {
			args.TokenizeString( text, false );
		}

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec) can insert data at the
		// beginning of the text buffer
        
		if ( i == textLength ) {
			textLength = 0;
            memset (textBuf, 0, MAX_CMD_BUFFER);
		} else {
            i++;
			textLength -= i;
			memmove( text, text+i, textLength );
		}

		// execute the command line that we have already tokenized
		ExecuteTokenizedString( args );
	}
}

/*
============
idCmdSystemLocal::ArgCompletion_FolderExtension
============
*/
void idCmdSystemLocal::ArgCompletion_FolderExtension( const idCmdArgs &args, void(*callback)( const char *s ), const char *folder, bool stripFolder, ... ) {
	int i;
	idStr string;
	const char *extension;
	va_list argPtr;

	string = args.Argv( 0 );
	string += " ";
	string += args.Argv( 1 );

    // taaaki: ensure that we clear the auto-complete list for the (d)map commands in case missions are
    //         installed/uninstalled while the game is running
	if ( string.Icmp( completionString ) != 0 || string.Icmp( "map " ) == 0 || string.Icmp( "dmap " ) == 0 ) {
		idStr parm, path;
		idFileList *names;
        const char* gamedir = NULL;

        // check if a fan mission has been set and that the "map" or "dmap" command is being used
        idStr currFm = cvarSystem->GetCVarString("fs_currentfm");
        if ( currFm.Icmp( "darkmod" ) != 0 && ( string.Icmp( "map " ) == 0 || string.Icmp( "dmap " ) == 0 ) ) {
            gamedir = currFm.c_str();
        }

		completionString = string;
		completionParms.Clear();

		parm = args.Argv( 1 );
		parm.ExtractFilePath( path );
		if ( stripFolder || path.Length() == 0 ) {
			path = folder + path;
		}
		path.StripTrailing( '/' );

        // taaaki: don't include folders if we are looking for the currentfm .map file
        //         this is a bit of a hack :/
        if ( !gamedir ) {
            names = fileSystem->ListFiles( path, "/", true, true, gamedir );

		    for ( i = 0; i < names->GetNumFiles(); i++ ) {
			    idStr name = names->GetFile( i );
			    if ( stripFolder ) {
				    name.Strip( folder );
			    } else {
				    name.Strip( "/" );
			    }
			    name = args.Argv( 0 ) + ( " " + name ) + "/";
			    completionParms.Append( name );
		    }
		    fileSystem->FreeFileList( names );
        }

		// list files
		va_start( argPtr, stripFolder );
		for ( extension = va_arg( argPtr, const char * ); extension; extension = va_arg( argPtr, const char * ) ) {
			names = fileSystem->ListFiles( path, extension, true, true, gamedir );
			for ( i = 0; i < names->GetNumFiles(); i++ ) {
				idStr name = names->GetFile( i );
				if ( stripFolder ) {
					name.Strip( folder );
				} else {
					name.Strip( "/" );
				}
				name = args.Argv( 0 ) + ( " " + name );
				completionParms.Append( name );
			}
			fileSystem->FreeFileList( names );
		}
		va_end( argPtr );
	}
	for ( i = 0; i < completionParms.Num(); i++ ) {
		callback( completionParms[i] );
	}
}

/*
============
idCmdSystemLocal::ArgCompletion_DeclName
============
*/
void idCmdSystemLocal::ArgCompletion_DeclName( const idCmdArgs &args, void(*callback)( const char *s ), int type ) {
	int i, num;

	if ( declManager == NULL ) {
		return;
	}
	num = declManager->GetNumDecls( (declType_t)type );
	for ( i = 0; i < num; i++ ) {
		callback( idStr( args.Argv( 0 ) ) + " " + declManager->DeclByIndex( (declType_t)type, i , false )->GetName() );
	}
}

/*
============
idCmdSystemLocal::SetupReloadEngine
============
*/
void idCmdSystemLocal::SetupReloadEngine( const idCmdArgs &args ) {
	BufferCommandText( CMD_EXEC_APPEND, "reloadEngine menu\n" ); // greebo: disable the system console during reloadEngine
	postReload = args;
}

/*
============
idCmdSystemLocal::PostReloadEngine
============
*/
bool idCmdSystemLocal::PostReloadEngine( void ) {
	if ( !postReload.Argc() ) {
		return false;
	}
	BufferCommandArgs( CMD_EXEC_APPEND, postReload );
	postReload.Clear();
	return true;
}
