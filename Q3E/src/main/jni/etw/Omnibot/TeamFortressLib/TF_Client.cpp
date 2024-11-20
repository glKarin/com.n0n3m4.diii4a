////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF.h"
#include "TF_Client.h"
#include "TF_NavigationFlags.h"
#include "TF_BaseStates.h"

#include "IGame.h"
#include "IGameManager.h"
#include "ScriptManager.h"

TF_Client::TF_Client()
{
	m_StepHeight = 18.0f;
}

TF_Client::~TF_Client()
{
}

void TF_Client::Init(int _gameid)
{
	Client::Init(_gameid);

	// We want to use a custom targeting filter.
	FilterPtr filter(new FilterClosestTF(this, AiState::SensoryMemory::EntEnemy));
	filter->AddCategory(ENT_CAT_SHOOTABLE);
	GetTargetingSystem()->SetDefaultTargetingFilter(filter);

	/*using namespace AiState;
	FINDSTATE(ws,WeaponSystem,GetStateRoot());
	if(ws)
	{
		ws->AddWeaponToInventory(TF_WP_GRENADE1);
		ws->AddWeaponToInventory(TF_WP_GRENADE2);
	}*/
}

void TF_Client::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	switch(_message.GetMessageId())
	{
		HANDLER(TF_MSG_GOT_ENGY_ARMOR)
		{
			_cb.CallScript();
			const Event_GotEngyArmor *m = _message.Get<Event_GotEngyArmor>();
			_cb.AddEntity("whodoneit", m->m_FromWho);
			_cb.AddInt("before", m->m_BeforeValue);
			_cb.AddInt("after", m->m_AfterValue);
			break;
		}
		HANDLER(TF_MSG_GAVE_ENGY_ARMOR)
		{
			_cb.CallScript();
			const Event_GaveEngyArmor *m = _message.Get<Event_GaveEngyArmor>();
			_cb.AddEntity("whoiarmored", m->m_ToWho);
			_cb.AddInt("before", m->m_BeforeValue);
			_cb.AddInt("after", m->m_AfterValue);
			break;
		}
		HANDLER(TF_MSG_GOT_MEDIC_HEALTH)
		{
			_cb.CallScript();
			const Event_GotMedicHealth *m = _message.Get<Event_GotMedicHealth>();
			_cb.AddEntity("whodoneit", m->m_FromWho);
			_cb.AddInt("before", m->m_BeforeValue);
			_cb.AddInt("after", m->m_AfterValue);
			break;
		}
		HANDLER(TF_MSG_GAVE_MEDIC_HEALTH)
		{
			_cb.CallScript();
			const Event_GaveMedicHealth *m = _message.Get<Event_GaveMedicHealth>();
			_cb.AddEntity("whoihealed", m->m_ToWho);
			_cb.AddInt("before", m->m_BeforeValue);
			_cb.AddInt("after", m->m_AfterValue);
			break;
		}
		HANDLER(TF_MSG_INFECTED)
		{
			_cb.CallScript();
			const Event_Infected *m = _message.Get<Event_Infected>();
			_cb.AddEntity("infected by", m->m_FromWho);
			break;
		}
		HANDLER(TF_MSG_CURED)
		{
			_cb.CallScript();
			const Event_Cured *m = _message.Get<Event_Cured>();
			_cb.AddEntity("cured by", m->m_ByWho);
			break;
		}
		HANDLER(TF_MSG_BURNLEVEL)
		{
			_cb.CallScript();
			const Event_Burn *m = _message.Get<Event_Burn>();
			_cb.AddEntity("burned by", m->m_ByWho);
			_cb.AddInt("burnlevel", m->m_BurnLevel);
			break;
		}
		HANDLER(TF_MSG_GOT_DISPENSER_AMMO)
		{
			_cb.CallScript();
			break;
		}
		HANDLER(TF_MSG_RADIOTAG_UPDATE)
		{
			_cb.CallScript();
			const Event_RadioTagUpdate_TF *m = _message.Get<Event_RadioTagUpdate_TF>();
			if(m->m_Detected.IsValid())
			{
				GetSensoryMemory()->UpdateWithTouchSource(m->m_Detected);
				_cb.AddEntity("detected", m->m_Detected);
			}
			else
			{
				DBG_MSG(0, this, kError, "TF_MSG_RADIOTAG_UPDATE got bad entity");
			}
			break;
		}
		HANDLER(TF_MSG_RADAR_DETECT_ENEMY)
		{
			_cb.CallScript();
			const Event_RadarUpdate_TF *m = _message.Get<Event_RadarUpdate_TF>();
			if(m->m_Detected.IsValid())
			{
				GetSensoryMemory()->UpdateWithTouchSource(m->m_Detected);
				_cb.AddEntity("detected", m->m_Detected);
			}
			else
			{
				DBG_MSG(0, this, kError, "TF_MSG_RADAR_DETECT_ENEMY got bad entity");
			}
			break;
		}
		HANDLER(TF_MSG_CANTDISGUISE_AS_TEAM)
		{
			_cb.CallScript();
			const Event_CantDisguiseTeam_TF *m = _message.Get<Event_CantDisguiseTeam_TF>();
			_cb.AddInt("team", m->m_TeamId);
			break;
		}
		HANDLER(TF_MSG_CANTDISGUISE_AS_CLASS)
		{
			_cb.CallScript();
			const Event_CantDisguiseClass_TF *m = _message.Get<Event_CantDisguiseClass_TF>();
			_cb.AddInt("class", m->m_ClassId);
			break;
		}
		HANDLER(TF_MSG_DISGUISING)
			HANDLER(TF_MSG_DISGUISED)
		{
			_cb.CallScript();
			const Event_Disguise_TF *m = _message.Get<Event_Disguise_TF>();
			_cb.AddInt("asTeam", m->m_TeamId);
			_cb.AddInt("asClass", m->m_ClassId);
			break;
		}
		//////////////////////////////////////////////////////////////////////////
		HANDLER(TF_MSG_SENTRY_NOTENOUGHAMMO)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Sentry NotEnough Ammo");
			break;
		}
		HANDLER(TF_MSG_SENTRY_ALREADYBUILT)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Sentry Already Built");
			break;
		}
		HANDLER(TF_MSG_SENTRY_CANTBUILD)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Sentry Can't Build");
			break;
		}
		HANDLER(TF_MSG_SENTRY_BUILDING)
		{
			_cb.CallScript();
			const Event_SentryBuilding_TF *m = _message.Get<Event_SentryBuilding_TF>();
			OBASSERT(m->m_Sentry.IsValid(), "Entity Expected");
			//m_SentryStatus.m_Entity = m->m_Sentry;
			//m_SentryStatus.m_Status = BUILDABLE_BUILDING;
			//DBG_MSG(0, m_Client, kNormal, "Sentry Building");
			_cb.AddEntity("sg", m->m_Sentry);
			break;
		}
		HANDLER(TF_MSG_SENTRY_BUILT)
		{
			_cb.CallScript();
			const Event_SentryBuilt_TF *m = _message.Get<Event_SentryBuilt_TF>();
			OBASSERT(m->m_Sentry.IsValid(), "Entity Expected");
			//m_SentryStatus.m_Entity = m->m_Sentry;

			//EngineFuncs::EntityPosition(m_SentryStatus.m_Entity, m_SentryStatus.m_Position);
			//EngineFuncs::EntityOrientation(m_SentryStatus.m_Entity, m_SentryStatus.m_Facing, NULL, NULL);
			//m_SentryStatus.m_Status = BUILDABLE_BUILT;
			//DBG_MSG(0, m_Client, kNormal, "Sentry Built");
			_cb.AddEntity("sg", m->m_Sentry);
			break;
		}
		HANDLER(TF_MSG_SENTRY_DESTROYED)
		{
			_cb.CallScript();
			//memset(&m_SentryStatus, 0, sizeof(m_SentryStatus));
			//DBG_MSG(0, m_Client, kNormal, "Sentry Destroyed");
			break;
		}
		HANDLER(TF_MSG_SENTRY_SPOTENEMY)
		{
			_cb.CallScript();
			const Event_SentrySpotEnemy_TF *m = _message.Get<Event_SentrySpotEnemy_TF>();
			OBASSERT(m->m_SpottedEnemy.IsValid(), "Entity Expected");
			//DBG_MSG(0, m_Client, kNormal, "Sentry Spot Enemy");
			_cb.AddEntity("enemy", m->m_SpottedEnemy);
			break;
		}
		HANDLER(TF_MSG_SENTRY_DAMAGED)
		{
			_cb.CallScript();
			const Event_SentryTakeDamage_TF *m = _message.Get<Event_SentryTakeDamage_TF>();
			OBASSERT(m->m_Inflictor.IsValid(), "Entity Expected");
			//DBG_MSG(0, m_Client, kNormal, "Sentry Damaged");
			_cb.AddEntity("inflictor", m->m_Inflictor);
			break;
		}
		HANDLER(TF_MSG_SENTRY_STATS)
		{
			/*const Event_SentryStatus_TF *m = _message.Get<Event_SentryStatus_TF>();
			m_SentryStatus.m_Health			= m->m_Health;
			m_SentryStatus.m_MaxHealth		= m->m_MaxHealth;
			m_SentryStatus.m_Shells[0]		= m->m_Shells[0];
			m_SentryStatus.m_Shells[1]		= m->m_Shells[1];
			m_SentryStatus.m_Rockets[0]		= m->m_Rockets[0];
			m_SentryStatus.m_Rockets[1]		= m->m_Rockets[1];
			m_SentryStatus.m_Level			= m->m_Level;

			DBG_MSG(0, m_Client, kNormal, "Sentry Stats");
			DBG_MSG(0, m_Client, kNormal, va("Level: %d/%d", m_SentryStatus.m_Level, 3));
			DBG_MSG(0, m_Client, kNormal, va("Health: %d/%d", m_SentryStatus.m_Health, m_SentryStatus.m_MaxHealth));
			DBG_MSG(0, m_Client, kNormal, va("Shells: %d/%d", m_SentryStatus.m_Shells[0], m_SentryStatus.m_Shells[1]));
			DBG_MSG(0, m_Client, kNormal, va("Rockets: %d/%d", m_SentryStatus.m_Rockets[0], m_SentryStatus.m_Rockets[1]));*/
			break;
		}
		HANDLER(TF_MSG_SENTRY_UPGRADED)
		{
			_cb.CallScript();
			const Event_SentryUpgraded_TF *m = _message.Get<Event_SentryUpgraded_TF>();
			//DBG_MSG(0, m_Client, kNormal, "Sentry Upgraded");
			_cb.AddInt("level", m->m_Level);
			break;
		}
		HANDLER(TF_MSG_SENTRY_DETONATED)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Sentry Detonated");
			break;
		}
		HANDLER(TF_MSG_SENTRY_DISMANTLED)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Sentry Dismantled");
			break;
		}
		HANDLER(TF_MSG_DISPENSER_NOTENOUGHAMMO)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Not Enough Ammo");
			break;
		}
		HANDLER(TF_MSG_DISPENSER_ALREADYBUILT)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Already Built");
			break;
		}
		HANDLER(TF_MSG_DISPENSER_CANTBUILD)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Can't Build");
			break;
		}			
		HANDLER(TF_MSG_DISPENSER_BUILDING)
		{
			_cb.CallScript();
			const Event_DispenserBuilding_TF *m = _message.Get<Event_DispenserBuilding_TF>();
			OBASSERT(m->m_Dispenser.IsValid(), "Entity Expected");
			//m_DispenserStatus.m_Entity = m->m_Dispenser;
			//m_DispenserStatus.m_Status = BUILDABLE_BUILDING;
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Building");
			_cb.AddEntity("dispenser", m->m_Dispenser);
			break;
		}
		HANDLER(TF_MSG_DISPENSER_BUILT)
		{
			_cb.CallScript();
			const Event_DispenserBuilt_TF *m = _message.Get<Event_DispenserBuilt_TF>();
			OBASSERT(m->m_Dispenser.IsValid(), "Entity Expected");
			//m_DispenserStatus.m_Entity = m->m_Dispenser;
			//EngineFuncs::EntityPosition(m_DispenserStatus.m_Entity, m_DispenserStatus.m_Position);
			//EngineFuncs::EntityOrientation(m_DispenserStatus.m_Entity, m_DispenserStatus.m_Facing, NULL, NULL);
			//m_DispenserStatus.m_Status = BUILDABLE_BUILT;
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Built");
			_cb.AddEntity("dispenser", m->m_Dispenser);
			break;
		}
		HANDLER(TF_MSG_DISPENSER_BUILDCANCEL)
		{
			_cb.CallScript();
			//m_DispenserStatus.m_Entity.Reset();
			break;
		}
		HANDLER(TF_MSG_DISPENSER_DESTROYED)
		{
			_cb.CallScript();
			//memset(&m_DispenserStatus, 0, sizeof(m_DispenserStatus));
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Destroyed");
			break;
		}
		HANDLER(TF_MSG_DISPENSER_ENEMYUSED)
		{
			_cb.CallScript();
			const Event_DispenserEnemyUsed_TF *m = _message.Get<Event_DispenserEnemyUsed_TF>();
			OBASSERT(m->m_Enemy.IsValid(), "Entity Expected");
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Enemy Used");
			_cb.AddEntity("usedby", m->m_Enemy);
			break;
		}
		HANDLER(TF_MSG_DISPENSER_DAMAGED)
		{
			_cb.CallScript();
			const Event_DispenserTakeDamage_TF *m = _message.Get<Event_DispenserTakeDamage_TF>();
			OBASSERT(m->m_Inflictor.IsValid(), "Entity Expected");
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Damaged");
			_cb.AddEntity("inflictor", m->m_Inflictor);
			break;
		}
		HANDLER(TF_MSG_DISPENSER_STATS)
		{
			/*const Event_DispenserStatus_TF *m = _message.Get<Event_DispenserStatus_TF>();
			m_DispenserStatus.m_Health	= m->m_Health;
			m_DispenserStatus.m_Shells	= m->m_Shells;
			m_DispenserStatus.m_Nails	= m->m_Nails;
			m_DispenserStatus.m_Rockets	= m->m_Rockets;
			m_DispenserStatus.m_Cells	= m->m_Cells;
			m_DispenserStatus.m_Armor	= m->m_Armor;

			DBG_MSG(0, m_Client, kNormal, "Dispenser Stats");
			DBG_MSG(0, m_Client, kNormal, va("Health: %d", m_DispenserStatus.m_Health));
			DBG_MSG(0, m_Client, kNormal, va("Shells: %d", m_DispenserStatus.m_Shells));
			DBG_MSG(0, m_Client, kNormal, va("Nails: %d", m_DispenserStatus.m_Nails));
			DBG_MSG(0, m_Client, kNormal, va("Rockets: %d", m_DispenserStatus.m_Rockets));
			DBG_MSG(0, m_Client, kNormal, va("Cells: %d", m_DispenserStatus.m_Cells));
			DBG_MSG(0, m_Client, kNormal, va("Armor: %d", m_DispenserStatus.m_Armor));*/
			break;
		}
		HANDLER(TF_MSG_DISPENSER_DETONATED)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Detonated");
			break;
		}
		HANDLER(TF_MSG_DISPENSER_DISMANTLED)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Dismantled");
			break;
		}
		HANDLER(TF_MSG_DISPENSER_BLOWITUP)
		{
			_cb.CallScript();
			//DBG_MSG(0, m_Client, kNormal, "Dispenser Blowitup");
			break;
		}
		//////////////////////////////////////////////////////////////////////////
		HANDLER(TF_MSG_CALLFORMEDIC)
		{
			_cb.CallScript();
			const Event_MedicCall *m = _message.Get<Event_MedicCall>();
			OBASSERT(m && m->m_ByWho.IsValid(),"Invalid Message Params");
			_cb.AddEntity("who",m->m_ByWho);
			break;
		}
		//////////////////////////////////////////////////////////////////////////
	}
	Client::ProcessEvent(_message, _cb);
}

NavFlags TF_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags TF_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case TF_TEAM_BLUE:
		return F_NAV_TEAM1;
	case TF_TEAM_RED:
		return F_NAV_TEAM2;
	case TF_TEAM_YELLOW:
		return F_NAV_TEAM3;
	case TF_TEAM_GREEN:
		return F_NAV_TEAM4;
	default:
		return defaultTeam;
	}
}

float TF_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 64.0f;
	}
	return 0.0f;
}

float TF_Client::GetAvoidRadius(int _class) const
{	
	switch(_class)
	{
	case TF_CLASS_SCOUT:
	case TF_CLASS_SNIPER:
	case TF_CLASS_SOLDIER:
	case TF_CLASS_DEMOMAN:
	case TF_CLASS_MEDIC:
	case TF_CLASS_HWGUY:
	case TF_CLASS_PYRO:
	case TF_CLASS_SPY:
	case TF_CLASS_ENGINEER:
	case TF_CLASS_CIVILIAN:
		return 24.0f;
	case TF_CLASSEX_SENTRY:
		return 32.0f;
	case TF_CLASSEX_DISPENSER:
		return 20.0f;		
	}
	return 0.0f;
}

bool TF_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	return _mapgoal ? _mapgoal->GetOwner() == GetGameEntity() : false;
}

bool TF_Client::CanBotSnipe() 
{
	if(GetClass() == TF_CLASS_SNIPER)
	{
		// Make sure we have a sniping weapon.
		if (GetWeaponSystem()->HasAmmo(TF_WP_SNIPER_RIFLE))
			return true;
	}
	return false;
}

bool TF_Client::GetSniperWeapon(int &nonscoped, int &scoped)
{
	nonscoped = 0;
	scoped = 0;

	if(GetClass() == TF_CLASS_SNIPER)
	{
		if(GetWeaponSystem()->HasWeapon(TF_WP_SNIPER_RIFLE))
		{
			nonscoped = TF_WP_SNIPER_RIFLE;
			scoped = TF_WP_SNIPER_RIFLE;
			return true;
		}
	}
	return false;
}

float TF_Client::NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to) 
{
	using namespace AiState;
	
	WeaponSystem *wsys = GetWeaponSystem();
	if(_flag & F_TF_NAV_ROCKETJUMP)
	{		
		if(wsys->HasWeapon(TF_Options::ROCKETJUMP_WPN) && wsys->HasAmmo(TF_Options::ROCKETJUMP_WPN))
			return 1.f;
	}

	if(_flag & F_TF_NAV_CONCJUMP)
	{
		if(wsys->HasWeapon(TF_WP_GRENADE_CONC) && wsys->HasAmmo(TF_WP_GRENADE_CONC))
			return 1.f;
	}

	if(_flag & F_NAV_TELEPORT)
	{
		// todo:		
		return 1.f;
	}

	if(_flag & F_TF_NAV_DOUBLEJUMP)
	{
		if(GetClass() == TF_CLASS_SCOUT)
			return 1.f;
	}

	return 0.f;
}

void TF_Client::ProcessGotoNode(const Path &_path)
{
	const bool OnGround = GetEntityFlags().CheckFlag(ENT_FLAG_ONGROUND);
	if(OnGround)
		m_DoubleJumping = false;

	Path::PathPoint pt;
	_path.GetCurrentPt(pt);

	if(pt.m_NavFlags & F_TF_NAV_DOUBLEJUMP)
	{
		if(!m_DoubleJumping && (GetPosition()-pt.m_Pt).Length() < pt.m_Radius)
		{
			//const float Speed = GetVelocity().Length();
			const Vector3f Vel = Normalize(GetVelocity());
			const Vector3f IdealVel = Normalize(pt.m_Pt - GetPosition());
			const float fDot = IdealVel.Dot(Vel);
			
			if(OnGround && fDot > 0.9f)
			{
				m_DoubleJumping = true;
				PressButton(BOT_BUTTON_JUMP);
			}

			static float JumpVelocity = 10.f;
			if(!OnGround && GetVelocity().z > JumpVelocity)
			{
				m_DoubleJumping = true;
			}

			m_DoubleJumpHeight = GetPosition().z;
		}
	}

	if(m_DoubleJumping)
	{
		const float NextHeight = GetPosition().z + GetVelocity().z * IGame::GetDeltaTimeSecs();
		if(NextHeight < m_DoubleJumpHeight)
		{
			PressButton(BOT_BUTTON_JUMP);
			m_DoubleJumping = false;
		}
		return;
	}

	m_DoubleJumping = false;
}

void TF_Client::SetupBehaviorTree()
{
	using namespace AiState;
	delete GetStateRoot()->RemoveState("ReturnTheFlag");
	
	GetStateRoot()->AppendTo("HighLevel", new Sentry);
	GetStateRoot()->AppendTo("HighLevel", new Dispenser);
	GetStateRoot()->AppendTo("HighLevel", new Detpack);
	GetStateRoot()->AppendTo("HighLevel", new PipeTrap);
	GetStateRoot()->AppendTo("MotorControl", new RocketJump);
	GetStateRoot()->AppendTo("MotorControl", new ConcussionJump);
	GetStateRoot()->AppendTo("LowLevel", new ThrowGrenade);	
}

void TF_Client::Update()
{
	using namespace AiState;
	if(TF_Options::POLL_SENTRY_STATUS)
	{
		TF_BuildInfo bi = InterfaceFuncs::GetBuildInfo(this);
		FINDSTATEIF_OPT(Sentry,GetStateRoot(),UpdateSentryStatus(bi.m_SentryStats));
		FINDSTATEIF_OPT(Dispenser,GetStateRoot(),UpdateDispenserStatus(bi.m_DispenserStats));
		FINDSTATEIF_OPT(Detpack,GetStateRoot(),UpdateDetpackStatus(bi.m_Detpack));
		FINDSTATEIF_OPT(Teleporter,GetStateRoot(),UpdateTeleporterStatus(bi.m_TeleporterStats));
	}
	Client::Update();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
