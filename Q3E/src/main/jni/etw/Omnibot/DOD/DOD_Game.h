////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __DOD_GAME_H__
#define __DOD_GAME_H__

class Waypoint;
class gmMachine;
class gmTableObject;

#include "IGame.h"
#include "DOD_Config.h"
#include "DOD_NavigationFlags.h"

// class: DOD_Game
//		Game Type for DOD
class DOD_Game : public IGame
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
	const char *GetGameDatabaseAbbrev() const { return "dod"; }

	void GetTeamEnumeration(const IntEnum *&_ptr, int &num);
	void GetWeaponEnumeration(const IntEnum *&_ptr, int &num);

	DOD_Game() {};
	virtual ~DOD_Game() {};
protected:

	void GetGameVars(GameVars &_gamevars);

	// Script support.
	void InitScriptBinds(gmMachine *_machine);
	void InitScriptEvents(gmMachine *_machine, gmTableObject *_table);
	void InitScriptClasses(gmMachine *_machine, gmTableObject *_table);
	void InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table);
	void InitVoiceMacros(gmMachine *_machine, gmTableObject *_table);

	static const float DOD_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags);
	static const float DOD_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags);
};

#endif
