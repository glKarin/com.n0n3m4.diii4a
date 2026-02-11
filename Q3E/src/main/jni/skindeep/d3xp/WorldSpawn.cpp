/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "framework/FileSystem.h"

#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"

#include "WorldSpawn.h"

/*
================
idWorldspawn

Worldspawn class.  Each map has one worldspawn which handles global spawnargs.
Every map should have exactly one worldspawn.
================
*/
CLASS_DECLARATION( idEntity, idWorldspawn )
	EVENT( EV_Remove,				idWorldspawn::Event_Remove )
	EVENT( EV_SafeRemove,			idWorldspawn::Event_Remove )
END_CLASS

/*
================
idWorldspawn::Spawn
================
*/
void idWorldspawn::Spawn( void ) {
	idStr				scriptname;
	idThread			*thread;
	const function_t	*func;
	//const idKeyValue	*kv;

	assert( gameLocal.world == NULL );
	gameLocal.world = this;

	g_gravity.SetFloat( spawnArgs.GetFloat( "gravity", va( "%f", DEFAULT_GRAVITY ) ) );
	gameLocal.SetGravityFromCVar();

	// disable stamina on hell levels
	if ( spawnArgs.GetBool( "no_stamina" ) == true )
	{
		pm_stamina.SetFloat( 0.0f );
	}

	// load script
	scriptname = gameLocal.GetMapName();
	scriptname.SetFileExtension( ".script" );
	if ( fileSystem->ReadFile( scriptname, NULL, NULL ) > 0 ) {
		gameLocal.program.CompileFile( scriptname );

		// call the main function by default
		func = gameLocal.program.FindFunction( "main" );
		if ( func != NULL ) {
			thread = new idThread( func );
			thread->DelayedStart( 0 );
		}
	}

	// call any functions specified in worldspawn

	//BC this default behavior calls ALL call commands. Removing this, so that it ONLY calls 'call' with no suffix
	/*
	kv = spawnArgs.MatchPrefix( "call" );
	while( kv != NULL ) {
		func = gameLocal.program.FindFunction( kv->GetValue() );
		if ( func == NULL ) {
			gameLocal.Error( "Function '%s' not found in script for '%s' key on worldspawn", kv->GetValue().c_str(), kv->GetKey().c_str() );
		}

		thread = new idThread( func );
		thread->DelayedStart( 0 );
		kv = spawnArgs.MatchPrefix( "call", kv );
	}*/

	//BC new script call function.
	idStr callScript = spawnArgs.GetString("call");
	if (callScript.Length() > 0)
	{
		func = gameLocal.program.FindFunction(callScript.c_str());
		if (func == NULL)
		{
			gameLocal.Error("Function '%s' not found in script for 'call' key on worldspawn", callScript.c_str());
		}

		thread = new idThread(func);
		thread->DelayedStart(0);
	}
	






	if (g_spawnfilter_usemapsetting.GetBool())
	{
		idStr customSpawnfilter = spawnArgs.GetString("spawnfilter", "");
		if (customSpawnfilter[0] != '\0')
		{
			cvarSystem->SetCVarString("g_spawnfilter", customSpawnfilter.c_str());
		}
		else
		{
			cvarSystem->SetCVarString("g_spawnfilter", "");
		}
	}

	InitializeSpawnFilter();


	showEventLogInfoFeed = spawnArgs.GetBool("showeventlog", "1");

	doSpacePush = spawnArgs.GetBool("spacepush", "1");
}

/*
=================
idWorldspawn::Save
=================
*/
void idWorldspawn::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( spawnfilterTable.Num() ); // idHashTable<idStrList> spawnfilterTable
	for (int idx = 0; idx < spawnfilterTable.Num(); idx++)
	{
		idStr outKey;
		idStrList outVal;
		spawnfilterTable.GetIndex( idx, &outKey, &outVal );
		savefile->WriteString( outKey );
		SaveFileWriteArray( outVal, outVal.Num(), WriteString );
	}

	savefile->WriteBool( showEventLogInfoFeed ); // bool showEventLogInfoFeed
	savefile->WriteBool( doSpacePush ); // bool doSpacePush

	// blendo eric: set in gamelocal, but we can just write it out here
	savefile->WriteBool( g_enableSlowmo.GetBool() ); // g_enableSlowmo
	savefile->WriteBool( g_hideHudInternal.GetBool() ); // g_hideHudInternal

}

/*
=================
idWorldspawn::Restore
=================
*/
void idWorldspawn::Restore( idRestoreGame *savefile ) {
	assert( gameLocal.world == this );

#if 0
	// blendo eric: this might be more accurate than grabbing from spawnargs?
	float fval;
	savefile->ReadFloat( fval );
	g_gravity.SetFloat( fval );
#else
	g_gravity.SetFloat( spawnArgs.GetFloat( "gravity", va( "%f", DEFAULT_GRAVITY ) ) );
#endif

#if 0 // blendo eric: this is set in player
	// disable stamina on hell levels
	if ( spawnArgs.GetBool( "no_stamina" ) ) {
		pm_stamina.SetFloat( 0.0f );
	}
#endif

	int num;
	savefile->ReadInt( num ); // idHashTable<idStrList> spawnfilterTable
	for (int idx = 0; idx < num; idx++) {
		idStrList outVal;
		idStr outKey;
		savefile->ReadString( outKey );
		SaveFileReadList( outVal, ReadString ); 
		spawnfilterTable.Set( outKey, outVal );
	}

	savefile->ReadBool( showEventLogInfoFeed ); // bool showEventLogInfoFeed
	savefile->ReadBool( doSpacePush ); // bool doSpacePush

	// blendo eric: set in gamelocal, but we can just write it out here
	bool bVal;
	savefile->ReadBool( bVal ); // g_enableSlowmo
	g_enableSlowmo.SetBool( bVal );
	savefile->ReadBool( bVal ); // g_hideHudInternal
	g_hideHudInternal.SetBool( bVal );


}

/*
================
idWorldspawn::~idWorldspawn
================
*/
idWorldspawn::~idWorldspawn() {
	if ( gameLocal.world == this ) {
		gameLocal.world = NULL;
	}
}

/*
================
idWorldspawn::Event_Remove
================
*/
void idWorldspawn::Event_Remove( void ) {
	gameLocal.Error( "Tried to remove world" );
}


//BC
//This creates the filter of entities we want to PROHIBIT from spawning. This is used for adjusting how difficult/complex levels are.
//The data for this is editable via the .def of target_meta.
void idWorldspawn::InitializeSpawnFilter()
{
	bool foundValue = false;

	for (int i = 0; i < spawnArgs.GetNumKeyVals(); ++i)
	{
		const idKeyValue* kv = spawnArgs.GetKeyVal(i);
		if (kv->GetKey().Find("def_spawnfilter_") != -1)
		{
			idStr rawValues = kv->GetValue();
			rawValues.StripTrailingWhitespace();
			rawValues.StripLeading(' ');

			idStrList valueList;
			for (int j = rawValues.Length(); j >= 0; j--)
			{
				if (rawValues[j] == ',')
				{
					valueList.Append(rawValues.Mid(j + 1, rawValues.Length()).c_str());
					rawValues = rawValues.Left(j); //Truncate the string and continue.
				}
				else if (j <= 0)
				{
					valueList.Append(rawValues.Mid(0, rawValues.Length()).c_str());
				}
			}

			//TODO: do safety check of verifying the entity type actually exists?

			idStr spawnKey = kv->GetKey();
			spawnKey.StripLeading("def_spawnfilter_");
			idStr key = spawnKey;
			spawnfilterTable.Set(key, valueList);

			foundValue = true;
		}
	}

	if (!foundValue)
	{
		gameLocal.Error("spawnfilter: unable to initialize '%s'", g_spawnfilter.GetString());
	}
}