
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"




CLASS_DECLARATION( idStaticEntity, idFrobCube )
	

END_CLASS

void idFrobCube::Save( idSaveGame *savefile ) const
{
	savefile->WriteString(functionName);
	savefile->WriteString(masterName);
}

void idFrobCube::Restore( idRestoreGame *savefile )
{
	savefile->ReadString(functionName);
	savefile->ReadString(masterName);

	//reinterpret_cast<idClass *&>(
}

void idFrobCube::Spawn( void )
{
	
	if ( spawnArgs.GetBool( "solid", "0" ) )
	{
		this->GetPhysics()->SetContents( CONTENTS_SOLID );
	}
	

	functionName = NULL;
	functionName = spawnArgs.GetString("funcName");

	if (!functionName)
	{
		gameLocal.Warning( "idFrobCube '%s' at (%s): cannot find funcName.", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	masterName = NULL;
	masterName = spawnArgs.GetString("owner");

	if (!masterName)
	{
		gameLocal.Warning( "idFrobCube '%s' at (%s): cannot find owner.", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}
}



void idFrobCube::OnFrob( idEntity* activator )
{
	//bc 3-28-2016 hide a specific thing.
	idStr hideName = spawnArgs.GetString("hidename");
	if (hideName)
	{
		idEntity *hideEnt = gameLocal.FindEntity( hideName );

		if (hideEnt)
		{
			hideEnt->Hide();
		}
	}


	

	//call script.
	idStr scriptName = spawnArgs.GetString( "call" );
	const function_t	*scriptFunction;
	scriptFunction = gameLocal.program.FindFunction( scriptName );
	if ( scriptFunction )
	{
		idThread *thread;
		thread = new idThread( scriptFunction );
		thread->DelayedStart( 0 );
	}




	if (spawnArgs.GetBool("gettable"))
	{
		gameLocal.GetLocalPlayer()->StartSound( "snd_get", SND_CHANNEL_ANY, 0, false, NULL );
		this->Hide();

		idDict args;
		args.Clear();
		args.SetVector( "origin",		this->GetPhysics()->GetOrigin() );
		args.Set(		"model",	    "pickupdust.prt" );
		args.SetBool(	"start_off",	false );
		gameLocal.SpawnEntityType( idExplodable::Type, &args );

		return;
	}






	StartSound( "snd_frob", SND_CHANNEL_ANY, 0, false, NULL );



	idEntity *ownerEnt = gameLocal.FindEntity( masterName );

	if (!ownerEnt)
	{
		//gameLocal.Warning( "idFrobCube '%s' at (%s): owner '%s' doesn't exist.", name.c_str(), GetPhysics()->GetOrigin().ToString(0), masterName );
		return;
	}

	gameLocal.GetLocalPlayer()->UseFrob(ownerEnt, functionName);


	

}