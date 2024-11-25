////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __SKELETON_GAME_H__
#define __SKELETON_GAME_H__

class Waypoint;
class gmMachine;
class gmTableObject;

#include "IGame.h"
#include "Skeleton_Config.h"
#include "Skeleton_NavigationFlags.h"

// class: Skeleton_Game
//		Basic Game subclass
class Skeleton_Game : public IGame
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
	const char *GetGameDatabaseAbbrev() const { return "skele"; }

	const char *FindClassName(obint32 _classId);

	void GetTeamEnumeration(const IntEnum *&_ptr, int &num);
	void GetWeaponEnumeration(const IntEnum *&_ptr, int &num);

	Skeleton_Game() {};
	virtual ~Skeleton_Game() {};
protected:

	void GetGameVars(GameVars &_gamevars);

	// Script support.
	void InitScriptClasses(gmMachine *_machine, gmTableObject *_table);
	void InitScriptEvents(gmMachine *_machine, gmTableObject *_table);

	// Commands
	void InitCommands();

	static const float Skeleton_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags);
	static const float Skeleton_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags);
};

#endif
