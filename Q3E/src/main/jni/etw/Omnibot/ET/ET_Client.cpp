#include "PrecompET.h"
#include "ScriptManager.h"

#include "ET_Client.h"
#include "ET_NavigationFlags.h"
#include "ET_VoiceMacros.h"
#include "ET_FilterClosest.h"
#include "ET_Messages.h"
//#include "ET_BaseStates.h"
#include "ET_Game.h"

//////////////////////////////////////////////////////////////////////////
// MOVE THIS

class Limbo : public StateSimultaneous
{
public:
	obReal GetPriority() 
	{
		return GetClient()->HasEntityFlag(ET_ENT_FLAG_INLIMBO) ? 1.f : 0.f;		
	}
	void Exit()
	{
		GetRootState()->OnSpawn();
		GetClient()->m_SpawnTime = IGame::GetTime();
	}
	State::StateStatus Update(float fDt) 
	{
		// need to do something special here?
		return State_Busy; 
	}
	Limbo() : StateSimultaneous("Limbo") 
	{
	}
protected:
};

class Incapacitated : public StateChild
{
	obint32 m_Timeout;

public:
	obReal GetPriority() 
	{
		return !InterfaceFuncs::IsAlive(GetClient()->GetGameEntity()) ? 1.f : 0.f; 
	}

	void Enter()
	{
		m_Timeout = IGame::GetTime() + 60000;
	}

	State::StateStatus Update(float fDt) 
	{
		if(InterfaceFuncs::GetReinforceTime(GetClient()) < 1.0f)
		{
			if(!InterfaceFuncs::IsMedicNear(GetClient()) || IGame::GetTime() > m_Timeout) 
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

ET_Client::ET_Client() :
	m_BreakableTargetDistance(0.f)
{
	m_StepHeight = 8.0f; // subtract a small value as a buffer to jump
}

ET_Client::~ET_Client()
{
}

void ET_Client::Init(int _gameid)
{
	if(IGame::GetGameState() != GAME_STATE_PLAYING) //warmup
		InterfaceFuncs::ChangeSpawnPoint(g_EngineFuncs->EntityFromID(_gameid), 0); //default spawn

	Client::Init(_gameid);

	FilterPtr filter(new ET_FilterClosest(this, AiState::SensoryMemory::EntEnemy));
	filter->AddCategory(ENT_CAT_SHOOTABLE);
	GetTargetingSystem()->SetDefaultTargetingFilter(filter);
}

int ET_Client::ConvertWeaponIdToMod(int weaponId)
{
	if(ET_Game::IsNoQuarter)
	{
		if(GetTeam()==OB_TEAM_1) { //Axis
			switch(weaponId){
				case ET_WP_MORTAR:
					weaponId = 92; //GRANATWERFER
					break;
				case ET_WP_MORTAR_SET:
					weaponId = 93; //GRANATWERFER_SET
					break;
			}
		}
		else { //Allies
			switch(weaponId){
				case ET_WP_MOBILE_MG42:
					weaponId = 88; //MOBILE_BROWNING
					break;
				case ET_WP_MOBILE_MG42_SET:
					weaponId = 89; //MOBILE_BROWNING_SET
					break;
				case ET_WP_KNIFE:
					weaponId = 94; //KNIFE_KABAR
					break;
			}
		}
	}
	return weaponId;
}

void ET_Client::UpdateBotInput()
{
	//ETBlight and bastardmod sniper hack
	if(m_ClientInput.m_ButtonFlags.CheckFlag(BOT_BUTTON_AIM))
	{
		if(ET_Game::IsETBlight)
		{
			if(m_ClientInput.m_CurrentWeapon == 72) //MN PROTO
			{
				m_ClientInput.m_CurrentWeapon = 75;
				m_ClientInput.m_ButtonFlags.ClearFlag(BOT_BUTTON_AIM);
			}
			if(m_ClientInput.m_CurrentWeapon == 79) //MP40SS
			{
				m_ClientInput.m_CurrentWeapon = 80;
				m_ClientInput.m_ButtonFlags.ClearFlag(BOT_BUTTON_AIM);
			}
		}
		if(ET_Game::IsBastardmod)
		{
			if(m_ClientInput.m_CurrentWeapon == 59) //BASTARD FG42
			{
				m_ClientInput.m_CurrentWeapon = 60;
			}
		}
	}

	m_ClientInput.m_CurrentWeapon = ConvertWeaponIdToMod(m_ClientInput.m_CurrentWeapon);

	Client::UpdateBotInput();
}

void ET_Client::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	switch(_message.GetMessageId())
	{
		HANDLER(ET_EVENT_PRETRIGGER_MINE)
		HANDLER(ET_EVENT_POSTTRIGGER_MINE)
		{
			_cb.CallScript();
			const Event_TriggerMine_ET *m = _message.Get<Event_TriggerMine_ET>();
			_cb.AddEntity("mine_entity", m->m_MineEntity);

			/*EntityInfoPair pr;
			pr.first = m->m_MineEntity;
			GetSensoryMemory()->UpdateWithTouchSource(pr);*/

			BitFlag64 b;
			b.SetFlag(BOT_BUTTON_SPRINT,true);
			HoldButton(b, 3000);
			break;
		}
		HANDLER(ET_EVENT_MORTAR_IMPACT)
		{
			_cb.CallScript();
			const Event_MortarImpact_ET *m = _message.Get<Event_MortarImpact_ET>();
			_cb.AddVector("position", m->m_Position[0], m->m_Position[1], m->m_Position[2]);
			break;
		}
		HANDLER(ET_EVENT_FIRETEAM_CREATED)
		{
			_cb.CallScript();
			const Event_FireTeamCreated *m = _message.Get<Event_FireTeamCreated>();		
			_cb.AddInt("fireteamnum",m->m_FireTeamNum);
			break;
		}
		HANDLER(ET_EVENT_FIRETEAM_DISBANDED)
		{
			_cb.CallScript();
			//const Event_FireTeamDisbanded *m = _message.Get<Event_FireTeamDisbanded>();
			break;
		}
		HANDLER(ET_EVENT_FIRETEAM_JOINED)
		{
			_cb.CallScript();
			const Event_FireTeamJoined *m = _message.Get<Event_FireTeamJoined>();		
			_cb.AddEntity("teamleader",m->m_TeamLeader);
			break;
		}
		HANDLER(ET_EVENT_FIRETEAM_LEFT)
		{
			_cb.CallScript();
			//const Event_FireTeamLeft *m = _message.Get<Event_FireTeamLeft>();
			break;
		}
		HANDLER(ET_EVENT_FIRETEAM_INVITED)
		{
			_cb.CallScript();
			const Event_FireTeamInvited *m = _message.Get<Event_FireTeamInvited>();		
			_cb.AddEntity("teamleader",m->m_TeamLeader);
			break;
		}
		HANDLER(ET_EVENT_FIRETEAM_PROPOSAL)
		{
			_cb.CallScript();
			const Event_FireTeamProposal *m = _message.Get<Event_FireTeamProposal>();		
			_cb.AddEntity("invitee",m->m_Invitee);
			break;
		}
		HANDLER(ET_EVENT_FIRETEAM_WARNED)
		{
			_cb.CallScript();
			const Event_FireTeamWarning *m = _message.Get<Event_FireTeamWarning>();		
			_cb.AddEntity("warnedby",m->m_WarnedBy);
			break;
		}
		HANDLER(ET_EVENT_RECIEVEDAMMO)
		{
			const Event_Ammo *m = _message.Get<Event_Ammo>();
			_cb.CallScript();
			_cb.AddEntity("who", m->m_WhoDoneIt);
			break;
		}
	}
	Client::ProcessEvent(_message, _cb);
}

NavFlags ET_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags ET_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case ET_TEAM_AXIS:
		return F_NAV_TEAM1;
	case ET_TEAM_ALLIES:
		return F_NAV_TEAM2;	
	default:
		return defaultTeam;
	}
}

void ET_Client::SendVoiceMacro(int _macroId) 
{
	ET_VoiceMacros::SendVoiceMacro(this, _macroId);
}

int ET_Client::HandleVoiceMacroEvent(const MessageHelper &_message)
{
	const Event_VoiceMacro *m = _message.Get<Event_VoiceMacro>();
	
	int iVoiceId = ET_VoiceMacros::GetVChatId(m->m_MacroString);
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
			if(m->m_WhoSaidIt.IsValid() && (GetClass() == ET_CLASS_MEDIC))
			{
				// FIXME
				/*BotBrain::EvaluatorPtr eval(new ET_Evaluator_RequestGiveHealth(this, m->m_WhoSaidIt));
				if(GetBrain())
					GetBrain()->AddGoalEvaluator(eval);*/
			}
			break;
		}
	case VCHAT_TEAM_NEEDAMMO:
		{
			if(m->m_WhoSaidIt.IsValid() && (GetClass() == ET_CLASS_FIELDOPS))
			{
				// FIXME
				/*BotBrain::EvaluatorPtr eval(new ET_Evaluator_RequestGiveAmmo(this, m->m_WhoSaidIt));
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

void ET_Client::ProcessGotoNode(const Path &_path)
{
	Path::PathPoint pt;
	_path.GetCurrentPt(pt);

	if(pt.m_NavFlags & F_ET_NAV_SPRINT)
	{
		PressButton(BOT_BUTTON_SPRINT);
	}

	// test for inwater / jump to move to surface
	if(pt.m_NavFlags & F_NAV_INWATER)
	{
		PressButton(BOT_BUTTON_JUMP);
	}

	if(pt.m_NavFlags & F_ET_NAV_STRAFE_L)
	{		
		PressButton(BOT_BUTTON_LSTRAFE);
	}
	else if(pt.m_NavFlags & F_ET_NAV_STRAFE_R)
	{
		PressButton(BOT_BUTTON_RSTRAFE);
	}
}

float ET_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 0.32f;
	}
	return 0.0f;
}

float ET_Client::GetAvoidRadius(int _class) const
{
	if(_class < FilterSensory::ANYPLAYERCLASS && _class > 0)
		return 16.0f;

	switch(_class)
	{
	//case ENT_CLASS_GENERIC_BUTTON:
	case ENT_CLASS_GENERIC_HEALTH:
	case ENT_CLASS_GENERIC_AMMO:
	case ENT_CLASS_GENERIC_ARMOR:
		return 5.0f;
	}

	switch(_class - ET_Game::CLASSEXoffset)
	{
	case ET_CLASSEX_DYNAMITE:
	case ET_CLASSEX_MINE:
	case ET_CLASSEX_SATCHEL:
		return 32.0f;
	}
	return 32.0f;
}

bool ET_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	return InterfaceFuncs::HasFlag(this);
}

bool ET_Client::IsFlagGrabbable(MapGoalPtr _mapgoal)
{
	return InterfaceFuncs::ItemCanBeGrabbed(this, _mapgoal->GetEntity());
}

bool ET_Client::IsItemGrabbable(GameEntity _ent)
{
	return InterfaceFuncs::ItemCanBeGrabbed(this, _ent);
}

bool ET_Client::CanBotSnipe() 
{
	// Make sure we have a sniping weapon.
	const int SniperWeapons[] = 
	{
		ET_WP_FG42_SCOPE,
		ET_WP_K43_SCOPE,
		ET_WP_GARAND_SCOPE,
	};
	for(unsigned int i = 0; i < sizeof(SniperWeapons)/sizeof(SniperWeapons[0]); ++i)
	{
		WeaponPtr w = GetWeaponSystem()->GetWeapon(SniperWeapons[i]);
		if(w && w->GetFireMode(Primary).HasAmmo())
			return true;
	}
	return false;
}

bool ET_Client::GetSniperWeapon(int &nonscoped, int &scoped)
{
	nonscoped = 0;
	scoped = 0;

	if(GetClass() == ET_CLASS_COVERTOPS)
	{
		if(GetWeaponSystem()->HasWeapon(ET_WP_FG42))
		{
			nonscoped = ET_WP_FG42;
			scoped = ET_WP_FG42;
			return true;
		}
		if(GetWeaponSystem()->HasWeapon(ET_WP_K43))
		{
			nonscoped = ET_WP_K43;
			scoped = ET_WP_K43;
			return true;
		}
		if(GetWeaponSystem()->HasWeapon(ET_WP_GARAND))
		{
			nonscoped = ET_WP_GARAND;
			scoped = ET_WP_GARAND;
			return true;
		}
	}
	return false;
}

bool ET_Client::GetSkills(gmMachine *machine, gmTableObject *tbl)
{
	ET_PlayerSkills data = {};
	MessageHelper msg(ET_MSG_SKILLLEVEL, &data, sizeof(data));
	if(SUCCESS(InterfaceMsg(msg, GetGameEntity())))
	{
		for(int i = 0; i < ET_SKILLS_NUM_SKILLS; ++i)
			tbl->Set(machine, i, gmVariable(data.m_Skill[i]));
		return true;
	}
	return false;
}

float ET_Client::NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to) 
{
	using namespace AiState;
	String gn;
	
	if(_flag & F_ET_NAV_DISGUISE)
	{
		if(HasEntityFlag(ET_ENT_FLAG_DISGUISED))
			return 1.f;
		return 0.f;
	}

	if(_flag & F_ET_NAV_USEPATH)
	{		
		const PropertyMap::ValueMap &pm = to->GetPropertyMap().GetProperties();
		PropertyMap::ValueMap::const_iterator cIt = pm.begin();
		FINDSTATE(hl,HighLevel,this->GetStateRoot());
		
		if(hl != NULL && hl->GetActiveState())
		{
			gn = Utils::StringToLower(hl->GetActiveState()->GetName());
			for(; cIt != pm.end(); ++cIt)
			{
				if ( gn == (*cIt).first && (*cIt).second == "true" )
					return 1.0f;
			}
		}
	}

	return 0.f;
}

//////////////////////////////////////////////////////////////////////////

void ET_Client::SetupBehaviorTree()
{
	using namespace AiState;
	delete GetStateRoot()->ReplaceState("Dead", new Limbo);
	GetStateRoot()->InsertAfter("Limbo", new Incapacitated);

	//GetStateRoot()->AppendTo("HighLevel", new BuildConstruction);
	//GetStateRoot()->AppendTo("HighLevel", new PlantExplosive);
	//GetStateRoot()->AppendTo("HighLevel", new MountMg42);
	//GetStateRoot()->AppendTo("HighLevel", new RepairMg42);
	//GetStateRoot()->AppendTo("HighLevel", new TakeCheckPoint);
	//GetStateRoot()->AppendTo("HighLevel", new MobileMg42);
	//GetStateRoot()->AppendTo("HighLevel", new MobileMortar);
	//GetStateRoot()->AppendTo("HighLevel", new ReviveTeammate);
	//GetStateRoot()->AppendTo("HighLevel", new DefuseDynamite);
	//GetStateRoot()->AppendTo("HighLevel", new PlantMine);
	//GetStateRoot()->AppendTo("HighLevel", new CallArtillery);
	//GetStateRoot()->AppendTo("HighLevel", new UseCabinet);
	//GetStateRoot()->AppendTo("HighLevel", new Flamethrower);
	//GetStateRoot()->AppendTo("HighLevel", new Panzer);

	//FINDSTATEIF(Flamethrower,GetStateRoot(),LimitToClass().SetFlag(ET_CLASS_SOLDIER));
	//FINDSTATEIF(Panzer,GetStateRoot(),LimitToClass().SetFlag(ET_CLASS_SOLDIER));
}
