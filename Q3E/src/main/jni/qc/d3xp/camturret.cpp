
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define SEARCHTIME		5000	//search for x millisecs
#define SUSPICOUSTIME	400    //must see player for this much millisecs
#define ALERTSTARETIME	5000	
#define SEARCHINTERVAL	300		//check for new player every x millisecs


const idEventDef EV_camturretactivate( "camturretactivate", "d" );

CLASS_DECLARATION( idAnimatedEntity, idCamturret )
	EVENT( EV_camturretactivate,		idCamturret::Event_camturretactivate)
END_CLASS


void idCamturret::GotoAlert( void )
{
	SetSkin(declManager->FindSkin( "skins/camturret/blink" ));

	Event_PlayAnim("alertspin", true);
	state = ALERTED;
}

void idCamturret::Event_camturretactivate( int value )
{
	if (value >= 1)
	{
		if (state == ALERTED || state == SUSPICIOUS)
		{
			return;
		}

		SetSkin(declManager->FindSkin( "skins/camturret/skin" ));

		if (state == SEARCHING)
		{
			//reset the search time.

			nextStateTime = gameLocal.time + SEARCHTIME; //how long to search.
			return;
		}

		//if alarm is hit while it's unfolding, then the camera instantly unfolds.
		if (state == OPENING)
		{

			Event_PlayAnim("opened");
			nextStateTime = gameLocal.time + 100;
			return;
		}

		SetSkin(declManager->FindSkin( "skins/camturret/searching" ));

		StartSound( "snd_deploy", SND_CHANNEL_ANY, 0, false, NULL );
		int animDoneTime = Event_PlayAnim("opening");
		state = OPENING;
		nextStateTime = animDoneTime; //unfold animation time.
	}
	else
	{
		SetSkin(declManager->FindSkin( "skins/camturret/skin" ));
		StartSound( "snd_close", SND_CHANNEL_ANY, 0, false, NULL );

		Event_PlayAnim("closing");
		state = OFF;
	}
}

void idCamturret::Spawn( void )
{
	nextSearchTime = 0;
	nextStateTime = 0;
	state = OFF;

	SetSkin(declManager->FindSkin( "skins/camturret/skin" ));

	BecomeActive( TH_THINK );
}


bool idCamturret::HasLOS()
{
	idVec3 playerPos, turretPos, up;
	trace_t				tr;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);

	playerPos = gameLocal.GetLocalPlayer()->GetEyePosition();
	turretPos = this->GetPhysics()->GetOrigin() + (up * -12);

	gameLocal.clip.TracePoint( tr, turretPos, playerPos, CONTENTS_OPAQUE, this );

	if (tr.fraction >= 1)
	{
		return true;
	}

	return false;
}

void idCamturret::PointCamera( const char* jointName, idVec3 aimPos)
{
	jointHandle_t		beamJoint;
	idMat3				beamAngle;
	idVec3				beamPos;
	idVec3				dirToPlayer;
	idAngles			tempAng;

	beamJoint = this->GetAnimator()->GetJointHandle( jointName );
	animator.GetJointTransform( beamJoint, gameLocal.time, beamPos, beamAngle);

	beamPos = this->GetPhysics()->GetOrigin() + beamPos * this->GetPhysics()->GetAxis();

	dirToPlayer = aimPos - beamPos;
	//dirToPlayer.Normalize();

	tempAng = dirToPlayer.ToAngles();
	tempAng.yaw += 180;


	animator.SetJointAxis( beamJoint, JOINTMOD_WORLD, tempAng.ToMat3() );
}

void idCamturret::Think( void )
{
	if (state == OPENING)
	{
		if (gameLocal.time > nextStateTime)
		{
			Event_PlayAnim("searching");
			nextStateTime = gameLocal.time + SEARCHTIME; //how long to search.
			state = SEARCHING;
		}
	}
	else if (state == ALERTED)
	{
		idVec3 playerPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0,0, -40);

		PointCamera("cam0", playerPos );
		PointCamera("cam1", playerPos );
		PointCamera("cam2", playerPos );

		//stays in alert mode forever, until the killcode is plugged in.
	}
	else if (state == SUSPICIOUS)
	{
		if (gameLocal.time > nextStateTime)
		{
			if (HasLOS())
			{
				//make all cameras go on alert.
				int i;
				for ( i = 0; i < gameLocal.num_entities; i++ )
				{
					if ( !gameLocal.entities[ i ] )
						continue;

					if (gameLocal.entities[i]->IsType(idCamturret::Type))
					{
						static_cast<idCamturret *>( gameLocal.entities[i] )->GotoAlert();
					}
				}

				ActivateTargets(this);
			}
			else
			{
				Event_PlayAnim("searching");
				nextStateTime = gameLocal.time + SEARCHTIME; //how long to search.
				state = SEARCHING;
			}
		}
	}
	else if (state == SEARCHING)
	{
		//search for LOS to player.
		if (gameLocal.time > nextSearchTime)
		{
			nextSearchTime = gameLocal.time + SEARCHINTERVAL;

			if (HasLOS())
			{
				idVec3 playerPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0,0, -40);

				state = SUSPICIOUS;
				nextStateTime = gameLocal.time + SUSPICOUSTIME;

				StartSound( "snd_suspicious", SND_CHANNEL_ANY, 0, false, NULL );
				Event_PlayAnim("opened");

				PointCamera("cam0", playerPos );
				PointCamera("cam1", playerPos );
				PointCamera("cam2", playerPos );
			}

		}
		else if (gameLocal.time > nextStateTime)
		{
			//give up search.
			idStr timeouttarget = spawnArgs.GetString( "timeouttarget", "" );
			if ( timeouttarget.Length() )
			{
				idEntity	*ent;
				ent = gameLocal.FindEntity( timeouttarget );

				if ( ent->RespondsTo( EV_Activate ) || ent->HasSignal( SIG_TRIGGER ) )
				{
					ent->Signal( SIG_TRIGGER );
					ent->ProcessEvent( &EV_Activate, this );
				}
			}

			Event_camturretactivate( 0 );
		}
	}

	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}


int idCamturret::Event_PlayAnim( const char* animname, bool loop)
{
	int anim;
	int channel = 0;
	int animBlendFrames = 4;
	int animDoneTime = 0;

	anim = animator.GetAnim( animname );
	if ( !anim )
	{
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	}
	else
	{
		if (loop)
		{
			animator.CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		}
		else
		{
			animator.PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		}


		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();
	}

	animBlendFrames = 0;
	return animDoneTime;
}
