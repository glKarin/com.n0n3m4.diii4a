#ifndef __GM_BOT_H__
#define __GM_BOT_H__

#include "gmBind.h"
#include "Client.h"

class gmBot : public gmBind<Client, gmBot>
{
public:
	GMBIND_DECLARE_FUNCTIONS();
	GMBIND_DECLARE_PROPERTIES();

	//////////////////////////////////////////////////////////////////////////
	// Accessors
	static int gmfGetGameEntity(gmThread *a_thread);
	static int gmfGetGameId(gmThread *a_thread);
	static int gmfGetPosition(gmThread *a_thread);
	static int gmfGetEyePosition(gmThread *a_thread);
	static int gmfGetFacing(gmThread *a_thread);
	static int gmfGetVelocity(gmThread *a_thread);
	static int gmfIsStuck(gmThread *a_thread);
	static int gmfResetStuckTime(gmThread *a_thread);
	static int gmfGetTeam(gmThread *a_thread);
	static int gmfGetClassId(gmThread *a_thread);
	static int gmfGetAllType(gmThread *a_thread);
	static int gmfGetAllEnemy(gmThread *a_thread);
	static int gmfGetAllAlly(gmThread *a_thread);
	static int gmfGetNearestEnemy(gmThread *a_thread);
	static int gmfGetNearestAlly(gmThread *a_thread);
	static int gmfGetNearest(gmThread *a_thread);
	static int gmfGetTarget(gmThread *a_thread);
	static int gmfGetLastTarget(gmThread *a_thread);
	static int gmfForceTarget(gmThread *a_thread);
	static int gmfGetTargetInfo(gmThread *a_thread);
	static int gmfIgnoreTargetForTime(gmThread *a_thread);
	static int gmfIgnoreTarget(gmThread *a_thread);
	static int gmfGetWeapon(gmThread *a_thread);
	static int gmfGetIsAllied(gmThread *a_thread);
	static int gmfInFieldOfView(gmThread *a_thread);
	static int gmfHasLineOfSightTo(gmThread *a_thread);
	static int gmfHasEntityFlagAll(gmThread *a_thread);
	static int gmfHasEntityFlagAny(gmThread *a_thread);
	static int gmfHasAnyWeapon(gmThread *a_thread);
	static int gmfHasTarget(gmThread *a_thread);
	static int gmfHasPowerUp(gmThread *a_thread);
	static int gmfGetHealthPercent(gmThread *a_thread);
	static int gmfAddScriptGoal(gmThread *a_thread);
	static int gmfFindState(gmThread *a_thread);
	static int gmfRemoveState(gmThread *a_thread);
	static int gmfSetStateEnabled(gmThread *a_thread);
	static int gmfCanSnipe(gmThread *a_thread);
	static int gmfGetSkills(gmThread *a_thread);
	static int gmfGetStat(gmThread *a_thread);
	static int gmfGetHighLevelGoalName(gmThread *a_thread);
	static int gmfGetMapGoalName(gmThread *a_thread);
	static int gmfSetRoles(gmThread *a_thread);
	static int gmfClearRoles(gmThread *a_thread);
	static int gmfHasRole(gmThread *a_thread);
	static int gmfIsCarryingFlag(gmThread *a_thread);
	static int gmfCanGrabItem(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////
	// Modifiers
	static int gmfSetMoveTo(gmThread *a_thread);
	static int gmfSetDebugFlag(gmThread *a_thread);

	static int gmfPressButton(gmThread *a_thread);
	static int gmfHoldButton(gmThread *a_thread);
	static int gmfReleaseButton(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////
	// Utilities
	static int gmfToLocalSpace(gmThread *a_thread);
	static int gmfToWorldSpace(gmThread *a_thread);	
	static int gmfDumpBotTable(gmThread *a_thread);
	static int gmfDistanceTo(gmThread *a_thread);
	static int gmfGetNearestDestination(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////
	// Actions
	static int gmfExecCommand(gmThread *a_thread);
	static int gmfSayVoice(gmThread *a_thread);
	static int gmfSay(gmThread *a_thread);
	static int gmfSayTeam(gmThread *a_thread);
	static int gmfPlaySound(gmThread *a_thread);
	static int gmfStopSound(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////
	// Weapon System Functions
	static int gmfClearWeapons(gmThread *a_thread);
	static int gmfFireWeapon(gmThread *a_thread);	
	static int gmfGetCurrentWeapon(gmThread *a_thread);
	static int gmfGetAmmo(gmThread *a_thread);
	static int gmfHasWeapon(gmThread *a_thread);
	static int gmfHasAmmo(gmThread *a_thread);
	static int gmfIsWeaponCharged(gmThread *a_thread);
	static int gmfGetMostDesiredAmmo(gmThread *a_thread);
	static int gmfGetBestWeapon(gmThread *a_thread);
	static int gmfGetRandomWeapon(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////
	// Goal Functions
	static int gmfSetGoal_GetAmmo(gmThread *a_thread);
	static int gmfSetGoal_GetArmor(gmThread *a_thread);
	static int gmfSetGoal_GetHealth(gmThread *a_thread);

	static int gmfChangeTeam(gmThread *a_thread);
	static int gmfChangeClass(gmThread *a_thread);
	static int gmfReloadProfile(gmThread *a_thread);

	static int gmfScriptEvent(gmThread *a_thread);
	static int gmfScriptMessage(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////
	// Systems
	static int gmfEnable(gmThread *a_thread);
	static int gmfSetEnableShooting(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////
	// Property Accessors
	static bool getName( Client *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool setName( Client *a_native, gmThread *a_thread, gmVariable *a_operands );
	
	static bool getMemorySpan( Client *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool setMemorySpan( Client *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool getAimPersistance( Client *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool setAimPersistance( Client *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool getReactionTime( Client *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool setReactionTime( Client *a_native, gmThread *a_thread, gmVariable *a_operands );

	//////////////////////////////////////////////////////////////////////////
	static Client *Constructor(gmThread *a_thread);
	static void Destructor(Client *_native);
	static void DebugInfo(gmUserObject *a_object, gmMachine *a_machine, gmChildInfoCallback a_infoCallback);
};

#endif

