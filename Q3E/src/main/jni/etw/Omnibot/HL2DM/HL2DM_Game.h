////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __HL2DM_GAME_H__
#define __HL2DM_GAME_H__

class Waypoint;
class gmMachine;
class gmTableObject;

#include "IGame.h"
#include "HL2DM_Config.h"
#include "HL2DM_NavigationFlags.h"

// class: HL2DM_Game
//		Game Type for Half-life 2 Deathmatch
class HL2DM_Game : public IGame
{
public:
	bool Init();

	void RegisterNavigationFlags(PathPlannerBase *_planner);
	void RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck);

	virtual Client *CreateGameClient();

	int GetVersionNum() const;
	const char *GetDLLName() const;
	const char *GetGameName() const;
	const char *GetModSubFolder() const;
	const char *GetNavSubfolder() const;
	const char *GetScriptSubfolder() const;
	const char *GetGameDatabaseAbbrev() const { return "hl2dm"; }

	const char *FindClassName(obint32 _classId);

	void GetTeamEnumeration(const IntEnum *&_ptr, int &num);
	void GetWeaponEnumeration(const IntEnum *&_ptr, int &num);

	HL2DM_Game() {};
	virtual ~HL2DM_Game() {};
protected:

	void GetGameVars(GameVars &_gamevars);

	// Script support.
	void InitScriptClasses(gmMachine *_machine, gmTableObject *_table);
	void InitScriptEvents(gmMachine *_machine, gmTableObject *_table);

	// Commands
	void InitCommands();

	static const float HL2DM_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags);
	static const float HL2DM_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags);
};

#endif
