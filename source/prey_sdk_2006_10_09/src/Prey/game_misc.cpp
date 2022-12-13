#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


//==========================================================================
//
// hhWallWalkable
//
//==========================================================================

CLASS_DECLARATION( idStaticEntity, hhWallWalkable )
	EVENT(EV_Activate,			hhWallWalkable::Event_Activate)
END_CLASS

void hhWallWalkable::Spawn( void ) {
	wallwalkOn = spawnArgs.GetBool("active");
	flicker	= spawnArgs.GetBool("flicker");

	// Get skin references (already precached)
	onSkin = declManager->FindSkin( spawnArgs.GetString("skinOn") );
	offSkin = declManager->FindSkin( spawnArgs.GetString("skinOff") );

	if (wallwalkOn) {
		SetSkin( onSkin );
		alphaOn.Init(gameLocal.time, 0, 1.0f, 1.0f);
	}
	else {
		SetSkin( offSkin );
		alphaOn.Init(gameLocal.time, 0, 0.0f, 0.0f);
	}
	alphaOn.SetHermiteParms(WALLWALK_HERM_S1, WALLWALK_HERM_S2);
	SetShaderParm(4, alphaOn.GetCurrentValue(gameLocal.time));
	UpdateVisuals();

	fl.networkSync = true;
}

void hhWallWalkable::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( alphaOn.GetStartTime() );	// hhHermiteInterpolate<float>
	savefile->WriteFloat( alphaOn.GetDuration() );
	savefile->WriteFloat( alphaOn.GetStartValue() );
	savefile->WriteFloat( alphaOn.GetEndValue() );
	savefile->WriteFloat( alphaOn.GetS1() );
	savefile->WriteFloat( alphaOn.GetS2() );

	savefile->WriteBool( wallwalkOn );
	savefile->WriteBool( flicker );
	savefile->WriteSkin( onSkin );
	savefile->WriteSkin( offSkin );
}

void hhWallWalkable::Restore( idRestoreGame *savefile ) {
	float set, set2;

	savefile->ReadFloat( set );			// hhHermiteInterpolate<float>
	alphaOn.SetStartTime( set );
	savefile->ReadFloat( set );
	alphaOn.SetDuration( set );
	savefile->ReadFloat( set );
	alphaOn.SetStartValue(set);
	savefile->ReadFloat( set );
	alphaOn.SetEndValue( set );
	savefile->ReadFloat( set );
	savefile->ReadFloat( set2 );
	alphaOn.SetHermiteParms(set, set2);

	savefile->ReadBool( wallwalkOn );
	savefile->ReadBool( flicker );
	savefile->ReadSkin( onSkin );
	savefile->ReadSkin( offSkin );
}

void hhWallWalkable::Think() {
	idEntity::Think();
	if (thinkFlags & TH_THINK) {
		SetShaderParm(4, alphaOn.GetCurrentValue(gameLocal.time));
		if (alphaOn.IsDone(gameLocal.time)) {
			BecomeInactive(TH_THINK);
			if (wallwalkOn) {
			}
			else {
				SetSkin( offSkin );
			}
		}
	}
}

void hhWallWalkable::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(wallwalkOn, 1);
	msg.WriteBits(IsActive(TH_THINK), 1);
}

void hhWallWalkable::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	bool enabled = !!msg.ReadBits(1);
	if (wallwalkOn != enabled) {
		SetWallWalkable(enabled);
	}

	bool thinking = !!msg.ReadBits(1);
	if (thinking != IsActive(TH_THINK)) {
		if (thinking) {
			BecomeActive(TH_THINK);
		}
		else {
			BecomeInactive(TH_THINK);
		}
	}
}

void hhWallWalkable::ClientPredictionThink( void ) {
	Think();
}

void hhWallWalkable::SetWallWalkable(bool on) {
	wallwalkOn = on;

	float curAlpha = alphaOn.GetCurrentValue(gameLocal.time);

	if (wallwalkOn) {	// Turning on
		BecomeActive(TH_THINK);
		SetSkin( onSkin );
		StartSound( "snd_powerup", SND_CHANNEL_ANY );
		alphaOn.Init(gameLocal.time, WALLWALK_TRANSITION_TIME, curAlpha, 1.0f );
		alphaOn.SetHermiteParms(WALLWALK_HERM_S1, WALLWALK_HERM_S2);
	}
	else {				// Turning off
		BecomeActive(TH_THINK);
		StartSound( "snd_powerdown", SND_CHANNEL_ANY );
		alphaOn.Init(gameLocal.time, WALLWALK_TRANSITION_TIME, curAlpha, 0.0f);
		alphaOn.SetHermiteParms(WALLWALK_HERM_S1, 1.0f);	// no overshoot
	}
}

void hhWallWalkable::Event_Activate(idEntity *activator) {
	SetWallWalkable(!wallwalkOn);
}

//==========================================================================
//
// hhFuncEmitter
//
//==========================================================================
CLASS_DECLARATION( idStaticEntity, hhFuncEmitter )
	EVENT( EV_Activate,			hhFuncEmitter::Event_Activate )
END_CLASS

void hhFuncEmitter::Spawn( void ) {
	particle = static_cast<const idDeclParticle*>( declManager->FindType(DECL_PARTICLE, spawnArgs.GetString("smoke_particle"), false) );
	particleStartTime = -1;

	(spawnArgs.GetBool("start_off")) ? Hide() : Show();
}

void hhFuncEmitter::Hide() {
	idStaticEntity::Hide();
	
	renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC( gameLocal.time );

	particleStartTime = -1;

	BecomeInactive( TH_TICKER );
}

void hhFuncEmitter::Show() {
	idStaticEntity::Show();

	renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = 0;
	renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.GetTime() );

	particleStartTime = gameLocal.GetTime();

	BecomeActive( TH_TICKER );
}

void hhFuncEmitter::Save( idSaveGame *savefile ) const {
	savefile->WriteParticle( particle );
	savefile->WriteInt( particleStartTime );
}

void hhFuncEmitter::Restore( idRestoreGame *savefile ) {
	savefile->ReadParticle( particle );
	savefile->ReadInt( particleStartTime );
}

void hhFuncEmitter::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteFloat( renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] );
	msg.WriteFloat( renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
}

void hhFuncEmitter::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] = msg.ReadFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = msg.ReadFloat();
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

void hhFuncEmitter::Ticker() {
	if( IsHidden() ) {
		return;
	}

	if( particle && particleStartTime != -1 ) {
		if( !gameLocal.smokeParticles->EmitSmoke(particle, particleStartTime, gameLocal.random.RandomFloat(), GetOrigin(), GetAxis()) ) {
			particleStartTime = -1;
		}
	}

	if( modelDefHandle != -1 ) {
		renderEntity.origin = GetOrigin();
		renderEntity.axis = GetAxis();
		UpdateVisuals();
	}
}

void hhFuncEmitter::Event_Activate( idEntity *activator ) {
	(IsHidden() || spawnArgs.GetBool("cycleTrigger")) ? Show() : Hide();

	UpdateVisuals();
}


//==========================================================================
//
// hhPathEmitter
//
//==========================================================================

CLASS_DECLARATION( hhFuncEmitter, hhPathEmitter )
END_CLASS

void hhPathEmitter::Spawn() {
}

//==========================================================================
//
// hhDeathWraithEnergy
//
//==========================================================================

CLASS_DECLARATION( hhPathEmitter, hhDeathWraithEnergy )
END_CLASS

void hhDeathWraithEnergy::Spawn() {
	startTime = MS2SEC(gameLocal.time);
	duration = spawnArgs.GetFloat("duration");

	startRadius = spawnArgs.GetFloat("startRadius");
	endRadius = spawnArgs.GetFloat("endRadius");
	startTheta = DEG2RAD(spawnArgs.GetFloat("startTheta"));
	endTheta = DEG2RAD(spawnArgs.GetFloat("endTheta"));
	startZ = spawnArgs.GetFloat("startZ");
	endZ = spawnArgs.GetFloat("endZ");

	// For testing
	SetDestination(vec3_origin);
	SetPlayer(static_cast<hhPlayer*>(gameLocal.GetLocalPlayer()));

	StartSound("snd_idle", SND_CHANNEL_BODY);
}

void hhDeathWraithEnergy::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( startTime );
	savefile->WriteFloat( duration );
	savefile->WriteFloat( startRadius );
	savefile->WriteFloat( endRadius );
	savefile->WriteFloat( startTheta );
	savefile->WriteFloat( endTheta );
	savefile->WriteFloat( startZ );
	savefile->WriteFloat( endZ );
	savefile->WriteVec3( centerPosition );
	thePlayer.Save(savefile);
	
	savefile->WriteFloat(spline.tension);
	savefile->WriteFloat(spline.continuity);
	savefile->WriteFloat(spline.bias);
	savefile->WriteInt(spline.nodes.Num());				// idList<idVec3>
	for (int i=0; i<spline.nodes.Num(); i++) {
		savefile->WriteVec3(spline.nodes[i]);
	}
}

void hhDeathWraithEnergy::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( startTime );
	savefile->ReadFloat( duration );
	savefile->ReadFloat( startRadius );
	savefile->ReadFloat( endRadius );
	savefile->ReadFloat( startTheta );
	savefile->ReadFloat( endTheta );
	savefile->ReadFloat( startZ );
	savefile->ReadFloat( endZ );
	savefile->ReadVec3( centerPosition );
	thePlayer.Restore(savefile);

	savefile->ReadFloat(spline.tension);
	savefile->ReadFloat(spline.continuity);
	savefile->ReadFloat(spline.bias);

	int num;
	spline.nodes.Clear();								// idList<idVec3>
	savefile->ReadInt(num);
	spline.nodes.SetNum(num);
	for (int i=0; i<num; i++) {
		savefile->ReadVec3(spline.nodes[i]);
	}
}

void hhDeathWraithEnergy::SetPlayer(hhPlayer *player) {
	thePlayer = player;
}

void hhDeathWraithEnergy::SetDestination(const idVec3 &destination) {
	// Cylindrical support
	this->centerPosition = destination;
	idVec3 toWraith = GetOrigin() - centerPosition;
	CartesianToCylindrical(toWraith, startRadius, startTheta, startZ);
	endTheta += startTheta;

	// Spline support
	spline.Clear();
	spline.SetControls(spawnArgs.GetFloat("tension"), spawnArgs.GetFloat("continuity"), spawnArgs.GetFloat("bias"));
	spline.AddPoint(GetOrigin());

	if (thePlayer.IsValid() && thePlayer->DeathWalkStage2()) {
		idEntity *holeMarker = gameLocal.FindEntity( "dw_floatingBodyMarker" );
		if (holeMarker) {
			spline.AddPoint(holeMarker->GetOrigin());
		}
	}

	spline.AddPoint(destination);
}

void hhDeathWraithEnergy::CartesianToCylindrical(idVec3 &cartesian, float &radius, float &theta, float &z) {
	radius = cartesian.ToVec2().Length();
	theta = idMath::ATan(cartesian.y, cartesian.x);
	z = cartesian.z;
}

idVec3 hhDeathWraithEnergy::CylindricalToCartesian(float radius, float theta, float z) {
	idVec3 cartesian;
	cartesian.x = radius * idMath::Cos(theta);
	cartesian.y = radius * idMath::Sin(theta);
	cartesian.z = z;
	return cartesian;
}

void hhDeathWraithEnergy::Ticker() {
	float theta;
	float radius;
	float z;

	if (!thePlayer.IsValid()) {
		return;
	}

	float alpha = (MS2SEC(gameLocal.time) - startTime) / duration;

	if (alpha < 1.0f) {

		if (thePlayer->DeathWalkStage2()) {
			SetOrigin( spline.GetValue(alpha) );
		}
		else {
			radius = startRadius + alpha*(endRadius-startRadius);
			theta = startTheta + alpha*(endTheta-startTheta);
			z = startZ + alpha * (endZ - startZ);

			idVec3 locationRelativeToCenter = CylindricalToCartesian(radius, theta, z);
			idEntity *destEntity = thePlayer->GetDeathwalkEnergyDestination();
			if (destEntity) {
				centerPosition = destEntity->GetOrigin();
			}

			SetOrigin(centerPosition + locationRelativeToCenter);
		}
	}
	else if (!IsHidden()) {
		Hide();
		StopSound(SND_CHANNEL_BODY);

		bool energyHealth = spawnArgs.GetBool("healthEnergy");

		idEntity *dwProxy = thePlayer->GetDeathwalkEnergyDestination();
		if (dwProxy) {
			// Spawn arrival effect
			StartSound("snd_arrival", SND_CHANNEL_ANY);

			dwProxy->SetShaderParm(SHADERPARM_TIMEOFFSET, -MS2SEC(gameLocal.time) );
			dwProxy->SetShaderParm(SHADERPARM_MODE, energyHealth ? 2 : 1 );
		}

		// Notify the player
		thePlayer->DeathWraithEnergyArived(energyHealth);

		PostEventMS(&EV_Remove, 5000);
	}

	hhPathEmitter::Ticker();
}


