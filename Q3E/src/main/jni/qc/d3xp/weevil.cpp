
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define		WALKSPEED		9   //was 9
#define		WALKBOUNCE		6   //if hit an obstacle then bounce a bit to dislodge itself.
#define		DIST_MOD		0.11f
#define		MAXPITCH		50
#define		MAXROLL			50
#define		OFFROADTIME		200

#define		JUMPDISTANCE	192
#define		JUMPHEIGHT		256

#define		PLUGMOVETIME	0.2f

#define		JUMP_STARTTIME	400

const idEventDef EV_weevil_forward(	"weevilforward",	"d" );
const idEventDef EV_weevil_turn(	"weevilturn",		"d" );
const idEventDef EV_weevil_stop(	"weevilstop" );
const idEventDef EV_weevil_stand(	"weevilstand" );
const idEventDef EV_weevil_light(	"weevillight",		"d" );
const idEventDef EV_weevil_jump(	"weeviljump" );
const idEventDef EV_weevil_gravity( "weevilgravity",	"d" );
const idEventDef EV_weevil_getgravity( "weevilgetgravity",	NULL, 'd' );
const idEventDef EV_weevil_doplug( "weevildoplug", "de" );
const idEventDef EV_weevil_plugConnected( "<weevilPlugConnected>", "v" );
const idEventDef EV_weevil_isPlugConnected( "isweevilplugconnected", NULL, 'd' );
const idEventDef EV_weevil_eyemove("weevileyemove", "ffd", 'd');


CLASS_DECLARATION( idMoveableItem, idWeevil )
	EVENT( EV_weevil_forward,			idWeevil::Event_forward )
	EVENT( EV_weevil_turn,				idWeevil::Event_turn )
	EVENT( EV_weevil_stop,				idWeevil::Event_stop )
	EVENT( EV_weevil_stand,				idWeevil::Event_stand )
	EVENT( EV_weevil_light,				idWeevil::Event_light )
	EVENT( EV_weevil_jump,				idWeevil::Event_jump )
	EVENT( EV_weevil_gravity,			idWeevil::Event_gravity )
	EVENT( EV_weevil_getgravity,		idWeevil::Event_getgravity )
	EVENT( EV_weevil_doplug,			idWeevil::Event_doPlug )
	EVENT( EV_weevil_plugConnected,		idWeevil::Event_plugdone)
	EVENT( EV_weevil_isPlugConnected,	idWeevil::Event_isweevilplugconnected)
	EVENT( EV_weevil_eyemove,			idWeevil::Event_weevilEyeMove)
END_CLASS


void idWeevil::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool(isGravity);
	savefile->WriteInt(state);
	savefile->WriteInt(walkDirection);
	savefile->WriteInt(turnCurrent);
	savefile->WriteInt(turnTarget);
	savefile->WriteInt(turnDirection);
	savefile->WriteInt(offroadTimer);
	savefile->WriteInt(jumpstartTimer);
	savefile->WriteVec3(lastPosition);

	savefile->WriteObject(model);
	savefile->WriteObject(nosecam);
	savefile->WriteObject(light);

	savefile->WriteObject(plugModel);
	savefile->WriteObject(beamStart);
	savefile->WriteObject(beamEnd);
	savefile->WriteJoint(headJoint);
}

void idWeevil::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool(isGravity);
	savefile->ReadInt(state);
	savefile->ReadInt(walkDirection);
	savefile->ReadInt(turnCurrent);
	savefile->ReadInt(turnTarget);
	savefile->ReadInt(turnDirection);
	savefile->ReadInt(offroadTimer);
	savefile->ReadInt(jumpstartTimer);
	savefile->ReadVec3(lastPosition);

	savefile->ReadObject(reinterpret_cast<idClass *&>(model));
	savefile->ReadObject(reinterpret_cast<idClass *&>(nosecam));
	savefile->ReadObject(reinterpret_cast<idClass *&>(light));

	savefile->ReadObject(reinterpret_cast<idClass *&>(plugModel));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beamStart));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beamEnd));
	savefile->ReadJoint(headJoint);
}


void idWeevil::Spawn( void )
{
	idDict args;
	
	jointHandle_t		noseJoint;
	idVec3				nosePos;
	idMat3				noseAxis;
	idVec3				forward;


	state = 0;
	walkDirection = 1;
	turnCurrent = 0;
	turnTarget = 0;
	turnDirection = 1;
	offroadTimer = 0;
	jumpstartTimer = 0;
	lastPosition = vec3_zero;
	isGravity = true;



	BecomeActive( TH_THINK );







	//MODEL
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward);

	args.Clear();
	args.SetVector(	"origin",   this->GetPhysics()->GetOrigin() );
	args.Set( "model",			"monster_weevil" );
	args.Set( "cycle",			"-1" );
	//args.Set( "notriggeronanim", "1" );
	model = ( idAnimatedEntity * )gameLocal.SpawnEntityType( idAnimatedEntity::Type, &args );
	model->SetAxis( this->GetPhysics()->GetAxis() );


	// BEAMS / PLUGS
	args.Clear();
	args.Set(		"model",	"models/datajack_plug/tris.ase" );
	args.SetBool(	"solid",	0);
	plugModel = ( idMover * )gameLocal.SpawnEntityType( idMover::Type, &args );
	plugModel->Hide();

	args.Clear();
	args.SetInt(	"start_off", 1);
	beamEnd = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type );

	args.Clear();
	args.Set(		"target",	beamEnd->name.c_str() );
	args.Set(		"skin", "skins/beam_black");
	args.SetInt(	"start_off", 1);
	beamStart = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
	beamStart->Hide();
	beamStart->Event_SetWidth(0.5f);

	idVec3 selfForward, selfUp;
	idAngles selfAng = this->GetPhysics()->GetAxis().ToAngles();	
	selfAng.ToVectors(&selfForward, NULL, &selfUp);
	idVec3 attachPoint = this->GetPhysics()->GetOrigin() + (selfForward * -3)  + (selfUp * 7);



	beamStart->SetOrigin(attachPoint );
	beamStart->Bind( this, false );




	idVec3 plugForward;
	idAngles plugAng = plugModel->GetPhysics()->GetAxis().ToAngles();	
	selfAng.ToVectors(&plugForward, NULL, NULL);
	idVec3 plugEndPoint = plugModel->GetPhysics()->GetOrigin() + (plugForward * 0.9f);

	beamEnd->SetOrigin(plugEndPoint);
	beamEnd->Bind(plugModel, false);
	beamEnd->Hide();



	model->Bind(this, true);
	


	//nose camera.
	noseJoint = model->GetAnimator()->GetJointHandle( "head" );
	model->GetAnimator()->GetJointTransform( noseJoint, gameLocal.time, nosePos, noseAxis );

	args.Clear();
	args.SetVector( "origin", nosePos + this->GetPhysics()->GetOrigin() + (forward * -2));
	args.Set( "classname", "func_cameraview" );
	gameLocal.SpawnEntityDef( args, &nosecam );
	nosecam->BindToJoint(model, noseJoint, true);
	nosecam->BecomeActive(TH_PHYSICS);
	this->spawnArgs.Set("nosecamera_name", nosecam->GetName());


	//spawn light.	
	args.Clear();
	args.SetVector(	"origin",   nosePos + this->GetPhysics()->GetOrigin() + (forward * 0.9f) );
	args.Set( "light_target", "1 0 -0.3" );
	args.Set( "light_up", "0 0 1" );
	args.Set( "light_right", "0 -1.5 0" );	
	args.Set( "light_end", "128 0 0" );
	args.Set( "texture", "lights/monitorglow" );	
	args.Set("start_off", "1");
	args.SetInt("alarmlight", 0);
	light = ( idLight * )gameLocal.SpawnEntityType( idLight::Type, &args );
	light->Bind(this, true);

	

	headJoint = model->GetAnimator()->GetJointHandle( "head" );

	
}

bool idWeevil::isJumping( void )
{
	if (jumpstartTimer > gameLocal.time)
		return true;

	return false;
}

void idWeevil::OnGet( void )
{
	gameLocal.KillCCTVs();

	beamStart->PostEventMS( &EV_Remove, 0 );
	beamEnd->PostEventMS( &EV_Remove, 0 );
	plugModel->PostEventMS( &EV_Remove, 0 );
}

void idWeevil::Event_isweevilplugconnected( void )
{
	if (plugModel->IsHidden())
		idThread::ReturnInt(0);
	else
		idThread::ReturnInt(1);
}

void idWeevil::Event_plugdone( idVec3 const &position )
{
	idDict args;

	args.Clear();
	args.SetVector( "origin",		position );
	args.Set(		"model",	    spawnArgs.GetString("smoke_plug", "plugconnect.prt") );
	args.SetBool(	"start_off",	false );
	gameLocal.SpawnEntityType( idExplodable::Type, &args );

	StartSound( "snd_plugin", SND_CHANNEL_ANY, 0, false, NULL );
}

void idWeevil::Event_doPlug( int active, idEntity *jackEnt )
{
	if (active <= 0)
	{
		//unplug.
		beamStart->Hide();
		plugModel->Hide();
		return;
	}

	//plug in.
	plugModel->GetPhysics()->SetAxis( jackEnt->GetPhysics()->GetAxis() );

	idVec3 jackForward, jackUp;
	idAngles selfAng = jackEnt->GetPhysics()->GetAxis().ToAngles();	
	selfAng.ToVectors(&jackForward, NULL, &jackUp);
	idVec3 jackEndPos = jackEnt->GetPhysics()->GetOrigin() + (jackForward * 0.4)  + (jackUp * 1.1);

	
	//gameRenderWorld->DebugLine(idVec4(0,1,0,1), plugModel->GetPhysics()->GetOrigin(), plugModel->GetPhysics()->GetOrigin() + idVec3(0,0,128), 10000);


	//gameRenderWorld->DebugLine(idVec4(1,1,0,1), jackEndPos, jackEndPos + idVec3(0,0,128), 10000);


	idVec3 weevilUp;
	idAngles weevilAng = this->GetPhysics()->GetAxis().ToAngles();	
	weevilAng.ToVectors(NULL, NULL, &weevilUp);
	plugModel->SetOrigin( this->GetPhysics()->GetOrigin() + (weevilUp * 6) );

	plugModel->Event_SetMoveTime(PLUGMOVETIME);
	plugModel->Event_MoveToPos(jackEndPos);
	PostEventSec( &EV_weevil_plugConnected, PLUGMOVETIME, jackEndPos);
	

	beamEnd->Show();
	beamStart->Show();
	plugModel->Show();
}

void idWeevil::Event_jump( void )
{
	Event_stand();

	state = JUMPSTART;
	this->Event_PlayAnim("jump_start", false);
	jumpstartTimer = gameLocal.time + JUMP_STARTTIME;

	StartSound( "snd_jump", SND_CHANNEL_ANY, 0, false, NULL );
}

void idWeevil::Event_getgravity( )
{
	if (this->isGravity <= 0)
	{
		idThread::ReturnInt( 0 );
	}
	else
	{
		idThread::ReturnInt( 1 );
	}
}

void idWeevil::Event_gravity( int value )
{
	if (value <= 0)
	{
		//gravity is now OFF.
		this->isGravity = 0;
		lastGravityState = false;
	}
	else
	{
		//turn on gravity.
		this->isGravity = 1;
	}

	if (value >= 1)
	{
		//turn on gravity.
		physicsObj.SetGravity( gameLocal.GetGravity() );
	}
}

void idWeevil::Event_light( int value )
{
	if (value)
	{
		if ( !light->isOn )
		{
			StartSound( "snd_light", SND_CHANNEL_ANY, 0, false, NULL );
		}

		light->On();
	}
	else
	{
		if ( light->isOn )
		{
			StartSound( "snd_light", SND_CHANNEL_ANY, 0, false, NULL );
		}

		light->Off();
	}
}


void idWeevil::Event_stand( void )
{
	CancelEvents( &EV_weevil_stop );
	state = IDLE;

	if (!isGravity)
	{
		return;
	}

	idAngles ang = this->GetPhysics()->GetAxis().ToAngles();
	ang.pitch = 0;
	ang.roll = 0;
	this->GetPhysics()->SetAxis( ang.ToMat3() );
	this->SetOrigin(this->GetPhysics()->GetOrigin() + idVec3(0,0,3.0f));

	StartSound( "snd_stand", SND_CHANNEL_ANY, 0, false, NULL );
}

void idWeevil::Event_turn( int degrees )
{
	if (degrees == 0)
	{
		return;
	}

	CancelEvents( &EV_weevil_stop );
	state = TURNING;

	turnCurrent = 0;
	turnTarget = idMath::Abs(degrees);

	if (degrees > 0)
	{
		turnDirection = -1;
		
	}
	else
	{
		turnDirection = 1;		
	}

	Event_PlayAnim("turning", true);

	//this->Event_PlayAnim( "idle", false );
}

void idWeevil::Event_forward( int distance )
{
	if (distance == 0)
	{
		return;
	}

	CancelEvents( &EV_weevil_stop );

	if (distance > 0)
	{
		walkDirection = 1;
		Event_PlayAnim("walk", true);
	}
	else
	{
		walkDirection = -1;
		Event_PlayAnim("walkbackwards", true);
	}

	offroadTimer = gameLocal.time + OFFROADTIME;

	state = WALKING;
	

	PostEventSec( &EV_weevil_stop, idMath::Abs(distance) * DIST_MOD );
}

void idWeevil::Event_stop( void )
{
	CancelEvents( &EV_weevil_stop );
	state = IDLE;
	Event_PlayAnim("idle", false);
}

void idWeevil::UpdateStates( void )
{
	if (state == WALKING)
	{
		if (CanMove())
		{
			idVec3 curPos;
			float bounceAmount;
			idVec3 forward;
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward);
			forward.Normalize();

			curPos = this->GetPhysics()->GetOrigin();


			if (HasObstacle() || 
				(idMath::Fabs(lastPosition.x - curPos.x) < 0.0001f
				&& idMath::Fabs(lastPosition.y - curPos.y) < 0.0001f
				&& idMath::Fabs(lastPosition.z - curPos.z) < 0.0001f))
			{
				//"off-road" mode.
				bounceAmount = WALKBOUNCE;
			}
			else
			{
				//normal walk.
				bounceAmount = 0;
			}

			this->GetPhysics()->SetLinearVelocity(forward * (WALKSPEED * walkDirection) + (idVec3(0,0,1) * bounceAmount));
			lastPosition.x = curPos.x;
			lastPosition.y = curPos.y;
			lastPosition.z = curPos.z;
		}
	}
	else if (state == TURNING)
	{
		idAngles ang = this->GetPhysics()->GetAxis().ToAngles();
		ang.yaw += turnDirection;

		turnCurrent++;
		if (turnCurrent >= turnTarget)
		{
			Event_stop();
			return;
		}

		if (CanMove())
		{
			this->GetPhysics()->SetAxis( ang.ToMat3() );
		}
	}
	else if (state == JUMPSTART)
	{
		if (gameLocal.time > jumpstartTimer)
		{
			idVec3 forward;
			idAngles ang = this->GetPhysics()->GetAxis().ToAngles();
			ang.pitch = 0;
			ang.roll = 0;
			ang.ToVectors(&forward);

			if (this->GetPhysics()->HasGroundContacts())
			{
				Event_stand();
				this->GetPhysics()->SetLinearVelocity((forward * JUMPDISTANCE) + (idVec3(0,0,1) * JUMPHEIGHT));
			}

			this->Event_PlayAnim( "jump_loop", true );
			state = JUMPING;

			jumpstartTimer = gameLocal.time + 200; //guarantee some jump time before it is allowed to enter landing state.
		}
	}
	else if (state == JUMPING)
	{
		if (this->IsGrabbed())
		{
			state = IDLE;
			return;
		}

		if (this->GetPhysics()->HasGroundContacts() && gameLocal.time > jumpstartTimer)
		{
			state = IDLE;
			
			Event_stand();
			this->GetPhysics()->PutToRest();

			this->Event_PlayAnim( "jump_end", false );

			return;
		}
	}
}

bool idWeevil::HasObstacle( void )
{
	trace_t forwardTr;
	idVec3 forward, up, right;
	idVec3 traceStart;

	trace_t downTr1, downTr2, downTr3;	
	
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	
	traceStart = this->GetPhysics()->GetOrigin() + (up * 1);	
	gameLocal.clip.TracePoint( forwardTr, traceStart, traceStart + (forward * (16 * walkDirection)), MASK_SOLID, this );

	if (forwardTr.fraction < 1)
	{
		return true;
	}

	//do the down trace to see if weevil is on a pit.
	gameLocal.clip.TracePoint( downTr1, traceStart + (forward * 5), traceStart + (forward * 5) + (up * -3), MASK_SOLID, this );
	if (downTr1.fraction >= 1)
	{
		return true;
	}

	gameLocal.clip.TracePoint( downTr2, traceStart + (forward * -5), traceStart + (forward * -5) + (up * -3), MASK_SOLID, this );
	if (downTr2.fraction >= 1)
	{
		return true;
	}

	gameLocal.clip.TracePoint( downTr3, traceStart, traceStart + (up * -3), MASK_SOLID, this );
	if (downTr3.fraction >= 1)
	{
		return true;
	}

	return false;
}

bool idWeevil::CanMove( void )
{
	idAngles angles = this->GetPhysics()->GetAxis().ToAngles();

	//return false if: being held, not touching ground, pitch is tilted far, roll is tilted far.

	if (this->IsGrabbed() || !this->GetPhysics()->HasGroundContacts() || idMath::Abs( angles.pitch) >= MAXPITCH || idMath::Abs(angles.roll) >= MAXROLL)
	{
		return false;
	}

	return true;
}

void idWeevil::Think( void )
{
	//RunPhysics();

	if (touchTriggers)
	{
		TouchTriggers();
	}

	//for the vacuum check.
	//if (!isGravity)
	{
		idMoveableItem::Think();
	}

	UpdateStates();
	Present();
}



void idWeevil::Present( void )
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

void idWeevil::Event_PlayAnim( const char* animname, bool cycle)
{
	int anim;
	int channel = 0;
	int animBlendFrames = 4;
	int animDoneTime = 0;

	anim = model->GetAnimator()->GetAnim( animname );
	if ( !anim )
	{
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		model->GetAnimator()->Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	}
	else
	{
		if (cycle)
		{
			model->GetAnimator()->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		}
		else
		{
			model->GetAnimator()->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		}

		animDoneTime = model->GetAnimator()->CurrentAnim( channel )->GetEndTime();
	}

	animBlendFrames = 0;
}

void idWeevil::Event_weevilEyeMove(float pitch, float yaw, bool reset)
{
	if (reset)
	{
		headAngle = idAngles(0,0,0);
		model->GetAnimator()->SetJointAxis(headJoint, JOINTMOD_WORLD, headAngle.ToMat3());
		idThread::ReturnInt(1);
		return;
	}

	
	float newYaw =		idMath::ClampFloat(-90, 90, headAngle.yaw - yaw);
	float newPitch =	idMath::ClampFloat(-90, 90, headAngle.roll + pitch);

	if (idMath::Fabs(headAngle.roll) + idMath::Fabs(pitch) > 90 || idMath::Fabs(headAngle.yaw) + idMath::Fabs(yaw) > 90)
	{
		idThread::ReturnInt(0);
	}
	else
	{
		idThread::ReturnInt(1);
	}

	headAngle.roll = newPitch;
	headAngle.yaw = newYaw;

	model->GetAnimator()->SetJointAxis(headJoint, JOINTMOD_LOCAL, headAngle.ToMat3());
}
