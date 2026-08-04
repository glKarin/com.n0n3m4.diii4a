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
#include "Grabber.h"
#include "DarkModGlobals.h"
#include "MeleeWeapon.h"

CLASS_DECLARATION( idMoveable, CMeleeWeapon )
END_CLASS

const int CONTENTS_MELEE_WORLDCOLLIDE = MASK_SHOT_RENDERMODEL | CONTENTS_MELEEWEAP | CONTENTS_CORPSE;
const int CONTENTS_MELEE_ACTCOLLIDE = CONTENTS_MELEEWEAP | CONTENTS_BODY; // parries/held items, AI

CMeleeWeapon::CMeleeWeapon( void ) 
{
	m_Owner = NULL;
	m_WeapClip = NULL;
	m_ClipOffset = vec3_zero;
	m_ClipRotation = mat3_identity;
	m_bClipYawOnly = false;
	m_ClipPitchAngle = 0.0f;
	m_bAttacking = false;
	m_bParrying = false;

	m_bWorldCollide = false;

	m_MeleeType = MELEETYPE_OVER;
	m_StopMass = 0.0f;
	m_ParticlesMade = 0;

	m_bModAICMs = false;
	m_AIWithModCMs.Clear();

	m_OldOrigin = vec3_zero;
	m_OldAxis = mat3_identity;
	m_bParryStopOnSuccess = false;

	m_bFailsafeTrace		 = false;
	m_vFailsafeTraceStart	= vec3_zero;
	m_vFailsafeTraceEnd		= vec3_zero;
}

CMeleeWeapon::~CMeleeWeapon( void )
{
	// avoid leaving behind a clipmodel with stale entity pointers
	DeactivateAttack();
	DeactivateParry();
	assert(!m_WeapClip);
}

void CMeleeWeapon::Save( idSaveGame *savefile ) const 
{
	m_Owner.Save( savefile );

	savefile->WriteBool( m_bAttacking );
	savefile->WriteBool( m_bParrying );
	savefile->WriteString( m_ActionName );
	savefile->WriteBool( m_bWorldCollide );
	savefile->WriteVec3( m_ClipOffset );
	savefile->WriteMat3( m_ClipRotation );
	savefile->WriteBool( m_bClipYawOnly );
	savefile->WriteFloat( m_ClipPitchAngle );
	savefile->WriteInt( m_MeleeType );
	savefile->WriteFloat( m_StopMass );
	savefile->WriteInt( m_ParticlesMade );

	savefile->WriteBool( m_bModAICMs );
	savefile->WriteInt( m_AIWithModCMs.Num() );
	for( int i=0; i < m_AIWithModCMs.Num(); i++ )
	{
		m_AIWithModCMs[i].Save( savefile );
	}

	savefile->WriteVec3( m_OldOrigin );
	savefile->WriteMat3( m_OldAxis );
	savefile->WriteBool( m_bParryStopOnSuccess );
	savefile->WriteBool( m_bFailsafeTrace );
	savefile->WriteVec3( m_vFailsafeTraceStart );
	savefile->WriteVec3( m_vFailsafeTraceEnd );

	savefile->WriteBool( m_WeapClip != NULL );
}

void CMeleeWeapon::Restore( idRestoreGame *savefile ) 
{
	m_Owner.Restore( savefile );

	savefile->ReadBool( m_bAttacking );
	savefile->ReadBool( m_bParrying );
	savefile->ReadString( m_ActionName );
	savefile->ReadBool( m_bWorldCollide );
	savefile->ReadVec3( m_ClipOffset );
	savefile->ReadMat3( m_ClipRotation );
	savefile->ReadBool( m_bClipYawOnly );
	savefile->ReadFloat( m_ClipPitchAngle );
	int mType;
	savefile->ReadInt( mType );
	m_MeleeType = (EMeleeType) mType;
	savefile->ReadFloat( m_StopMass );
	savefile->ReadInt( m_ParticlesMade );

	savefile->ReadBool( m_bModAICMs );
	m_AIWithModCMs.Clear();
	int num;
	savefile->ReadInt( num );
	m_AIWithModCMs.SetNum( num );
	for( int i=0; i < num; i++ )
	{
		m_AIWithModCMs[i].Restore( savefile );
	}

	savefile->ReadVec3( m_OldOrigin );
	savefile->ReadMat3( m_OldAxis );
	savefile->ReadBool( m_bParryStopOnSuccess );
	savefile->ReadBool( m_bFailsafeTrace );
	savefile->ReadVec3( m_vFailsafeTraceStart );
	savefile->ReadVec3( m_vFailsafeTraceEnd );

	// Regenerate the clipmodel
	bool modCM;
	savefile->ReadBool( modCM );
	if( modCM )
		SetupClipModel();
	else
		m_WeapClip = NULL;
}

void CMeleeWeapon::ActivateAttack( idActor *ActOwner, const char *AttName )
{
	const idKeyValue *key;
	idClipModel *pClip;

	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Activate attack called.  Weapon %s, owner %s, attack name %s.\r",
											name.c_str(), ActOwner->name.c_str(), AttName );

	if( (key = spawnArgs.FindKey(va("att_type_%s", AttName))) == NULL )
	{
		DM_LOG(LC_WEAPON, LT_WARNING)LOGSTRING("Did not find attack %s on melee weapon %s\r", AttName, name.c_str());
		gameLocal.Warning("Invalid attack name %s on weapon entity %s", AttName, name.c_str() );
		return;
	}

	m_MeleeType = (EMeleeType) atoi(key->GetValue().c_str());
	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING("Attack type is %d\r", m_MeleeType);
	m_ActionName = AttName;
	m_bAttacking = true;
	// TODO: We shouldn't set the owner every time, should only have to set once
	m_Owner = ActOwner;
	m_bWorldCollide = spawnArgs.GetBool( va("att_world_collide_%s", AttName) );
	m_bFailsafeTrace = spawnArgs.GetBool( va("att_failsafe_trace_%s", AttName) );
	m_vFailsafeTraceStart = spawnArgs.GetVector( va("att_failsafe_trace_start_%s", AttName) );
	m_vFailsafeTraceEnd = spawnArgs.GetVector( va("att_failsafe_trace_end_%s", AttName) );
	m_StopMass = spawnArgs.GetFloat("stop_mass");
	m_ParticlesMade = 0;
	m_bModAICMs = spawnArgs.GetBool("use_larger_ai_head_CMs");

	pClip = GetPhysics()->GetClipModel();
	if( spawnArgs.GetBool(va("att_mod_cm_%s", AttName) ) )
	{
		SetupClipModel();
		pClip = m_WeapClip;
	}

	m_OldOrigin = GetPhysics()->GetOrigin() + m_ClipOffset;
	m_OldAxis = GetPhysics()->GetAxis() * m_ClipRotation;

	
	// before initial collision test, see if we should mod head CMs on any AI in range
	if( m_bModAICMs )
		CheckAICMSwaps();

	// Initial collision test (if we start out already colliding with something)


	int TestContents;
	if( m_bWorldCollide )
		TestContents = CONTENTS_MELEE_WORLDCOLLIDE;
	else
		TestContents = CONTENTS_MELEE_ACTCOLLIDE;

	// Hack to ignore the owner during the trace
	// stgatilov: ignore combat model too if it exists (do not hit self: #2339)
	int contentsOwner = m_Owner.GetEntity()->GetPhysics()->GetContents();
	m_Owner.GetEntity()->GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
	bool combatModelContents = ActOwner->SetCombatContents(false);

	trace_t tr;
	gameLocal.clip.Translation(tr, m_OldOrigin, m_OldOrigin, pClip, m_OldAxis, TestContents, this);

	ActOwner->SetCombatContents(combatModelContents);
	m_Owner.GetEntity()->GetPhysics()->SetContents( contentsOwner );

	// If we started out in something we hit, we're already done
	if ( tr.fraction < 1.0f )
	{
		idEntity *other = gameLocal.entities[tr.c.entityNum];
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Attack clipmodel started out inside entity %s.\r", other->name.c_str());

		// additional failsafe trace:
		if( m_bFailsafeTrace )
		{
			trace_t tr2;
			idVec3 tr2Start, tr2End;
			tr2Start = (m_vFailsafeTraceStart * m_OldAxis) + m_OldOrigin;
			tr2End = (m_vFailsafeTraceEnd * m_OldAxis) + m_OldOrigin;

			m_Owner.GetEntity()->GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
			combatModelContents = ActOwner->SetCombatContents(false);
			gameLocal.clip.TracePoint( tr2, tr2Start, tr2End, TestContents, this );
			ActOwner->SetCombatContents(combatModelContents);
			m_Owner.GetEntity()->GetPhysics()->SetContents( contentsOwner );

			if( tr2.fraction < 1.0f )
			{
				// failsafe trace connected, handle valid collision and return
				idVec3 velDir = tr2End - tr2Start;
				velDir.Normalize();

				DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Failsafe trace successful, treating as valid collision\r");
				HandleValidCollision( tr2, tr2Start, velDir);
				return;
			}
		}

		// otherwise, we still don't have a valid collision, just an instanced clipmodel already inside something
		// fill in best guesses for collision data

		// grayman #3071 - replace line deleted as part of the solution to #1151
		tr.c.material = NULL;
		
		// the point is also inaccurate sometimes, set to origin of the weapon object
		// ishtvan 1/2010: This should be more accurate, to use center of clipmodel
		// tr.c.point = m_OldOrigin;
		tr.c.point = pClip->GetBounds().GetCenter();
		tr.c.point *= m_OldAxis;
		tr.c.point += m_OldOrigin;

		if( cv_melee_debug.GetBool() )
			gameRenderWorld->DebugArrow( colorMagenta, m_OldOrigin, tr.c.point, 3, 1000 );

		// set the normal to the owner's view forward
		idMat3 viewAxis;
		idVec3 dummy;
		idActor *owner = m_Owner.GetEntity();
		owner->GetViewPos( dummy, viewAxis );
		tr.c.normal = -viewAxis[0];

		// hit a parry (make sure we don't hit our own other melee weapons)
		if( other->IsType(CMeleeWeapon::Type)
			&& static_cast<CMeleeWeapon *>(other)->GetOwner()
			&& static_cast<CMeleeWeapon *>(other)->GetOwner() != owner )
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING
				("MeleeWeapon: Started out hitting a melee parry put up by %s\r", 
				  static_cast<CMeleeWeapon *>(other)->GetOwner()->name.c_str() );
			// Test our attack against their parry
			TestParry( static_cast<CMeleeWeapon *>(other), idVec3(1,0,0), &tr );
			return;
		}

		// need to get attachments' owners to check enemy status (duplicated code from checkattack :( )
		idActor *AttachOwner(NULL);
		idEntity *OthBindMaster(NULL);
		if( other->IsType(idAFAttachment::Type) )
		{
			idEntity *othBody = static_cast<idAFAttachment *>(other)->GetBody();
			if( othBody->IsType(idActor::Type) )
				AttachOwner = static_cast<idActor *>(othBody);
		}
		// Also check for any object bound to an actor (helmets, etc)
		else if( (OthBindMaster = other->GetBindMaster()) != NULL
					&& OthBindMaster->IsType(idActor::Type) )
			AttachOwner = static_cast<idActor *>(OthBindMaster);

		idActor *otherActor = NULL;
		if( other->IsType(idActor::Type) )
			otherActor = static_cast<idActor *>(other);
		else if( AttachOwner )
			otherActor = AttachOwner;

		// don't hit friendly actors or ourselves, hit everything else
		if( !( owner->IsType(idAI::Type) && otherActor
				&& (!(static_cast<idAI *>(owner)->IsEnemy(otherActor)) || otherActor == owner) ) )
		{
			MeleeCollision( gameLocal.entities[tr.c.entityNum], idVec3(1,0,0), &tr, -1 );

			CMeleeStatus *pStatus = &owner->m_MeleeStatus;
			pStatus->m_ActionResult = MELEERESULT_AT_HIT;
			pStatus->m_ActionPhase = MELEEPHASE_RECOVERING;

			DeactivateAttack();
		}
	}
}

void CMeleeWeapon::DeactivateAttack( void )
{
	if( m_bAttacking )
	{
		m_bAttacking = false;
		ClearClipModel();

		// Undo any clipmodel modifications we made to AI during this test
		for( int i=0; i < m_AIWithModCMs.Num(); i++ )
		{
			if( m_AIWithModCMs[i].IsValid() )
			{
				m_AIWithModCMs[i].GetEntity()->SwapHeadAFCM(false);
			}
		}
		m_AIWithModCMs.Clear();
	}
}


void CMeleeWeapon::ActivateParry( idActor *ActOwner, const char *ParryName )
{
	const idKeyValue *key;
	
	if( ActOwner )
	{
		DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Activate parry called.  Weapon %s, owner %s, parry name %s.\r",
												name.c_str(), ActOwner->name.c_str(), ParryName );
	}
	else
	{	
		DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Activate parry called.  Weapon %s, owner is NULL, parry name %s.\r",
												name.c_str(), ParryName );
	}

	// Test to see if parry exists in our args
	if( (key = spawnArgs.FindKey(va("par_type_%s", ParryName))) != NULL )
	{
		m_MeleeType = (EMeleeType) atoi(key->GetValue().c_str());
		DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING("Parry type is %d\r", m_MeleeType);

		// TODO: We shouldn't set the owner every time, should only have to set once
		m_Owner = ActOwner;
		m_ActionName = ParryName;
		m_bParrying = true;

		// set up the clipmodel
		idClipModel *pClip;
		if( spawnArgs.GetBool(va("par_mod_cm_%s", ParryName)) )
		{
			SetupClipModel();
			pClip = m_WeapClip;
		}
		else
			pClip = GetPhysics()->GetClipModel();

		pClip->SetContents( pClip->GetContents() | CONTENTS_MELEEWEAP );
	}
	else
	{
		gameLocal.Warning("Invalid parry name %s on weapon entity %s", ParryName, name.c_str() );
	}

}

void CMeleeWeapon::DeactivateParry( void )
{
	if( m_bParrying )
	{
		m_bParrying = false;
		ClearClipModel();
	}
}

void CMeleeWeapon::ClearClipModel( void )
{
	if( m_WeapClip )
	{
		m_WeapClip->Unlink();
		delete m_WeapClip;

		m_WeapClip = NULL;
	}
	else
	{
		idClipModel *pClip = GetPhysics()->GetClipModel();
		pClip->SetContents( pClip->GetContents() & (~CONTENTS_MELEEWEAP) );
	}

	m_ClipOffset = vec3_zero;
	m_ClipRotation = mat3_identity;
	m_bClipYawOnly = false;
	m_ClipPitchAngle = 0.0f;
}

void CMeleeWeapon::Think( void )
{
	idMoveable::Think();

	// Move the custom clipmodel around to match the weapon
	if( m_WeapClip )
	{
		idMat3 CMaxis = GetPhysics()->GetAxis();

		// option to maintain pitch relative to world
		// (only implemented for parries for now)
		if( m_bClipYawOnly )
		{
			idAngles tempAng = CMaxis.ToAngles();
			// idAngles pitchAng;
			// pitchAng.Zero();
			// pitchAng.pitch = m_ClipPitchAngle;
			tempAng.pitch = 0;
			tempAng.roll = 0;
			CMaxis = tempAng.ToMat3();
			// CMaxis = pitchAng.ToMat3() * CMaxis;
			// CMaxis = mat3_identity;
		}

		// Is id = 0 correct here, or will that cause problems with
		// the other clipmodel, which is also zero?
		
		if( m_bParrying )
		{
			idClipModel *pClip;
			if( m_WeapClip )
			{
				// m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin() + m_ClipOffset, CMaxis * m_ClipRotation );
				m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin() + m_ClipOffset, m_ClipRotation * CMaxis  );
				pClip = m_WeapClip;
			}
			else
				pClip = GetPhysics()->GetClipModel();
			
			// Debug display of the parry clipmodel
			if( m_bParrying && cv_melee_debug.GetBool())
			{
				collisionModelManager->DrawModel
					(
						pClip->Handle(), GetPhysics()->GetOrigin() + m_ClipOffset, m_ClipRotation * CMaxis,
						gameLocal.GetLocalPlayer()->GetEyePosition(), idMath::INFINITY 
					);
			}
		}
	}

	// Run the checks for an attack, handle what happens when something is hit
	// TODO: Time interleave this test?
	if( m_bAttacking )
	{
		CheckAttack( m_OldOrigin, m_OldAxis );

		m_OldOrigin = GetPhysics()->GetOrigin() + m_ClipOffset;
		m_OldAxis = GetPhysics()->GetAxis() * m_ClipRotation;
	}
}

void CMeleeWeapon::TestParry( CMeleeWeapon *other, idVec3 dir, trace_t *trace )
{
	CMeleeStatus *pStatus = &m_Owner.GetEntity()->m_MeleeStatus;
	CMeleeStatus *pOthStatus = &other->GetOwner()->m_MeleeStatus;

	EMeleeType otherType = other->GetMeleeType();
	if( !(m_MeleeType == MELEETYPE_UNBLOCKABLE)
			&& (otherType == MELEETYPE_BLOCKALL || otherType == m_MeleeType ) )
	{
		// parry was succesful
		// TODO: Play bounce animation or reverse attack animation?

		// message the attacking AI
		pStatus->m_ActionResult = MELEERESULT_AT_PARRIED;
		pStatus->m_ActionPhase = MELEEPHASE_RECOVERING;
		DeactivateAttack();

		// Hack: Play metal sound for now
		const idSoundShader *snd = declManager->FindSound( "sword_hit_metal" );
		StartSoundShader( snd, SND_CHANNEL_BODY2, 0, true, NULL );
		// TODO: Propagate sound to AI
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("Parry was successful\r");

		// update the AI that parried
		// TODO: What happens if they decide to keep parrying, should result
		// change back to IN_PROGRESS?
		pOthStatus->m_ActionResult = MELEERESULT_PAR_BLOCKED;

		if (other->m_bParryStopOnSuccess)
			other->DeactivateParry();

		// Parrying an attack can cause flat-footedness in AI
		if( other->GetOwner()->IsType(idAI::Type) )
		{
			idAI *otherAI = static_cast<idAI *>(other->GetOwner());
			
			if( otherAI->m_bCanBeFlatFooted )
			{
				int num = otherAI->m_FlatFootParryNum;
				int max = otherAI->m_FlatFootParryMax;

				// Only the first few parries induce flatness, then a timer kicks in
				bool bCanAdd = num < max;
				bool bTimerDone = (gameLocal.time - otherAI->m_FlatFootParryTimer) > otherAI->m_FlatFootParryTime;

				if( bCanAdd )
				{
					otherAI->m_bFlatFooted = true;
					otherAI->m_FlatFootedTimer = gameLocal.time;

					otherAI->m_FlatFootParryNum++;
				}
				else if( bTimerDone )
				{
					otherAI->m_bFlatFooted = true;
					otherAI->m_FlatFootedTimer = gameLocal.time;

					// reset to flat-footed parry #1 again
					otherAI->m_FlatFootParryNum = 1;
				}

				// Each successful parry after the max number resets the timer
				// This is a gameplay tweak so the player can't just spam attack
				// They must wait until the AI hasn't parried in a while and 'surprise' them again
				if( otherAI->m_FlatFootParryNum == max )
				{
					otherAI->m_FlatFootParryTimer = gameLocal.time;
				}
			}
		}
	}
	else
	{
		// Parry failed

		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("Parry failed, attack type was %d, parry type was %d\r", m_MeleeType, other->GetMeleeType() );

		pOthStatus->m_ActionResult = MELEERESULT_PAR_FAILED;
		// Try disabling this parry now that it has failed
		other->DeactivateParry();
		// May run into problems later when multiple opponents are attacking,
		// if one grazes the parry (but no the player), it would deactivate the parry
		// rendering it unable to block a matching attack from another AI
		// this is probably not that important, 
		// and could be seen as the other AI beating the blade away

		// TODO: Make the player weapon auto-stop the parry in this case
	}
}

void CMeleeWeapon::CheckAttack( idVec3 OldOrigin, idMat3 OldAxis )
{
	trace_t tr;
	idMat3 axis;
	idClipModel *pClip;
	int ClipMask;

	if( m_WeapClip )
		pClip = m_WeapClip;
	else
		pClip = GetPhysics()->GetClipModel();

	idVec3 NewOrigin = GetPhysics()->GetOrigin() + m_ClipOffset;
	idMat3 NewAxis = GetPhysics()->GetAxis() * m_ClipRotation;
	TransposeMultiply( OldAxis, NewAxis, axis );

	idRotation rotation = axis.ToRotation();
	rotation.SetOrigin( OldOrigin );

	// before collision test, check for AI in range whose head CMs we should modify
	// for best accuracy, we should run this every frame here in CheckAttack
	// this accounts for enemy and AI closing at high velocities
	// If this causes slowdown, we can comment it out and lose the accuracy
	if( m_bModAICMs )
		CheckAICMSwaps();

	if( m_bWorldCollide )
		ClipMask = CONTENTS_MELEE_WORLDCOLLIDE;
	else
		ClipMask = CONTENTS_MELEE_ACTCOLLIDE; // parries and AI
	// TODO: Is CONTENTS_BODY going to hit the outer collision box and cause problems?

	// Hack: We really want to ignore more than one entity, but we can't do that,
	// so temporarily set our CONTENTS so that we don't hit ourself
	int contentsEnt = GetPhysics()->GetContents();
	GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
	// Also ignore the owner for this trace
	int contentsOwner = m_Owner.GetEntity()->GetPhysics()->GetContents();
	m_Owner.GetEntity()->GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );

// Uncomment for debugging
/*
	DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING
		(
			"Check Attack called, old origin %s, new origin %s, old axis %s, new axis %s\r", 
			OldOrigin.ToString(), NewOrigin.ToString(), OldAxis.ToString(), NewAxis.ToString()
		);
*/

	if( cv_melee_debug.GetBool() )
		collisionModelManager->DrawModel( pClip->Handle(), NewOrigin, NewAxis, gameLocal.GetLocalPlayer()->GetEyePosition(), idMath::INFINITY );

	gameLocal.clip.Motion
	(
		tr, OldOrigin, NewOrigin, 
		rotation, pClip, OldAxis, 
		ClipMask, m_Owner.GetEntity()
	);
	
	GetPhysics()->SetContents( contentsEnt );
	m_Owner.GetEntity()->GetPhysics()->SetContents( contentsOwner );
	
	// ishtvan: new approach calling separate function HandleValidCollision:
	if( tr.fraction < 1.0f )
	{
		// hit something
		
		// Calculate the instantaneous velocity _direction_ of the point that hit
		// For now, we just need direction of velocity, not magnitude, don't need delta_t
		idVec3 vLinearTrans = (NewOrigin - OldOrigin);

		// Calc. translation due to angular velocity
		// velocity of point = r x w
		// Keep in mind that rotation was defined as a rotation about origin,
		// not about the center of mass
		idVec3 r = tr.c.point - OldOrigin;
		idVec3 vSpinTrans = r.Cross((rotation.ToAngularVelocity()));

		idVec3 PointVelDir = vLinearTrans + vSpinTrans;
		// TODO: Divide by delta_t if we want to store velocity magnitude later
		PointVelDir.Normalize();

		HandleValidCollision( tr, OldOrigin, PointVelDir );
	}
}

void CMeleeWeapon::HandleValidCollision(trace_t &tr, idVec3 trOrigin, idVec3 PointVelDir)
{
	int location;

	idEntity *other = gameLocal.entities[ tr.c.entityNum ];
	DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit entity %s\r", other->name.c_str());
	// Show the initial trace collision point
	if( cv_melee_debug.GetBool() )
		gameRenderWorld->DebugArrow( colorBlue, trOrigin, tr.c.point, 3, 1000 );

	location = JOINT_HANDLE_TO_CLIPMODEL_ID( tr.c.id );

	// Secondary trace for when we hit the AF structure of an AI and want
	// to see where we would hit on the actual model
	if(	(tr.c.contents & CONTENTS_CORPSE)
		&& other->IsType(idAFEntity_Base::Type) )
	{
		idAFEntity_Base *otherAF = static_cast<idAFEntity_Base *>(other);
		trace_t tr2;

		// NOTE: Just extrapolating along the velocity can fail when the AF
		// extends outside of the rendermodel
		// Just using the AF face normal can fail when hit at a corner
		// So take an equal average of both
		idVec3 trDir = ( PointVelDir - tr.c.normal ) / 2.0f;
		trDir.Normalize();

		idVec3 start = tr.c.point - 8.0f * trDir;
		int contentsEnt = GetPhysics()->GetContents();
		GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
		
		gameLocal.clip.TracePoint
			( 
				tr2, start, tr.c.point + 8.0f * trDir, 
				CONTENTS_RENDERMODEL, m_Owner.GetEntity() 
			);
		GetPhysics()->SetContents( contentsEnt );

		// Ishtvan: Ignore secondary trace if we hit a swapped-in head model on the first pass
		// we want to register a hit on that swapped in head body for gameplay reasons
		bool bHitSwappedHead = false;
		if( otherAF->IsType(idAI::Type) )
		{
			idAI *otherAI = static_cast<idAI *>(otherAF);
			if( otherAI->m_bHeadCMSwapped && (tr.c.id == otherAI->m_HeadBodyID) )
				bHitSwappedHead = true;
		}

		if( tr2.fraction < 1.0f && !bHitSwappedHead )
		{
			
			tr = tr2;
			other = gameLocal.entities[ tr.c.entityNum ];
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: CONTENTS_CORPSE secondary trace hit entity %s\r", other->name.c_str());

			// Draw the new collision point
			if( cv_melee_debug.GetBool() )
			{
				gameRenderWorld->DebugArrow( colorRed, start, tr2.c.point, 3, 1000 );
			}

			other = gameLocal.entities[tr2.c.entityNum];
			// update location
			location = JOINT_HANDLE_TO_CLIPMODEL_ID( tr.c.id );
		}
		else
		{
			// failed to connect with the rendermodel.  
			// Last ditch effort to find the correct AF body
			location = otherAF->JointForBody(tr.c.id);

			// If we failed to find anything, draw the attempted trace in green
			if( cv_melee_debug.GetBool() )
				gameRenderWorld->DebugArrow( colorGreen, start, tr.c.point + 8.0f * PointVelDir, 3, 1000 );
		}

		// Uncomment for translational and angular velocity debugging
		if( cv_melee_debug.GetBool() )
		{
		// no longer works with new function format
		// if we really want to separate angular and linear,
		// we could do this same operation in outer function
		/*
			idVec3 vLinDir = vLinearTrans;
			idVec3 vSpinDir = vSpinTrans;
			vLinDir.Normalize();
			vSpinDir.Normalize();

			gameRenderWorld->DebugArrow
				(
					colorCyan, (tr.c.point - 8.0f * vLinDir), 
					(tr.c.point + 8.0f * vLinDir), 3, 1000
				);
			// Uncomment for translational and angular velocity debugging
			gameRenderWorld->DebugArrow
				(
					colorPurple, (tr.c.point - 8.0f * vSpinDir), 
					(tr.c.point + 8.0f * vSpinDir), 3, 1000
				);
		*/
		// instead, show combined velocity
			gameRenderWorld->DebugArrow
			(
				colorCyan, (tr.c.point - 8.0f * PointVelDir), 
				(tr.c.point + 8.0f * PointVelDir), 3, 1000
			);
		}
	}

	// Direction of the velocity of point that hit (renamed it for brevity)
	idVec3 dir = PointVelDir;

	idActor *AttachOwner(NULL);
	idEntity *OthBindMaster(NULL);
	if( other->IsType(idAFAttachment::Type) )
	{
		idEntity *othBody = static_cast<idAFAttachment *>(other)->GetBody();
		if( othBody->IsType(idActor::Type) )
			AttachOwner = static_cast<idActor *>(othBody);
	}
	// Also check for any object bound to an actor (helmets, etc)
	else if( (OthBindMaster = other->GetBindMaster()) != NULL
				&& OthBindMaster->IsType(idActor::Type) )
		AttachOwner = static_cast<idActor *>(OthBindMaster);

	CMeleeStatus *pStatus = &m_Owner.GetEntity()->m_MeleeStatus;

	// Check if we hit a melee parry or held object 
	// (for some reason tr.c.contents erroneously returns CONTENTS_MELEEWEAP for everything except the world)
	// if( (tr.c.contents & CONTENTS_MELEEWEAP) != 0 )
	// this doesn't always work either, it misses linked clipmodels on melee weapons
	// if( other->GetPhysics() && ( other->GetPhysics()->GetContents(tr.c.id) & CONTENTS_MELEEWEAP) )
	// have to do this to catch all cases:
	if
		( 
			(other->GetPhysics() && ( other->GetPhysics()->GetContents(tr.c.id) & CONTENTS_MELEEWEAP))
			|| ( other->IsType(CMeleeWeapon::Type) && static_cast<CMeleeWeapon *>(other)->GetOwner() )
		)
	{
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit someting with CONTENTS_MELEEWEAP\r");
		// hit a parry (make sure we don't hit our own other melee weapons)
		if( other->IsType(CMeleeWeapon::Type)
			&& static_cast<CMeleeWeapon *>(other)->GetOwner()
			&& static_cast<CMeleeWeapon *>(other)->GetOwner() != m_Owner.GetEntity() )
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING
				("MeleeWeapon: Hit a melee parry put up by %s\r", 
				  static_cast<CMeleeWeapon *>(other)->GetOwner()->name.c_str() );
			// Test our attack against their parry
			TestParry( static_cast<CMeleeWeapon *>(other), dir, &tr );
		}
		// hit a held object
		else if( other == gameLocal.m_Grabber->GetSelected() )
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit an object held by the player\r");
			
			MeleeCollision( other, dir, &tr, location );

			// TODO: Message the grabber that the grabbed object has been hit
			// So that it can fly out of player's hands if desired
			
			// update AI status (consider this a miss)
			pStatus->m_ActionResult = MELEERESULT_AT_MISSED;
			pStatus->m_ActionPhase = MELEEPHASE_RECOVERING;

			// TODO: Message the attacking AI to play a bounce off animation if appropriate

			DeactivateAttack();
		}
		else
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit something with CONTENTS_MELEEWEAP that's not an active parry or a held object (this shouldn't normally happen).\r");
			MeleeCollision( other, dir, &tr, location );

			// update AI status (consider this a miss)
			pStatus->m_ActionResult = MELEERESULT_AT_MISSED;
			pStatus->m_ActionPhase = MELEEPHASE_RECOVERING;

			DeactivateAttack();
		}
	}
	// Hit an actor, or an AF attachment that is part of an actor
	else if( other->IsType(idActor::Type) || AttachOwner != NULL )
	{
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit actor or part of actor %s\r", other->name.c_str());
		
		idActor *owner = m_Owner.GetEntity();
		idActor *otherAct;

		if( AttachOwner )
			otherAct = AttachOwner;
		else
			otherAct = static_cast<idActor *>(other);

		// Don't do anything if we hit our own AF attachment
		// AI also don't do damage to friendlies/neutral
		if( AttachOwner != owner 
			&& !(owner->IsType(idAI::Type) && !(static_cast<idAI *>(owner)->IsEnemy(otherAct))) )
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit AI other than ourselves.\r");
			// TODO: Scale damage with instantaneous velocity of the blade?
			MeleeCollision( other, dir, &tr, location );

			// update actor's melee status
			pStatus->m_ActionResult = MELEERESULT_AT_HIT;
			pStatus->m_ActionPhase = MELEEPHASE_RECOVERING;

			DeactivateAttack();
		}
		else
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit yourself.  Stop hitting yourself!\r");
		}
	}
	// Hit something else in the world (only happens to the player)
	else
	{
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit a non-AI object: %s\r", other->name.c_str());
		MeleeCollision( other, dir, &tr, location );
		
		// TODO: Message the attacking actor to play a bounce off animation if appropriate

		// keep the attack going if the object hit is a moveable below a certain mass
		// TODO: Handle moveables bound to other things and use the total mass of the system?
		if( !( other->IsType(idMoveable::Type) && other->GetPhysics()->GetMass() < m_StopMass ) )
		{
			// message the AI, consider this a miss
			pStatus->m_ActionResult = MELEERESULT_AT_MISSED;
			pStatus->m_ActionPhase = MELEEPHASE_RECOVERING;

			DeactivateAttack();
		}
	}
}

void CMeleeWeapon::MeleeCollision( idEntity *other, idVec3 dir, trace_t *tr, int location )
{
	const char *DamageDefName;
	const idDict *DmgDef;
	float push(0.0f), DmgScale(1.0f);
	idVec3 impulse(vec3_zero);
	idStr hitSound, sndName, surfType;

	DamageDefName = spawnArgs.GetString( va("def_damage_%s", m_ActionName.c_str()) );
	DmgDef = gameLocal.FindEntityDefDict( DamageDefName, false );

	if( !DmgDef )
	{
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Did not find damage def %s\r", DamageDefName);
		return;
	}

	// Apply physical impulse
	push = DmgDef->GetFloat( "push" );
	impulse = -push * tr->c.normal;
	// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Applying impulse\r");
	other->ApplyImpulse( this, tr->c.id, tr->c.point, impulse );
	// uncomment for physics debugging
	// gameRenderWorld->DebugArrow( colorBlue, tr->c.point, tr->c.point + impulse, 3, 1000 );

	// get type of material hit (armor, etc)
	int type;
	if( tr->c.material != NULL )
	{
		type = tr->c.material->GetSurfaceType();
	}
	else
	{
		type = SURFTYPE_NONE;
	}

	if ( type == SURFTYPE_NONE )
	{
		// use a default surface type if we are unable to detect one
		surfType = DmgDef->GetString("default_surface_inanimate");
	}
	else
		g_Global.GetSurfName( tr->c.material, surfType );

	// scale the damage by owner's ability and surface type hit
	DmgScale *= m_Owner.GetEntity()->m_MeleeDamageMult;
	DmgScale *= DmgDef->GetFloat( va("damage_mult_%s",surfType.c_str()), "1.0" );

	// Damage
	// Check for reroute entity (can happen with attachments to AI)
	if( other->IsType(idAFEntity_Base::Type) )
	{
		idAFEntity_Base *otherAF = static_cast<idAFEntity_Base *>(other);
		int bodID = otherAF->BodyForClipModelId( tr->c.id );
		idAFBody* StruckBody = otherAF->GetAFPhysics()->GetBody( bodID );

		if( StruckBody != NULL )
		{
			idEntity* reroute = StruckBody->GetRerouteEnt();
			if (reroute != NULL) 
			{
				// joint and clipmodel id's become invalid if we reroute, but that's okay
				other = reroute;
			}
		}
	}

	idActor *otherAct = NULL;
	if( other->IsType(idActor::Type) )
	{
		otherAct = static_cast<idActor *>(other);
	}
	else if( other->IsType(idAFAttachment::Type) )
	{
		otherAct = (idActor *) static_cast<idAFAttachment *>(other)->GetBody();
	}
	// actor-specific stuff
	if( otherAct != NULL )
	{
		// update melee status
		CMeleeStatus *pOthMeleeStatus = &otherAct->m_MeleeStatus;
		pOthMeleeStatus->m_LastHitByType = m_MeleeType;

		// switch the default surface type to that for actors
		if ( type == SURFTYPE_NONE )
		{
			// use a default surface type if we are unable to detect one
			surfType = DmgDef->GetString("default_surface_actor");
		}
	}

	if ( other->fl.takedamage )
	{
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Applying damage at clipmodel id %d, joint handle %d\r", tr->c.id, location );
		// TODO: Damage scaling - on the weapon * melee proficiency on the actor
		other->Damage
		(
			this, m_Owner.GetEntity(), 
			dir, DamageDefName,
			DmgScale, location, tr 
		);
	}

	// apply a LARGE tactile alert to AI (this must be done AFTER damage, otherwise sneak attacks are broken)
	if( other->IsType(idAI::Type) )
	{
		idAI *otherAI = static_cast<idAI *>(other);
		otherAI->TactileAlert( GetOwner(), 100 );

		// being hit always causes flat-footedness
		if( otherAI->m_bCanBeFlatFooted )
		{
			otherAI->m_bFlatFooted = true;
			otherAI->m_FlatFootedTimer = gameLocal.time;
		}
	}

	// Moved impact_damage_effect to DmgDef instead of weapon ent spawnargs
	// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Applying impact damage FX\r");
	if ( DmgDef->GetBool( "impact_damage_effect" ) ) 
	{
		// ishtvan: Rewrote this to get rid of old D3 code
		// that played the sounds in AddDamageEffect for bleeders, that doesn't make sense
		if ( other->spawnArgs.GetBool( "bleed" ) ) 
		{
			// stuff that bleeds handle smoke and decal in AddDamageEffect
			other->AddDamageEffect( *tr, impulse, DmgDef->GetString( "classname" ) );
		}

		// Handle smoke (particles), and decals
		// NOTE: AddDamageEffect above will handle all wound particles and decals (smoke_wound_*, mtr_wound_*)
		// So we only have to worry armour and other non-wounding strikes here (smoke_strike_* and mtr_strike_*)

		// Uncomment for surface type debugging
		// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Surface hit was %s\r", tr->c.material->GetName() );
		// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Material type name was %s\r", surfType.c_str() );

		// start impact sound based on material type
		// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Playing hit sound\r");
		sndName = va( "snd_%s", surfType.c_str() );
		hitSound = DmgDef->GetString( sndName );

		if ( hitSound.IsEmpty() ) 
		{
			// snd_hit is the default sound
			sndName = "snd_hit";
			hitSound = DmgDef->GetString( sndName );
		}

		// project decal 
		// ishtvan: got rid of min time between decals, let it be up to anim
		// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Displaying decal\r");
		// TODO: Make this read the surface type like smoke_strike_* ?
		const char *decal;
		decal = DmgDef->GetString( "mtr_strike" );
		if ( decal && *decal ) 
		{
			gameLocal.ProjectDecal( tr->c.point, -tr->c.normal, 8.0f, true, 6.0, decal, 0.0f, other, true );
		}

		// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Launching smoke\r");
		// Strike particle FX (sparks.. blood is handled in AddDamageEffect)
		const char *smokeName = DmgDef->GetString( va("smoke_strike_%s", surfType.c_str()) );

		if ( m_ParticlesMade < cv_melee_max_particles.GetInteger() && 
				smokeName && *smokeName != '\0' )
		{
			const idDeclParticle *smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
			float chance = DmgDef->GetFloat( va("smoke_chance_%s", surfType.c_str()), "1.0" );
			if( gameLocal.random.RandomFloat() > chance )
				smoke = NULL;

			if( smoke )
			{
				gameLocal.smokeParticles->EmitSmoke
					( 
						smoke, gameLocal.time, 
						gameLocal.random.RandomFloat(), 
						tr->c.point, -tr->endAxis
					);
				m_ParticlesMade++;
			}
		}
	}

	// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Sound playback\r");
	if ( !hitSound.IsEmpty() ) 
	{
		const idSoundShader *snd = declManager->FindSound( hitSound.c_str() );
		StartSoundShader( snd, SND_CHANNEL_BODY2, 0, true, NULL );
			
		// Propagate the sound to AI, must find global sound first because it's on a different dict
		sndName.StripLeading("snd_");
		sndName = DmgDef->GetString( va("sprS_%s", sndName.c_str()) );
		if( !sndName.IsEmpty() )
		{
			// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Propagating AI sound %s\r", sndName.c_str());
			PropSoundDirect( sndName.c_str(), false, false, 0.0f, 0 ); // grayman #3355
		}
	}
	// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Done!\r");

	return;
}

void CMeleeWeapon::SetupClipModel( )
{
	// stgatilov #6225: if custom clipmodel is used now, make sure to deallocate it
	// since otherwise we'll overwrite it, leaving a dangling clipmodel in idClip
	ClearClipModel();

	float size(0.0f);
	idBounds CMBounds;
	idTraceModel trm;
	int numSides(0);
	idStr Prefix;
	const char *APrefix, *AName, *cmName;

	if( m_bAttacking )
		APrefix = "att";
	else
		APrefix = "par";

	AName = m_ActionName.c_str();

	// TODO: Allow offset and rotation of CM relative to self origin and axis

	// Try loading a clipmodel directly from a file
	spawnArgs.GetString( va("%s_clipmodel_%s", APrefix, AName), "", &cmName );
	if ( cmName[0] )
	{
		if ( !collisionModelManager->TrmFromModel( cmName, trm ) ) 
		{
			gameLocal.Error( "CMeleeWeapon '%s': cannot load collision model %s", name.c_str(), cmName );
		}

		// angua: check if the cm is valid
		if (idMath::Fabs(trm.bounds[0].x) == idMath::INFINITY)
		{
			gameLocal.Error( "CMeleeWeapon '%s': invalid collision model %s", name.c_str(), cmName );
		}
	}
	else if ( spawnArgs.GetVector( va("%s_cm_mins_%s", APrefix, AName), NULL, CMBounds[0] ) &&
				spawnArgs.GetVector( va("%s_cm_maxs_%s", APrefix, AName), NULL, CMBounds[1] ) )
	{
		if ( CMBounds[0][0] > CMBounds[1][0] || CMBounds[0][1] > CMBounds[1][1] || CMBounds[0][2] > CMBounds[1][2] )
		{
			gameLocal.Error( "CMeleeWeapon '%s': Invalid clipmodel bounds '%s'-'%s' for action %s", name.c_str(), CMBounds[0].ToString(), CMBounds[1].ToString(), AName );
		}
	} 
	else 
	{
		spawnArgs.GetFloat( va("%s_cm_size_%s", APrefix, AName), "10.0", size );
		CMBounds.Zero();
		CMBounds.ExpandSelf( size );
	}

	if ( spawnArgs.GetInt( va("%s_cm_cylinder_%s", APrefix, AName), "0", numSides ) && numSides > 0 )
		trm.SetupCylinder( CMBounds, numSides < 3 ? 3 : numSides );
	else if ( spawnArgs.GetInt( va("%s_cm_cone_%s", APrefix, AName), "0", numSides ) && numSides > 0 )
		trm.SetupCone( CMBounds, numSides < 3 ? 3 : numSides );
	else
		trm.SetupBox( CMBounds );

	assert(!m_WeapClip);
	m_WeapClip = new idClipModel( trm );

	m_ClipOffset = spawnArgs.GetVector( va("%s_cm_offset_%s", APrefix, AName) );
	idAngles tempAng = spawnArgs.GetAngles( va("%s_cm_angles_%s", APrefix, AName) );
	m_ClipRotation = tempAng.ToMat3();
	m_bClipYawOnly = spawnArgs.GetBool( va("%s_cm_yaw_only_%s", APrefix, AName) );
	m_ClipPitchAngle = spawnArgs.GetFloat( va("%s_cm_pitch_angle_%s", APrefix, AName) );

	// Override the default CONTENTS_SOLID on moveables (we don't want the player to run into their own parries!)
	m_WeapClip->SetContents( CONTENTS_MELEEWEAP );
	
	// Only parries need a linked and auto-upated clipmodel
	if( m_bParrying )
		m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin() + m_ClipOffset, GetPhysics()->GetAxis() * m_ClipRotation );
}

void CMeleeWeapon::AttachedToActor(idActor *actor)
{
	SetOwner( actor );

	// parse spawnargs to actor's melee capabilities
	CMeleeStatus *pMeleeStatus = &actor->m_MeleeStatus;
	// NOTE: Don't know what order sword/shield will be attached in,
	// so don't clear true values on parry and can parry all
	if( !pMeleeStatus->m_bCanParry )
		pMeleeStatus->m_bCanParry = spawnArgs.GetBool("can_parry");
	if( !pMeleeStatus->m_bCanParryAll )
		pMeleeStatus->m_bCanParryAll = spawnArgs.GetBool("can_parry_all");

	// parse the list of possible attacks:
	idLexer src;
	idToken	token;
	idList<EMeleeType> attacks;

	idStr StrIn = spawnArgs.GetString("attacks_allowed");

	src.LoadMemory( StrIn.c_str(), StrIn.Length(), "" );
	while( src.ReadToken( &token ) )
	{
		if( token.IsNumeric() )
			attacks.Append( (EMeleeType) token.GetIntValue() );
	}
	src.FreeSource();

	pMeleeStatus->m_attacks = attacks;

	// add weapon reach to AI's unarmed reach
	actor->melee_range = actor->melee_range_unarmed + spawnArgs.GetFloat("reach");

	if( actor->IsType(idAI::Type) )
	{
		// overwrite AI's initial attack prediction time if we have this spawnarg
		const idKeyValue *kv = spawnArgs.FindKey("predicted_attack_time");
		if( kv )
			actor->m_MeleePredictedAttTime = 0.001f * atof(kv->GetValue().c_str());
	}

}

void CMeleeWeapon::CheckAICMSwaps( )
{
	// check for AI within a bounds around weapon origin
	// if AI is conscious and not in combat alert state, try to swap to larger head CM
	idBounds b;
	b.Zero();
	b.ExpandSelf(50.0f);
	b.TranslateSelf( GetPhysics()->GetOrigin() + m_ClipOffset );

	idClip_EntityList ents;
	int numEnts = gameLocal.clip.EntitiesTouchingBounds( b, CONTENTS_BODY, ents );

	for( int i = 0; i < numEnts; i++ )
	{
		idEntity *pEnt = ents[i];
		if( !pEnt->IsType(idAI::Type) )
			continue;
		idAI *pAI = static_cast<idAI *>(pEnt);
		if( pAI->IsKnockedOut() || pAI->health < 0 || ( pAI->AI_AlertIndex >= ai::ECombat ) )
			continue;
		
		// do the swap
		idEntityPtr<idAI> AIEntPtr;
		AIEntPtr = pAI;

		int num = m_AIWithModCMs.Num();
		int index;
		index = m_AIWithModCMs.AddUnique( AIEntPtr );
		// sneaky way of checking if we're handling this AI for the first time:
		if( index == num )
			pAI->SwapHeadAFCM( true );
	}
}
