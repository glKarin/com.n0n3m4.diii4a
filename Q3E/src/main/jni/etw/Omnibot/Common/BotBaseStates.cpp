#include "PrecompCommon.h"
#include "BotBaseStates.h"
#include "ScriptManager.h"
#include "gmBotLibrary.h"

const obReal ROAM_GOAL_PRIORITY = 0.05f;
const obReal CTF_PRIORITY = 0.55f;
const obReal RETURN_FLAG_PRIORITY = 0.35f;
const obReal SNIPE_PRIORITY = 0.7f;
const obReal ATTACK_GOAL_PRIORITY = 0.5f;
const obReal DEFEND_GOAL_PRIORITY = 0.5f;

const float MIN_RENDER_TIME = 0.05f;

extern float g_fTopWaypointOffset;
extern float g_fBottomWaypointOffset;

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////
	//CaptureTheFlag::CaptureTheFlag() 
	//	: StateChild("CaptureTheFlag")
	//	, FollowPathUser("CaptureTheFlag")
	//	, m_GoalState(Idle)
	//	, m_LastFlagState(0)
	//	, m_NextMoveTime(0)
	//{
	//}

	//void CaptureTheFlag::GetDebugString(StringStr &out)
	//{
	//	if(!IsActive())
	//		return;
	//	
	//	switch(m_GoalState)
	//	{
	//	case Idle:
	//		out << "Idle";
	//		break;
	//	case GettingFlag:
	//		out << "Getting Flag";
	//		break;
	//	case CarryingToCap:
	//		out << "Carrying To Cap";
	//		break;
	//	case CarryingToHold:
	//		out << "Carrying To Hold";
	//		break;
	//	case HoldingFlag:
	//		out << "Holding Flag";
	//		break;
	//	default:
	//		break;
	//	}

	//	if(m_MapGoalFlag)
	//		out << std::endl << m_MapGoalFlag->GetName();
	//	if(m_MapGoalCap)
	//		out << std::endl << m_MapGoalCap->GetName();
	//}

	//MapGoal *CaptureTheFlag::GetMapGoalPtr()
	//{
	//	return m_MapGoalCap ? m_MapGoalCap.get() : m_MapGoalFlag.get();
	//}

	//void CaptureTheFlag::RenderDebug()
	//{
	//	if(m_MapGoalFlag)
	//	{
	//		Utils::OutlineOBB(m_MapGoalFlag->GetWorldBounds(), COLOR::GREEN, MIN_RENDER_TIME);
	//	}
	//	if(m_MapGoalCap)
	//	{
	//		Utils::OutlineOBB(m_MapGoalCap->GetWorldBounds(), COLOR::GREEN, MIN_RENDER_TIME);
	//	}
	//}

	//bool CaptureTheFlag::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	switch(GetGoalState())
	//	{
	//	case GettingFlag:
	//		{
	//			if(!m_MapGoalFlag)
	//				return false;

	//			_final = !m_MapGoalFlag->RouteTo(GetClient(), _desination);
	//			return true;
	//		}
	//	case CarryingToCap:
	//	case CarryingToHold:
	//	case HoldingFlag:
	//		{
	//			_final = !m_MapGoalCap->RouteTo(GetClient(), _desination);
	//			return true;
	//		}
	//	case Idle:
	//		break;
	//	}
	//	return false;
	//}

	//obReal CaptureTheFlag::GetPriority()
	//{
	//	if(!IsActive())
	//	{
	//		if(!m_MapGoalFlag)
	//		{
	//			MapGoalPtr available, carrying;
	//			GoalManager::Query qry(0xbdeaa8d7 /* flag */, GetClient());
	//			GoalManager::GetInstance()->GetGoals(qry);
	//			for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//			{
	//				if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//					continue;

	//				if(qry.m_List[i]->GetOwner() == GetClient()->GetGameEntity())
	//				{
	//					carrying = qry.m_List[i];
	//					continue;
	//				}

	//				if(!GetClient()->IsFlagGrabbable(qry.m_List[i]))
	//					continue;

	//				if(!available)
	//				{
	//					if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) < 1)
	//						continue;

	//					if(qry.m_List[i]->GetGoalState() != S_FLAG_CARRIED)
	//					{
	//						available = qry.m_List[i];
	//						continue;
	//					}
	//				}
	//			}

	//			if(carrying)
	//			{
	//				m_MapGoalFlag = carrying;
	//			}
	//			else if(available)
	//			{
	//				m_GoalState = GettingFlag;
	//				m_MapGoalFlag = available;
	//			}		
	//		}

	//		if(GetClient()->DoesBotHaveFlag(m_MapGoalFlag) && !m_MapGoalCap)
	//		{
	//			// if we got a flag but not cap lets not activate
	//			// TODO: consider an evade substate?
	//			if(!LookForCapGoal(m_MapGoalCap,m_GoalState))
	//				m_MapGoalFlag.reset();
	//		}
	//	}

	//	// cs: check m_MapGoalCap first in case the bot randomly walks over a dropped flag
	//	// otherwise it will return zero priority since m_MapGoalFlag is set to 'carrying' above
	//	if(m_MapGoalCap)
	//		return m_MapGoalCap->GetPriorityForClient(GetClient());
	//	if(m_MapGoalFlag)
	//		return m_MapGoalFlag->GetPriorityForClient(GetClient());

	//	return 0.f;
	//}

	//void CaptureTheFlag::Enter()
	//{
	//	m_LastFlagState = m_MapGoalFlag ? m_MapGoalFlag->GetGoalState() : 0;

	//	Tracker.InProgress.Set(m_MapGoalFlag, GetClient()->GetTeam());
	//	FINDSTATEIF(FollowPath,GetRootState(),Goto(this));
	//}

	//void CaptureTheFlag::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_GoalState = Idle;
	//	m_NextMoveTime = 0;
	//	
	//	m_MapGoalFlag.reset();
	//	m_MapGoalCap.reset();

	//	Tracker.Reset();
	//}

	//State::StateStatus CaptureTheFlag::Update(float fDt)
	//{
	//	switch(GetGoalState())
	//	{
	//	case GettingFlag:
	//		{
	//			// cs: this is a hack. if too many are in progress, bail.
	//			if (m_MapGoalFlag && m_MapGoalFlag->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) < 0) {
	//				BlackboardDelay(10.f, m_MapGoalFlag->GetSerialNum());
	//				return State_Finished;
	//			}

	//			if(m_MapGoalFlag && !GetClient()->IsFlagGrabbable(m_MapGoalFlag))
	//				return State_Finished;

	//			if(GetClient()->DoesBotHaveFlag(m_MapGoalFlag))
	//			{
	//				m_MapGoalCap.reset();
	//				if(!LookForCapGoal(m_MapGoalCap,m_GoalState))
	//				{
	//					return State_Finished;
	//				}
	//				Tracker.InUse.Set(m_MapGoalFlag, GetClient()->GetTeam());
	//				FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	//			}
	//			else
	//			{	
	//				/*if(m_MapGoalFlag)
	//				{
	//					const int flagState = m_MapGoalFlag->GetGoalState();
	//					if(flagState != m_LastFlagState)
	//					{
	//						FINDSTATEIF(FollowPath,GetRootState(),Stop());
	//						return State_Finished;
	//					}
	//				}*/
	//				
	//				// Someone else grab it?
	//				if(m_MapGoalFlag->GetGoalState() == S_FLAG_CARRIED)
	//				{
	//					FINDSTATEIF(FollowPath,GetRootState(),Stop());
	//					return State_Finished;
	//				}

	//				const int iMyTeam = GetClient()->GetTeam();
	//				if(m_MapGoalFlag->GetControllingTeam(iMyTeam))
	//					return State_Finished;

	//				if(DidPathSucceed())
	//					GetClient()->GetSteeringSystem()->SetTarget(m_MapGoalFlag->GetPosition(), m_MapGoalFlag->GetRadius());
	//			}

	//			// Doing this last so we can change state before finishing
	//			// flag may be disabled as part of pickup process, and we 
	//			// want to be able to go into the cap state before that.
	//			if(m_MapGoalFlag && !m_MapGoalFlag->IsAvailable(GetClient()->GetTeam()))
	//				return State_Finished;
	//			break;
	//		}
	//	case CarryingToCap:
	//		{
	//			if(!m_MapGoalCap->IsAvailable(GetClient()->GetTeam()))
	//				return State_Finished;

	//			if(!GetClient()->DoesBotHaveFlag(m_MapGoalFlag))
	//				return State_Finished;

	//			if(DidPathSucceed())
	//				GetClient()->GetSteeringSystem()->SetTarget(m_MapGoalCap->GetPosition(), m_MapGoalCap->GetRadius());

	//			if(m_MapGoalCap && !m_MapGoalCap->IsAvailable(GetClient()->GetTeam()))
	//				return State_Finished;
	//			break;
	//		}
	//	case CarryingToHold:
	//		{
	//			if(DidPathSucceed())
	//			{
	//				// shift positions?
	//				m_GoalState = HoldingFlag;
	//				m_NextMoveTime = IGame::GetTime() + Utils::SecondsToMilliseconds(Mathf::UnitRandom() * 10.f);
	//			}

	//			if(m_MapGoalCap && !m_MapGoalCap->IsAvailable(GetClient()->GetTeam()))
	//				return State_Finished;
	//			break;
	//		}
	//	case HoldingFlag:
	//		{
	//			// We don't care about path failure in here, so we return instead of letting it
	//			// check m_PathFailed below.
	//			if(IGame::GetTime() >= m_NextMoveTime)
	//			{
	//				m_GoalState = CarryingToHold;
	//				FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	//			}

	//			if(m_MapGoalCap && !m_MapGoalCap->IsAvailable(GetClient()->GetTeam()))
	//				return State_Finished;

	//			return State_Busy;
	//		}
	//	case Idle:
	//		{
	//			OBASSERT(0, "Invalid Flag State!");
	//			return State_Finished;
	//		}
	//	}

	//	//////////////////////////////////////////////////////////////////////////
	//	if(DidPathFail())
	//	{
	//		// Delay it from being used for a while.
	//		if(m_MapGoalFlag)
	//			BlackboardDelay(10.f, m_MapGoalFlag->GetSerialNum());
	//		return State_Finished;
	//	}
	//	//////////////////////////////////////////////////////////////////////////

	//	return State_Busy;
	//}

	//bool CaptureTheFlag::LookForCapGoal(MapGoalPtr &ptr, GoalState &st)
	//{
	//	//////////////////////////////////////////////////////////////////////////
	//	{
	//		GoalManager::Query qry(0xc9326a43 /* cappoint */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qry);
	//		if(qry.GetBest(ptr))
	//		{
	//			m_GoalState = CarryingToCap;
	//			return true;
	//		}
	//	}
	//	//////////////////////////////////////////////////////////////////////////
	//	{
	//		GoalManager::Query qry(0xc7ab68ba /* caphold */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qry);
	//		if(qry.GetBest(ptr))
	//		{
	//			m_GoalState = CarryingToHold;
	//			return true;
	//		}
	//	}
	//	return false;
	//}

	//////////////////////////////////////////////////////////////////////////

	//Snipe::Snipe()
	//	: StateChild("Snipe")
	//	, FollowPathUser("Snipe")
	//	, m_MinCampTime(6.0)
	//	, m_MaxCampTime(10.0)
	//	, m_ExpireTime(0)
	//	, m_Stance(StanceStand)
	//	, m_NextScanTime(0)
	//{
	//}

	//void Snipe::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void Snipe::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineAABB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, MIN_RENDER_TIME);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,MIN_RENDER_TIME);
	//		m_TargetZone.RenderDebug();
	//	}
	//}

	//// FollowPathUser functions.
	//bool Snipe::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool Snipe::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_AimPosition;
	//	return true;
	//}

	//void Snipe::OnTarget()
	//{
	//	FINDSTATE(ws, WeaponSystem, GetRootState());
	//	if(ws && ws->CurrentWeaponIs(m_NonScopedWeaponId) || ws->CurrentWeaponIs(m_ScopedWeaponId))
	//	{
	//		ws->ChargeWeapon();
	//		ws->ZoomWeapon();
	//	}
	//}

	//obReal Snipe::GetPriority()
	//{
	//	if(!GetClient()->CanBotSnipe())
	//		return 0.f;

	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0xe8fbadc0 /* snipe */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	qry.GetBest(m_MapGoal);

	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void Snipe::Enter()
	//{
	//	m_AimPosition = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;

	//	GetClient()->GetSniperWeapon(m_NonScopedWeaponId, m_ScopedWeaponId);

	//	m_ExpireTime = 0;

	//	m_MapGoal->GetProperty("Stance",m_Stance);
	//	m_MapGoal->GetProperty("MinCampTime",m_MinCampTime);
	//	m_MapGoal->GetProperty("MaxCampTime",m_MaxCampTime);

	//	Tracker.InProgress = m_MapGoal;
	//	m_TargetZone.Restart(256.f);
	//	m_NextScanTime = 0;

	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));
	//}

	//void Snipe::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();

	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(),ReleaseWeaponRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus Snipe::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	if(!Tracker.InUse && m_MapGoal->GetSlotsOpen(MapGoal::TRACK_INUSE) < 1)
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		// Only hang around here for a certain amount of time
	//		if(m_ExpireTime==0)
	//		{
	//			m_ExpireTime = IGame::GetTime()+Mathf::IntervalRandomInt(m_MinCampTime.GetMs(),m_MaxCampTime.GetMs());
	//			Tracker.InUse = m_MapGoal;
	//		}
	//		else if(IGame::GetTime() > m_ExpireTime)
	//		{
	//			// Delay it from being used for a while.
	//			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}

	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Low,this,GetNameHash()));
	//		FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), m_ScopedWeaponId));

	//		m_AimPosition = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;

	//		m_TargetZone.Update(GetClient());

	//		if(m_NextScanTime <= IGame::GetTime())
	//		{
	//			m_NextScanTime = IGame::GetTime()+Mathf::IntervalRandomInt(2000,4000);
	//			m_TargetZone.UpdateAimPosition();
	//		}
	//		
	//		if(m_TargetZone.HasAim())
	//			m_AimPosition = m_TargetZone.GetAimPosition();

	//		GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition(), m_MapGoal->GetRadius());

	//		if(m_Stance==StanceProne)
	//			GetClient()->PressButton(BOT_BUTTON_PRONE);
	//		else if(m_Stance==StanceCrouch)
	//			GetClient()->PressButton(BOT_BUTTON_CROUCH);
	//	}
	//	return State_Busy;
	//}
	//////////////////////////////////////////////////////////////////////////

	//ReturnTheFlag::ReturnTheFlag() 
	//	: StateChild("ReturnTheFlag")
	//	, FollowPathUser("ReturnTheFlag")
	//{
	//	m_LastGoalPosition = Vector3f::ZERO;
	//}

	///*void ReturnTheFlag::GetDebugString(StringStr &out)
	//{
	//	return "";
	//}*/

	//MapGoal *ReturnTheFlag::GetMapGoalPtr()
	//{
	//	return m_MapGoal.get();
	//}

	//void ReturnTheFlag::RenderDebug()
	//{
	//	if(m_MapGoal)
	//	{
	//		Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::GREEN, MIN_RENDER_TIME);
	//		Utils::DrawLine(GetClient()->GetEyePosition(), m_MapGoal->GetWorldUsePoint(), COLOR::MAGENTA, MIN_RENDER_TIME);
	//	}
	//}

	//obReal ReturnTheFlag::GetPriority()
	//{
	//	if(!m_MapGoal)
	//	{
	//		if(!m_MapGoal)
	//		{
	//			GoalManager::Query qry(0xa06840e5 /* flagreturn */, GetClient());
	//			GoalManager::GetInstance()->GetGoals(qry);
	//			for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//			{
	//				if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//					continue;

	//				if (qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INUSE, GetClient()->GetTeam()) < 1)
	//					continue;

	//				const int iGoalState = qry.m_List[i]->GetGoalState();

	//				if(iGoalState == S_FLAG_DROPPED || iGoalState == S_FLAG_CARRIED)
	//				{
	//					m_MapGoal = qry.m_List[i];
	//					break;
	//				}
	//				else if(iGoalState == S_FLAG_AT_BASE)
	//				{
	//					continue;
	//				}
	//				else if(GetClient()->IsFlagGrabbable(qry.m_List[i]))
	//				{
	//					m_MapGoal = qry.m_List[i];
	//					break;
	//				}
	//			}
	//		}
	//	}

	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void ReturnTheFlag::Enter()
	//{
	//	m_MapGoalProg.Set(m_MapGoal, GetClient()->GetTeam());
	//	m_LastGoalPosition = m_MapGoal->GetWorldUsePoint();
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, m_LastGoalPosition, m_MapGoal->GetRadius()));
	//}

	//void ReturnTheFlag::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoalProg.Reset();
	//	m_MapGoal.reset();
	//	m_LastGoalPosition = Vector3f::ZERO;
	//}

	//State::StateStatus ReturnTheFlag::Update(float fDt)
	//{
	//	OBASSERT(m_MapGoal, "No Map Goal!");
	//	//////////////////////////////////////////////////////////////////////////
	//	if(DidPathFail())
	//	{
	//		// Delay it from being used for a while.
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(m_MapGoal->GetDeleteMe())
	//		return State_Finished;

	//	//////////////////////////////////////////////////////////////////////////
	//	// Re-plan if it has moved a bit from last position
	//	// Predict path?
	//	Vector3f vUpdatedPosition = m_MapGoal->GetWorldUsePoint();
	//	if(SquaredLength(m_LastGoalPosition, vUpdatedPosition) > Mathf::Sqr(100.f))
	//	{
	//		m_LastGoalPosition = vUpdatedPosition;
	//		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, m_LastGoalPosition, m_MapGoal->GetRadius()));
	//	}

	//	if(m_MapGoal->GetGoalState() == S_FLAG_AT_BASE || !GetClient()->IsFlagGrabbable(m_MapGoal))
	//		return State_Finished;

	//	FINDSTATE(follow, FollowPath, GetRootState());
	//	if(follow != NULL && !follow->IsActive())
	//		follow->Goto(this, m_LastGoalPosition, m_MapGoal->GetRadius());

	//	return State_Busy;
	//}

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////

	//Defend::Defend() 
	//	: StateChild("Defend")
	//	, FollowPathUser("Attack")
	//	, m_MinCampTime(2.f)
	//	, m_MaxCampTime(8.f)
	//	, m_ExpireTime(0)
	//	, m_EquipWeapon(0)
	//	, m_Stance(StanceStand)
	//{
	//}

	//void Defend::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void Defend::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		m_TargetZone.RenderDebug();
	//	}
	//}

	//bool Defend::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	OBASSERT(m_MapGoal,"No Map Goal!");
	//	_final = !m_MapGoal->RouteTo(GetClient(), _desination);
	//	return true;
	//}

	//bool Defend::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_AimPosition;
	//	return true;
	//}

	//void Defend::OnTarget()
	//{
	//}

	//obReal Defend::GetPriority()
	//{
	//	if(!m_MapGoal)
	//	{
	//		//////////////////////////////////////////////////////////////////////////
	//		// do we have the weapon capabilities?
	//		m_EquipWeapon = 0;

	//		/*bool bNeedsWeapon = false;
	//		int UseWeapon[DefendGoal::MaxUseWeapon];
	//		AiState::WeaponSystem *wsys = GetClient()->GetWeaponSystem();
	//		for(int i = 0; i < AttackGoal::MaxUseWeapon; ++i)
	//		{
	//			m_MapGoal->GetProperty(va("UseWeapon%d",i),UseWeapon[i]);
	//			if(UseWeapon[i])
	//			{
	//				bNeedsWeapon = true;

	//				if(wsys->HasWeapon(UseWeapon[i]))
	//				{
	//					m_EquipWeapon = UseWeapon[i];
	//					break;
	//				}
	//			}
	//		}
	//		if(bNeedsWeapon && !m_EquipWeapon)
	//			return 0.f;*/
	//		//////////////////////////////////////////////////////////////////////////

	//		GoalManager::Query qry(0x4fed19c1 /* defend */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qry);
	//		qry.GetBest(m_MapGoal);
	//	}
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void Defend::Enter()
	//{
	//	m_AimPosition = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;

	//	Tracker.InProgress = m_MapGoal;

	//	m_TargetZone.Restart(256.f);
	//	m_ExpireTime = 0;

	//	m_MapGoal->GetProperty("Stance",m_Stance);
	//	m_MapGoal->GetProperty("MinCampTime",m_MinCampTime);
	//	m_MapGoal->GetProperty("MaxCampTime",m_MaxCampTime);

	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));

	//	//FINDSTATEIF(FloodFiller,GetRootState(),StartFloodFill(m_MapGoal->GetPosition()));
	//}

	//void Defend::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();

	//	FINDSTATEIF(WeaponSystem,GetRootState(),ClearOverrideWeaponID());
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus Defend::Update(float fDt)
	//{
	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	//////////////////////////////////////////////////////////////////////////
	//	if(DidPathFail())
	//	{
	//		// Delay it from being used for a while.
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}
	//	//////////////////////////////////////////////////////////////////////////

	//	if(!Tracker.InUse && m_MapGoal->GetSlotsOpen(MapGoal::TRACK_INUSE) < 1)
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		// Only hang around here for a certain amount of time
	//		if(m_ExpireTime==0)
	//		{
	//			m_ExpireTime = IGame::GetTime()+Mathf::IntervalRandomInt(m_MinCampTime.GetMs(),m_MaxCampTime.GetMs());
	//			Tracker.InUse = m_MapGoal;
	//		}
	//		else if(IGame::GetTime() > m_ExpireTime)
	//		{
	//			// Delay it from being used for a while.
	//			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}

	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Low,this,GetNameHash()));
	//		if(m_EquipWeapon)
	//		{
	//			FINDSTATEIF(WeaponSystem,GetRootState(),SetOverrideWeaponID(m_EquipWeapon));
	//		}

	//		m_AimPosition = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;

	//		m_TargetZone.Update(GetClient());

	//		if(m_TargetZone.HasAim())
	//			m_AimPosition = m_TargetZone.GetAimPosition();

	//		GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition(), m_MapGoal->GetRadius());

	//		NavFlags nodeFlags = m_MapGoal->GetFlags();
	//		if (nodeFlags & F_NAV_PRONE || m_Stance==StanceProne)
	//			GetClient()->PressButton(BOT_BUTTON_PRONE);
	//		else if (nodeFlags & F_NAV_CROUCH || m_Stance==StanceCrouch)
	//			GetClient()->PressButton(BOT_BUTTON_CROUCH);
	//	}
	//	return State_Busy;
	//}

	//////////////////////////////////////////////////////////////////////////

	//Attack::Attack() 
	//	: StateChild("Attack")
	//	, FollowPathUser("Attack")
	//	, m_MinCampTime(2.f)
	//	, m_MaxCampTime(8.f)
	//	, m_ExpireTime(0)
	//	, m_EquipWeapon(0)
	//	, m_Stance(StanceStand)
	//{
	//}

	//void Attack::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void Attack::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		m_TargetZone.RenderDebug();
	//	}
	//}

	//bool Attack::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	_final = !m_MapGoal->RouteTo(GetClient(), _desination);
	//	return true;
	//}

	//bool Attack::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_AimPosition;
	//	return true;
	//}

	//void Attack::OnTarget()
	//{
	//}

	//obReal Attack::GetPriority()
	//{
	//	if(!m_MapGoal)
	//	{
	//		//////////////////////////////////////////////////////////////////////////
	//		// do we have the weapon capabilities?
	//		m_EquipWeapon = 0;

	//		/*bool bNeedsWeapon = false;
	//		int UseWeapon[DefendGoal::MaxUseWeapon];
	//		AiState::WeaponSystem *wsys = GetClient()->GetWeaponSystem();
	//		for(int i = 0; i < AttackGoal::MaxUseWeapon; ++i)
	//		{
	//			m_MapGoal->GetProperty(va("UseWeapon%d",i),UseWeapon[i]);
	//			if(UseWeapon[i])
	//			{
	//				bNeedsWeapon = true;

	//				if(wsys->HasWeapon(UseWeapon[i]))
	//				{
	//					m_EquipWeapon = UseWeapon[i];
	//					break;
	//				}
	//			}
	//		}
	//		if(bNeedsWeapon && !m_EquipWeapon)
	//			return 0.f;*/
	//		//////////////////////////////////////////////////////////////////////////

	//		GoalManager::Query qry(0x4595b8fd /* attack */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qry);
	//		qry.GetBest(m_MapGoal);
	//	}
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void Attack::Enter()
	//{
	//	m_AimPosition = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;

	//	Tracker.InProgress = m_MapGoal;

	//	m_TargetZone.Restart(256.f);
	//	m_ExpireTime = 0;

	//	m_MapGoal->GetProperty("Stance",m_Stance);
	//	m_MapGoal->GetProperty("MinCampTime",m_MinCampTime);
	//	m_MapGoal->GetProperty("MaxCampTime",m_MaxCampTime);

	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));
	//}

	//void Attack::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();

	//	FINDSTATEIF(WeaponSystem,GetRootState(),ClearOverrideWeaponID());
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus Attack::Update(float fDt)
	//{
	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	//////////////////////////////////////////////////////////////////////////
	//	if(DidPathFail())
	//	{
	//		// Delay it from being used for a while.
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}
	//	//////////////////////////////////////////////////////////////////////////

	//	if(!Tracker.InUse && m_MapGoal->GetSlotsOpen(MapGoal::TRACK_INUSE) < 1)
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		// Only hang around here for a certain amount of time
	//		if(m_ExpireTime==0)
	//		{
	//			m_ExpireTime = IGame::GetTime()+Mathf::IntervalRandomInt(m_MinCampTime.GetMs(),m_MaxCampTime.GetMs());
	//			Tracker.InUse = m_MapGoal;
	//		}
	//		else if(IGame::GetTime() > m_ExpireTime)
	//		{
	//			// Delay it from being used for a while.
	//			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}

	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Low,this,GetNameHash()));
	//		if(m_EquipWeapon)
	//		{
	//			FINDSTATEIF(WeaponSystem,GetRootState(),SetOverrideWeaponID(m_EquipWeapon));
	//		}

	//		m_AimPosition = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;

	//		m_TargetZone.Update(GetClient());

	//		if(m_TargetZone.HasAim())
	//			m_AimPosition = m_TargetZone.GetAimPosition();

	//		GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition(), m_MapGoal->GetRadius());

	//		NavFlags nodeFlags = m_MapGoal->GetFlags();
	//		if (nodeFlags & F_NAV_PRONE || m_Stance==StanceProne)
	//			GetClient()->PressButton(BOT_BUTTON_PRONE);
	//		else if (nodeFlags & F_NAV_CROUCH || m_Stance==StanceCrouch)
	//			GetClient()->PressButton(BOT_BUTTON_CROUCH);
	//	}
	//	return State_Busy;
	//}

	//////////////////////////////////////////////////////////////////////////

	Roam::Roam() 
		: StateChild("Roam")
		, FollowPathUser("Roam")
	{
	}

	obReal Roam::GetPriority()
	{
		return ROAM_GOAL_PRIORITY; 
	}

	void Roam::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));
	}

	State::StateStatus Roam::Update(float fDt)
	{
		FINDSTATE(follow, FollowPath, GetRootState());
		if(follow != NULL && !follow->IsActive())
			follow->GotoRandomPt(this);
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	HighLevel::HighLevel() 
		: StatePrioritized("HighLevel")
	{
		//AppendState(new CaptureTheFlag);
		//AppendState(new ReturnTheFlag);

		AppendState(new Roam);
	}

	/*obReal HighLevel::GetPriority()
	{
		if(IGame::GetGameState() != IGame::GetLastGameState())
			return 0.f;

		return StatePrioritized::GetPriority();
	}*/
	
	//////////////////////////////////////////////////////////////////////////

	FollowPathUser::FollowPathUser(const String &_user)
		: m_UserName(0)
		, m_CallingThread(GM_INVALID_THREAD)
		, m_DestinationIndex(0)
		, m_PathFailed(None)
		, m_PathSuccess(false)
	{
		m_UserName = Utils::MakeHash32(_user);
	}
	FollowPathUser::FollowPathUser(obuint32 _name)
		: m_UserName(_name)
		, m_CallingThread(GM_INVALID_THREAD)
		, m_PathFailed(None)
		, m_PathSuccess(false)
	{
	}

	void FollowPathUser::SetFollowUserName(obuint32 _name)
	{
		m_UserName = _name;
	}

	void FollowPathUser::SetFollowUserName(const String &_name)
	{
		SetFollowUserName(Utils::MakeHash32(_name));
	}

	//////////////////////////////////////////////////////////////////////////

	bool FollowPath::m_OldLadderStyle = false;

	FollowPath::FollowPath() 
		: StateChild("FollowPath")
		, m_PathStatus(PathFinished)
		, m_PtOnPath(Vector3f::ZERO)
		, m_LookAheadPt(Vector3f::ZERO)
		, m_JumpTime(0)
		, m_LastStuckTime(0)
		, m_PassThroughState(0)
		, m_RayDistance(-1.0f)
	{
	}

	void FollowPath::GetDebugString(StringStr &out)
	{
		if(m_Query.m_User && IsActive())
			out << Utils::HashToString(m_Query.m_User->GetFollowUserName());
		else
			out << "<none>";
	}

	void FollowPath::RenderDebug()
	{
		Utils::DrawLine(GetClient()->GetPosition(), m_PtOnPath, COLOR::BLUE, MIN_RENDER_TIME);
		Utils::DrawLine(GetClient()->GetPosition(), m_LookAheadPt, COLOR::MAGENTA, MIN_RENDER_TIME);
		m_CurrentPath.DebugRender(COLOR::RED, MIN_RENDER_TIME);
		Path::PathPoint pt;
		m_CurrentPath.GetCurrentPt(pt);
		Utils::DrawRadius(pt.m_Pt, pt.m_Radius, COLOR::GREEN, MIN_RENDER_TIME);
	}

	bool FollowPath::GetAimPosition(Vector3f &_aimpos)
	{
		_aimpos = m_LookAheadPt;
		return true;
	}

	void FollowPath::OnTarget()
	{
	}

	void FollowPath::Enter()
	{
		m_LookAheadPt = GetClient()->GetEyePosition() + GetClient()->GetFacingVector() * 512.f;
		FINDSTATEIF(Aimer, GetParent(), AddAimRequest(Priority::Idle, this, GetNameHash()));
	}

	void FollowPath::Exit()
	{
		Stop();

		FINDSTATEIF(SteeringSystem, GetRootState(), SetTarget(GetClient()->GetPosition()));
		FINDSTATEIF(Aimer, GetParent(), ReleaseAimRequest(GetNameHash()));
	}

	void FollowPath::Stop(bool _clearuser)
	{
		// _clearuser is true if HighLevel goal was aborted
		if(m_PassThroughState && _clearuser)
		{
			if(m_Query.m_User && m_Query.m_User->GetFollowUserName() == m_PassThroughState)
			{
				// don't interrupt active paththrough
				if(m_SavedQuery.m_User)
				{
					if(m_SavedQuery.m_User!=m_Query.m_User) 
						m_SavedQuery.m_User->OnPathFailed(FollowPathUser::Interrupted);
					m_SavedQuery.m_User = 0;
				}
				return;
			}
		}

		if(m_PathStatus == PathInProgress)
		{
			NotifyUserFailed(FollowPathUser::Interrupted);
		}

		m_PathStatus = PathFinished;
		m_CurrentPath.Clear();

		if(_clearuser)
			ClearUser();
	}

	void FollowPath::ClearUser()
	{
		m_Query.m_User = 0;
	}

	void FollowPath::CancelPathThrough()
	{
		// always want to do this, if state active or not
		if(m_PassThroughState)
		{
			FINDSTATE(ll,LowLevel,GetRootState());
			if ( ll != NULL ) {
				State *pPathThrough = ll->FindState(m_PassThroughState);
				if(pPathThrough)
				{
					pPathThrough->EndPathThrough();
				}
			}
			m_PassThroughState = 0;
		}
	}

	void FollowPath::SaveQuery()
	{
		m_SavedQuery = m_Query;
	}

	void FollowPath::RestoreQuery()
	{
		m_Query = m_SavedQuery;
	}

	void FollowPath::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(MESSAGE_DYNAMIC_PATHS_CHANGED)
			{
				using namespace AiState;			
				const Event_DynamicPathsChanged *m = _message.Get<Event_DynamicPathsChanged>();
				if(m != NULL && (m->m_TeamMask & 1<<GetClient()->GetTeam()))
				{
					DynamicPathUpdated(m);
				}
				break;
			}
		};
	}

	void FollowPath::DynamicPathUpdated(const Event_DynamicPathsChanged *_m)
	{
		const Path::PathPoint *p;
		for(int i = m_CurrentPath.GetCurrentPtIndex(); i < m_CurrentPath.GetNumPts(); ++i)
		{
			p = m_CurrentPath.GetPt(i);

			if(_m->m_NavId)
			{
				if(p->m_NavId==_m->m_NavId)
				{
					Repath();
					break;
				}
			}
			else if(_m->m_NavFlags)
			{
				if(p->m_NavFlags & _m->m_NavFlags)
				{
					Repath();
					break;
				}
			}
			else if(p->m_NavFlags&F_NAV_DYNAMIC)
			{
				Repath();
				break;
			}
		}
	}

	bool FollowPath::GotoRandomPt(FollowPathUser *_owner, MoveMode _movemode /*= Run*/)
	{
		PathPlannerBase *pPathPlanner = IGameManager::GetInstance()->GetNavSystem();
		Vector3f vDestination = pPathPlanner->GetRandomDestination(GetClient(),GetClient()->GetPosition(),GetClient()->GetTeamFlag());
		return Goto(_owner,vDestination);
	}

	bool FollowPath::Goto(FollowPathUser *_owner, MoveMode _movemode /*= Run*/, bool _skiplastpt /*= false*/)
	{
		m_IgnorePathNotFound = false;
		bool bFinalDest = true;
		if (!_owner) return false;

		DestinationVector destlist;
		if(_owner->GetNextDestination(destlist, bFinalDest, _skiplastpt))
		{
			return Goto(_owner, destlist, _movemode, _skiplastpt, bFinalDest);
		}

		if(_owner == m_Query.m_User)
		{
			m_PathStatus = PathNotFound;
			NotifyUserFailed(FollowPathUser::NoPath);
			m_Query.m_User = 0;
		}
		else
		{
			_owner->OnPathFailed(FollowPathUser::NoPath);
		}
		return false;
	}

	bool FollowPath::Goto(FollowPathUser *_owner, const Vector3f &_pos, float _radius /*= 32.f*/, MoveMode _movemode /*= Run*/, bool _skiplastpt /*= false*/)
	{
		m_IgnorePathNotFound = true;
		DestinationVector destlist;
		destlist.push_back(Destination(_pos,_radius));
		return Goto(_owner, destlist, _movemode, _skiplastpt);
	}

	bool FollowPath::Goto(FollowPathUser *_owner, const Vector3List &_goals, float _radius /*= 32.f*/, MoveMode _movemode /*= Run*/, bool _skiplastpt /*= false*/)
	{
		m_IgnorePathNotFound = true;
		DestinationVector destlist;
		for(obuint32 i = 0; i < _goals.size(); ++i)
		    destlist.push_back(Destination(_goals[i],_radius));
		return Goto(_owner, destlist, _movemode, _skiplastpt);
	}

	bool FollowPath::Goto(FollowPathUser *_owner, const MapGoalList &_goals, MoveMode _movemode /*= Run*/, bool _skiplastpt /*= false*/)
	{
		DestinationVector destlist;
		for(obuint32 i = 0; i < _goals.size(); ++i)
			destlist.push_back(Destination(_goals[i]->GetPosition(),_goals[i]->GetRadius()));
		return Goto(_owner, destlist, _movemode, _skiplastpt);
	}

	bool FollowPath::Goto(FollowPathUser *_owner, const DestinationVector &_goals, MoveMode _movemode /*= Run*/, bool _skiplastpt /*= false*/, bool _final /*= true*/)
	{
		if (m_PassThroughState && _owner != m_Query.m_User)
		{
			if(_owner->GetFollowUserName() == m_PassThroughState)
			{
				// paththrough called Goto,
				// save HighLevel goal's query
				OBASSERT(!m_SavedQuery.m_User, "m_SavedQuery overwritten");
				SaveQuery();
			}
			else if(m_Query.m_User && m_Query.m_User->GetFollowUserName() == m_PassThroughState)
			{
				if (m_SavedQuery.m_User && _owner != m_SavedQuery.m_User)
					m_SavedQuery.m_User->OnPathFailed(FollowPathUser::Interrupted);

				// remember current query and don't interrupt active paththrough
				m_SavedQuery.m_User = _owner;
				m_SavedQuery.m_Destination = _goals;
				m_SavedQuery.m_MoveMode = _movemode;
				m_SavedQuery.m_SkipLastPt = _skiplastpt;
				m_SavedQuery.m_Final = _final;
				return true;
			}
		}

		m_Query.m_User = _owner;
		m_Query.m_Destination = _goals;
		m_Query.m_MoveMode = _movemode;
		m_Query.m_SkipLastPt = _skiplastpt;
		m_Query.m_Final = _final;
		return Repath();
	}

	bool FollowPath::Repath()
	{
		if(!m_Query.m_User) return false;
		m_Query.m_User->ResetPathUser();
		m_PathThroughPtIndex = -1;


		if(GetClient()->IsDebugEnabled(BOT_DEBUG_PLANNER))
		{
			String mgn;
			FINDSTATE(hl,HighLevel,GetRootState());
			if(hl != NULL)
			{
				State *state = hl->GetActiveState();
				if(!state) mgn="not active";
				else {
					MapGoal *g = state->GetMapGoalPtr();
					if(g) mgn = g->GetName();
					else mgn = state->GetName();
				}
			}
			const Vector3f &pos = GetClient()->GetPosition();
			EngineFuncs::ConsoleMessage(va("Path planner: time %.2f, position (%.0f,%.0f,%.0f), %s, %s %s", 
				IGame::GetTimeSecs(), pos.x, pos.y, pos.z, this->GetClient()->GetName(), mgn.c_str(),
				m_PassThroughState ? "- paththrough" : ""));
		}

		PathPlannerBase *pPathPlanner = IGameManager::GetInstance()->GetNavSystem();
		m_Query.m_User->m_DestinationIndex = pPathPlanner->PlanPathToNearest(GetClient(), GetClient()->GetPosition(), m_Query.m_Destination, GetClient()->GetTeamFlag());
		if(pPathPlanner->FoundGoal())
		{
			m_CurrentPath.Clear();
			pPathPlanner->GetPath(m_CurrentPath);
			if(!m_Query.m_SkipLastPt || m_CurrentPath.GetNumPts()==0)
			{
				Destination dest = m_Query.m_Destination[m_Query.m_User->m_DestinationIndex];
				int num = m_CurrentPath.GetNumPts();
				if(num >= 2){
					Path::PathPoint pt1, pt2;
					m_CurrentPath.GetPt(num-1, pt2);
					if(Length(pt2.m_Pt, dest.m_Position) > pt2.m_Radius && !pt2.m_OnPathThrough){
						m_CurrentPath.GetPt(num-2, pt1);
						Vector3f vClosest;
						float t = Utils::ClosestPtOnLine(pt1.m_Pt, pt2.m_Pt, dest.m_Position, vClosest);
						if(t < 1.f && Length(vClosest, dest.m_Position) <= MaxT(dest.m_Radius, pt2.m_Radius)){
							//destination is near connection between last two waypoints
							m_CurrentPath.RemoveLastPt();
							m_CurrentPath.AddPt(vClosest, MinT(dest.m_Radius, pt2.m_Radius))
								.Flags(pt2.m_NavFlags)
								.NavId(pt2.m_NavId);
						}
					}
				}
				//append destination to path
				m_CurrentPath.AddPt(dest.m_Position, dest.m_Radius);
			}
			GetClient()->ResetStuckTime();
			m_PathStatus = PathInProgress;
		}
		else
		{
			//path not found
			if(!m_IgnorePathNotFound) {
				FINDSTATE(hl, HighLevel, GetRootState());
				if(hl) {
					State *state = hl->GetActiveState();
					if(state) {
						MapGoal *g = state->GetMapGoalPtr();
						if(g) {
							const Vector3f &pos = GetClient()->GetPosition();
							MapDebugPrint(va("Path not found from (%.0f,%.0f,%.0f) to %s", pos.x, pos.y, pos.z, g->GetName().c_str()));
						}
					}
				}
			}

			m_PathStatus = PathNotFound;
			NotifyUserFailed(FollowPathUser::NoPath);
			if(!m_PassThroughState) m_Query.m_User = 0;
		}
		return m_PathStatus < PathFinished;
	}

	bool FollowPath::Goto(FollowPathUser *_owner, const Path &_path, MoveMode _movemode /*= Run*/)
	{
		if(!_owner)
			return false;
		//OBASSERT(_owner,"No User Defined!");

		m_Query.m_SkipLastPt = false;
		m_Query.m_Final = true;
		m_Query.m_User = _owner;
		m_Query.m_MoveMode = _movemode;
		m_Query.m_User->ResetPathUser();
		m_Query.m_Destination.resize(0);

		GetClient()->ResetStuckTime();
		m_CurrentPath = _path;
		m_PathStatus = PathInProgress;
		return true;
	}

	bool FollowPath::QueryPath(Path &pathOut, const DestinationVector &_goals, bool _skiplastpt, float _limitexpansion)
	{
		PathPlannerBase *pPathPlanner = IGameManager::GetInstance()->GetNavSystem();
		int iDestIndex = pPathPlanner->PlanPathToNearest(
			0, 
			GetClient()->GetPosition(), 
			_goals, 
			GetClient()->GetTeamFlag());

		if(pPathPlanner->FoundGoal())
		{
			pathOut.Clear();
			pPathPlanner->GetPath(pathOut);
			if(!m_Query.m_SkipLastPt)
			{
				m_CurrentPath.AddPt(
					_goals[iDestIndex].m_Position, 
					_goals[iDestIndex].m_Radius);
			}
		}
		else
		{
			return false;
		}
		return true;
	}

	bool FollowPath::IsMoving()
	{
		return !m_CurrentPath.IsEndOfPath();
	}

	void FollowPath::NotifyUserSuccess()
	{
		if(m_Query.m_User)
			m_Query.m_User->OnPathSucceeded();
	}

	void FollowPath::NotifyUserFailed(FollowPathUser::FailType _how)
	{
		if(m_Query.m_User)
			m_Query.m_User->OnPathFailed(_how);

		if(GetClient()->IsDebugEnabled(BOT_DEBUG_LOG_FAILED_PATHS))
		{
			const char *FailType = 0;
			switch(_how)
			{
			case FollowPathUser::Blocked:
				FailType = "Blocked";
				break;
			case FollowPathUser::NoPath:
				FailType = "Interrupted";
				break;
			case FollowPathUser::None:
			case FollowPathUser::Interrupted:
			default:
				break;
			}

			if(FailType)
			{
				File f;
				f.OpenForWrite(va("user/failedpaths.txt"), File::Text, true);
				if(f.IsOpen())
				{
					Vector3f Position = GetClient()->GetPosition();
					Vector3f Dest = Vector3f::ZERO;

					Path::PathPoint pt;
					if(m_CurrentPath.GetCurrentPt(pt))
						Dest = pt.m_Pt;

					f.WriteString("{"); 
					f.WriteNewLine();
					
					f.Printf("\tType = \"%s\",",FailType); f.WriteNewLine();
					f.Printf("\tP = Vector3(%f,%f,%f),",Position.x,Position.y,Position.z);
					f.WriteNewLine();
					
					if(_how == FollowPathUser::NoPath)
					{
						f.WriteString("\tDest = {"); f.WriteNewLine();
						for(obuint32 i = 0; i < m_Query.m_Destination.size(); ++i)
						{
							f.Printf("\t\tVector3(%f,%f,%f),",
								m_Query.m_Destination[i].m_Position.x,
								m_Query.m_Destination[i].m_Position.y,
								m_Query.m_Destination[i].m_Position.z);
							f.WriteNewLine();
						}
						f.WriteString("\t},"); f.WriteNewLine();
					}
					else
					{
						f.Printf("\tDest = Vector3(%f,%f,%f),",
							pt.m_Pt.x,pt.m_Pt.y,pt.m_Pt.z);
						f.WriteNewLine();
					}

					f.WriteString("},"); f.WriteNewLine();
					f.Close();
				}
			}
		}
	}

	obReal FollowPath::GetPriority() 
	{
		// always want to do this, if state active or not
		if(m_PassThroughState)
		{
			FINDSTATE(ll,LowLevel,GetRootState());
			if ( ll ) {
				State *pPathThrough = ll->FindState(m_PassThroughState);
				if(!pPathThrough || (!pPathThrough->IsActive() && pPathThrough->GetLastPriority()<Mathf::EPSILON))
				{
					// paththrough state finished
					if(m_Query.m_User && m_Query.m_User->GetFollowUserName() == m_PassThroughState)
					{
						m_PassThroughState = 0;

						if(m_SavedQuery.m_User)
						{
							// find new path to the current HighLevel goal
							RestoreQuery();
							Repath();
						}
						else
						{
							Stop(false);
							m_Query.m_User = 0;
						}
					}
					else
					{
						m_PassThroughState = 0;

						if(m_CurrentPath.GetCurrentPtIndex() != m_PathThroughPtIndex + 1)
						{
							// repath if the previous waypoint has paththrough which has been skipped
							Path::PathPoint ptPrev;
							if(m_CurrentPath.GetPreviousPt(ptPrev) && ptPrev.m_OnPathThrough && ptPrev.m_OnPathThroughParam)
							{
								Repath();
							}
						}
					}
				}
			}
		}
		return m_PathStatus < PathFinished ? (obReal)1.0 : (obReal)0.0; 
	}

	Vector3f RawWpPos(const Path::PathPoint &pp)
	{
		const float fWpHeight = g_fTopWaypointOffset - g_fBottomWaypointOffset;

		Vector3f v = pp.m_Pt;
		v.z -= fWpHeight*0.5f;
		v.z -= g_fBottomWaypointOffset;
		return v;
	}

	State::StateStatus FollowPath::Update(float fDt)
	{
		Prof(FollowPath);
		{
			Prof(Update);

			Path::PathPoint pt;
			m_CurrentPath.GetCurrentPt(pt);

			GetClient()->ProcessGotoNode(m_CurrentPath);

			bool b3dDistanceCheck = (pt.m_NavFlags & F_NAV_ELEVATOR) || (pt.m_NavFlags & F_NAV_CLIMB);
			bool b3dMovement = b3dDistanceCheck;
			bool bConsumeWaypoint = true;

			const Vector3f vPos = GetClient()->GetPosition();
			Vector3f vGotoTarget = RawWpPos(pt);

			float fDistanceSq = 0.f;
			const float fDistSq = SquaredLength(vPos, vGotoTarget);
			const float fDistSq2d = SquaredLength2d(vPos, vGotoTarget);
			const float fWpRadiusSq = Mathf::Sqr(pt.m_Radius);

			if(b3dDistanceCheck)
				fDistanceSq = fDistSq;
			else
				fDistanceSq = fDistSq2d;

			// Handle traversal options.
			if(pt.m_NavFlags & F_NAV_DOOR && IGame::GetFrameNumber() & 3)
				GetClient()->PressButton(BOT_BUTTON_USE);
			if(pt.m_NavFlags & F_NAV_SNEAK)
				GetClient()->PressButton(BOT_BUTTON_WALK);
			if(pt.m_NavFlags & F_NAV_ELEVATOR)
			{
				if(!CheckForMover(pt.m_Pt) && 
					!CheckForMover(GetClient()->GetPosition()))
				{
					GetClient()->ResetStuckTime();

					bConsumeWaypoint = false;
					Path::PathPoint lastpt;
					if(m_CurrentPath.GetPreviousPt(lastpt))
					{
						if(lastpt.m_NavFlags & F_NAV_ELEVATOR)
							vGotoTarget = vPos;
						else
							vGotoTarget = lastpt.m_Pt;
					}
					else
						vGotoTarget = vPos;
				}
				else
				{
					// hack: let it succeed if we're in 2d radius and above the wp
					if(fDistSq2d < fWpRadiusSq && vPos.z > vGotoTarget.z)
					{
						fDistanceSq = fWpRadiusSq;
					}
				}
			}
			if(pt.m_NavFlags & F_NAV_TELEPORT)
			{
				Path::PathPoint nextpt;
				if(m_CurrentPath.GetNextPt(nextpt))
				{
					if(nextpt.m_NavFlags & F_NAV_TELEPORT)
					{
						// have we teleported to the destination teleport?
						const float fDistSqTp = SquaredLength2d(vPos, nextpt.m_Pt);
						if(fDistSqTp < Mathf::Sqr(nextpt.m_Radius))
						{
							fDistanceSq = 0.f;
							bConsumeWaypoint = true;
						}
						else
							bConsumeWaypoint = false;
					}
				}
			}

			// Close enough?
			if(bConsumeWaypoint && fDistanceSq <= fWpRadiusSq)
			{
				//////////////////////////////////////////////////////////////////////////
				if(!m_PassThroughState && pt.m_OnPathThrough && pt.m_OnPathThroughParam)
				{
					FINDSTATE(ll,LowLevel,GetRootState());
					State *pPathThrough = ll->FindState(pt.m_OnPathThrough);
					if(pPathThrough)
					{
						String s = Utils::HashToString(pt.m_OnPathThroughParam);
						if(pPathThrough->OnPathThrough(s))
						{
							m_SavedQuery.m_User = 0;
							m_PassThroughState = pt.m_OnPathThrough;
							m_PathThroughPtIndex = m_CurrentPath.GetCurrentPtIndex();
						}
					}
				}
				//////////////////////////////////////////////////////////////////////////

				if(m_CurrentPath.IsEndOfPath())
				{
					if(!m_Query.m_Final)
					{
						bool bSkipLast = false, bFinalDest = false;
						DestinationVector destlist;
						if(m_Query.m_User && m_Query.m_User->GetNextDestination(destlist, bFinalDest, bSkipLast))
						{
							if(Goto(m_Query.m_User, destlist, m_Query.m_MoveMode, bSkipLast, bFinalDest))
							{
								return State_Busy;
							}
						}
					}

					FINDSTATEIF(SteeringSystem, GetRootState(), SetTarget(vPos));
					NotifyUserSuccess();
					m_PathStatus = PathFinished;
					return State_Finished;
				}

				m_CurrentPath.NextPt();
				GetClient()->ResetStuckTime();
			}

			if(GetClient()->GetStuckTime() > 2000)
			{
				obint32 lastStuck = m_LastStuckTime;
				m_LastStuckTime = IGame::GetTime();
				if(IGame::GetTime() > lastStuck + 7000) {
					// find new path to current goal
					if(Repath()) return State_Busy;
				}
				// "pathfailed" error
				FINDSTATEIF(SteeringSystem, GetRootState(), SetTarget(vPos));
				NotifyUserFailed(FollowPathUser::Blocked);
				m_PathStatus = PathFinished;
				return State_Finished;
			}
			
			FINDSTATEIF(SteeringSystem, GetRootState(), 
				SetTarget(vGotoTarget,
				pt.m_Radius,
				m_Query.m_MoveMode,
				b3dMovement));

			//if(pt.m_NavFlags & F_NAV_UNDERWATER)
			//{
				//b3dMovement = true;
				//if(NeedsAir())
			//}

			if(pt.m_NavFlags & F_NAV_JUMP && (fDistanceSq <= Mathf::Sqr(pt.m_Radius)))
				GetClient()->PressButton(BOT_BUTTON_JUMP);
			if(pt.m_NavFlags & F_NAV_JUMPLOW)
				CheckForLowJumps(pt.m_Pt);
			if(pt.m_NavFlags & F_NAV_JUMPGAP)
				CheckForGapJumps(pt.m_Pt);
			if (pt.m_NavFlags & F_NAV_PRONE)
				GetClient()->PressButton(BOT_BUTTON_PRONE);
			else if (pt.m_NavFlags & F_NAV_CROUCH)
				GetClient()->PressButton(BOT_BUTTON_CROUCH);

			m_PtOnPath = m_CurrentPath.FindNearestPtOnPath(vPos, &m_LookAheadPt, 128.f);

			//////////////////////////////////////////////////////////////////////////
			if(!m_OldLadderStyle)
			{
				if(GetClient()->HasEntityFlag(ENT_FLAG_ONLADDER))
				{
					float fHeight = pt.m_Pt.z - vPos.z;
					
					if(m_LadderDirection == 0)
					{
						if(fDistanceSq < fWpRadiusSq * 4)
						{
							// current waypoint has not been consumed yet
							Path::PathPoint nextpt;
							if(m_CurrentPath.GetNextPt(nextpt))
							{
								fHeight = nextpt.m_Pt.z - vPos.z;
							}
						}
					}

					if(Mathf::FAbs(fHeight) > g_fTopWaypointOffset || m_LadderDirection == 0)
					{
						if (fHeight > 0) m_LadderDirection = 1;
						else m_LadderDirection = -1;
					}
					
					if( 2 * fHeight * fHeight > fDistSq2d //climb only if next waypoint is above or below player
						|| (m_LadderDirection > 0 && fHeight > -g_fTopWaypointOffset)
						|| (m_LadderDirection < 0 && -fHeight > g_fTopWaypointOffset))
					{
						Vector3f vEye = GetClient()->GetEyePosition();
						Vector3f vLook = pt.m_Pt - vEye;
						float h, p, r, cl = Mathf::DegToRad(60.f);

						if(m_LadderDirection > 0)
						{
							GetClient()->PressButton(BOT_BUTTON_MOVEUP);
							vLook.z += g_fTopWaypointOffset;
							vLook.ToSpherical(h, p, r);
							p = ClampT(p,-cl,cl);
						}
						else
						{
							GetClient()->PressButton(BOT_BUTTON_MOVEDN);
							vLook.ToSpherical(h, p, r);
							p = cl;
						}

						vLook.FromSpherical(h, p, r);
						m_LookAheadPt = vEye + vLook;
						//Utils::DrawLine(vEye,m_LookAheadPt, (m_LadderDirection > 0) ? COLOR::GREEN : COLOR::RED,3.f);
					}
				}
				else
				{
					m_LadderDirection = 0;
				}
			}
			//////////////////////////////////////////////////////////////////////////

			// Ignore vertical look points unless it's significantly out of our bbox height.
			const Box3f &worldObb = GetClient()->GetWorldBounds();
			const float fTolerance = worldObb.Extent[2];
			if(Mathf::FAbs(m_LookAheadPt.z - worldObb.Center.z) < fTolerance)
			{
				m_LookAheadPt.z = GetClient()->GetEyePosition().z;
			}
		}
		return State_Busy;
	}

	bool FollowPath::CheckForMover(const Vector3f &_pos)
	{
		const float fWpHeight = g_fTopWaypointOffset - g_fBottomWaypointOffset;

		// Use a ray the size of a waypoint offset down by half a waypoint height.

		Vector3f pos1 = _pos.AddZ(g_fTopWaypointOffset - fWpHeight * 0.5f);
		Vector3f pos2 = pos1.AddZ(-fWpHeight);
		
		const bool bMover = InterfaceFuncs::IsMoverAt(pos1,pos2);

		if(DebugDrawingEnabled())
		{
			Utils::DrawLine(pos1,pos2,bMover?COLOR::GREEN:COLOR::RED,0.5f);
		}		
		return bMover;
	}

	void FollowPath::CheckForLowJumps(const Vector3f &_destination)
	{
		Prof(CheckForLowJumps);

		// bot MUST NOT press jump button every frame, otherwise he jumps only once
		obint32 time = IGame::GetTime();
		if(time - m_JumpTime < 100) return;

		// todo: should be game dependant
		const float fStepHeight = GetClient()->GetStepHeight();
		const float fStepBoxWidth = 9.0f; // GAME DEP
		const float fStepBoxHeight = 16.0f; // GAME DEP
		const float fStepRayLength = 48.0f; // GAME DEP

		// Calculate the vector we're wanting to move, this will be
		// the direction of the trace
		Vector3f vMoveVec = _destination - GetClient()->GetPosition();
		vMoveVec.z = 0.0f; // get rid of the vertical component
		vMoveVec.Normalize();

		// Get this entities bounding box to use for traces.
		AABB worldAABB, localAABB;
		EngineFuncs::EntityWorldAABB(GetClient()->GetGameEntity(), worldAABB);

		// Calculate the local AABB
		const Vector3f &vPosition = GetClient()->GetPosition();

		// Adjust the AABB mins to the step height
		localAABB.m_Mins[2] = worldAABB.m_Mins[2] + fStepHeight - vPosition[2];
		// Adjust the AABB mins to the step height + fStepBoxHeight
		localAABB.m_Maxs[2] = localAABB.m_Mins[2] + fStepBoxHeight;
		// Adjust the mins and max for the width of the box
		localAABB.m_Mins[0] = localAABB.m_Mins[1] = -fStepBoxWidth;
		localAABB.m_Maxs[0] = localAABB.m_Maxs[1] = fStepBoxWidth;

		// Trace a line forward from the bot.
		Vector3f vStartPos;
		worldAABB.CenterPoint(vStartPos);

		// The end of the ray is out towards the move direction, fStepRayLength length
		Vector3f vEndPos(vStartPos + (vMoveVec * fStepRayLength));

		// Trace a line start to end to see if it hits anything.
		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStartPos, vEndPos, 
			&localAABB, TR_MASK_SOLID | TR_MASK_PLAYERCLIP, GetClient()->GetGameID(), False);

		bool bHit = false;
		if(tr.m_Fraction != 1.0f)
		{
			bHit = true;
			m_JumpTime = time;
			GetClient()->PressButton(BOT_BUTTON_JUMP);
		}
		
		if(DebugDrawingEnabled())
		{
			// Line Ray
			obColor color = bHit ? COLOR::RED : COLOR::GREEN;
			Utils::DrawLine(vStartPos, vEndPos, color, 2.0f);
			Utils::DrawLine(vStartPos.AddZ(localAABB.m_Mins[2]), vEndPos.AddZ(localAABB.m_Mins[2]), color, 2.0f);
			Utils::DrawLine(vStartPos.AddZ(localAABB.m_Maxs[2]), vEndPos.AddZ(localAABB.m_Maxs[2]), color, 2.0f);
		}
	}

	void FollowPath::CheckForGapJumps(const Vector3f &_destination)
	{
		Prof(CheckForGapJumps);

		// Get this entities bounding box to use for traces.
		AABB aabb;
		EngineFuncs::EntityWorldAABB(GetClient()->GetGameEntity(), aabb);

		// Trace a line down from the bot.
		//const float fStartUpOffset = 5.0f;
		static const float fEndDownOffset = GetClient()->GetGameVar(Client::JumpGapOffset);
		Vector3f vStartPos(Vector3f::ZERO);
		aabb.CenterPoint(vStartPos); 

		Vector3f vEndPos(vStartPos);
		vEndPos.z -= fEndDownOffset;

		// Trace a line downward to see the distance below us.
		//DEBUG_ONLY(Utils::DrawLine(vStartPos, vEndPos, COLOR::RED));

		// Trace a line directly underneath us to try and detect rapid drops in elevation.
		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStartPos, vEndPos, NULL, 
			TR_MASK_SOLID | TR_MASK_PLAYERCLIP, GetClient()->GetGameID(), False);

		Vector3f trEnd(tr.m_Endpos);
		if(m_RayDistance == -1.0f && tr.m_Fraction != 1.0f)
		{
			m_RayDistance = (vStartPos - trEnd).SquaredLength();
		}
		else if(m_RayDistance != -1.0f && tr.m_Fraction == 1.0f)
		{
			GetClient()->PressButton(BOT_BUTTON_JUMP);
		}
		else if(tr.m_Fraction != 1.0f)
		{
			// See if there is any sudden drops. (+20% length in ray maybe?)
			float fNewDist = (vStartPos - trEnd).SquaredLength();
			if(fNewDist > (m_RayDistance + (m_RayDistance * 0.2f)))
			{
				// Try to jump.
				GetClient()->PressButton(BOT_BUTTON_JUMP);
			}
			m_RayDistance = fNewDist;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	Aimer::AimRequest::AimRequest()
	{
		Reset();
	}

	void Aimer::AimRequest::Reset()
	{
		m_Owner = 0;
		m_Priority = Priority::Zero;
		m_AimVector = Vector3f::ZERO;
		m_AimType = WorldPosition;		
		m_AimerUser = NULL;
	}

	Aimer::Aimer() 
		: StateChild("Aimer")
		, m_BestAimOwner(0)
	{
		for(obint32 i = 0; i < MaxAimRequests; ++i)
			m_AimRequests[i].Reset();

		// Initialize default aim request.
		m_AimRequests[0].m_Priority = Priority::Min;
		m_AimRequests[0].m_AimerUser = 0;
		m_AimRequests[0].m_AimVector = Vector3f::UNIT_X;
		m_AimRequests[0].m_AimType = WorldFacing;
		m_AimRequests[0].m_Owner = GetNameHash();
	}

	Aimer::AimRequest *Aimer::FindAimRequest(obuint32 _owner)
	{
		int iOpen = -1;
		for(obint32 i = 0; i < MaxAimRequests; ++i)
		{
			if(m_AimRequests[i].m_Owner == _owner)
			{
				iOpen = i;
				break;
			}
			if(m_AimRequests[i].m_Priority == Priority::Zero)
			{
				if(iOpen == -1)
					iOpen = i;
			}
		}

		if(iOpen != -1)
		{
			return &m_AimRequests[iOpen];
		}
		return NULL;
	}

	bool Aimer::AddAimRequest(Priority::ePriority _prio, AimerUser *_owner, obuint32 _ownername)
	{
		AimRequest *pRequest = FindAimRequest(_ownername);
		if(pRequest)
		{
			pRequest->m_Priority = _prio;
			pRequest->m_Owner = _ownername;
			pRequest->m_AimType = UserCallback;
			pRequest->m_AimerUser = _owner;
			return true;
		}
		return false;
	}

	bool Aimer::AddAimFacingRequest(Priority::ePriority _prio, obuint32 _owner, const Vector3f &_v)
	{
		AimRequest *pRequest = FindAimRequest(_owner);
		if(pRequest)
		{
			pRequest->m_Priority = _prio;
			pRequest->m_Owner = _owner;
			pRequest->m_AimType = WorldFacing;
			pRequest->m_AimerUser = NULL;
			pRequest->m_AimVector = _v;
			return true;
		}
		return false;
	}

	bool Aimer::AddAimPositionRequest(Priority::ePriority _prio, obuint32 _owner, const Vector3f &_v)
	{
		AimRequest *pRequest = FindAimRequest(_owner);
		if(pRequest)
		{
			pRequest->m_Priority = _prio;
			pRequest->m_Owner = _owner;
			pRequest->m_AimType = WorldPosition;
			pRequest->m_AimerUser = NULL;
			pRequest->m_AimVector = _v;
			return true;
		}
		return false;
	}

	bool Aimer::AddAimMoveDirRequest(Priority::ePriority _prio, obuint32 _owner)
	{
		AimRequest *pRequest = FindAimRequest(_owner);
		if(pRequest)
		{
			pRequest->m_Priority = _prio;
			pRequest->m_Owner = _owner;
			pRequest->m_AimType = MoveDirection;
			pRequest->m_AimerUser = NULL;
			return true;
		}
		return false;
	}

	void Aimer::ReleaseAimRequest(obuint32 _owner)
	{
		for(obint32 i = 0; i < MaxAimRequests; ++i)
		{
			if(m_AimRequests[i].m_Owner == _owner)
			{
				m_AimRequests[i].Reset();
				break;
			}
		}
	}

	void Aimer::UpdateAimRequest(obuint32 _owner, const Vector3f &_pos)
	{
		for(obint32 i = 0; i < MaxAimRequests; ++i)
		{
			if(m_AimRequests[i].m_Owner == _owner)
			{
				m_AimRequests[i].m_AimVector = _pos;
				break;
			}
		}
	}

	void Aimer::GetDebugString(StringStr &out)
	{
		out << Utils::HashToString(m_BestAimOwner);
	}

	int Aimer::GetAllRequests(AimRequest *_records, int _max)
	{
		int iNum = 0;
		for(int i = 0; i < MaxAimRequests; ++i)
		{
			if(m_AimRequests[i].m_Priority==Priority::Zero)
				continue;

			_records[iNum++] = m_AimRequests[i];

			if(iNum>=_max-1)
				break;
		}
		return iNum;
	}

	void Aimer::RenderDebug()
	{
		const Aimer::AimRequest *pCurrent = Aimer::GetHighestAimRequest(false);

		for(obint32 i = 1; i < MaxAimRequests; ++i)
		{
			if(m_AimRequests[i].m_Priority > Priority::Zero)
			{
				obColor c = pCurrent==&m_AimRequests[i] ? COLOR::MAGENTA : COLOR::WHITE;
				if(m_AimRequests[i].m_AimType!=WorldFacing)
					Utils::DrawLine(GetClient()->GetEyePosition(),m_AimRequests[i].m_AimVector, c, MIN_RENDER_TIME);
				else
					Utils::DrawLine(GetClient()->GetEyePosition(),GetClient()->GetEyePosition()+GetClient()->GetFacingVector() * 128.f, c, MIN_RENDER_TIME);
			}
		}
	}

	Aimer::AimRequest *Aimer::GetHighestAimRequest(bool _clearontarget)
	{
		int iBestAim = 0;
		for(obint32 i = 1; i < MaxAimRequests; ++i)
		{
			if(m_AimRequests[i].m_Priority > m_AimRequests[iBestAim].m_Priority)
				iBestAim = i;
		}
		return &m_AimRequests[iBestAim];
	}

	void Aimer::OnSpawn()
	{
		m_AimRequests[0].m_AimVector = GetClient()->GetFacingVector();
	}

	void Aimer::Enter()
	{
	}

	void Aimer::Exit()
	{
	}

	State::StateStatus Aimer::Update(float fDt)
	{
		Prof(Aimer);
		{
			Prof(Update);

			AimRequest *curAim = GetHighestAimRequest(true);
			m_BestAimOwner = curAim->m_Owner;
			switch(curAim->m_AimType)
			{
			case WorldPosition:
				{
					GetClient()->TurnTowardPosition(curAim->m_AimVector);
					break;
				}
			case WorldFacing:
				{
					GetClient()->TurnTowardFacing(curAim->m_AimVector);
					break;
				}
			case MoveDirection:
				{
					FINDSTATE(fp, FollowPath, GetRootState());
					if(fp != NULL && fp->IsActive())
					{
						curAim->m_AimVector = fp->GetLookAheadPt();
						GetClient()->TurnTowardPosition(curAim->m_AimVector);
						break;
					}
					FINDSTATE(steer, SteeringSystem, GetParent());
					if(steer)
					{
						curAim->m_AimVector = steer->GetTarget();
						curAim->m_AimVector.z = GetClient()->GetEyePosition().z;
						GetClient()->TurnTowardPosition(curAim->m_AimVector);
					}
					break;
				}
			case UserCallback:
				{
					Prof(UserCallback);
					OBASSERT(curAim->m_AimerUser, "No Aim User");
					if(curAim->m_AimerUser && 
						curAim->m_AimerUser->GetAimPosition(curAim->m_AimVector) &&
						GetClient()->TurnTowardPosition(curAim->m_AimVector))
					{
						curAim->m_AimerUser->OnTarget();
					}
					break;
				}
			}
		}
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	LookAround::LookAround() 
		: StateChild("LookAround")
		, m_NextLookTime(0)
	{
	}

	int LookAround::GetNextLookTime()
	{
		return IGame::GetTime()+Utils::SecondsToMilliseconds(Mathf::IntervalRandom(5.f,15.f));
	}

	void LookAround::OnSpawn()
	{
		m_NextLookTime = GetNextLookTime();
	}

	obReal LookAround::GetPriority()
	{
		if(IGame::GetTime() > m_NextLookTime)
		{
			FINDSTATE(fp,FollowPath,GetParent());
			if(fp)
			{
				Path::PathPoint pp;
				if(fp->IsMoving() && fp->GetCurrentPath().GetCurrentPt(pp) && 
					(pp.m_NavFlags & (F_NAV_CLIMB|F_NAV_DOOR) ))
				{
					m_NextLookTime = GetNextLookTime();
					return 0.f;
				}
			}
			return 1.f;
		}
		return 0.0f;
	}

	void LookAround::Enter()
	{
		//Quaternionf q(Vector3f::UNIT_Z, Mathf::IntervalRandom(-Mathf::PI, Mathf::PI));
		//Vector3f vStartAim = q.Rotate(GetClient()->GetFacingVector());
		FINDSTATE(aim, Aimer, GetParent());
		if(aim)
			aim->AddAimFacingRequest(Priority::VeryLow, GetNameHash(), -GetClient()->GetFacingVector());
	}

	void LookAround::Exit()
	{
		FINDSTATEIF(Aimer, GetParent(), ReleaseAimRequest(GetNameHash()));
		m_NextLookTime = GetNextLookTime();
	}

	State::StateStatus LookAround::Update(float fDt)
	{
		return (GetStateTime() > 1.f) ? State_Finished : State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////

	MotorControl::MotorControl() 
		: StateSimultaneous("MotorControl") 
	{
		AppendState(new FollowPath);
		AppendState(new SteeringSystem);
		AppendState(new Aimer);
		AppendState(new LookAround);
		//AppendState(new ClimbLadder);
		//AppendState(new RideElevator);
	}

	//////////////////////////////////////////////////////////////////////////

	LowLevel::LowLevel() 
		: StateSimultaneous("LowLevel") 
	{
		AppendState(new MotorControl);
		AppendState(new WeaponSystem);
		AppendState(new TargetingSystem);
		AppendState(new SensoryMemory);
		AppendState(new ProximityWatcher);
		//AppendState(new FloodFiller);
	}

	//////////////////////////////////////////////////////////////////////////

	Main::Main() 
		: StateSimultaneous("Main"),
		m_OnSpawnCalled(false)
	{
		AppendState(new LowLevel);
		AppendState(new HighLevel);		
	}

	obReal Main::GetPriority()
	{
		if(IGame::GetGameState() != IGame::GetLastGameState())
			return 0.f;

		//return StateSimultaneous::GetPriority();
		return 1.0f;
	}

	void Main::Enter()
	{
		if(!m_OnSpawnCalled)
			GetRootState()->OnSpawn();
		m_OnSpawnCalled = false;
		GetClient()->SendEvent(MessageHelper(MESSAGE_SPAWN));
	}

	void Main::OnSpawn()
	{
		m_OnSpawnCalled = true;
		State::OnSpawn();
	}

	//////////////////////////////////////////////////////////////////////////

	Dead::Dead() 
		: StateChild("Dead")
		, bForceActivate(true)
	{
	}

	obReal Dead::GetPriority() 
	{
		if(bForceActivate)
		{
			bForceActivate = false;
			return 1.f;
		}
		return !InterfaceFuncs::IsAlive(GetClient()->GetGameEntity()) ? 1.f : 0.f; 
	}

	State::StateStatus Dead::Update(float fDt) 
	{
		if(IGame::GetFrameNumber() & 2)
		{
			GetClient()->PressButton(BOT_BUTTON_RESPAWN);
		}
		GetClient()->SetMovementVector(Vector3f::ZERO);
		return State_Busy; 
	}

	//////////////////////////////////////////////////////////////////////////
	
	Warmup::Warmup() 
		: StateChild("Warmup") 
	{
	}

	obReal Warmup::GetPriority() 
	{
		GameState gs = InterfaceFuncs::GetGameState();		
		return (gs != GAME_STATE_PLAYING && gs != GAME_STATE_SUDDENDEATH) ? 1.f : 0.f;		
	}

	State::StateStatus Warmup::Update(float fDt) 
	{
		// need to do something special here?
		return State_Busy; 
	}

	//////////////////////////////////////////////////////////////////////////

	Root::Root() : StateFirstAvailable("Root") 
	{
		AppendState(new Dead);
		AppendState(new Main);
	}

	//////////////////////////////////////////////////////////////////////////

	ProximityWatcher::ProximityWatcher()
		: StateChild("ProximityWatcher")
	{
		for(int i = 0; i < MaxTriggers; ++i)
		{
			m_Triggers[i].m_OwnerState = 0;
			m_Triggers[i].m_DeleteOnFire = true;
		}
	}
	void ProximityWatcher::RenderDebug()
	{
		for(int i = 0; i < MaxTriggers; ++i)
		{
			if(m_Triggers[i].m_SensoryFilter)
			{
				for(int p = 0; p < m_Triggers[i].m_SensoryFilter->GetNumPositions(); ++p)
				{
					float r = Mathf::Max(m_Triggers[i].m_SensoryFilter->GetMaxDistance(), 10.f);
					Utils::DrawRadius(
						m_Triggers[i].m_SensoryFilter->GetPosition(p),
						r, 
						COLOR::MAGENTA, 
						MIN_RENDER_TIME);
				}
			}
		}
	}
	void ProximityWatcher::AddWatch(obuint32 _owner, FilterPtr _filter, bool _fireonce)
	{
		for(int i = 0; i < MaxTriggers; ++i)
		{
			if(!m_Triggers[i].m_SensoryFilter)
			{
				m_Triggers[i].m_OwnerState = _owner;
				m_Triggers[i].m_SensoryFilter = _filter;
				m_Triggers[i].m_DeleteOnFire = _fireonce;
			}
		}
	}
	void ProximityWatcher::RemoveWatch(obuint32 _owner)
	{
		for(int i = 0; i < MaxTriggers; ++i)
		{
			if(m_Triggers[i].m_SensoryFilter && m_Triggers[i].m_OwnerState == _owner)
			{
				m_Triggers[i].m_OwnerState = 0;
				m_Triggers[i].m_SensoryFilter.reset();
				m_Triggers[i].m_DeleteOnFire = false;
			}
		}
	}

	obReal ProximityWatcher::GetPriority()
	{
		for(int i = 0; i < MaxTriggers; ++i)
		{
			if(m_Triggers[i].m_SensoryFilter)
				return 1.f;
		}
		return 0.f;
	}

	State::StateStatus ProximityWatcher::Update(float fDt)
	{
		SensoryMemory *sensory = GetClient()->GetSensoryMemory();

		for(int i = 0; i < MaxTriggers; ++i)
		{
			if(m_Triggers[i].m_SensoryFilter)
			{
				m_Triggers[i].m_SensoryFilter->Reset();

				// Set up ignore list so we don't get events every frame from
				// entities that are still inside the trigger
				// basically only want the event when they enter.
				//m_Triggers[i].m_SensoryFilter->ResetIgnoreEntity();

				sensory->QueryMemory(*m_Triggers[i].m_SensoryFilter);

				if(m_Triggers[i].m_SensoryFilter->DetectedSomething())
				{
					Event_ProximityTrigger trig;
					trig.m_OwnerState = m_Triggers[i].m_OwnerState;
					trig.m_Entity = m_Triggers[i].m_SensoryFilter->GetBestEntity();
					trig.m_Position = m_Triggers[i].m_SensoryFilter->GetTriggerPosition();
					GetClient()->SendEvent(MessageHelper(MESSAGE_PROXIMITY_TRIGGER, &trig, sizeof(trig)));

					if(m_Triggers[i].m_DeleteOnFire)
					{
						m_Triggers[i].m_OwnerState = 0;
						m_Triggers[i].m_SensoryFilter.reset();
						m_Triggers[i].m_DeleteOnFire = false;
					}
				}
			}
		}
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	//TrackTargetZone::TrackTargetZone()
	//	: m_Radius(0.f)
	//{
	//	Restart(0.f);
	//}
	//void TrackTargetZone::Restart(float _radius)
	//{
	//	m_Radius = _radius;
	//	m_ValidAim = false;

	//	m_LastTarget.Reset();
	//	for(int i = 0; i < MaxTargetZones; ++i)
	//	{
	//		m_TargetZones[i].m_InUse = false;
	//		m_TargetZones[i].m_TargetCount = 0;
	//		m_TargetZones[i].m_Position = Vector3f::ZERO;
	//	}
	//}

	//void TrackTargetZone::UpdateAimPosition()
	//{
	//	int iNumZones = 0;
	//	float fTotalWeight = 0.f;
	//	for(int i = 0; i < MaxTargetZones; ++i)
	//	{
	//		if(m_TargetZones[i].m_InUse)
	//		{
	//			fTotalWeight += (float)m_TargetZones[i].m_TargetCount;
	//			iNumZones++;
	//		}			
	//	}

	//	float fRand = Mathf::IntervalRandom(0.f, fTotalWeight);
	//	for(int i = 0; i < MaxTargetZones; ++i)
	//	{
	//		if(m_TargetZones[i].m_InUse)
	//		{
	//			fRand -= (float)m_TargetZones[i].m_TargetCount;
	//			if(fRand < 0.f)
	//			{
	//				m_AimPosition = m_TargetZones[i].m_Position;
	//				m_ValidAim = true;
	//				return;
	//			}
	//		}
	//	}
	//	m_ValidAim = false;
	//}

	//void TrackTargetZone::RenderDebug()
	//{
	//	for(int i = 0; i < MaxTargetZones; ++i)
	//	{
	//		if(m_TargetZones[i].m_InUse)
	//		{
	//			Utils::DrawRadius(
	//				m_TargetZones[i].m_Position,
	//				m_Radius, 
	//				COLOR::MAGENTA, 
	//				MIN_RENDER_TIME);

	//			Utils::PrintText(
	//				m_TargetZones[i].m_Position,
	//				COLOR::WHITE,
	//				1.f, 
	//				"%d",
	//				m_TargetZones[i].m_TargetCount);
	//		}
	//	}
	//}

	//void TrackTargetZone::Update(Client *_client)
	//{
	//	const MemoryRecord *pTargetRec = _client->GetTargetingSystem()->GetCurrentTargetRecord();
	//	if(pTargetRec != NULL && pTargetRec->GetEntity() != m_LastTarget)
	//	{
	//		for(int i = 0; i < MaxTargetZones; ++i)
	//		{
	//			// if new target, add it to the zone counter
	//			bool bFound = false;
	//			TargetZone *pFreeZone = 0;
	//			for(int z = 0; z < MaxTargetZones; ++z)
	//			{
	//				if(m_TargetZones[z].m_InUse)
	//				{
	//					const float fSqDistance = SquaredLength(m_TargetZones[z].m_Position,pTargetRec->GetLastSensedPosition());

	//					if(fSqDistance < Mathf::Sqr(m_Radius))
	//					{
	//						m_TargetZones[z].m_TargetCount++;
	//						bFound = true;
	//					}
	//				}
	//				else
	//				{
	//					if(!pFreeZone)
	//						pFreeZone = &m_TargetZones[z];
	//				}
	//			}

	//			if(!bFound && pFreeZone)
	//			{
	//				pFreeZone->m_InUse = true;
	//				pFreeZone->m_Position = pTargetRec->GetLastSensedPosition();
	//				pFreeZone->m_TargetCount = 1;
	//			}
	//		}

	//		m_LastTarget = pTargetRec->GetEntity();
	//	}

	//	if(m_LastTarget.IsValid() && !InterfaceFuncs::IsAlive(m_LastTarget))
	//		m_LastTarget.Reset();
	//}

	//////////////////////////////////////////////////////////////////////////

	DeferredCaster::DeferredCaster() 
		: StateChild("DeferredCaster")
		, CastReadPosition(0)
		, CastWritePosition(0)
		, GroupNext(0)
	{
		for(int i = 0; i < MaxCasts; ++i)
		{
			GroupId[i] = 0;
			CastOutputs[i].Reset();
		}
	}

	int DeferredCaster::AddDeferredCasts(const CastInput *_CastIn, int _NumCasts, const char *_UserName)
	{
		int InputIndices[MaxCasts] = {0};

		// verify we have room to add them
		for(int i = 0; i < _NumCasts; ++i)
		{
			const int ix = (CastWritePosition+i)%MaxCasts;

			if(GroupId[ix] != InvalidGroup) // not enough room
				return InvalidGroup;

			InputIndices[i] = ix;
		}

		// copy them over to the internal buffer
		while(GroupNext==0) ++GroupNext;

		for(int i = 0; i < _NumCasts; ++i)
		{
			const int ix = InputIndices[i];
			CastInputs[ix] = _CastIn[i];
			UserName[ix] = _UserName;
			CastOutputs[ix].Done = false;
			GroupId[ix] = GroupNext;
		}
		CastWritePosition = (CastWritePosition+_NumCasts)%MaxCasts;
		return GroupNext++;
	}

	DeferredCaster::Status DeferredCaster::GetDeferredCasts(int _GroupId, CastOutput *_CastOut, int _NumCasts)
	{

		return Pending;
	}

	void DeferredCaster::GetDebugString(StringStr &out)
	{

	}

	void DeferredCaster::RenderDebug()
	{

	}

	obReal DeferredCaster::GetPriority()
	{
		return CastReadPosition != CastWritePosition ? 1.f : 0.f;
	}
	void DeferredCaster::Enter()
	{

	}
	void DeferredCaster::Exit()
	{

	}
	State::StateStatus DeferredCaster::Update(float fDt)
	{
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	void FloodFiller::Connection::Reset()
	{
		Destination = 0;
		Jump = false;
	}

	void FloodFiller::Node::Init(obint16 _X, obint16 _Y, float _Height, bool _Open /* = false */)
	{
		MinOffset.X = MaxOffset.X = _X;
		MinOffset.Y = MaxOffset.Y = _Y;

		Height = _Height;
		Open = _Open;

		OpenNess = 0;

		for(int i = 0; i < DIR_NUM; ++i)
			Connections[i].Reset();

		NearObject = false;
		NearEdge = false;
		Sectorized = false;
		SectorId = 0;
	}

	//////////////////////////////////////////////////////////////////////////

	const float CHARACTER_HEIGHT = 64.f;
	const float CHARACTER_STEPHEIGHT = 18.f;
	const float CHARACTER_JUMPHEIGHT = 60.f;
	const float MIN_COVER_HEIGHT = 40.f;
	const float MAX_COVER_HEIGHT = 60.f;

	FloodFiller::FloodFiller() 
		: StateChild("FloodFiller")
	{
		Reset();
		State = FillDone;
	}

	Vector3f FloodFiller::_GetNodePosition(const Node &_Node)
	{
		Vector3f vNodePos = Start;
		
		const float nX = (float)(_Node.MinOffset.X + _Node.MaxOffset.X) * 0.5f;
		const float nY = (float)(_Node.MinOffset.Y + _Node.MaxOffset.Y) * 0.5f;

		vNodePos.x += ((Radius*2.f) * nX);
		vNodePos.y += ((Radius*2.f) * nY);
		vNodePos.z = _Node.Height;

		return vNodePos;
	}

	FloodFiller::Node *FloodFiller::_NodeExists(obint16 _X, obint16 _Y, float _Height)
	{
		for(int i = 0; i < FreeNode; ++i)
		{
			if(Nodes[i].MinOffset.X >= _X && Nodes[i].MinOffset.Y >= _Y && 
				Nodes[i].MaxOffset.X <= _X && Nodes[i].MaxOffset.Y <= _Y)
			{
				if(Mathf::FAbs(_Height-Nodes[i].Height) < CHARACTER_HEIGHT)
					return &Nodes[i];
			}
		}
		return NULL;
	}

	FloodFiller::Node *FloodFiller::_NextOpenNode()
	{
		for(int i = 0; i < FreeNode; ++i)
		{
			if(Nodes[i].Open)
			{
				return &Nodes[i];					
			}
		}
		return NULL;
	}

	bool FloodFiller::_TestNode(const Node *_Node)
	{
		Vector3f vPos = _GetNodePosition(*_Node);

		obTraceResult tr;
		EngineFuncs::TraceLine(tr, 
			vPos,
			vPos,
			&FloodBlock,
			TR_MASK_FLOODFILL,
			-1, 
			False);

		return (tr.m_Fraction == 1.0f);
	}

	bool FloodFiller::_DropToGround(Node *_Node)
	{
		Vector3f vPos = _GetNodePosition(*_Node);

		const float fDropDistance = 512.f;

		AABB bounds = FloodBlock;
		bounds.m_Maxs[2] = CHARACTER_HEIGHT - CHARACTER_JUMPHEIGHT;

		static float START_OFFSET = CHARACTER_JUMPHEIGHT + 10.f;
		static float END_OFFSET = -fDropDistance;

		obTraceResult tr;
		EngineFuncs::TraceLine(tr, 
			vPos.AddZ(START_OFFSET),
			vPos.AddZ(END_OFFSET),
			&bounds,
			TR_MASK_FLOODFILL,
			-1, 
			False);

		bool bGood = true;

		// adjust node height
		if(tr.m_Fraction < 1.f)
			_Node->Height = tr.m_Endpos[2] + CHARACTER_STEPHEIGHT;

		if(tr.m_StartSolid)
			bGood = false;

		return bGood;
	}

	void FloodFiller::_MakeConnection(Node *_NodeA, Node *_NodeB, Direction _Dir)
	{
		{
			_NodeA->Connections[_Dir].Destination = _NodeB;
			_NodeA->Connections[_Dir].Jump = (_NodeB->Height - _NodeA->Height) >= CHARACTER_STEPHEIGHT;
			
			if((_NodeB->Height - _NodeA->Height) >= MIN_COVER_HEIGHT &&
				(_NodeB->Height - _NodeA->Height) <= MAX_COVER_HEIGHT)
			{
				_NodeA->Connections[_Dir].Cover |= (1<<_Dir);
			}
		}

		{
			//// connect the opposite direction
			//Direction Opposite[DIR_NUM] = 
			//{
			//	DIR_SOUTH,
			//	DIR_WEST,
			//	DIR_NORTH,
			//	DIR_EAST
			//};

			//_Dir = Opposite[_Dir];
			//_NodeB->Connections[_Dir].Destination = _NodeB;
			//_NodeB->Connections[_Dir].Jump = (_NodeA->Height - _NodeB->Height) >= CHARACTER_STEPHEIGHT;
			//_NodeB->Connections[_Dir].Cover = (_NodeA->Height - _NodeB->Height) >= MIN_COVER_HEIGHT &&
			//	(_NodeA->Height - _NodeB->Height) <= MAX_COVER_HEIGHT;
		}
	}

	void FloodFiller::_FillOpenNess(bool _ResetAll)
	{
		//////////////////////////////////////////////////////////////////////////
		for(int i = 0; i < FreeNode; ++i)
		{
			if(_ResetAll)
				Nodes[i].OpenNess = 0;

			if(!Nodes[i].Sectorized)
			{
				if(Nodes[i].NearEdge || Nodes[i].NearObject)
					Nodes[i].OpenNess = 1;

				// bordering another sector also resets
				for(int d = 0; d < DIR_NUM; ++d)
				{
					if(Nodes[i].Connections[d].Destination)
					{
						if(Nodes[i].Connections[d].Destination->Sectorized)
							Nodes[i].OpenNess = 1;
					}
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////

		obuint8 iCurrentDistance = 1;
		bool bKeepGoing = true;
		while(bKeepGoing)
		{
			bKeepGoing = false;

			for(int i = 0; i < FreeNode; ++i)
			{
				if(Nodes[i].OpenNess == iCurrentDistance)
				{
					for(int d = 0; d < DIR_NUM; ++d)
					{
						if(Nodes[i].Connections[d].Destination)
						{
							if(!Nodes[i].Connections[d].Destination->OpenNess)
							{
								bKeepGoing = true;
								Nodes[i].Connections[d].Destination->OpenNess = Nodes[i].OpenNess+1;
							}
						}
					}
				}
			}
			++iCurrentDistance;
		}
	}

	bool FloodFiller::_CanMergeWith(Node *_Node, Node *_WithNode)
	{
		if(!_WithNode)
			return false;

		if(Mathf::FAbs(_WithNode->Height - _Node->Height) >= CHARACTER_STEPHEIGHT)
			return false;

		if(_WithNode->SectorId != 0)
			return false;

		return true;
	}

	void FloodFiller::_MergeSectors()
	{
		_FillOpenNess(true);

		obuint16 NextSectorId = 0;

		for(;;)
		{
			NextSectorId++;
			//////////////////////////////////////////////////////////////////////////
			Node *pLargestNode = 0;
			obuint8 iLargestOpenness = 0;
			for(int i = 0; i < FreeNode; ++i)
			{
				if(!Nodes[i].Sectorized && Nodes[i].OpenNess > iLargestOpenness)
				{
					pLargestNode = &Nodes[i];
					iLargestOpenness = Nodes[i].OpenNess;
				}
			}
			if(!pLargestNode)
				break;
			//////////////////////////////////////////////////////////////////////////
			if(pLargestNode)
			{
				/*Utils::PrintText(
					_GetNodePosition(*pLargestNode) + Vector3f(0,0,64),
					30.f,
					"%d",NextSectorId);*/

				pLargestNode->Sectorized = true;
				pLargestNode->SectorId = NextSectorId;

				Node s;
				s.Init(pLargestNode->MinOffset.X,pLargestNode->MinOffset.Y,pLargestNode->Height,false);

				int ExpansionMask = (1<<DIR_NORTH)|(1<<DIR_EAST)|(1<<DIR_SOUTH)|(1<<DIR_WEST);

				int NumMergeNodes = 0;
				Node *MergeNodes[2048] = {};

				MergeNodes[NumMergeNodes++] = pLargestNode;

				while(ExpansionMask)
				{
					for(int d = DIR_NORTH; d < DIR_NUM; ++d)
					{
						int NumMergeEdge = 0;
						Node *MergeEdge[2048] = {};

						if(ExpansionMask & (1<<d))
						{
							switch(d)
							{
							case DIR_NORTH:
							case DIR_SOUTH:
								{
									//for(int c = 0)

									obint16 y = 0;
									if(d==DIR_NORTH)
										y = s.MaxOffset.Y + 1;
									else
										y = s.MinOffset.Y - 1;
									for(obint16 x = s.MinOffset.X; x <= s.MaxOffset.X; ++x)
									{
										Node *pNext = _NodeExists(x,y,s.Height);
										if(!_CanMergeWith(pLargestNode,pNext))
										{
											/*Node tn;
											tn.MinOffset.X = tn.MaxOffset.X = x;
											tn.MinOffset.Y = tn.MaxOffset.Y = y;
											tn.Height = s.Height;
											Vector3f vMissing = _GetNodePosition(tn);
											Utils::DrawLine(
												vMissing,
												vMissing + Vector3f(0,0,64),
												COLOR::ORANGE,
												20.f);*/

											ExpansionMask &= ~(1<<d);
											break;
										}

										MergeEdge[NumMergeEdge++] = pNext;
									}
									break;
								}
							case DIR_EAST:
							case DIR_WEST:
								{
									obint16 x = 0;
									if(d==DIR_EAST)
										x = s.MaxOffset.X + 1;
									else
										x = s.MinOffset.X - 1;
									for(obint16 y = s.MinOffset.Y; y <= s.MaxOffset.Y; ++y)
									{
										Node *pNext = _NodeExists(x,y,s.Height);
										if(!_CanMergeWith(pLargestNode,pNext))
										{
											/*Node tn;
											tn.MinOffset.X = tn.MaxOffset.X = x;
											tn.MinOffset.Y = tn.MaxOffset.Y = y;
											tn.Height = s.Height;
											Vector3f vMissing = _GetNodePosition(tn);
											Utils::DrawLine(
												vMissing,
												vMissing + Vector3f(0,0,64),
												COLOR::ORANGE,
												20.f);*/


											ExpansionMask &= ~(1<<d);
											break;
										}

										MergeEdge[NumMergeEdge++] = pNext;
									}
									break;
								}
							}
							//////////////////////////////////////////////////////////////////////////
							if(ExpansionMask & (1<<d))
							{
								// adjust the bounds
								switch(d)
								{
								case DIR_NORTH:
									s.MaxOffset.Y++;
									break;
								case DIR_SOUTH:
									s.MinOffset.Y--;
									break;
								case DIR_EAST:
									s.MaxOffset.X++;
									break;
								case DIR_WEST:
									s.MinOffset.X--;
									break;
								}

								// add to the merge list
								for(int m = 0; m < NumMergeEdge; ++m)
								{
									MergeEdge[m]->Sectorized = true;
									MergeEdge[m]->SectorId = NextSectorId;
									MergeNodes[NumMergeNodes++] = MergeEdge[m];
								}
							}
						}
					}
				}

				_FillOpenNess(false);

				// done merging sector, debug render
				Node *EdgeNodes[DIR_NUM][2048] = {};
				int NumEdges[DIR_NUM] = {};

				for(int m = 0; m < NumMergeNodes; ++m)
				{
					for(int d = DIR_NORTH; d < DIR_NUM; ++d)
					{
						bool bHasDirectionConnection = false;
						if(MergeNodes[m]->Connections[d].Destination)
						{
							bHasDirectionConnection = true;
							if(MergeNodes[m]->SectorId != MergeNodes[m]->Connections[d].Destination->SectorId)
							{								
								EdgeNodes[d][NumEdges[d]++] = MergeNodes[m];
							}
						}
						if(!bHasDirectionConnection)
							EdgeNodes[d][NumEdges[d]++] = MergeNodes[m];
					}
				}

				static float RT = 10.f;
				if(RT>0.f)
				{
					for(int d = DIR_NORTH; d < DIR_NUM; ++d)
					{
						Vector3f vP1, vP2;
						float fFurthestPoints = 0.f;

						for(int i1 = 0; i1 < NumEdges[d]; ++i1)
						{
							for(int i2 = 0; i2 < NumEdges[d]; ++i2)
							{
								Vector3f p1 = _GetNodePosition(*EdgeNodes[d][i1]);
								Vector3f p2 = _GetNodePosition(*EdgeNodes[d][i2]);
								float fDist = Length(p1,p2);

								if(fFurthestPoints==0.f)
								{
									vP1 = p1;
									vP2 = p2;
								}

								if(fDist > fFurthestPoints)
								{
									fFurthestPoints = fDist;
									vP1 = p1;
									vP2 = p2;
								}
							}
						}
						if(fFurthestPoints>0.f)
						{
							Utils::DrawLine(
								vP1 + Vector3f(0,0,64),
								vP2 + Vector3f(0,0,64),
								COLOR::BLUE,RT);
						}
					}
				}
			}
		}
	}

	void FloodFiller::Reset()
	{
		for(int i = 0; i < NumSectors; ++i)
		{
			Nodes[i].Init();
		}
		FreeNode = 0;
	}

	void FloodFiller::NextFillState()
	{
		if(State < FillDone)
			State = (FillState)(State+1);
	}

	void FloodFiller::StartFloodFill(const Vector3f &_Start, float _Radius)
	{
		Start = _Start;
		Radius = _Radius;

		FloodBlock = AABB(Vector3f::ZERO);
		FloodBlock.ExpandAxis(0,Radius);
		FloodBlock.ExpandAxis(1,Radius);
		FloodBlock.m_Maxs[2] = CHARACTER_HEIGHT - CHARACTER_STEPHEIGHT;

		State = FillInit;
	}

	void FloodFiller::GetDebugString(StringStr &out)
	{

	}

	void FloodFiller::RenderDebug()
	{
		static int MS_DELAY = 1000;
		static int NEXT_UPDATE = 0;
		if(IGame::GetTime() < NEXT_UPDATE)
			return;

		Vector3f vAim;
		Utils::GetLocalAimPoint(vAim);

		float fNearestSector = 99999999.f;
		const Node *pNearestSector = 0;

		NEXT_UPDATE = IGame::GetTime() + MS_DELAY;

		const float RENDER_TIME = Utils::MillisecondsToSeconds(MS_DELAY);
		for(int i = 0; i < FreeNode; ++i)
		{
			//if(FloodNode[i].OffsetX && FloodNode[i].OffsetY)
			{
				const Node &node = Nodes[i];

				obColor nodeCol = COLOR::GREEN;
				if(node.NearEdge)
					nodeCol = COLOR::MAGENTA;
				if(node.NearObject)
					nodeCol = COLOR::RED;
				
				Vector3f vNodePos = _GetNodePosition(node);

				static bool RenderOpenness = false;
				if (RenderOpenness)
				{
					Utils::PrintText(
						vNodePos,
						COLOR::WHITE,
						RENDER_TIME,
						"%d",
						node.OpenNess);
				}
				

				float fSectorDist = Length(vNodePos,vAim);
				if(fSectorDist < fNearestSector)
				{
					fNearestSector = fSectorDist;
					pNearestSector = &node;
				}

				for(int d = 0; d < DIR_NUM; ++d)
				{
					if(node.Connections[d].Destination)
					{						
						Vector3f vNeighbor = _GetNodePosition(*node.Connections[d].Destination);
						Utils::DrawLine(vNodePos.AddZ(8), vNeighbor, nodeCol, RENDER_TIME);

						const float fDist = Length(vNodePos, vNeighbor);
						if(fDist > 128.f)
						{
							Utils::OutputDebug(kNormal,"bad connection: %f", fDist);
						}
					}

					static bool bDrawCover = false;
					if(bDrawCover)
					{
						if(node.Connections[d].Cover)
						{
							obint16 Offset[DIR_NUM][2] =
							{
								{ 0, 1 },
								{ 1, 0 },
								{ 0, -1 },
								{ -1, 0 },
							};

							Vector3f vNodePos2 = _GetNodePosition(node);
							for(int j = 0; j < DIR_NUM; ++j)
							{
								if(node.Connections[d].Cover & (1<<j))
								{
									Vector3f vNeighborPos = vNodePos2;
									vNeighborPos.x += (Radius*2.f) * Offset[j][0];
									vNeighborPos.y += (Radius*2.f) * Offset[j][1];
									Utils::DrawLine(
										vNodePos2.AddZ(32),
										vNeighborPos.AddZ(32),
										COLOR::BLUE,
										RENDER_TIME);
								}
							}
						}
					}
				}
			}
		}

		if(pNearestSector)
		{
			//int iNumConnections = 0;
			//for(int d = 0; d < DIR_NUM; ++d)
			//	if(pNearestSector->Connections[d].Destination)
			//		++iNumConnections;
			Vector3f vNodePos = _GetNodePosition(*pNearestSector);
			Utils::PrintText(
				vNodePos+Vector3f(0,0,32),
				COLOR::WHITE,
				RENDER_TIME,
				"%d, sid %d",
				pNearestSector-Nodes,
				pNearestSector->Sectorized?pNearestSector->SectorId:-1);
			Utils::DrawLine(vNodePos, vNodePos + Vector3f(0,0,32.f),COLOR::CYAN,RENDER_TIME);
		}
	}

	obReal FloodFiller::GetPriority()
	{
		//return State != FillDone ? 1.f : 0.f;
		return 1.f;
	}
	void FloodFiller::Enter()
	{
		DebugDraw(true);
	}
	void FloodFiller::Exit()
	{	
		//DebugDraw(false);
	}
	State::StateStatus FloodFiller::Update(float fDt)
	{
		//Timer tmr;
		if(Length(Start,GetClient()->GetPosition()) > 256.f)
			StartFloodFill(GetClient()->GetPosition());

		//////////////////////////////////////////////////////////////////////////
		if(State == FillInit)
		{
			Reset();

			// Add a starting flood node
			Nodes[FreeNode++].Init(0,0,Start.z,true);

			_DropToGround(&Nodes[0]);

			NextFillState();
		}
		//////////////////////////////////////////////////////////////////////////
		while(State == FillSearching)
		{
			Node *currentNode = _NextOpenNode();
			if(!currentNode)
			{
				NextFillState();
				return State_Busy;
			}

			currentNode->Open = false;

			// Expand in each direction
			for(int d = DIR_NORTH; d < DIR_NUM; ++d)
			{
				obint16 Offset[DIR_NUM][2] =
				{
					{ 0, 1 },
					{ 1, 0 },
					{ 0, -1 },
					{ -1, 0 },
				};

				obint16 iOffX = currentNode->MinOffset.X + Offset[d][0];
				obint16 iOffY = currentNode->MinOffset.Y + Offset[d][1];
				float fHeight = currentNode->Height;

				Node *pNeighbor = _NodeExists(iOffX, iOffY, fHeight);
				if(pNeighbor)
				{
					_MakeConnection(currentNode,pNeighbor,(Direction)d);
					continue;
				}

				if(FreeNode==NumSectors)
				{
					_MergeSectors();					

					// fail if that didn't free up anything.
					if(FreeNode==NumSectors)
					{
						NextFillState();
						return State_Busy;
					}
				}

				// next node
				pNeighbor = &Nodes[FreeNode];
				pNeighbor->Init(iOffX,iOffY,fHeight,false);

				static float FLOOD_RADIUS = 512.f;
				Vector3f vNodePos = _GetNodePosition(*pNeighbor);
				if(Length(vNodePos, Start) > FLOOD_RADIUS)
				{
					currentNode->NearEdge = true;
					continue;
				}

				if(!_DropToGround(pNeighbor))
				{
					currentNode->NearObject |= true;
					continue;
				}

				currentNode->NearEdge |= (currentNode->Height - pNeighbor->Height) > CHARACTER_JUMPHEIGHT;

				// make the connection.
				_MakeConnection(currentNode,pNeighbor,(Direction)d);

				pNeighbor->Open = true;

				++FreeNode;
			}

			if(currentNode->NearObject || currentNode->NearEdge)
				currentNode->OpenNess = 1;
		}
		//////////////////////////////////////////////////////////////////////////
		if(State==FillOpenNess)
		{
			NextFillState();
			//_FillOpenNess();
		}
		//////////////////////////////////////////////////////////////////////////
		if(State==FillSectorize)
		{
			_MergeSectors();
			NextFillState();
		}
		//////////////////////////////////////////////////////////////////////////
		if(State == FillDone)
		{

		}
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////
}
