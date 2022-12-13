#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/**********************************************************************

hhBoneController

**********************************************************************/

/*
================
hhBoneController::hhBoneController
================
*/
hhBoneController::hhBoneController() {
	m_JointHandle = INVALID_JOINT;
	m_pOwner = NULL;

	m_Factor.Set(1.0f, 1.0f, 1.0f);
	m_TurnRate.Set(0.0f, 0.0f, 0.0f);
	
	m_IdealAng.Zero();
	m_CurrentAng.Zero();
	m_MinAngles.Zero();
	m_MaxAngles.Zero();
	m_Fraction.Zero();
	m_MaxDelta.Zero();
}

/*
================
hhBoneController::Setup
================
*/
void hhBoneController::Setup( idEntity *pOwner, const char *pJointname, idAngles &MinAngles, idAngles &MaxAngles, idAngles& Rate, idAngles& Factor ) {
	jointHandle_t Joint = ( pOwner ) ? pOwner->GetAnimator()->GetJointHandle( pJointname ) : INVALID_JOINT;

	Setup( pOwner, Joint, MinAngles, MaxAngles, Rate, Factor );
}

/*
================
hhBoneController::Setup
================
*/
void hhBoneController::Setup( idEntity *pOwner, jointHandle_t Joint, idAngles &MinAngles, idAngles &MaxAngles, idAngles& Rate, idAngles& Factor ) {
	m_IdealAng.Zero();
	m_CurrentAng.Zero();
	m_Fraction.Zero();
	
	m_JointHandle = Joint;

	m_MinAngles = MinAngles;
	m_MaxAngles = MaxAngles;
	m_Factor = Factor;
	m_TurnRate = Rate;
	m_pOwner = pOwner;
}

/*
================
hhBoneController::Update
================
*/
void hhBoneController::Update( int iCurrentTime ) {
	idAngles Delta;
	int iIndex;
	idAngles Turn;
	idAngles Angle;

	if ( !m_pOwner || 
	   ( m_JointHandle == INVALID_JOINT ) ) {
		return;
	}

	Turn = m_TurnRate * MS2SEC(gameLocal.msec) * gameLocal.GetTimeScale();

	Delta = m_IdealAng - m_CurrentAng;
	for( iIndex = 0; iIndex < 3; ++iIndex ) {
		if ( Delta[ iIndex ] > Turn[iIndex] ) {
			Delta[ iIndex ] = Turn[iIndex];
		}
		if ( Delta[ iIndex ] < -Turn[iIndex] ) {
			Delta[ iIndex ] = -Turn[iIndex];
		}
	}

	m_CurrentAng += Delta;
	for(iIndex = 0; iIndex < 3; ++iIndex) {
		Angle[iIndex] = m_CurrentAng[iIndex] * m_Factor[iIndex];
	}

	m_pOwner->GetAnimator()->SetJointAxis( m_JointHandle, JOINTMOD_WORLD, Angle.ToMat3() );

	m_Fraction.pitch = 1.0f - ( (m_MaxDelta.pitch > VECTOR_EPSILON) ? idMath::Fabs((m_IdealAng.pitch - m_CurrentAng.pitch)) / m_MaxDelta.pitch : 0.0f );
	m_Fraction.yaw = 1.0f - ( (m_MaxDelta.yaw > VECTOR_EPSILON) ? idMath::Fabs((m_IdealAng.yaw - m_CurrentAng.yaw)) / m_MaxDelta.yaw : 0.0f );
	m_Fraction.roll = 1.0f - ( (m_MaxDelta.roll > VECTOR_EPSILON) ? idMath::Fabs((m_IdealAng.roll - m_CurrentAng.roll)) / m_MaxDelta.roll : 0.0f );
}

/*
================
hhBoneController::TurnTo
================
*/
bool hhBoneController::TurnTo( idAngles &Target ) {
	m_IdealAng = Target;
	bool bClampAngles = !ClampAngles();

	m_MaxDelta.pitch = idMath::Fabs(m_IdealAng.pitch - m_CurrentAng.pitch);
	m_MaxDelta.yaw = idMath::Fabs(m_IdealAng.yaw - m_CurrentAng.yaw);
	m_MaxDelta.roll = idMath::Fabs(m_IdealAng.roll - m_CurrentAng.roll);

	return bClampAngles;
}

/*
================
hhBoneController::AimAt
================
*/
bool hhBoneController::AimAt( idVec3 &Target ) {
	idVec3 dir;
	idVec3 localDir;

	if ( !m_pOwner ) {
		return false;
	}
	
	dir = Target - m_pOwner->GetOrigin();
	dir.Normalize();

	m_pOwner->GetAxis().ProjectVector( dir, localDir );

	m_IdealAng.yaw = idMath::AngleNormalize180( localDir.ToYaw() );
	m_IdealAng.pitch = -idMath::AngleNormalize180( localDir.ToPitch() );

	bool bClampAngles = !ClampAngles();

	m_MaxDelta.pitch = idMath::Fabs(m_IdealAng.pitch - m_CurrentAng.pitch);
	m_MaxDelta.yaw = idMath::Fabs(m_IdealAng.yaw - m_CurrentAng.yaw);

	return bClampAngles;
}

/*
=====================
hhBoneController::ClampAngles
=====================
*/
bool hhBoneController::ClampAngles( void ) {
	int i;
	bool clamp;

	clamp = false;
	for( i = 0; i < 3; i++ ) {
		if ( m_IdealAng[ i ] > m_MaxAngles[ i ] ) {
			m_IdealAng[ i ] = m_MaxAngles[ i ];
			clamp = true;
		}
		if ( m_IdealAng[ i ] < m_MinAngles[ i ] ) {
			m_IdealAng[ i ] = m_MinAngles[ i ];
			clamp = true;
		}
	}

	return clamp;
}

/*
=====================
hhBoneController::SetRotationFactor
=====================
*/
void hhBoneController::SetRotationFactor(idAngles& RotationFactor) {
	m_Factor = RotationFactor;
}

/*
=====================
hhBoneController::IsFinishedMoving
=====================
*/
bool hhBoneController::IsFinishedMoving(int iAxis) {
	return (m_IdealAng[iAxis] == m_CurrentAng[iAxis]);
}

/*
=====================
hhBoneController::IsFinishedMoving
=====================
*/
bool hhBoneController::IsFinishedMoving() {
	return (m_IdealAng == m_CurrentAng);
}

/*
=====================
hhBoneController::AdjustScanRateToLinearizeBonePath
=====================
*/
void hhBoneController::AdjustScanRateToLinearizeBonePath(float fLinearScanRate) {
	float fDeltaPitch = idMath::Fabs( (m_IdealAng.pitch - m_CurrentAng.pitch) * m_Factor.pitch );
	float fDeltaYaw = idMath::Fabs( (m_IdealAng.yaw - m_CurrentAng.yaw) * m_Factor.yaw );

	float fHypotenuse = idMath::Sqrt(fDeltaPitch * fDeltaPitch + fDeltaYaw * fDeltaYaw);
	if(fHypotenuse) {
		float fFrac = fLinearScanRate / fHypotenuse;

		m_TurnRate.pitch = fDeltaPitch * fFrac;
		m_TurnRate.yaw = fDeltaYaw * fFrac;
	}
}

//================
//hhBoneController::Save
//================
void hhBoneController::Save( idSaveGame *savefile ) const {
	savefile->WriteAngles( m_Fraction );
	savefile->WriteAngles( m_MaxDelta );
	savefile->WriteInt( m_JointHandle );
	savefile->WriteAngles( m_IdealAng );
	savefile->WriteAngles( m_CurrentAng );
	savefile->WriteAngles( m_MinAngles );
	savefile->WriteAngles( m_MaxAngles );
	savefile->WriteAngles( m_Factor );
	savefile->WriteAngles( m_TurnRate );
	savefile->WriteObject( m_pOwner );
}

//================
//hhBoneController::Restore
//================
void hhBoneController::Restore( idRestoreGame *savefile ) {
	savefile->ReadAngles( m_Fraction );
	savefile->ReadAngles( m_MaxDelta );
	savefile->ReadInt( reinterpret_cast<int &> ( m_JointHandle ) );
	savefile->ReadAngles( m_IdealAng );
	savefile->ReadAngles( m_CurrentAng );
	savefile->ReadAngles( m_MinAngles );
	savefile->ReadAngles( m_MaxAngles );
	savefile->ReadAngles( m_Factor );
	savefile->ReadAngles( m_TurnRate );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( m_pOwner ) );
}

