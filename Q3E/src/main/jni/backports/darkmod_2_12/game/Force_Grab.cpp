/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "Force_Grab.h"
#include "Grabber.h"

class CDarkModPlayer;

CLASS_DECLARATION( idForce, CForce_Grab )
END_CLASS

/*
================
CForce_Grab::CForce_Grab
================
*/
CForce_Grab::CForce_Grab( void ) 
{
	m_damping			= 0.0f;
	m_physics			= NULL;
	m_id				= 0;
	m_p					= vec3_zero;
	m_dragPosition		= vec3_zero;
	m_dragAxis.Zero();
	m_centerOfMass		= vec3_zero;
	m_inertiaTensor.Identity();

	m_bApplyDamping = false;
	m_bLimitForce = false;
	m_RefEnt = NULL;
}

/*
================
CForce_Grab::~CForce_Grab
================
*/
CForce_Grab::~CForce_Grab( void ) {
}

/*
================
CForce_Grab::Init
================
*/
void CForce_Grab::Init( float damping ) {
	if ( damping >= 0.0f && damping < 1.0f ) 
	{
		m_damping = damping;
	}
	else
	{
		m_damping = 0;
	}
	m_dragPositionFrames.Clear();
}

void CForce_Grab::Save( idSaveGame *savefile ) const
{
	m_RefEnt.Save( savefile );
	savefile->WriteFloat(m_damping);
	savefile->WriteVec3(m_centerOfMass);
	savefile->WriteMat3(m_inertiaTensor);

	// Don't save m_physics, gets restored from the parent class after load
	
	savefile->WriteVec3(m_p);
	savefile->WriteInt(m_id);
	savefile->WriteVec3(m_dragPosition);
	savefile->WriteMat3(m_dragAxis);
	savefile->WriteBool(m_bApplyDamping);
	savefile->WriteBool(m_bLimitForce);

	savefile->WriteInt(m_dragPositionFrames.Num());
	for (int i = 0; i < m_dragPositionFrames.Num(); i++)
		savefile->WriteVec4(m_dragPositionFrames[i]);
	savefile->WriteInt(m_originalFriction.Num());
	for (int i = 0; i < m_originalFriction.Num(); i++)
		savefile->WriteVec3(m_originalFriction[i]);
}

void CForce_Grab::Restore( idRestoreGame *savefile )
{
	m_RefEnt.Restore( savefile );
	savefile->ReadFloat(m_damping);
	savefile->ReadVec3(m_centerOfMass);
	savefile->ReadMat3(m_inertiaTensor);

	m_physics = NULL; // gets restored from the parent class after loading

	savefile->ReadVec3(m_p);
	savefile->ReadInt(m_id);
	savefile->ReadVec3(m_dragPosition);
	savefile->ReadMat3(m_dragAxis);
	savefile->ReadBool(m_bApplyDamping);
	savefile->ReadBool(m_bLimitForce);

	int n;
	savefile->ReadInt(n);
	m_dragPositionFrames.SetNum(n, false);
	for (int i = 0; i < m_dragPositionFrames.Num(); i++)
		savefile->ReadVec4(m_dragPositionFrames[i]);
	savefile->ReadInt(n);
	m_originalFriction.SetNum(n, false);
	for (int i = 0; i < m_originalFriction.Num(); i++)
		savefile->ReadVec3(m_originalFriction[i]);
}

/*
================
CForce_Grab::SetFrictionOverride
================
*/
void CForce_Grab::SetFrictionOverride(bool enabled, float linear, float angular, float contact) {
	if (!m_physics)
		return;

	if (m_physics->IsType(idPhysics_AF::Type)) {
		idPhysics_AF* phys = (idPhysics_AF*)m_physics;
		int n = phys->GetNumBodies();
		if (enabled) {
			if (m_originalFriction.Num() != n) {
				m_originalFriction.SetNum(n, false);
				for (int i = 0; i < n; i++) {
					idAFBody* body = phys->GetBody(i);
					idVec3 friction(body->GetLinearFriction(), body->GetAngularFriction(), body->GetContactFriction());
					m_originalFriction[i] = friction;
				}
			}
			for (int i = 0; i < n; i++) {
				idAFBody* body = phys->GetBody(i);
				idVec3 friction = m_originalFriction[i];
				if (linear >= 0.0)
					friction.x = linear;
				if (angular >= 0.0)
					friction.y = angular;
				if (contact >= 0.0)
					friction.z = contact;
				body->SetFriction(friction.x, friction.y, friction.z);
			}
		}
		else {
			for (int i = 0; i < n && i < m_originalFriction.Num(); i++) {
				idAFBody* body = phys->GetBody(i);
				idVec3 friction = m_originalFriction[i];
				body->SetFriction(friction.x, friction.y, friction.z);
			}
			m_originalFriction.Clear();
		}
	}
}

/*
================
CForce_Grab::SetPhysics
================
*/
void CForce_Grab::SetPhysics( idPhysics *phys, int id, const idVec3 &p ) {
	float mass, MassOut, density;
	idClipModel *clipModel;

	// stgatilov #5599: restore normal air friction
	SetFrictionOverride(false, 0, 0, 0);

	m_physics = phys;
	m_id = id;
	m_p = p;

	if (m_physics) {
		clipModel = m_physics->GetClipModel( m_id );
		if ( clipModel != NULL && clipModel->IsTraceModel() ) 
		{
			mass = m_physics->GetMass( m_id );
			// PROBLEM: No way to query physics object for density!
			// Trick: Use a test density of 1.0 here, then divide the actual mass by output mass to get actual density
			clipModel->GetMassProperties( 1.0f, MassOut, m_centerOfMass, m_inertiaTensor );
			density = mass / MassOut;
			// Now correct the inertia tensor by using actual density
			clipModel->GetMassProperties( density, mass, m_centerOfMass, m_inertiaTensor );
		} else 
		{
			m_centerOfMass.Zero();
			m_inertiaTensor = mat3_identity;
		}
	}
}

/*
================
CForce_Grab::SetDragPosition
================
*/
void CForce_Grab::SetDragPosition( const idVec3 &pos ) 
{
	m_dragPosition = pos;
}

/*
================
CForce_Grab::GetDragPosition
================
*/
const idVec3 &CForce_Grab::GetDragPosition( void ) const 
{
	return m_dragPosition;
}

/*
================
CForce_Grab::SetDragAxis
================
*/
void CForce_Grab::SetDragAxis( const idMat3 &Axis )
{
	m_dragAxis = Axis;
}

/*
================
CForce_Grab::GetDragAxis
================
*/
idMat3 CForce_Grab::GetDragAxis( void )
{
	return m_dragAxis;
}

/*
================
CForce_Grab::GetDraggedPosition
================
*/
idVec3 CForce_Grab::GetDraggedPosition( void ) const 
{
	return ( m_physics->GetOrigin( m_id ) + m_p * m_physics->GetAxis( m_id ) );
}

/*
================
CForce_Grab::UpdateAverageDragPosition
================
*/
void CForce_Grab::UpdateAverageDragPosition( float dT ) {
	float fulltime = cv_drag_targetpos_averaging_time.GetFloat();
	// update ages of frames
	float aging = 0.0f;
	if (fulltime > 0.0)
		aging = idMath::Exp(-dT / fulltime);
	for (int i = 0; i < m_dragPositionFrames.Num(); i++)
		m_dragPositionFrames[i].w *= aging;
	// add a new frame
	m_dragPositionFrames.Append(idVec4(m_dragPosition.x, m_dragPosition.y, m_dragPosition.z, 1.0f));
	// remove obsolete frames
	while (m_dragPositionFrames[0].w < 0.1f) {
		m_dragPositionFrames.RemoveIndex(0);
		assert(m_dragPositionFrames.Num() > 0);	//current frame must never be removed
	}
}

/*
================
CForce_Grab::ComputeAverageDragPosition
================
*/
idVec3 CForce_Grab::ComputeAverageDragPosition() const {
	assert(m_dragPositionFrames.Num() > 0);
	// compute current average
	idVec3 sumDragPos(0.0f);
	float sumWeight = FLT_MIN;
	for (int i = 0; i < m_dragPositionFrames.Num(); i++) {
		sumDragPos += m_dragPositionFrames[i].ToVec3() * m_dragPositionFrames[i].w;
		sumWeight += m_dragPositionFrames[i].w;
	}
	sumDragPos /= sumWeight;
	return sumDragPos;
}

/*
================
CForce_Grab::Evaluate
================
*/
void CForce_Grab::Evaluate( int time ) 
{
	if ( !m_physics ) 
		return;

	CGrabber *grabber = gameLocal.m_Grabber;
	float dT = MS2SEC(gameLocal.getMsec());


	idVec3 velocity;

	if (cv_drag_new.GetBool()) {
		// ========================== NEW grabber ===========================

		// update and compute smoothened drag position
		UpdateAverageDragPosition(dT);
		idVec3 avgTargetDragPosition = ComputeAverageDragPosition();

		idVec3 draggedPosition = GetDraggedPosition();
		idVec3 dragShift = avgTargetDragPosition - draggedPosition;
		idVec3 COM = GetCenterOfMass();

		// AF dragging is a special story
		if ( m_physics->IsType( idPhysics_AF::Type ) ) {
			idPhysics_AF *phys = (idPhysics_AF*)m_physics;

			// compute total weight
			float totalMass = 0.0f;
			bool inAir = true;
			for (int i = 0; i < phys->GetNumBodies(); i++) {
				totalMass += phys->GetMass(i);
				if (phys->HasGroundContactsAtJoint(i))
					inAir = false;
			}
			float gravAccel = gameLocal.GetGravity().Length();
			float totalWeight = gravAccel * totalMass;

			// check if player can lift the AF or it must be always on ground
			bool mustGround = false;
			if (idEntity *entity = m_physics->GetSelf())
				if (entity->IsType(idAFEntity_Base::Type) && ((idAFEntity_Base*)entity)->m_bGroundWhenDragged)
					mustGround = true;
			if (cv_drag_AF_free.GetBool())
				mustGround = false;

			// compute force / weight factor
			float maxFactor = cv_drag_af_weight_ratio_canlift.GetFloat();
			if (mustGround)
				maxFactor = cv_drag_af_weight_ratio.GetFloat();
			maxFactor = idMath::Fmin(maxFactor, grabber->m_MaxForce / totalWeight);

			// compute weakening coefficient if distance to target is small
			idVec3 dir = dragShift;
			float len = dir.Normalize();
			float weakenRadius = cv_drag_af_reduceforce_radius.GetFloat();
			float weakenCoeff = idMath::Fmin(len / (weakenRadius + 1e-3f), 1.0f);

			if (inAir) {
				// apply force uniformly to all parts
				// otherwise, small body parts will oscillate wildly
				for (int i = 0; i < phys->GetNumBodies(); i++) {
					idVec3 force = weakenCoeff * maxFactor * gravAccel * phys->GetMass(i) * dir;
					m_physics->AddForce(i, phys->GetOrigin(i), force, this);
				}

				// temporarily increase air friction
				// to avoid whole-body oscillations
				float friction = cv_drag_af_inair_friction.GetFloat();
				SetFrictionOverride(true, friction, friction, friction);
			}
			else {
				// apply force to the grabbed part only
				idVec3 force = weakenCoeff * maxFactor * totalWeight * dir;
				m_physics->AddForce(m_id, COM, force, this);
				// restore normal friction coefficients
				SetFrictionOverride(false, 0, 0, 0);
			}
			
			// don't do anything else
			return;
		}

		// compute collision contacts of dragged item
		idVec6 dir;
		dir.Zero();
		// note: it is very important to specify the direction we are going to move to
		dir.SubVec3(0) = dragShift.Normalized();
		idClipModel *clipModel = m_physics->GetClipModel(m_id);
		contactInfo_t contacts[CONTACTS_MAX_NUMBER];
		int num = gameLocal.clip.Contacts(
			contacts, CONTACTS_MAX_NUMBER, clipModel->GetOrigin(),
			dir, CONTACT_EPSILON, clipModel, clipModel->GetAxis(),
			m_physics->GetClipMask(m_id), m_physics->GetSelf()
		);

		// filter contacts with what we consider to be obstacles
		idList<idVec3> normals;
		for (int i = 0; i < num; i++) {
			const contactInfo_t &contact = contacts[i];

			idEntity *contEnt = gameLocal.entities[contact.entityNum];
			if (!contEnt->GetPhysics())
				continue;

			// e.g. to fix candle and its holder
			if (contEnt->IsBoundTo(m_physics->GetSelf()) || m_physics->GetSelf()->IsBoundTo(contEnt))
				continue;	// same bind-team

			if (contEnt->GetPhysics()->IsType(idPhysics_Base::Type) && !grabber->IsInSilentMode())
				continue;	// we can push this thing

			// non-moveable obstacle: we have to slide along it
			normals.Append(contact.normal);
		}

		// project drag vector onto admissible cone allowed by obstacles
		dragShift = dragShift.ProjectToConvexCone(normals.Ptr(), normals.Num(), 0.01f);

		// compute velocity from drag vector
		float halfingTime = cv_drag_rigid_distance_halfing_time.GetFloat();
		float accelRadius = cv_drag_rigid_acceleration_radius.GetFloat();
		float period = halfingTime - idMath::Fmin(accelRadius / (1e-3f + dragShift.Length()), 1.0f) * (halfingTime - dT);
		// if accelRadius == 0, then item should travel to target along straight line,
		// with distance decreasing as D(t) = exp(t / T), where T = halfingTime
		velocity = dragShift / period;

		// ==================================================================
	}
	else {

		// ========================== OLD grabber ===========================

		idVec3 COM = this->GetCenterOfMass();
		idVec3 dragOrigin = COM;

		idVec3 dir1 = m_dragPosition - dragOrigin;
		double l1 = dir1.Normalize();
		dT = MS2SEC(gameLocal.getMsec()); // duzenko 4409: fixed tic + USERCMD_MSEC -> flickering
		if (dT < MS2SEC( USERCMD_MSEC )) // BluePill : Fix grab speed for higher framerates
			dT = MS2SEC( USERCMD_MSEC ); // time elapsed is time between user mouse commands

		if( !m_bApplyDamping )
			m_damping = 0.0f;

		idVec3 prevVel;
		if( grabber->m_bIsColliding )
		{
			// Zero out previous velocity when we start out colliding
			prevVel = vec3_zero;

			idVec3 newDir = dir1;

			for( int i=0; i < grabber->m_CollNorms.Num(); i++ )
			{
				// subtract out component of desired dir going in to surface
				if( newDir * grabber->m_CollNorms[i] < 0.0f )
				{
					newDir -= (newDir * grabber->m_CollNorms[i]) * grabber->m_CollNorms[i];
				}

				if( cv_drag_debug.GetBool() )
					gameRenderWorld->DebugArrow( colorBlue, COM, (COM + 30 * grabber->m_CollNorms[i]), 4.0f, 1);
			}

			// Clear m_CollNorms so it can be filled next time there's a collision
			grabber->m_CollNorms.Clear();
		
			newDir.Normalize();

			float newl1 = l1 * (dir1 * newDir);

			// avoid jittering due to floating point error
			if( newl1 > 0.1f )
			{
				l1 = newl1; // project the magnitude in the new direction
				dir1 = newDir;
			}
			else
			{
				dir1 = vec3_zero;
				l1 = 0.0f;
			}

			if( cv_drag_debug.GetBool() )
				gameRenderWorld->DebugArrow( colorRed, COM, (COM + l1 * dir1), 4.0f, 1);
		}
		else 
		{
			prevVel = m_physics->GetLinearVelocity( m_id );
			if( cv_drag_debug.GetBool() )
				gameRenderWorld->DebugArrow( colorGreen, COM, (COM + l1 * dir1), 4.0f, 1);
		}

		// "Realistic" finite acceleration
		float Accel = ( 1.0f - m_damping ) * l1 / (dT * dT);

		if( m_bLimitForce )
		{
			// max force our arms can exert
			float MaxArmAccel = grabber->m_MaxForce / m_physics->GetMass();
			// if player moves object down, gravity will help
			if( dir1 * m_physics->GetGravityNormal() > 0 )
			{
				idVec3 gravNormal = m_physics->GetGravity();
				float gravMag = gravNormal.Normalize();
				float MaxAccelDown = MaxArmAccel + gravMag;

				// break up desired motion into with gravity and the rest
				float DownAccel = Accel * (dir1 * gravNormal);
				idVec3 vDownAccel = DownAccel * gravNormal;
				idVec3 vOthAccel = Accel * dir1 - vDownAccel;
				float OthAccel = vOthAccel.NormalizeFast();

				OthAccel = idMath::ClampFloat(0.0f, MaxArmAccel, OthAccel );
				DownAccel = idMath::ClampFloat(0.0f, MaxAccelDown, DownAccel );
				// recalculate the vectors now that magnitude is clamped
				vDownAccel = DownAccel * gravNormal;
				vOthAccel = OthAccel * vOthAccel;
				velocity = prevVel * m_damping + (vDownAccel + vOthAccel) * dT;
			}
			else
			{
				// we're nice and don't make the player's arms fight gravity
				Accel = idMath::ClampFloat(0.0f, MaxArmAccel, Accel );
				velocity = prevVel * m_damping + dir1 * Accel * dT;
			}
		}
		else
		{
			// unlimited force
			velocity = prevVel * m_damping + dir1 * Accel * dT;
		}

		// ==================================================================
	}


	if( m_RefEnt.GetEntity() )
	{
		// reference frame velocity
		velocity += m_RefEnt.GetEntity()->GetPhysics()->GetLinearVelocity();

		// TODO: Add velocity due to spin angular momentum of reference entity
		// e.g., player standing on spinning thing
	}
	m_physics->SetLinearVelocity( velocity, m_id );

// ======================== ANGULAR =========================
	idVec3 Alph, DeltAngVec, RotDir, PrevOmega, Omega; // angular acceleration
	float AlphMag, MaxAlph, AlphMod, IAxis, DeltAngLen;
	idMat3 DesiredRot;

	// TODO: Make the following into cvars / spawnargs:
	float ang_damping = 0.0f;
	float max_torque = 100000 * 40;

	// Don't rotate AFs at all for now
	if( m_physics->GetSelf()->IsType( idAFEntity_Base::Type ) )
		return;

	// Find the rotation matrix needed to get from current rotation to desired rotation:
	DesiredRot =  m_dragAxis.Transpose() * m_physics->GetAxis( m_id );

	// DeltAngVec = DeltAng.ToAngularVelocity();
	DeltAngVec = DesiredRot.ToAngularVelocity();
	RotDir = DeltAngVec;
	DeltAngLen = RotDir.Normalize();
	DeltAngLen = DEG2RAD( idMath::AngleNormalize360( RAD2DEG(DeltAngLen) ) );
	DeltAngVec = DeltAngLen * RotDir;

	// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: Desired angular velocity is %s\r", DeltAngVec.ToString() );
	// test for FP error
	if( (DeltAngLen / dT) <= 0.00001 )
	{
		m_physics->SetAngularVelocity( vec3_zero, m_id );
		m_dragAxis = m_physics->GetAxis( m_id );
		return;
	}

	if (cv_drag_new.GetBool()) {
		float halfingTime = cv_drag_rigid_angle_halfing_time.GetFloat();
		float accelRadius = cv_drag_rigid_acceleration_angle.GetFloat();
		float period = halfingTime - idMath::Fmin(accelRadius / (1e-3f + DeltAngVec.Length()), 1.0f) * idMath::Fmax(halfingTime - dT, 0.0f);
		// if accelRadius == 0, then item should travel to target along straight line,
		// with distance decreasing as D(t) = exp(t / T), where T = halfingTime
		idVec3 angVelocity = DeltAngVec / period;

		m_physics->SetAngularVelocity( angVelocity, m_id );
	}
	else {

		// Finite angular acceleration:
		Alph = DeltAngVec * (1.0f - ang_damping) / (dT * dT);
		AlphMag = Alph.Length();

		// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: Desird angular accel this frame is %s\r", Alph.ToString() );

		if( m_bLimitForce )
		{
			// Find the scalar moment of inertia about this axis:
			IAxis = (m_inertiaTensor * RotDir) * RotDir;

			// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: I about axis is %f\r", IAxis );

			// Calculate max alpha about this axis from max torque
			MaxAlph = max_torque / IAxis;
			AlphMod = idMath::ClampFloat(0.0f, MaxAlph, AlphMag );

			// Finally, adjust our angular acceleration vector
			Alph *= (AlphMod/AlphMag);
			// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: Modified alpha is %s\r", Alph.ToString() );
		}

		if( grabber->m_bIsColliding )
			PrevOmega = vec3_zero;
		else
			PrevOmega = m_physics->GetAngularVelocity( m_id );

		Omega = PrevOmega * ang_damping + Alph * dT;

		// TODO: Toggle visual debugging with cvar
		// gameRenderWorld->DebugLine( colorGreen, COM, (COM + 30 * RotDir), 1);

		// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: Setting angular velocity to %s\r", Omega.ToString() );
		m_physics->SetAngularVelocity( Omega, m_id );
	}
}

/*
================
CForce_Grab::RemovePhysics
================
*/
void CForce_Grab::RemovePhysics( const idPhysics *phys ) {
	if ( m_physics == phys ) {
		m_physics = NULL;
	}
}

/*
================
CForce_Grab::GetCenterOfMass
================
*/
idVec3 CForce_Grab::GetCenterOfMass( void ) const 
{
	return ( m_physics->GetOrigin( m_id ) + m_centerOfMass * m_physics->GetAxis( m_id ) );
}


/*
================
CForce_Grab::Rotate
================
*/
void CForce_Grab::Rotate( const idVec3 &vec, float angle ) 
{
	idRotation r;

	r.Set( vec3_origin, vec, angle );
	r.RotatePoint( m_p );
}

void CForce_Grab::ApplyDamping( bool bVal )
{
	m_bApplyDamping = bVal;
}

void CForce_Grab::LimitForce( bool bVal )
{
	m_bLimitForce = bVal;
}

void CForce_Grab::SetRefEnt( idEntity *InputEnt )
{
	m_RefEnt = InputEnt;
}
