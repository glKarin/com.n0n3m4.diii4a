
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define WHEEL_STEP			40
#define YAW_STEP			0.7f

#define MAX_FORCE			1024 //was 1024
#define MIN_FORCE			96


#define MULTIPLIER_MIN		1.1f
#define MULTIPLIER_MAX		2.2f


#define MIN_PITCH			-10
#define MAX_PITCH			-89

#define RESETTIME			50
#define WHEELSPEED			2
#define WHEELTIME			300

#define RISETIME			300
#define BEAMWIDTH			8

#define JUMPPADWIDTH		32
#define JUMPPADHEIGHT		128


const idEventDef EV_launcherkill( "launcherkill" );

CLASS_DECLARATION( idMoveableItem, idLauncher )
	EVENT( EV_launcherkill,			idLauncher::launcherkill)
END_CLASS



void idLauncher::Save( idSaveGame *savefile ) const
{
	savefile->WriteObject(peakEnt);
	savefile->WriteObject(jumpPad);
	savefile->WriteObject(gatorEnt);
	savefile->WriteObject(remoteEnt);
	savefile->WriteObject(hitpointEnt);
	savefile->WriteObject(displaymodel);
	savefile->WriteObject(particles);
	

	

	savefile->WriteInt(state);
	savefile->WriteVec3(lastLaunchdir);
	savefile->WriteFloat(lastForce);	
	savefile->WriteInt(nextAimrecord);
	savefile->WriteInt(remoteLerpStart);
	savefile->WriteInt(remoteLerpEnd);
	savefile->WriteVec3(baseGatorPos);
	savefile->WriteInt(resettime_up);
	savefile->WriteInt(resettime_down);
	savefile->WriteInt(resettime_left);
	savefile->WriteInt(resettime_right);
	savefile->WriteInt(wheelValue);
	savefile->WriteInt(wheelMovetime);
	savefile->WriteInt(wheelMovedir);
	savefile->WriteInt(beamUpdateTime);
	savefile->WriteFloat(remoteRaiseLerp);
	savefile->WriteInt(lastGametime);

	for (int i = 0; i < LINECOUNT; i++)
	{
		savefile->WriteObject(beamStarts[i]);
		savefile->WriteObject(beamEnds[i]);
	}	

	savefile->WriteVec3(hitpointPos);
	savefile->WriteVec3(launchDir);
	savefile->WriteFloat(force);

	forcefieldIgnore.Save(savefile);
}

void idLauncher::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass *&>(peakEnt));
	savefile->ReadObject(reinterpret_cast<idClass *&>(jumpPad));
	savefile->ReadObject(reinterpret_cast<idClass *&>(gatorEnt));
	savefile->ReadObject(reinterpret_cast<idClass *&>(remoteEnt));
	savefile->ReadObject(reinterpret_cast<idClass *&>(hitpointEnt));
	savefile->ReadObject(reinterpret_cast<idClass *&>(displaymodel));
	savefile->ReadObject(reinterpret_cast<idClass *&>(particles));
	


	savefile->ReadInt(state);
	savefile->ReadVec3(lastLaunchdir);
	savefile->ReadFloat(lastForce);	
	savefile->ReadInt(nextAimrecord);
	savefile->ReadInt(remoteLerpStart);
	savefile->ReadInt(remoteLerpEnd);
	savefile->ReadVec3(baseGatorPos);
	savefile->ReadInt(resettime_up);
	savefile->ReadInt(resettime_down);
	savefile->ReadInt(resettime_left);
	savefile->ReadInt(resettime_right);
	savefile->ReadInt(wheelValue);
	savefile->ReadInt(wheelMovetime);
	savefile->ReadInt(wheelMovedir);
	savefile->ReadInt(beamUpdateTime);
	savefile->ReadFloat(remoteRaiseLerp);
	savefile->ReadInt(lastGametime);

	for (int i = 0; i < LINECOUNT; i++)
	{
		savefile->ReadObject(reinterpret_cast<idClass *&>(beamStarts[i]));
		savefile->ReadObject(reinterpret_cast<idClass *&>(beamEnds[i]));
	}	

	savefile->ReadVec3(hitpointPos);
	savefile->ReadVec3(launchDir);
	savefile->ReadFloat(force);


	forcefieldIgnore.Restore(savefile);
	//this->forcefieldIgnore = jumpPad;
}

void idLauncher::Spawn( void )
{
	int i;
	idVec3 forward, right, up;
	idDict args;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() + idVec3(0,0,0.1f) );
	args.Set( "classname", "info_location" );
	gameLocal.SpawnEntityDef( args, &peakEnt );
	peakEnt->Bind(this, false);


	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() );
	args.SetVector( "mins", idVec3(-JUMPPADWIDTH, -JUMPPADWIDTH, 3) );
	args.SetVector( "maxs", idVec3(JUMPPADWIDTH, JUMPPADWIDTH, JUMPPADHEIGHT) );
	args.Set( "target", peakEnt->GetName() );
	args.Set("snd_jump", "launcher_fire");
	args.Set("smoke_jump", spawnArgs.GetString("smoke_burst"));
	args.Set("owner", this->GetName());
	jumpPad = ( idJumpPad * )gameLocal.SpawnEntityType( idJumpPad::Type, &args );
	jumpPad->Bind(this, false);
	
	args.Clear();
	args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * -16) + (up * 4) );
	args.Set("skin", "skins/cable/nophone_brown");
	args.Set("classname", spawnArgs.GetString("model_cable"));
	args.SetInt("isremote", 1);
	args.Set("master", this->GetName());
	gameLocal.SpawnEntityDef(args, &gatorEnt);
	gatorEnt->GetPhysics()->SetAxis(forward.ToMat3());
	gatorEnt->Bind( this, false );

	//gatorEnt->ApplyImpulse( gatorEnt, 13 , gatorEnt->GetPhysics()->GetOrigin(), idVec3(0, 0, 4096));
	//gatorEnt->GetPhysics()->SetLinearVelocity(idVec3(0,0,4096));

	args.Clear();
	args.SetVector("origin", this->GetPhysics()->GetOrigin());
	args.Set("classname", "moveable_launcherremote");
	gameLocal.SpawnEntityDef(args, &remoteEnt);

	
	args.Clear();
	args.SetVector("origin", this->GetPhysics()->GetOrigin());
	args.SetInt("solid", 0);
	args.SetBool("noclipmodel", 1);
	args.Set("model", spawnArgs.GetString("model_hitpoint"));
	hitpointEnt = ( idStaticEntity * )gameLocal.SpawnEntityType( idStaticEntity::Type, &args );	
	//hitpointEnt->BecomeActive(TH_UPDATEVISUALS);
	hitpointEnt->Bind(this, false);

	args.Clear();
	args.SetVector("origin", this->GetPhysics()->GetOrigin());
	args.Set("classname", "env_launchermodel");
	gameLocal.SpawnEntityDef(args, &displaymodel);
	displaymodel->SetAxis(this->GetPhysics()->GetAxis());
	displaymodel->Bind(this, true);



	args.Clear();
	args.SetVector("origin", this->GetPhysics()->GetOrigin());
	args.Set("classname", "func_emitter");
	args.Set( "model", spawnArgs.GetString("smoke_fan") );
	gameLocal.SpawnEntityDef(args, &particles);
	particles->Bind(this, true);
	
	



	this->forcefieldIgnore = jumpPad;
	force = 192;
	launchDir = idVec3(-45, 0, 0);
	nextAimrecord = 0;
	remoteLerpStart  = 0;
	remoteLerpEnd  = 0;

	remoteRaiseLerp = 0;
	lastGametime = 0;
	
	for (i = 0;i < LINECOUNT; i++)
	{
		idDict args;
		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin());
		beamEnds[i] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );


		args.Clear();
		args.Set("skin", "skins/beam_launcher");
		args.SetInt("width", BEAMWIDTH);
		args.SetVector("origin", this->GetPhysics()->GetOrigin());
		args.Set("target", beamEnds[i]->GetName() );
		beamStarts[i] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );


		if (i > 0)
		{
			beamEnds[i]->Bind(this, false);
			beamStarts[i]->Bind(this, false);			
		}
	}

	//bind first beam to launcher model.
	beamStarts[0]->SetOrigin(this->GetPhysics()->GetOrigin());
	beamStarts[0]->Bind(this, false);
	beamEnds[0]->SetOrigin(peakEnt->GetPhysics()->GetOrigin());
	beamEnds[0]->Bind(peakEnt, false);	

	beamStarts[0]->BecomeActive(TH_PHYSICS);


	resettime_up = 0;
	resettime_down = 0;
	resettime_left = 0;
	resettime_right = 0;
	wheelMovetime = 0;
	wheelValue = 0;
	beamUpdateTime = 0;


	UpdateBeams();
	UpdateMultiplier();

	BecomeActive( TH_THINK );

	StartSound( "snd_idle", SND_CHANNEL_HEART, 0, false, NULL );
}

void idLauncher::SetLaunchDir(idVec3 newdir, float newForce)
{
	launchDir = newdir;
	force = newForce;

	UpdateBeams();
}

void idLauncher::UpdateBeams( void )
{
	int i;

	idAngles launchdirNormalized = idAngles(launchDir.x, launchDir.y, launchDir.z);
	idVec3 forward = launchdirNormalized.ToForward();

	idVec3 peakPos = forward * force;
	peakEnt->SetOrigin(peakPos);

	idAngles particleDir = forward.ToAngles();
	//particleDir.yaw += 180;
	particleDir.pitch = particleDir.pitch + 90;

	particles->GetPhysics()->SetAxis(particleDir.ToMat3());
	

	//determine whether first beam hits a wall.
	trace_t trPeak;

	peakEnt->Unbind();
	gameLocal.clip.TracePoint(trPeak,
		GetPhysics()->GetOrigin(),
		peakEnt->GetPhysics()->GetOrigin(),
		MASK_SOLID, this);
	peakEnt->Bind(this, true);

	if (trPeak.fraction < 1)
	{
		peakEnt->Unbind();
		peakEnt->SetOrigin(trPeak.endpos);
		peakEnt->Bind(this, true);	

		hitpointEnt->Unbind();
		hitpointEnt->GetPhysics()->SetOrigin(trPeak.endpos);
		hitpointEnt->GetPhysics()->SetAxis(trPeak.c.normal.ToMat3());
		hitpointEnt->Bind(this, false);

		hitpointPos = trPeak.endpos;

		//hide the rest of the beams.
		for (i = 1; i < LINECOUNT; i++)
		{
			beamStarts[i]->Hide();
		}

		return;
	}

	//set up the first descent beam.
	idAngles beam1Normalized = idAngles(0, launchDir.y, 0);
	idVec3 forwardbeam1 = beam1Normalized.ToForward();

	float horizontalDist = idVec3(peakPos.x, peakPos.y, 0).Length();
	float verticalDist = peakPos.z;

	idVec3 lastPos = peakPos;

	bool placedHitpoint = false;
	for (i = 1; i < LINECOUNT; i++)
	{
		idVec3 endPos = lastPos + (forwardbeam1 * horizontalDist) + (idVec3(0,0,-1) * verticalDist);

        //BC 2-11-2026: possible crash fix for interacting with launcher remote while restarting the level.
        if (beamStarts[i]->entityNumber >= MAX_GENTITIES - 1
            || beamEnds[i]->entityNumber >= MAX_GENTITIES -1)
            return;

		beamStarts[i]->SetOrigin(lastPos);
		beamEnds[i]->SetOrigin(endPos);

		lastPos = endPos;
		verticalDist = verticalDist * 2;
	}

	if (gameLocal.time > nextAimrecord)
	{
		nextAimrecord = gameLocal.time + 200;

		if (lastLaunchdir.x != launchDir.x || lastLaunchdir.y != launchDir.y || lastLaunchdir.z != launchDir.z
			|| lastForce != force)
		{
			idEntity *recordEnt;
			recordEnt = gameLocal.FindEntity( "world1" );
			if (recordEnt)
			{
				int id = this->spawnArgs.GetInt("id", "-1");
				static_cast<idWorldManager *>( recordEnt )->RecordLaunchAim(id, launchDir, force);
			}

			lastLaunchdir.x = launchDir.x;
			lastLaunchdir.y = launchDir.y;
			lastLaunchdir.z = launchDir.z;
			lastForce = force;
		}
	}



	
	int lastValidTraceIndex = 0;
	for (i = 1; i < LINECOUNT; i++)
	{
		trace_t tr;

		gameLocal.clip.TracePoint(tr, 
			beamStarts[i]->GetPhysics()->GetOrigin(),
			beamEnds[i]->GetPhysics()->GetOrigin(),
			MASK_SOLID, this);

		if (tr.fraction >= 1)
			continue;

		lastValidTraceIndex = i;

		hitpointEnt->Unbind();
		hitpointEnt->GetPhysics()->SetOrigin(tr.endpos);
		hitpointEnt->GetPhysics()->SetAxis(tr.c.normal.ToMat3());
		hitpointEnt->UpdateVisuals();
		hitpointEnt->Bind(this, false);

		hitpointPos = tr.endpos;

		beamEnds[i]->Unbind();
		beamEnds[i]->GetPhysics()->SetOrigin(tr.endpos);
		beamEnds[i]->Bind(this, true);

		break;
	}

	
	for (i = 0; i < LINECOUNT; i++)
	{
		if (i > lastValidTraceIndex)
			beamStarts[i]->Hide();
		else
			beamStarts[i]->Show();
	}
}

void idLauncher::OnFrob( idEntity* activator )
{
	gameLocal.GetLocalPlayer()->Event_usePicker(gatorEnt);
}

void idLauncher::OnYaw(int value)
{
	launchDir.y += value * YAW_STEP;

	idAngles ang = displaymodel->GetPhysics()->GetAxis().ToAngles();
	ang.yaw = launchDir.y;

	jointHandle_t jointHandle;
	jointHandle = displaymodel->GetAnimator()->GetJointHandle( "body" );
	static_cast< idAnimatedEntity * >( displaymodel )->GetAnimator()->SetJointAxis(jointHandle, JOINTMOD_WORLD, ang.ToMat3());

	if (value > 0)
	{
		MoveBone("a", idVec3(-0.7f,0,0));

		if (resettime_right < gameLocal.time)
		{
			remoteEnt->StartSound( "snd_button", SND_CHANNEL_ANY, 0, false, NULL );
		}
		
		resettime_right = gameLocal.time + RESETTIME;
	}
	else
	{
		MoveBone("d", idVec3(-0.7f,0,0));

		if (resettime_left < gameLocal.time)
		{
			remoteEnt->StartSound( "snd_button", SND_CHANNEL_ANY, 0, false, NULL );
		}

		resettime_left = gameLocal.time + RESETTIME;
	}

	UpdateBeams();
}

void idLauncher::OnPitch(int value)
{
	bool playError = false;
	launchDir.x += value;

	if (launchDir.x < MAX_PITCH)
	{
		launchDir.x = MAX_PITCH;
		playError = true;
	}
	else if (launchDir.x > MIN_PITCH)
	{
		launchDir.x = MIN_PITCH;
		playError = true;
	}

	if (value > 0)
	{
		MoveBone("w", idVec3(-0.7f,0,0));

		if (resettime_up < gameLocal.time)
		{
			remoteEnt->StartSound( "snd_button", SND_CHANNEL_ANY, 0, false, NULL );
		}

		resettime_up = gameLocal.time + RESETTIME;
	}
	else
	{
		MoveBone("s", idVec3(-0.7f,0,0));

		if (resettime_down < gameLocal.time)
		{
			remoteEnt->StartSound( "snd_button", SND_CHANNEL_ANY, 0, false, NULL );
		}

		resettime_down = gameLocal.time + RESETTIME;
	}

	if (playError)
	{
		remoteEnt->StartSound( "snd_error", SND_CHANNEL_ANY, 0, false, NULL );
	}

	UpdateBeams();
}

void idLauncher::OnWheelMove(int value)
{
	bool playError = false;

	force += WHEEL_STEP * value;

	if (force > MAX_FORCE)
	{
		force = MAX_FORCE;
		playError = true;
	}
	else if (force < MIN_FORCE)
	{
		force = MIN_FORCE;
		playError = true;
	}

	if (value < 0)
		wheelMovedir = WHEELSPEED;
	else
		wheelMovedir = -WHEELSPEED;

	if (wheelMovetime < gameLocal.time)	
	{
		remoteEnt->StartSound( "snd_dial", SND_CHANNEL_ANY, 0, false, NULL );
	}
	
	if (playError)
	{
		remoteEnt->StartSound( "snd_error", SND_CHANNEL_ANY, 0, false, NULL );
	}

	wheelMovetime = gameLocal.time + WHEELTIME;

	UpdateBeams();


	UpdateMultiplier();
}

void idLauncher::UpdateMultiplier()
{
	//magic numbers hack........


	//determine force based on mousewheel power.
	float forceLerp = (force - MIN_FORCE) / (float)(MAX_FORCE - MIN_FORCE);
	float forceMultiplier = idMath::Lerp(MULTIPLIER_MIN, MULTIPLIER_MAX, forceLerp);


	//adjust force based on altitude delta.
	float altitudeDelta = this->GetPhysics()->GetOrigin().z - hitpointPos.z;

	float altitude_minMultiplier = 0.75f;
	float altitude_maxMultiplier = 1.0f;
	float altitude_distanceThreshold = 384.0f;

	if (altitudeDelta <= 0)
	{
		forceMultiplier *= altitude_minMultiplier;
	}
	else if (altitudeDelta >= altitude_distanceThreshold)
	{
		//use 100% force, so do nothing here.
	}
	else
	{
		//scale the force.
		float altitudeLerp = altitudeDelta / altitude_distanceThreshold;

		float scaleAmount = idMath::Lerp(altitude_minMultiplier, altitude_maxMultiplier, altitudeLerp);
		forceMultiplier *= scaleAmount;
	}	


	jumpPad->Event_UpdateMultiplier(forceMultiplier);
	//common->Printf("new force multiplier: %f\n", forceMultiplier);
}

void idLauncher::MoveBone(const char *bonename, idVec3 direction)
{
	if (remoteEnt == NULL)
		return;

    //BC 2-11-2026: possible crash fix
    if (remoteEnt->entityNumber >= MAX_GENTITIES - 1)
        return;

    if (remoteEnt->GetAnimator() == NULL)
        return;

	jointHandle_t jointHandle;
	jointHandle = remoteEnt->GetAnimator()->GetJointHandle( bonename );
	static_cast< idAnimatedEntity * >( remoteEnt )->GetAnimator()->SetJointPos(jointHandle, JOINTMOD_LOCAL, direction);
}

idVec3 idLauncher::GetGatorPos( void )
{
	jointHandle_t jointHandle;
	idVec3 jointPos;
	idMat3 jointAxis;

	jointHandle = gatorEnt->GetAnimator()->GetJointHandle( "bone16" );
	static_cast< idAnimatedEntity * >(gatorEnt)->GetJointWorldTransform(jointHandle, gameLocal.time, jointPos, jointAxis );
	return jointPos;
}

void idLauncher::Think( void )
{	
	int i;





	jumpPad->Event_UpdatePeak();
	
	if (gameLocal.time > resettime_up)
	{
		MoveBone("w", idVec3(0,0,0));
	}

	if (gameLocal.time > resettime_down)
	{
		MoveBone("s", idVec3(0,0,0));
	}

	if (gameLocal.time > resettime_left)
	{
		MoveBone("a", idVec3(0,0,0));
	}

	if (gameLocal.time > resettime_right)
	{
		MoveBone("d", idVec3(0,0,0));
	}

	if (gameLocal.time < wheelMovetime)
	{
		wheelValue += wheelMovedir;

		idAngles ang;
		ang.pitch = wheelValue;
		ang.yaw = 0;
		ang.roll = 0;

		jointHandle_t jointHandle;
		jointHandle = remoteEnt->GetAnimator()->GetJointHandle( "wheel" );
		static_cast< idAnimatedEntity * >( remoteEnt )->GetAnimator()->SetJointAxis(jointHandle, JOINTMOD_LOCAL, ang.ToMat3());
	}


	if (gameLocal.GetLocalPlayer()->Event_isHolding(gatorEnt))
	{
		if (remoteRaiseLerp <= 0)
		{
			baseGatorPos = GetGatorPos();
		}

		remoteRaiseLerp += gameLocal.time - lastGametime;

		if (remoteRaiseLerp >= RISETIME)
		{
			remoteRaiseLerp = RISETIME;
		}

		if (remoteEnt->IsBound())
		{
			remoteEnt->Unbind();
		}

		//gameLocal.GetLocalPlayer()->viewAngles

		

		idAngles remoteAng = gameLocal.GetLocalPlayer()->viewAngles;
		remoteAng.yaw += 180;
		remoteAng.pitch *= -1;
		remoteAng.pitch += 3 * idMath::Sin(gameLocal.time * 0.0005f); //sway
		remoteAng.yaw += 5 * idMath::Sin(gameLocal.time * 0.0003f);  //sway
		remoteEnt->SetAngles( remoteAng );

		idVec3	up;
		remoteAng.ToVectors( NULL, NULL, &up );


		idVec3 finalPos = gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->viewAngles.ToForward() * 6) + (up * -3.8f     );

		if (remoteRaiseLerp < RISETIME)
		{
			//is raising.
			float amt = remoteRaiseLerp / RISETIME;
			finalPos.x = idMath::Lerp(baseGatorPos.x, finalPos.x, amt);
			finalPos.y = idMath::Lerp(baseGatorPos.y, finalPos.y, amt);
			finalPos.z = idMath::Lerp(baseGatorPos.z, finalPos.z, amt);
		}


		//remoteEnt->SetOrigin( gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->viewAngles.ToForward() * 6) + (up * -3.8f     ));
		remoteEnt->SetOrigin(finalPos);
	}
	else
	{
		remoteRaiseLerp -= gameLocal.time - lastGametime;

		if (remoteRaiseLerp <= 0)
		{
			remoteRaiseLerp = 0;
		}

		if (!remoteEnt->IsBound())
		{
			idVec3 jointPos = GetGatorPos();
			jointHandle_t jointHandle;

			jointHandle = gatorEnt->GetAnimator()->GetJointHandle( "bone16" );

			remoteEnt->SetOrigin(jointPos);
			remoteEnt->BindToJoint(gatorEnt, jointHandle, true);
		}
	}

	//common->Printf("%f\n", remoteRaiseLerp);
	lastGametime = gameLocal.time;







	if (gameLocal.time > beamUpdateTime)
	{
		beamUpdateTime = gameLocal.time + 300;
		UpdateBeams();
	}



	//RunPhysics();
	idMoveableItem::Think();
	//idMoveableItem::Present();
}


void idLauncher::OnGet( void )
{
	StopSound( SND_CHANNEL_HEART, false );	
	StopSound( SND_CHANNEL_ANY, false );	

	//particle effect.
	idDict args;
	args.Clear();
	args.SetVector( "origin",		GetPhysics()->GetOrigin());
	args.Set(		"model",		spawnArgs.GetString("smoke_pickup") );
	args.SetBool(	"start_off",	false );
	gameLocal.SpawnEntityType( idExplodable::Type, &args );


	hitpointEnt->PostEventMS( &EV_Remove, 0 );
	this->PostEventMS( &EV_Remove, 0 );

	gameLocal.GetLocalPlayer()->GiveInventoryItem("weapon_launcher");
	gameLocal.GetLocalPlayer()->GiveInventoryItem("ammo_launchers_small");	


	gameLocal.GetLocalPlayer()->StartSound( "snd_get" , SND_CHANNEL_ANY, 0, false, NULL );
}

void idLauncher::launcherkill( void )
{
	StopSound( SND_CHANNEL_HEART, false );	
	StopSound( SND_CHANNEL_ANY, false );	
}