#ifndef __RTCW_GAME_H__
#define __RTCW_GAME_H__

class Waypoint;
class gmMachine;
class gmTableObject;

#include "IGame.h"

// class: RTCW_Game
//		Game Type for Returnto Castle Wolfenstein
class RTCW_Game : public IGame
{
public:
	bool Init();

	void RegisterNavigationFlags(PathPlannerBase *_planner);
	NavFlags DeprecatedNavigationFlags() const;
	void RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck);

	virtual void GetMapScriptFile(filePath &script);
	virtual Client *CreateGameClient();

	int GetVersionNum() const;
	const char *GetDLLName() const;
	const char *GetGameName() const;
	const char *GetModSubFolder() const;
	const char *GetNavSubfolder() const;
	const char *GetScriptSubfolder() const;
	const char *GetGameDatabaseAbbrev() const { return "rtcw"; }
	eNavigatorID GetDefaultNavigator() const;
	bool ReadyForDebugWindow() const;

	GoalManager *GetGoalManager();

	void AddBot(Msg_Addbot &_addbot, bool _createnow = true);
	void ClientJoined(const Event_SystemClientConnected *_msg);

	const char *FindClassName(obint32 _classId);

	void GetTeamEnumeration(const IntEnum *&_ptr, int &num);
	void GetWeaponEnumeration(const IntEnum *&_ptr, int &num);

	int GetLogSize();

	RTCW_Game() {};
	virtual ~RTCW_Game() {};
protected:

	void GetGameVars(GameVars &_gamevars);

	// Script support.
	void InitScriptBinds(gmMachine *_machine);
	void InitScriptClasses(gmMachine *_machine, gmTableObject *_table);
	void InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table);
	void InitScriptEvents(gmMachine *_machine, gmTableObject *_table);
	void InitScriptPowerups(gmMachine *_machine, gmTableObject *_table);
	void InitVoiceMacros(gmMachine *_machine, gmTableObject *_table);

	// Commands
	void InitCommands();

	static const float RTCW_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags);
	static const float RTCW_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags);
	static const void RTCW_GetEntityVisDistance(float &_distance, const TargetInfo &_target, const Client *_client);
	static const bool RTCW_CanSensoreEntity(const EntityInstance &_ent);
	static const float RTCW_GetEntityClassAvoidRadius(const int _class);
};

#endif
