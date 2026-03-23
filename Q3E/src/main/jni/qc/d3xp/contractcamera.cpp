
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_contractcamreset(		"contractcamreset"  );

CLASS_DECLARATION( idAnimatedEntity, idContractCamera )
	EVENT( EV_contractcamreset,		idContractCamera::Event_reset)
	
END_CLASS


void idContractCamera::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(nextStateTime);
	savefile->WriteInt(maxcount);
	savefile->WriteInt(count);
	savefile->WriteObject(frobcube);
	savefile->WriteObject(light);
}

void idContractCamera::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(nextStateTime);
	savefile->ReadInt(maxcount);
	savefile->ReadInt(count);
	savefile->ReadObject(reinterpret_cast<idClass *&>(frobcube));
	savefile->ReadObject(reinterpret_cast<idClass *&>(light));
}

void idContractCamera::Spawn( void )
{
	idDict args;
	idVec3 up;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);

	args.SetVector( "origin",	this->GetPhysics()->GetOrigin() );
	args.Set( "model",			"models/doccam/tris_cm.ase" );
	args.SetInt( "frobbable",	1 );
	args.SetInt( "corpse",		1 );
	args.SetInt( "noGrab",		1 );
	args.Set( "owner",			this->GetName() );
	args.Set( "recordable",		this->spawnArgs.GetString("recordable", "1") );
	//args.SetInt( "index",		i );
	this->frobcube = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	//this->frobcube->isFrobbable = false;
	
	this->SetSkin(declManager->FindSkin("skins/doccam/nocam"));

	state = OFF;
	nextStateTime = 0;

	maxcount = spawnArgs.GetInt("count", "5");
	count = 0;


	
	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() + (up * 5) );
	args.Set( "texture", "lights/monitorglow" );
	args.SetInt("alarmlight", 0);
	args.SetInt("start_off", 1);
	light = ( idLight * )gameLocal.SpawnEntityType( idLight::Type, &args );
	light->SetRadius(15);
	light->Bind(this, true);


	Event_reset();

	BecomeActive( TH_THINK );
}


void idContractCamera::OnFrob( idEntity* activator )
{
	if (state == OFF)
	{		
		StartSound(  "snd_cameraopen" , SND_CHANNEL_ANY, 0, false, NULL );
		nextStateTime = Event_PlayAnim("deploy", 4);
		this->SetSkin(declManager->FindSkin("skins/doccam/cam"));

		this->frobcube->isFrobbable = false;

		this->state = UNFOLDING;
	}
	else if (state == PHOTODONE)
	{
		this->SetSkin(declManager->FindSkin("skins/doccam/nocam"));
		StartSound(  "snd_putaway" , SND_CHANNEL_ANY, 0, false, NULL );

		idStr scriptName = spawnArgs.GetString( "call" );
		const function_t	*scriptFunction;
		scriptFunction = gameLocal.program.FindFunction( scriptName );
		if ( scriptFunction )
		{
			idThread *thread;
			thread = new idThread( scriptFunction );
			thread->DelayedStart( 0 );
		}

		this->state = TAKEN;
	}
	else if (state == TAKEN)
	{
		gameLocal.GetLocalPlayer()->Event_hudMessage( common->GetLanguageDict()->GetString( "#str_02103" ) );
		StartSound(  "snd_error" , SND_CHANNEL_ANY, 0, false, NULL );
	}
}

void idContractCamera::UpdateStates( void )
{
	if (state == UNFOLDING)
	{
		if (gameLocal.GetTime() > nextStateTime)
		{
			state = PHOTOSNAPPING;
		}
	}
	else if (state == PHOTOSNAPPING)
	{
		StartSound(  "snd_camerasnap" , SND_CHANNEL_ANY, 0, false, NULL );
		
		
		light->On();

		nextStateTime = gameLocal.time + 90;
		state = PHOTOSNAPDONE;
		
		count++;
		this->renderEntity.gui[0]->SetStateInt("gui_parm1", count);
	}
	else if (state == PHOTOSNAPDONE)
	{
		if (gameLocal.time > nextStateTime)
		{
			light->Off();

			

			if (count >= maxcount)
			{
				state = PHOTODONE;
				gameLocal.GetLocalPlayer()->Event_hudMessage( common->GetLanguageDict()->GetString( "#str_02110" ) );
				this->frobcube->isFrobbable = true;
				return;
			}

			state = PHOTOFLIPPING;
			nextStateTime = Event_PlayAnim("pageturn", 4);
		}
	}
	else if (state == PHOTOFLIPPING)
	{
		if (nextStateTime < gameLocal.GetTime())
		{
			state = PHOTOSNAPPING;
		}
	}
}

void idContractCamera::Event_reset( void )
{
	this->SetSkin(declManager->FindSkin("skins/doccam/nocam"));
	this->state = OFF;
	this->frobcube->isFrobbable = true;

	count = 0;
	this->renderEntity.gui[0]->SetStateInt("gui_parm1", count);
	this->renderEntity.gui[0]->SetStateInt("gui_parm2", maxcount);
}

void idContractCamera::Think( void )
{
	UpdateStates();
	
	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}




