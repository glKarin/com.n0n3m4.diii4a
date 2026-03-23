
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define		CYCLETIME		2
#define		ACCELTIME		10
#define		DECELTIME		6

#define		FLYFORCE		256
#define		FLYTIME			1000



CLASS_DECLARATION( idStaticEntity, idAirlock )
	

END_CLASS

void idAirlock::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool(initialized);
	savefile->WriteInt(nextFlingTime);
	savefile->WriteInt(lastOpenTime);
	savefile->WriteBool(lastOpenState);
	//savefile->WriteString(functionName);
	//savefile->WriteString(masterName);

	savefile->WriteObject(carousel);
	savefile->WriteObject(leverCarousel);
	savefile->WriteObject(frobcubeCarousel);
	savefile->WriteObject(light);
	savefile->WriteObject(door_round);
	savefile->WriteObject(door_inner);
	savefile->WriteObject(bin);
	savefile->WriteObject(vacuumSeparator1);
	savefile->WriteObject(vacuumSeparator2);
}

void idAirlock::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool(initialized);
	savefile->ReadInt(nextFlingTime);
	savefile->ReadInt(lastOpenTime);
	savefile->ReadBool(lastOpenState);
	//savefile->ReadString(functionName);
	//savefile->ReadString(masterName);

	savefile->ReadObject(reinterpret_cast<idClass *&>(carousel));
	savefile->ReadObject(reinterpret_cast<idClass *&>(leverCarousel));
	savefile->ReadObject(reinterpret_cast<idClass *&>(frobcubeCarousel));
	savefile->ReadObject(reinterpret_cast<idClass *&>(light));
	savefile->ReadObject(reinterpret_cast<idClass *&>(door_round));
	savefile->ReadObject(reinterpret_cast<idClass *&>(door_inner));
	savefile->ReadObject(reinterpret_cast<idClass *&>(bin));
	savefile->ReadObject(reinterpret_cast<idClass *&>(vacuumSeparator1));
	savefile->ReadObject(reinterpret_cast<idClass *&>(vacuumSeparator2));
}

void idAirlock::Spawn( void )
{
	idDict args;
	idVec3 forward, right, up;
	idVec3 leverPos;

	BecomeActive( TH_THINK );

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() );
	args.Set( "model", "models/airlock/carousel.ase" );
	args.Set( "snd_accel", "m1_accel" );
	args.Set( "snd_decel", "m1_decel" );
	args.Set( "snd_move", "m1_move" );
	carousel = ( idMover * )gameLocal.SpawnEntityType( idMover::Type, &args );
	carousel->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );
	carousel->Event_SetMoveTime( CYCLETIME );
	carousel->Event_SetAccellerationTime( ACCELTIME );
	carousel->Event_SetDecelerationTime( DECELTIME );

	leverPos = this->GetPhysics()->GetOrigin() + (forward * -66) + (up * 64);
	args.Clear();
	args.SetVector( "origin", leverPos );
	args.Set( "model", "env_lever3" );
	leverCarousel = ( idAnimated * )gameLocal.SpawnEntityType( idAnimated::Type, &args );
	leverCarousel->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );
	leverCarousel->Bind(carousel, true);

	args.Clear();
	args.SetVector( "origin", leverPos );
	args.Set( "classname", "frobcube" );
	args.Set( "model", "models/frob/cube_16x16.ase" );
	args.Set( "owner", this->GetName() );
	args.SetInt( "arg", 1);
	gameLocal.SpawnEntityDef( args, &frobcubeCarousel );
	frobcubeCarousel->Bind(carousel, true);

	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() /*+ (forward * 72)*/ + (up * 64) );
	args.Set( "texture", "lights/fan01" );
	args.SetInt("alarmlight", 0);
	args.SetInt("noshadows", 1);
	args.SetVector("light_radius", idVec3(112,112,112));
	args.SetVector("light_center", idVec3(0,0,-110));
	light = ( idLight * )gameLocal.SpawnEntityType( idLight::Type, &args );
	light->Bind(carousel, true);

	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() + (forward * 86) + (up * 6) );
	args.Set( "model", "models/airlock/door_round.ase" );
	args.SetInt( "no_touch", 1 );
	//args.SetInt( "movedir", right.ToAngles().yaw );
	args.SetInt( "movedir", -1 );
	args.SetFloat( "time", 1.5f );
	args.SetInt( "lip", 32 );
	args.Set( "owner", this->GetName() );
	args.SetInt( "arg", 3);
	args.Set("snd_close",	"d4_open");
	args.Set("snd_open",	"d4_open");
	args.Set("snd_opened",	"d4_close");
	args.Set("snd_closed",	"d4_close");
	//args.Set("snd_move",	"d4_move");
	door_round = ( idDoor * )gameLocal.SpawnEntityType( idDoor::Type, &args );
	door_round->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );


	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() + (forward * -88) + (up * 6) );
	args.Set( "model", "models/airlock/door_inner.ase" );
	args.SetInt( "no_touch", 1 );
	//args.SetInt( "movedir", right.ToAngles().yaw );
	args.SetInt( "angle", right.ToAngles().yaw + 90 );
	args.SetInt( "movedir", right.ToAngles().yaw );
	args.SetFloat( "time", 1.0f );
	args.SetInt( "lip", 3 );
	args.Set( "owner", this->GetName() );
	args.SetInt( "arg", 2);
	args.Set("snd_close",	"d3_open");
	args.Set("snd_open",	"d3_open");
	args.Set("snd_opened",	"d3_close");
	args.Set("snd_closed",	"d3_close");
	door_inner = ( idDoor * )gameLocal.SpawnEntityType( idDoor::Type, &args );
	//door_inner->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );


	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() + (forward * -88) + (up * 64) );
	vacuumSeparator1 = ( idVacuumSeparatorEntity * )gameLocal.SpawnEntityType( idVacuumSeparatorEntity::Type, &args );

	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() + (forward * 86) + (up * 64) );
	vacuumSeparator2 = ( idVacuumSeparatorEntity * )gameLocal.SpawnEntityType( idVacuumSeparatorEntity::Type, &args );

	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() + (up * 56) + (forward * -67) + (right * -42) );
	args.Set( "model", "models/airlock/bin.ase" );
	args.Set( "clipmodel", "models/airlock/bin_cm.ase" );
	args.SetInt( "frobbable", 1 );
	args.SetInt( "corpse", 1 );
	args.SetInt( "noGrab", 1 );
	args.Set( "owner", this->GetName() );
	args.SetInt( "arg", 9);
	this->bin = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );

	//this->bin->BecomeActive( TH_PHYSICS );
	idAngles ang = this->GetPhysics()->GetAxis().ToAngles();
	ang.yaw -= 34;
	this->bin->GetPhysics()->SetAxis(ang.ToMat3() );
	this->bin->UpdateVisuals();
	

	nextFlingTime = 0;
	lastOpenTime = 0;
	lastOpenState = false;

	initialized = false;
}

void idAirlock::Think( void )
{
	if (!initialized)
	{
		door_inner->SetPortalState(false);
		door_round->SetPortalState(false);
		initialized = true;
	}

	if (door_round->IsOpen())
	{
		if (!lastOpenState)
		{
			//open now.
			lastOpenTime = gameLocal.time + FLYTIME;
		}

		if (gameLocal.time > lastOpenTime)
		{
			Present();
			return;
		}

		if (gameLocal.time < nextFlingTime)
		{
			Present();
			return;
		}

		nextFlingTime = gameLocal.time + 100;

		//outer door is open. suck out items.
		idEntity	*entityList[ MAX_GENTITIES ];
		int			i;
		int			listedEntities;
		idVec3		frameBottom;
		idVec3		doorBottom;

		idVec3 flyDestination, forward, up;

		//determine where items should fly to.
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
		
		//fly to the centerpoint between bottom of rounddoor and the doorframe.
		frameBottom = this->GetPhysics()->GetOrigin() + (forward * 86) + (up * 6);
		doorBottom = door_round->GetPhysics()->GetOrigin();

		//gameRenderWorld->DebugCircle(idVec4(1,1,0,1), frameBottom, idVec3(0,0,1), 4, 9, 300);
		//gameRenderWorld->DebugCircle(idVec4(0,1,0,1), doorBottom, idVec3(0,0,1), 4, 9, 300);



		flyDestination = (frameBottom + doorBottom) * 0.5f; //midpoint between door + frame
		//flyDestination = (flyDestination + doorBottom) * 0.5f; //midpoint between midpoint + door
		flyDestination += (forward * 32);
		
		//gameRenderWorld->DebugCircle(idVec4(1,0,0,1), flyDestination, idVec3(0,0,1), 4, 9, 300);


		listedEntities = gameLocal.EntitiesWithinRadius( GetPhysics()->GetOrigin(), 96, entityList, MAX_GENTITIES );

		for (i = 0; i < listedEntities; i++)
		{
			idEntity *ent = entityList[ i ];

			if (!ent)
			{
				continue;
			}

			//ensure it's within Z values.
			float ent_zpos = ent->GetPhysics()->GetOrigin().z;
			float this_zpos = this->GetPhysics()->GetOrigin().z;

			//add a couple units of buffer, just in case.
			if (ent_zpos < this_zpos - 2 /*floor*/ || ent_zpos > this_zpos + 130 /*ceiling*/)
			{
				continue;
			}

			if ( ent->IsType( idMoveableItem::Type ) || ent->IsType( idMoveable::Type ) )
			{
				bool shouldPush = true;
				if (gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.IsValid())
				{
					int dragentID = gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.GetEntityNum();
					if (dragentID == ent->entityNumber)
					{
						shouldPush = false;
					}
				}

				if (shouldPush)
				{
					idVec3 flyDir = flyDestination - ent->GetPhysics()->GetOrigin();
					flyDir.Normalize();

					ent->GetPhysics()->SetLinearVelocity(flyDir * FLYFORCE);
				}
			}
		}
	}

	lastOpenState = door_round->IsOpen();
	
	Present();
}

void idAirlock::OnFrob( idEntity* activator )
{
	int arg = activator->spawnArgs.GetInt("arg");

	if (arg == 1)
	{
		//carousel lever.
		light->Event_FadeLight(0.3f, idVec3(.62,.9,.9));

		//if is moving, then abort.
		if (carousel->Event_IsRotating())
		{
			//play a boop-boop error sound.
		}
		else
		{
			leverCarousel->Event_PlayAnim("push", 4);

            idAngles angle(0, 180, 0);
			carousel->Event_RotateOnce(angle);
		}

		door_inner->Close();
		door_round->Close();
	}
	else if (arg == 2)
	{
		//inner door.
		light->Event_FadeLight(0.3f, idVec3(.62,.9,.9));

		idVec3 forward;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward);

		if ( idMath::Fabs( carousel->GetPhysics()->GetAxis().ToAngles().yaw - forward.ToAngles().yaw + 180) > 2
			&& !carousel->Event_IsRotating())
		{
            idAngles angle(0, forward.ToAngles().yaw + 180, 0);
			carousel->Event_RotateTo(angle);
		}
	}
	else if (arg == 3)
	{
		//outer door.
		idVec3 up, forward;
		idDict args;
		idEntity *particles;
		idAngles particlesAngles(0,0,0);

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

		if (!door_round->IsOpen())
		{
			light->Event_FadeLight(0.3f, idVec3(.7,0,0));

			//particles.
			args.Clear();
			args.SetVector( "origin",		this->GetPhysics()->GetOrigin() + (up * 48) + (forward * 32));
			args.Set(		"model",		"vacuum.prt" );
			args.SetBool(	"start_off",	false );
			particles = gameLocal.SpawnEntityType( idExplodable::Type, &args );
			particlesAngles.pitch = 90;
			particlesAngles.yaw = this->GetPhysics()->GetAxis().ToAngles().yaw;
			particles->SetAngles(particlesAngles);

			door_round->StartSoundShader(  declManager->FindSound(spawnArgs.GetString("snd_vacuum")) , SND_CHANNEL_DEMONIC, 0, false, NULL );
		}



		if ( idMath::Fabs( carousel->GetPhysics()->GetAxis().ToAngles().yaw - forward.ToAngles().yaw) > 2
			&& !carousel->Event_IsRotating())
		{
            idAngles angle(0, forward.ToAngles().yaw, 0);
			carousel->Event_RotateTo(angle);
		}
		
	}
	else if (arg == 9)
	{
		if (gameLocal.GetLocalPlayer()->PowerUpActive(ENVIROSUIT))
		{
			gameLocal.GetLocalPlayer()->ClearPowerup(ENVIROSUIT);
			bin->StartSoundShader(  declManager->FindSound(spawnArgs.GetString("snd_usebin")) , SND_CHANNEL_ANY, 0, false, NULL );
		}
		else
		{
			bin->StartSoundShader(  declManager->FindSound(spawnArgs.GetString("snd_cancelbin")), SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
}



void idAirlock::Present( void )
{
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) )
	{
		return;
	}

	BecomeInactive( TH_UPDATEVISUALS );
	
	// if set to invisible, skip
	if ( !renderEntity.hModel || IsHidden() )
	{
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 )
	{
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}
	else
	{
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}