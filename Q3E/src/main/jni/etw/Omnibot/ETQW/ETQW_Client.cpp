////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETQW.h"
#include "ScriptManager.h"

#include "ETQW_Client.h"
#include "ETQW_NavigationFlags.h"
#include "ETQW_VoiceMacros.h"
#include "ETQW_FilterClosest.h"
#include "ETQW_Messages.h"
#include "ETQW_BaseStates.h"

//////////////////////////////////////////////////////////////////////////
// MOVE THIS

class Limbo : public StateChild
{
public:

	obReal GetPriority() 
	{
		return GetClient()->HasEntityFlag(ETQW_ENT_FLAG_INLIMBO) ? 1.f : 0.f;		
	}

	void Exit()
	{
		GetRootState()->OnSpawn();
	}

	State::StateStatus Update(float fDt) 
	{
		// need to do something special here?
		return State_Busy; 
	}

	Limbo() : StateChild("Limbo") 
	{
	}
protected:
};

class Incapacitated : public StateChild
{
public:
	obReal GetPriority() 
	{
		return !InterfaceFuncs::IsAlive(GetClient()->GetGameEntity()) ? 1.f : 0.f; 
	}

	State::StateStatus Update(float fDt) 
	{
		if(InterfaceFuncs::GetReinforceTime(GetClient()) < 1.0f)
		{
			if(!InterfaceFuncs::IsMedicNear(GetClient())) 
			{
				InterfaceFuncs::GoToLimbo(GetClient());
			}
		}
		GetClient()->SetMovementVector(Vector3f::ZERO);
		return State_Busy; 
	}

	Incapacitated() : StateChild("Incapacitated") 
	{
	}
protected:
};

//////////////////////////////////////////////////////////////////////////

ETQW_Client::ETQW_Client() :
	m_BreakableTargetDistance(0.f)
{
	m_StepHeight = 8.0f; // subtract a small value as a buffer to jump
}

ETQW_Client::~ETQW_Client()
{
}

void ETQW_Client::Init(int _gameid)
{
	Client::Init(_gameid);

	FilterPtr filter(new ETQW_FilterClosest(this, AiState::SensoryMemory::EntEnemy));
	filter->AddCategory(ENT_CAT_SHOOTABLE);
	GetTargetingSystem()->SetDefaultTargetingFilter(filter);
}

void ETQW_Client::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	switch(_message.GetMessageId())
	{
		HANDLER(ETQW_EVENT_PRETRIGGER_MINE)
		HANDLER(ETQW_EVENT_POSTTRIGGER_MINE)
		{
			const Event_TriggerMine_ETQW *m = _message.Get<Event_TriggerMine_ETQW>();
			_cb.AddEntity("mine_entity", m->m_MineEntity);

			/*EntityInfoPair pr;
			pr.first = m->m_MineEntity;
			GetSensoryMemory()->UpdateWithTouchSource(pr);*/
			break;
		}
		HANDLER(ETQW_EVENT_MORTAR_IMPACT)
		{
			const Event_MortarImpact_ETQW *m = _message.Get<Event_MortarImpact_ETQW>();
			_cb.AddVector("position", m->m_Position[0], m->m_Position[1], m->m_Position[2]);
			break;
		}
	}

	Client::ProcessEvent(_message, _cb);
}

NavFlags ETQW_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags ETQW_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case ETQW_TEAM_STROGG:
		return F_NAV_TEAM1;
	case ETQW_TEAM_GDF:
		return F_NAV_TEAM2;	
	default:
		return defaultTeam;
	}
}

void ETQW_Client::SendVoiceMacro(int _macroId) 
{
	ETQW_VoiceMacros::SendVoiceMacro(this, _macroId);
}

int ETQW_Client::HandleVoiceMacroEvent(const MessageHelper &_message)
{
	const Event_VoiceMacro *m = _message.Get<Event_VoiceMacro>();
	
	int iVoiceId = ETQW_VoiceMacros::GetVChatId(m->m_MacroString);
	switch(iVoiceId)
	{
		/*case VCHAT_TEAM_PATHCLEARED:
		case VCHAT_TEAM_ENEMYWEAK:
		case VCHAT_TEAM_ALLCLEAR:
		case VCHAT_TEAM_INCOMING:*/
	case VCHAT_TEAM_FIREINTHEHOLE:
		{
			break;
		}
		/*case VCHAT_TEAM_ONDEFENSE:
		case VCHAT_TEAM_ONOFFENSE:
		case VCHAT_TEAM_TAKINGFIRE:
		case VCHAT_TEAM_MINESCLEARED:
		case VCHAT_TEAM_ENEMYDISGUISED:*/
	case VCHAT_TEAM_MEDIC:
		{
			if(m->m_WhoSaidIt.IsValid() && (GetClass() == ETQW_CLASS_MEDIC))
			{
				// FIXME
				/*BotBrain::EvaluatorPtr eval(new ETQW_Evaluator_RequestGiveHealth(this, m->m_WhoSaidIt));
				if(GetBrain())
					GetBrain()->AddGoalEvaluator(eval);*/
			}
			break;
		}
	case VCHAT_TEAM_NEEDAMMO:
		{
			if(m->m_WhoSaidIt.IsValid() && (GetClass() == ETQW_CLASS_FIELDOPS))
			{
				// FIXME
				/*BotBrain::EvaluatorPtr eval(new ETQW_Evaluator_RequestGiveAmmo(this, m->m_WhoSaidIt));
				if(GetBrain())
					GetBrain()->AddGoalEvaluator(eval);*/
			}
			break;
		}
		/*case VCHAT_TEAM_NEEDBACKUP:
		case VCHAT_TEAM_NEEDENGINEER:
		case VCHAT_TEAM_COVERME:
		case VCHAT_TEAM_HOLDFIRE:
		case VCHAT_TEAM_WHERETO:
		case VCHAT_TEAM_NEEDOPS:
		case VCHAT_TEAM_FOLLOWME:
		case VCHAT_TEAM_LETGO:
		case VCHAT_TEAM_MOVE:
		case VCHAT_TEAM_CLEARPATH:
		case VCHAT_TEAM_DEFENDOBJECTIVE:
		case VCHAT_TEAM_DISARMDYNAMITE:
		case VCHAT_TEAM_CLEARMINES:
		case VCHAT_TEAM_REINFORCE_OFF:
		case VCHAT_TEAM_REINFORCE_DEF:
		case VCHAT_TEAM_AFFIRMATIVE:
		case VCHAT_TEAM_NEGATIVE:
		case VCHAT_TEAM_THANKS:
		case VCHAT_TEAM_WELCOME:
		case VCHAT_TEAM_SORRY:
		case VCHAT_TEAM_OOPS:*/

		// Command related
		/*case VCHAT_TEAM_COMMANDACKNOWLEDGED:
		case VCHAT_TEAM_COMMANDDECLINED:
		case VCHAT_TEAM_COMMANDCOMPLETED:
		case VCHAT_TEAM_DESTROYPRIMARY:
		case VCHAT_TEAM_DESTROYSECONDARY:
		case VCHAT_TEAM_DESTROYCONSTRUCTION:
		case VCHAT_TEAM_CONSTRUCTIONCOMMENCING:
		case VCHAT_TEAM_REPAIRVEHICLE:
		case VCHAT_TEAM_DESTROYVEHICLE:
		case VCHAT_TEAM_ESCORTVEHICLE:*/

		// Global messages
		/*case VCHAT_GLOBAL_AFFIRMATIVE:
		case VCHAT_GLOBAL_NEGATIVE:
		case VCHAT_GLOBAL_ENEMYWEAK:
		case VCHAT_GLOBAL_HI:
		case VCHAT_GLOBAL_BYE:
		case VCHAT_GLOBAL_GREATSHOT:
		case VCHAT_GLOBAL_CHEER:
		case VCHAT_GLOBAL_THANKS:
		case VCHAT_GLOBAL_WELCOME:
		case VCHAT_GLOBAL_OOPS:
		case VCHAT_GLOBAL_SORRY:
		case VCHAT_GLOBAL_HOLDFIRE:
		case VCHAT_GLOBAL_GOODGAME:*/
	}
	return iVoiceId;
}

void ETQW_Client::ProcessGotoNode(const Path &_path)
{
	Path::PathPoint pt;
	_path.GetCurrentPt(pt);

	if(pt.m_NavFlags & F_ETQW_NAV_SPRINT)
	{
		PressButton(BOT_BUTTON_SPRINT);
	}
}

float ETQW_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 0.32f;
	}
	return 0.0f;
}

float ETQW_Client::GetAvoidRadius(int _class) const
{
	switch(_class)
	{
	//case ENT_CLASS_GENERIC_BUTTON:
	case ENT_CLASS_GENERIC_HEALTH:
	case ENT_CLASS_GENERIC_AMMO:
	case ENT_CLASS_GENERIC_ARMOR:
		return 5.0f;
	}

	switch(_class)
	{
	case ETQW_CLASS_SOLDIER:
	case ETQW_CLASS_MEDIC:
	case ETQW_CLASS_ENGINEER:
	case ETQW_CLASS_FIELDOPS:
	case ETQW_CLASS_COVERTOPS:
		return 16.0f;
	case ETQW_CLASSEX_DYNAMITE:
	case ETQW_CLASSEX_MINE:
	case ETQW_CLASSEX_SATCHEL:
		return 32.0f;
	}

	return 32.0f;
}

bool ETQW_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	return InterfaceFuncs::HasFlag(this);
}

bool ETQW_Client::CanBotSnipe() 
{
	if(GetClass() == ETQW_CLASS_COVERTOPS)
	{
		// Make sure we have a sniping weapon.
		if (GetWeaponSystem()->HasAmmo(ETQW_WP_SNIPERRIFLE))
			return true;
	}
	return false;
}

bool ETQW_Client::GetSniperWeapon(int &nonscoped, int &scoped)
{
	nonscoped = 0;
	scoped = 0;

	if(GetClass() == ETQW_CLASS_COVERTOPS)
	{
		nonscoped = ETQW_WP_SNIPERRIFLE;
		scoped = ETQW_WP_SNIPERRIFLE;
		return true;
	}
	return false;
}

bool ETQW_Client::GetSkills(gmMachine *machine, gmTableObject *tbl)
{
	ETQW_PlayerSkills data = {};
	MessageHelper msg(ETQW_MSG_SKILLLEVEL, &data, sizeof(data));
	if(SUCCESS(InterfaceMsg(msg, GetGameEntity())))
	{
		for(int i = 0; i < ETQW_SKILLS_NUM_SKILLS; ++i)
			tbl->Set(machine, i, gmVariable(data.m_Skill[i]));
		return true;
	}
	return false;
}

float ETQW_Client::NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to) 
{
	using namespace AiState;
	if(_flag & F_ETQW_NAV_DISGUISE)
	{
		if(/*GetClass() == ETQW_CLASS_COVERTOPS && */HasEntityFlag(ETQW_ENT_FLAG_DISGUISED))
			return 1.f;
		return 0.f;
	}	
	return 0.f;
}

//////////////////////////////////////////////////////////////////////////

void ETQW_Client::SetupBehaviorTree()
{
	using namespace AiState;
	delete GetStateRoot()->ReplaceState("Dead", new Limbo);
	GetStateRoot()->InsertAfter("Limbo", new Incapacitated);

	GetStateRoot()->AppendTo("HighLevel", new BuildConstruction);
	GetStateRoot()->AppendTo("HighLevel", new PlantExplosive);
	GetStateRoot()->AppendTo("HighLevel", new MountMg42);
	GetStateRoot()->AppendTo("HighLevel", new TakeCheckPoint);
	GetStateRoot()->AppendTo("HighLevel", new MobileMg42);
	GetStateRoot()->AppendTo("HighLevel", new ReviveTeammate);
	GetStateRoot()->AppendTo("HighLevel", new DefuseDynamite);
}
