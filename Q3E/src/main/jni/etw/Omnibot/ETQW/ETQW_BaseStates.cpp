////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETQW.h"
#include "ETQW_BaseStates.h"

namespace AiState
{
	BuildConstruction::BuildConstruction()
		: StateChild("BuildConstruction")
		, FollowPathUser("BuildConstruction")
		, m_AdjustedPosition(false)
	{
		LimitToClass().SetFlag(ETQW_CLASS_ENGINEER);
	}

	void BuildConstruction::RenderDebug()
	{
		if(IsActive())
		{
			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
			Utils::DrawLine(GetClient()->GetEyePosition(),m_ConstructionPos,COLOR::GREEN,5.f);
		}
	}

	// FollowPathUser functions.
	bool BuildConstruction::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
			_final = false;
		else 
			_final = true;
		return true;
	}

	// AimerUser functions.
	bool BuildConstruction::GetAimPosition(Vector3f &_aimpos)
	{
		_aimpos = m_ConstructionPos;
		return true;
	}

	void BuildConstruction::OnTarget()
	{
		FINDSTATE(ws, WeaponSystem, GetRootState());
		if(ws && ws->CurrentWeaponIs(ETQW_WP_PLIERS))
			ws->FireWeapon();

		//InterfaceFuncs::GetCurrentCursorHint
	}

	obReal BuildConstruction::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();
		
		m_MapGoal.reset();

		GoalManager::Query qry(0xc39bf2a3 /* BUILD */, GetClient());
		GoalManager::GetInstance()->GetGoals(qry);
		for(obuint32 i = 0; i < qry.m_List.size(); ++i)
		{
			if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
				continue;

			if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS) < 1)
				continue;

			ConstructableState cState = InterfaceFuncs::GetConstructableState(GetClient(),qry.m_List[i]->GetEntity());
			if(cState == CONST_UNBUILT)
			{
				m_MapGoal = qry.m_List[i];
				break;
			}
		}
		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}

	void BuildConstruction::Enter()
	{
		m_AdjustedPosition = false;
		m_ConstructionPos = m_MapGoal->GetWorldBounds().Center;

		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));

		Tracker.InProgress = m_MapGoal;
	}

	void BuildConstruction::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_MapGoal.reset();
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

		Tracker.Reset();
	}

	State::StateStatus BuildConstruction::Update(float fDt)
	{
		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		//////////////////////////////////////////////////////////////////////////
		// Check the construction status
		ConstructableState cState = InterfaceFuncs::GetConstructableState(GetClient(),m_MapGoal->GetEntity());
		switch(cState)
		{
		case CONST_INVALID: // Invalid constructable, fail
			return State_Finished;
		case CONST_BUILT:	// It's built, consider it a success.
			return State_Finished;
		case CONST_UNBUILT:
		case CONST_BROKEN:
			// This is what we want.
			break;
		}
		//////////////////////////////////////////////////////////////////////////

		if(DidPathSucceed())
		{
			float fDistanceToConstruction = (m_ConstructionPos - GetClient()->GetPosition()).SquaredLength();
			if (fDistanceToConstruction > 4096.0f)
			{
				// check for badly waypointed maps
				if (!m_AdjustedPosition)
				{
					// use our z value because some trigger entities may be below the ground
					Vector3f checkPos(m_ConstructionPos.x, m_ConstructionPos.y, GetClient()->GetEyePosition().z);

					obTraceResult tr;
					EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(),checkPos, 
						NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), GetClient()->GetGameID(), True);

					if (tr.m_Fraction != 1.0f && !tr.m_HitEntity.IsValid())
					{
						m_MapGoal->SetDeleteMe(true);
						return State_Finished;
					}

					// do a trace to adjust position
					EngineFuncs::TraceLine(tr, 
						GetClient()->GetEyePosition(),
						m_ConstructionPos, 
						NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, False);

					if (tr.m_Fraction != 1.0f)
					{
						m_ConstructionPos = *(Vector3f*)tr.m_Endpos;
					}

					m_AdjustedPosition = true;
				}
			}
			else
			{
				FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
				FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ETQW_WP_PLIERS));
			}
			GetClient()->GetSteeringSystem()->SetTarget(m_ConstructionPos, 64.f);
		}
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	PlantExplosive::PlantExplosive()
		: StateChild("PlantExplosive")
		, FollowPathUser("PlantExplosive")
	{
		LimitToClass().SetFlag(ETQW_CLASS_ENGINEER);
		LimitToClass().SetFlag(ETQW_CLASS_COVERTOPS);
		SetAlwaysRecieveEvents(true);
	}

	void PlantExplosive::RenderDebug()
	{
		if(IsActive())
		{
			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
		}
	}

	// FollowPathUser functions.
	bool PlantExplosive::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
			_final = false;
		else 
			_final = true;
		return true;
	}

	// AimerUser functions.
	bool PlantExplosive::GetAimPosition(Vector3f &_aimpos)
	{
		switch(m_GoalState)
		{
		case LAY_EXPLOSIVE:
			_aimpos = m_TargetPosition;
			break;
		case ARM_EXPLOSIVE:
			_aimpos = m_ExplosivePosition;
			break;
		case RUNAWAY:
		case DETONATE_EXPLOSIVE:
		default:
			OBASSERT(0, "Invalid Aim State");
			return false;
		}
		return true;
	}

	void PlantExplosive::OnTarget()
	{
		FINDSTATE(ws, WeaponSystem, GetRootState());
		if(ws)
		{
			if(m_GoalState == LAY_EXPLOSIVE)
			{
				if(ws->CurrentWeaponIs(ETQW_WP_HE_CHARGE))
					ws->FireWeapon();
			}
			else if(m_GoalState == ARM_EXPLOSIVE)
			{
				if(ws->CurrentWeaponIs(ETQW_WP_PLIERS))
					ws->FireWeapon();
			}
			else if(m_GoalState == DETONATE_EXPLOSIVE)
			{
				/*if(ws->CurrentWeaponIs(ETQW_WP_SATCHEL_DET))
					ws->FireWeapon();*/
			}
		}
	}

	obReal PlantExplosive::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();

		ExplosiveTargetType myTargetType = XPLO_TYPE_DYNAMITE;
		ETQW_Weapon weaponType = ETQW_WP_HE_CHARGE;
		switch(GetClient()->GetClass())
		{
		case ETQW_CLASS_ENGINEER:
			{
				weaponType = ETQW_WP_HE_CHARGE;
				myTargetType = XPLO_TYPE_DYNAMITE;
				break;
			}
		/*case ETQW_CLASS_COVERTOPS:
			{
				weaponType = ETQW_WP_SATCHEL;
				myTargetType = XPLO_TYPE_SATCHEL;
				break;
			}*/
		default:
			OBASSERT(0, "Wrong Class with Evaluator_PlantExplosive");
			return 0.0;
		}

		m_MapGoal.reset();

		if(InterfaceFuncs::IsWeaponCharged(GetClient(), weaponType, Primary)) 
		{
			{
				GoalManager::Query qry(0xbbcae592 /* PLANT */, GetClient());
				GoalManager::GetInstance()->GetGoals(qry);
				for(obuint32 i = 0; i < qry.m_List.size(); ++i)
				{
					if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
						continue;

					if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS) < 1)
						continue;

					ConstructableState cState = InterfaceFuncs::IsDestroyable(GetClient(), qry.m_List[i]->GetEntity());
					if(cState == CONST_DESTROYABLE)
					{
						m_MapGoal = qry.m_List[i];
						break;
					}
				}
			}

			if(!m_MapGoal)
			{
				GoalManager::Query qry(0xa411a092 /* MOVER */, GetClient());
				GoalManager::GetInstance()->GetGoals(qry);
				for(obuint32 i = 0; i < qry.m_List.size(); ++i)
				{
					if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
						continue;
					
					m_MapGoal = qry.m_List[i];
					break;
				}
			}
		}
		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}

	void PlantExplosive::Enter()
	{
		// set position to base of construction
		Box3f obb = m_MapGoal->GetWorldBounds();
		m_ExplosivePosition = obb.GetCenterBottom();
		m_TargetPosition = m_ExplosivePosition;

		m_AdjustedPosition = false;
		m_GoalState = LAY_EXPLOSIVE;

		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));

		Tracker.InProgress = m_MapGoal;
	}

	void PlantExplosive::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_ExplosiveEntity.Reset();

		m_MapGoal.reset();
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

		Tracker.Reset();
	}

	State::StateStatus PlantExplosive::Update(float fDt)
	{
		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		// If it's not destroyable, consider it a success.
		if(!InterfaceFuncs::IsDestroyable(GetClient(), m_MapGoal->GetEntity()))
			return State_Finished;

		if(m_ExplosiveEntity.IsValid() && !IGame::IsEntityValid(m_ExplosiveEntity))
			return State_Finished;

		if(DidPathSucceed())
		{
			switch(GetClient()->GetClass())
			{
			case ETQW_CLASS_ENGINEER:
				return _UpdateDynamite();
			case ETQW_CLASS_COVERTOPS:
				return _UpdateSatchel();
			default:
				OBASSERT(0, "Wrong Class in PlantExplosive");
			}
		}		
		return State_Busy;
	}

	State::StateStatus PlantExplosive::_UpdateDynamite()
	{
		switch(m_GoalState)
		{
		case LAY_EXPLOSIVE:
			{
				if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ETQW_WP_HE_CHARGE, Primary))
					return State_Finished;

				/*if(m_Client->IsDebugEnabled(BOT_DEBUG_GOALS))
				{
					Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 1.f);

					Vector3f vCenter;
					m_MapGoal->GetWorldBounds().CenterPoint(vCenter);
					Utils::DrawLine(m_Client->GetPosition(), vCenter, COLOR::GREEN, 1.0f);
					Utils::DrawLine(m_Client->GetPosition(), m_MapGoal->GetPosition(), COLOR::RED, 1.0f);
				}*/

				float fDistanceToTarget = (m_TargetPosition - GetClient()->GetPosition()).SquaredLength();
				if (fDistanceToTarget > 10000.0f)
				{
					// Move toward it.
					//m_Client->GetSteeringSystem()->SetTarget(m_TargetPosition);

					// check for badly waypointed maps
					if (!m_AdjustedPosition)
					{
						m_AdjustedPosition = true;

						// use our z value because some trigger entities may be below the ground
						Vector3f vCheckPos(m_TargetPosition.x, m_TargetPosition.y, GetClient()->GetEyePosition().z);
						/*if(m_Client->IsDebugEnabled(BOT_DEBUG_GOALS))
						{
							Utils::DrawLine(GetClient()->GetEyePosition(), vCheckPos, COLOR::GREEN, 2.0f);
						}*/

						obTraceResult tr;
						EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(), vCheckPos, 
							NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), GetClient()->GetGameID(), True);
						if (tr.m_Fraction != 1.0f && !tr.m_HitEntity.IsValid())
						{
							//m_TargetEntity->SetDeleteMe(true);
							return State_Finished;
						}

						// do a trace to adjust position
						EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(), m_TargetPosition, 
							NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, False);
						if (tr.m_Fraction != 1.0f)
						{
							m_TargetPosition = Vector3f(tr.m_Endpos);
						}
					}
				}
				else
				{
					// We're within range, so let's start laying.
					GetClient()->ResetStuckTime();
					FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
					FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ETQW_WP_HE_CHARGE));
				}
				break;
			}
		case ARM_EXPLOSIVE:
			{
				if(InterfaceFuncs::GetExplosiveState(GetClient(), m_ExplosiveEntity) == XPLO_ARMED)
				{
					BlackboardDelay(30.f, m_MapGoal->GetSerialNum());
					return State_Finished;
				}

				// Disable avoidance for this frame.
				GetClient()->GetSteeringSystem()->SetNoAvoidTime(IGame::GetTime());

				// update dynamite position
				EngineFuncs::EntityPosition(m_ExplosiveEntity, m_ExplosivePosition);

				// move a little bit close if dyno too far away
				if ((m_ExplosivePosition - GetClient()->GetPosition()).SquaredLength() > 2500.0f)
				{
					GetClient()->GetSteeringSystem()->SetTarget(m_ExplosivePosition);
				}
				else
				{
					GetClient()->ResetStuckTime();
					FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
					FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ETQW_WP_PLIERS));					
				}
				break;
			}
		default:
			// keep processing
			break;
		}
		return State_Busy;
	}

	State::StateStatus PlantExplosive::_UpdateSatchel()
	{
		//switch(m_GoalState)
		//{
		//case LAY_EXPLOSIVE:
		//	{
		//		if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ETQW_WP_SATCHEL, Primary))
		//			return State_Finished;

		//		float fDistanceToTarget = (m_TargetPosition - GetClient()->GetPosition()).SquaredLength();
		//		if (fDistanceToTarget > 10000.0f)
		//		{
		//			// Move toward it.
		//			//GetClient()->GetSteeringSystem()->SetTarget(m_TargetPosition);

		//			// check for badly waypointed maps
		//			if (!m_AdjustedPosition)
		//			{
		//				m_AdjustedPosition = true;

		//				// use our z value because some trigger entities may be below the ground
		//				Vector3f vCheckPos(m_TargetPosition.x, m_TargetPosition.y, GetClient()->GetEyePosition().z);
		//				/*if(m_Client->IsDebugEnabled(BOT_DEBUG_GOALS))
		//				{
		//					Utils::DrawLine(m_Client->GetEyePosition(), vCheckPos, COLOR::GREEN, 2.0f);
		//				}*/

		//				obTraceResult tr;
		//				EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(), vCheckPos, 
		//					NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), GetClient()->GetGameID(), True);
		//				if (tr.m_Fraction != 1.0f && !tr.m_HitEntity.IsValid())
		//				{
		//					AABB aabb, mapaabb;
		//					EngineFuncs::EntityWorldAABB(m_MapGoal->GetEntity(), aabb);
		//					//g_EngineFuncs->GetMapExtents(mapaabb);

		//					//m_TargetEntity->SetDeleteMe(true);
		//					return State_Finished;
		//				}

		//				// do a trace to adjust position
		//				EngineFuncs::TraceLine(tr, GetClient()->GetEyePosition(), m_TargetPosition, 
		//					NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, False);
		//				if (tr.m_Fraction != 1.0f)
		//				{
		//					m_TargetPosition = Vector3f(tr.m_Endpos);
		//				}
		//			}
		//		}
		//		else
		//		{
		//			// We're within range, so let's start laying.
		//			GetClient()->ResetStuckTime();
		//			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(0.6f,this,GetNameHash()));
		//			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(0.6f, GetNameHash(), ETQW_WP_SATCHEL));
		//		}
		//		break;
		//	}
		//case ARM_EXPLOSIVE:
		//case RUNAWAY:
		//	{
		//		OBASSERT(m_ExplosiveEntity.IsValid(), "No Explosive Entity!");			

		//		// Generate a random goal.
		//		FINDSTATEIF(FollowPath,GetRootState(),GotoRandomPt(this));
		//		m_GoalState = DETONATE_EXPLOSIVE;
		//		break;
		//	}		
		//case DETONATE_EXPLOSIVE:
		//	{
		//		// Are we far enough away to blow it up?
		//		const float SATCHEL_DETQW_DISTANCE = 350.0f;
		//		const bool BLOW_TARGETQW_OR_NOT = true;

		//		Vector3f vSatchelPos;
		//		if(EngineFuncs::EntityPosition(m_ExplosiveEntity, vSatchelPos))
		//		{
		//			if((GetClient()->GetPosition() - vSatchelPos).Length() >= SATCHEL_DETQW_DISTANCE)
		//			{
		//				// Do we need this? perhaps make this flag scriptable?
		//				if(BLOW_TARGETQW_OR_NOT || !GetClient()->GetTargetingSystem()->HasTarget())
		//				{
		//					FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(1.f, GetNameHash(), ETQW_WP_SATCHEL_DET));
		//					ExplosiveState eState = InterfaceFuncs::GetExplosiveState(GetClient(), m_ExplosiveEntity);							
		//					if(eState == XPLO_INVALID)
		//						return State_Finished;
		//				}
		//			}
		//		}
		//		break;
		//	}
		//}
		return State_Busy;
	}

	void PlantExplosive::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(ACTION_WEAPON_FIRE)
			{
				const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
				if(m->m_WeaponId == ETQW_WP_HE_CHARGE && m->m_Projectile.IsValid())
				{
					m_ExplosiveEntity = m->m_Projectile;
					m_GoalState = ARM_EXPLOSIVE;
				}
				/*else if(m->m_WeaponId == ETQW_WP_SATCHEL && m->m_Projectile.IsValid())
				{
					m_ExplosiveEntity = m->m_Projectile;
					m_GoalState = RUNAWAY;
				}*/
				break;
			}			
		}
	}

	//////////////////////////////////////////////////////////////////////////

	MountMg42::MountMg42()
		: StateChild("MountMg42")
		, FollowPathUser("MountMg42")
		, m_ScanDirection(SCAN_DEFAULT)
		, m_NextScanTime(0)
	{
	}

	void MountMg42::GetDebugString(StringStr &out)
	{
		if(IsActive())
		{
			if(!GetClient()->HasEntityFlag(ETQW_ENT_FLAG_MOUNTED))
				out << "Mounting ";
			else
			{
				switch(m_ScanDirection)
				{
				case SCAN_DEFAULT:
					out << "Scan Facing ";
					break;
				case SCAN_MIDDLE:
					out << "Scan Middle ";
					break;
				case SCAN_LEFT:
					out << "Scan Left ";
					break;
				case SCAN_RIGHT:
					out << "Scan Right ";
					break;
				}
			}

			if(m_MapGoal)
				out << m_MapGoal->GetName();
		}
	}

	void MountMg42::RenderDebug()
	{
		if(IsActive())
		{
			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
			m_TargetZone.RenderDebug();
		}
	}

	// FollowPathUser functions.
	bool MountMg42::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
			_final = false;
		else 
			_final = true;
		return true;
	}

	// AimerUser functions.
	bool MountMg42::GetAimPosition(Vector3f &_aimpos)
	{
		_aimpos = m_AimPoint;
		return true;
	}

	void MountMg42::OnTarget()
	{
		if(!GetClient()->HasEntityFlag(ETQW_ENT_FLAG_MOUNTED))
			GetClient()->PressButton(BOT_BUTTON_USE);
	}

	obReal MountMg42::GetPriority()
	{
		if(IsActive() || GetClient()->HasEntityFlag(ETQW_ENT_FLAG_MOUNTED))
			return GetLastPriority();
		
		BitFlag64 entFlags;

		GoalManager::Query qry(0xe1a2b09c /* MOUNTMG42 */, GetClient());
		GoalManager::GetInstance()->GetGoals(qry);
		for(obuint32 i = 0; i < qry.m_List.size(); ++i)
		{
			if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
				continue;

			GameEntity gunOwner = InterfaceFuncs::GetMountedPlayerOnMG42(GetClient(), qry.m_List[i]->GetEntity());
			int gunHealth = InterfaceFuncs::GetGunHealth(GetClient(), qry.m_List[i]->GetEntity());
			bool bBroken = InterfaceFuncs::IsMountableGunRepairable(GetClient(), qry.m_List[i]->GetEntity());

			if(bBroken)
				continue;

			if(!InterfaceFuncs::GetEntityFlags(qry.m_List[i]->GetEntity(), entFlags) ||
				!entFlags.CheckFlag(ETQW_ENT_FLAG_ISMOUNTABLE))
				continue;

			// Make sure nobody has it mounted.
			if((!gunOwner.IsValid() || !GetClient()->IsAllied(gunOwner)) && (gunHealth > 0))
			{
				m_MapGoal = qry.m_List[i];
				break;
			}
		}
		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}

	void MountMg42::Enter()
	{
		m_ScanDirection = SCAN_MIDDLE;
		m_NextScanTime = IGame::GetTime() + (int)Mathf::IntervalRandom(2000.0f, 7000.0f);

		m_AimPoint = m_MapGoal->GetPosition();
		m_MG42Position = m_AimPoint;

		m_ScanLeft = Vector3f::ZERO;
		m_ScanRight = Vector3f::ZERO;

		m_GotGunProperties = false;
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));

		Tracker.InProgress = m_MapGoal;

		m_TargetZone.Restart(256.f);
	}

	void MountMg42::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_MapGoal.reset();
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));

		if(GetClient()->HasEntityFlag(ETQW_ENT_FLAG_MOUNTED))
			GetClient()->PressButton(BOT_BUTTON_USE);

		Tracker.Reset();
	}

	State::StateStatus MountMg42::Update(float fDt)
	{
		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		//////////////////////////////////////////////////////////////////////////
		// Only fail if a friendly player is on this gun or gun has been destroyed in the meantime
		//int gunHealth = InterfaceFuncs::GetGunHealth(m_Client, m_MG42Goal->GetEntity());
		GameEntity mounter = InterfaceFuncs::GetMountedPlayerOnMG42(GetClient(), m_MapGoal->GetEntity());
		if(InterfaceFuncs::IsMountableGunRepairable(GetClient(), m_MapGoal->GetEntity()) ||
			(mounter.IsValid() && (mounter != GetClient()->GetGameEntity()) && GetClient()->IsAllied(mounter)))
		{
			return State_Finished;
		}
		//////////////////////////////////////////////////////////////////////////

		if(DidPathSucceed())
		{
			GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition());
			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));

			if(GetClient()->HasEntityFlag(ETQW_ENT_FLAG_MOUNTED))
			{
				m_TargetZone.Update(GetClient());

				if(!m_GotGunProperties)
				{
					m_GotGunProperties = true;
					_GetMG42Properties();
					m_AimPoint = m_MapGoal->GetPosition() + m_GunCenterArc * 512.f;
				}

				if(m_NextScanTime < IGame::GetTime())
				{
					m_NextScanTime = IGame::GetTime() + (int)Mathf::IntervalRandom(2000.0f, 7000.0f);
					m_ScanDirection = (int)Mathf::IntervalRandom(0.0f, (float)NUM_SCAN_TYPES);

					// we're mounted, so lets look around mid view.
					m_TargetZone.UpdateAimPosition();
				}

				if(m_TargetZone.HasAim())
					m_ScanDirection = SCAN_ZONES;

				switch(m_ScanDirection)
				{
				case SCAN_DEFAULT:
					if(m_MapGoal->GetFacing() != Vector3f::ZERO)
					{
						m_AimPoint = m_MG42Position + m_MapGoal->GetFacing() * 1024.f;
						break;
					}
				case SCAN_MIDDLE:
					{
						m_AimPoint = m_MG42Position + m_GunCenterArc * 1024.f;
						break;
					}
				case SCAN_LEFT:
					if(m_ScanLeft != Vector3f::ZERO)
					{
						m_AimPoint = m_MG42Position + m_ScanLeft * 1024.f;
						break;
					}						
				case SCAN_RIGHT:
					if(m_ScanRight != Vector3f::ZERO)
					{
						m_AimPoint = m_MG42Position + m_ScanRight * 1024.f;
						break;
					}
				case SCAN_ZONES:
					{
						m_AimPoint = m_TargetZone.GetAimPosition();
						break;
					}
				default:
					break;
				}
			}
		}
		return State_Busy;
	}

	bool MountMg42::_GetMG42Properties()
	{
		ETQW_MG42Info data;
		if(!InterfaceFuncs::GetMg42Properties(GetClient(), data))
			return false;

		m_GunCenterArc = Vector3f(data.m_CenterFacing);

		m_MinHorizontalArc = data.m_MinHorizontalArc;
		m_MaxHorizontalArc = data.m_MaxHorizontalArc;
		m_MinVerticalArc = data.m_MinVerticalArc;
		m_MaxVerticalArc = data.m_MaxVerticalArc;

		// Calculate the planes for the MG42

		/*Matrix3f planeMatrices[4];
		planeMatrices[0].FromEulerAnglesXYZ(m_MinHorizontalArc, 0.0f, 0.0f);
		planeMatrices[1].FromEulerAnglesXYZ(m_MaxHorizontalArc, 0.0f, 0.0f);
		planeMatrices[2].FromEulerAnglesXYZ(0.0f, m_MinHorizontalArc, 0.0f);
		planeMatrices[3].FromEulerAnglesXYZ(0.0f, m_MaxHorizontalArc, 0.0f);

		m_GunArcPlanes[0] = Plane3f(m_GunCenterArc * planeMatrices[0], m_MG42Position);
		m_GunArcPlanes[1] = Plane3f(m_GunCenterArc * planeMatrices[1], m_MG42Position);
		m_GunArcPlanes[2] = Plane3f(m_GunCenterArc * planeMatrices[2], m_MG42Position);
		m_GunArcPlanes[3] = Plane3f(m_GunCenterArc * planeMatrices[3], m_MG42Position);*/

		const float fScanPc = 0.4f;

		Quaternionf ql;
		ql.FromAxisAngle(Vector3f::UNIT_Z, Mathf::DegToRad(m_MinHorizontalArc * fScanPc));
		m_ScanLeft = ql.Rotate(m_GunCenterArc);

		Quaternionf qr;
		qr.FromAxisAngle(Vector3f::UNIT_Z, Mathf::DegToRad(m_MaxHorizontalArc * fScanPc));
		m_ScanRight = qr.Rotate(m_GunCenterArc);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////

	TakeCheckPoint::TakeCheckPoint()
		: StateChild("TakeCheckPoint")
		, FollowPathUser("TakeCheckPoint")
	{
	}

	void TakeCheckPoint::RenderDebug()
	{
		if(IsActive())
		{
			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
		}
	}

	// FollowPathUser functions.
	bool TakeCheckPoint::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
			_final = false;
		else 
			_final = true;
		return true;
	}

	obReal TakeCheckPoint::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();

		m_MapGoal.reset();

		GoalManager::Query qry(0xf7e4a57f /* CHECKPOINT */, GetClient());
		GoalManager::GetInstance()->GetGoals(qry);
		qry.GetBest(m_MapGoal);

		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}

	void TakeCheckPoint::Enter()
	{
		m_TargetPosition = m_MapGoal->GetWorldBounds().Center;

		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));

		Tracker.InProgress = m_MapGoal;
	}

	void TakeCheckPoint::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_MapGoal.reset();
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

		Tracker.Reset();
	}

	State::StateStatus TakeCheckPoint::Update(float fDt)
	{
		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		if(DidPathSucceed())
		{
			m_TargetPosition.z = GetClient()->GetPosition().z;
			GetClient()->GetSteeringSystem()->SetTarget(m_TargetPosition, 32.f);			
		}
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	PlantMine::PlantMine()
		: StateChild("PlantMine")
		, FollowPathUser("PlantMine")
	{
		LimitToClass().SetFlag(ETQW_CLASS_ENGINEER);
		SetAlwaysRecieveEvents(true);
	}

	void PlantMine::RenderDebug()
	{
		if(IsActive())
		{
			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
		}
	}

	// FollowPathUser functions.
	bool PlantMine::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
			_final = false;
		else 
			_final = true;
		return true;
	}

	// AimerUser functions.
	bool PlantMine::GetAimPosition(Vector3f &_aimpos)
	{
		if(!m_LandMineEntity.IsValid())
			_aimpos = m_TargetPosition;
		else
			_aimpos = m_LandMinePosition;
		return true;
	}

	void PlantMine::OnTarget()
	{
		FINDSTATE(ws, WeaponSystem, GetRootState());
		if(ws)
		{
			if(m_LandMineEntity.IsValid() && ws->CurrentWeaponIs(ETQW_WP_PLIERS))
				ws->FireWeapon();
			else if(!m_LandMineEntity.IsValid() && ws->CurrentWeaponIs(ETQW_WP_LANDMINE))
				ws->FireWeapon();
		}
	}

	obReal PlantMine::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();

		m_MapGoal.reset();

		if(InterfaceFuncs::IsWeaponCharged(GetClient(), ETQW_WP_LANDMINE, Primary)) 
		{
			GoalManager::Query qry(0xf2dffa59 /* PLANTMINE */, GetClient());
			GoalManager::GetInstance()->GetGoals(qry);
			qry.GetBest(m_MapGoal);
		}
		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}

	void PlantMine::Enter()
	{
		// generate a random position in the goal radius
		float fRandDistance = Mathf::IntervalRandom(0.0f, m_MapGoal->GetRadius());
		Quaternionf quat(Vector3f::UNIT_Y, Mathf::DegToRad(Mathf::IntervalRandom(0.0f, 360.0f)));
		m_TargetPosition = m_MapGoal->GetPosition() + quat.Rotate(Vector3f::UNIT_Y * fRandDistance);

		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run, true));

		Tracker.InProgress = m_MapGoal;
	}

	void PlantMine::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_MapGoal.reset();
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

		Tracker.Reset();
	}

	State::StateStatus PlantMine::Update(float fDt)
	{
		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		// If it's not destroyable, consider it a success.
		if (!InterfaceFuncs::IsDestroyable(GetClient(), m_MapGoal->GetEntity()))
		{
			return State_Finished;
		}

		if(m_LandMineEntity.IsValid() && !IGame::IsEntityValid(m_LandMineEntity))
			return State_Finished;

		if(DidPathSucceed())
		{
			GetClient()->ResetStuckTime();

			static float MIN_DIST_TO_TARGETPOS = 32.0f;
			static float MIN_DIST_TO_MINE = 32.0f;

			// Have we already thrown out a mine?
			if(m_LandMineEntity.IsValid())
			{
				// Is it armed yet?
				if(InterfaceFuncs::GetExplosiveState(GetClient(), m_LandMineEntity) == XPLO_ARMED)
					return State_Finished;

				// Disable avoidance for this frame.
				//m_Client->GetSteeringSystem()->SetNoAvoidTime(IGame::GetTime());

				// Not armed yet, keep trying.
				if(EngineFuncs::EntityPosition(m_LandMineEntity, m_LandMinePosition) && 
					EngineFuncs::EntityVelocity(m_LandMineEntity, m_LandMineVelocity))
				{
					GetClient()->PressButton(BOT_BUTTON_CROUCH);

					FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
					FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ETQW_WP_PLIERS));

					// Do we need to get closer?
					bool bCloseEnough = (GetClient()->GetPosition() - m_LandMinePosition).Length() < MIN_DIST_TO_MINE;
					GetClient()->GetSteeringSystem()->SetTarget(bCloseEnough ? GetClient()->GetPosition() : m_LandMinePosition);
				}
				return State_Busy;
			}

			// Move closer if necessary.
			bool bCloseEnough = (GetClient()->GetPosition() - m_TargetPosition).Length() < MIN_DIST_TO_TARGETPOS;
			GetClient()->GetSteeringSystem()->SetTarget(bCloseEnough ? GetClient()->GetPosition() : m_TargetPosition);

			// keep watching the target position.
			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ETQW_WP_LANDMINE));
		}		
		return State_Busy;
	}

	void PlantMine::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(ACTION_WEAPON_FIRE)
			{
				const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
				if(m->m_WeaponId == ETQW_WP_LANDMINE && m->m_Projectile.IsValid())
				{
					m_LandMineEntity = m->m_Projectile;
				}
				break;
			}			
		}
	}

	//////////////////////////////////////////////////////////////////////////

	MobileMg42::MobileMg42()
		: StateChild("MobileMg42")
		, FollowPathUser("MobileMg42")
	{
		LimitToClass().SetFlag(ETQW_CLASS_SOLDIER);
	}

	void MobileMg42::RenderDebug()
	{
		if(IsActive())
		{
			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
		}
	}

	// FollowPathUser functions.
	bool MobileMg42::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
			_final = false;
		else 
			_final = true;
		return true;
	}

	// AimerUser functions.
	bool MobileMg42::GetAimPosition(Vector3f &_aimpos)
	{
		_aimpos = m_MapGoal->GetPosition() + m_MapGoal->GetFacing() * 1024.f;
		return true;
	}

	void MobileMg42::OnTarget()
	{
		//FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(1.f, GetNameHash(), ETQW_WP_MOBILE_MG42_SET));
	}

	obReal MobileMg42::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();

		m_MapGoal.reset();

		if(InterfaceFuncs::IsWeaponCharged(GetClient(), ETQW_WP_LANDMINE, Primary)) 
		{
			GoalManager::Query qry(0xbe8488ed /* MOBILEMG42 */, GetClient());
			GoalManager::GetInstance()->GetGoals(qry);
			qry.GetBest(m_MapGoal);
		}
		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}

	void MobileMg42::Enter()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));

		Tracker.InProgress = m_MapGoal;
	}

	void MobileMg42::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_MapGoal.reset();

		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

		Tracker.Reset();
	}

	State::StateStatus MobileMg42::Update(float fDt)
	{
		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		if(DidPathSucceed())
		{
			GetClient()->PressButton(BOT_BUTTON_PRONE);
			if(GetClient()->HasEntityFlag(ENT_FLAG_PRONED))
				FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
		}
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	ReviveTeammate::ReviveTeammate()
		: StateChild("ReviveTeammate")
		, FollowPathUser("ReviveTeammate")
	{
		LimitToClass().SetFlag(ETQW_CLASS_MEDIC);
	}

	void ReviveTeammate::GetDebugString(StringStr &out)
	{
		switch(m_GoalState)
		{
		case REVIVING:
			out << "Reviving ";
			break;
		case HEALING:
			out << "Healing ";
			break;
		}

		if(m_MapGoal && m_MapGoal->GetEntity().IsValid())
			out << EngineFuncs::EntityName(m_MapGoal->GetEntity(), "<noname>");
	}

	void ReviveTeammate::RenderDebug()
	{
		if(IsActive())
		{
			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
			Utils::DrawLine(GetClient()->GetPosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::MAGENTA,5.f);
		}
	}

	// FollowPathUser functions.
	bool ReviveTeammate::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
			_final = false;
		else 
			_final = true;
		return true;
	}

	// AimerUser functions.
	bool ReviveTeammate::GetAimPosition(Vector3f &_aimpos)
	{
		_aimpos = m_MapGoal->GetWorldBounds().GetCenterBottom();
		return true;
	}

	void ReviveTeammate::OnTarget()
	{
		FINDSTATE(ws, WeaponSystem, GetRootState());
		if(ws)
		{
			if(InterfaceFuncs::IsAlive(m_MapGoal->GetEntity()))
			{
				if(ws->CurrentWeaponIs(ETQW_WP_HEALTH))
					ws->FireWeapon();
			}
			else
			{
				if(ws->CurrentWeaponIs(ETQW_WP_NEEDLE))
					ws->FireWeapon();
			}
		}
	}

	obReal ReviveTeammate::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();

		m_MapGoal.reset();

		GoalManager::Query qry(0x2086cdf0 /* REVIVE */, GetClient());
		GoalManager::GetInstance()->GetGoals(qry);
		qry.GetBest(m_MapGoal);

		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}

	void ReviveTeammate::Enter()
	{
		m_GoalState = REVIVING;

		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));

		Tracker.InProgress = m_MapGoal;
	}

	void ReviveTeammate::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_MapGoal.reset();

		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(),ReleaseWeaponRequest(GetNameHash()));

		Tracker.Reset();
	}

	State::StateStatus ReviveTeammate::Update(float fDt)
	{
		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		GameEntity reviveEnt = m_MapGoal->GetEntity();

		if(DidPathSucceed())
		{
			Vector3f vEntPos;
			if(!EngineFuncs::EntityPosition(reviveEnt, vEntPos))
				return State_Finished;

			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
			GetClient()->GetSteeringSystem()->SetTarget(m_MapGoal->GetPosition());

			switch(m_GoalState)
			{
			case REVIVING:
				{
					if(InterfaceFuncs::IsAlive(reviveEnt))
						m_GoalState = HEALING;

					GetClient()->GetSteeringSystem()->SetNoAvoidTime(IGame::GetTime() + 1000);
					FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ETQW_WP_NEEDLE));					
					break;
				}				
			case HEALING:
				{
					if(GetClient()->GetTargetingSystem()->HasTarget())
						return State_Finished;

					if(!InterfaceFuncs::IsWeaponCharged(GetClient(), ETQW_WP_HEALTH, Primary))
						return State_Finished;

					Msg_HealthArmor ha;
					if(InterfaceFuncs::GetHealthAndArmor(reviveEnt, ha) && ha.m_CurrentHealth >= ha.m_MaxHealth)
						return State_Finished;

					GetClient()->GetSteeringSystem()->SetNoAvoidTime(IGame::GetTime() + 1000);
					FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ETQW_WP_NEEDLE));
					break;
				}
			}
		}
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

	DefuseDynamite::DefuseDynamite()
		: StateChild("DefuseDynamite")
		, FollowPathUser("DefuseDynamite")
	{
		LimitToClass().SetFlag(ETQW_CLASS_ENGINEER);
	}

	void DefuseDynamite::RenderDebug()
	{
		if(IsActive())
		{
			Utils::OutlineOBB(m_MapGoal->GetWorldBounds(), COLOR::ORANGE, 5.f);
			Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::GREEN,5.f);
		}
	}

	// FollowPathUser functions.
	bool DefuseDynamite::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal && m_MapGoal->RouteTo(GetClient(), _desination, 64.f))
			_final = false;
		else 
			_final = true;
		return true;
	}

	// AimerUser functions.
	bool DefuseDynamite::GetAimPosition(Vector3f &_aimpos)
	{
		_aimpos = m_MapGoal->GetPosition();
		return true;
	}

	void DefuseDynamite::OnTarget()
	{
		FINDSTATE(ws, WeaponSystem, GetRootState());
		if(ws && ws->CurrentWeaponIs(ETQW_WP_PLIERS))
			ws->FireWeapon();
	}

	obReal DefuseDynamite::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();

		m_MapGoal.reset();

		GoalManager::Query qry(0x1899efc7 /* DEFUSE */, GetClient());
		GoalManager::GetInstance()->GetGoals(qry);
		for(obuint32 i = 0; i < qry.m_List.size(); ++i)
		{
			if(BlackboardIsDelayed(qry.m_List[i]->GetSerialNum()))
				continue;

			if(qry.m_List[i]->GetSlotsOpen(MapGoal::TRACK_INPROGRESS) < 1)
				continue;

			if(InterfaceFuncs::GetExplosiveState(GetClient(), qry.m_List[i]->GetEntity()) == XPLO_ARMED)
			{
				m_MapGoal = qry.m_List[i];
				break;
			}
			else
			{
				qry.m_List[i]->SetDeleteMe(true);
			}
		}
		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}

	void DefuseDynamite::Enter()
	{
		m_TargetPosition = m_MapGoal->GetWorldBounds().Center;

		FINDSTATEIF(FollowPath, GetRootState(), Goto(this, Run));

		Tracker.InProgress = m_MapGoal;
	}

	void DefuseDynamite::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_MapGoal.reset();

		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(),ReleaseWeaponRequest(GetNameHash()));

		Tracker.Reset();
	}

	State::StateStatus DefuseDynamite::Update(float fDt)
	{
		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		if(DidPathSucceed())
		{
			ExplosiveState eState = InterfaceFuncs::GetExplosiveState(GetClient(), m_MapGoal->GetEntity());
			switch(eState)
			{
			case XPLO_INVALID:
			case XPLO_UNARMED:
				return State_Finished;
			default:
				break; // keep processing
			}

			m_TargetPosition = m_MapGoal->GetWorldBounds().Center;
			float fDistanceToDynamite = (m_TargetPosition - GetClient()->GetPosition()).SquaredLength();

			if(fDistanceToDynamite > 2500.0f)
			{
				GetClient()->GetSteeringSystem()->SetTarget(m_TargetPosition);
			}
			else
			{
				GetClient()->PressButton(BOT_BUTTON_CROUCH);
				FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
				FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), ETQW_WP_PLIERS));
			}
		}
		return State_Busy;
	}

	//////////////////////////////////////////////////////////////////////////

};
