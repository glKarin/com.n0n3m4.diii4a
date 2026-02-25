
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#ifdef STEAM
#include "steamstats.h"
#endif

const idEventDef EV_powersaw_spawndelay( "<powersaw_spawndelay>" );
const idEventDef EV_powersaw_reset( "powersawreset" );

CLASS_DECLARATION( idMover, idPowerSawGeneric )
	
	EVENT( EV_powersaw_spawndelay,		idPowerSawGeneric::Spawn2)
	EVENT( EV_powersaw_reset,			idPowerSawGeneric::reset)
END_CLASS


void idPowerSawGeneric::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(nextSparktime);
	savefile->WriteString(callName.c_str());
	savefile->WriteObject(mover);
	savefile->WriteObject(splineEnt);
	savefile->WriteObject(sparks);
	savefile->WriteObject(light);
}

void idPowerSawGeneric::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(nextSparktime);
	savefile->ReadString(callName);
	savefile->ReadObject(reinterpret_cast<idClass *&>(mover));
	savefile->ReadObject(reinterpret_cast<idClass *&>(splineEnt));
	savefile->ReadObject(reinterpret_cast<idClass *&>(sparks));
	savefile->ReadObject(reinterpret_cast<idClass *&>(light));
}

void idPowerSawGeneric::Spawn2( void )
{
	idDict args;

	idStr splineName = spawnArgs.GetString( "spline" );
	splineName.StripTrailingWhitespace();
	splineName.StripLeading(' ');
	if ( !splineName.Length() )
	{
		gameLocal.Error( "idPowerSawGeneric '%s' at (%s) doesn't have 'spline' key specified", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	splineEnt = gameLocal.FindEntity( splineName );
	

	if (!splineEnt)
	{
		gameLocal.Error( "idPowerSawGeneric '%s' at (%s) can't find spline '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), splineName.c_str() );
	}

	splineEnt->Hide();

	int movetime = spawnArgs.GetFloat("movetime", "6");
	int acceltime = spawnArgs.GetFloat("acceltime", "2");
	int deceltime = spawnArgs.GetFloat("deceltime", "1");

	args.Clear();
	args.SetVector( "origin", splineEnt->GetPhysics()->GetOrigin() );
	args.Set( "model", "models/powersaw/tris.ase" );
	args.SetInt( "solid", 0 );
	args.SetInt( "frobbable", 0 );
	args.Set( "onsplineend", this->GetName() );
	mover = ( idMover * )gameLocal.SpawnEntityType( idMover::Type, &args );
	//mover->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );
	mover->Event_SetMoveTime( movetime );
	mover->Event_SetAccellerationTime( acceltime);
	mover->Event_SetDecelerationTime( deceltime );


	args.Clear();
	args.SetVector("origin", mover->GetPhysics()->GetOrigin() );
	args.SetInt( "noshadows", 1);
	args.Set("texture", "lights/powersaw" );
	light = ( idLight * )gameLocal.SpawnEntityType( idLight::Type, &args );
	light->SetRadius(64);
	light->Event_FadeLight(0.3f, idVec3(0.8,0.5,0));
	light->Event_Off();	
	light->Bind(mover, false);
	
	


	if (spawnArgs.GetInt("ceiling", "0") == 1)
	{
		idAngles moverAng = mover->GetPhysics()->GetAxis().ToAngles();
		moverAng.roll += 180;
		mover->SetAngles(moverAng);
	}

	mover->Hide();

	state = IDLE;
	nextSparktime = 0;

	BecomeActive( TH_THINK );
}

void idPowerSawGeneric::Spawn( void )
{
	state = IDLE;

	//force a little delay to let all world entities spawn in.
	PostEventSec( &EV_powersaw_spawndelay, 0.3f );
}

void idPowerSawGeneric::Think( void )
{
	

	if (gameLocal.time > nextSparktime && state == SAWING)
	{
		idDict args;

		nextSparktime = gameLocal.time + 2500;
		
		args.Clear();
		args.SetVector( "origin",		this->mover->GetPhysics()->GetOrigin() );
		args.Set(		"model",		"sparkloop.prt" );
		args.SetBool(	"start_off",	false );
		sparks = gameLocal.SpawnEntityType( idExplodable::Type, &args );
		sparks->Bind(mover, false);

		StartSound(  "snd_sawhit" , SND_CHANNEL_BODY2, 0, false, NULL );
	}

	idMover::Think();
}



void idPowerSawGeneric::OnSplineEnd( void )
{
	if (state == IDLE)
	{
		//if the grate is reset during sawing, then we end up here.
		return;
	}

	//BC 7-20-2016 fix saveload crash. do not spline end if it's already done.
	if (state == DONE)
	{
		return;
	}

	state = DONE;

	sparks->PostEventMS( &EV_Remove, 0 );

	mover->Hide();
	light->Event_Off();

	StartSound( "snd_sawend" , SND_CHANNEL_ANY, 0, false, NULL );
	StopSound( SND_CHANNEL_BODY2, false );
	StopSound( SND_CHANNEL_BODY3, false );
	
	idStr scriptName = spawnArgs.GetString( "call" );
	const function_t	*scriptFunction;
	scriptFunction = gameLocal.program.FindFunction( scriptName );
	if ( scriptFunction )
	{
		idThread *thread;
		thread = new idThread( scriptFunction );
		thread->DelayedStart( 0 );
	}

	
}

void idPowerSawGeneric::Frob( void )
{

	//BC 5-15-2015
	//auto detect whether buzzsaw appears on floor or ceiling.
	if (spawnArgs.GetInt("ceiling") >= 2)
	{
		float eye_z = gameLocal.GetLocalPlayer()->GetEyePosition().z;

		float grate_z = GetPhysics()->GetOrigin().z;

		if (grate_z > eye_z)
		{
			//ceiling.
			idAngles moverAng = mover->GetPhysics()->GetAxis().ToAngles();
			moverAng.roll += 180;
			mover->SetAngles(moverAng);
		}
		else
		{
			//floor.
		}		
	}




	this->isFrobbable = false;
	
	//start the mover.
	mover->Show();
	mover->Event_StartSpline(splineEnt);
	light->Event_On();

	StartSound(  "snd_sawloop" , SND_CHANNEL_BODY3, 0, false, NULL );

	state = SAWING;

#ifdef STEAM
	g_SteamStats->SetStatDelta("stat_buzzsaws", 1);
#endif
}

void idPowerSawGeneric::reset( void )
{
	state = IDLE;
	nextSparktime = 0;
	this->isFrobbable = true;

	this->Event_StopMoving();

	mover->Event_StopMoving();
	mover->Hide();
	light->Event_Off();


	this->GetPhysics()->SetOrigin( this->originalPosition);
	this->GetPhysics()->SetAxis( this->originalAngle);
	this->UpdateVisuals();
}