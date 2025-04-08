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

#ifndef __EDIT_PUBLIC_H__
#define __EDIT_PUBLIC_H__

/*
===============================================================================

	Editors.

===============================================================================
*/


class	idProgram;
class	idInterpreter;


// Radiant Level Editor
void	RadiantInit( void );
void	RadiantShutdown( void );
void	RadiantRun( void );
void	RadiantPrint( const char *text );
void	RadiantSync( const char *mapName, const idVec3 &viewOrg, const idAngles &viewAngles );


// in-game Light Editor
void	LightEditorInit( const idDict *spawnArgs );
void	LightEditorShutdown( void );
void	LightEditorRun( void );


// in-game Sound Editor
void	SoundEditorInit( const idDict *spawnArgs );
void	SoundEditorShutdown( void );
void	SoundEditorRun( void );


// in-game Articulated Figure Editor
void	AFEditorInit( const idDict *spawnArgs );
void	AFEditorShutdown( void );
void	AFEditorRun( void );


// in-game Particle Editor
void	ParticleEditorInit( const idDict *spawnArgs );
void	ParticleEditorShutdown( void );
void	ParticleEditorRun( void );


// in-game Script Editor
void	ScriptEditorInit( const idDict *spawnArgs );
void	ScriptEditorShutdown( void );
void	ScriptEditorRun( void );


// in-game Declaration Browser
void	DeclBrowserInit( const idDict *spawnArgs );
void	DeclBrowserShutdown( void );
void	DeclBrowserRun( void );
void	DeclBrowserReloadDeclarations( void );


// GUI Editor
void	GUIEditorInit( void );
void	GUIEditorShutdown( void );
void	GUIEditorRun( void );
bool	GUIEditorHandleMessage( void *msg );


// Script Debugger
void	DebuggerClientLaunch( void );
void	DebuggerClientInit( const char *cmdline );
bool	DebuggerServerInit( void );
void	DebuggerServerShutdown( void );
void	DebuggerServerPrint( const char *text );
void	DebuggerServerCheckBreakpoint( idInterpreter *interpreter, idProgram *program, int instructionPointer );

//Material Editor
void	MaterialEditorInit( void );
void	MaterialEditorRun( void );
void	MaterialEditorShutdown( void );
void	MaterialEditorPrintConsole( const char *msg );

#endif /* !__EDIT_PUBLIC_H__ */
