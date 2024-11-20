////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompFF.h"
#include "FF_Game.h"
#include "FF_Config.h"

#include "TF_Client.h"

#include "NameManager.h"
#include "ScriptManager.h"
#include "NavigationManager.h"

IGame *CreateGameInstance()
{
	return new FF_Game;
}

int FF_Game::GetVersionNum() const
{
	return FF_VERSION_LATEST;
}

Client *FF_Game::CreateGameClient()
{
	return new TF_Client;
}

const char *FF_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "ff\\";
#else
	return "ff";
#endif
}

const char *FF_Game::GetDLLName() const
{
#ifdef WIN32
	return "omnibot_ff.dll";
#else
	return "omnibot_ff.so";
#endif
}

const char *FF_Game::GetGameName() const
{
	return "FF";
}

const char *FF_Game::GetNavSubfolder() const
{
#ifdef WIN32
	return "ff\\nav\\";
#else
	return "ff/nav";
#endif
}

const char *FF_Game::GetScriptSubfolder() const
{
#ifdef WIN32
	return "ff\\scripts\\";
#else
	return "ff/scripts";
#endif
}

bool FF_Game::Init() 
{
	if(!TF_Game::Init())
		return false;

	// Set the sensory systems callback for getting aim offsets for entity types.
	//AiState::SensoryMemory::SetEntityTraceOffsetCallback(FF_Game::FF_GetEntityClassTraceOffset);
	//AiState::SensoryMemory::SetEntityAimOffsetCallback(FF_Game::FF_GetEntityClassAimOffset);

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/ff_autoexec.gm", threadId);

	return true;
}

void FF_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 72.f;
}

//const float FF_Game::FF_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
//{
//	if(InRangeT<int>(_class, TF_TEAM_NONE, TF_CLASS_MAX))
//	{
//		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
//			return -20.0f;
//		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
//			return -10.0f;
//		return 5.0f;
//	}
//	return TF_Game::TF_GetEntityClassTraceOffset(_class,_entflags);
//}
//
//const float FF_Game::FF_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
//{
//	if(InRangeT<int>(_class, TF_TEAM_NONE, TF_CLASS_MAX))
//	{
//		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
//			return -20.0f;
//		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
//			return -10.0f;
//		return 5.0f;
//	}
//	return TF_Game::TF_GetEntityClassAimOffset(_class,_entflags);
//}

ClientPtr &FF_Game::GetClientFromCorrectedGameId(int _gameid) 
{
	return m_ClientList[_gameid-1]; 
}

//void FF_Game::AddBot(const String &_name, int _team, int _class, const String _profile, bool _createnow)
//{
//	// Attempt to spawn a bot into the game.
//	if(_createnow)
//		m_BotJoining = true;
//	int iGameID = InterfaceFuncs::Addbot(_name, _team, _class);
//	if(_createnow)
//		m_BotJoining = false;
//	if(iGameID != -1 && _createnow)
//	{
//		if(!m_ClientList[iGameID-1])
//		{
//			// Initialize the appropriate slot in the list.
//			m_ClientList[iGameID-1].reset(CreateGameClient());
//			m_ClientList[iGameID-1]->Init(iGameID);
//		}
//
//		m_ClientList[iGameID-1]->m_DesiredTeam = _team;
//		m_ClientList[iGameID-1]->m_DesiredClass = _class;
//
//		//////////////////////////////////////////////////////////////////////////
//		// Script callbacks
//		if(m_ClientList[iGameID-1]->m_DesiredTeam == -1)
//		{
//			gmVariable vteam = ScriptManager::GetInstance()->ExecBotCallback(
//				m_ClientList[iGameID-1].get(), 
//				"SelectTeam");
//			m_ClientList[iGameID-1]->m_DesiredTeam = vteam.IsInt() ? vteam.GetInt() : -1;
//		}
//		if(m_ClientList[iGameID-1]->m_DesiredClass == -1)
//		{
//			gmVariable vclass = ScriptManager::GetInstance()->ExecBotCallback(
//				m_ClientList[iGameID-1].get(), 
//				"SelectClass");
//			m_ClientList[iGameID-1]->m_DesiredClass = vclass.IsInt() ? vclass.GetInt() : -1;
//		}
//		//////////////////////////////////////////////////////////////////////////
//		g_EngineFuncs->ChangeTeam(iGameID, m_ClientList[iGameID-1]->m_DesiredTeam, NULL);
//		g_EngineFuncs->ChangeClass(iGameID, m_ClientList[iGameID-1]->m_DesiredClass, NULL);
//	}
//}