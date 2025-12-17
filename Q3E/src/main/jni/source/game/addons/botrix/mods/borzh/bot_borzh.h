#ifdef BOTRIX_BORZH

#ifndef __BOTRIX_BOT_BORZHMOD__
#define __BOTRIX_BOT_BORZHMOD__


#include "bot.h"
#include "server_plugin.h"
#include "mods/borzh/types_borzh.h"
#include "mods/borzh/mod_borzh.h"


class CAction; // Forward declaration.

//****************************************************************************************************************
/// Class representing a bot for BorzhMod.
//****************************************************************************************************************
class CBotBorzh: public CBot
{

public:
    /// Constructor.
    CBotBorzh( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence );

    /// Destructor.
    virtual ~CBotBorzh();


    //------------------------------------------------------------------------------------------------------------
    // Next functions are mod dependent. You need to implement those in order to make bot moving around.
    //------------------------------------------------------------------------------------------------------------
    /// Called when player becomes active, before first respawn.
    virtual void Activated();

    /// Called each time bot is respawned.
    virtual void Respawned();

    /// Called when this bot just killed an enemy.
    virtual void KilledEnemy( int iPlayerIndex, CPlayer* pVictim ) {}

    /// Called when enemy just shot this bot.
    virtual void HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow );

    /// Called when chat arrives from other player.
    virtual void ReceiveChat( int iPlayerIndex, CPlayer* pPlayer , bool bTeamOnly, const char* szText );

    /// Called when chat request arrives from other player.
    virtual void ReceiveChatRequest( const CBotChat& cRequest );


protected: // Inherited methods.

    // Called each frame. Set move and look variables. You can also set shooting/crouching/jumping buttons in m_cCmd.buttons.
    virtual void Think();

    // This function get's called when next waypoint in path becomes closer than current one. By default sets new
    // look forward to next waypoint in path (after current one).
    virtual void CurrentWaypointJustChanged();

    // Check waypoint flags and perform waypoint action. Called when arrives to next waypoint. By default this
    // will check if waypoint has health/armor machine flags and if bot needs to use it. If so, then this function
    // returns true to not to call ApplyPathFlags() / DoPathAction(), while performing USE action (for machine),
    // and ApplyPathFlags() / DoPathAction() will be called after using machine.
    virtual bool DoWaypointAction();

    // Check waypoint path flags and set bot flags accordingly. Called when arrives to next waypoint.
    // Note that you need to set action variables in DoPathAction(). This function is just to
    // figure out if need to crouch/sprint/etc and set new aim/destination to next waypoint in path.
    virtual void ApplyPathFlags();

    // Called when started to touch waypoint. You can use iCurrentWaypoint and iNextWaypoint to get path flags.
    // Will set all needed action variables to jump / jump with duck / break.
    virtual void DoPathAction();

    // Return true if given player is enemy.
    virtual bool IsEnemy( CPlayer* pPlayer ) const { return false; }

    // Bot just picked up given item.
    virtual void PickItem( const CEntity& cItem, TEntityType iEntityType, TEntityIndex iIndex );


protected: // Methods.

    // Set areas that can be reached from current area according to seen&opened doors.
    void SetReachableAreas( const good::bitset& cOpenedDoors );

    // Check if button is reachable.
    TWaypointId GetButtonWaypoint( TEntityIndex iButton, const good::bitset& cReachableAreas, bool bSameArea = false  );

    // Check if door is reachable.
    static TWaypointId GetDoorWaypoint( TEntityIndex iDoor, const good::bitset& cReachableAreas );

    // Return true if current task needs collaborative players.
    bool IsCollaborativeTask() { return (m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor) || (m_cCurrentBigTask.iTask == EBorzhTaskBringBox) ||
                                        (m_cCurrentBigTask.iTask == EBorzhTaskGoToGoal); }

    // Return true if using a planner.
    bool IsUsingPlanner() { return IsCollaborativeTask() && (m_cCurrentBigTask.iArgument&1); }

    // Check if player is in area where door is.
    bool IsPlayerInDoorArea( TPlayerIndex iPlayer, const CEntity& cDoor )
    {
        TAreaId iArea = m_aPlayersAreas[iPlayer];
        if ( iArea == EAreaIdInvalid )
            return false;
        TWaypointId iWaypoint1 = cDoor.iWaypoint;
        TWaypointId iWaypoint2 = (TWaypointId)cDoor.pArguments;
        BASSERT( iWaypoint1 != EWaypointIdInvalid && iWaypoint2 != EWaypointIdInvalid );
        TAreaId iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
        TAreaId iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
        return (iArea == iArea1) || (iArea == iArea2);
    }

    // Check if there is a box in area.
    good::vector<CBoxInfo>::iterator GetBoxInArea( TAreaId iArea )
    {
        good::vector<CBoxInfo>::iterator result;
        for ( result = m_aBoxes.begin(); result != m_aBoxes.end(); ++result )
            if ( result->iArea == iArea )
                break;
        return result;
    }

    // Get last plan step performer.
    TPlayerIndex GetPlanStepPerformer();

    // Called when bot figures out if a button toggles or doesn't affect a door.
    void SetButtonTogglesDoor( TEntityIndex iButton, TEntityIndex iDoor, bool bToggle )
    {
        if ( bToggle )
            m_cButtonTogglesDoor[iButton].set(iDoor);
        else
            m_cButtonNoAffectDoor[iButton].set(iDoor);
    }

    // When starting new task, check for doors at current waypoint.
    void CheckPathsAtCurrentWaypoint();

    // When coming near door, check it's status.
    void DoorStatusCheck( TEntityIndex iDoor, bool bOpened, bool bNeedToPassThrough, bool bSpoken );

    // Door has same status as bot thinks.
    void DoorStatusSame( TEntityIndex iDoor, bool bOpened, bool bCheckingDoors, bool bSpoken );

    // Bot thinks that door was opened when it is closed, and viceversa.
    void DoorStatusDifferent( TEntityIndex iDoor, bool bOpened, bool bCheckingDoors, bool bSpoken );

    // Bot needs to pass through the door, but it is closed.
    void DoorClosedOnTheWay( TEntityIndex iDoor, bool bCheckingDoors, bool bSpoken );

    // Say hello to all players and start investigating areas.
    void Start();

    // Task stack is empty, check if the big task has more steps.
    bool CheckBigTask();

    // Task stack is empty, check if there is something to do: investigate new area, push some button, or use FF.
    // If iProposedTask is valid, then check if there is some other task of more importance to do. Return true when accepting proposed task.
    bool CheckForNewTasks( TBorzhTask iProposedTask = EBorzhTaskInvalid );

    // Init current task.
    void InitNewTask();

    // Save current task in stack.
    void SaveCurrentTask()
    {
        if ( m_cCurrentTask.iTask == EBorzhTaskInvalid )
            return;
        if ( m_cCurrentTask.iTask == EBorzhTaskWait )
            m_cCurrentTask.iArgument = (m_fEndWaitTime - CBotrixPlugin::fTime) * 1000; // Save only time left to wait.
        m_cTaskStack.push_back(m_cCurrentTask);
        m_cCurrentTask.iTask = EBorzhTaskInvalid;
    }

    // Wait given amount of milliseconds.
    void Wait( int iMSecs, bool bSaveCurrentTask = false )
    {
        if ( bSaveCurrentTask )
            SaveCurrentTask();

        m_cCurrentTask.iTask = EBorzhTaskWait;
        m_cCurrentTask.iArgument = iMSecs;
        m_fEndWaitTime = CBotrixPlugin::fTime + iMSecs / 1000.0f;
        m_bNeedMove = m_bNeedAim = false;
    }

    // Speak about door/button/box/weapon.
    void PushSpeakTask( TBotChat iChat, int iArgument = 0, TEntityType iType = EEntityTypeInvalid, TEntityIndex iIndex = EEntityIndexInvalid );

    // Switch to speak task.
    void SwitchToSpeakTask( TBotChat iChat, int iArgument = 0, TEntityType iType = EEntityTypeInvalid, TEntityIndex iIndex = EEntityIndexInvalid )
    {
        SaveCurrentTask();
        PushSpeakTask(iChat, iArgument, iType, iIndex);
        m_cCurrentTask.iTask = EBorzhTaskInvalid;
    }

    // Start planner trying to reach goal area for all collaborative players.
    void StartPlannerForGoal();

    // Check if planner can bring some box to area to climb to a new area.
    bool CheckBoxForAreas();

    // Check if there is some unknown button-door configuration and start planner trying to figuring it out.
    // Return false if there is none such configuration.
    bool CheckButtonDoorConfigurations();

    // When carrying a box check if it is not fallen.
    void CheckCarryingBox();

    // Perform speak task, say a phrase.
    void DoSpeakTask( int iArgument );

    // End big task.
    void BigTaskFinish();

    // Get new task from stack.
    void TaskPop()
    {
        BASSERT( m_cCurrentTask.iTask == EBorzhTaskInvalid );

        if ( m_cTaskStack.size() > 0 )
        {
            m_cCurrentTask = m_cTaskStack.back();
            m_cTaskStack.pop_back();
            m_bNewTask = true;
        }
        m_bTaskFinished = false;
    }

    // Cancel last task.
    void CancelTasksInStack()
    {
        m_cTaskStack.clear();
        m_cCurrentTask.iTask = EBorzhTaskInvalid;
        m_bNeedMove = m_bNeedAim = false;
    }


    // Go to button and push it.
    void PushPressButtonTask( TEntityIndex iButton );

    // Offer task to go to button and push it.
    void PushCheckButtonTask( TEntityIndex iButton, TEntityIndex iDoor );

    // Carry box (maybe from upstairs).
    void PerformActionCarryBox( TEntityIndex iBox );

    // Drop given box.
    void PerformActionDropBox( TEntityIndex iBox );

    // Go to box and use gravity gun on it.
    void PushGrabBoxTask( TEntityIndex iBox, TWaypointId iBoxWaypoint, bool bSpeak = true );

    // Release box at needed waypoint. Will check totem waypoint and look at it before dropping the box.
    void PushDropBoxTask( TEntityIndex iBox, TWaypointId iWhere );

    // Drop box in place.
    void DropBox();

    // Check if all collaborative players accepted big task.
    void CheckAcceptedPlayersForCollaborativeTask();

    // Offer current task to players.
    void OfferCollaborativeTask( int iArgument = -1 );

    // Receive task offer from another player.
    void ReceiveTaskOffer( TBorzhTask iProposedTask, int iArgument1, int iArgument2, TPlayerIndex iSpeaker );

    // Button was pushed, update reacheable areas according to which doors button closes.
    void ButtonPushed( TEntityIndex iButton );

    // Return true if task of planner is finished.
    bool IsPlanPerformed();

    // Perform next step in plan in execution. Return false if there is nothing left to do.
    bool PlanStepNext();

    // Execute action of the plan.
    void PlanStepExecute( const CAction& cAction );

    // Execute last step of the plan.
    void PlanStepLast();

    // Called when unexpected chat arrives when performing collaborative task.
    void UnexpectedChatForCollaborativeTask( TPlayerIndex iSpeaker, bool bWaitForThisPlayer );

    // Cancel collaborative task (if planner task, stop planner).
    void CancelCollaborativeTask( TPlayerIndex iWaitForPlayer = EPlayerIndexInvalid );

    // Check if goal of plan is reached. If it is push button and we are in button area, then change objective to push it.
    void CheckGoalReached( TPlayerIndex iSpeaker );

    // Check if bot sees a box.
    void CheckIfSeeBox();

    // Check if box is needed to go back, and bring it in that case.
    void CheckToReturnBox();


protected: // Members.

    friend void GeneratePddl( bool bFromBotBelief );

    class CBorzhTask
    {
    public:
        CBorzhTask(TBorzhTask iTask = EBorzhTaskInvalid, int iArgument = 0):
            iTask(iTask), iArgument(iArgument), pArguments(NULL) {}

        TBorzhTask iTask;                                   // Task number.
        int iArgument;                                      // Task argument. May be number of door, button, etc.
        void* pArguments;                                   // Other task arguments;
    };

protected: // Flags.

    bool m_bStarted:1;                                      // Started (someone must say 'start'). Say 'pause' to pause all bots.
    bool m_bWasMovingBeforePause:1;                         // True if bot was moving before pause.

    bool m_bSaidHello:1;                                    // To not say hello twice.
    bool m_bNothingToDo:1;                                  // Bot has no more task to do, wait for domain change.

    bool m_bNewTask:1;                                      // New task need to be initiated.
    bool m_bTaskFinished:1;                                 // Need to pop new task from stack.

    bool m_bUsingPlanner:1;                                 // Trying to get plan for something currently.
    bool m_bUsedPlannerForGoal:1;                           // .
    bool m_bUsedPlannerForBox:1;                            // .
    bool m_bUsedPlannerForButton:1;                         // .

    bool m_bCarryingBox:1;                                  // .
    bool m_bLostBox:1;                                      // .

protected: // Members.
    static const int m_iTimeAfterPushingButton = 4000;      // Wait 2 seconds after pushing button.
    static const int m_iTimeAfterSpeak = 3000;              // Wait 3 seconds after speaking (other bots will stop and wait too).
    static const int m_iTimeToWaitPlayer = 30000;           // Wait 60 seconds for another player.

    static good::vector<TEntityIndex> m_aLastPushedButtons; // We need to know the order of pushed buttons.
    static CBorzhTask m_cCurrentProposedTask;               // Last proposed task.

    typedef good::vector< CBorzhTask > task_stack_t;        // Typedef for stack of tasks.
    good::string m_sNameWithoutSpaces;                      // Bot's name without spaces to use it in planner. TODO:

    task_stack_t m_cTaskStack;                              // Stack of tasks for small tasks.
    CBorzhTask m_cCurrentTask;                              // Current bot's task.

    CBorzhTask m_cCurrentBigTask;                           // Current bot's task at high level.

    good::bitset m_aVisitedWaypoints;                       // Visited waypoints (1 = visited, 0 = unknown).

    good::bitset m_cVisitedAreas;                           // Areas that were explored (1 = explored, 0 = unknown).
    good::bitset m_cReachableAreas;                         // Areas that can be reached from current one.
    //good::bitset m_cAuxReachableAreas;                      // Aux reachable areas, tested when need to push button and test if area can be reached.
    good::bitset m_cVisitedAreasAfterPushButton;            // Areas that wasn't visited after pushing buttons. TODO

    good::bitset m_cSeenDoors;                              // Seen doors. Other bot could see it also.
    good::bitset m_cOpenedDoors;                            // Opened doors. Closed door means seen & !opened.
    good::bitset m_cFalseOpenedDoors;                       // Used when checking a button.
    good::bitset m_cCheckedDoors;                           // Useful bitset when checking doors.

    good::bitset m_cSeenButtons;                            // Seen buttons. Other bot could see it also.
    good::bitset m_cPushedButtons;                          // Buttons pushed at least once.

    good::bitset m_cSeenWalls;                              // Areas that were explored (1 = explored, 0 = unknown).

    good::vector<good::bitset> m_cButtonTogglesDoor;        // Bot's belief of which set of doors button DO toggle (button is index in array).
    good::vector<good::bitset> m_cButtonNoAffectDoor;       // Bot's belief of which set of doors button DOESN'T toggle (button is index in array).

    good::vector<good::bitset> m_cTestedToggles;            // Button-doors configurations that has been tested. When domain change is produced, they will be cleared.

    good::bitset m_cCollaborativePlayers;                   // Players that are collaborating with this bot.
    good::bitset m_cWaitingPlayers;                         // Players that are waiting for this bot.
    good::bitset m_cBusyPlayers;                            // Players that are busy. m_bNothingToDo will be true until all of them became idle.
    good::bitset m_cAcceptedPlayers;                        // Players that accepted this bot task. All players need to accept task in order to perform it.
    good::bitset m_cPlayersWithPhyscannon;                  // Players that have a gravity gun.
    good::bitset m_cPlayersWithCrossbow;                    // Players that have a crossbow.
    good::vector<TAreaId> m_aPlayersAreas;                  // Bot's belief of where another player is.
    good::vector<TBorzhTask> m_aPlayersTasks;               // Bot's belief of what another player is doing.

    good::vector<CBoxInfo> m_aBoxes;                        // Bot's belief of where some box is.

    TWeaponId m_iCrossbow;                                  // Index of crossbow. -1 if bot doesn't have it.

    TEntityIndex m_iBox;                                    // If m_bCarryingBox then this is box index.
    Vector m_vPrevBoxPosition;                              // Util when box stucks to know when it stops moving.

    int m_iPlanStep;                                        // Current plan step.
    float m_fEndWaitTime;                                   // Time when can stop waiting.

    float m_fNextBoxCheck;                                  // Time when will check next box.

};


#endif // __BOTRIX_BOT_BORZHMOD__

#endif // BOTRIX_BORZH
