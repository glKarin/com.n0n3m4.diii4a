#include "PrecompCommon.h"
#include "BotSteeringSystem.h"
#include "Client.h"
#include "IGame.h"
#include "IGameManager.h"

namespace AiState
{
	SteeringSystem::SteeringSystem() 
		: StateChild("SteeringSystem")
		, m_TargetVector	(Vector3f::ZERO)
		, m_TargetRadius	(32.f)
		, m_MoveMode		(Run)
		, m_DistanceToTarget(0.f)
		, m_MoveVec			(Vector3f::ZERO)
		, m_NoAvoidTime		(0)
		, m_MoveType		(Normal)
		, m_bMoveEnabled	(true)
		, m_TargetVector3d	(false)
	{
	}

	SteeringSystem::~SteeringSystem()
	{
	}

	void SteeringSystem::RenderDebug()
	{
		Utils::DrawLine(GetClient()->GetEyePosition(), GetTarget(), COLOR::GREEN, IGame::GetDeltaTimeSecs()*2.f);
	}

	bool SteeringSystem::InTargetRadius() const
	{
		return m_DistanceToTarget <= m_TargetRadius;
	}

	Vector3f SteeringSystem::GetMoveVector(MoveType _movetype)
	{
		return m_bMoveEnabled ? m_MoveVec : Vector3f::ZERO;
	}

	bool SteeringSystem::SetTarget(const Vector3f &_pos, float _radius, MoveMode _movemode, bool _in3d)
	{
		m_TargetVector = _pos;	
		m_TargetRadius = _radius;
		m_TargetVector3d = _in3d;
		m_MoveMode = _movemode;
		m_MoveType = Arrive;
		return true;
	}

	void SteeringSystem::SetNoAvoidTime(int _time)
	{
		m_NoAvoidTime = IGame::GetTime() + _time; 
	}

	void SteeringSystem::UpdateSteering()
	{
		Prof(UpdateSteering);

		if(!m_bMoveEnabled)
		{
			m_MoveVec = Vector3f::ZERO;
			return;
		}

		// Calculate the vector to the current target.
		/*m_vMoveVec = (m_vTargetVector - m_Client->GetPosition());
		m_vMoveVec.Normalize();*/
		//obstacleManager.ModifyForObstacles( GetClient(), m_TargetVector );

		m_DistanceToTarget = 0.f;
		switch(m_MoveType)
		{
		case Normal:
			m_MoveVec = m_TargetVector - GetClient()->GetPosition();
			if(!m_TargetVector3d)
				m_MoveVec = m_MoveVec.Flatten();
			m_DistanceToTarget = m_MoveVec.Normalize();
			break;
		case Arrive:
			m_MoveVec = m_TargetVector - GetClient()->GetPosition();
			if(!m_TargetVector3d)
				m_MoveVec = m_MoveVec.Flatten();
			m_DistanceToTarget = m_MoveVec.Normalize();
			//m_vMoveVec *= _Arrive(m_vTargetVector, normal);
			break;
		}

		//////////////////////////////////////////////////////////////////////////
		if(m_DistanceToTarget <= m_TargetRadius)
		{
			m_MoveVec = Vector3f::ZERO;
			return;
		}

		//////////////////////////////////////////////////////////////////////////

		if(GetClient()->IsDebugEnabled(BOT_DEBUG_MOVEVEC))
		{
			Vector3f vPos = GetClient()->GetPosition();
			Utils::DrawLine(vPos, vPos+m_MoveVec * 64.f, COLOR::GREEN, 0.1f);
		}

		if(GetClient()->HasEntityFlag(ENT_FLAG_ON_ICE))
		{
			GetClient()->PressButton(BOT_BUTTON_WALK);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void SteeringSystem::Enter()
	{
		m_TargetVector = GetClient()->GetPosition();
		GetClient()->SetMovementVector(Vector3f::ZERO);
		FINDSTATEIF(FollowPath,GetRootState(),Stop());
	}

	State::StateStatus SteeringSystem::Update(float fDt)
	{
		Prof(SteeringSystem);
		UpdateSteering();
		GetClient()->SetMovementVector(GetMoveVector());
		switch(m_MoveMode)
		{
		case Walk:
			GetClient()->PressButton(BOT_BUTTON_WALK);
			break;
		case Run:
		default:
			/*GetClient()->ReleaseButton(BOT_BUTTON_WALK);*/
			break;
		}
		return State_Busy;
	}
}
