
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define ROCKETSPEED			256
#define AIMAHEAD_DISTANCE	256


#define LIGHT_AHEADDISTANCE	32

const idEventDef EV_zeppelinactivate(	"zeppelinactivate",	"d" );

CLASS_DECLARATION( idAnimatedEntity, idZeppelinBig )
	EVENT( EV_zeppelinactivate,			idZeppelinBig::Event_zeppelinactivate )
END_CLASS

void idZeppelinBig::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(nextAttackTime);
	savefile->WriteInt(nextVolleyTime);
	savefile->WriteInt(volleyCount);
	savefile->WriteInt(maxVolley);
	savefile->WriteInt(attackDelay);
	savefile->WriteFloat(initialDelay);

	savefile->WriteObject(beam1Start);
	savefile->WriteObject(beam1End);
	savefile->WriteObject(beam2Start);
	savefile->WriteObject(beam2End);
	savefile->WriteObject(light);
}

void idZeppelinBig::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(nextAttackTime);
	savefile->ReadInt(nextVolleyTime);
	savefile->ReadInt(volleyCount);
	savefile->ReadInt(maxVolley);
	savefile->ReadInt(attackDelay);
	savefile->ReadFloat(initialDelay);

	savefile->ReadObject(reinterpret_cast<idClass *&>(beam1Start));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beam1End));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beam2Start));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beam2End));
	savefile->ReadObject(reinterpret_cast<idClass *&>(light));
}

void idZeppelinBig::Event_zeppelinactivate(int value)
{
	if (value > 0 && state == OFF)
	{
		this->Show();
		beam1Start->Show();
		beam2Start->Show();
		light->On();

		if (initialDelay > 0)
		{
			nextAttackTime = gameLocal.time + (initialDelay);
		}

		StartSound( "snd_activate", SND_CHANNEL_BODY2, 0, false, NULL );

		state = ON;
	}
	else
	{
		this->Hide();
		beam1Start->Hide();
		beam2Start->Hide();
		light->Off();

		state = OFF;
	}
}

void idZeppelinBig::AimBeam(const char *jointName, idBeam *beamEnd, idVec3 aimPos)
{
	jointHandle_t		beamJoint;
	idVec3				dirToPlayer;
	idVec3				beamPos;
	idMat3				beamAngle;
	trace_t				tr;

	beamJoint = this->GetAnimator()->GetJointHandle( jointName );
	animator.GetJointTransform( beamJoint, gameLocal.time, beamPos, beamAngle);

	beamPos = this->GetPhysics()->GetOrigin() + beamPos * this->GetPhysics()->GetAxis();
	dirToPlayer = aimPos - beamPos;
	dirToPlayer.Normalize();

	animator.SetJointAxis( beamJoint, JOINTMOD_WORLD, dirToPlayer.ToMat3() );

	//do traceline.
	gameLocal.clip.TracePoint( tr, beamPos, beamPos + (dirToPlayer * 16384), MASK_SOLID, this );

	//beamEnd->GetPhysics()->SetOrigin(tr.endpos);
	beamEnd->SetOrigin(tr.endpos);
}

void idZeppelinBig::Think( void )
{
	//aim the spotlight ahead of the player.

	if (state == OFF)
	{
		return;
	}
	

	trace_t				tr;
	jointHandle_t gunJoint;
	idVec3 gunPos;
	idMat3 gunAxis;		
	idVec3 playerPos;

	gunJoint = this->GetAnimator()->GetJointHandle( "gun" );
	animator.GetJointTransform( gunJoint, gameLocal.time, gunPos, gunAxis);
	gunPos = this->GetPhysics()->GetOrigin() + gunPos * this->GetPhysics()->GetAxis();

	playerPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0,0,32);		
	gameLocal.clip.TracePoint( tr, gunPos, playerPos, MASK_SOLID, this );


	/*
	gameLocal.GetLocalPlayer()->DebugMessage(va("%f %f %f\n", gameLocal.GetLocalPlayer()->GetPhysics()->GetLinearVelocity().x,
		gameLocal.GetLocalPlayer()->GetPhysics()->GetLinearVelocity().y,
		gameLocal.GetLocalPlayer()->GetPhysics()->GetLinearVelocity().z));
		*/

	//Find a point 32 units in front of player.
	if (tr.fraction >= 1)
	{
		trace_t lightTrace;
		idVec3 lightPos;
		idAngles playerAng = gameLocal.GetLocalPlayer()->viewAngles;
		playerAng.pitch = 0;
		playerAng.roll = 0;
		idVec3 playerForwardPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + (playerAng.ToForward() * LIGHT_AHEADDISTANCE);			

		idVec3 aimPos1 = playerForwardPos;
		idVec3 aimPos2 = playerForwardPos;

		//Give spotlight a little idle sway.
		aimPos1.x += idMath::Sin(gameLocal.time * 0.0005f) * 32.0f;
		aimPos1.y += idMath::Sin(gameLocal.time * 0.0004f) * 32.0f;

		aimPos2.x += idMath::Sin(gameLocal.time * 0.0003f) * 32.0f;
		aimPos2.y += idMath::Sin(gameLocal.time * 0.0006f) * 32.0f;

		//Aim the beams.

		lightPos = beam1End->GetPhysics()->GetOrigin();
		lightPos += playerAng.ToForward() * 32;		
		lightPos.x += idMath::Sin(gameLocal.time * 0.001f) * 32.0f;
		lightPos.y += idMath::Sin(gameLocal.time * 0.001f) * 32.0f;

		AimBeam( "beam1" , beam1End, aimPos1);
		AimBeam( "beam2" , beam2End, aimPos2);


		gameLocal.clip.TracePoint( lightTrace, gunPos, lightPos, MASK_SOLID, this );
		light->SetOrigin( lightTrace.endpos + idVec3(0, 0, 32) );
	}




	//GUN CODE.
	if (gameLocal.time > nextAttackTime && gameLocal.time >= nextVolleyTime && gameLocal.GetLocalPlayer()->health > 0 && g_skill.GetInteger() > 0 && !gameLocal.GetLocalPlayer()->noclip)
	{
		if (tr.fraction >= 1)
		{
			const idDict *	projectileDef;
			idVec3 dirToPlayer;

			float aheadModifier = 1.0f;
			idVec3 playerVelocity = gameLocal.GetLocalPlayer()->GetPhysics()->GetLinearVelocity();

			if (playerVelocity.x <= 0 && playerVelocity.y <= 0 && playerVelocity.z <= 0)
			{
				aheadModifier = 0.5f;
			}

			idVec3 playerAngle = gameLocal.GetLocalPlayer()->viewAngles.ToForward();
			playerAngle.Normalize();
			playerPos += playerAngle * (AIMAHEAD_DISTANCE * aheadModifier);



			//random variation.
			float aimVariation = spawnArgs.GetFloat("aim_variation");
			if (aimVariation > 0)
			{
				playerPos.x += -aimVariation + gameLocal.random.RandomFloat() * (aimVariation * 2);
				playerPos.y += -aimVariation + gameLocal.random.RandomFloat() * (aimVariation * 2);
			}

			


			dirToPlayer = playerPos - gunPos;
			dirToPlayer.Normalize();

			StartSound( "snd_fire", SND_CHANNEL_ANY, 0, false, NULL );



			trace_t		groundTr;
			gameLocal.clip.TracePoint( groundTr, gunPos, gunPos + (dirToPlayer * 4096), MASK_SOLID, this );


			if (tr.fraction >= 1)
			{
				idEntity *		bulletEnt;
				//idStaticEntity* markerEnt;
				//idStaticEntity* beamEnt;


				/*
				idDict args;
				args.Clear();
				args.Set(		"model",	"models/zeppelin_hitmarker/tris.ase" );
				args.SetVector(	"origin",	groundTr.endpos);
				markerEnt = ( idStaticEntity * )gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
				markerEnt->SetAxis(groundTr.c.normal.ToMat3());
				*/


				idVec3 dirToGun = gunPos - groundTr.endpos;
				dirToGun.Normalize();


				/*
				args.Clear();
				args.Set(		"model",	"models/beam_light/tris_512.ase" );
				args.SetVector(	"origin",	groundTr.endpos);
				args.Set(		"skin",		"skins/beam_light/lasersight");
				beamEnt = ( idStaticEntity * )gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
				beamEnt->SetAxis(dirToGun.ToMat3());
				*/


				//FIRE.
				idProjectile *bullet;
				projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString( "projectile", "projectile_rocket"), false );
				gameLocal.SpawnEntityDef( *projectileDef, &bulletEnt, false );
				bullet = ( idProjectile * )bulletEnt;
				bullet->Create( this, gunPos, dirToPlayer );
				bullet->Launch( gunPos, dirToPlayer, dirToPlayer * ROCKETSPEED );
				//bullet->spawnArgs.Set("markername", markerEnt->GetName());
				//bullet->spawnArgs.Set("beamname", beamEnt->GetName());
				bullet->SetAxis(dirToPlayer.ToMat3());
			}



			

			//gameRenderWorld->DebugCircle(idVec4(1,1,1,1), gunPos, idVec3(0,0,1), 32, 9, 2000);

			//gameRenderWorld->DebugCircle(idVec4(1,1,1,1), playerPos, idVec3(0,0,1), 32, 9, 2000);
			//gameRenderWorld->DebugLine(idVec4(1,1,1,1), playerPos, gunPos, 2000);


			


		}

		volleyCount++;
		nextVolleyTime = gameLocal.time + volleyDelay;

		if (volleyCount >= maxVolley)
		{
			nextAttackTime = gameLocal.time + attackDelay;
			nextVolleyTime = 0;
			volleyCount = 0;
		}
	}


	idAnimatedEntity::Present();
}

void idZeppelinBig::SpawnBeam(idBeam *beamStart, idBeam *beamEnd, const char *attachJoint)
{
	jointHandle_t spotlightJoint;
	idDict args;
	idVec3 muzzlePos;
	idMat3 muzzleAxis;

	beamEnd = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
	spotlightJoint = this->GetAnimator()->GetJointHandle( attachJoint );
	this->GetAnimator()->GetJointTransform( spotlightJoint, gameLocal.time, muzzlePos, muzzleAxis );

	muzzlePos = this->GetPhysics()->GetOrigin() + muzzlePos * this->GetPhysics()->GetAxis();

	beamStart->Event_SetWidth(64);
	beamStart->SetOrigin(muzzlePos );
	beamStart->BindToJoint( this, spotlightJoint, false );
	
	
}

void idZeppelinBig::Spawn( void )
{
	idDict	args;
	bool	startOff;

	startOff = this->spawnArgs.GetBool("start_off", "1");

	beam1End = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type );
	beam2End = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type );

	args.Clear();
	args.Set(		"target",	beam1End->name.c_str() );
	args.SetBool(	"start_off", startOff );
	args.Set(		"skin", "skins/spotlight");
	beam1Start = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );

	args.Clear();
	args.Set(		"target",	beam2End->name.c_str() );
	args.SetBool(	"start_off", startOff );
	args.Set(		"skin", "skins/spotlight");
	beam2Start = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );

	SpawnBeam(beam1Start, beam1End, "beam1");
	SpawnBeam(beam2Start, beam2End, "beam2");


	args.Clear();
	args.Set( "texture", "lights/spotlight" );
	args.SetInt( "alarmlight", 0 );
	light = ( idLight * )gameLocal.SpawnEntityType( idLight::Type, &args );
	light->SetColor(.73, .9, 1);
	light->SetRadius( 128 );
	

	volleyDelay = spawnArgs.GetFloat( "volleydelay", "0.4") * 1000;
	maxVolley = spawnArgs.GetInt("maxvolley", "3");
	initialDelay = spawnArgs.GetFloat( "initialdelay", "3" ) * 1000;
	attackDelay = spawnArgs.GetFloat("attackdelay", "3") * 1000;
	nextAttackTime = 0;
	state	= OFF;
	nextVolleyTime = 0;
	volleyCount = 0;
	BecomeActive( TH_THINK );

	//make it non solid.
	//GetPhysics()->SetContents( 0 );

	if (startOff)
	{
		this->Event_zeppelinactivate(0);
	}	
}

