
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define TIME_TO_FRONTMACHINE		400
#define TIME_TO_PAUSE				400
#define TIME_TO_LOOK_UP				500

//const idEventDef EV_ButtonSwitcher_activate( "buttonswitcheractivate", "f" );

CLASS_DECLARATION( idStaticEntity, idCybervendor )
	//EVENT( EV_ButtonSwitcher_activate,			idButtonSwitcher::Event_buttonswitcheractivate)
END_CLASS

void idCybervendor::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(nextUpdate);
	savefile->WriteInt(lerpStart);
	savefile->WriteInt(lerpEnd);
	savefile->WriteVec3(posStart);
	savefile->WriteVec3(posEnd);
	savefile->WriteVec3(angStart);
	savefile->WriteVec3(angEnd);
	savefile->WriteInt(count);
}

void idCybervendor::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(nextUpdate);
	savefile->ReadInt(lerpStart);
	savefile->ReadInt(lerpEnd);
	savefile->ReadVec3(posStart);
	savefile->ReadVec3(posEnd);
	savefile->ReadVec3(angStart);
	savefile->ReadVec3(angEnd);
	savefile->ReadInt(count);
}

void idCybervendor::Spawn( void )
{
	BecomeActive( TH_THINK );

	GetPhysics()->SetContents( CONTENTS_SOLID );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );

	this->noGrab = true;
	this->isFrobbable = true;

	state = 0;
	nextUpdate = 0;
	lerpStart = 0;
	lerpEnd = 0;

	count = 2;
}

void idCybervendor::OnFrob()
{
	if (state == 0)
	{

		float player_z = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z;
		float machine_z = this->GetPhysics()->GetOrigin().z;

		if (player_z < machine_z - 4)
		{
			StartSound( "snd_error", SND_CHANNEL_ANY, 0, false, NULL );			
			return;
		}



		count *= 2;

		gameLocal.GetLocalPlayer()->Event_setFrozen(1);
		isFrobbable = 0;

		lerpStart = gameLocal.time;
		lerpEnd = gameLocal.time + TIME_TO_FRONTMACHINE; //time to transition to the machine front.


		idVec3	forward, right;
		GetPhysics()->GetAxis().ToAngles().ToVectors( &forward, &right );
		posEnd = this->GetPhysics()->GetOrigin() + (forward * 48) + (right * 12);

		posStart = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();

		
		angStart.x = gameLocal.GetLocalPlayer()->viewAngles.pitch;
		angStart.y = gameLocal.GetLocalPlayer()->viewAngles.yaw;
		angStart.z = gameLocal.GetLocalPlayer()->viewAngles.roll;

		if (angStart.y < 0)
			angStart.y += 360;

		if (angStart.y < 180)
			angStart.y += 360;
		
		angEnd.x = GetPhysics()->GetAxis().ToAngles().pitch + 5;
		angEnd.y = GetPhysics()->GetAxis().ToAngles().yaw + 180;
		angEnd.z = GetPhysics()->GetAxis().ToAngles().roll;

		if (angEnd.y < 0)
			angEnd.y += 360;

		if (angEnd.y < 180)
			angEnd.y += 360;

		StartSound( "snd_jingle", SND_CHANNEL_ANY, 0, false, NULL );

		state = 1;		
	}
}

void idCybervendor::Think()
{

	if (state == 1)
	{
		//lerp the player to the machine front AND look at the machine front.
		float lerp;
		float currentTime = gameLocal.time;
		float lookTimeMax = this->lerpEnd - this->lerpStart;
		float lookTimeMin = this->lerpStart;			
		currentTime -= this->lerpStart;
		lerp = currentTime / lookTimeMax;

		idVec3 newPosition;
		newPosition.x = idMath::Lerp(posStart.x, posEnd.x, lerp);
		newPosition.y = idMath::Lerp(posStart.y, posEnd.y, lerp);
		newPosition.z = idMath::Lerp(posStart.z, posEnd.z, lerp);

		gameLocal.GetLocalPlayer()->GetPhysics()->SetOrigin(newPosition);




		idVec3 lookAng;
		lookAng.x = idMath::Lerp(this->angStart.x, this->angEnd.x, lerp);
		lookAng.y = idMath::Lerp(this->angStart.y, this->angEnd.y, lerp);
		lookAng.z = idMath::Lerp(this->angStart.z, this->angEnd.z, lerp);
		//common->Printf("%f : %f : %f %f %f\n", angStart.y, angEnd.y, lookAng.x, lookAng.y, lookAng.z);

	
		gameLocal.GetLocalPlayer()->Event_setviewangle(lookAng);



		//common->Printf("%f\n", lerp);
		if (lerp >= 1)
		{
			lerpEnd = gameLocal.time + TIME_TO_PAUSE;
			state = 2;
		}
	}
	else if (state == 2)
	{
		//pause.

		if (gameLocal.time >= lerpEnd)
		{
			angStart.x = gameLocal.GetLocalPlayer()->viewAngles.pitch;
			angStart.y = gameLocal.GetLocalPlayer()->viewAngles.yaw;
			angStart.z = gameLocal.GetLocalPlayer()->viewAngles.roll;

			if (angStart.y < 0)
				angStart.y += 360;

			if (angStart.y < 180)
				angStart.y += 360;
		
			angEnd.x = spawnArgs.GetInt("drillangle", "50");
			angEnd.y = GetPhysics()->GetAxis().ToAngles().yaw + 180;
			angEnd.z = GetPhysics()->GetAxis().ToAngles().roll;

			if (angEnd.y < 0)
				angEnd.y += 360;

			if (angEnd.y < 180)
				angEnd.y += 360;


			lerpStart = gameLocal.time;
			lerpEnd = gameLocal.time + TIME_TO_LOOK_UP; //time to transition to the machine front.

			state = 3;
		}
	}
	else if (state == 3)
	{
		//look at the hole.

		float lerp;
		float currentTime = gameLocal.time;
		float lookTimeMax = this->lerpEnd - this->lerpStart;
		float lookTimeMin = this->lerpStart;			
		currentTime -= this->lerpStart;
		lerp = currentTime / lookTimeMax;

		//look up.
		idVec3 lookAng;
		lookAng.x = idMath::Lerp(this->angStart.x, this->angEnd.x, lerp);
		lookAng.y = idMath::Lerp(this->angStart.y, this->angEnd.y, lerp);
		lookAng.z = idMath::Lerp(this->angStart.z, this->angEnd.z, lerp);
		//common->Printf("%f : %f : %f %f %f\n", angStart.y, angEnd.y, lookAng.x, lookAng.y, lookAng.z);

	
		gameLocal.GetLocalPlayer()->Event_setviewangle(lookAng);

		if (lerp >= 1)
		{
			StartSound( "snd_drill", SND_CHANNEL_ANY, 0, false, NULL );

			idVec3	forward, right, up;
			GetPhysics()->GetAxis().ToAngles().ToVectors( &forward, &right, &up );
			posStart = this->GetPhysics()->GetOrigin() + (forward * 32) + (right * 12) + (up * 55);

			gameLocal.GetLocalPlayer()->Event_viewlook(posStart, 0.2f, 0, 1);
			lerpEnd = gameLocal.time + 1500;

			//gameLocal.GetLocalPlayer()->Event_ToggleBloom(1);
			cvarSystem->SetCVarString("g_testpostprocess", "textures/fx/fastforward_fast");

			state = 4;
		}
	}
	else if (state == 4)
	{
		//drilling.
		gameLocal.GetLocalPlayer()->Event_setviewangle(angEnd);

		if (gameLocal.time >= lerpEnd)
		{
			gameLocal.GetLocalPlayer()->Event_viewlook(posStart, 0.4f, 1, 0);
			StartSound( "snd_ding", SND_CHANNEL_ANY, 0, false, NULL );
			

			angStart.x = gameLocal.GetLocalPlayer()->viewAngles.pitch;		
			angEnd.x = 20;
			angEnd.y = gameLocal.GetLocalPlayer()->viewAngles.yaw;

			lerpStart = gameLocal.time;
			lerpEnd = gameLocal.time + 300; //time to transition to the machine front.
			cvarSystem->SetCVarString("g_testpostprocess", "0");

			renderEntity.gui[0]->HandleNamedEvent("done");


			//spew some decals into the world.
			if (count <= 256)
			{
				idVec3	forward, right, up;
				GetPhysics()->GetAxis().ToAngles().ToVectors( &forward, &right, &up );
				idVec3 spitPoint = this->GetPhysics()->GetOrigin() + (forward * 25) + (right * 12) + (up * 128);

				int i;
				for (i = 0; i < count; i++)
				{
					idVec3 randSpot;
					randSpot = this->GetPhysics()->GetOrigin() + (forward * (25 + gameLocal.random.RandomFloat() * 64)) + (right * (12 - 64 + gameLocal.random.RandomFloat() * 128)   ) + (up * 128);

					trace_t		groundTr;
					gameLocal.clip.TracePoint( groundTr, randSpot, randSpot + idVec3(0,0,-1024), MASK_SOLID, this );

					if ((randSpot - spitPoint).Normalize() > 64)
						continue;

					gameLocal.ProjectDecal( groundTr.c.point, -groundTr.c.normal, 0.5f, true, 4 + gameLocal.random.RandomFloat() * 36, spawnArgs.GetString("mtr_oil") );
				}
			}




			state = 5;
		}
	}
	else if (state == 5)
	{
		//look up.
		float lerp;
		float currentTime = gameLocal.time;
		float lookTimeMax = this->lerpEnd - this->lerpStart;
		float lookTimeMin = this->lerpStart;			
		currentTime -= this->lerpStart;
		lerp = currentTime / lookTimeMax;

		//look up.
		idVec3 lookAng;
		lookAng.x = idMath::Lerp(this->angStart.x, this->angEnd.x, lerp);
		lookAng.y = angEnd.y;
		lookAng.z = 0;
		//common->Printf("%f : %f : %f %f %f\n", angStart.y, angEnd.y, lookAng.x, lookAng.y, lookAng.z);
	
		gameLocal.GetLocalPlayer()->Event_setviewangle(lookAng);

		if (lerp >= 1)
		{
			gameLocal.GetLocalPlayer()->Event_setFrozen(0);
			isFrobbable = 1;
			state = 0;

			ActivateTargets( this );
		}
	}






	if (gameLocal.time > nextUpdate && state == 0)
	{
		//check if player angle faces the FRONT of the machine.
		idAngles playerAng = gameLocal.GetLocalPlayer()->viewAngles;
		playerAng.pitch = 0;
		playerAng.roll = 0;
		float vdot = DotProduct( this->GetPhysics()->GetAxis().ToAngles().ToForward(), playerAng.ToForward() );

		//check if player is positioned in FRONT of the machine.
		idVec3	forward;
		GetPhysics()->GetAxis().ToAngles().ToVectors( &forward );
		idVec3 frontOfMachinePos = this->GetPhysics()->GetOrigin() + (forward * 24);
		idVec3 toPlayer =  frontOfMachinePos - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
		idVec3 machineFacing = this->GetPhysics()->GetAxis().ToAngles().ToForward();
		float facingResult = DotProduct( toPlayer, machineFacing );

		//gameLocal.GetLocalPlayer()->DebugMessage(va("dot %f", facingResult));


		if (vdot < -0.1f && facingResult < 0)
		{
			this->isFrobbable = true;
		}
		else
		{
			this->isFrobbable = false;
		}

		nextUpdate = gameLocal.time + 300; //update every X milliseconds.
	}
	

	Present();
}
