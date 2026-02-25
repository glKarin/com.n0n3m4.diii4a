
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define		LASERTHINKTIME	200

#define		MAXWIDTH		7
#define		MINWIDTH		0.5


#define		REDCOLOR_R		1
#define		REDCOLOR_G		0.5
#define		REDCOLOR_B		0.5

#define		CAMERAGHOST_LIFETIME			9 //how long ghost sits there.


const idEventDef EV_LaserWire_camOff( "laseroff", "f" );
const idEventDef EV_LaserWire_camOn(  "laseron" );
const idEventDef EV_LaserWire_KillGhost(  "<killghost>" );
const idEventDef EV_LaserWire_laserenable( "laserenable", "f" );



CLASS_DECLARATION( idEntity, idLaserWire )

	EVENT( EV_LaserWire_camOff,			idLaserWire::Event_off)
	EVENT( EV_LaserWire_camOn,			idLaserWire::Event_on)
	EVENT( EV_LaserWire_KillGhost,		idLaserWire::Event_KillGhost)
	EVENT( EV_LaserWire_laserenable,	idLaserWire::Event_laserenable)
END_CLASS


void idLaserWire::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(nextThinkTime);
	savefile->WriteInt(state);
	savefile->WriteInt(dimEndTime);
	savefile->WriteInt(dimStartTime);
	savefile->WriteInt(turnonTime);
	savefile->WriteInt(nextAlarmTime);
	savefile->WriteInt(softAlarmTime);
	savefile->WriteBool(softAlarmReady);

	savefile->WriteObject(ghostImage);

	int i;
	for (i = 0; i < BEAMCOUNT; i++)
	{
		savefile->WriteObject(boundBeam[i]);
		savefile->WriteObject(boundBeamTarget[i]);
	}

	savefile->WriteInt(softAlarmTimeMax);
}

void idLaserWire::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(nextThinkTime);
	savefile->ReadInt(state);
	savefile->ReadInt(dimEndTime);
	savefile->ReadInt(dimStartTime);
	savefile->ReadInt(turnonTime);
	savefile->ReadInt(nextAlarmTime);
	savefile->ReadInt(softAlarmTime);
	savefile->ReadBool(softAlarmReady);

	savefile->ReadObject(reinterpret_cast<idClass *&>(ghostImage));

	int i;
	for (i = 0; i < BEAMCOUNT; i++)
	{
		savefile->ReadObject(reinterpret_cast<idClass *&>(boundBeam[i]));
		savefile->ReadObject(reinterpret_cast<idClass *&>(boundBeamTarget[i]));
	}

	savefile->ReadInt(softAlarmTimeMax);
}

void idLaserWire::Spawn( void ) {
	
	int i;

	int gapdistance = this->spawnArgs.GetInt("gapdistance", "15");


	softAlarmTimeMax =  (int)(this->spawnArgs.GetFloat("softalarmtime", "3") * 1000);

	this->softAlarmReady = false;
	this->nextAlarmTime = 0;
	this->state = IDLE;
	this->nextThinkTime = 0;
	this->dimEndTime = 0;
	this->turnonTime = 0;
	this->softAlarmTime = 0;

	


	//Spawn the beams.
	for (i = 0; i < 5; i++)
	{
		idDict args;
		idVec3 beamOrigin;
		idVec3 forward, right;
		int rightAmount;
		idVec3 beamTargetOrigin;
		trace_t tr;
		bool shouldSkip;

		shouldSkip = false;

		switch (i)
		{
			case 0:
				rightAmount = gapdistance * 2;
				shouldSkip = this->spawnArgs.GetBool("beam0skip", "0");
				break;
			case 1:
				rightAmount = gapdistance;
				shouldSkip = this->spawnArgs.GetBool("beam1skip", "0");
				break;
			case 2:
				rightAmount = 0;
				shouldSkip = this->spawnArgs.GetBool("beam2skip", "0");
				break;
			case 3:
				rightAmount = -gapdistance;
				shouldSkip = this->spawnArgs.GetBool("beam3skip", "0");
				break;
			case 4:
				rightAmount = -gapdistance * 2;
				shouldSkip = this->spawnArgs.GetBool("beam4skip", "0");
				break;
			default:
				rightAmount = 8;
				break;
		}

		if (shouldSkip)
		{
			this->boundBeam[i] = NULL;
			this->boundBeamTarget[i] = NULL;
			continue;
		}

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right);
		beamOrigin = this->GetPhysics()->GetOrigin() + (forward * 1.5) + (right * rightAmount);		

		//TODO: don't spawn beam if it starts in geometry.
		//do the trace. Determine if beam starts off in a solid.
		//gameLocal.clip.TracePoint( tr, beamOrigin, beamOrigin + (forward * 32), /*idBounds(idVec3(-4,-4,-4),idVec3(4,4,4)),*/ MASK_SOLID, this );
		//common->Printf(va("%f %d %f\n", tr.fraction, tr.c.contents, tr.c.dist));

		args.SetVector( "origin", vec3_origin );
		this->boundBeamTarget[i] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );		
		
		args.Clear();
		args.Set(		"target",	boundBeamTarget[i]->name.c_str() );
		args.SetVector(	"origin",	beamOrigin );
		args.SetBool(	"start_off", false );
		args.Set(		"skin",		"skins/beam_red" );
	
		this->boundBeam[i] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->boundBeam[i]->Event_SetWidth( MAXWIDTH );
		this->boundBeam[i]->Bind(this, false);
	}

	//tell it to think.
	BecomeActive( TH_THINK );

	//the flares.
	renderEntity.shaderParms[ SHADERPARM_RED ] = REDCOLOR_R;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = REDCOLOR_G;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = REDCOLOR_B;

	//spawn the Laser Ghost.
	idDict args;
	args.Clear();
	args.SetVector( "origin", vec3_origin );
	args.Set( "classname", "env_cameradummy" );
	args.Set( "solid", "0" );
	gameLocal.SpawnEntityDef( args, &this->ghostImage );

	this->ghostImage->Hide();

	StartSound( "snd_idle" , SND_CHANNEL_RADIO, 0, false, NULL );
}

void idLaserWire::Event_laserenable( int value )
{
	if (value <= 0)
	{
		int i;
		this->state = DISABLED;

		renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
		renderEntity.shaderParms[ SHADERPARM_GREEN ] = 0;
		renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;
		UpdateVisuals();

		for (i = 0; i < 5; i++)
		{
			if (this->boundBeam[i] == NULL)
				continue;

			this->boundBeam[i]->Hide();
		}
	}
	else
	{
		Event_on();
	}
}

void idLaserWire::Event_KillGhost( void )
{
	this->ghostImage->Hide();
}

void idLaserWire::Event_off( float delay )
{




	if (!this->softAlarmReady)
	{
		this->softAlarmReady = true;
		this->softAlarmTime = gameLocal.time + (softAlarmTimeMax);
	}
	

	if (this->state == IDLE)
	{
		float dimTime;
		dimTime = this->spawnArgs.GetFloat("dimtime", "1.3");

		if (dimTime <= 0)
		{
			dimTime = 0.1;
		}

		this->state = DIMMING;
		this->dimStartTime = gameLocal.time;
		this->dimEndTime = gameLocal.time + (dimTime * 1000);
		StartSound( "snd_laserhumdown" , SND_CHANNEL_ANY, 0, false, NULL );
		StopSound(SND_CHANNEL_RADIO, false);
	}

	if (delay <= 0)
	{
		this->turnonTime = gameLocal.time + (9000 * 1000);
	}
	else
	{
		this->turnonTime = gameLocal.time + (delay * 1000);
	}

	uiHackTimer = gameLocal.time + UIHACKTIME;
}

void idLaserWire::Event_on( void )
{
	if (this->state == IDLE)
		return;

	uiHackTimer = gameLocal.time + UIHACKTIME;

	this->state = TURNON;
	this->turnonTime = 0;

	
}

/*
================
idSecurityCamera::Think
================
*/
void idLaserWire::Think( void )
{
	//power down the beams.
	if (this->state == DIMMING)
	{
		int i;
		float beamWidth, curLerp, endTime;
		curLerp = gameLocal.time - this->dimStartTime;
		endTime = this->dimEndTime - this->dimStartTime;

		curLerp = idMath::ClampFloat(0, 1, (float)curLerp / endTime);
		
		beamWidth = MAXWIDTH * (1 - curLerp);

		for (i = 0; i < 5; i++)
		{
			if (this->boundBeam[i] == NULL)
				continue;

			this->boundBeam[i]->Event_SetWidth(beamWidth);
		}

		if (curLerp >= 1)
		{
			idDict args;
			this->state = OFF;

			renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
			renderEntity.shaderParms[ SHADERPARM_GREEN ] = 0;
			renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;
			UpdateVisuals();

			for (i = 0; i < 5; i++)
			{
				if (this->boundBeam[i] == NULL)
					continue;

				this->boundBeam[i]->Hide();

				args.Clear();
				args.SetVector( "origin",		boundBeam[i]->GetPhysics()->GetOrigin() );
				args.Set(		"model",		"sparkburst2.prt" );
				args.SetBool(	"start_off",	false );
				gameLocal.SpawnEntityType( idExplodable::Type, &args );
			}

			StartSound(  "snd_laseroff" , SND_CHANNEL_ANY, 0, false, NULL );
			SetSkin(declManager->FindSkin("skins/laserwire/off"));
		}
	}

	//turn on the beams.
	if ((this->state == DIMMING || this->state == OFF || this->state == TURNON) && gameLocal.time > this->turnonTime)
	{
		int i;

		//turn back on.
		this->state = IDLE;

		//reset flares.
		renderEntity.shaderParms[ SHADERPARM_RED ] = REDCOLOR_R;
		renderEntity.shaderParms[ SHADERPARM_GREEN ] = REDCOLOR_G;
		renderEntity.shaderParms[ SHADERPARM_BLUE ] = REDCOLOR_B;
		UpdateVisuals();

		//reset beams.
		for (i = 0; i < 5; i++)
		{
			if (this->boundBeam[i] == NULL)
				continue;

			this->boundBeam[i]->Event_SetWidth( MAXWIDTH );
			this->boundBeam[i]->Show();
		}

		//disable soft alarm
		this->softAlarmReady = false;

		StartSound( "snd_laseron" , SND_CHANNEL_ANY, 0, false, NULL );
		SetSkin(declManager->FindSkin("skins/laserwire/skin"));

		this->softAlarmTime = 0;

		StartSound( "snd_idle" , SND_CHANNEL_RADIO, 0, false, NULL );
	}

	//check for soft alarm.
	if ((this->state == DIMMING || this->state == OFF) && this->softAlarmTime < gameLocal.time && this->softAlarmReady && softAlarmTimeMax > 0)
	{
		uiAlarmTimer = gameLocal.time + UIALARMTIME;

		//only fire it once. reset soft alarm when it's turned back on.
		this->softAlarmReady = false;

		common->Printf("Event: 3-SECOND ALARM TRIGGERED: %s\n", this->GetName() );
		gameLocal.GetLocalPlayer()->Event_hudMessage( va( common->GetLanguageDict()->GetString( "#str_01203" ), this->GetName() ) );

		//BC 11-23-2013
		//turn on laser after soft alarm.
		Event_on();

		idStr funcname = spawnArgs.GetString( "softalarmtarget", "" );
		if ( funcname.Length() )
		{
			idEntity	*softAlarmEnt;
			softAlarmEnt = gameLocal.FindEntity( funcname );
			if ( softAlarmEnt->RespondsTo( EV_Activate ) || softAlarmEnt->HasSignal( SIG_TRIGGER ) )
			{
				softAlarmEnt->Signal( SIG_TRIGGER );
				softAlarmEnt->ProcessEvent( &EV_Activate, this );
			}
		}

		//pneumo functionality. BLOW IT UP.
		idStr pneumoName = spawnArgs.GetString( "pneumomaster", "" );
		if ( pneumoName.Length() )
		{
			idEntity	*pneumoEnt;
			pneumoEnt = gameLocal.FindEntity( pneumoName );
			if ( pneumoEnt )
			{
				//UseFrob( deckMaster.GetEntity(), "getAndClose");
				gameLocal.GetLocalPlayer()->UseFrob( pneumoEnt, "onLaserHit" );
			}
		}
	}


	//detect player.
	if (nextThinkTime < gameLocal.time && (this->state == DIMMING || this->state == IDLE))
	{
		int i;

		/*
		idPlayer *playerEnt;

		playerEnt = static_cast<idPlayer*>(gameLocal.entities[ 0 ]);

		if ( !playerEnt || ( playerEnt->fl.notarget ) || playerEnt->noclip)
		{
			Present();
			return;
		}
		*/

		//how often to update.
		nextThinkTime = gameLocal.time + LASERTHINKTIME;

		for (i = 0; i < 5; i++)
		{
			idVec3 beamOrigin, beamTarget;
			idVec3 forward;
			trace_t tr;

			if (this->boundBeam[i] == NULL || this->boundBeamTarget[i] == NULL)
				continue;

			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward);

			beamOrigin = this->boundBeam[i]->GetPhysics()->GetOrigin();
			beamTarget = beamOrigin + (forward * 1024);

			//do the trace.
			gameLocal.clip.TracePoint( tr, beamOrigin, beamTarget, MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX, this );
			
			//move the beam target.
			boundBeamTarget[i]->SetOrigin(tr.endpos);
			
			if (g_skill.GetInteger() <= 0)
				continue;

			//check if player is hit.
			
			if (!gameLocal.GetTraceEntity( tr ))
			{
				continue;
			}

			if (gameLocal.GetTraceEntity( tr )->IsType( idWorldspawn::Type ))
			{
				continue;
			}

			if (gameLocal.GetTraceEntity( tr )->IsType( idStaticEntity::Type ))
			{
				continue;
			}

			if (gameLocal.GetTraceEntity( tr )->IsType( idPlayer::Type ))
			{
				idPlayer *playerEnt;
				playerEnt = static_cast<idPlayer*>(gameLocal.GetTraceEntity( tr ));
				if ( playerEnt->fl.notarget || playerEnt->noclip )
				{
					continue;
				}
			}

			//common->Printf("name %s %s\n", 			gameLocal.GetTraceEntity( tr )->Type.super );

			if ( gameLocal.GetTraceEntity( tr )->IsType( idEntity::Type))
			{
				//player is hit.
				boundBeamTarget[i]->SetOrigin(tr.endpos);

				//hit the alarm.
				if (this->nextAlarmTime < gameLocal.time)
				{
					this->nextAlarmTime = gameLocal.time + 2000;
					ActivateTargets(this);

					//pneumo functionality. BLOW IT UP.
					idStr pneumoName = spawnArgs.GetString( "pneumomaster", "" );
					if ( pneumoName.Length() )
					{
						idEntity	*pneumoEnt;
						pneumoEnt = gameLocal.FindEntity( pneumoName );
						if ( pneumoEnt )
						{
							//UseFrob( deckMaster.GetEntity(), "getAndClose");
							gameLocal.GetLocalPlayer()->UseFrob( pneumoEnt, "onLaserHit" );
						}
					}	

					uiAlarmTimer = gameLocal.time + UIALARMTIME;


					common->Printf("Event: ALARM TRIGGERED: %s\n", this->GetName() );
					gameLocal.GetLocalPlayer()->Event_hudMessage( va( common->GetLanguageDict()->GetString( "#str_01204" ), this->GetName() ) );

					CancelEvents( &EV_LaserWire_KillGhost );
					
					if (gameLocal.GetTraceEntity( tr )->IsType( idPlayer::Type))
					{
						this->ghostImage->SetOrigin( gameLocal.GetTraceEntity( tr )->GetPhysics()->GetOrigin() );

						//set the ghost angle.
						idAngles playerAng;
						playerAng.yaw = static_cast<idPlayer *>( gameLocal.GetTraceEntity( tr ) )->viewAngles.yaw;
						//playerAng.yaw = gameLocal.GetTraceEntity( tr )->GetPhysics()->GetAxis().ToAngles().yaw;
						playerAng.pitch = 0;
						playerAng.roll = 0;
						this->ghostImage->SetAxis( playerAng.ToMat3() );

						PostEventSec( &EV_LaserWire_KillGhost, CAMERAGHOST_LIFETIME );
					
						this->ghostImage->Show();
					}
				}
			}
		}
	}

	Present();
}



/*
================
idSecurityCamera::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void idLaserWire::Present( void )
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
