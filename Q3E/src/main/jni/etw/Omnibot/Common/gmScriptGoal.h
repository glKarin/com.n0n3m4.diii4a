#ifndef __GM_SCRIPTGOAL_H__
#define __GM_SCRIPTGOAL_H__

#include "gmBind.h"
#include "ScriptGoal.h"

class gmScriptGoal : public gmBind<AiState::ScriptGoal, gmScriptGoal>
{
public:
	GMBIND_DECLARE_FUNCTIONS( );
	GMBIND_DECLARE_PROPERTIES( );

	// Functions
	static int gmfLimitToClass(gmThread *a_thread);
	static int gmfLimitToNoClass(gmThread *a_thread);
	static int gmfLimitToTeam(gmThread *a_thread);
	static int gmfLimitToPowerUp(gmThread *a_thread);
	static int gmfLimitToNoPowerup(gmThread *a_thread);
	static int gmfLimitToEntityFlag(gmThread *a_thread);
	static int gmfLimitToNoEntityFlag(gmThread *a_thread);
	static int gmfLimitToWeapon(gmThread *a_thread);
	static int gmfLimitToRole(gmThread *a_thread);
	static int gmfLimitTo(gmThread *a_thread);

	static int gmfLimitToNoTarget(gmThread *a_thread);
	static int gmfLimitToTargetClass(gmThread *a_thread);
	static int gmfLimitToTargetTeam(gmThread *a_thread);
	static int gmfLimitToTargetPowerUp(gmThread *a_thread);
	static int gmfLimitToTargetNoPowerUp(gmThread *a_thread);
	static int gmfLimitToTargetEntityFlag(gmThread *a_thread);
	static int gmfLimitToTargetNoEntityFlag(gmThread *a_thread);
	static int gmfLimitToTargetWeapon(gmThread *a_thread);

	static int gmfFinished(gmThread *a_thread);

	static int gmfDidPathSucceed(gmThread *a_thread);
	static int gmfDidPathFail(gmThread *a_thread);
	static int gmfIsActive(gmThread *a_thread);

	static int gmfGoto(gmThread *a_thread);
	static int gmfGotoAsync(gmThread *a_thread);
	static int gmfGotoRandom(gmThread *a_thread);
	static int gmfGotoRandomAsync(gmThread *a_thread);
	static int gmfRouteTo(gmThread *a_thread);
	static int gmfStop(gmThread *a_thread);

	static int gmfAddAimRequest(gmThread *a_thread);
	static int gmfReleaseAimRequest(gmThread *a_thread);
	static int gmfAddWeaponRequest(gmThread *a_thread);
	static int gmfReleaseWeaponRequest(gmThread *a_thread);
	static int gmfUpdateWeaponRequest(gmThread *a_thread);
	static int gmfBlockForWeaponChange(gmThread *a_thread);
	static int gmfBlockForWeaponFire(gmThread *a_thread);
	static int gmfBlockForVoiceMacro(gmThread *a_thread);
	
	static int gmfThreadFork(gmThread *a_thread);
	static int gmfThreadKill(gmThread *a_thread);
	static int gmfSignal(gmThread *a_thread);

	static int gmfDelayGetPriority(gmThread *a_thread);

	static int gmfBlackboardDelay(gmThread *a_thread);
	static int gmfBlackboardIsDelayed(gmThread *a_thread);

	static int gmfMarkTracker(gmThread *a_thread, bool (AiState::ScriptGoal::*_func)(MapGoalPtr));
	static int gmfMarkInProgress(gmThread *a_thread);
	static int gmfMarkInUse(gmThread *a_thread);

	static int gmfAddFinishCriteria(gmThread *a_thread);
	static int gmfClearFinishCriteria(gmThread *a_thread);
	static int gmfQueryMapGoals(gmThread *a_thread);
	static int gmfWatchForEntityCategory(gmThread *a_thread);
	static int gmfWatchForMapGoalsInRadius(gmThread *a_thread);
	static int gmfClearWatchForMapGoalsInRadius(gmThread *a_thread);

	// Property Accessors
	static bool getName(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setName(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getInsertParent(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setInsertParent(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getInsertBefore(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setInsertBefore(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getInsertAfter(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setInsertAfter(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getDisable(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setDisable(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getAutoAdd(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAutoAdd(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	
	static bool getInitializeFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setInitializeFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getSpawnFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setSpawnFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getPriorityFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setPriorityFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getEnterFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setEnterFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getExitFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setExitFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getUpdateFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setUpdateFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getPathThroughFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setPathThroughFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);

	static bool getAimVectorFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAimVectorFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getAimWeaponIdFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAimWeaponIdFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getAutoReleaseAim(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAutoReleaseAim(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getAutoReleaseWeapon(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAutoReleaseWeapon(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getAutoReleaseTrackers(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAutoReleaseTrackers(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getAutoFinishOnUnavailable(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAutoFinishOnUnavailable(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);

	static bool getAutoFinishOnNoProgressSlots(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAutoFinishOnNoProgressSlots(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getAutoFinishOnNoUseSlots(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAutoFinishOnNoUseSlots(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getSkipGetPriorityWhenActive(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setSkipGetPriorityWhenActive(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);

	static bool setGetPriorityDelay(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getGetPriorityDelay(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);

	static bool getAlwaysRecieveEvents(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setAlwaysRecieveEvents(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getEvents(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setEvents(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getCommands(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setCommands(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);

	static bool getDebugString(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setDebugString(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getDebug(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setDebug(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);

	static bool getBot(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getMapGoal(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setMapGoal(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool getScriptPriority(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);
	static bool setScriptPriority(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands);

	static AiState::ScriptGoal *Constructor(gmThread *a_thread);
	static void Destructor(AiState::ScriptGoal *_native);
	static void AsStringCallback(AiState::ScriptGoal * a_object, char * a_buffer, int a_bufferLen);
};

#endif
