#include "PrecompET.h"
#include "ET_BaseStates.h"
#include "ET_FilterClosest.h"
#include "ET_Game.h"
#include "WeaponDatabase.h"

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////

	//PlantMine::PlantMine()
	//	: StateChild("PlantMine")
	//	, FollowPathUser("PlantMine")
	//{
	//	LimitToWeapon().SetFlag(ET_WP_LANDMINE);
	//	SetAlwaysRecieveEvents(true);
	//}

	//void PlantMine::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//MapGoal *PlantMine::GetMapGoalPtr()
	//{
	//	return m_MapGoal.get();
	//}

	//void PlantMine::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//	}
	//}

	//// FollowPathUser functions.
	//bool PlantMine::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 32.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool PlantMine::GetAimPosition(Vector3f &_aimpos)
	//{
	//	static float MINE_PITCH = -75.f;
	//	if(!m_LandMineEntity.IsValid())
	//		_aimpos = GetClient()->GetEyePosition() + Utils::ChangePitch(m_MapGoal->GetFacing(),MINE_PITCH) * 32.f;
	//	else
	//		_aimpos = m_LandMinePosition;
	//	return true;
	//}

	//void PlantMine::OnTarget()
	//{
	//	FINDSTATE(ws, WeaponSystem, GetRootState());
	//	if(ws)
	//	{
	//		if(m_LandMineEntity.IsValid() && ws->CurrentWeaponIs(ET_WP_PLIERS))
	//			ws->FireWeapon();
	//		else if(!m_LandMineEntity.IsValid() && ws->CurrentWeaponIs(ET_WP_LANDMINE))
	//			ws->FireWeapon();
	//	}
	//}

	//obReal PlantMine::GetPriority()
	//{
	//	int currentMines, maxMines;
	//	InterfaceFuncs::NumTeamMines(GetClient(), currentMines, maxMines);
	//	if ( currentMines >= maxMines ) return 0.f;

	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();

	//	if(InterfaceFuncs::IsWeaponCharged(GetClient(), ET_WP_LANDMINE, Primary)) 
	//	{
	//		GoalManager::Query qry(0xf2dffa59 /* PLANTMINE */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qry);
	//		qry.GetBest(m_MapGoal);
	//	}
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void PlantMine::Enter()
	//{
	//	Tracker.InProgress.Set(m_MapGoal, GetClient()->GetTeam());
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));
	//}

	//void PlantMine::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_LandMineEntity.Reset();
	//	m_MapGoal.reset();
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus PlantMine::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	// If it's not destroyable, consider it a success.
	//	if (!InterfaceFuncs::IsDestroyable(GetClient(), m_MapGoal->GetEntity()))
	//	{
	//		return State_Finished;
	//	}

	//	if(m_LandMineEntity.IsValid() && !IGame::IsEntityValid(m_LandMineEntity))
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		GetClient()->ResetStuckTime();

	//		// abort if they have a target
	//		if(GetClient()->GetTargetingSystem()->HasTarget())
	//			return State_Finished;

	//		static float THROW_DISTANCE = 40.f;
	//		static float ARM_DISTANCE = 32.f;

	//		// Have we already thrown out a mine?
	//		if(m_LandMineEntity.IsValid())
	//		{
	//			// Is it armed yet?
	//			if(InterfaceFuncs::GetExplosiveState(GetClient(), m_LandMineEntity) == XPLO_ARMED)
	//			{
	//				BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//				return State_Finished;
	//			}

	//			// Disable avoidance for this frame.
	//			//m_Client->GetSteeringSystem()->SetNoAvoidTime(IGame::GetTime());

	//			// Not armed yet, keep trying.
	//			if(EngineFuncs::EntityPosition(m_LandMineEntity, m_LandMinePosition) && 
	//				EngineFuncs::EntityVelocity(m_LandMineEntity, m_LandMineVelocity))
	//			{
	//				GetClient()->PressButton(BOT_BUTTON_CROUCH);

	//				FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
	//				FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_PLIERS));

	//				// Get into position?
	//				Vector3f vMineToMe = GetClient()->GetPosition() - m_LandMinePosition;
	//				vMineToMe.Flatten();

	//				/*Utils::DrawLine(m_LandMinePosition,
	//					m_LandMinePosition + Normalize(vMineToMe) * ARM_DISTANCE,
	//					COLOR::GREEN,
	//					5.f);*/

	//				GetClient()->GetSteeringSystem()->SetTarget(
	//					m_LandMinePosition + Normalize(vMineToMe) * ARM_DISTANCE);
	//			}
	//			return State_Busy;
	//		}

	//		// Get into position?
	//		Vector3f vMineToMe = GetClient()->GetPosition() - m_MapGoal->GetPosition();
	//		vMineToMe.Flatten();

	//		/*Utils::DrawLine(m_MapGoal->GetPosition(),
	//			m_MapGoal->GetPosition() + Normalize(vMineToMe) * THROW_DISTANCE,
	//			COLOR::GREEN,
	//			5.f);*/

	//		GetClient()->GetSteeringSystem()->SetTarget(
	//			m_MapGoal->GetPosition() + Normalize(vMineToMe) * THROW_DISTANCE);

	//		// keep watching the target position.
	//		// cs: make sure they are within bounds before planting
	//		// TODO: FIXME: check for unreachable.
	//		const float fDistanceToTarget = SquaredLength2d(m_MapGoal->GetPosition(), GetClient()->GetPosition());
	//		if ( fDistanceToTarget <= 1024 ) // 32 units.
	//		{
	//			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
	//			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_LANDMINE));
	//		}
	//		else
	//		{
	//			GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition());
	//		}
	//	}		
	//	return State_Busy;
	//}

	//void PlantMine::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	//{
	//	switch(_message.GetMessageId())
	//	{
	//		HANDLER(ACTION_WEAPON_FIRE)
	//		{
	//			const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
	//			if(m->m_WeaponId == ET_WP_LANDMINE && m->m_Projectile.IsValid())
	//			{
	//				m_LandMineEntity = m->m_Projectile;
	//			}
	//			break;
	//		}			
	//	}
	//}

	//////////////////////////////////////////////////////////////////////////

	//MobileMortar::MobileMortar()
	//	: StateChild("MobileMortar")
	//	, FollowPathUser("MobileMortar")
	//{
	//	LimitToWeapon().SetFlag(ET_WP_MORTAR);
	//}

	//void MobileMortar::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//MapGoal *MobileMortar::GetMapGoalPtr()
	//{
	//	return m_MapGoal.get();
	//}

	//void MobileMortar::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::CYAN,5.f);
	//	}
	//}

	//// FollowPathUser functions.
	//bool MobileMortar::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 32.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool MobileMortar::GetAimPosition(Vector3f &_aimpos)
	//{
	//	FINDSTATE(wp,WeaponSystem,GetRootState());
	//	if(wp != NULL && wp->CurrentWeaponIs(ET_WP_MORTAR_SET))
	//		_aimpos = GetClient()->GetEyePosition() + m_MortarAim[m_CurrentAim] * 512.f;
	//	else
	//		_aimpos = GetClient()->GetEyePosition() + m_MapGoal->GetFacing() * 512.f;
	//	return true;
	//}

	//void MobileMortar::OnTarget()
	//{
	//	FINDSTATE(wp,WeaponSystem,GetRootState());
	//	if(wp)
	//	{
	//		if(!wp->CurrentWeaponIs(ET_WP_MORTAR_SET))
	//		{
	//			if(wp->CurrentWeaponIs(ET_WP_MORTAR))
	//				wp->AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_MORTAR_SET);
	//			else
	//				wp->AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_MORTAR);
	//		}
	//		else
	//		{
	//			if(IGame::GetTime() >= m_FireDelay)
	//			{
	//				wp->FireWeapon();
	//			}
	//		}
	//	}
	//}

	//obReal MobileMortar::GetPriority()
	//{
	//	int curAmmo = 0, maxAmmo = 0;
	//	g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(),ET_WP_MORTAR,Primary,curAmmo,maxAmmo);
	//	if(curAmmo <= 0)
	//		return 0.f;

	//	if(IsActive())
	//		return GetLastPriority();

	//	/*if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ET_WP_MORTAR_SET))
	//		return 0.f;*/

	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0x74708d7a /* MOBILEMORTAR */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	if(!qry.GetBest(m_MapGoal) || !CacheGoalInfo(m_MapGoal))
	//		m_MapGoal.reset();

	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void MobileMortar::Enter()
	//{
	//	Tracker.InProgress.Set(m_MapGoal, GetClient()->GetTeam());
	//	m_FireDelay = 0;
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));
	//}

	//void MobileMortar::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem,GetRootState(),ReleaseWeaponRequest(GetNameHash()));
	//	Tracker.Reset();
	//}

	//State::StateStatus MobileMortar::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	if (!Tracker.InUse && m_MapGoal->GetSlotsOpen(MapGoal::TRACK_INUSE, GetClient()->GetTeam()) < 1)
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		if(m_FireDelay == 0)
	//		{
	//			Tracker.InUse.Set(m_MapGoal, GetClient()->GetTeam());
	//			m_FireDelay = IGame::GetTime() + 2000;
	//		}
	//		else if(IGame::GetTime() - m_FireDelay > 10000)
	//		{
	//			//timeout, try to shoot from current position
	//			OnTarget();
	//		}

	//		// TODO: FIXME: check for unreachable.
	//		const float fDistanceToTarget = SquaredLength2d(m_MapGoal->GetPosition(), GetClient()->GetPosition());
	//		if ( fDistanceToTarget <= 1024 ) // 32 units.
	//		{
	//			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
	//		}
	//		else
	//		{
	//			GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition());
	//		}
	//	}
	//	return State_Busy;
	//}

	//void MobileMortar::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	//{
	//	switch(_message.GetMessageId())
	//	{
	//		HANDLER(ACTION_WEAPON_FIRE)
	//		{
	//			const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
	//			if(m != NULL && m->m_Projectile.IsValid())
	//			{
	//				if(InterfaceFuncs::GetEntityClass(m->m_Projectile) - ET_Game::CLASSEXoffset == ET_CLASSEX_MORTAR)
	//				{
	//					m_CurrentAim = (m_CurrentAim+1)%m_NumMortarAims;
	//					m_FireDelay = IGame::GetTime() + 2000;
	//				}
	//			}
	//			break;
	//		}
	//	}
	//}

	//bool MobileMortar::CacheGoalInfo(MapGoalPtr mg)
	//{
	//	m_CurrentAim = 0;
	//	m_NumMortarAims = 0;

	//	for(int i = 0; i < MaxMortarAims; ++i)
	//	{
	//		if(m_MapGoal->GetProperty(va("MortarAim[%d]",i),m_MortarAim[m_NumMortarAims]))
	//		{
	//			if(!m_MortarAim[m_NumMortarAims].IsZero())
	//			{
	//				++m_NumMortarAims;
	//			}		
	//		}
	//	}
	//	std::random_shuffle(m_MortarAim, m_MortarAim + m_NumMortarAims);
	//	return m_NumMortarAims > 0;
	//}

	//////////////////////////////////////////////////////////////////////////

	//CallArtillery::CallArtillery()
	//	: StateChild("CallArtillery")
	//	, FollowPathUser("CallArtillery")
	//	, m_MinCampTime(1.f)
	//	, m_MaxCampTime(2.f)
	//	, m_Stance(StanceStand)
	//	, m_ExpireTime(0)
	//{
	//	LimitToWeapon().SetFlag(ET_WP_BINOCULARS);
	//}

	//void CallArtillery::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//MapGoal *CallArtillery::GetMapGoalPtr()
	//{
	//	return m_MapGoal.get();
	//}

	//void CallArtillery::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		if(m_MapGoal)
	//		{
	//			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(),COLOR::ORANGE, 5.f);
	//			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//		}
	//	}
	//}

	//// FollowPathUser functions.
	//bool CallArtillery::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool CallArtillery::GetAimPosition(Vector3f &_aimpos)
	//{
	//	if(m_MapGoalTarget)
	//	{
	//		_aimpos = m_MapGoalTarget->GetPosition();
	//	}
	//	else if(m_TargetEntity.IsValid())
	//	{
	//		const float LOOKAHEAD_TIME = 5.0f;
	//		const MemoryRecord *mr = GetClient()->GetSensoryMemory()->GetMemoryRecord(m_TargetEntity);
	//		if(mr)
	//		{
	//			const Vector3f vVehicleOffset = Vector3f(0.0f, 0.0f, 32.0f);
	//			_aimpos = vVehicleOffset + mr->m_TargetInfo.m_LastPosition + 
	//				mr->m_TargetInfo.m_LastVelocity * LOOKAHEAD_TIME;
	//			m_FireTime = IGame::GetTime() + 1000;
	//		}
	//	}
	//	return true;
	//}

	//void CallArtillery::OnTarget()
	//{
	//	FINDSTATE(ws, WeaponSystem, GetRootState());
	//	if(ws != NULL && ws->CurrentWeaponIs(ET_WP_BINOCULARS))
	//	{
	//		GetClient()->PressButton(BOT_BUTTON_AIM);
	//		if(m_FireTime < IGame::GetTime())
	//		{
	//			if(GetClient()->HasEntityFlag(ENT_FLAG_ZOOMING))
	//			{
	//				ws->FireWeapon();
	//			}
	//		}
	//	}
	//}

	//obReal CallArtillery::GetPriority()
	//{
	//	if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ET_WP_BINOCULARS, Primary))
	//		return 0.f;

	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();
	//	m_MapGoalTarget.reset();

	//	//////////////////////////////////////////////////////////////////////////
	//	ET_FilterClosest filter(GetClient(), AiState::SensoryMemory::EntEnemy);
	//	filter.AddCategory(ENT_CAT_SHOOTABLE);
	//	filter.AddClass(ET_CLASSEX_VEHICLE + ET_Game::CLASSEXoffset);
	//	filter.AddClass(ET_CLASSEX_VEHICLE_HVY + ET_Game::CLASSEXoffset);
	//	FINDSTATE(sensory,SensoryMemory,GetRootState());
	//	if(sensory)
	//	{
	//		sensory->QueryMemory(filter);
	//		
	//		if(filter.GetBestEntity().IsValid())
	//		{
	//			GoalManager::Query q(0xa411a092 /* MOVER */);
	//			q.Ent(filter.GetBestEntity());
	//			GoalManager::GetInstance()->GetGoals(q);
	//			
	//			if(!q.m_List.empty())
	//			{
	//				m_TargetEntity = filter.GetBestEntity();
	//				return q.m_List.front()->GetDefaultPriority();
	//			}				
	//		}
	//	}
	//	//////////////////////////////////////////////////////////////////////////
	//	{
	//		GoalManager::Query qry(0x312ad48d /* CALLARTILLERY */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qry);
	//		for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//		{
	//			if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//				continue;

	//			if (qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) < 1)
	//				continue;

	//			m_MapGoal = qry.m_List[i];
	//			break;
	//		}
	//	}
	//	if(!m_MapGoal)
	//		return 0.f;
	//	//////////////////////////////////////////////////////////////////////////
	//	Vector3f vSource = m_MapGoal->GetPosition();
	//	//vSource = vSource + Vector3(0,0,60);
	//	vSource.z = vSource.z + 60;
	//	//////////////////////////////////////////////////////////////////////////
	//	if(!m_MapGoalTarget)
	//	{
	//		GoalManager::Query qry(0xb708821b /* ARTILLERY_S */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qry);
	//		for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//		{
	//			if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//				continue;

	//			if (qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) < 1)
	//				continue;

	//			if(!Client::HasLineOfSightTo(vSource,qry.m_List[i]->GetPosition()))
	//				continue;

	//			m_MapGoalTarget = qry.m_List[i];
	//			break;
	//		}
	//	}
	//	//////////////////////////////////////////////////////////////////////////
	//	if(!m_MapGoalTarget)
	//	{
	//		GoalManager::Query qry(0xac0870ca /* ARTILLERY_D */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qry);
	//		for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//		{
	//			if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//				continue;

	//			if (qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) < 1)
	//				continue;

	//			if(!Client::HasLineOfSightTo(vSource,qry.m_List[i]->GetPosition()))
	//				continue;

	//			m_MapGoalTarget = qry.m_List[i];
	//			break;
	//		}
	//	}
	//	return m_MapGoalTarget ? m_MapGoalTarget->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void CallArtillery::Enter()
	//{
	//	if(m_MapGoalTarget && m_MapGoalTarget->GetGoalType()=="ARTILLERY_D")
	//		m_FireTime = std::numeric_limits<int>::max();
	//	else
	//		m_FireTime = 0;			

	//	m_Fired = false;
	//	m_ExpireTime = 0;
	//	
	//	if(m_MapGoal)
	//	{
	//		m_MapGoal->GetProperty("Stance",m_Stance);
	//		m_MapGoal->GetProperty("MinCampTime",m_MinCampTime);
	//		m_MapGoal->GetProperty("MaxCampTime",m_MaxCampTime);
	//	}

	//	if(m_MapGoalTarget)
	//	{
	//		if(!m_WatchFilter)
	//		{
	//			m_WatchFilter.reset(new ET_FilterClosest(GetClient(), AiState::SensoryMemory::EntEnemy));
	//		}
	//		
	//		m_WatchFilter->AddClass(FilterSensory::ANYPLAYERCLASS);
	//		m_WatchFilter->AddPosition(m_MapGoalTarget->GetPosition());
	//		m_WatchFilter->SetMaxDistance(100.f);
	//		FINDSTATEIF(ProximityWatcher, GetRootState(), AddWatch(GetNameHash(),m_WatchFilter,false));
	//	}
	//	Tracker.InProgress.Set(m_MapGoal, GetClient()->GetTeam());
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));
	//}

	//void CallArtillery::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();

	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(),ReleaseWeaponRequest(GetNameHash()));
	//	FINDSTATEIF(ProximityWatcher, GetRootState(), RemoveWatch(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus CallArtillery::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		if(m_MapGoal)
	//			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(m_MapGoal && !m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;
	//	if(m_MapGoalTarget && !m_MapGoalTarget->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		if(m_Fired)
	//			return State_Finished;

	//		if(m_ExpireTime==0)
	//		{
	//			m_ExpireTime = IGame::GetTime()+Mathf::IntervalRandomInt(m_MinCampTime.GetMs(),m_MaxCampTime.GetMs());
	//			Tracker.InUse.Set(m_MapGoal, GetClient()->GetTeam());
	//		}
	//		else if(IGame::GetTime() > m_ExpireTime)
	//		{
	//			// Delay it from being used for a while.
	//			if(m_MapGoal)
	//				BlackboardDelay(30.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}

	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
	//		FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_BINOCULARS));

	//		if (m_Stance==StanceProne)
	//			GetClient()->PressButton(BOT_BUTTON_PRONE);
	//		else if (m_Stance==StanceCrouch)
	//			GetClient()->PressButton(BOT_BUTTON_CROUCH);
	//	}
	//	return State_Busy;
	//}

	//void CallArtillery::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	//{
	//	switch(_message.GetMessageId())
	//	{
	//		HANDLER(MESSAGE_PROXIMITY_TRIGGER)
	//		{
	//			const AiState::Event_ProximityTrigger *m = _message.Get<AiState::Event_ProximityTrigger>();
	//			if(m->m_OwnerState == GetNameHash())
	//			{
	//				m_FireTime = IGame::GetTime();// + 1000;
	//			}
	//			break;
	//		}
	//		HANDLER(ACTION_WEAPON_FIRE)
	//		{
	//			const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
	//			if(m != NULL && m->m_WeaponId == ET_WP_BINOCULARS)
	//			{
	//				m_Fired = true;
	//			}
	//			break;
	//		}
	//	}
	//}
	
	//////////////////////////////////////////////////////////////////////////

	//UseCabinet::UseCabinet()
	//	: StateChild("UseCabinet")
	//	, FollowPathUser("UseCabinet")
	//	, m_Range(1250.f)
	//{
	//}

	//void UseCabinet::GetDebugString(StringStr &out)
	//{
	//	out << 
	//		(m_MapGoal ? m_MapGoal->GetName() : "") <<
	//		" (" << m_HealthPriority << "," << m_AmmoPriority << ")";
	//}

	//MapGoal *UseCabinet::GetMapGoalPtr()
	//{
	//	return m_MapGoal.get();
	//}

	//void UseCabinet::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		if(m_MapGoal)
	//		{
	//			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(),COLOR::ORANGE, 5.f);
	//			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//		}
	//	}
	//}

	//// FollowPathUser functions.
	//bool UseCabinet::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	///*bool UseHealthCabinet::GetAimPosition(Vector3f &_aimpos)
	//{
	//	if(m_MapGoalTarget)
	//	{
	//		_aimpos = m_MapGoalTarget->GetPosition();
	//	}
	//	else if(m_TargetEntity.IsValid())
	//	{
	//		const float LOOKAHEAD_TIME = 5.0f;
	//		const MemoryRecord *mr = GetClient()->GetSensoryMemory()->GetMemoryRecord(m_TargetEntity);
	//		if(mr)
	//		{
	//			const Vector3f vVehicleOffset = Vector3f(0.0f, 0.0f, 32.0f);
	//			_aimpos = vVehicleOffset + mr->m_TargetInfo.m_LastPosition + 
	//				mr->m_TargetInfo.m_LastVelocity * LOOKAHEAD_TIME;
	//			m_FireTime = IGame::GetTime() + 1000;
	//		}
	//	}
	//	return true;
	//}

	//void UseHealthCabinet::OnTarget()
	//{
	//	FINDSTATE(ws, WeaponSystem, GetRootState());
	//	if(ws && ws->CurrentWeaponIs(ET_WP_BINOCULARS))
	//	{
	//		GetClient()->PressButton(BOT_BUTTON_AIM);
	//		if(m_FireTime < IGame::GetTime())
	//		{
	//			if(GetClient()->HasEntityFlag(ENT_FLAG_ZOOMING))
	//			{
	//				ws->FireWeapon();
	//			}
	//		}
	//	}
	//}*/

	//obReal UseCabinet::GetPriority()
	//{
	//	if(IsActive())
	//		return GetLastPriority();

	//	m_HealthPriority = 0.f;
	//	m_AmmoPriority = 0.f;

	//	const float fHealthPc = GetClient()->GetHealthPercent();
	//	const float fHealthPcInv = ClampT(1.f - fHealthPc,0.f,1.f);
	//	const float fHealthPriority = fHealthPcInv;
	//	const float fAmmoPriority = GetClient()->GetWeaponSystem()->GetMostDesiredAmmo(m_AmmoType,m_GetAmmoAmount);

	//	m_MapGoal.reset();

	//	//MapGoalPtr mgHealth;
	//	//MapGoalPtr mgAmmo;

	//	// don't bother if desire for health is too low
	//	if(fHealthPriority > 0.7f)
	//	{
	//		GoalManager::Query qryHealth(0x63217fa7 /* HEALTHCAB */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qryHealth);
	//		float fDistToHealthCabinet;

	//		MapGoalList::iterator mIt = qryHealth.m_List.begin();
	//		for(; mIt != qryHealth.m_List.end(); )
	//		{
	//			if(BlackboardIsDelayed((*mIt)->GetSerialNum()) ||
	//				(*mIt)->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) < 1)
	//			{
	//				mIt = qryHealth.m_List.erase(mIt);
	//				continue;
	//			}

	//			fDistToHealthCabinet = SquaredLength2d((*mIt)->GetPosition(), GetClient()->GetPosition());
	//			(*mIt)->GetProperty("Range",m_Range);
	//			if ( fDistToHealthCabinet > m_Range * m_Range )
	//			{
	//				mIt = qryHealth.m_List.erase(mIt);
	//				continue;
	//			}

	//			++mIt;
	//		}

	//		if(!qryHealth.m_List.empty())
	//		{
	//			//mgAmmo = mgHealth;
	//			m_Query = qryHealth;
	//			m_CabinetType = Health;
	//			m_HealthPriority = fHealthPriority;
	//		}
	//	}
	//	
	//	// don't bother if we need health more than ammo or if desire for ammo is too low
	//	if(fAmmoPriority > fHealthPriority && fAmmoPriority > 0.7f && m_AmmoType != -1)
	//	{
	//		GoalManager::Query qryAmmo(0x52ad0a47 /* AMMOCAB */, GetClient());
	//		GoalManager::GetInstance()->GetGoals(qryAmmo);
	//		float fDistToAmmoCabinet;

	//		MapGoalList::iterator mIt = qryAmmo.m_List.begin();
	//		for(; mIt != qryAmmo.m_List.end(); )
	//		{
	//			if(BlackboardIsDelayed((*mIt)->GetSerialNum()) ||
	//				(*mIt)->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) < 1)
	//			{
	//				mIt = qryAmmo.m_List.erase(mIt);
	//				continue;
	//			}

	//			fDistToAmmoCabinet = SquaredLength2d((*mIt)->GetPosition(), GetClient()->GetPosition());
	//			(*mIt)->GetProperty("Range",m_Range);
	//			if ( fDistToAmmoCabinet > m_Range * m_Range )
	//			{
	//				mIt = qryAmmo.m_List.erase(mIt);
	//				continue;
	//			}

	//			++mIt;
	//		}

	//		if(!qryAmmo.m_List.empty())
	//		{
	//			//m_MapGoal = mgAmmo;
	//			m_Query = qryAmmo;
	//			m_CabinetType = Ammo;
	//			m_AmmoPriority = fAmmoPriority;
	//		}
	//	}

	//	return m_HealthPriority > m_AmmoPriority ? m_HealthPriority : m_AmmoPriority;
	//}

	//void UseCabinet::Enter()
	//{	
	//	m_UseTime = 0;
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this,m_Query.m_List,Run,true));
	//	if(!DidPathFail())
	//	{
	//		m_MapGoal = m_Query.m_List[GetDestinationIndex()];
	//		Tracker.InProgress.Set(m_MapGoal, GetClient()->GetTeam());
	//	}
	//}

	//void UseCabinet::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();

	//	//FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(),ReleaseWeaponRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus UseCabinet::Update(float fDt)
	//{
	//	if(!m_MapGoal || DidPathFail())
	//	{
	//		// delay all of em.
	//		for(obuint32 i = 0; i < m_Query.m_List.size(); ++i)
	//			BlackboardDelay(10.f, m_Query.m_List[i]->GetSerialNum());
	//		return State_Finished;
	//	}
	//	OBASSERT(m_MapGoal,"No Map Goal!");

	//	ET_CabinetData cData;
	//	if(InterfaceFuncs::GetCabinetData(m_MapGoal->GetEntity(),cData) && 
	//		cData.m_CurrentAmount == 0)
	//	{
	//		BlackboardDelay(20.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(m_MapGoal && !m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	switch(m_CabinetType)
	//	{
	//	case Health:
	//		if(GetClient()->GetHealthPercent() >= 1.0)
	//			return State_Finished;
	//		break;
	//	case Ammo:
	//		{
	//			if(m_GetAmmoAmount>0)
	//			{
	//				if(GetClient()->GetWeaponSystem()->HasAmmo(m_AmmoType,Primary,m_GetAmmoAmount))
	//					return State_Finished;
	//			}
	//			else
	//			{
	//				int AmmoType = 0, GetAmmo = 0;
	//				if(GetClient()->GetWeaponSystem()->GetMostDesiredAmmo(AmmoType,GetAmmo) <= 0.f)
	//					return State_Finished;
	//			}
	//			break;
	//		}
	//	}

	//	if(DidPathSucceed())
	//	{
	//		GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetWorldUsePoint());

	//		if(m_UseTime==0)
	//		{
	//			m_UseTime = IGame::GetTime();
	//		}
	//		else if(IGame::GetTime() - m_UseTime > 15000)
	//		{
	//			//timeout
	//			BlackboardDelay(30.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}
	//	}
	//	return State_Busy;
	//}

	//////////////////////////////////////////////////////////////////////////

	//BuildConstruction::BuildConstruction()
	//	: StateChild("BuildConstruction")
	//	, FollowPathUser("BuildConstruction")
	//	, m_AdjustedPosition(false)
	//	, m_Crouch(true)
	//	, m_Prone(false)
	//	, m_IgnoreTargets(false)
	//{
	//	LimitToWeapon().SetFlag(ET_WP_PLIERS);
	//}

	//void BuildConstruction::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void BuildConstruction::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineAABB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_ConstructionPos,COLOR::GREEN,5.f);
	//	}
	//}

	//// FollowPathUser functions.
	//bool BuildConstruction::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//	{
	//		_skiplastpt = true;
	//		_final = true;
	//	}
	//	return true;
	//}

	//// AimerUser functions.
	//bool BuildConstruction::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_ConstructionPos;
	//	return true;
	//}

	//void BuildConstruction::OnTarget()
	//{
	//	FINDSTATE(ws, WeaponSystem, GetRootState());
	//	if(ws && ws->CurrentWeaponIs(ET_WP_PLIERS))
	//		ws->FireWeapon();

	//	//InterfaceFuncs::GetCurrentCursorHint
	//}

	//obReal BuildConstruction::GetPriority()
	//{
	//	if(IsActive())
	//		return GetLastPriority();
	//	
	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0xc39bf2a3 /* BUILD */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//	{
	//		if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//			continue;

	//		if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS) < 1)
	//			continue;

	//		ConstructableState cState = InterfaceFuncs::GetConstructableState(GetClient(),qry.m_List[i]->GetEntity());
	//		if(cState == CONST_UNBUILT)
	//		{
	//			m_MapGoal = qry.m_List[i];
	//			break;
	//		}
	//	}
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void BuildConstruction::Enter()
	//{
	//	m_AdjustedPosition = false;
	//	m_MapGoal->GetWorldBounds().CenterPoint(m_ConstructionPos);
	//	Tracker.InProgress = m_MapGoal;

	//	m_MapGoal->GetProperty("Crouch",m_Crouch);
	//	m_MapGoal->GetProperty("Prone",m_Prone);
	//	m_MapGoal->GetProperty("IgnoreTargets",m_IgnoreTargets);
	//	
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));
	//}

	//void BuildConstruction::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();
	//	Tracker.Reset();

	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));
	//}

	//State::StateStatus BuildConstruction::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	//////////////////////////////////////////////////////////////////////////
	//	// Check the construction status
	//	ConstructableState cState = InterfaceFuncs::GetConstructableState(GetClient(),m_MapGoal->GetEntity());
	//	switch(cState)
	//	{
	//	case CONST_INVALID: // Invalid constructable, fail
	//		return State_Finished;
	//	case CONST_BUILT:	// It's built, consider it a success.
	//		return State_Finished;
	//	case CONST_UNBUILT:
	//	case CONST_BROKEN:
	//		// This is what we want.
	//		break;
	//	}
	//	//////////////////////////////////////////////////////////////////////////

	//	if(DidPathSucceed())
	//	{
	//		const float fDistanceToConstruction = SquaredLength2d(m_ConstructionPos, GetClient()->GetPosition());
	//		if (fDistanceToConstruction > 4096.0f)
	//		{
	//			// check for badly waypointed maps
	//			if (!m_AdjustedPosition)
	//			{
	//				// use our z value because some trigger entities may be below the ground
	//				Vector3f checkPos(m_ConstructionPos.x, m_ConstructionPos.y, GetClient()->GetEyePosition().z);

	//				obTraceResult tr;
	//				EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(),checkPos, 
	//					NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), GetClient()->GetGameID(), True);

	//				if (tr.m_Fraction != 1.0f && !tr.m_HitEntity.IsValid())
	//				{
	//					m_MapGoal->SetDeleteMe(true);
	//					return State_Finished;
	//				}

	//				// do a trace to adjust position
	//				EngineFuncs::TraceLine(tr, 
	//					GetClient()->GetEyePosition(),
	//					m_ConstructionPos, 
	//					NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, False);

	//				if (tr.m_Fraction != 1.0f)
	//				{
	//					m_ConstructionPos = *(Vector3f*)tr.m_Endpos;
	//				}

	//				m_AdjustedPosition = true;
	//			}
	//		}
	//		else
	//		{
	//			if(m_Prone)
	//				GetClient()->PressButton(BOT_BUTTON_PRONE);
	//			else if(m_Crouch)
	//				GetClient()->PressButton(BOT_BUTTON_CROUCH);

	//			Priority::ePriority pri = m_IgnoreTargets ? Priority::High : Priority::Medium;
	//			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(pri,this,GetNameHash()));
	//			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(pri, GetNameHash(), ET_WP_PLIERS));
	//		}
	//		GetClient()->GetSteeringSystem()->SetTarget(m_ConstructionPos, 64.f);
	//	}
	//	return State_Busy;
	//}

	//////////////////////////////////////////////////////////////////////////

	//PlantExplosive::PlantExplosive()
	//	: StateChild("PlantExplosive")
	//	, FollowPathUser("PlantExplosive")
	//	, m_IgnoreTargets(false)
	//{
	//	LimitToWeapon().SetFlag(ET_WP_DYNAMITE);
	//	LimitToWeapon().SetFlag(ET_WP_SATCHEL);
	//	LimitToWeapon().SetFlag(ET_WP_SATCHEL_DET);
	//	SetAlwaysRecieveEvents(true);
	//}

	//void PlantExplosive::GetDebugString(StringStr &out)
	//{
	//	switch(m_GoalState)
	//	{
	//	case LAY_EXPLOSIVE:
	//		out << "Lay Explosive ";
	//		break;
	//	case ARM_EXPLOSIVE:
	//		out << "Arm Explosive ";
	//		break;
	//	case RUNAWAY:
	//		out << "Run! ";
	//		break;
	//	case DETONATE_EXPLOSIVE:
	//		out << "Det Explosive ";
	//		break;
	//	default:
	//		break;
	//	}
	//	out << std::endl << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void PlantExplosive::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineAABB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//	}
	//}

	//// FollowPathUser functions.
	//bool PlantExplosive::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool PlantExplosive::GetAimPosition(Vector3f &_aimpos)
	//{
	//	switch(m_GoalState)
	//	{
	//	case LAY_EXPLOSIVE:
	//		_aimpos = m_TargetPosition;
	//		break;
	//	case ARM_EXPLOSIVE:
	//		_aimpos = m_ExplosivePosition;
	//		break;
	//	case RUNAWAY:
	//	case DETONATE_EXPLOSIVE:
	//	default:
	//		OBASSERT(0, "Invalid Aim State");
	//		return false;
	//	}
	//	return true;
	//}

	//void PlantExplosive::OnTarget()
	//{
	//	FINDSTATE(ws, WeaponSystem, GetRootState());
	//	if(ws)
	//	{
	//		if(m_GoalState == LAY_EXPLOSIVE)
	//		{
	//			if(ws->CurrentWeaponIs(ET_WP_DYNAMITE) || ws->CurrentWeaponIs(ET_WP_SATCHEL))
	//				ws->FireWeapon();
	//		}
	//		else if(m_GoalState == ARM_EXPLOSIVE)
	//		{
	//			if(ws->CurrentWeaponIs(ET_WP_PLIERS))
	//				ws->FireWeapon();
	//		}
	//	}
	//}

	//obReal PlantExplosive::GetPriority()
	//{
	//	if(IsActive())
	//		return GetLastPriority();

	//	ExplosiveTargetType		targetType = XPLO_TYPE_DYNAMITE;
	//	ET_Weapon				weaponType = ET_WP_DYNAMITE;
	//	switch(GetClient()->GetClass())
	//	{
	//	case ET_CLASS_ENGINEER:
	//		{
	//			weaponType = ET_WP_DYNAMITE;
	//			targetType = XPLO_TYPE_DYNAMITE;
	//			break;
	//		}
	//	case ET_CLASS_COVERTOPS:
	//		{
	//			weaponType = ET_WP_SATCHEL;
	//			targetType = XPLO_TYPE_SATCHEL;
	//			break;
	//		}
	//	default:
	//		OBASSERT(0, "Wrong Class with Evaluator_PlantExplosive");
	//		return 0.0;
	//	}

	//	m_MapGoal.reset();

	//	if(InterfaceFuncs::IsWeaponCharged(GetClient(), weaponType, Primary)) 
	//	{
	//		{
	//			GoalManager::Query qry(0xbbcae592 /* PLANT */, GetClient());
	//			GoalManager::GetInstance()->GetGoals(qry);
	//			for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//			{
	//				if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//					continue;

	//				/*if(!(qry.m_List[i]->GetGoalState() & targetType))
	//					continue;*/

	//				if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS) < 1)
	//					continue;

	//				if(!_IsGoalAchievable(qry.m_List[i], weaponType))
	//					continue;

	//				ConstructableState cState = InterfaceFuncs::IsDestroyable(GetClient(), qry.m_List[i]->GetEntity());
	//				if(cState == CONST_DESTROYABLE)
	//				{
	//					m_MapGoal = qry.m_List[i];
	//					break;
	//				}
	//			}
	//		}

	//		if(!m_MapGoal)
	//		{
	//			GoalManager::Query qry(0xa411a092 /* MOVER */, GetClient());
	//			GoalManager::GetInstance()->GetGoals(qry);
	//			for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//			{
	//				if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//					continue;

	//				if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS) < 1)
	//					continue;
	//				
	//				if(!(qry.m_List[i]->GetGoalState() & targetType))
	//					continue;

	//				m_MapGoal = qry.m_List[i];
	//				break;
	//			}
	//		}
	//	}
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void PlantExplosive::Enter()
	//{		
	//	m_MapGoal->GetProperty("IgnoreTargets",m_IgnoreTargets);
	//	
	//	// set position to base of construction
	//	AABB aabb = m_MapGoal->GetWorldBounds();
	//	aabb.CenterPoint(m_ExplosivePosition);
	//	m_ExplosivePosition.z = aabb.m_Mins[2];
	//	m_TargetPosition = m_ExplosivePosition;
	//	m_AdjustedPosition = false;
	//	m_GoalState = LAY_EXPLOSIVE;
	//	Tracker.InProgress = m_MapGoal;
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));
	//}

	//void PlantExplosive::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_ExplosiveEntity.Reset();

	//	m_MapGoal.reset();
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus PlantExplosive::Update(float fDt)
	//{
	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	// If it's not destroyable, consider it a success.
	//	if(!InterfaceFuncs::IsDestroyable(GetClient(), m_MapGoal->GetEntity()))
	//		return State_Finished;

	//	if(m_ExplosiveEntity.IsValid() && !IGame::IsEntityValid(m_ExplosiveEntity))
	//		return State_Finished;

	//	if(DidPathSucceed() || m_GoalState==DETONATE_EXPLOSIVE)
	//	{
	//		switch(GetClient()->GetClass())
	//		{
	//		case ET_CLASS_ENGINEER:
	//			return _UpdateDynamite();
	//		case ET_CLASS_COVERTOPS:
	//			return _UpdateSatchel();
	//		default:
	//			OBASSERT(0, "Wrong Class in PlantExplosive");
	//		}
	//	}

	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}
	//	return State_Busy;
	//}

	//State::StateStatus PlantExplosive::_UpdateDynamite()
	//{
	//	Priority::ePriority pri = m_IgnoreTargets ? Priority::High : Priority::Medium;

	//	switch(m_GoalState)
	//	{
	//	case LAY_EXPLOSIVE:
	//		{
	//			if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ET_WP_DYNAMITE, Primary))
	//				return State_Finished;

	//			/*if(m_Client->IsDebugEnabled(BOT_DEBUG_GOALS))
	//			{
	//				Utils::OutlineAABB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 1.f);

	//				Vector3f vCenter;
	//				m_MapGoal->GetWorldBounds().CenterPoint(vCenter);
	//				Utils::DrawLine(m_Client->GetPosition(), vCenter, COLOR::GREEN, 1.0f);
	//				Utils::DrawLine(m_Client->GetPosition(), m_MapGoal->GetPosition(), COLOR::RED, 1.0f);
	//			}*/

	//			const float fDistanceToTarget = SquaredLength2d(m_TargetPosition, GetClient()->GetPosition());
	//			if (fDistanceToTarget > Mathf::Sqr(100.0f))
	//			{
	//				// check for badly waypointed maps
	//				if (!m_AdjustedPosition)
	//				{
	//					m_AdjustedPosition = true;

	//					// use our z value because some trigger entities may be below the ground
	//					Vector3f vCheckPos(m_TargetPosition.x, m_TargetPosition.y, GetClient()->GetEyePosition().z);
	//					/*if(m_Client->IsDebugEnabled(BOT_DEBUG_GOALS))
	//					{
	//						Utils::DrawLine(GetClient()->GetEyePosition(), vCheckPos, COLOR::GREEN, 2.0f);
	//					}*/

	//					obTraceResult tr;
	//					EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(), vCheckPos, 
	//						NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), GetClient()->GetGameID(), True);
	//					if (tr.m_Fraction != 1.0f && !tr.m_HitEntity.IsValid())
	//					{
	//						//m_TargetEntity->SetDeleteMe(true);
	//						return State_Finished;
	//					}

	//					// do a trace to adjust position
	//					EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(), m_TargetPosition, 
	//						NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, False);
	//					if (tr.m_Fraction != 1.0f)
	//					{
	//						m_TargetPosition = Vector3f(tr.m_Endpos);
	//					}
	//				}

	//				// Move toward it.
	//				GetClient()->GetSteeringSystem()->SetTarget(m_TargetPosition);
	//			}
	//			else
	//			{
	//				// We're within range, so let's start laying.
	//				GetClient()->ResetStuckTime();
	//				FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(pri,this,GetNameHash()));
	//				FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(pri, GetNameHash(), ET_WP_DYNAMITE));
	//			}
	//			break;
	//		}
	//	case ARM_EXPLOSIVE:
	//		{
	//			GetClient()->PressButton(BOT_BUTTON_CROUCH);

	//			if(InterfaceFuncs::GetExplosiveState(GetClient(), m_ExplosiveEntity) == XPLO_ARMED)
	//			{
	//				BlackboardDelay(30.f, m_MapGoal->GetSerialNum());
	//				return State_Finished;
	//			}

	//			// Disable avoidance for this frame.
	//			GetClient()->GetSteeringSystem()->SetNoAvoidTime(IGame::GetTime());

	//			// update dynamite position
	//			EngineFuncs::EntityPosition(m_ExplosiveEntity, m_ExplosivePosition);

	//			// move a little bit close if dyno too far away
	//			if ((m_ExplosivePosition - GetClient()->GetPosition()).SquaredLength() > 2500.0f)
	//			{
	//				GetClient()->GetSteeringSystem()->SetTarget(m_ExplosivePosition);
	//			}
	//			else
	//			{
	//				GetClient()->ResetStuckTime();
	//				FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(pri,this,GetNameHash()));
	//				FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(pri, GetNameHash(), ET_WP_PLIERS));
	//			}
	//			break;
	//		}
	//	default:
	//		// keep processing
	//		break;
	//	}
	//	return State_Busy;
	//}

	//State::StateStatus PlantExplosive::_UpdateSatchel()
	//{
	//	Priority::ePriority pri = m_IgnoreTargets ? Priority::High : Priority::Medium;

	//	switch(m_GoalState)
	//	{
	//	case LAY_EXPLOSIVE:
	//		{
	//			if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ET_WP_SATCHEL, Primary))
	//				return State_Finished;

	//			const float fDistanceToTarget = SquaredLength2d(m_TargetPosition, GetClient()->GetPosition());
	//			if (fDistanceToTarget > 10000.0f)
	//			{
	//				// Move toward it.
	//				//GetClient()->GetSteeringSystem()->SetTarget(m_TargetPosition);

	//				// check for badly waypointed maps
	//				if (!m_AdjustedPosition)
	//				{
	//					m_AdjustedPosition = true;

	//					// use our z value because some trigger entities may be below the ground
	//					Vector3f vCheckPos(m_TargetPosition.x, m_TargetPosition.y, GetClient()->GetEyePosition().z);
	//					/*if(m_Client->IsDebugEnabled(BOT_DEBUG_GOALS))
	//					{
	//						Utils::DrawLine(m_Client->GetEyePosition(), vCheckPos, COLOR::GREEN, 2.0f);
	//					}*/

	//					obTraceResult tr;
	//					EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(), vCheckPos, 
	//						NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), GetClient()->GetGameID(), True);
	//					if (tr.m_Fraction != 1.0f && !tr.m_HitEntity.IsValid())
	//					{
	//						AABB aabb, mapaabb;
	//						EngineFuncs::EntityWorldAABB(m_MapGoal->GetEntity(), aabb);
	//						//g_EngineFuncs->GetMapExtents(mapaabb);

	//						//m_TargetEntity->SetDeleteMe(true);
	//						return State_Finished;
	//					}

	//					// do a trace to adjust position
	//					EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(), m_TargetPosition, 
	//						NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, False);
	//					if (tr.m_Fraction != 1.0f)
	//					{
	//						m_TargetPosition = Vector3f(tr.m_Endpos);
	//					}
	//				}
	//			}
	//			else
	//			{
	//				// We're within range, so let's start laying.
	//				GetClient()->ResetStuckTime();
	//				FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(pri,this,GetNameHash()));
	//				FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(pri, GetNameHash(), ET_WP_SATCHEL));
	//			}
	//			break;
	//		}
	//	case ARM_EXPLOSIVE:
	//	case RUNAWAY:
	//		{
	//			OBASSERT(m_ExplosiveEntity.IsValid(), "No Explosive Entity!");			

	//			// Generate a random goal.
	//			FINDSTATEIF(FollowPath,GetRootState(),GotoRandomPt(this));
	//			m_GoalState = DETONATE_EXPLOSIVE;
	//			break;
	//		}		
	//	case DETONATE_EXPLOSIVE:
	//		{
	//			if(DidPathFail() || DidPathSucceed())
	//			{
	//				FINDSTATEIF(FollowPath,GetRootState(),GotoRandomPt(this));
	//				break;
	//			}

	//			// Are we far enough away to blow it up?
	//			const float SATCHEL_DET_DISTANCE = 350.0f;
	//			const bool BLOW_TARGET_OR_NOT = true;

	//			Vector3f vSatchelPos;
	//			if(EngineFuncs::EntityPosition(m_ExplosiveEntity, vSatchelPos))
	//			{
	//				if(Length(GetClient()->GetPosition(), vSatchelPos) >= SATCHEL_DET_DISTANCE)
	//				{
	//					// Do we need this? perhaps make this flag scriptable?
	//					if(BLOW_TARGET_OR_NOT || !GetClient()->GetTargetingSystem()->HasTarget())
	//					{
	//						FINDSTATE(ws,WeaponSystem,GetRootState());
	//						if(ws)
	//						{
	//							ws->AddWeaponRequest(Priority::Override, GetNameHash(), ET_WP_SATCHEL_DET);
	//							if(ws->CurrentWeaponIs(ET_WP_SATCHEL_DET))
	//								ws->FireWeapon();
	//						}

	//						ExplosiveState eState = InterfaceFuncs::GetExplosiveState(GetClient(), m_ExplosiveEntity);							
	//						if(eState == XPLO_INVALID)
	//							return State_Finished;
	//					}
	//				}
	//			}
	//			break;
	//		}
	//	}
	//	return State_Busy;
	//}

	//void PlantExplosive::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	//{
	//	if(IsActive())
	//	{
	//		Priority::ePriority pri = m_IgnoreTargets ? Priority::High : Priority::Medium;
	//		switch(_message.GetMessageId())
	//		{
	//			HANDLER(ACTION_WEAPON_FIRE)
	//			{
	//				const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
	//				if(m->m_WeaponId == ET_WP_DYNAMITE && m->m_Projectile.IsValid())
	//				{
	//					SetLastPriority(2.f); // up the priority so we aren't interrupted(to arm/det)
	//					m_ExplosiveEntity = m->m_Projectile;
	//					m_GoalState = ARM_EXPLOSIVE;
	//					SetLastPriority(2.f); // up the priority so we aren't interrupted(to arm/det)

	//					FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(pri, GetNameHash(), ET_WP_PLIERS));
	//				}
	//				else if(m->m_WeaponId == ET_WP_SATCHEL && m->m_Projectile.IsValid())
	//				{
	//					SetLastPriority(2.f); // up the priority so we aren't interrupted(to arm/det)
	//					m_ExplosiveEntity = m->m_Projectile;
	//					m_GoalState = RUNAWAY;
	//					SetLastPriority(2.f); // up the priority so we aren't interrupted(to arm/det)

	//					FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//					FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));
	//				}
	//				break;
	//			}			
	//		}
	//	}
	//}

	//bool PlantExplosive::_IsGoalAchievable(MapGoalPtr _g, int _weaponId)
	//{
	//	static bool bDynaUnderWater = false;
	//	static bool bSatchelUnderWater = false;
	//	static bool bDynaCached = false;
	//	static bool bSatchelCached = false;
	//	if(!bDynaCached)
	//	{
	//		WeaponPtr w = g_WeaponDatabase.GetWeapon(ET_WP_DYNAMITE);
	//		if(w)
	//		{
	//			bDynaUnderWater = w->GetFireMode(Primary).CheckFlag(Weapon::Waterproof);
	//			bDynaCached = true;
	//		}
	//	}
	//	if(!bSatchelCached)
	//	{
	//		WeaponPtr w = g_WeaponDatabase.GetWeapon(ET_WP_SATCHEL);
	//		if(w)
	//		{
	//			bSatchelUnderWater = w->GetFireMode(Primary).CheckFlag(Weapon::Waterproof);
	//			bSatchelCached = true;
	//		}
	//	}
	//	return _weaponId==ET_WP_DYNAMITE ? bDynaUnderWater : bSatchelUnderWater;
	//}

	//////////////////////////////////////////////////////////////////////////

	//Flamethrower::Flamethrower() 
	//	: StateChild("Flamethrower")
	//	, FollowPathUser("Flamethrower")
	//	, m_MinCampTime(2000)
	//	, m_MaxCampTime(8000)
	//	, m_ExpireTime(0)
	//	, m_Stance(StanceStand)
	//{
	//	LimitToWeapon().SetFlag(ET_WP_FLAMETHROWER);
	//}

	//void Flamethrower::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void Flamethrower::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		m_TargetZone.RenderDebug();
	//	}
	//}

	//bool Flamethrower::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	_final = !m_MapGoal->RouteTo(GetClient(), _desination);
	//	return true;
	//}

	//bool Flamethrower::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_AimPosition;
	//	return true;
	//}

	//void Flamethrower::OnTarget()
	//{
	//	FINDSTATE(wp,WeaponSystem,GetRootState());

	//	if (wp)
	//	{
	//		if(!wp->CurrentWeaponIs(ET_WP_FLAMETHROWER))
	//		{
	//			wp->AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_FLAMETHROWER);
	//		}
	//	}
	//}

	//obReal Flamethrower::GetPriority()
	//{
	//	int curAmmo = 0, maxAmmo = 0;
	//	g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(),ET_WP_FLAMETHROWER,Primary,curAmmo,maxAmmo);
	//	if(curAmmo <= 0)
	//		return 0.f;

	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0x86584d00 /* FLAME */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	qry.GetBest(m_MapGoal);

	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void Flamethrower::Enter()
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

	//void Flamethrower::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem,GetRootState(),ReleaseWeaponRequest(GetNameHash()));
	//	Tracker.Reset();
	//}

	//State::StateStatus Flamethrower::Update(float fDt)
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
	//			m_ExpireTime = IGame::GetTime()+Mathf::IntervalRandomInt(m_MinCampTime,m_MaxCampTime);
	//			Tracker.InUse = m_MapGoal;
	//		}
	//		else if(IGame::GetTime() > m_ExpireTime)
	//		{
	//			// Delay it from being used for a while.
	//			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}

	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Low,this,GetNameHash()));

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

	////////////////////////////////////////////////////////////////////////////

	//Panzer::Panzer() 
	//	: StateChild("Panzer")
	//	, FollowPathUser("Panzer")
	//	, m_MinCampTime(2000)
	//	, m_MaxCampTime(8000)
	//	, m_ExpireTime(0)
	//	, m_Stance(StanceStand)
	//{
	//	LimitToWeapon().SetFlag(ET_WP_PANZERFAUST);
	//}

	//void Panzer::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void Panzer::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		m_TargetZone.RenderDebug();
	//	}
	//}

	//bool Panzer::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	_final = !m_MapGoal->RouteTo(GetClient(), _desination);
	//	return true;
	//}

	//bool Panzer::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_AimPosition;
	//	return true;
	//}

	//void Panzer::OnTarget()
	//{
	//	FINDSTATE(wp,WeaponSystem,GetRootState());

	//	if (wp)
	//	{
	//		if(!wp->CurrentWeaponIs(ET_WP_PANZERFAUST))
	//		{
	//			wp->AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_PANZERFAUST);
	//		}
	//	}
	//}

	//obReal Panzer::GetPriority()
	//{
	//	int curAmmo = 0, maxAmmo = 0;
	//	g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(),ET_WP_PANZERFAUST,Primary,curAmmo,maxAmmo);
	//	if(curAmmo <= 0)
	//		return 0.f;

	//	if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ET_WP_PANZERFAUST, Primary))
	//		return 0.f;

	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0x731f6315 /* PANZER */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	qry.GetBest(m_MapGoal);

	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void Panzer::Enter()
	//{
	//	m_AimPosition = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;

	//	Tracker.InProgress = m_MapGoal;

	//	m_TargetZone.Restart(256.f);
	//	m_ExpireTime = 0;
	//	
	//	m_MapGoal->GetProperty("Stance",m_Stance);
	//	m_MapGoal->GetProperty("MinCampTime",m_MinCampTime);
	//	m_MapGoal->GetProperty("MaxCampTime",m_MaxCampTime);

	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));
	//}

	//void Panzer::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem,GetRootState(),ReleaseWeaponRequest(GetNameHash()));
	//	Tracker.Reset();
	//}

	//State::StateStatus Panzer::Update(float fDt)
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
	//			m_ExpireTime = IGame::GetTime()+Mathf::IntervalRandomInt(m_MinCampTime,m_MaxCampTime);
	//			Tracker.InUse = m_MapGoal;
	//		}
	//		else if(IGame::GetTime() > m_ExpireTime)
	//		{
	//			// Delay it from being used for a while.
	//			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}

	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Low,this,GetNameHash()));

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

	///////////////////////////////////////////////////////////////////////////

	//MountMg42::MountMg42()
	//	: StateChild("MountMg42")
	//	, FollowPathUser("MountMg42")
	//	, m_MinCampTime(6.f)
	//	, m_MaxCampTime(10.f)
	//	, m_ExpireTime(0)
	//	, m_StartTime(0)
	//	, m_ScanDirection(SCAN_DEFAULT)
	//	, m_NextScanTime(0)
	//	, m_IgnoreTargets(false)
	//{
	//}

	//void MountMg42::GetDebugString(StringStr &out)
	//{
	//	if(IsActive())
	//	{
	//		if(!GetClient()->HasEntityFlag(ET_ENT_FLAG_MOUNTED))
	//			out << "Mounting ";
	//		else
	//		{
	//			switch(m_ScanDirection)
	//			{
	//			case SCAN_DEFAULT:
	//				out << "Scan Facing ";
	//				break;
	//			case SCAN_MIDDLE:
	//				out << "Scan Middle ";
	//				break;
	//			case SCAN_LEFT:
	//				out << "Scan Left ";
	//				break;
	//			case SCAN_RIGHT:
	//				out << "Scan Right ";
	//				break;
	//			}
	//		}

	//		if(m_MapGoal)
	//			out << m_MapGoal->GetName();
	//	}
	//}

	//void MountMg42::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//		m_TargetZone.RenderDebug();
	//	}
	//}

	//// FollowPathUser functions.
	//bool MountMg42::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool MountMg42::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_AimPoint;
	//	return true;
	//}

	//void MountMg42::OnTarget()
	//{
	//	if(!GetClient()->HasEntityFlag(ET_ENT_FLAG_MOUNTED) && (IGame::GetFrameNumber()&1))
	//		GetClient()->PressButton(BOT_BUTTON_USE);
	//}

	//obReal MountMg42::GetPriority()
	//{
	//	if(IsActive())
	//		return GetLastPriority();
	//	
	//	BitFlag64 entFlags;

	//	GoalManager::Query qry(0xe1a2b09c /* MOUNTMG42 */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//	{
	//		if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//			continue;

	//		if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS) < 1)
	//			continue;

	//		GameEntity gunOwner = InterfaceFuncs::GetMountedPlayerOnMG42(GetClient(), qry.m_List[i]->GetEntity());
	//		int gunHealth = InterfaceFuncs::GetGunHealth(GetClient(), qry.m_List[i]->GetEntity());
	//		bool bBroken = InterfaceFuncs::IsMountableGunRepairable(GetClient(), qry.m_List[i]->GetEntity());

	//		if(bBroken)
	//			continue;

	//		if(!InterfaceFuncs::GetEntityFlags(qry.m_List[i]->GetEntity(), entFlags) ||
	//			!entFlags.CheckFlag(ET_ENT_FLAG_ISMOUNTABLE))
	//			continue;

	//		// Make sure nobody has it mounted.
	//		if((!gunOwner.IsValid() || !GetClient()->IsAllied(gunOwner)) && (gunHealth > 0))
	//		{
	//			m_MapGoal = qry.m_List[i];
	//			break;
	//		}
	//	}
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void MountMg42::Enter()
	//{
	//	m_MapGoal->GetProperty("MinCampTime",m_MinCampTime);
	//	m_MapGoal->GetProperty("MaxCampTime",m_MaxCampTime);
	//	m_MapGoal->GetProperty("IgnoreTargets",m_IgnoreTargets);

	//	m_ExpireTime = 0;

	//	m_ScanDirection = SCAN_MIDDLE;
	//	m_NextScanTime = IGame::GetTime() + (int)Mathf::IntervalRandom(2000.0f, 7000.0f);
	//	m_AimPoint = m_MapGoal->GetPosition();
	//	m_MG42Position = m_AimPoint;
	//	m_ScanLeft = Vector3f::ZERO;
	//	m_ScanRight = Vector3f::ZERO;
	//	m_GotGunProperties = false;
	//	Tracker.InProgress = m_MapGoal;
	//	m_TargetZone.Restart(256.f);
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));
	//}

	//void MountMg42::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));

	//	if(GetClient()->HasEntityFlag(ET_ENT_FLAG_MOUNTED))
	//		GetClient()->PressButton(BOT_BUTTON_USE);

	//	Tracker.Reset();
	//}

	//State::StateStatus MountMg42::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	//////////////////////////////////////////////////////////////////////////
	//	// Only fail if a friendly player is on this gun or gun has been destroyed in the meantime
	//	//int gunHealth = InterfaceFuncs::GetGunHealth(m_Client, m_MG42Goal->GetEntity());
	//	GameEntity mounter = InterfaceFuncs::GetMountedPlayerOnMG42(GetClient(), m_MapGoal->GetEntity());
	//	if(InterfaceFuncs::IsMountableGunRepairable(GetClient(), m_MapGoal->GetEntity()) ||
	//		(mounter.IsValid() && (mounter != GetClient()->GetGameEntity()) && GetClient()->IsAllied(mounter)))
	//	{
	//		return State_Finished;
	//	}
	//	//////////////////////////////////////////////////////////////////////////

	//	if(DidPathSucceed())
	//	{
	//		GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition());

	//		const bool bMounted = GetClient()->HasEntityFlag(ET_ENT_FLAG_MOUNTED);
	//		const int currentTime = IGame::GetTime();

	//		Priority::ePriority pri = m_IgnoreTargets && !bMounted ? Priority::High : Priority::Low;
	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(pri,this,GetNameHash()));

	//		// Only hang around here for a certain amount of time. 3 seconds max if they don't get mounted.
	//		if(m_ExpireTime==0)
	//		{
	//			m_ExpireTime = currentTime+Mathf::IntervalRandomInt(m_MinCampTime.GetMs(),m_MaxCampTime.GetMs());
	//			m_StartTime = currentTime;
	//			Tracker.InUse = m_MapGoal;
	//		}
	//		else if(currentTime > m_ExpireTime || (!bMounted && currentTime - m_StartTime > 3000))
	//		{
	//			// Delay it from being used for a while.
	//			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}

	//		if(bMounted)
	//		{
	//			m_TargetZone.Update(GetClient());

	//			if(!m_GotGunProperties)
	//			{
	//				m_GotGunProperties = true;
	//				_GetMG42Properties();
	//				m_AimPoint = m_MapGoal->GetPosition() + m_GunCenterArc * 512.f;
	//			}

	//			if(m_NextScanTime < IGame::GetTime())
	//			{
	//				m_NextScanTime = IGame::GetTime() + (int)Mathf::IntervalRandom(2000.0f, 7000.0f);
	//				m_ScanDirection = (int)Mathf::IntervalRandom(0.0f, (float)NUM_SCAN_TYPES);

	//				// we're mounted, so lets look around mid view.
	//				m_TargetZone.UpdateAimPosition();
	//			}

	//			if(m_TargetZone.HasAim())
	//				m_ScanDirection = SCAN_ZONES;

	//			switch(m_ScanDirection)
	//			{
	//			case SCAN_DEFAULT:
	//				if(m_MapGoal->GetFacing() != Vector3f::ZERO)
	//				{
	//					m_AimPoint = m_MG42Position + m_MapGoal->GetFacing() * 1024.f;
	//					break;
	//				}
	//			case SCAN_MIDDLE:
	//				{
	//					m_AimPoint = m_MG42Position + m_GunCenterArc * 1024.f;
	//					break;
	//				}
	//			case SCAN_LEFT:
	//				if(m_ScanLeft != Vector3f::ZERO)
	//				{
	//					m_AimPoint = m_MG42Position + m_ScanLeft * 1024.f;
	//					break;
	//				}						
	//			case SCAN_RIGHT:
	//				if(m_ScanRight != Vector3f::ZERO)
	//				{
	//					m_AimPoint = m_MG42Position + m_ScanRight * 1024.f;
	//					break;
	//				}
	//			case SCAN_ZONES:
	//				{
	//					m_AimPoint = m_TargetZone.GetAimPosition();
	//					break;
	//				}
	//			default:
	//				break;
	//			}
	//		}
	//	}
	//	return State_Busy;
	//}

	//bool MountMg42::_GetMG42Properties()
	//{
	//	ET_MG42Info data;
	//	if(!InterfaceFuncs::GetMg42Properties(GetClient(), data))
	//		return false;

	//	m_GunCenterArc = Vector3f(data.m_CenterFacing);

	//	m_MinHorizontalArc = data.m_MinHorizontalArc;
	//	m_MaxHorizontalArc = data.m_MaxHorizontalArc;
	//	m_MinVerticalArc = data.m_MinVerticalArc;
	//	m_MaxVerticalArc = data.m_MaxVerticalArc;

	//	// Calculate the planes for the MG42

	//	/*Matrix3f planeMatrices[4];
	//	planeMatrices[0].FromEulerAnglesXYZ(m_MinHorizontalArc, 0.0f, 0.0f);
	//	planeMatrices[1].FromEulerAnglesXYZ(m_MaxHorizontalArc, 0.0f, 0.0f);
	//	planeMatrices[2].FromEulerAnglesXYZ(0.0f, m_MinHorizontalArc, 0.0f);
	//	planeMatrices[3].FromEulerAnglesXYZ(0.0f, m_MaxHorizontalArc, 0.0f);

	//	m_GunArcPlanes[0] = Plane3f(m_GunCenterArc * planeMatrices[0], m_MG42Position);
	//	m_GunArcPlanes[1] = Plane3f(m_GunCenterArc * planeMatrices[1], m_MG42Position);
	//	m_GunArcPlanes[2] = Plane3f(m_GunCenterArc * planeMatrices[2], m_MG42Position);
	//	m_GunArcPlanes[3] = Plane3f(m_GunCenterArc * planeMatrices[3], m_MG42Position);*/

	//	const float fScanPc = 0.4f;

	//	Quaternionf ql;
	//	ql.FromAxisAngle(Vector3f::UNIT_Z, Mathf::DegToRad(m_MinHorizontalArc * fScanPc));
	//	m_ScanLeft = ql.Rotate(m_GunCenterArc);

	//	Quaternionf qr;
	//	qr.FromAxisAngle(Vector3f::UNIT_Z, Mathf::DegToRad(m_MaxHorizontalArc * fScanPc));
	//	m_ScanRight = qr.Rotate(m_GunCenterArc);

	//	return true;
	//}

	//////////////////////////////////////////////////////////////////////////

	//MobileMg42::MobileMg42()
	//	: StateChild("MobileMg42")
	//	, FollowPathUser("MobileMg42")
	//	, m_MinCampTime(6.f)
	//	, m_MaxCampTime(10.f)
	//	, m_ExpireTime(0)
	//{
	//	LimitToWeapon().SetFlag(ET_WP_MOBILE_MG42);
	//}

	//void MobileMg42::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void MobileMg42::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::CYAN,5.f);
	//		m_TargetZone.RenderDebug();
	//	}
	//}

	//// FollowPathUser functions.
	//bool MobileMg42::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool MobileMg42::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_AimPosition;
	//	return true;
	//}

	//void MobileMg42::OnTarget()
	//{
	//	FINDSTATE(wp,WeaponSystem,GetRootState());
	//	if(wp)
	//	{
	//		if(!wp->CurrentWeaponIs(ET_WP_MOBILE_MG42_SET) && GetClient()->HasEntityFlag(ENT_FLAG_PRONED))
	//		{
	//			if(wp->CurrentWeaponIs(ET_WP_MOBILE_MG42))
	//				wp->AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_MOBILE_MG42_SET);
	//			else
	//				wp->AddWeaponRequest(Priority::Medium, GetNameHash(), ET_WP_MOBILE_MG42);
	//		}
	//		else
	//		{
	//			wp->SetOverrideWeaponID(ET_WP_MOBILE_MG42_SET);
	//		}
	//	}
	//}

	//obReal MobileMg42::GetPriority()
	//{
	//	FINDSTATE(at,AttackTarget,GetRootState());
	//	if(at != NULL && at->IsActive() && at->TargetExceedsWeaponLimits())
	//		return 0.f;

	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0xbe8488ed /* MOBILEMG42 */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	qry.GetBest(m_MapGoal);
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void MobileMg42::Enter()
	//{
	//	m_MapGoal->GetProperty("MinCampTime",m_MinCampTime);
	//	m_MapGoal->GetProperty("MaxCampTime",m_MaxCampTime);
	//	m_ExpireTime = 0;

	//	m_TargetZone.Restart(256.f);
	//	Tracker.InProgress = m_MapGoal;
	//	m_AimPosition = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));
	//}

	//void MobileMg42::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();

	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATE(wp,WeaponSystem,GetRootState());
	//	if(wp)
	//	{
	//		wp->ReleaseWeaponRequest(GetNameHash());
	//		wp->ClearOverrideWeaponID();
	//	}
	//	Tracker.Reset();
	//}

	//State::StateStatus MobileMg42::Update(float fDt)
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

	//		GetClient()->PressButton(BOT_BUTTON_PRONE);

	//		Priority::ePriority pri = /*m_IgnoreTargets ? Priority::High :*/ Priority::Low;
	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(pri,this,GetNameHash()));

	//		// Handle aim limitations.
	//		FINDSTATE(wp,WeaponSystem,GetRootState());
	//		if(wp != NULL && wp->CurrentWeaponIs(ET_WP_MOBILE_MG42_SET))
	//		{
	//			m_TargetZone.Update(GetClient());
	//			if(m_TargetZone.HasAim())
	//				m_AimPosition = m_TargetZone.GetAimPosition();
	//		}
	//	}
	//	return State_Busy;
	//}

	//////////////////////////////////////////////////////////////////////////

	//RepairMg42::RepairMg42()
	//	: StateChild("RepairMg42")
	//	, FollowPathUser("RepairMg42")
	//	, m_IgnoreTargets(false)
	//{
	//	LimitToWeapon().SetFlag(ET_WP_PLIERS);
	//}

	//void RepairMg42::GetDebugString(StringStr &out)
	//{
	//	if(IsActive())
	//	{
	//		//if(!GetClient()->HasEntityFlag(ET_ENT_FLAG_MOUNTED))
	//		out << "Repairing " + (m_MapGoal?m_MapGoal->GetName():"");
	//	}
	//}

	//void RepairMg42::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//	}
	//}

	//// FollowPathUser functions.
	//bool RepairMg42::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool RepairMg42::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_MG42Position;
	//	return true;
	//}

	//void RepairMg42::OnTarget()
	//{
	//	FINDSTATE(ws,WeaponSystem,GetRootState());
	//	if(ws != NULL && ws->GetCurrentRequestOwner() == GetNameHash())
	//		ws->FireWeapon();
	//}

	//obReal RepairMg42::GetPriority()
	//{
	//	if(IsActive())
	//		return GetLastPriority();

	//	BitFlag64 entFlags;

	//	GoalManager::Query qry(0x17929136 /* REPAIRMG42 */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//	{
	//		if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//			continue;

	//		bool bBroken = InterfaceFuncs::IsMountableGunRepairable(GetClient(), qry.m_List[i]->GetEntity());
	//		if(!bBroken)
	//			continue;

	//		m_MapGoal = qry.m_List[i];
	//		break;
	//	}
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void RepairMg42::Enter()
	//{
	//	m_MapGoal->GetProperty("IgnoreTargets",m_IgnoreTargets);

	//	Tracker.InProgress = m_MapGoal;
	//	m_MG42Position = m_MapGoal->GetWorldBounds().Center;
	//	//m_MapGoal->GetWorldBounds().CenterBottom(m_MG42Position);
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));
	//}

	//void RepairMg42::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();
	//	Tracker.Reset();
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem,GetRootState(),ReleaseWeaponRequest(GetNameHash()));
	//}

	//State::StateStatus RepairMg42::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	bool bBroken = InterfaceFuncs::IsMountableGunRepairable(GetClient(), m_MapGoal->GetEntity());
	//	if(!bBroken || !m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		GetClient()->PressButton(BOT_BUTTON_CROUCH);
	//		GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition());

	//		Priority::ePriority pri = m_IgnoreTargets ? Priority::High : Priority::Medium;
	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(pri,this,GetNameHash()));
	//		FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(pri, GetNameHash(), ET_WP_PLIERS));
	//	}
	//	return State_Busy;
	//}

	//////////////////////////////////////////////////////////////////////////

	//TakeCheckPoint::TakeCheckPoint()
	//	: StateChild("TakeCheckPoint")
	//	, FollowPathUser("TakeCheckPoint")
	//{
	//}

	//void TakeCheckPoint::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void TakeCheckPoint::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//	}
	//}

	//// FollowPathUser functions.
	//bool TakeCheckPoint::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//obReal TakeCheckPoint::GetPriority()
	//{
	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0xf7e4a57f /* CHECKPOINT */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	qry.GetBest(m_MapGoal);

	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void TakeCheckPoint::Enter()
	//{
	//	m_TargetPosition = m_MapGoal->GetWorldBounds().Center;
	//	Tracker.InProgress = m_MapGoal;
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));
	//}

	//void TakeCheckPoint::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();
	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus TakeCheckPoint::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		m_TargetPosition.z = GetClient()->GetPosition().z;
	//		GetClient()->GetSteeringSystem()->SetTarget(m_TargetPosition, 32.f);			
	//	}
	//	return State_Busy;
	//}

	//////////////////////////////////////////////////////////////////////////

	//ReviveTeammate::ReviveTeammate()
	//	: StateChild("ReviveTeammate")
	//	, FollowPathUser("ReviveTeammate")
	//	, m_Range(2000.f)
	//{
	//	LimitToWeapon().SetFlag(ET_WP_SYRINGE);
	//}

	//void ReviveTeammate::GetDebugString(StringStr &out)
	//{
	//	switch(m_GoalState)
	//	{
	//	case REVIVING:
	//		out << "Reviving ";
	//		break;
	//	case HEALING:
	//		out << "Healing ";
	//		break;
	//	}

	//	if(m_MapGoal && m_MapGoal->GetEntity().IsValid())
	//		out << std::endl << EngineFuncs::EntityName(m_MapGoal->GetEntity(), "<noname>");
	//}

	//void ReviveTeammate::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
	//		Utils::DrawLine(GetClient()->GetPosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::MAGENTA,5.f);
	//	}
	//}

	//// FollowPathUser functions.
	//bool ReviveTeammate::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool ReviveTeammate::GetAimPosition(Vector3f &_aimpos)
	//{
	//	const Box3f obb = m_MapGoal->GetWorldBounds();
	//	if ( m_GoalState == REVIVING )
	//		_aimpos = obb.GetCenterBottom();
	//	else
	//		_aimpos = obb.Center;
	//	return true;
	//}

	//void ReviveTeammate::OnTarget()
	//{
	//	FINDSTATE(ws, WeaponSystem, GetRootState());
	//	if(ws)
	//	{
	//		if(InterfaceFuncs::IsAlive(m_MapGoal->GetEntity()))
	//		{
	//			if(ws->CurrentWeaponIs(ET_WP_MEDKIT))
	//				ws->FireWeapon();
	//		}
	//		else
	//		{
	//			if(ws->CurrentWeaponIs(ET_WP_SYRINGE))
	//				ws->FireWeapon();
	//		}
	//	}
	//}

	//obReal ReviveTeammate::GetPriority()
	//{
	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0x2086cdf0 /* REVIVE */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	//qry.GetBest(m_MapGoal);

	//	float fDistToRevive;
	//	float fClosestRevive = 0.f;
	//	MapGoalList::iterator mIt = qry.m_List.begin();
	//	for(; mIt != qry.m_List.end(); )
	//	{
	//		fDistToRevive = SquaredLength2d((*mIt)->GetPosition(), GetClient()->GetPosition());
	//		(*mIt)->GetProperty("Range",m_Range);
	//		if ( fDistToRevive > m_Range * m_Range )
	//		{
	//			BlackboardDelay(5.f, (*mIt)->GetSerialNum()); // ignore it for a while so dist calcs aren't done every frame
	//			mIt = qry.m_List.erase(mIt);
	//			continue;
	//		}

	//		// use the closest one or the first one found within 200 units
	//		if ( fClosestRevive == 0.f || (fDistToRevive < fClosestRevive && fDistToRevive > 40000.f) )
	//		{
	//			fClosestRevive = fDistToRevive;
	//			m_MapGoal = (*mIt);
	//		}

	//		++mIt;
	//	}

	//	m_List = qry.m_List;
	//	// m_MapGoal.reset();
	//	// TODO: check weapon capability vs target underwater?

	//	return !m_List.empty() && m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void ReviveTeammate::Enter()
	//{
	//	m_GoalState = REVIVING;

	//	m_CheckReviveTimer.Delay(2.f);

	//	FINDSTATE(fp, FollowPath, GetRootState());
	//	if(fp != NULL && fp->Goto(this, m_List, Run))
	//	{
	//		m_MapGoal = m_List[GetDestinationIndex()];
	//		Tracker.InProgress = m_MapGoal;
	//	}
	//	else
	//	{
	//		m_MapGoal.reset();
	//		Tracker.Reset();
	//		for(obuint32 i = 0; i < m_List.size(); ++i)
	//		{
	//			BlackboardDelay(10.f, m_List[i]->GetSerialNum());
	//		}
	//	}
	//}

	//void ReviveTeammate::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();

	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(),ReleaseWeaponRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus ReviveTeammate::Update(float fDt)
	//{
	//	if(!m_MapGoal)
	//		return State_Finished;

	//	if(m_CheckReviveTimer.IsExpired())
	//	{
	//		// finish if there's new goals, so we can activate again and go to a new goal
	//		if(AreThereNewReviveGoals())
	//			return State_Finished;
	//		m_CheckReviveTimer.Delay(2.f);
	//	}

	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//		return State_Finished;
	//	}

	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	GameEntity reviveEnt = m_MapGoal->GetEntity();

	//	//////////////////////////////////////////////////////////////////////////
	//	Msg_HealthArmor ha;
	//	if(InterfaceFuncs::GetHealthAndArmor(reviveEnt, ha) && ha.m_CurrentHealth >= ha.m_MaxHealth)
	//		return State_Finished;

	//	BitFlag64 ef;
	//	if(InterfaceFuncs::GetEntityFlags(reviveEnt,ef))
	//	{
	//		if(ef.CheckFlag(ET_ENT_FLAG_INLIMBO))
	//			return State_Finished;
	//	}
	//	//////////////////////////////////////////////////////////////////////////

	//	if(DidPathSucceed())
	//	{
	//		if(GetClient()->GetStuckTime() > 2000)
	//		{
	//			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
	//			return State_Finished;
	//		}

	//		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::High,this,GetNameHash()));
	//		GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition());

	//		switch(m_GoalState)
	//		{
	//		case REVIVING:
	//			{
	//				if(InterfaceFuncs::IsAlive(reviveEnt))
	//				{
	//					m_GoalState = HEALING;
	//				}
	//				else
	//				{
	//					GetClient()->GetSteeringSystem()->SetNoAvoidTime(IGame::GetTime() + 1000);
	//					FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::High, GetNameHash(), ET_WP_SYRINGE));
	//				}

	//				// if the height differences are significant, jump/crouch
	//				const Vector3f eyePos = GetClient()->GetEyePosition();
	//				Vector3f aimPos;
	//				GetAimPosition( aimPos );
	//				const float heightDiff = aimPos.z - eyePos.z;
	//				if ( heightDiff > 20.f ) {
	//					if(GetClient()->GetEntityFlags().CheckFlag(ENT_FLAG_ONGROUND)) {
	//						GetClient()->PressButton(BOT_BUTTON_JUMP);
	//					}
	//				} else /*if ( heightDiff > 20.f )*/ {
	//					BitFlag64 btns;
	//					btns.SetFlag(BOT_BUTTON_CROUCH);
	//					GetClient()->HoldButton(btns,IGame::GetDeltaTime()*2);
	//				}

	//				break;
	//			}				
	//		case HEALING:
	//			{
	//				// cs: bb delay any time the goal needs interrupted here
	//				// otherwise it's a loop of goto and then finish which causes them
	//				// to just 'stick' to the target without doing anything useful 

	//				if(GetClient()->GetTargetingSystem()->HasTarget())
	//				{
	//					BlackboardDelay(5.f, m_MapGoal->GetSerialNum());
	//					return State_Finished;
	//				}

	//				if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ET_WP_MEDKIT, Primary))
	//				{
	//					BlackboardDelay(5.f, m_MapGoal->GetSerialNum());
	//					return State_Finished;
	//				}

	//				GetClient()->GetSteeringSystem()->SetNoAvoidTime(IGame::GetTime() + 1000);
	//				FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::High, GetNameHash(), ET_WP_MEDKIT));
	//				break;
	//			}
	//		}
	//	}
	//	return State_Busy;
	//}

	//bool ReviveTeammate::AreThereNewReviveGoals()
	//{
	//	GoalManager::Query qry(0x2086cdf0 /* REVIVE */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);

	//	for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//	{
	//		if(std::find(m_List.begin(),m_List.end(),qry.m_List[i]) == m_List.end())
	//			return true;
	//	}

	//	return false;
	//}

	//////////////////////////////////////////////////////////////////////////

	//DefuseDynamite::DefuseDynamite()
	//	: StateChild("DefuseDynamite")
	//	, FollowPathUser("DefuseDynamite")
	//{
	//	LimitToWeapon().SetFlag(ET_WP_PLIERS);
	//}

	//void DefuseDynamite::GetDebugString(StringStr &out)
	//{
	//	out << (m_MapGoal ? m_MapGoal->GetName() : "");
	//}

	//void DefuseDynamite::RenderDebug()
	//{
	//	if(IsActive())
	//	{
	//		Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
	//		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
	//	}
	//}

	//// FollowPathUser functions.
	//bool DefuseDynamite::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	//{
	//	if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
	//		_final = false;
	//	else 
	//		_final = true;
	//	return true;
	//}

	//// AimerUser functions.
	//bool DefuseDynamite::GetAimPosition(Vector3f &_aimpos)
	//{
	//	_aimpos = m_MapGoal->GetWorldBounds().Center;
	//	return true;
	//}

	//void DefuseDynamite::OnTarget()
	//{
	//	FINDSTATE(ws, WeaponSystem, GetRootState());
	//	if(ws != NULL && ws->CurrentWeaponIs(ET_WP_PLIERS))
	//		ws->FireWeapon();
	//}

	//obReal DefuseDynamite::GetPriority()
	//{
	//	if(IsActive())
	//		return GetLastPriority();

	//	m_MapGoal.reset();

	//	GoalManager::Query qry(0x1899efc7 /* DEFUSE */, GetClient());
	//	GoalManager::GetInstance()->GetGoals(qry);
	//	for(obuint32 i = 0; i < qry.m_List.size(); ++i)
	//	{
	//		if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
	//			continue;

	//		if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS) < 1)
	//			continue;

	//		if(InterfaceFuncs::GetExplosiveState(GetClient(), qry.m_List[i]->GetEntity()) == XPLO_ARMED)
	//		{
	//			m_MapGoal = qry.m_List[i];
	//			break;
	//		}
	//		else
	//		{
	//			qry.m_List[i]->SetDeleteMe(true);
	//		}
	//	}
	//	return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	//}

	//void DefuseDynamite::Enter()
	//{
	//	m_TargetPosition = m_MapGoal->GetWorldBounds().Center;
	//	Tracker.InProgress = m_MapGoal;
	//	FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));
	//}

	//void DefuseDynamite::Exit()
	//{
	//	FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

	//	m_MapGoal.reset();

	//	FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	//	FINDSTATEIF(WeaponSystem, GetRootState(),ReleaseWeaponRequest(GetNameHash()));

	//	Tracker.Reset();
	//}

	//State::StateStatus DefuseDynamite::Update(float fDt)
	//{
	//	if(DidPathFail())
	//	{
	//		BlackboardDelay(2.f, m_MapGoal->GetSerialNum()); // cs: was 10 seconds
	//		return State_Finished;
	//	}

	//	if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
	//		return State_Finished;

	//	if(DidPathSucceed())
	//	{
	//		ExplosiveState eState = InterfaceFuncs::GetExplosiveState(GetClient(), m_MapGoal->GetEntity());
	//		switch(eState)
	//		{
	//		case XPLO_INVALID:
	//		case XPLO_UNARMED:
	//			return State_Finished;
	//		default:
	//			break; // keep processing
	//		}

	//		m_TargetPosition = m_MapGoal->GetWorldBounds().Center;

	//		const float fDistanceToDynamite = SquaredLength2d(m_TargetPosition, GetClient()->GetPosition());
	//		if(fDistanceToDynamite > 2500.0f)
	//		{
	//			GetClient()->GetSteeringSystem()->SetTarget(m_TargetPosition);
	//		}
	//		else
	//		{
	//			GetClient()->PressButton(BOT_BUTTON_CROUCH);

	//			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::High,this,GetNameHash()));
	//			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::High, GetNameHash(), ET_WP_PLIERS));
	//		}
	//	}
	//	return State_Busy;
	//}
};
