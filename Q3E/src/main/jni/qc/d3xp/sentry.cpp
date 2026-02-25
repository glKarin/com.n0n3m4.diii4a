
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define TURN_TIME		300
#define SHOT_DELAY		350

const idEventDef EV_sentry_activate( "sentryactivate", "d" );
const idEventDef EV_sentry_issentryactive( "issentryactive", NULL, 'd' );
const idEventDef EV_sentry_turn( "sentryturn", "f" );
const idEventDef EV_sentry_pitch( "sentrypitch", "f" );
const idEventDef EV_sentry_face( "sentryface", "fff" );
const idEventDef EV_sentry_stand( "sentrystand" );
const idEventDef EV_sentry_fire( "sentryfire" );
const idEventDef EV_sentry_getlaser( "sentrygetlaser", NULL, 'v' );
const idEventDef EV_sentry_sentrykill( "sentrykill" );

CLASS_DECLARATION( idMoveableItem, idSentry )

	EVENT( EV_sentry_activate,			idSentry::Event_activate)
	EVENT( EV_sentry_issentryactive,	idSentry::Event_issentryactive)
	EVENT( EV_sentry_turn,				idSentry::Event_sentryturn)
	EVENT( EV_sentry_pitch,				idSentry::Event_sentrypitch)
	EVENT( EV_sentry_face,				idSentry::Event_sentryface)
	EVENT( EV_sentry_fire,				idSentry::Event_sentryfire)
	EVENT( EV_sentry_stand,				idSentry::Event_sentrystand)
	EVENT( EV_sentry_getlaser,			idSentry::Event_sentrygetlaser)
	EVENT( EV_sentry_sentrykill,		idSentry::Event_sentrykill)

END_CLASS


void idSentry::Spawn( void )
{
	idDict args;
	jointHandle_t			muzzleJoint;
	idVec3					muzzlePos;
	idMat3					muzzleAxis;
	idVec3					forward, right, up;

	pitch = 0;
	yaw = 0;
	state = OFF;

	BecomeActive( TH_THINK );

	args.Clear();
	args.SetVector(	"origin",   this->GetPhysics()->GetOrigin() );
	args.Set( "model",			"env_sentry" );
	
	model = ( idAnimatedEntity * )gameLocal.SpawnEntityType( idAnimatedEntity::Type, &args );
	model->Bind(this, true);
	
	model->Event_PlayAnim( "closed", 4 );


	//spawn beam end.
	args.SetVector( "origin", vec3_origin );
	beamEnd = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
	
	muzzleJoint = model->GetAnimator()->GetJointHandle( "muzzle" );
	model->GetAnimator()->GetJointTransform( muzzleJoint, gameLocal.time, muzzlePos, muzzleAxis );

	//spawn beam start.
	args.Clear();
	args.Set(		"target",	beamEnd->name.c_str() );
	args.SetVector(	"origin",    muzzlePos + this->GetPhysics()->GetOrigin() );
	args.SetBool(	"start_off", true );
	args.Set(		"skin",		spawnArgs.GetString("beamskin", "skins/beam_turret") );
	args.SetInt(	"width",	spawnArgs.GetInt("beamwidth", "3") );
	
	beamStart = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
	beamStart->BindToJoint( model, muzzleJoint, false );
	beamStart->BecomeActive(TH_PHYSICS);
	beamStart->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 0;
	beamStart->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 1;
	beamStart->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0;

	//laser sight dot.
	args.Clear();
	args.SetVector( "origin", vec3_origin );
	args.Set( "model", spawnArgs.GetString("lasermodel", "models/lasersight/tris.ase")     );
	args.SetInt( "solid", 0 );
	laserdot = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	laserdot->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 0;
	laserdot->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 1;
	laserdot->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0;

	laserdot->Hide();




	yawActive = false;
	yawTimeEnd = 0;
	yawTimeStart = 0;
	yawStart = 0;
	yawEnd = 0;

	pitchActive = false;
	pitchTimeEnd = 0;
	pitchTimeStart = 0;
	pitchStart = 0;
	pitchEnd = 0;



	//nose camera.
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	args.Clear();
	args.SetVector( "origin", muzzlePos + this->GetPhysics()->GetOrigin() + (up * 2.6f) );
	args.Set( "classname", "func_cameraview" );
	gameLocal.SpawnEntityDef( args, &nosecam );
	nosecam->BindToJoint( model, muzzleJoint, true );
	nosecam->BecomeActive(TH_PHYSICS);
	this->spawnArgs.Set("nosecamera_name", nosecam->GetName());

	nextShotTime = 0;
	queuedShots = 0;
}


void idSentry::Save( idSaveGame *savefile ) const {
	savefile->WriteInt(state);
	savefile->WriteFloat(yaw);
	savefile->WriteFloat(pitch);

	savefile->WriteObject(beamStart);
	savefile->WriteObject(beamEnd);
	savefile->WriteObject(model);
	savefile->WriteObject(laserdot);
	savefile->WriteObject(nosecam);

	savefile->WriteInt(laserActivateTime);

	savefile->WriteFloat(yawStart);
	savefile->WriteFloat(yawEnd);
	savefile->WriteInt(yawTimeStart);
	savefile->WriteInt(yawTimeEnd);
	savefile->WriteBool(yawActive);

	savefile->WriteFloat(pitchStart);
	savefile->WriteFloat(pitchEnd);
	savefile->WriteInt(pitchTimeStart);
	savefile->WriteInt(pitchTimeEnd);
	savefile->WriteBool(pitchActive);

	savefile->WriteInt(queuedShots);
	savefile->WriteInt(nextShotTime);

}

void idSentry::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt(state);
	savefile->ReadFloat(yaw);
	savefile->ReadFloat(pitch);

	savefile->ReadObject(reinterpret_cast<idClass *&>(beamStart));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beamEnd));
	savefile->ReadObject(reinterpret_cast<idClass *&>(model));
	savefile->ReadObject(reinterpret_cast<idClass *&>(laserdot));
	savefile->ReadObject(reinterpret_cast<idClass *&>(nosecam));
	

	savefile->ReadInt(laserActivateTime);

	savefile->ReadFloat(yawStart);
	savefile->ReadFloat(yawEnd);
	savefile->ReadInt(yawTimeStart);
	savefile->ReadInt(yawTimeEnd);
	savefile->ReadBool(yawActive);

	savefile->ReadFloat(pitchStart);
	savefile->ReadFloat(pitchEnd);
	savefile->ReadInt(pitchTimeStart);
	savefile->ReadInt(pitchTimeEnd);
	savefile->ReadBool(pitchActive);

	savefile->ReadInt(queuedShots);
	savefile->ReadInt(nextShotTime);
}

void idSentry::Event_sentrygetlaser()
{
	idThread::ReturnVector(this->beamEnd->GetPhysics()->GetOrigin());
}

void idSentry::Event_sentrystand()
{
	idAngles ang = this->GetPhysics()->GetAxis().ToAngles();
	ang.pitch = 0;
	ang.roll = 0;
	this->GetPhysics()->SetAxis( ang.ToMat3() );
	this->SetOrigin(this->GetPhysics()->GetOrigin() + idVec3(0,0,0.1f));
}

void idSentry::Event_sentryfire()
{
	if (gameLocal.time > nextShotTime)
	{
		fireShot();
		nextShotTime = gameLocal.time  + SHOT_DELAY;
	}
	else
	{
		queuedShots++;
	}
}

void idSentry::fireShot()
{
	jointHandle_t	muzzleJoint, muzzleBackJoint;
	idVec3 offset1, offset2;
	idMat3 axis;

	muzzleJoint = model->GetAnimator()->GetJointHandle( "muzzle" );
	muzzleBackJoint = model->GetAnimator()->GetJointHandle( "muzzle_back" );

	if ( model->GetJointWorldTransform( muzzleJoint, gameLocal.time, offset1, axis ) 
		&& model->GetJointWorldTransform( muzzleBackJoint, gameLocal.time, offset2, axis ))
	{
		//trace_t tr;
		const idDict *	projectileDef;
		idEntity *		ent;
		idVec3			forward;
		idDict			args;
		idEntity *		particles;
		idAngles		particlesAngles;
		idVec3			bulletspeed;
		
		StartSound( "snd_fire", SND_CHANNEL_ANY, 0, false, NULL );


		forward = offset1 - offset2;
		forward.Normalize();
		

		//fire the projectile.

		//FIRE.
		
		idProjectile *bullet;
		projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("projectile", "projectile_sentrybullet"), false );
		bulletspeed = projectileDef->GetVector("velocity", "256 0 0");
		gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
		bullet = ( idProjectile * )ent;
		bullet->Create( this, offset1, forward );
		bullet->Launch( offset1, forward, forward * bulletspeed.x );
		

		args.Clear();
		args.SetVector( "origin",		offset1);
		args.Set(		"model",		spawnArgs.GetString("smoke_fire", "gunsmoke.prt") );
		args.SetBool(	"start_off",	false );
		particles = gameLocal.SpawnEntityType( idExplodable::Type, &args );
		particlesAngles = forward.ToAngles();
		particlesAngles.pitch += 90;
		particles->SetAngles(particlesAngles);






		//BC shell casing.
		idVec3 gunPos;
		jointHandle_t gunJoint;
		idMat3 gunAxis;
		gunJoint = model->GetAnimator()->GetJointHandle( "pivot" );		
		model->GetAnimator()->GetJointTransform( gunJoint, gameLocal.time, gunPos, gunAxis);
		gunPos = this->GetPhysics()->GetOrigin() + gunPos * this->GetPhysics()->GetAxis();

		idEntity *shellEnt;
		const idDict *	shellDef = gameLocal.FindEntityDefDict( spawnArgs.GetString( "shell", "moveable_shell"), false );
		gameLocal.SpawnEntityDef( *shellDef, &shellEnt, false );
		shellEnt->GetPhysics()->SetOrigin(gunPos);
		shellEnt->GetPhysics()->SetAxis(this->GetPhysics()->GetAxis());

		idVec3 right;		
		forward.ToAngles().ToVectors( NULL, &right, NULL);
		idVec3 shellVelocity = (right * (16 + gameLocal.random.CRandomFloat() * 8) ) + idVec3(0,0,80+gameLocal.random.CRandomFloat() * 12) + (forward * (gameLocal.random.CRandomFloat() * 8) );
		shellEnt->GetPhysics()->SetLinearVelocity(shellVelocity);
	}
}

void idSentry::Event_sentryface( float x, float y, float z )
{
	idVec3				dirToTarget;
	jointHandle_t		pivotJoint;
	idVec3				offset;
	idMat3				axis;

	pivotJoint = model->GetAnimator()->GetJointHandle( "pivot" );
	model->GetJointWorldTransform(pivotJoint, gameLocal.time, offset, axis);



	dirToTarget = idVec3(x,y,z) - offset;

	idAngles ang1 = dirToTarget.ToAngles();
	ang1.yaw -= this->GetPhysics()->GetAxis().ToAngles().yaw;
	ang1.pitch -= this->GetPhysics()->GetAxis().ToAngles().pitch;


	dirToTarget = ang1.ToForward();
	dirToTarget.Normalize();

	model->GetAnimator()->SetJointAxis(pivotJoint, JOINTMOD_WORLD, dirToTarget.ToMat3());
}

void idSentry::Event_sentryturn( float yawValue )
{
	yawTimeStart = gameLocal.time;
	yawTimeEnd = gameLocal.time + TURN_TIME;

	yawStart = yaw;
	yawEnd = yaw - yawValue;

	yawActive = true;
}

void idSentry::Event_sentrypitch( float pitchValue )
{
	pitchTimeStart = gameLocal.time;
	pitchTimeEnd = gameLocal.time + TURN_TIME;

	pitchStart = pitch;
	pitchEnd = pitch - pitchValue;

	pitchActive = true;

	/*
	jointHandle_t		gunJoint;
	this->pitch -= pitchValue;
	
	idAngles ang(pitch, yaw,0);
	gunJoint = model->GetAnimator()->GetJointHandle( "pivot" );
	model->GetAnimator()->SetJointAxis(gunJoint, JOINTMOD_WORLD, ang.ToMat3());*/
}

void idSentry::Event_activate( int value )
{
	if (value >= 1)
	{
		//turn on. Deploy.
		model->Event_PlayAnim( "deploy", 4 );
		state = DEPLOYING;

		
		laserActivateTime = gameLocal.time + 1300;
		
	}
	else
	{
		//turn off. Pack up.
		model->Event_PlayAnim( "close", 4 );
		state = CLOSING;



		pitchTimeStart = gameLocal.time;
		pitchTimeEnd = gameLocal.time + 400;
		pitchStart = pitch;
		pitchEnd = 0;
		pitchActive = true;

		yawTimeStart = gameLocal.time;
		yawTimeEnd = gameLocal.time + 400;
		yawStart = yaw;
		yawEnd = 0;
		yawActive = true;




		laserdot->Hide();
		beamStart->Hide();

		queuedShots = 0;
	}
}

void idSentry::Event_issentryactive()
{
	if (state == ON || state == DEPLOYING)
	{
		idThread::ReturnInt( 1 );
		return;
	}
	else
	{
		idThread::ReturnInt( 0 );
		return;
	}
}


void idSentry::Think( void )
{
	trace_t tr;
	idVec3 offset1, offset2;
	idMat3 axis;
	jointHandle_t	muzzleJoint, muzzleBackJoint;


	//RunPhysics();

	muzzleJoint = model->GetAnimator()->GetJointHandle( "muzzle" );
	muzzleBackJoint = model->GetAnimator()->GetJointHandle( "muzzle_back" );

	if ( model->GetJointWorldTransform( muzzleJoint, gameLocal.time, offset1, axis ) 
		&& model->GetJointWorldTransform( muzzleBackJoint, gameLocal.time, offset2, axis ))
	{
		trace_t tr;
		idVec3 forward;
		
		forward = offset1 - offset2;
		forward.Normalize();

		//beam end goes through glass.

		int i;
		int maxGlassPanes = 32;
		idVec3 beamTempStartPos = offset1;

		for (i = 0; i < maxGlassPanes; i++)
		{
			trace_t beamTr;
			gameLocal.clip.TracePoint( beamTr, beamTempStartPos, beamTempStartPos + (forward * 4096), MASK_SOLID | MASK_SHOT_RENDERMODEL, this );

			if (!gameLocal.entities[ beamTr.c.entityNum ])
				continue;

			if (gameLocal.entities[ beamTr.c.entityNum ]->IsType( idBrittleFracture::Type ) || gameLocal.entities[ beamTr.c.entityNum ]->IsType( idQGlass::Type ))
			{
				//glass. continue the beam.

				//gameRenderWorld->DebugLine(idVec4(1,1,0,1), beamTempStartPos, beamTr.endpos, 1, false);

				beamTempStartPos = beamTr.endpos + (forward * 0.1f);
				continue;
			}

			beamEnd->SetOrigin(  beamTr.endpos );
			break;
		}

		//gameLocal.clip.TracePoint( tr, offset1, offset1 + (forward * 4096), MASK_OPAQUE, this );
		//beamEnd->SetOrigin(  tr.endpos );



		//beam dot stops at any solid surface.
		gameLocal.clip.TracePoint( tr, offset1, offset1 + (forward * 4096), MASK_SOLID | MASK_SHOT_RENDERMODEL, this );
		laserdot->SetOrigin( tr.endpos);
	}
	

	//yaw think.
	if (yawActive)
	{
		jointHandle_t		gunJoint;

		float lerp;
		float currentTime = gameLocal.time;
		float lookTimeMax = yawTimeEnd - yawTimeStart;
		float lookTimeMin = yawTimeStart;			
		currentTime -= yawTimeStart;
		lerp = currentTime / lookTimeMax;

		if (lerp > 1)
			lerp = 1;

		yaw = idMath::Lerp(yawStart, yawEnd, lerp);	
		idAngles ang(pitch, yaw,0);
		gunJoint = model->GetAnimator()->GetJointHandle( "pivot" );
		model->GetAnimator()->SetJointAxis(gunJoint, JOINTMOD_WORLD, ang.ToMat3());
		
		if (lerp >= 1)
		{
			yawActive = false;
		}
	}


	//yaw think.
	if (pitchActive)
	{
		jointHandle_t		gunJoint;

		float lerp;
		float currentTime = gameLocal.time;
		float lookTimeMax = pitchTimeEnd - pitchTimeStart;
		float lookTimeMin = pitchTimeStart;			
		currentTime -= pitchTimeStart;
		lerp = currentTime / lookTimeMax;

		if (lerp > 1)
			lerp = 1;

		pitch = idMath::Lerp(pitchStart, pitchEnd, lerp);	
		idAngles ang(pitch, yaw,0);
		gunJoint = model->GetAnimator()->GetJointHandle( "pivot" );
		model->GetAnimator()->SetJointAxis(gunJoint, JOINTMOD_WORLD, ang.ToMat3());
		
		if (lerp >= 1)
		{
			pitchActive = false;
		}
	}


	if (gameLocal.time > laserActivateTime && (laserdot->IsHidden() || beamStart->IsHidden())
		&& (state == DEPLOYING || state == ON) )
	{
		laserdot->Show();
		beamStart->Show();
	}

	//BC 10-2-2015 so that the camera can see the laserline. ugh! hack.
	if (!beamStart->IsHidden())
		gameRenderWorld->DebugLine(idVec4(0,1,0,1), beamStart->GetPhysics()->GetOrigin(), beamEnd->GetPhysics()->GetOrigin(),1, true);


	if (gameLocal.time > nextShotTime && queuedShots > 0 && state != OFF)
	{
		fireShot();
		nextShotTime = gameLocal.time + SHOT_DELAY;
		queuedShots--;
	}


	//Present();
	idMoveableItem::Think();
}







void idSentry::OnGet( void )
{
	//BC 6-28-2016 crash fix
	gameLocal.KillCCTVs();

	gameLocal.GetLocalPlayer()->StartSound( "snd_get", SND_CHANNEL_ANY, 0, false, NULL );
	this->PostEventMS( &EV_Remove, 0 );
	gameLocal.GetLocalPlayer()->GiveInventoryItem("weapon_sentry");
}

void idSentry::Event_sentrykill( void )
{
	gameLocal.KillCCTVs();
	laserdot->PostEventMS( &EV_Remove, 0 );
	beamStart->PostEventMS( &EV_Remove, 0 );
	beamEnd->PostEventMS( &EV_Remove, 0 );
}