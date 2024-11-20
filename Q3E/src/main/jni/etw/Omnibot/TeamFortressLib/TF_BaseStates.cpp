////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF.h"
#include "TF_BaseStates.h"
#include "TF_NavigationFlags.h"
#include "TF_Game.h"
#include "ScriptManager.h"
#include "gmScriptGoal.h"

const obReal SENTRY_UPGRADE_PRIORITY = 0.5f;

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////
	namespace TF_Options
	{
		int BUILD_AMMO_TYPE = TF_WP_SPANNER;

		int SENTRY_BUILD_AMMO = 130;
		int SENTRY_UPGRADE_AMMO = 130;
		int SENTRY_REPAIR_AMMO = 20;
		int SENTRY_UPGRADE_WPN = TF_WP_SPANNER;

		int BUILD_ATTEMPT_DELAY = 1000;

		int TELEPORT_BUILD_AMMO = 125;
		int DISPENSER_BUILD_AMMO = 100;

		int PIPE_WEAPON = TF_WP_PIPELAUNCHER;
		int PIPE_WEAPON_WATCH = TF_WP_GRENADE_LAUNCHER;
		int PIPE_AMMO = TF_WP_GRENADE_LAUNCHER;
		int PIPE_MAX_DEPLOYED = 8;

		int ROCKETJUMP_WPN = TF_WP_ROCKET_LAUNCHER;

		float GRENADE_VELOCITY = 500.f;

		bool POLL_SENTRY_STATUS = false;

		bool REPAIR_ON_SABOTAGED = false;
	};
	//////////////////////////////////////////////////////////////////////////
	int SentryBuild::BuildEquipWeapon = TF_WP_NONE;

	SentryBuild::SentryBuild()
		: StateChild("SentryBuild")
		, FollowPathUser("SentryBuild")
		, m_NeedAmmoAmount(0)
		, m_CantBuild(false)
	{
		REGISTER_STATE(SentryBuild,SG_NONE);
		REGISTER_STATE(SentryBuild,SG_GETTING_AMMO);
		REGISTER_STATE(SentryBuild,SG_BUILDING);
		REGISTER_STATE(SentryBuild,SG_AIMING);
		REGISTER_STATE(SentryBuild,SG_AIMED);
		REGISTER_STATE(SentryBuild,SG_UPGRADING);
		REGISTER_STATE(SentryBuild,SG_REPAIRING);
		REGISTER_STATE(SentryBuild,SG_RESUPPLY);
		REGISTER_STATE(SentryBuild,SG_DONE);
	}

	bool SentryBuild::HasEnoughAmmo(int _ammotype, int _ammorequired)
	{
		int iAmmo = 0, iMaxAmmo = 0;
		g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), _ammotype, Primary, iAmmo, iMaxAmmo);
		if(iAmmo < _ammorequired)
		{
			return false;
		}
		return true;
	}

	STATE_ENTER(SentryBuild, SG_NONE) { }
	STATE_EXIT(SentryBuild, SG_NONE) { }
	STATE_UPDATE(SentryBuild, SG_NONE)
	{
		FINDSTATE(sg,Sentry,GetParent());
		if(sg)
		{
			FINDSTATE(sg,Sentry,GetParent());
			if(sg && sg->HasSentry() && sg->SentryFullyBuilt())
			{
				if(sg->GetSentryStatus().m_Level < 3)
				{
					SetNextState(SG_UPGRADING);
					return;
				}

				const Sentry::SentryStatus &ss = sg->GetSentryStatus();
				if((ss.m_Sabotaged && TF_Options::REPAIR_ON_SABOTAGED) ||
					(ss.m_Health < ss.m_MaxHealth))
				{
					SetNextState(SG_REPAIRING);
					return;
				}

				if(ss.m_Rockets[0] < ss.m_Rockets[1]/2 ||
					ss.m_Shells[0] < ss.m_Shells[1]/2)
				{
					SetNextState(SG_RESUPPLY);
					return;
				}
				SetNextState(SG_DONE);
			}
			else
			{
				SetNextState(SG_BUILDING);
			}
		}
	}

	STATE_ENTER(SentryBuild, SG_GETTING_AMMO)
	{
		SensoryMemory *sensory = GetClient()->GetSensoryMemory();
		MemoryRecords records;
		Vector3List recordpos;

		FilterAllType filter(GetClient(), AiState::SensoryMemory::EntAny, records);
		filter.MemorySpan(Utils::SecondsToMilliseconds(7.f));
		filter.AddClass(TF_CLASSEX_RESUPPLY);
		filter.AddClass(TF_CLASSEX_BACKPACK);
		filter.AddClass(TF_CLASSEX_BACKPACK_AMMO);
		sensory->QueryMemory(filter);

		sensory->GetRecordInfo(records, &recordpos, NULL);

		m_AmmoPack.Reset();

		if(!recordpos.empty())
		{
			FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
			FINDSTATEIF(FollowPath,GetRootState(),Goto(this, recordpos));

			const int x = GetDestinationIndex();
			m_AmmoPack = records[x];
		}
		else
		{
			BlackboardDelay(10.f, m_MapGoalSentry->GetSerialNum());
			SetNextState(SG_DONE);
		}
	}
	STATE_EXIT(SentryBuild, SG_GETTING_AMMO)
	{
	}
	STATE_UPDATE(SentryBuild, SG_GETTING_AMMO)
	{
		if(HasEnoughAmmo(TF_Options::BUILD_AMMO_TYPE, m_NeedAmmoAmount))
		{
			SetNextState(SG_NONE);
			return;
		}

		if(m_AmmoPack.IsValid())
		{
			SensoryMemory *sensory = GetClient()->GetSensoryMemory();
			if(sensory)
			{
				const MemoryRecord *mr = sensory->GetMemoryRecord(m_AmmoPack);
				if(!mr)
				{
					m_AmmoPack.Reset();
					SetNextState(SG_GETTING_AMMO);
					return;
				}
				if(mr->m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DISABLED))
				{
					m_AmmoPack.Reset();
					SetNextState(SG_GETTING_AMMO);
					return;
				}
			}
		}

		// check if the position is bad.
	}

	STATE_ENTER(SentryBuild, SG_BUILDING)
	{
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	}
	STATE_EXIT(SentryBuild, SG_BUILDING)
	{
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	}
	STATE_UPDATE(SentryBuild, SG_BUILDING)
	{
		if(GetClient()->HasEntityFlag(TF_ENT_FLAG_BUILDING_SG))
			return;

		FINDSTATE(sg,Sentry,GetParent());
		if(sg && sg->SentryFullyBuilt())
		{
			SetNextState(SG_AIMING);
			return;
		}

		//////////////////////////////////////////////////////////////////////////
		m_NeedAmmoAmount = TF_Options::SENTRY_BUILD_AMMO;
		if(!HasEnoughAmmo(TF_Options::BUILD_AMMO_TYPE, m_NeedAmmoAmount))
		{
			SetNextState(SG_GETTING_AMMO);
			return;
		}
		//////////////////////////////////////////////////////////////////////////

		if(DidPathFail())
		{
			BlackboardDelay(10.f, m_MapGoalSentry->GetSerialNum());
			SetNextState(SG_DONE);
			return;
		}

		if(DidPathSucceed())
		{
			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));

			if(BuildEquipWeapon)
				FINDSTATEIF(WeaponSystem,GetRootState(),AddWeaponRequest(Priority::Medium,GetNameHash(),BuildEquipWeapon));

			if(m_CantBuild)
			{
				GetClient()->SetMovementVector(-m_MapGoalSentry->GetFacing());
				GetClient()->PressButton(BOT_BUTTON_WALK);
				m_NextBuildTry = 0;
			}
			else
				GetClient()->SetMovementVector(Vector3f::ZERO);
		}
	}

	STATE_ENTER(SentryBuild, SG_AIMING)
	{
		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
	}
	STATE_EXIT(SentryBuild, SG_AIMING)
	{
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem,GetRootState(),ReleaseWeaponRequest(GetNameHash()));
	}
	STATE_UPDATE(SentryBuild, SG_AIMING)
	{
	}

	STATE_ENTER(SentryBuild, SG_AIMED)
	{
	}
	STATE_EXIT(SentryBuild, SG_AIMED)
	{
	}
	STATE_UPDATE(SentryBuild, SG_AIMED)
	{
		SetNextState(SG_DONE);
	}

	STATE_ENTER(SentryBuild, SG_UPGRADING)
	{
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	}
	STATE_EXIT(SentryBuild, SG_UPGRADING)
	{
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));
	}
	STATE_UPDATE(SentryBuild, SG_UPGRADING)
	{
		//////////////////////////////////////////////////////////////////////////
		m_NeedAmmoAmount = TF_Options::SENTRY_UPGRADE_AMMO;
		if(!HasEnoughAmmo(TF_Options::BUILD_AMMO_TYPE, m_NeedAmmoAmount))
		{
			SetNextState(SG_GETTING_AMMO);
			return;
		}
		//////////////////////////////////////////////////////////////////////////

		FINDSTATE(sg,Sentry,GetRootState());
		if(sg && sg->GetSentryStatus().m_Level == 3)
		{
			SetNextState(SG_DONE);
			return;
		}

		if(DidPathSucceed())
		{
			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), TF_Options::SENTRY_UPGRADE_WPN));

			FINDSTATE(sg,Sentry,GetRootState());
			if(sg)
				GetClient()->GetSteeringSystem()->SetTarget(sg->GetSentryStatus().m_Position);
		}
	}

	STATE_ENTER(SentryBuild, SG_REPAIRING)
	{
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	}
	STATE_EXIT(SentryBuild, SG_REPAIRING)
	{
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));
	}
	STATE_UPDATE(SentryBuild, SG_REPAIRING)
	{
		//////////////////////////////////////////////////////////////////////////
		m_NeedAmmoAmount = TF_Options::SENTRY_REPAIR_AMMO;
		if(!HasEnoughAmmo(TF_Options::BUILD_AMMO_TYPE, m_NeedAmmoAmount))
		{
			SetNextState(SG_GETTING_AMMO);
			return;
		}
		//////////////////////////////////////////////////////////////////////////

		FINDSTATE(sg,Sentry,GetRootState());
		if(sg && sg->GetSentryStatus().m_Health == sg->GetSentryStatus().m_MaxHealth)
		{
			SetNextState(SG_DONE);
			return;
		}

		if(DidPathSucceed())
		{
			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), TF_Options::SENTRY_UPGRADE_WPN));

			FINDSTATE(sg,Sentry,GetRootState());
			if(sg)
				GetClient()->GetSteeringSystem()->SetTarget(sg->GetSentryStatus().m_Position);
		}
	}

	STATE_ENTER(SentryBuild, SG_RESUPPLY)
	{
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	}
	STATE_EXIT(SentryBuild, SG_RESUPPLY)
	{
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));
	}
	STATE_UPDATE(SentryBuild, SG_RESUPPLY)
	{
		//////////////////////////////////////////////////////////////////////////
		int iShells = 0, iShellsMax = 0;
		int iSpanner = 0, iSpannerMax = 0;
		g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), TF_WP_SHOTGUN, Primary, iShells, iShellsMax);
		g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), TF_WP_SPANNER, Primary, iSpanner, iSpannerMax);
		if(iSpanner == 0 || iShells == 0)
		{
			m_NeedAmmoAmount = 200;
			SetNextState(SG_GETTING_AMMO);
			return;
		}
		//////////////////////////////////////////////////////////////////////////

		FINDSTATE(sg,Sentry,GetRootState());
		if(sg)
		{
			if(sg->GetSentryStatus().m_Shells[0] == sg->GetSentryStatus().m_Shells[1] &&
				sg->GetSentryStatus().m_Rockets[0] == sg->GetSentryStatus().m_Rockets[1])
			{
				SetNextState(SG_DONE);
				return;
			}
		}

		if(DidPathSucceed())
		{
			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), TF_Options::SENTRY_UPGRADE_WPN));

			FINDSTATE(sg,Sentry,GetRootState());
			if(sg)
				GetClient()->GetSteeringSystem()->SetTarget(sg->GetSentryStatus().m_Position);
		}
	}

	STATE_ENTER(SentryBuild, SG_DONE) { }
	STATE_EXIT(SentryBuild, SG_DONE) { }
	STATE_UPDATE(SentryBuild, SG_DONE) { }

	void SentryBuild::GetDebugString(StringStr &out)
	{
		if(IsActive())
		{
			switch(GetCurrentStateId())
			{
			case SG_NONE:
				break;
			case SG_GETTING_AMMO:
				out << "Getting Ammo ";
				break;
			case SG_BUILDING:
				out << (m_CantBuild ? "Cant Build " : "Building ");
				break;
			case SG_AIMING:
				out << "Aiming ";
				break;
			case SG_AIMED:
				out << "Aimed ";
				break;
			case SG_UPGRADING:
				out << "Upgrading ";
				break;
			case SG_REPAIRING:
				out << "Repairing ";
				break;
			case SG_RESUPPLY:
				out << "Resupply ";
				break;
			case SG_DONE:
				out << "Done ";
				break;
			}

			if(m_BuiltSentry)
				out << m_BuiltSentry->GetName() << " ";
			else if(m_MapGoalSentry)
				out << m_MapGoalSentry->GetName() << " ";

			FINDSTATE(sg,Sentry,GetParent());
			if(sg && sg->HasSentry() && sg->SentryFullyBuilt())
			{
				const Sentry::SentryStatus &ss = sg->GetSentryStatus();
				const float hlthpc = 100.f * ((float)ss.m_Health / (float)ss.m_MaxHealth);

				out << std::setprecision(3);
				out << "H(" << hlthpc << ") ";
				out << "S(" << ss.m_Shells[0] << "/" << ss.m_Shells[1] << ") ";
				out << "R(" << ss.m_Rockets[0] << "/" << ss.m_Rockets[1] << ") ";
			}
		}
	}
	bool SentryBuild::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		FINDSTATE(sg,Sentry,GetParent());
		if(sg && sg->HasSentry())
		{
			_desination.push_back(Destination(sg->GetSentryStatus().m_Position, 64.f));
			_final = true;
		}
		else
		{
			if(m_MapGoalSentry->RouteTo(GetClient(), _desination))
				_final = false;
			else
				_final = true;
		}
		return true;
	}
	bool SentryBuild::GetAimPosition(Vector3f &_aimpos)
	{
		switch(GetCurrentStateId())
		{
		case SG_BUILDING:
			{
				_aimpos = GetClient()->GetEyePosition() + m_MapGoalSentry->GetFacing() * 1024.f;
				break;
			}
		case SG_AIMING:
			{
				_aimpos = GetClient()->GetEyePosition() - m_MapGoalSentry->GetFacing() * 1024.f;

				Vector3f vAimPos;
				if(m_MapGoalSentry->GetProperty("AimPosition", vAimPos))
				{
					if(!vAimPos.IsZero())
						_aimpos = vAimPos;
					else
						SetNextState(SG_AIMED);
				}
				break;
			}
		case SG_UPGRADING:
		case SG_REPAIRING:
		case SG_RESUPPLY:
			{
				FINDSTATE(sg,Sentry,GetParent());
				if(sg)
				{
					_aimpos = sg->GetSentryStatus().m_Position;
					return true;
				}
			}
		default:
			OBASSERT(0,"Invalid State");
		}
		return true;
	}
	void SentryBuild::OnTarget()
	{
		FINDSTATE(sg,Sentry,GetParent());
		if(!sg)
			return;

		switch(GetCurrentStateId())
		{
		case SG_BUILDING:
			{
				if(!sg->HasSentry() && IGame::GetTime() >= m_NextBuildTry)
				{
					GetClient()->PressButton(TF_BOT_BUTTON_BUILDSENTRY);
					m_NextBuildTry = IGame::GetTime() + TF_Options::BUILD_ATTEMPT_DELAY;
				}
				break;
			}
		case SG_AIMING:
			{
				GetClient()->PressButton(TF_BOT_BUTTON_AIMSENTRY);
				SetNextState(SG_AIMED);
				break;
			}
		case SG_UPGRADING:
		case SG_REPAIRING:
		case SG_RESUPPLY:
			{
				FINDSTATE(ws, WeaponSystem, GetRootState());
				if(ws && ws->CurrentWeaponIs(TF_Options::SENTRY_UPGRADE_WPN))
					ws->FireWeapon();
				break;
			}
		default:
			OBASSERT(0,"Invalid State");
		}
	}
	obReal SentryBuild::GetPriority()
	{
		// If we built somewhere, but that spot is now unavailable, abandon it.
		if(m_BuiltSentry)
		{
			if(!m_BuiltSentry->IsAvailable(GetClient()->GetTeam()))
			{
				GetClient()->PressButton(TF_BOT_BUTTON_DETSENTRY);
				m_BuiltSentry.reset();
			}
		}

		if(!m_MapGoalSentry)
		{
			FINDSTATE(sg,Sentry,GetParent());
			if(sg && sg->HasSentry() && sg->SentryFullyBuilt())
			{
				if(IsActive())
					return m_SentryPriority;

				if(sg->GetSentryStatus().m_Level < 3)
				{
					return m_SentryPriority;
				}

				if(sg->GetSentryStatus().m_Health < sg->GetSentryStatus().m_MaxHealth)
				{
					return m_SentryPriority;
				}

				const Sentry::SentryStatus &status = sg->GetSentryStatus();
				if(status.m_Rockets[0] < status.m_Rockets[1]/2 ||
					status.m_Shells[0] < status.m_Shells[1]/2)
				{
					return m_SentryPriority;
				}
				return 0.f;
			}

			GoalManager::Query qry(0xca96590a /* sentry */, GetClient());
			GoalManager::GetInstance()->GetGoals(qry);
			qry.GetBest(m_MapGoalSentry);
		}
		m_SentryPriority = m_MapGoalSentry ? m_MapGoalSentry->GetPriorityForClient(GetClient()) : 0.f;
		return m_SentryPriority;
	}
	void SentryBuild::Enter()
	{
		OBASSERT(m_MapGoalSentry||m_BuiltSentry, "No Map Goal!");

		m_NextBuildTry = 0;
		m_CantBuild = false;

		SetNextState(SG_NONE);
	}
	void SentryBuild::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		SetNextState(SG_NONE);
		UpdateFsm(0.f);

		m_MapGoalSentry.reset();
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		GetClient()->PressButton(TF_BOT_BUTTON_CANCELBUILD);
	}
	State::StateStatus SentryBuild::Update(float fDt)
	{
		OBASSERT(m_MapGoalSentry||m_BuiltSentry, "No Map Goal!");

		UpdateFsm(fDt);

		if(GetCurrentStateId()==SG_DONE)
			return State_Finished;

		if(DidPathFail())
		{
			// Delay it from being used for a while.
			if(m_MapGoalSentry)
				BlackboardDelay(10.f, m_MapGoalSentry->GetSerialNum());
			return State_Finished;
		}

		return State_Busy;
	}
	void SentryBuild::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(TF_MSG_SENTRY_BUILDING)
			{
				if(m_MapGoalSentry)
					m_BuiltSentry = m_MapGoalSentry;
				break;
			}
			HANDLER(TF_MSG_SENTRY_BUILT)
			{
				if(m_MapGoalSentry)
					m_BuiltSentry = m_MapGoalSentry;
				break;
			}
			HANDLER(TF_MSG_SENTRY_BUILDCANCEL)
			{
				m_BuiltSentry.reset();
				break;
			}
			HANDLER(TF_MSG_SENTRY_DESTROYED)
			{
				m_BuiltSentry.reset();
				break;
			}
			HANDLER(TF_MSG_SENTRY_DETONATED)
			{
				m_BuiltSentry.reset();
				break;
			}
			HANDLER(TF_MSG_SENTRY_DISMANTLED)
			{
				m_BuiltSentry.reset();
				break;
			}
			HANDLER(TF_MSG_SENTRY_CANTBUILD)
			{
				m_CantBuild = true;
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	SentryAlly::SentryAlly()
		: StateChild("SentryAlly")
		, FollowPathUser("SentryAlly")
	{
	}
	void SentryAlly::GetDebugString(StringStr &out)
	{
		if(IsActive())
		{
			switch(m_State)
			{
			case UPGRADING:
				out << "Upgrading Sentry ";
				break;
			case REPAIRING:
				out << "Repairing Sentry ";
				break;
			case RESUPPLY:
				out << "Resupply Sentry ";
				break;
			}

			const MemoryRecord *rec = GetClient()->GetSensoryMemory()->GetMemoryRecord(m_AllySentry);
			if(rec)
			{
				GameEntity owner = EngineFuncs::EntityOwner(rec->GetEntity());
				if(owner.IsValid())
				{
					String ownerName = EngineFuncs::EntityName(owner);
					if(!ownerName.empty())
						out << "Owner: " << ownerName;
				}
			}
		}
	}
	bool SentryAlly::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		const MemoryRecord *rec = GetClient()->GetSensoryMemory()->GetMemoryRecord(m_AllySentry);
		if(rec)
		{
			_desination.push_back(Destination(rec->GetLastSensedPosition(), 64.f));
			_final = true;
			return true;
		}
		return false;
	}
	bool SentryAlly::GetAimPosition(Vector3f &_aimpos)
	{
		const MemoryRecord *rec = GetClient()->GetSensoryMemory()->GetMemoryRecord(m_AllySentry);
		_aimpos = rec->GetLastSensedPosition();
		return rec != NULL;
	}
	void SentryAlly::OnTarget()
	{
		FINDSTATE(ws, WeaponSystem, GetRootState());
		if(ws && ws->CurrentWeaponIs(TF_Options::SENTRY_UPGRADE_WPN))
			ws->FireWeapon();
	}
	obReal SentryAlly::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();

		FINDSTATE(sensory,SensoryMemory,GetRootState());
		if(sensory)
		{
			FINDSTATE(sg,Sentry,GetParent());

			int iAmmo = 0, iMaxAmmo = 0;
			g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), TF_Options::BUILD_AMMO_TYPE, Primary, iAmmo, iMaxAmmo);

			MemoryRecords rcs;
			FilterAllType filter(GetClient(), AiState::SensoryMemory::EntAny, rcs);
			filter.MemorySpan(Utils::SecondsToMilliseconds(7.f));
			filter.AddClass(TF_CLASSEX_SENTRY);
			sensory->QueryMemory(filter);

			for(obuint32 i = 0; i < rcs.size(); ++i)
			{
				const MemoryRecord *pRec = sensory->GetMemoryRecord(rcs[i]);
				if(pRec)
				{
					if(sg && sg->HasSentry() && sg->GetSentryStatus().m_Entity == pRec->GetEntity())
						continue;

					const BitFlag64 entflags = pRec->m_TargetInfo.m_EntityFlags;
					if(!entflags.CheckFlag(TF_ENT_FLAG_BUILDINPROGRESS) &&
						!entflags.CheckFlag(TF_ENT_FLAG_LEVEL3) &&
						iAmmo >= TF_Options::SENTRY_UPGRADE_AMMO)
					{
						m_State = UPGRADING;
						m_AllySentry = rcs[i];
						return GetLastPriority();
					}

					Msg_HealthArmor hlth;
					if(InterfaceFuncs::GetHealthAndArmor(pRec->GetEntity(), hlth) &&
						hlth.m_CurrentHealth <= hlth.m_MaxHealth/2 &&
						iAmmo >= TF_Options::SENTRY_REPAIR_AMMO)
					{
						m_State = REPAIRING;
						m_AllySentry = rcs[i];
						return GetLastPriority();
					}
				}
			}
		}
		return 0.f;
	}
	void SentryAlly::Enter()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	}
	void SentryAlly::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));
	}
	State::StateStatus SentryAlly::Update(float fDt)
	{
		//////////////////////////////////////////////////////////////////////////
		if(DidPathFail())
		{
			// Delay it from being used for a while.
			//BlackboardDelay(10.f, m_MapGoalSentry->GetSerialNum());
			return State_Finished;
		}
		//////////////////////////////////////////////////////////////////////////

		// Abandon ally upgrade if it gets upgraded to l3 in the mean time.
		const MemoryRecord *rec = GetClient()->GetSensoryMemory()->GetMemoryRecord(m_AllySentry);
		if(!rec)
			return State_Finished;

		int iAmmo = 0, iMaxAmmo = 0;
		g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), TF_Options::BUILD_AMMO_TYPE, Primary, iAmmo, iMaxAmmo);

		switch(m_State)
		{
			case UPGRADING:
				{
					if(iAmmo < TF_Options::SENTRY_UPGRADE_AMMO)
						return State_Finished;

					if(rec->m_TargetInfo.m_EntityFlags.CheckFlag(TF_ENT_FLAG_LEVEL3))
						return State_Finished;
					break;
				}
			case REPAIRING:
				{
					if(iAmmo < TF_Options::SENTRY_REPAIR_AMMO)
						return State_Finished;

					Msg_HealthArmor hlth;
					if(InterfaceFuncs::GetHealthAndArmor(rec->GetEntity(), hlth) &&
						hlth.m_CurrentHealth == hlth.m_MaxHealth)
					{
						return State_Finished;
					}
					break;
				}
			case RESUPPLY:
				{
					break;
				}
		}

		if(DidPathSucceed())
		{
			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
			FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Medium, GetNameHash(), TF_Options::SENTRY_UPGRADE_WPN));

			GetClient()->GetSteeringSystem()->SetTarget(rec->m_TargetInfo.m_LastPosition);
		}
		return State_Busy;
	}
	//////////////////////////////////////////////////////////////////////////
	int DispenserBuild::BuildEquipWeapon = TF_WP_NONE;

	DispenserBuild::DispenserBuild()
		: StateChild("DispenserBuild")
		, FollowPathUser("DispenserBuild")
		, m_CantBuild(false)
	{
	}
	void DispenserBuild::GetDebugString(StringStr &out)
	{
		if(IsActive())
		{
			switch(m_State)
			{
			default:
			case DISP_NONE:
				break;
			case DISP_GETTING_AMMO:
				out << "Getting Ammo";
				break;
			case DISP_BUILDING:
				out << (m_CantBuild ? "Cant Build " : "Building ");
				if(m_MapGoalDisp)
					out << m_MapGoalDisp->GetName();
				break;
			}
		}
	}
	bool DispenserBuild::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoalDisp->RouteTo(GetClient(), _desination))
			_final = false;
		else
			_final = true;
		return true;
	}
	bool DispenserBuild::GetAimPosition(Vector3f &_aimpos)
	{
		switch(m_State)
		{
		case DISP_BUILDING:
			_aimpos = GetClient()->GetEyePosition() + m_MapGoalDisp->GetFacing() * 1024.f;
			break;
		default:
			OBASSERT(0,"Invalid State");
		}
		return true;
	}
	void DispenserBuild::OnTarget()
	{
		FINDSTATE(disp,Dispenser,GetParent());
		if(!disp)
			return;

		switch(m_State)
		{
		case DISP_BUILDING:
			{
				if(!disp->HasDispenser() && IGame::GetTime() >= m_NextBuildTry)
				{
					GetClient()->PressButton(TF_BOT_BUTTON_BUILDDISPENSER);
					m_NextBuildTry = IGame::GetTime() + TF_Options::BUILD_ATTEMPT_DELAY;
				}
				break;
			}
		default:
			OBASSERT(0,"Invalid State");
		}
	}
	obReal DispenserBuild::GetPriority()
	{
		if(m_BuiltDisp)
		{
			if(!m_BuiltDisp->IsAvailable(GetClient()->GetTeam()))
			{
				GetClient()->PressButton(TF_BOT_BUTTON_DETDISPENSER);
				m_BuiltDisp.reset();
			}
		}

		if(!m_MapGoalDisp)
		{
			FINDSTATE(disp,Dispenser,GetParent());
			if(disp && disp->HasDispenser())
				return 0.f;

			GoalManager::Query qry(0x4961e1a8 /* dispenser */, GetClient());
			GoalManager::GetInstance()->GetGoals(qry);
			qry.GetBest(m_MapGoalDisp);
		}
		return m_MapGoalDisp ? m_MapGoalDisp->GetPriorityForClient(GetClient()) : 0.f;
	}
	void DispenserBuild::Enter()
	{
		m_NextAmmoCheck = 0;
		m_NextBuildTry = 0;
		m_State = DISP_BUILDING;
		m_CantBuild = false;
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	}
	void DispenserBuild::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));
		m_State = DISP_NONE;
		m_MapGoalDisp.reset();
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		GetClient()->PressButton(TF_BOT_BUTTON_CANCELBUILD);
		FINDSTATEIF(WeaponSystem,GetRootState(),ReleaseWeaponRequest(GetNameHash()));
	}
	State::StateStatus DispenserBuild::Update(float fDt)
	{
		OBASSERT(m_MapGoalDisp, "No Map Goal!");
		//////////////////////////////////////////////////////////////////////////
		if(DidPathFail())
		{
			// Delay it from being used for a while.
			BlackboardDelay(10.f, m_MapGoalDisp->GetSerialNum());
			return State_Finished;
		}
		//////////////////////////////////////////////////////////////////////////
		if(GetClient()->HasEntityFlag(TF_ENT_FLAG_BUILDING_DISP))
			return State_Busy;

		FINDSTATE(disp,Dispenser,GetParent());
		if(disp && disp->DispenserFullyBuilt())
		{
			m_BuiltDisp = m_MapGoalDisp;
			return State_Finished;
		}
		//////////////////////////////////////////////////////////////////////////
		// Need ammo?
		if(IGame::GetTime() >= m_NextAmmoCheck)
		{
			m_NextAmmoCheck = IGame::GetTime() + 1000;

			int iAmmo = 0, iMaxAmmo = 0;
			g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), TF_Options::BUILD_AMMO_TYPE, Primary, iAmmo, iMaxAmmo);
			if(m_State == DISP_GETTING_AMMO && iAmmo >= TF_Options::DISPENSER_BUILD_AMMO)
				return State_Finished;
			if(m_State != DISP_GETTING_AMMO && iAmmo < TF_Options::DISPENSER_BUILD_AMMO)
			{
				SensoryMemory *sensory = GetClient()->GetSensoryMemory();

				MemoryRecords records;
				Vector3List recordpos;

				FilterAllType filter(GetClient(), AiState::SensoryMemory::EntAny, records);
				filter.MemorySpan(Utils::SecondsToMilliseconds(7.f));
				filter.AddClass(TF_CLASSEX_RESUPPLY);
				filter.AddClass(TF_CLASSEX_BACKPACK);
				filter.AddClass(TF_CLASSEX_BACKPACK_AMMO);
				sensory->QueryMemory(filter);

				sensory->GetRecordInfo(records, &recordpos, NULL);

				if(!recordpos.empty())
				{
					m_State = DISP_GETTING_AMMO;
					FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
					FINDSTATEIF(FollowPath, GetRootState(), Goto(this, recordpos));
				}
				else
				{
					//BlackboardDelay(10.f, m_MapGoalSentry->GetSerialNum());
					return State_Finished;
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////
		if(m_State == DISP_BUILDING && !m_MapGoalDisp->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		if(DidPathSucceed())
		{
			if(m_State == DISP_GETTING_AMMO)
				return State_Finished;

			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
			if(BuildEquipWeapon)
				FINDSTATEIF(WeaponSystem,GetRootState(),AddWeaponRequest(Priority::Medium,GetNameHash(),BuildEquipWeapon));

			if(m_CantBuild)
			{
				GetClient()->SetMovementVector(-m_MapGoalDisp->GetFacing());
				GetClient()->PressButton(BOT_BUTTON_WALK);
				m_NextBuildTry = 0;
			}
			else
				GetClient()->SetMovementVector(Vector3f::ZERO);
		}
		return State_Busy;
	}
	void DispenserBuild::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(TF_MSG_DISPENSER_BUILDING)
			{
				if(m_MapGoalDisp)
					m_BuiltDisp = m_MapGoalDisp;
				break;
			}
			HANDLER(TF_MSG_DISPENSER_BUILT)
			{
				if(m_MapGoalDisp)
					m_BuiltDisp = m_MapGoalDisp;
				break;
			}
			HANDLER(TF_MSG_DISPENSER_BUILDCANCEL)
			{
				m_BuiltDisp.reset();
				break;
			}
			HANDLER(TF_MSG_DISPENSER_DESTROYED)
			{
				m_BuiltDisp.reset();
				break;
			}
			HANDLER(TF_MSG_DISPENSER_DETONATED)
			{
				m_BuiltDisp.reset();
				break;
			}
			HANDLER(TF_MSG_DISPENSER_DISMANTLED)
			{
				m_BuiltDisp.reset();
				break;
			}
			HANDLER(TF_MSG_DISPENSER_CANTBUILD)
			{
				m_CantBuild = true;
				break;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	Sentry::Sentry() : StatePrioritized("Sentry")
	{
		AppendState(new SentryBuild);
		AppendState(new SentryAlly);

		LimitToClass().SetFlag(TF_CLASS_ENGINEER);
		SetAlwaysRecieveEvents(true);
	}
	void Sentry::GetDebugString(StringStr &out)
	{
		switch(m_SentryStatus.m_Status)
		{
		case BUILDABLE_BUILDING:
		case BUILDABLE_BUILT:
			out << m_SentryStatus.m_Health << "/" << m_SentryStatus.m_MaxHealth;
			break;
		case BUILDABLE_INVALID:
		default:
			out << "<none>";
			break;
		}
	}
	void Sentry::UpdateSentryStatus(const Event_SentryStatus_TF &_stats)
	{
		if(_stats.m_Entity.IsValid())
		{
			if(m_SentryStatus.m_Entity != _stats.m_Entity)
			{
				Event_SentryBuilding_TF d1 = { _stats.m_Entity };
				GetClient()->SendEvent(MessageHelper(TF_MSG_SENTRY_BUILDING, &d1, sizeof(d1)));

				Event_SentryBuilt_TF d2 = { _stats.m_Entity };
				GetClient()->SendEvent(MessageHelper(TF_MSG_SENTRY_BUILT, &d2, sizeof(d2)));
			}

			Event_SentryStatus_TF d3 = _stats;
			GetClient()->SendEvent(MessageHelper(TF_MSG_SENTRY_STATS, &d3, sizeof(d3)));
		}
		else
		{
			if(m_SentryStatus.m_Entity != _stats.m_Entity)
			{
				Event_BuildableDestroyed_TF d = { GameEntity() };
				GetClient()->SendEvent(MessageHelper(TF_MSG_SENTRY_DESTROYED, &d, sizeof(d)));
			}
		}
	}
	State::StateStatus Sentry::Update(float fDt)
	{
		return State_Busy;
	}
	void Sentry::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(TF_MSG_SENTRY_BUILDING)
			{
				const Event_SentryBuilding_TF *m = _message.Get<Event_SentryBuilding_TF>();
				OBASSERT(m->m_Sentry.IsValid(), "Entity Expected");
				m_SentryStatus.Reset();
				m_SentryStatus.m_Status = BUILDABLE_BUILDING;
				m_SentryStatus.m_Entity = m->m_Sentry;
				DBG_MSG(0, GetClient(), kNormal, "Sentry Building");
				break;
			}
			HANDLER(TF_MSG_SENTRY_BUILT)
			{
				const Event_SentryBuilt_TF *m = _message.Get<Event_SentryBuilt_TF>();
				OBASSERT(m->m_Sentry.IsValid(), "Entity Expected");
				m_SentryStatus.m_Entity = m->m_Sentry;
				m_SentryStatus.m_Level = 1;

				EngineFuncs::EntityPosition(m->m_Sentry, m_SentryStatus.m_Position);
				EngineFuncs::EntityOrientation(m->m_Sentry, m_SentryStatus.m_Facing, NULL, NULL);
				m_SentryStatus.m_Status = BUILDABLE_BUILT;
				DBG_MSG(0, GetClient(), kNormal, "Sentry Built");
				break;
			}
			HANDLER(TF_MSG_SENTRY_BUILDCANCEL)
			{
				m_SentryStatus.Reset();
				break;
			}
			HANDLER(TF_MSG_SENTRY_DESTROYED)
			{
				m_SentryStatus.Reset();
				DBG_MSG(0, GetClient(), kNormal, "Sentry Destroyed");
				break;
			}
			HANDLER(TF_MSG_SENTRY_DETONATED)
			{
				m_SentryStatus.Reset();
				DBG_MSG(0, GetClient(), kNormal, "Sentry Detonated");
				break;
			}
			HANDLER(TF_MSG_SENTRY_DISMANTLED)
			{
				m_SentryStatus.Reset();
				DBG_MSG(0, GetClient(), kNormal, "Sentry Dismantled");
				break;
			}
			HANDLER(TF_MSG_SENTRY_STATS)
			{
				const Event_SentryStatus_TF		*m = _message.Get<Event_SentryStatus_TF>();
				m_SentryStatus.m_Health			= m->m_Health;
				m_SentryStatus.m_MaxHealth		= m->m_MaxHealth;
				m_SentryStatus.m_Shells[0]		= m->m_Shells[0];
				m_SentryStatus.m_Shells[1]		= m->m_Shells[1];
				m_SentryStatus.m_Rockets[0]		= m->m_Rockets[0];
				m_SentryStatus.m_Rockets[1]		= m->m_Rockets[1];
				m_SentryStatus.m_Level			= m->m_Level;

				/*DBG_MSG(0, GetClient(), kNormal, "Sentry Stats");
				DBG_MSG(0, GetClient(), kNormal, va("Level: %d", m_SentryStatus.m_Level));
				DBG_MSG(0, GetClient(), kNormal, va("Health: %d/%d", m_SentryStatus.m_Health, m_SentryStatus.m_MaxHealth));
				DBG_MSG(0, GetClient(), kNormal, va("Shells: %d/%d", m_SentryStatus.m_Shells[0], m_SentryStatus.m_Shells[1]));
				DBG_MSG(0, GetClient(), kNormal, va("Rockets: %d/%d", m_SentryStatus.m_Rockets[0], m_SentryStatus.m_Rockets[1]));*/
				break;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	Dispenser::Dispenser() : StatePrioritized("Dispenser")
	{
		AppendState(new DispenserBuild);

		LimitToClass().SetFlag(TF_CLASS_ENGINEER);
		SetAlwaysRecieveEvents(true);
	}
	void Dispenser::GetDebugString(StringStr &out)
	{
		switch(m_DispenserStatus.m_Status)
		{
		case BUILDABLE_BUILDING:
		case BUILDABLE_BUILT:
			out << m_DispenserStatus.m_Health;
			return;
		case BUILDABLE_INVALID:
		default:
			out << "<none>";
			break;
		}
	}
	void Dispenser::UpdateDispenserStatus(const Event_DispenserStatus_TF &_stats)
	{
		if(_stats.m_Entity.IsValid())
		{
			if(m_DispenserStatus.m_Entity != _stats.m_Entity)
			{
				Event_DispenserBuilding_TF d1 = { _stats.m_Entity };
				GetClient()->SendEvent(MessageHelper(TF_MSG_DISPENSER_BUILDING, &d1, sizeof(d1)));

				Event_DispenserBuilt_TF d2 = { _stats.m_Entity };
				GetClient()->SendEvent(MessageHelper(TF_MSG_DISPENSER_BUILT, &d2, sizeof(d2)));
			}

			Event_DispenserStatus_TF d3 = _stats;
			GetClient()->SendEvent(MessageHelper(TF_MSG_DISPENSER_STATS, &d3, sizeof(d3)));
		}
		else
		{
			if(m_DispenserStatus.m_Entity != _stats.m_Entity)
			{
				Event_BuildableDestroyed_TF d = { GameEntity() };
				GetClient()->SendEvent(MessageHelper(TF_MSG_DISPENSER_DESTROYED, &d, sizeof(d)));
			}
		}
	}
	State::StateStatus Dispenser::Update(float fDt)
	{
		return State_Busy;
	}
	void Dispenser::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(TF_MSG_DISPENSER_BUILDING)
			{
				const Event_DispenserBuilding_TF *m = _message.Get<Event_DispenserBuilding_TF>();
				OBASSERT(m->m_Dispenser.IsValid(), "Entity Expected");
				m_DispenserStatus.Reset();
				m_DispenserStatus.m_Status = BUILDABLE_BUILDING;
				m_DispenserStatus.m_Entity = m->m_Dispenser;
				DBG_MSG(0, GetClient(), kNormal, "Dispenser Building");
				break;
			}
			HANDLER(TF_MSG_DISPENSER_BUILT)
			{
				const Event_DispenserBuilt_TF *m = _message.Get<Event_DispenserBuilt_TF>();
				OBASSERT(m->m_Dispenser.IsValid(), "Entity Expected");
				m_DispenserStatus.m_Entity = m->m_Dispenser;
				EngineFuncs::EntityPosition(m->m_Dispenser, m_DispenserStatus.m_Position);
				EngineFuncs::EntityOrientation(m->m_Dispenser, m_DispenserStatus.m_Facing, NULL, NULL);
				m_DispenserStatus.m_Status = BUILDABLE_BUILT;
				DBG_MSG(0, GetClient(), kNormal, "Dispenser Built");
				break;
			}
			HANDLER(TF_MSG_DISPENSER_BUILDCANCEL)
			{
				m_DispenserStatus.Reset();
				break;
			}
			HANDLER(TF_MSG_DISPENSER_DESTROYED)
			{
				m_DispenserStatus.Reset();
				DBG_MSG(0, GetClient(), kNormal, "Dispenser Destroyed");
				break;
			}
			HANDLER(TF_MSG_DISPENSER_DETONATED)
			{
				m_DispenserStatus.Reset();
				DBG_MSG(0, GetClient(), kNormal, "Dispenser Detonated");
				break;
			}
			HANDLER(TF_MSG_DISPENSER_DISMANTLED)
			{
				m_DispenserStatus.Reset();
				DBG_MSG(0, GetClient(), kNormal, "Dispenser Dismantled");
				break;
			}
			HANDLER(TF_MSG_DISPENSER_STATS)
			{
				const Event_DispenserStatus_TF	*m = _message.Get<Event_DispenserStatus_TF>();
				m_DispenserStatus.m_Health		= m->m_Health;
				m_DispenserStatus.m_Shells		= m->m_Shells;
				m_DispenserStatus.m_Nails		= m->m_Nails;
				m_DispenserStatus.m_Rockets		= m->m_Rockets;
				m_DispenserStatus.m_Cells		= m->m_Cells;
				m_DispenserStatus.m_Armor		= m->m_Armor;

				/*DBG_MSG(0, GetClient(), kNormal, "Dispenser Stats");
				DBG_MSG(0, GetClient(), kNormal, va("Health: %d", m_DispenserStatus.m_Health));
				DBG_MSG(0, GetClient(), kNormal, va("Shells: %d", m_DispenserStatus.m_Shells));
				DBG_MSG(0, GetClient(), kNormal, va("Nails: %d", m_DispenserStatus.m_Nails));
				DBG_MSG(0, GetClient(), kNormal, va("Rockets: %d", m_DispenserStatus.m_Rockets));
				DBG_MSG(0, GetClient(), kNormal, va("Cells: %d", m_DispenserStatus.m_Cells));
				DBG_MSG(0, GetClient(), kNormal, va("Armor: %d", m_DispenserStatus.m_Armor));*/
				break;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	DetpackBuild::DetpackBuild()
		: StateChild("DetpackBuild")
		, FollowPathUser("DetpackBuild")
		, m_DetpackFuse(TF_BOT_BUTTON_BUILDDETPACK_5)
		, m_CantBuild(false)
	{
	}
	void DetpackBuild::GetDebugString(StringStr &out)
	{
		if(IsActive())
		{
			switch(m_State)
			{
			case NONE:
				break;
			case DETPACK_BUILDING:
				out << (m_CantBuild ? "Cant Build " : "Building ");
				if(m_MapGoal)
					out << m_MapGoal->GetName();
			}
		}
	}
	bool DetpackBuild::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(m_MapGoal->RouteTo(GetClient(), _desination))
			_final = false;
		else
			_final = true;
		return true;
	}
	bool DetpackBuild::GetAimPosition(Vector3f &_aimpos)
	{
		switch(m_State)
		{
		case DETPACK_BUILDING:
			_aimpos = GetClient()->GetEyePosition() + m_MapGoal->GetFacing() * 1024.f;
			break;
		default:
			OBASSERT(0,"Invalid State");
		}
		return true;
	}
	void DetpackBuild::OnTarget()
	{
		FINDSTATE(detp,Detpack,GetParent());
		if(!detp)
			return;

		switch(m_State)
		{
		case DETPACK_BUILDING:
			{
				if(!detp->HasDetpack() && IGame::GetTime() >= m_NextBuildTry)
				{
					GetClient()->PressButton(TF_BOT_BUTTON_BUILDDETPACK_5);
					m_NextBuildTry = IGame::GetTime() + TF_Options::BUILD_ATTEMPT_DELAY;
				}
				break;
			}
		default:
			OBASSERT(0,"Invalid State");
		}
	}
	obReal DetpackBuild::GetPriority()
	{
		if(!m_MapGoal)
		{
			FINDSTATE(detp,Detpack,GetParent());
			if(detp && detp->HasDetpack())
				return 0.f;

			int iDetpacks = 0, iMaxDetpacks = 0;
			g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), TF_WP_DETPACK, Primary, iDetpacks, iMaxDetpacks);
			if(iDetpacks==0)
				return 0.f;

			GoalManager::Query qry(0x3b15b60b /* detpack */, GetClient());
			GoalManager::GetInstance()->GetGoals(qry);
			qry.GetBest(m_MapGoal);
		}
		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}
	void DetpackBuild::Enter()
	{
		m_NextBuildTry = 0;
		m_State = DETPACK_BUILDING;
		m_CantBuild = false;

		float fuse = 5.f;
		if(m_MapGoal->GetProperty("Fuse",fuse))
		{
			if(fuse>30.f)
				m_DetpackFuse = TF_BOT_BUTTON_BUILDDETPACK_30;
			else if(fuse>20.f)
				m_DetpackFuse = TF_BOT_BUTTON_BUILDDETPACK_20;
			else if(fuse>10.f)
				m_DetpackFuse = TF_BOT_BUTTON_BUILDDETPACK_10;
			else
				m_DetpackFuse = TF_BOT_BUTTON_BUILDDETPACK_5;
		}

		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	}
	void DetpackBuild::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));
		m_State = NONE;
		m_MapGoal.reset();
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		GetClient()->PressButton(TF_BOT_BUTTON_CANCELBUILD);
	}
	State::StateStatus DetpackBuild::Update(float fDt)
	{
		OBASSERT(m_MapGoal, "No Map Goal!");
		//////////////////////////////////////////////////////////////////////////
		if(DidPathFail())
		{
			// Delay it from being used for a while.
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}

		//////////////////////////////////////////////////////////////////////////
		if(m_State == DETPACK_BUILDING && !m_MapGoal->IsAvailable(GetClient()->GetTeam()))
			return State_Finished;

		if(DidPathSucceed())
		{
			FINDSTATE(detp,Detpack,GetParent());
			if(detp && detp->DetpackFullyBuilt())
				return State_Finished;

			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));

			if(m_CantBuild)
			{
				GetClient()->SetMovementVector(-m_MapGoal->GetFacing());
				GetClient()->PressButton(BOT_BUTTON_WALK);
				m_NextBuildTry = 0;
			}
			else
				GetClient()->SetMovementVector(Vector3f::ZERO);
		}
		return State_Busy;
	}
	void DetpackBuild::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(TF_MSG_DETPACK_CANTBUILD)
			{
				m_CantBuild = true;
				break;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////

	Detpack::Detpack() : StatePrioritized("Detpack")
	{
		AppendState(new DetpackBuild);

		LimitToClass().SetFlag(TF_CLASS_DEMOMAN);
		SetAlwaysRecieveEvents(true);
	}

	void Detpack::UpdateDetpackStatus(GameEntity _ent)
	{
		m_DetpackStatus.m_Entity = _ent;
		if(m_DetpackStatus.m_Entity.IsValid())
		{
			m_DetpackStatus.m_Status = BUILDABLE_BUILT;
		}
		else
		{
			m_DetpackStatus.m_Status = BUILDABLE_INVALID;
		}
	}

	State::StateStatus Detpack::Update(float fDt)
	{
		return State_Busy;
	}

	void Detpack::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(TF_MSG_DETPACK_BUILDING)
			{
				const Event_DetpackBuilding_TF *m = _message.Get<Event_DetpackBuilding_TF>();
				OBASSERT(m->m_Detpack.IsValid(), "Entity Expected");
				m_DetpackStatus.m_Entity = m->m_Detpack;
				m_DetpackStatus.m_Status = BUILDABLE_BUILDING;
				DBG_MSG(0, GetClient(), kNormal, "Detpack Building");
				break;
			}
			HANDLER(TF_MSG_DETPACK_BUILT)
			{
				const Event_DetpackBuilt_TF *m = _message.Get<Event_DetpackBuilt_TF>();
				OBASSERT(m->m_Detpack.IsValid(), "Entity Expected");
				m_DetpackStatus.m_Entity = m->m_Detpack;
				m_DetpackStatus.m_Status = BUILDABLE_BUILDING;
				DBG_MSG(0, GetClient(), kNormal, "Detpack Building");
				EngineFuncs::EntityPosition(m->m_Detpack, m_DetpackStatus.m_Position);
				m_DetpackStatus.m_Status = BUILDABLE_BUILT;
				DBG_MSG(0, GetClient(), kNormal, "Detpack Built");
				break;
			}
			HANDLER(TF_MSG_DETPACK_BUILDCANCEL)
			{
				memset(&m_DetpackStatus, 0, sizeof(m_DetpackStatus));
				m_DetpackStatus.m_Entity.Reset();
				break;
			}
			HANDLER(TF_MSG_DETPACK_DETONATED)
			{
				memset(&m_DetpackStatus, 0, sizeof(m_DetpackStatus));
				m_DetpackStatus.m_Entity.Reset();
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	PipeTrap::PipeTrap()
		: StateChild("PipeTrap")
		, FollowPathUser("PipeTrap")
		, m_Substate(IDLE)
	{
		LimitToClass().SetFlag(TF_CLASS_DEMOMAN);
		SetAlwaysRecieveEvents(true);

		m_Pipes.m_PipeCount = 0;

		m_NumTraps = 0;
		m_NumWaits = 0;

		m_CurrentTrap = 0;
		m_CurrentWait = 0;

		m_PlaceOrder = OrderRandomAll;
		m_WaitOrder = OrderRandomAll;
	}
	void PipeTrap::GetDebugString(StringStr &out)
	{
		if(IsActive())
		{
			if(m_MapGoal)
				out << m_MapGoal->GetName() << " ";
			out << "# Pipes: " << GetPipeCount();
		}
	}
	void PipeTrap::RenderDebug()
	{
		AABB b;
		for(int i = 0; i < Pipes::MaxPipes; ++i)
		{
			if(m_Pipes.m_Pipes[i].m_Entity.IsValid())
			{
				if(EngineFuncs::EntityWorldAABB(m_Pipes.m_Pipes[i].m_Entity, b))
				{
					Utils::OutlineAABB(b,
						m_Pipes.m_Pipes[i].m_Moving ? COLOR::GREEN : COLOR::RED,
						IGame::GetDeltaTimeSecs() * 2.f);
				}
			}
		}
	}

	// FollowPathUser functions.
	bool PipeTrap::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		if(!m_MapGoal)
			return false;

		if(m_MapGoal->RouteTo(GetClient(), _desination))
			_final = false;
		else
			_final = true;
		return true;
	}

	// AimerUser functions.
	bool PipeTrap::GetAimPosition(Vector3f &_aimpos)
	{
		switch(m_Substate)
		{
		case LAY_PIPES:
			_aimpos = GetClient()->GetEyePosition() + m_Traps[m_CurrentTrap].m_Facing * 256.f;
			break;
		case WATCH_PIPES:
			_aimpos = GetClient()->GetEyePosition() + m_Wait[m_CurrentWait].m_Facing * 256.f;
			break;
		case IDLE:
		case GETTING_AMMO:
			break;
		}
		return true;
	}

	void PipeTrap::OnTarget()
	{
		if(m_Substate == LAY_PIPES)
		{
			if(GetClient()->GetSteeringSystem()->InTargetRadius())
			{
				FINDSTATE(ws, WeaponSystem, GetRootState());
				if(ws && ws->CurrentWeaponIs(TF_Options::PIPE_WEAPON) && (IGame::GetFrameNumber()&2))
					ws->FireWeapon();
			}
		}
	}

	bool PipeTrap::CacheGoalInfo(MapGoalPtr mg)
	{
		m_CurrentTrap = 0;
		m_CurrentWait = 0;

		m_NumTraps = 0;
		m_NumWaits = 0;

		mg->GetProperty("PlaceOrder",m_PlaceOrder);
		mg->GetProperty("WaitOrder",m_WaitOrder);

		for(int i = 0; i < MaxPlacementPts; ++i)
		{
			m_Traps[m_NumTraps].Reset();
			if(mg->GetProperty(va("Place[%d].Position",i),m_Traps[m_NumTraps].m_Source) &&
				mg->GetProperty(va("Place[%d].Facing",i),m_Traps[m_NumTraps].m_Facing))
			{
				if(!m_Traps[m_NumTraps].m_Facing.IsZero())
				{
					++m_NumTraps;
				}
			}
		}

		for(int i = 0; i < MaxWaitPositions; ++i)
		{
			m_Wait[m_NumWaits].Reset();

			mg->GetProperty(va("Wait[%d].Position",i),m_Wait[m_NumWaits].m_Position);
			mg->GetProperty(va("Wait[%d].Facing",i),m_Wait[m_NumWaits].m_Facing);
			/*mg->GetProperty("MinWaitTime",m_MinWaitTime);
			mg->GetProperty("MaxWaitTime",m_MaxWaitTime);*/

			if(!m_Wait[m_NumWaits].m_Position.IsZero() && !m_Wait[m_NumWaits].m_Facing.IsZero())
			{
				++m_NumWaits;
			}
		}
		if(m_NumWaits == 0)
		{
			m_Wait[m_NumWaits].m_Position = mg->GetPosition();
			m_Wait[m_NumWaits].m_Facing = mg->GetFacing();
			++m_NumWaits;
		}

		//////////////////////////////////////////////////////////////////////////
		switch(m_PlaceOrder)
		{
			case OrderRandomAll:
#if __cplusplus >= 201703L //karin: random_shuffle removed since C++17
                compat::random_shuffle(m_Traps, m_Traps + m_NumTraps);
#else
                std::random_shuffle(m_Traps, m_Traps + m_NumTraps);
#endif
				break;
			case OrderRandomPick1:
#if __cplusplus >= 201703L //karin: random_shuffle removed since C++17
                compat::random_shuffle(m_Traps, m_Traps + m_NumTraps);
#else
                std::random_shuffle(m_Traps, m_Traps + m_NumTraps);
#endif
				m_NumTraps = ClampT<int>(m_NumTraps,0,1);
				break;
			case OrderSequentialAll:
				break;
		}
		//////////////////////////////////////////////////////////////////////////
		switch(m_WaitOrder)
		{
		case OrderRandomAll:
#if __cplusplus >= 201703L //karin: random_shuffle removed since C++17
			compat::random_shuffle(m_Wait, m_Wait + m_NumWaits);
#else
            std::random_shuffle(m_Wait, m_Wait + m_NumWaits);
#endif
			break;
		case OrderRandomPick1:
#if __cplusplus >= 201703L //karin: random_shuffle removed since C++17
            compat::random_shuffle(m_Wait, m_Wait + m_NumWaits);
#else
            std::random_shuffle(m_Wait, m_Wait + m_NumWaits);
#endif
			m_NumWaits = ClampT<int>(m_NumWaits,0,1);
			break;
		case OrderSequentialAll:
			break;
		}
		//////////////////////////////////////////////////////////////////////////
		return m_NumTraps > 0;
	}

	obReal PipeTrap::GetPriority()
	{
		m_Substate = LAY_PIPES;

		if(GetPipeCount() >= TF_Options::PIPE_MAX_DEPLOYED)
		{
			m_Substate = WATCH_PIPES;
			return 1.f;
		}

		if(!m_MapGoal)
		{
			GoalManager::Query qry(0x7e67445e /* pipetrap */, GetClient());
			GoalManager::GetInstance()->GetGoals(qry);

			if(!qry.GetBest(m_MapGoal) || !CacheGoalInfo(m_MapGoal))
				m_MapGoal.reset();
		}
		return m_MapGoal ? m_MapGoal->GetPriorityForClient(GetClient()) : 0.f;
	}
	void PipeTrap::Enter()
	{
		//////////////////////////////////////////////////////////////////////////
		// Cache pipe trap goal information
		//////////////////////////////////////////////////////////////////////////
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));

		m_ExpireTime = 0;
	}
	void PipeTrap::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_Substate = IDLE;

		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));

		FINDSTATEIF(ProximityWatcher, GetRootState(), RemoveWatch(GetNameHash()));
		m_WatchFilter.reset();
	}
	State::StateStatus PipeTrap::Update(float fDt)
	{
		OBASSERT(m_MapGoal, "No Map Goal!");

		// Update Pipe Positions
		m_Pipes.m_PipeCount = 0;

		//////////////////////////////////////////////////////////////////////////
		// Make sure pipes are valid and update their positions.
		bool bRebuildWatchPositions = false;
		for(int i = 0; i < Pipes::MaxPipes; ++i)
		{
			if(m_Pipes.m_Pipes[i].m_Entity.IsValid())
			{
				if(!IGame::IsEntityValid(m_Pipes.m_Pipes[i].m_Entity))
				{
					m_Pipes.m_Pipes[i].m_Entity.Reset();
					bRebuildWatchPositions = true;
					continue;
				}

				m_Pipes.m_PipeCount++;

				Vector3f vVelocity(Vector3f::ZERO);
				if(m_Pipes.m_Pipes[i].m_Moving)
				{
					if(!EngineFuncs::EntityVelocity(m_Pipes.m_Pipes[i].m_Entity, vVelocity) ||
						!EngineFuncs::EntityPosition(m_Pipes.m_Pipes[i].m_Entity, m_Pipes.m_Pipes[i].m_Position))
					{
						m_Pipes.m_Pipes[i].m_Entity.Reset();
						continue;
					}

					if(vVelocity.SquaredLength() <= Mathf::EPSILON)
					{
						m_Pipes.m_Pipes[i].m_Moving = false;

						if(m_WatchFilter)
						{
							m_WatchFilter->AddPosition(m_Pipes.m_Pipes[i].m_Position);
						}
						else
						{
							m_WatchFilter.reset(new FilterClosestTF(GetClient(), AiState::SensoryMemory::EntEnemy));
							m_WatchFilter->AddClass(FilterSensory::ANYPLAYERCLASS);
							m_WatchFilter->AddClass(TF_CLASSEX_SENTRY);
							m_WatchFilter->AddClass(TF_CLASSEX_DISPENSER);
							m_WatchFilter->AddPosition(m_Pipes.m_Pipes[i].m_Position);
							m_WatchFilter->SetMaxDistance(100.f);
							FINDSTATEIF(ProximityWatcher, GetRootState(), AddWatch(GetNameHash(),m_WatchFilter,false));
						}
					}
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Rebuild the positions so it's not watching for targets at old expired pipe positions
		if(bRebuildWatchPositions && m_WatchFilter)
		{
			m_WatchFilter->ClearPositions();
			for(int i = 0; i < Pipes::MaxPipes; ++i)
			{
				if(m_Pipes.m_Pipes[i].m_Entity.IsValid() && !m_Pipes.m_Pipes[i].m_Moving)
				{
					m_WatchFilter->AddPosition(m_Pipes.m_Pipes[i].m_Position);
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		const bool bGoForAmmo = ShouldGoForAmmo();
		if(m_Substate != GETTING_AMMO && bGoForAmmo)
		{
			SensoryMemory *sensory = GetClient()->GetSensoryMemory();

			MemoryRecords records;
			Vector3List recordpos;

			FilterAllType filter(GetClient(), AiState::SensoryMemory::EntAny, records);
			filter.MemorySpan(Utils::SecondsToMilliseconds(7.f));
			filter.AddClass(TF_CLASSEX_RESUPPLY);
			filter.AddClass(TF_CLASSEX_BACKPACK);
			filter.AddClass(TF_CLASSEX_BACKPACK_AMMO);
			sensory->QueryMemory(filter);

			sensory->GetRecordInfo(records, &recordpos, NULL);

			if(!recordpos.empty())
			{
				m_Substate = GETTING_AMMO;
				FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
				FINDSTATEIF(FollowPath, GetRootState(), Goto(this, recordpos));
			}
			else
			{
				//BlackboardDelay(10.f, m_MapGoalSentry->GetSerialNum());
				return State_Finished;
			}
		}
		else if(m_Substate == GETTING_AMMO && !bGoForAmmo)
			return State_Finished;

		//////////////////////////////////////////////////////////////////////////
		if(DidPathFail())
		{
			// Delay it from being used for a while.
			BlackboardDelay(10.f, m_MapGoal->GetSerialNum());
			return State_Finished;
		}
		//////////////////////////////////////////////////////////////////////////
		FINDSTATE(ws,WeaponSystem,GetRootState());
		FINDSTATE(ts,TargetingSystem,GetRootState());

		Priority::ePriority AimPriority = Priority::Low;
		Priority::ePriority WeaponPriority = ts&&ts->HasTarget() ? Priority::Min : Priority::Low;

		obint32 wpn = TF_Options::PIPE_WEAPON;
		switch(m_Substate)
		{
		case LAY_PIPES:
			{
				m_ExpireTime = 0; // reset expire

				wpn = TF_Options::PIPE_WEAPON;
				if(GetPipeCount() >= TF_Options::PIPE_MAX_DEPLOYED)
				{
					m_Substate = WATCH_PIPES;
					m_CurrentWait = rand() % m_NumWaits;
				}
				break;
			}
		case WATCH_PIPES:
			{
				if(GetPipeCount() < TF_Options::PIPE_MAX_DEPLOYED)
				{
					m_Substate = LAY_PIPES;
					break;
				}

				WeaponPriority = Priority::VeryLow;

				wpn = TF_Options::PIPE_WEAPON_WATCH;

				// reload pipe launcher before switching to GL
				/*WeaponPtr curWpn = ws->GetCurrentWeapon();
				if(curWpn->GetWeaponID()==TF_Options::PIPE_WEAPON)
				{
					if(curWpn->CanReload()!=InvalidFireMode)
					{
						wpn = TF_Options::PIPE_WEAPON;
						curWpn->ReloadWeapon();
					}
				}*/
				break;
			}
		case IDLE:
		case GETTING_AMMO:
			break;
		}

		if(DidPathSucceed())
		{
			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(AimPriority,this,GetNameHash()));
			ws->AddWeaponRequest(WeaponPriority, GetNameHash(), wpn);

			SteeringSystem *steer = GetClient()->GetSteeringSystem();
			switch(m_Substate)
			{
			case LAY_PIPES:
				{
					steer->SetTarget(m_Traps[m_CurrentTrap].m_Source);
					break;
				}
			case WATCH_PIPES:
				{
					steer->SetTarget(m_Wait[m_CurrentWait].m_Position);
					//////////////////////////////////////////////////////////////////////////
					// Cycle?
					if(m_NumWaits > 1 && m_WaitOrder!=OrderRandomPick1)
					{
						if(m_ExpireTime==0 && steer->InTargetRadius())
						{
							m_ExpireTime = IGame::GetTime()+
								Mathf::IntervalRandomInt(m_Wait[m_CurrentWait].m_MinWaitTime,m_Wait[m_CurrentWait].m_MaxWaitTime);
						}
						else if(IGame::GetTime() > m_ExpireTime)
						{
							m_ExpireTime = 0;

							switch(m_WaitOrder)
							{
							case OrderRandomAll:
								m_CurrentWait = rand() % m_NumWaits;
								break;
							case OrderSequentialAll:
								m_CurrentWait = (m_CurrentWait+1) % m_NumWaits;
								break;
							}
						}
					}
					break;
				}
			case IDLE:
				break;
			case GETTING_AMMO:
				return State_Finished;
			}

		}
		return State_Busy;
	}

	void PipeTrap::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(MESSAGE_PROXIMITY_TRIGGER)
			{
				const AiState::Event_ProximityTrigger *m = _message.Get<AiState::Event_ProximityTrigger>();
				if(m->m_OwnerState == GetNameHash())
				{
					if(IGame::GetFrameNumber()&1)
						GetClient()->PressButton(TF_BOT_BUTTON_DETPIPES);
					//GetClient()->SendEvent(MessageHelper(TF_MSG_DETPIPES))
				}
				break;
			}
			HANDLER(ACTION_WEAPON_FIRE)
			{
				const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
				if(m && m->m_Projectile.IsValid())
				{
					if(InterfaceFuncs::GetEntityClass(m->m_Projectile) == TF_CLASSEX_PIPE)
					{
						for(int i = 0; i < Pipes::MaxPipes; ++i)
						{
							if(!m_Pipes.m_Pipes[i].m_Entity.IsValid())
							{
								//Utils::OutputDebug(kInfo, "--Adding Pipe To Pipe List--");
								if(EngineFuncs::EntityPosition(m->m_Projectile, m_Pipes.m_Pipes[i].m_Position))
								{
									m_CurrentTrap = (m_CurrentTrap+1) % m_NumTraps;

									m_Pipes.m_Pipes[i].m_Entity = m->m_Projectile;
									m_Pipes.m_Pipes[i].m_Moving = true;
									m_Pipes.m_PipeCount++;
								}
								break;
							}
						}
					}
				}
				break;
			}
		}
	}

	bool PipeTrap::ShouldGoForAmmo()
	{
		int iPipes = 0, iMaxPipes = 0;
		g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), TF_Options::PIPE_AMMO, Primary, iPipes, iMaxPipes);
		return iPipes == 0;
	}
	//////////////////////////////////////////////////////////////////////////
	RocketJump::RocketJump()
		: StateChild("RocketJump")
		, m_IsDone(false)
	{
		LimitToClass().SetFlag(TF_CLASS_SOLDIER);
		//LimitToClass().SetFlag(TF_CLASS_PYRO);
	}

	bool RocketJump::GetAimPosition(Vector3f &_aimpos)
	{
		Vector3f vFlatFacing = Vector3f(m_NextPt.m_Pt - GetClient()->GetPosition()).Flatten();
		_aimpos = GetClient()->GetEyePosition() + Utils::ChangePitch(vFlatFacing,-89.f) * 32.f;
		return true;
	}

	void RocketJump::OnTarget()
	{
		bool bInRange = SquaredLength(GetClient()->GetPosition().As2d(), m_NextPt.m_Pt.As2d()) <= Mathf::Sqr(m_NextPt.m_Radius);
		if(bInRange)
		{
			FINDSTATE(ws, WeaponSystem, GetRootState());
			if(ws && ws->CurrentWeaponIs(TF_Options::ROCKETJUMP_WPN))
			{
				GetClient()->PressButton(BOT_BUTTON_JUMP);
				ws->FireWeapon();
				m_IsDone = true;
			}
		}
	}

	void RocketJump::Enter()
	{
		m_IsDone = false;
		FINDSTATEIF(Aimer,GetParent(),AddAimRequest(Priority::Override,this,GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::Override, GetNameHash(), TF_Options::ROCKETJUMP_WPN));
	}

	void RocketJump::Exit()
	{
		FINDSTATEIF(Aimer,GetParent(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));
	}

	void RocketJump::RenderDebug()
	{
		if(IsActive())
		{
			Utils::DrawLine(GetClient()->GetEyePosition(),m_NextPt.m_Pt,COLOR::GREEN,IGame::GetDeltaTimeSecs()*2.f);
		}
	}

	obReal RocketJump::GetPriority()
	{
		FINDSTATE(fp,FollowPath,GetParent());
		if(fp)
		{
			if(fp->IsMoving() &&
				fp->GetCurrentPath().GetCurrentPt(m_NextPt) &&
				(m_NextPt.m_NavFlags & F_TF_NAV_ROCKETJUMP))
			{
				return 1.f;
			}
		}
		return 0.f;
	}

	State::StateStatus RocketJump::Update(float fDt)
	{
		return m_IsDone ? State_Finished : State_Busy;
	}
	void RocketJump::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(ACTION_WEAPON_FIRE)
			{
				const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
				if(m && m->m_Projectile.IsValid())
				{
					if(InterfaceFuncs::GetEntityClass(m->m_Projectile) == TF_CLASSEX_ROCKET)
					{
						m_IsDone = true;
					}
				}
				break;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	ConcussionJump::ConcussionJump() : StateChild("ConcussionJump")
	{
		LimitToClass().SetFlag(TF_CLASS_MEDIC);
		LimitToClass().SetFlag(TF_CLASS_SCOUT);
	}
	void ConcussionJump::RenderDebug()
	{
		if(IsActive())
		{
			Utils::DrawLine(GetClient()->GetEyePosition(),m_NextPt.m_Pt,COLOR::GREEN,IGame::GetDeltaTimeSecs()*2.f);
		}
	}
	bool ConcussionJump::GetAimPosition(Vector3f &_aimpos)
	{
		_aimpos = m_NextPt.m_Pt;
		return true;
	}
	void ConcussionJump::OnTarget()
	{

	}
	obReal ConcussionJump::GetPriority()
	{
		FINDSTATE(fp,FollowPath,GetParent());
		if(fp)
		{
			if(fp->IsMoving())
			{
				const Path &p = fp->GetCurrentPath();
				if(!p.IsEndOfPath())
				{
					Path::PathPoint pt;

					int ix = p.GetCurrentPtIndex();
					for(int i = ix; i < p.GetNumPts(); ++i)
					{
						p.GetCurrentPt(pt);
						if(pt.m_NavFlags & F_TF_NAV_CONCJUMP)
						{
							m_NextPt = pt;
							return 1.f;
						}
					}
				}
			}
		}
		return 0.f;
	}
	void ConcussionJump::Enter()
	{
		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Override,this,GetNameHash()));
	}	void ConcussionJump::Exit()
	{
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	}
	State::StateStatus ConcussionJump::Update(float fDt)
	{
		return State_Busy;
	}
	void ConcussionJump::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		/*switch(_message.GetMessageId())
		{
			HANDLER(ACTION_WEAPON_FIRE)
			{
				const Event_WeaponFire *m = _message.Get<Event_WeaponFire>();
				if(m && m->m_Projectile.IsValid())
				{
					if(InterfaceFuncs::GetEntityClass(m->m_Projectile) == TF_CLASSEX_CONC_GRENADE)
					{
						m_IsDone = true;
					}
				}
				break;
			}
		}*/
	}
	//////////////////////////////////////////////////////////////////////////
	ThrowGrenade::ThrowGrenade()
		: StateChild("ThrowGrenade")
		, m_PrimaryGrenade(0)
		, m_SecondaryGrenade(0)
		, m_Gren1Ammo(0)
		, m_Gren1AmmoMax(0)
		, m_Gren2Ammo(0)
		, m_Gren2AmmoMax(0)
		, m_GrenType(0)
		, m_LastThrowTime(0)
	{
	}
	void ThrowGrenade::GetDebugString(StringStr &out)
	{
		if(IsActive())
		{
			out << (m_GrenType==TF_BOT_BUTTON_GREN1?
				"Throwing Primary Grenade":
			"Throwing Secondary Grenade");
		}
	}
	void ThrowGrenade::RenderDebug()
	{
		if(IsActive())
			Utils::DrawLine(GetClient()->GetEyePosition(), m_AimPos, m_OnTarget?COLOR::GREEN:COLOR::RED, 0.1f);
	}
	bool ThrowGrenade::GetAimPosition(Vector3f &_aimpos)
	{
		_aimpos = m_AimPos;
		return true;
	}
	void ThrowGrenade::OnTarget()
	{
		m_OnTarget = true;
	}
	void ThrowGrenade::OnSpawn()
	{
		_UpdateGrenadeTypes();
	}
	obReal ThrowGrenade::GetPriority()
	{
		const float FF_MIN_GREN_DIST = 200.f;
		const float FF_MAX_GREN_DIST = 800.f;

		if(!IsActive())
		{
			const MemoryRecord *pRecord = GetClient()->GetTargetingSystem()->GetCurrentTargetRecord();

			if(pRecord && pRecord->IsShootable())
			{
				/*const GameEntity &vTargetEntity = m_Client->GetTargetingSystem()->GetCurrentTarget();
				const TargetInfo *pTargetInfo = m_Client->GetSensoryMemory()->GetTargetInfo(vTargetEntity);*/
				const TargetInfo &targetInfo = pRecord->m_TargetInfo;

				// Range check.
				if(!InRangeT<float>(targetInfo.m_DistanceTo, FF_MIN_GREN_DIST, FF_MAX_GREN_DIST))
					return 0.f;

				// Calculate the desirability to use this item.
				obReal fDesirability = 0.0;
				obReal fGren1Desir = TF_Game::_GetDesirabilityFromTargetClass(m_PrimaryGrenade, targetInfo.m_EntityClass);
				obReal fGren2Desir = TF_Game::_GetDesirabilityFromTargetClass(m_SecondaryGrenade, targetInfo.m_EntityClass);

				if(fGren1Desir >= fGren2Desir)
				{
					fDesirability = fGren1Desir;
					m_GrenType = TF_BOT_BUTTON_GREN1;
				}
				else
				{
					fDesirability = fGren2Desir;
					m_GrenType = TF_BOT_BUTTON_GREN2;
				}

				// Prefer more when reloading.
				if(GetClient()->GetWeaponSystem()->IsReloading())
					fDesirability += 0.1f;

				// Take the last thrown time into account.
				int iTimeSinceLast = IGame::GetTime() - m_LastThrowTime;
				fDesirability *= ClampT<obReal>((obReal)iTimeSinceLast / 7500.f, 0.f, 2.f);

				// How about some randomness?
				//dDesirability *= Mathd::UnitRandom() * 1.1;

				// With bias
				m_GrenType = TF_BOT_BUTTON_GREN1;

				const obReal fUseThreshhold = 0.9f;
				if(fDesirability >= fUseThreshhold)
				{
					//Dbg::dout << Dbg::info << "Using " << GetItemName() << std::endl;
					m_AimMode = GRENADE_AIM;
					return 1.f;
				}
			}
			return 0.f;
		}
		return 1.f ;
	}
	void ThrowGrenade::Enter()
	{
		m_OnTarget = false;
		m_LastThrowTime = IGame::GetTime();
		//FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::High,this,GetNameHash()));
	}
	void ThrowGrenade::Exit()
	{
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
	}
	State::StateStatus ThrowGrenade::Update(float fDt)
	{
		const Vector3f vEyePos = GetClient()->GetEyePosition();
		const float fProjectileSpeed = _GetProjectileSpeed(m_GrenType);
		//const float fProjectileGravity = _GetProjectileGravity(m_GrenType);
		const float fTimeToDet = 4.0f - GetStateTime();

		// We need to aim the grenade.
		const MemoryRecord *pRecord = GetClient()->GetTargetingSystem()->GetCurrentTargetRecord();
		if(pRecord)
		{
			const TargetInfo &targetInfo = pRecord->m_TargetInfo;
			m_AimPos = Utils::PredictFuturePositionOfTarget(GetClient()->GetPosition(),
				fProjectileSpeed, targetInfo, Vector3f::ZERO);

			// Add a vertical aim offset
			m_AimPos.z += targetInfo.m_DistanceTo * 0.25f;

			bool bThrow = fTimeToDet < 1.0;

			// Calculate the timer for the grenade based on the distance to the aim point.
			float fExplodeDistToTarget = (m_AimPos - GetClient()->GetPosition()).Length();

			if(!bThrow)
			{
				// How far will the grenade go with the time left?
				const float fFakeGrenadeRange = (fTimeToDet-0.15f) * fProjectileSpeed;
				const float fGrenadeRange = fTimeToDet * fProjectileSpeed;

				if(fExplodeDistToTarget >= fFakeGrenadeRange)
				{
					m_AimMode = GRENADE_AIM;
					FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::High,this,GetNameHash()));
				}
				if(m_OnTarget && fExplodeDistToTarget >= fGrenadeRange)
				{
					bThrow = true;
				}
			}

			if(bThrow)
			{
				return State_Finished;
			}
		}
		else
		{
			if(m_OnTarget && m_AimMode == GRENADE_RID)
				return State_Finished;

			if(fTimeToDet <= 1.f)
			{
				m_AimMode = GRENADE_RID;
				FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::High,this,GetNameHash()));
				m_AimPos = vEyePos - GetClient()->GetVelocity() * 256.f; // face back
			}
		}
		m_OnTarget = false;
		GetClient()->PressButton(m_GrenType);
		return State_Busy;
	}
	void ThrowGrenade::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
	}
	void ThrowGrenade::_UpdateAmmo()
	{
		g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(),TF_WP_GRENADE1,Primary,m_Gren1Ammo,m_Gren1AmmoMax);
		g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(),TF_WP_GRENADE2,Primary,m_Gren2Ammo,m_Gren2AmmoMax);
	}
	void ThrowGrenade::_UpdateGrenadeTypes()
	{
		switch(GetClient()->GetClass())
		{
		case TF_CLASS_SCOUT:
			m_PrimaryGrenade = TF_WP_GRENADE_CALTROPS;
			m_SecondaryGrenade = TF_WP_GRENADE_CONC;
			break;
		case TF_CLASS_SNIPER:
			m_PrimaryGrenade = TF_WP_GRENADE;
			m_SecondaryGrenade = TF_WP_NONE;
			break;
		case TF_CLASS_SOLDIER:
			m_PrimaryGrenade = TF_WP_GRENADE;
			m_SecondaryGrenade = TF_WP_GRENADE_NAIL;
			break;
		case TF_CLASS_DEMOMAN:
			m_PrimaryGrenade = TF_WP_GRENADE;
			m_SecondaryGrenade = TF_WP_GRENADE_MIRV;
			break;
		case TF_CLASS_MEDIC:
			m_PrimaryGrenade = TF_WP_GRENADE;
			m_SecondaryGrenade = TF_WP_GRENADE_CONC;
			break;
		case TF_CLASS_HWGUY:
			m_PrimaryGrenade = TF_WP_GRENADE;
			m_SecondaryGrenade = TF_WP_GRENADE_MIRV;
			break;
		case TF_CLASS_PYRO:
			m_PrimaryGrenade = TF_WP_GRENADE;
			m_SecondaryGrenade = TF_WP_GRENADE_NAPALM;
			break;
		case TF_CLASS_SPY:
			m_PrimaryGrenade = TF_WP_GRENADE;
			m_SecondaryGrenade = TF_WP_GRENADE_GAS;
			break;
		case TF_CLASS_ENGINEER:
			m_PrimaryGrenade = TF_WP_GRENADE;
			m_SecondaryGrenade = TF_WP_GRENADE_EMP;
			break;
		case TF_CLASS_CIVILIAN:
		default:
			m_PrimaryGrenade = 0;
			m_SecondaryGrenade = 0;
			break;
		}
	}
	float ThrowGrenade::_GetProjectileSpeed(int _type) const
	{
		return TF_Options::GRENADE_VELOCITY;
		//return 500.f;
	}
	float ThrowGrenade::_GetProjectileGravity(int _type) const
	{
		return 1.f;
	}
	//////////////////////////////////////////////////////////////////////////
	Teleporter::Teleporter() : StatePrioritized("Teleporter")
	{
		AppendState(new TeleporterBuild);

		LimitToClass().SetFlag(TF_CLASS_ENGINEER);
		SetAlwaysRecieveEvents(true);
	}
	void Teleporter::UpdateTeleporterStatus(const Event_TeleporterStatus_TF &_stats)
	{
		if(m_TeleporterStatus.m_EntityEntrance.IsValid())
		{
			m_TeleporterStatus.m_StatusEntrance = BUILDABLE_BUILT;
		}
		else
		{
			m_TeleporterStatus.m_StatusEntrance = BUILDABLE_INVALID;
		}

		if(m_TeleporterStatus.m_EntityExit.IsValid())
		{
			m_TeleporterStatus.m_StatusExit = BUILDABLE_BUILT;
		}
		else
		{
			m_TeleporterStatus.m_StatusExit = BUILDABLE_INVALID;
		}

		m_TeleporterStatus.m_EntityEntrance = _stats.m_EntityEntrance;
		m_TeleporterStatus.m_EntityExit = _stats.m_EntityExit;
	}
	State::StateStatus Teleporter::Update(float fDt)
	{
		return State_Busy;
	}
	void Teleporter::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(TF_MSG_TELE_ENTRANCE_BUILDING)
			{
				const Event_TeleporterBuilding_TF *m = _message.Get<Event_TeleporterBuilding_TF>();
				OBASSERT(m->m_Teleporter.IsValid(), "Entity Expected");
				m_TeleporterStatus.m_StatusEntrance = BUILDABLE_BUILDING;
				m_TeleporterStatus.m_EntityEntrance = m->m_Teleporter;
				DBG_MSG(0, GetClient(), kNormal, "Tele Entrance Building");
				break;
			}
			HANDLER(TF_MSG_TELE_EXIT_BUILDING)
			{
				const Event_TeleporterBuilding_TF *m = _message.Get<Event_TeleporterBuilding_TF>();
				OBASSERT(m->m_Teleporter.IsValid(), "Entity Expected");
				m_TeleporterStatus.m_StatusExit = BUILDABLE_BUILDING;
				m_TeleporterStatus.m_EntityExit = m->m_Teleporter;
				DBG_MSG(0, GetClient(), kNormal, "Tele Exit Building");
				break;
			}
			HANDLER(TF_MSG_TELE_ENTRANCE_BUILT)
			{
				const Event_TeleporterBuilt_TF *m = _message.Get<Event_TeleporterBuilt_TF>();
				OBASSERT(m->m_Teleporter.IsValid(), "Entity Expected");
				m_TeleporterStatus.m_EntityEntrance = m->m_Teleporter;
				EngineFuncs::EntityPosition(m->m_Teleporter, m_TeleporterStatus.m_EntrancePos);
				m_TeleporterStatus.m_StatusEntrance = BUILDABLE_BUILT;
				DBG_MSG(0, GetClient(), kNormal, "Tele Entrance Built");
				break;
			}
			HANDLER(TF_MSG_TELE_EXIT_BUILT)
			{
				const Event_TeleporterBuilt_TF *m = _message.Get<Event_TeleporterBuilt_TF>();
				OBASSERT(m->m_Teleporter.IsValid(), "Entity Expected");
				m_TeleporterStatus.m_EntityExit = m->m_Teleporter;
				EngineFuncs::EntityPosition(m->m_Teleporter, m_TeleporterStatus.m_ExitPos);
				m_TeleporterStatus.m_StatusExit = BUILDABLE_BUILT;
				DBG_MSG(0, GetClient(), kNormal, "Tele Exit Built");
				break;
			}
			HANDLER(TF_MSG_TELE_ENTRANCE_DESTROYED)
			{
				m_TeleporterStatus.m_EntityEntrance.Reset();
				break;
			}
			HANDLER(TF_MSG_TELE_EXIT_DESTROYED)
			{
				m_TeleporterStatus.m_EntityExit.Reset();
				break;
			}
			HANDLER(TF_MSG_TELE_STATS)
			{
				const Event_TeleporterStatus_TF	*m = _message.Get<Event_TeleporterStatus_TF>();
				m;
				/*DBG_MSG(0, GetClient(), kNormal, "Dispenser Stats");
				DBG_MSG(0, GetClient(), kNormal, va("Health: %d", m_DispenserStatus.m_Health));
				DBG_MSG(0, GetClient(), kNormal, va("Shells: %d", m_DispenserStatus.m_Shells));
				DBG_MSG(0, GetClient(), kNormal, va("Nails: %d", m_DispenserStatus.m_Nails));
				DBG_MSG(0, GetClient(), kNormal, va("Rockets: %d", m_DispenserStatus.m_Rockets));
				DBG_MSG(0, GetClient(), kNormal, va("Cells: %d", m_DispenserStatus.m_Cells));
				DBG_MSG(0, GetClient(), kNormal, va("Armor: %d", m_DispenserStatus.m_Armor));*/
				break;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	int TeleporterBuild::BuildEquipWeapon = TF_WP_NONE;

	TeleporterBuild::TeleporterBuild()
		: StateChild("TeleporterBuild")
		, FollowPathUser("TeleporterBuild")
		, m_CantBuild(false)
	{
	}
	void TeleporterBuild::GetDebugString(StringStr &out)
	{
		if(IsActive())
		{
			switch(m_State)
			{
			case NONE:
				break;
			case GETTING_AMMO:
				out << "Getting Ammo";
				break;
			case TELE_BUILDING_ENTRANCE:
				out << (m_CantBuild ? "Cant Build " : "Building ");
				if(m_MapGoalTeleEntrance)
					out << m_MapGoalTeleEntrance->GetName();
				break;
			case TELE_BUILDING_EXIT:
				out << (m_CantBuild ? "Cant Build " : "Building ");
				if(m_MapGoalTeleExit)
					out << m_MapGoalTeleExit->GetName();
				break;
			}
		}
	}
	bool TeleporterBuild::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		switch(m_State)
		{
		case TELE_BUILDING_ENTRANCE:
			if(m_MapGoalTeleEntrance->RouteTo(GetClient(), _desination))
				_final = false;
			else
				_final = true;
			break;
		case TELE_BUILDING_EXIT:
			if(m_MapGoalTeleExit->RouteTo(GetClient(), _desination))
				_final = false;
			else
				_final = true;
			break;

		case GETTING_AMMO:
		default:
			return false;
		}
		return true;
	}
	bool TeleporterBuild::GetAimPosition(Vector3f &_aimpos)
	{
		switch(m_State)
		{
		case TELE_BUILDING_ENTRANCE:
			_aimpos = GetClient()->GetEyePosition() + m_MapGoalTeleEntrance->GetFacing() * 1024.f;
			break;
		case TELE_BUILDING_EXIT:
			_aimpos = GetClient()->GetEyePosition() + m_MapGoalTeleExit->GetFacing() * 1024.f;
			break;
		default:
			OBASSERT(0,"Invalid State");
		}
		return true;
	}
	void TeleporterBuild::OnTarget()
	{
		FINDSTATE(tele,Teleporter,GetParent());
		if(!tele)
			return;

		switch(m_State)
		{
		case TELE_BUILDING_ENTRANCE:
			{
				if(!tele->HasTeleporterEntrance() && IGame::GetTime() >= m_NextBuildTry)
				{
					GetClient()->PressButton(TF_BOT_BUTTON_BUILD_TELE_ENTRANCE);
					m_NextBuildTry = IGame::GetTime() + TF_Options::BUILD_ATTEMPT_DELAY;
				}
				break;
			}
		case TELE_BUILDING_EXIT:
			{
				if(!tele->HasTeleporterExit() && IGame::GetTime() >= m_NextBuildTry)
				{
					GetClient()->PressButton(TF_BOT_BUTTON_BUILD_TELE_EXIT);
					m_NextBuildTry = IGame::GetTime() + TF_Options::BUILD_ATTEMPT_DELAY;
				}
				break;
			}
		default:
			OBASSERT(0,"Invalid State");
		}
	}
	obReal TeleporterBuild::GetPriority()
	{
		if(m_MapGoalTeleEntrance)
		{
			if(!m_MapGoalTeleEntrance->IsAvailable(GetClient()->GetTeam()))
			{
				GetClient()->PressButton(TF_BOT_BUTTON_DETTELE_ENTRANCE);
				m_MapGoalTeleEntrance.reset();
			}
		}
		if(m_MapGoalTeleExit)
		{
			if(!m_MapGoalTeleExit->IsAvailable(GetClient()->GetTeam()))
			{
				GetClient()->PressButton(TF_BOT_BUTTON_DETTELE_EXIT);
				m_MapGoalTeleExit.reset();
			}
		}

		if(IsActive())
			return GetLastPriority();

		m_MapGoalTeleEntrance.reset();
		m_MapGoalTeleExit.reset();

		FINDSTATE(tp,Teleporter,GetParent());

		if(!tp->HasTeleporterEntrance())
		{
			GoalManager::Query qry(0xed2f457b /* TeleEntrance */, GetClient());
			GoalManager::GetInstance()->GetGoals(qry);
			qry.GetBest(m_MapGoalTeleEntrance);
			m_State = TELE_BUILDING_ENTRANCE;

			if(m_MapGoalTeleEntrance)
				return m_MapGoalTeleEntrance->GetPriorityForClient(GetClient());
		}
		if(!tp->HasTeleporterExit())
		{
			GoalManager::Query qry(0x31bccf2d /* TeleExit */, GetClient());
			if(m_BuiltTeleEntrance)
			{
				const String &groupName = m_BuiltTeleEntrance->GetGroupName();
				if(!groupName.empty())
					qry.Group(groupName.c_str());
			}

			GoalManager::GetInstance()->GetGoals(qry);
			qry.GetBest(m_MapGoalTeleExit);
			m_State = TELE_BUILDING_EXIT;

			if(m_MapGoalTeleExit)
				return m_MapGoalTeleExit->GetPriorityForClient(GetClient());
		}
		return 0.f;
	}
	void TeleporterBuild::Enter()
	{
		m_NextAmmoCheck = 0;
		m_NextBuildTry = 0;
		m_CantBuild = false;
		FINDSTATEIF(FollowPath, GetRootState(), Goto(this));
	}
	void TeleporterBuild::Exit()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop(true));

		m_State = NONE;

		m_MapGoalTeleEntrance.reset();
		m_MapGoalTeleExit.reset();

		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		GetClient()->PressButton(TF_BOT_BUTTON_CANCELBUILD);
		FINDSTATEIF(WeaponSystem,GetRootState(),ReleaseWeaponRequest(GetNameHash()));
	}
	State::StateStatus TeleporterBuild::Update(float fDt)
	{
		FINDSTATE(tp,Teleporter,GetParent());

		switch(m_State)
		{
		case TELE_BUILDING_ENTRANCE:
			{
				if(!m_MapGoalTeleEntrance)
					return State_Finished;

				if(tp->HasTeleporterEntrance())
					return State_Finished;
				break;
			}
		case TELE_BUILDING_EXIT:
			{
				if(!m_MapGoalTeleExit)
					return State_Finished;

				if(tp->HasTeleporterExit())
					return State_Finished;
				break;
			}
		case GETTING_AMMO:
		default:
			break;
		}

		//////////////////////////////////////////////////////////////////////////
		if(DidPathFail())
		{
			// Delay it from being used for a while.
			if(m_State == TELE_BUILDING_ENTRANCE && m_MapGoalTeleEntrance)
				BlackboardDelay(10.f, m_MapGoalTeleEntrance->GetSerialNum());
			else if(m_State == TELE_BUILDING_EXIT && m_MapGoalTeleExit)
				BlackboardDelay(10.f, m_MapGoalTeleExit->GetSerialNum());

			return State_Finished;
		}
		//////////////////////////////////////////////////////////////////////////
		if(GetClient()->HasEntityFlag(TF_ENT_FLAG_BUILDING_ENTRANCE) || GetClient()->HasEntityFlag(TF_ENT_FLAG_BUILDING_EXIT))
			return State_Busy;

		//////////////////////////////////////////////////////////////////////////
		// Need ammo?
		if(IGame::GetTime() >= m_NextAmmoCheck)
		{
			m_NextAmmoCheck = IGame::GetTime() + 1000;

			int iAmmo = 0, iMaxAmmo = 0;
			g_EngineFuncs->GetCurrentAmmo(GetClient()->GetGameEntity(), TF_Options::BUILD_AMMO_TYPE, Primary, iAmmo, iMaxAmmo);
			if(m_State == GETTING_AMMO && iAmmo >= TF_Options::TELEPORT_BUILD_AMMO)
				return State_Finished;
			if(m_State != GETTING_AMMO && iAmmo < TF_Options::TELEPORT_BUILD_AMMO)
			{
				SensoryMemory *sensory = GetClient()->GetSensoryMemory();

				MemoryRecords records;
				Vector3List recordpos;

				FilterAllType filter(GetClient(), AiState::SensoryMemory::EntAny, records);
				filter.MemorySpan(Utils::SecondsToMilliseconds(7.f));
				filter.AddClass(TF_CLASSEX_RESUPPLY);
				filter.AddClass(TF_CLASSEX_BACKPACK);
				filter.AddClass(TF_CLASSEX_BACKPACK_AMMO);
				sensory->QueryMemory(filter);

				sensory->GetRecordInfo(records, &recordpos, NULL);

				if(!recordpos.empty())
				{
					m_State = GETTING_AMMO;
					FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
					FINDSTATEIF(FollowPath, GetRootState(), Goto(this, recordpos));
				}
				else
				{
					//BlackboardDelay(10.f, m_MapGoalSentry->GetSerialNum());
					return State_Finished;
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////
		if(DidPathSucceed())
		{
			if(m_State == GETTING_AMMO)
				return State_Finished;

			FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::Medium,this,GetNameHash()));
			if(BuildEquipWeapon)
				FINDSTATEIF(WeaponSystem,GetRootState(),AddWeaponRequest(Priority::Medium,GetNameHash(),BuildEquipWeapon));

			if(m_CantBuild)
			{
				if(m_State == TELE_BUILDING_ENTRANCE && m_MapGoalTeleEntrance)
					GetClient()->SetMovementVector(-m_MapGoalTeleEntrance->GetFacing());
				else if(m_State == TELE_BUILDING_EXIT && m_MapGoalTeleExit)
					GetClient()->SetMovementVector(-m_MapGoalTeleExit->GetFacing());

				GetClient()->PressButton(BOT_BUTTON_WALK);
				m_NextBuildTry = 0;
			}
			else
				GetClient()->SetMovementVector(Vector3f::ZERO);
		}
		return State_Busy;
	}
	void TeleporterBuild::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(TF_MSG_TELE_ENTRANCE_BUILDING)
			{
				m_BuiltTeleEntrance = m_MapGoalTeleEntrance;
				break;
			}
			HANDLER(TF_MSG_TELE_EXIT_BUILDING)
			{
				m_BuiltTeleExit = m_MapGoalTeleExit;
				break;
			}
			HANDLER(TF_MSG_TELE_ENTRANCE_CANCEL)
			{
				m_BuiltTeleEntrance.reset();
				break;
			}
			HANDLER(TF_MSG_TELE_EXIT_CANCEL)
			{
				m_BuiltTeleExit.reset();
				break;
			}
			HANDLER(TF_MSG_TELE_ENTRANCE_DESTROYED)
			{
				m_BuiltTeleEntrance.reset();
				break;
			}
			HANDLER(TF_MSG_TELE_EXIT_DESTROYED)
			{
				m_BuiltTeleExit.reset();
				break;
			}
			HANDLER(TF_MSG_TELE_ENTRANCE_CANTBUILD)
			{
				m_CantBuild = true;
				break;
			}
			HANDLER(TF_MSG_TELE_EXIT_CANTBUILD)
			{
				m_CantBuild = true;
				break;
			}
		}
	}
};
