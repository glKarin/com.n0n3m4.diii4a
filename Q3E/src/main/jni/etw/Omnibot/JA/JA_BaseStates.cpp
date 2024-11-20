////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompJA.h"
#include "JA_BaseStates.h"
#include "JA_NavigationFlags.h"
#include "ScriptManager.h"
#include "gmScriptGoal.h"

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////
	ForceJump::ForceJump() 
		: StateChild("ForceJump")
		, m_IsJumping(false)
	{
	}

	void ForceJump::Enter()
	{
		m_IsJumping = false;
		m_IsOnGround = true;
		m_DistanceLeft = m_DistanceTotal = 0.f;
	}

	void ForceJump::RenderDebug()
	{
		if(IsActive())
		{
			Utils::DrawLine(GetClient()->GetEyePosition(),m_NextPt.m_Pt,COLOR::GREEN,IGame::GetDeltaTimeSecs()*2.f);
		}
	}

	obReal ForceJump::GetPriority()
	{
		if(IsActive())
			return 1.f;

		FINDSTATE(fp,FollowPath,GetParent());
		if(fp)
		{
			if(fp->IsMoving() && 
				fp->GetCurrentPath().GetCurrentPt(m_NextPt) && 
				(m_NextPt.m_NavFlags & F_JA_NAV_FORCEJUMP))
			{
				return 1.f;
			}
		}
		return 0.f;
	}

	State::StateStatus ForceJump::Update(float fDt)
	{
		Vector3f vMyPos = GetClient()->GetPosition();

		const float fDistanceToPt = Length(vMyPos.As2d(), m_NextPt.m_Pt.As2d());
		bool bInRadius = fDistanceToPt <= m_NextPt.m_Radius;
		
		FINDSTATEIF(SteeringSystem, GetRootState(), SetTarget(m_NextPt.m_Pt));

		/*Trajectory::AimTrajectory traj[2];
		int t = Trajectory::Calculate(
			vMyPos, 
			m_NextPt.m_Pt, 
			GetClient()->GetMaxSpeed(), 
			IGame::GetGravity(), 
			traj);*/

		if(!m_IsJumping)
		{
			// Wait till we get in radius to jump
			if(!bInRadius)
				return State_Busy;

			if(GetClient()->HasEntityFlag(ENT_FLAG_ONGROUND))
			{
				m_IsJumping = true;
				m_IsOnGround = true;
			}
		}

		GetClient()->PressButton(BOT_BUTTON_JUMP);

		// Detect when we leave the ground
		if(m_IsOnGround && !GetClient()->HasEntityFlag(ENT_FLAG_ONGROUND))
		{
			m_DistanceTotal = fDistanceToPt;
			m_IsOnGround = false;
		}

		m_DistanceLeft = fDistanceToPt;

		// When we touch the ground again after leaving it, we consider it done.
		if(!m_IsOnGround && GetClient()->HasEntityFlag(ENT_FLAG_ONGROUND))
			return State_Finished;

		// Finish when we exceed the targets height, because of this, it isn't usable for
		// long distance force jumps, only height based force jumps.
		if(m_IsJumping && vMyPos.z >= m_NextPt.m_Pt.z && (m_DistanceLeft < m_DistanceTotal*0.35f))
			return State_Finished;

		return State_Busy;
	}
	//////////////////////////////////////////////////////////////////////////
};
