
#ifndef __ET_GAME_H__
#define __ET_GAME_H__

class Waypoint;
class gmMachine;
class gmTableObject;

#include "IGame.h"

// class: ET_Game
//		Game Type for Enemy-Territory.
class ET_Game : public IGame
{
public:
	bool Init();

	void RegisterNavigationFlags(PathPlannerBase *_planner);
	NavFlags DeprecatedNavigationFlags() const;
	void RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck);

	virtual Client *CreateGameClient();

	int GetVersionNum() const;
	bool CheckVersion(int _version);
	const char *GetDLLName() const;
	const char *GetGameName() const;
	const char *GetModSubFolder() const;
	const char *GetNavSubfolder() const;
	const char *GetScriptSubfolder() const;
	const char *GetGameDatabaseAbbrev() const { return "et"; }
	eNavigatorID GetDefaultNavigator() const;
	bool ReadyForDebugWindow() const;
	virtual const char *IsDebugDrawSupported() const;

	GoalManager *GetGoalManager();

	void AddBot(Msg_Addbot &_addbot, bool _createnow = true);

	void ClientJoined(const Event_SystemClientConnected *_msg);

	const char *FindClassName(obint32 _classId);

	void GetTeamEnumeration(const IntEnum *&_ptr, int &num);
	void GetWeaponEnumeration(const IntEnum *&_ptr, int &num);

	virtual bool AddWeaponId(const char * weaponName, int weaponId);
	virtual int ConvertWeaponId(int weaponId);

	int GetLogSize();

	static int CLASSEXoffset;
	static bool IsETBlight, IsBastardmod, IsNoQuarter;
	static bool m_WatchForMines;

	ET_Game() {};
	virtual ~ET_Game() {};
protected:

	void GetGameVars(GameVars &_gamevars);

	// Script support.
	void InitScriptBinds(gmMachine *_machine);
	void InitScriptCategories(gmMachine *_machine, gmTableObject *_table);
	void InitScriptClasses(gmMachine *_machine, gmTableObject *_table);
	void InitScriptSkills(gmMachine *_machine, gmTableObject *_table);
	void InitScriptEvents(gmMachine *_machine, gmTableObject *_table);
	void InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table);
	void InitScriptPowerups(gmMachine *_machine, gmTableObject *_table);
	void InitVoiceMacros(gmMachine *_machine, gmTableObject *_table);
	void InitWeaponEnum();

	// Commands
	void InitCommands();

	static const float ET_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags);
	static const float ET_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags);
	static const void ET_GetEntityVisDistance(float &_distance, const TargetInfo &_target, const Client *_client);
	static const bool ET_CanSensoreEntity(const EntityInstance &_ent);
	static void ET_AddSensorCategory(BitFlag32 category);
	static const float ET_GetEntityClassAvoidRadius(const int _class);

	StringBuffer m_ExtraWeaponNames;
	int m_NumWeapons;
};

#endif
