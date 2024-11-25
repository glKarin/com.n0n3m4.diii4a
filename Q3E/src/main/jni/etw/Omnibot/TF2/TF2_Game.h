////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF2_GAME_H__
#define __TF2_GAME_H__

class Waypoint;
class gmMachine;
class gmTableObject;

#include "TF_Game.h"
#include "TF2_Config.h"
#include "TF2_NavigationFlags.h"

// class: TF2_Game
//		Game Type for TF2
class TF2_Game : public TF_Game
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
	const char *GetGameDatabaseAbbrev() const { return "tf2"; }

	void GetWeaponEnumeration(const IntEnum *&_ptr, int &num);

	TF2_Game() {};
	virtual ~TF2_Game() {};
protected:

	void GetGameVars(GameVars &_gamevars);

	// Script support.
	void InitScriptBinds(gmMachine *_machine);
	void InitScriptEvents(gmMachine *_machine, gmTableObject *_table);
	void InitVoiceMacros(gmMachine *_machine, gmTableObject *_table);

	static const float TF2_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags);
	static const float TF2_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags);
};

#endif
