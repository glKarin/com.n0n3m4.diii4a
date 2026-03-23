
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define		WAKEUPTIME		1100		//activation time.

#define		MAXVOLLEY		5			//how many shots in one volley.
#define		WARMUPTIME		1900		//how long the pre-volley warning warmup is.
#define		VOLLEYDELAY		250			//ms between each shot in a volley.


#define		IDLESOUNDDELAY	2000
#define		IDLESOUNDRAND	2000

#define		MUZZLEFLASHTIME	0.05
#define		BEAMWIDTH		4.0f


#define		PITCHADJUSTMENT		-8 //aim a little below the eyes.

#define		THROWTIME_THRESHOLD	1000	//thrown objects are valid targets for X milliseconds after being thrown.

const idEventDef EV_Turret_activate( "turretactivate", "d" );
const idEventDef EV_Turret_isactive( "turretisactive", NULL, 'd' );
const idEventDef EV_Turret_muzzleflashoff( "turretmuzzleoff", NULL );


CLASS_DECLARATION( idAnimatedEntity, idTurret )

	EVENT( EV_Turret_activate,			idTurret::Event_activate)
	EVENT( EV_Turret_isactive,			idTurret::Event_isactive)
	EVENT( EV_Turret_muzzleflashoff,	idTurret::MuzzleflashOff)

END_CLASS

void idTurret::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(attackState);
	savefile->WriteInt(volleyCount);
	savefile->WriteInt(nextIdleSound);
	savefile->WriteMat3(bodyAxis);
	savefile->WriteMat3(turretAxis);

	savefile->WriteObject(beamStart);
	savefile->WriteObject(beamEnd);
	savefile->WriteObject(laserdot);

	//BC 7-29-2016	
	savefile->WriteInt(nextStateTime);
	targetEnt.Save(savefile);
}

void idTurret::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(attackState);
	savefile->ReadInt(volleyCount);
	savefile->ReadInt(nextIdleSound);
	savefile->ReadMat3(bodyAxis);
	savefile->ReadMat3(turretAxis);

	savefile->ReadObject(reinterpret_cast<idClass *&>(beamStart));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beamEnd));
	savefile->ReadObject(reinterpret_cast<idClass *&>(laserdot));

	//BC 7-29-2016	
	savefile->ReadInt(nextStateTime);
	targetEnt.Restore(savefile);
	
}

void idTurret::Spawn( void )
{
	jointHandle_t			bodyJoint, turretJoint;
	idVec3 origin;
	idDict args;
	idVec3 turretPos;

	this->state = OFF;
	this->attackState = IDLE;
	this->nextStateTime = 0;
	this->volleyCount = 0;
	this->nextIdleSound = 0;

	targetEnt = NULL;

	BecomeActive( TH_THINK );
	
	bodyJoint = animator.GetJointHandle( "body" );
	animator.GetJointTransform( bodyJoint, gameLocal.time, origin, this->bodyAxis );

	turretJoint = animator.GetJointHandle( "turret" );
	animator.GetJointTransform( turretJoint, gameLocal.time, turretPos, this->turretAxis );
	turretPos = this->GetPhysics()->GetOrigin() + turretPos * this->GetPhysics()->GetAxis();


	SetSkin(declManager->FindSkin( "skins/turret/skin" ));

	//spawn beam end.
	args.SetVector( "origin", vec3_origin );
	beamEnd = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );		
	
	
	
	//spawn beam start.
	args.Clear();
	args.Set(		"target",	beamEnd->name.c_str() );
	args.SetVector(	"origin",    turretPos  );
	args.SetBool(	"start_off", true );
	args.Set(		"skin",		"skins/beam_turret" );
	args.SetFloat(	"width",	BEAMWIDTH );
	
	beamStart = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
	beamStart->BindToJoint(this, turretJoint, false);
	beamStart->BecomeActive(TH_PHYSICS);
	beamStart->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] =	spawnArgs.GetVector("_color").x;
	beamStart->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = spawnArgs.GetVector("_color").y;
	beamStart->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] =	spawnArgs.GetVector("_color").z;
	beamStart->Hide();




	args.Clear();
	args.SetVector( "origin", vec3_origin );
	args.Set( "model", "models/lasersight/tris.ase" );
	args.SetInt( "solid", 0 );
	laserdot = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	laserdot->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] =	spawnArgs.GetVector("_color").x;
	laserdot->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] =	spawnArgs.GetVector("_color").y;
	laserdot->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] =	spawnArgs.GetVector("_color").z;
	laserdot->Hide();

	laserdot->SetOrigin(beamEnd->GetPhysics()->GetOrigin());
	laserdot->Bind(beamEnd, false);



	if (spawnArgs.GetBool( "start_on", "0" ))
	{
		Event_activate( 1 );
	}


	//precache the projectile.


}

void idTurret::Event_isactive()
{
	idThread::ReturnInt( this->state );
}

void idTurret::Event_activate( int value )
{
	if (value == 1)
	{
		if (state == ON)
		{
			return;
		}

		//open.
		Event_PlayAnim("opening", 4);
		this->state = ON;
		StartSound( "snd_opening", SND_CHANNEL_ANY, 0, false, NULL );

		this->nextStateTime = gameLocal.time + WAKEUPTIME;
		this->nextIdleSound = gameLocal.time + 2000;
	}
	else
	{
		if (state == OFF)
		{
			return;
		}

		//close up.
		int doneTime = Event_PlayAnim("closing", 4);
		this->state = CLOSING;
		this->nextStateTime = doneTime;

		StartSound( "snd_closing", SND_CHANNEL_ANY, 0, false, NULL );

		this->beamStart->Hide();
		this->laserdot->Hide();
	}
}

void idTurret::MuzzleflashOff( void )
{
	SetSkin(declManager->FindSkin( "skins/turret/skin" ));
}

void idTurret::GotoWarmupState( void )
{
	this->attackState = WARMUP;
	StartSound( "snd_warmup", SND_CHANNEL_ANY, 0, false, NULL );
	this->volleyCount = 0;
	this->nextStateTime = gameLocal.time + WARMUPTIME;
}

bool idTurret::CheckTargetValidity(idEntity *ent)
{
	if (CheckTargetLOS(ent, vec3_zero))
	{
		return true;
	}
	
	//first check failed. so now do the fallback offset check.
	idBounds bounds = ent->GetPhysics()->GetBounds();
	idVec3			tmax;
	tmax[2] = bounds[1][2] ;

	tmax[2]--; //shrink it a little.

	if (CheckTargetLOS(ent, idVec3(0,0,tmax[2])))
	{
		return true;
	}

	//both checks have failed. abort.
	return false;
}

bool idTurret::CheckTargetLOS(idEntity *ent, idVec3 offset)
{
	//check distance.
	idVec3 selfPos = GetPhysics()->GetOrigin() + idVec3(0,0,-12);
	float distanceToTarget = (	ent->GetPhysics()->GetOrigin() - selfPos ).Normalize();

	float maxDist = spawnArgs.GetInt("maxrange", "1024");
	if (maxDist < 0)
		maxDist = 1024;

	if (distanceToTarget >= maxDist)
	{
		//too far. Out of range.
		return false;
	}

	idVec3 forward, right, up;
	ent->GetPhysics()->GetAxis().ToAngles().ToVectors( &forward, &right, &up );

	idVec3 offsetPosition = vec3_zero;
	offsetPosition += (forward * offset.x) + (right * offset.y) + (up * offset.z);


	//check LOS.
	trace_t tr;
	gameLocal.clip.TracePoint( tr, selfPos, ent->GetPhysics()->GetOrigin() + offsetPosition, CONTENTS_OPAQUE, this );

	if (tr.fraction < 0)
	{
		//blocked.
		return false;
	}


	idVec3 movedir = (ent->GetPhysics()->GetOrigin() + offsetPosition) - selfPos;
	movedir.Normalize();

	

	int i;
	idVec3 hitpoint = selfPos;
	int maxGlassPanes = 32;
	for (i = 0; i < maxGlassPanes; i++)
	{
		trace_t paneTr;
		idVec3 startTrace = hitpoint + (movedir * 1.0f);
		gameLocal.clip.TracePoint( paneTr, startTrace, startTrace + (movedir * 4096), MASK_SOLID | MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX, this );


		//gameRenderWorld->DebugArrow(idVec4(1,1,0,1), startTrace, paneTr.endpos, 16, 5000);

		
		if ( paneTr.c.material != NULL )
		{
			surfTypes_t materialType = paneTr.c.material->GetSurfaceType();

			if (materialType == SURFTYPE_GLASS)
			{
				hitpoint = paneTr.endpos;
				continue;
			}
		}

		if (paneTr.c.entityNum == ent->entityNumber)
		{
			break;
		}

		//if (gameLocal.entities[paneTr.c.entityNum]->IsType(idDoor::Type) || gameLocal.entities[paneTr.c.entityNum]->IsType( idWorldspawn::Type ))
		{
			return false;
		}
	}

	
	//turret has a valid target. Start the gunfire sequence.
	if ( developer.GetBool() )
	{
		gameRenderWorld->DebugArrow(idVec4(1,0,0,1), selfPos, ent->GetPhysics()->GetOrigin() + offsetPosition, 16, 5000);
	}

	return true;
}

void idTurret::UpdateStates( void )
{
	if (this->state == CLOSING)
	{
		idAnimatedEntity::Think();

		if (gameLocal.time > this->nextStateTime)
		{
			this->state = OFF;
		}

		return;
	}
	else if (this->state == ON)
	{
		jointHandle_t			bodyJoint, turretJoint;
		idRotation				bodyRotation, turretRotation;
		idVec3					playerPos;
		idVec3					turretPos;
		idMat3					turretAxis;

		bodyJoint = animator.GetJointHandle( "body" );
		turretJoint = animator.GetJointHandle( "turret" );
		
		
		
		//get yaw.
		if (targetEnt.IsValid())
		{
			playerPos = targetEnt.GetEntity()->GetPhysics()->GetOrigin();
		}
		else
		{
			playerPos = gameLocal.GetLocalPlayer()->GetEyePosition();
			playerPos[2] += PITCHADJUSTMENT; //aim a little below the eyes
		}


		idVec3 diff = playerPos - this->GetPhysics()->GetOrigin();
		float yaw = diff.ToYaw();
		
		//BC 9-14-2015 fix bad facing.
		yaw -= this->GetPhysics()->GetAxis().ToAngles().yaw;



		//get pitch.
		animator.GetJointTransform(turretJoint, gameLocal.time, turretPos, turretAxis);

		turretPos = this->GetPhysics()->GetOrigin() + turretPos * this->GetPhysics()->GetAxis();
		//turretPos = turretPos + this->GetPhysics()->GetOrigin();

		idVec3 pitchDiff = playerPos - turretPos;
		float pitch = pitchDiff.ToPitch();




		if (gameLocal.GetLocalPlayer()->health <= 0
				|| gameLocal.GetLocalPlayer()->fl.notarget
				|| gameLocal.GetLocalPlayer()->noclip
				|| g_skill.GetInteger() <= 0)
		{

		}
		else
		{
			//rotate body.
			bodyRotation.SetVec( bodyAxis[1] );
			bodyRotation.SetAngle( yaw );
			animator.SetJointAxis( bodyJoint, JOINTMOD_WORLD, bodyRotation.ToMat3() );
		}






		if (this->attackState == IDLE)
		{
			//make beam idle around.
			idVec3 forward, up;
			idVec3 bodyPos;
			idVec3 facingAngle;
			idVec3 idlePos;

			bodyPos = this->GetPhysics()->GetOrigin();
			bodyPos.z = turretPos.z;
			facingAngle = turretPos - bodyPos;
			facingAngle.Normalize();

			facingAngle.ToAngles().ToVectors(&forward, NULL, &up);

			trace_t idleTr;

			idlePos = turretPos + (forward * 2048) + (up * -512);
			idlePos.x += idMath::Sin(gameLocal.time * 0.001f) * 16.0f;
			idlePos.z += idMath::Sin(gameLocal.time * 0.0005f) * 16.0f;

			gameLocal.clip.TracePoint( idleTr, turretPos, idlePos, CONTENTS_SOLID, this );
			this->beamEnd->SetOrigin(idleTr.endpos);




			if (this->nextStateTime > gameLocal.time)
			{
				return;
			}
			else
			{
				//show laser.
				this->beamStart->Show();
				this->laserdot->Show();

				//how often to search for targets.
				this->nextStateTime = gameLocal.time + 300;
			}

			if (this->nextIdleSound < gameLocal.time)
			{
				int rand = gameLocal.random.RandomInt(3);
				StartSound( "snd_idle", SND_CHANNEL_ANY, 0, false, NULL );


				if (rand == 0)
				{
					Event_PlayAnim("idle", 4);
				}
				else if (rand == 1)
				{
					Event_PlayAnim("idle2", 4);
				}
				else
				{
					Event_PlayAnim("idle3", 4);
				}
				this->nextIdleSound = gameLocal.time + IDLESOUNDDELAY + (gameLocal.random.RandomFloat() * IDLESOUNDRAND);
			}

			if (gameLocal.GetLocalPlayer()->health <= 0
				|| gameLocal.GetLocalPlayer()->fl.notarget
				|| gameLocal.GetLocalPlayer()->noclip
				|| g_skill.GetInteger() <= 0)
			{
				return;
			}

			//search for things to shoot.
			//check maximum range.



			//player target check gets priority.
			if (CheckTargetValidity(gameLocal.GetLocalPlayer()))
			{
				targetEnt = NULL;
				GotoWarmupState();
				return;
			}

			//if no player
			//check if player threw an object.
			float maxDist = spawnArgs.GetInt("maxrange", "1024");


			int			listedEntities;
			idEntity	*entityList[ MAX_GENTITIES ];

			listedEntities = gameLocal.EntitiesWithinRadius( GetPhysics()->GetOrigin(), maxDist, entityList, MAX_GENTITIES );

			for (int i = 0; i < listedEntities; i++)
			{
				idEntity *ent = entityList[ i ];

				if (!ent)
				{
					continue;
				}

				if ( ent->IsType( idMoveableItem::Type ) || ent->IsType( idMoveable::Type ) )
				{
					if (ent->lastThrowTime + THROWTIME_THRESHOLD > gameLocal.time)
					{
						if (CheckTargetValidity(ent))
						{
							targetEnt = ent;
							GotoWarmupState();
							return;
						}
					}
				}

				if (ent->IsType(idWeevil::Type))
				{
					if (static_cast<idWeevil *>( ent)->isJumping())
					{
						targetEnt = ent;
						GotoWarmupState();
						return;
					}
				}				
			}



			//no target found. oh well.



		}
		else if (this->attackState == WARMUP)
		{
			//update laser.
			trace_t tr;
			gameLocal.clip.TracePoint( tr, beamStart->GetPhysics()->GetOrigin(), playerPos, CONTENTS_SOLID, this );
			this->beamEnd->SetOrigin(tr.endpos);

			if (gameLocal.time > nextStateTime)
			{
				this->attackState = VOLLEYING;
				this->nextStateTime = 0;
			}
		}
		else if (this->attackState == VOLLEYING)
		{
			//update laser.
			trace_t tr;
			gameLocal.clip.TracePoint( tr, beamStart->GetPhysics()->GetOrigin(), playerPos, CONTENTS_SOLID, this );
			this->beamEnd->SetOrigin(tr.endpos);

			if (this->volleyCount >= MAXVOLLEY)
			{
				this->attackState = IDLE;
				this->nextStateTime = 0;
				return;
			}

			if (this->nextStateTime < gameLocal.time)
			{
				const idDict *	projectileDef;
				idEntity *		ent;
				idVec3 bulletVelocity;

				pitchDiff.Normalize();

				//FIRE.				
				idProjectile *bullet;
				projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("projectile", "projectile_turretbullet"), false );
				bulletVelocity = projectileDef->GetVector("velocity");
				gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
				bullet = ( idProjectile * )ent;
				bullet->Create( this, turretPos, pitchDiff );
				bullet->Launch( turretPos, pitchDiff, pitchDiff * bulletVelocity.x );				


				//muzzle flash skin.
				SetSkin(declManager->FindSkin( "skins/turret/firing" ));
				PostEventSec( &EV_Turret_muzzleflashoff, MUZZLEFLASHTIME);

				//play animation.
				Event_PlayAnim("fire", 4);

				//play sound.
				StartSound( "snd_fire", SND_CHANNEL_ANY, 0, false, NULL );

				//keep track of fire count.
				this->volleyCount++;
				this->nextStateTime = gameLocal.time + VOLLEYDELAY;
				return;
			}
		}
	}
}

void idTurret::Think( void )
{
	this->RunPhysics();

	UpdateStates();
	

	idAnimatedEntity::Think();
	Present();
	
}



/*
================
idSecurityCamera::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void idTurret::Present( void )
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

