#ifdef BOTRIX_BORZH

#include "chat.h"
#include "clients.h"
#include "planner.h"
#include "type2string.h"

#include "mods/borzh/bot_borzh.h"
#include "mods/borzh/mod_borzh.h"

#define GET_TYPE(arg)                GET_1ST_BYTE(arg)
//#define SET_TYPE(type, arg)          SET_1ST_BYTE(type, arg)
#define MAKE_TYPE(type)              MAKE_1ST_BYTE(type)

#define IS_USING_PLANNER()           ( (m_cCurrentBigTask.iArgument & 0x01) )
#define SET_USING_PLANNER()          m_cCurrentBigTask.iArgument |= 0x01
#define CLEAR_USING_PLANNER()        m_cCurrentBigTask.iArgument &= ~0x01

#define IS_ASKED_FOR_HELP()          ( (m_cCurrentBigTask.iArgument & 0x02) )
#define SET_ASKED_FOR_HELP()         m_cCurrentBigTask.iArgument |= 0x02

#define IS_ACCEPTED()                ( (m_cCurrentBigTask.iArgument & 0x04) )
#define SET_ACCEPTED()               m_cCurrentBigTask.iArgument |= 0x04

#define IS_PUSHED()                  ( (m_cCurrentBigTask.iArgument & 0x08) )
#define SET_PUSHED()                 m_cCurrentBigTask.iArgument |= 0x08

#define IS_PLAYER_DONE()             ( (m_cCurrentBigTask.iArgument & 0x10) )
#define SET_PLAYER_DONE()            m_cCurrentBigTask.iArgument |= 0x10
#define CLEAR_PLAYER_DONE()          m_cCurrentBigTask.iArgument &= ~0x10

#define IS_FOLLOWING_ORDERS()        ( (m_cCurrentBigTask.iArgument & 0x20) )
#define SET_FOLLOWING_ORDERS()       m_cCurrentBigTask.iArgument |= 0x20
#define CLEAR_FOLLOWING_ORDERS()     m_cCurrentBigTask.iArgument &= ~0x20

#define IS_ENDED()                   ( (m_cCurrentBigTask.iArgument & 0x40) )
#define SET_ENDED()                  m_cCurrentBigTask.iArgument |= 0x40

#define GET_INDEX(arg)               GET_2ND_BYTE(arg)
#define SET_INDEX(index, arg)        SET_2ND_BYTE(index, arg)
#define MAKE_INDEX(index)            MAKE_2ND_BYTE(index)

#define GET_AREA(arg)                GET_2ND_BYTE(arg)
#define SET_AREA(area, arg)          SET_2ND_BYTE(area, arg)
#define MAKE_AREA(area)              MAKE_2ND_BYTE(area)

#define GET_WEAPON(arg)              GET_2ND_BYTE(arg)
#define SET_WEAPON(w, arg)           SET_2ND_BYTE(w, arg)
#define MAKE_WEAPON(w)               MAKE_2ND_BYTE(w)

#define GET_BUTTON(arg)              GET_2ND_BYTE(arg)
#define SET_BUTTON(button, arg)      SET_2ND_BYTE(button, arg)
#define MAKE_BUTTON(button)          MAKE_2ND_BYTE(button)

#define GET_BOX(arg)                 GET_2ND_BYTE(arg)
#define SET_BOX(box, arg)            SET_2ND_BYTE(box, arg)
#define MAKE_BOX(box)                MAKE_2ND_BYTE(box)

#define GET_DOOR(arg)                GET_3RD_BYTE(arg)
#define SET_DOOR(door, arg)          SET_3RD_BYTE(door, arg)
#define MAKE_DOOR(door)              MAKE_3RD_BYTE(door)

#define GET_DOOR_GOTO(arg)           GET_4TH_BYTE(arg)
#define SET_DOOR_GOTO(door, arg)     SET_4TH_BYTE(door, arg)

#define GET_DOOR_STATUS(arg)         GET_4TH_BYTE(arg)
#define SET_DOOR_STATUS(status, arg) SET_4TH_BYTE(status, arg)
#define MAKE_DOOR_STATUS(status)     MAKE_4TH_BYTE(status)

#define GET_PLAYER(arg)              GET_4TH_BYTE(arg)
#define SET_PLAYER(player, arg)      SET_4TH_BYTE(player, arg)
#define MAKE_PLAYER(player)          MAKE_4TH_BYTE(player)


good::vector<TEntityIndex> CBotBorzh::m_aLastPushedButtons;
CBotBorzh::CBorzhTask CBotBorzh::m_cCurrentProposedTask;

//----------------------------------------------------------------------------------------------------------------
CBotBorzh::CBotBorzh( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence ):
    CBot(pEdict, iIndex, iIntelligence), m_aBoxes(4), m_bStarted(false), m_bLostBox(false)
{
    CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(pEdict, "cl_autowepswitch", "0");
    CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(pEdict, "cl_defaultweapon", "weapon_crowbar");
}

//----------------------------------------------------------------------------------------------------------------
CBotBorzh::~CBotBorzh()
{
    if ( IsUsingPlanner() )
    {
        CPlanner::Stop();
        CPlanner::Unlock(this);
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::Activated()
{
//#define BOT_KNOWS_EVERYTHING
    CBot::Activated();

    m_cAcceptedPlayers.resize( CPlayers::Size() );
    m_cAcceptedPlayers.reset();
    m_cBusyPlayers.resize( CPlayers::Size() );
    m_cBusyPlayers.reset();
    m_cCollaborativePlayers.resize( CPlayers::Size() );
    m_cCollaborativePlayers.reset();
    m_cWaitingPlayers.resize( CPlayers::Size() );
    m_cWaitingPlayers.reset();
    m_cCollaborativePlayers.set(m_iIndex); // Annotate self as collaborative player.
    m_cPlayersWithPhyscannon.resize( CPlayers::Size() );
    m_cPlayersWithPhyscannon.reset();
    m_cPlayersWithCrossbow.resize( CPlayers::Size() );
    m_cPlayersWithCrossbow.reset();

    m_aPlayersAreas.resize( m_cAcceptedPlayers.size() );
    memset(m_aPlayersAreas.data(), 0xFF, m_aPlayersAreas.size());

    m_aPlayersTasks.resize( m_cAcceptedPlayers.size() );
    memset(m_aPlayersTasks.data(), 0xFF, m_aPlayersTasks.size());

    // Initialize doors and buttons.
    m_cSeenDoors.resize( CItems::GetItems(EEntityTypeDoor).size() );
#ifdef BOT_KNOWS_EVERYTHING
    m_cSeenDoors.set();
#else
    m_cSeenDoors.reset();
#endif

    m_cSeenButtons.resize( CItems::GetItems(EEntityTypeButton).size() );
#ifdef BOT_KNOWS_EVERYTHING
    m_cSeenButtons.set();
#else
    m_cSeenButtons.reset();
#endif

    m_cPushedButtons.resize( CItems::GetItems(EEntityTypeButton).size() );
#ifdef BOT_KNOWS_EVERYTHING
    m_cPushedButtons.set();
#else
    m_cPushedButtons.reset();
#endif

    m_cOpenedDoors.resize( m_cSeenDoors.size() );
    m_cOpenedDoors.reset();

    m_cCheckedDoors.resize( m_cSeenButtons.size() );

    m_cFalseOpenedDoors.resize( m_cSeenDoors.size() );
    m_cFalseOpenedDoors.reset();

    m_cButtonTogglesDoor.resize( m_cSeenButtons.size()  );
    m_cButtonNoAffectDoor.resize( m_cSeenButtons.size() );
    m_cTestedToggles.resize( m_cSeenButtons.size() );
    for ( int i=0; i < m_cButtonTogglesDoor.size(); ++i )
    {
        m_cButtonTogglesDoor[i].resize( m_cSeenDoors.size() );
        m_cButtonTogglesDoor[i].reset();

        m_cButtonNoAffectDoor[i].resize( m_cSeenDoors.size() );
        m_cButtonNoAffectDoor[i].reset();

        m_cTestedToggles[i].resize( m_cSeenDoors.size() );
        m_cTestedToggles[i].reset();
    }

#ifdef BOT_KNOWS_EVERYTHING
    m_cButtonTogglesDoor[0].set(0);
    m_cButtonTogglesDoor[0].set(5);
    m_cButtonTogglesDoor[1].set(3);
    m_cButtonTogglesDoor[1].set(1);
    m_cButtonTogglesDoor[2].set(6);
    m_cButtonTogglesDoor[3].set(7);
    m_cButtonTogglesDoor[3].set(2);
    m_cButtonTogglesDoor[4].set(8);
    m_cButtonTogglesDoor[4].set(0);
    m_cButtonTogglesDoor[5].set(9);
    m_cButtonTogglesDoor[5].set(6);
    m_cButtonTogglesDoor[6].set(7);
    m_cButtonTogglesDoor[6].set(0);
    m_cButtonTogglesDoor[7].set(1);
    m_cButtonTogglesDoor[7].set(4);
    m_cButtonTogglesDoor[8].set(5);
    m_cButtonTogglesDoor[8].set(7);
    m_cButtonTogglesDoor[9].set(0);
    m_cButtonTogglesDoor[9].set(9);

    for ( int iButton=0; iButton < m_cSeenButtons.size(); ++iButton )
        for ( int iDoor=0; iDoor < m_cSeenDoors.size(); ++iDoor )
            if ( !m_cButtonTogglesDoor[iButton].test(iDoor) )
                m_cButtonNoAffectDoor[iButton].set(iDoor);

    m_aBoxes.push_back( CBoxInfo( 0, 99, 1) );
#endif

    m_aVisitedWaypoints.resize( CWaypoints::Size() );
#ifdef BOT_KNOWS_EVERYTHING
    m_aVisitedWaypoints.set();
#else
    m_aVisitedWaypoints.reset();
#endif

    const StringVector& aAreas = CWaypoints::GetAreas();
    m_cVisitedAreas.resize( aAreas.size() );
#ifdef BOT_KNOWS_EVERYTHING
    m_cVisitedAreas.set();
#else
    m_cVisitedAreas.reset();
#endif

    m_cSeenWalls.resize( aAreas.size() );
#ifdef BOT_KNOWS_EVERYTHING
    m_cSeenWalls.set();
#else
    m_cSeenWalls.reset();
#endif

    m_cVisitedAreasAfterPushButton.resize( aAreas.size() );

    m_cReachableAreas.resize( aAreas.size() );

    m_iCrossbow = CWeapons::GetIdFromWeaponName("weapon_crossbow");
    BASSERT( m_iCrossbow != EWeaponIdInvalid );

    m_bSaidHello = false;
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::Respawned()
{
    CBot::Respawned();

    m_bDontAttack = m_bDontBreakObjects = m_bDontThrowObjects = true;
    m_bNothingToDo = m_bCarryingBox = false;
    m_cCurrentTask.iTask = EBorzhTaskInvalid;

    if ( iCurrentWaypoint != -1 )
        m_aVisitedWaypoints.set(iCurrentWaypoint);

    TAreaId iCurrentArea = (iCurrentWaypoint == EWaypointIdInvalid ) ? EAreaIdInvalid : CWaypoints::Get(iCurrentWaypoint).iAreaId;
    m_aPlayersAreas[m_iIndex] = iCurrentArea;

    m_bUsedPlannerForGoal = m_bUsedPlannerForBox = m_bUsedPlannerForButton = false;
    m_bWasMovingBeforePause = false;
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow )
{
    m_cChat.iBotChat = EBotChatDontHurt;
    m_cChat.iDirectedTo = iPlayerIndex;
    m_cChat.cMap.clear();

    CBot::Speak(false);
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::ReceiveChat( int iPlayerIndex, CPlayer* pPlayer, bool bTeamOnly, const char* szText )
{
    good::string sText(szText, true, true);
    sText.lower_case();

    if ( sText == "start" )
    {
        if ( !m_bStarted )
        {
            m_bNeedMove = m_bWasMovingBeforePause;
            m_bStarted = true;
        }
    }
    else if ( sText == "pause" )
    {
        if ( m_bStarted )
        {
            m_bWasMovingBeforePause = m_bNeedMove;
            m_bStarted = m_bNeedMove = false;
        }
    }
    else if ( sText == "reset" )
    {
        m_bNothingToDo = false;
    }
    else if ( sText == "restart" )
    {
        m_bNothingToDo = false;
        CancelTasksInStack();
        BigTaskFinish();
    }
    else if ( sText == "print-buttons" )
    {
        for (TEntityIndex iButton = 0; iButton < m_cSeenButtons.size(); ++iButton)
        {
            for (TEntityIndex iDoor = 0; iDoor < m_cSeenDoors.size(); ++iDoor)
            {
                if ( m_cButtonTogglesDoor[iButton].test(iDoor) )
                {
                    BotMessage("%s -> button %d - door %d, opens.", GetName(), iButton+1, iDoor+1);
                }
                else if ( m_cButtonNoAffectDoor[iButton].test(iDoor) )
                {
                    BotMessage("%s -> button %d - door %d, doesn't affect.", GetName(), iButton+1, iDoor+1);
                }
            }
        }
    }
    else if ( m_bStarted )
    {
        SwitchToSpeakTask(EBotChatError);
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::ReceiveChatRequest( const CBotChat& cRequest )
{
    // TODO: synchronize chat, receive in buffer at the Think().
    TChatVariableValue iVars[CModBorzh::iTotalVars];
    memset(&iVars, 0xFF, CModBorzh::iTotalVars * sizeof(TChatVariableValue));

#define AREA        iVars[CModBorzh::iVarArea]
#define BUTTON      iVars[CModBorzh::iVarButton]
#define BOX         iVars[CModBorzh::iVarBox]
#define DOOR        iVars[CModBorzh::iVarDoor]
#define DOOR_STATUS iVars[CModBorzh::iVarDoorStatus]
#define WEAPON      iVars[CModBorzh::iVarWeapon]

    // Get needed chat variables.
    for ( int i=0; i < cRequest.cMap.size(); ++i )
    {
        const CChatVarValue& cVarValue = cRequest.cMap[i];
        iVars[cVarValue.iVar] = cVarValue.iValue;
    }

    // Respond to a request/chat. TODO: check arguments.
    switch (cRequest.iBotChat)
    {
        case EBotChatGreeting:
            m_cCollaborativePlayers.set(cRequest.iSpeaker);
            break;

        case EBotChatAffirmative:
        case EBotChatAffirm:
        case EBorzhChatOk:
            if ( IsCollaborativeTask() && IS_ASKED_FOR_HELP() && !IS_ACCEPTED() && m_cCollaborativePlayers.test(cRequest.iSpeaker) )
            {
                m_aPlayersTasks[cRequest.iSpeaker] = m_cCurrentBigTask.iTask;
                m_cAcceptedPlayers.set(cRequest.iSpeaker);
                CheckAcceptedPlayersForCollaborativeTask();
            }
            break;

        case EBorzhChatDone:
            if ( IsUsingPlanner() && IS_ACCEPTED() )
            {
                if ( GetPlanStepPerformer() == cRequest.iSpeaker )
                {
                    if ( m_cCurrentTask.iTask == EBorzhTaskWaitPlayer )
                        m_cCurrentTask.iTask = EBorzhTaskInvalid;
                    else
                        SET_PLAYER_DONE();
                    // TODO: sacar done.
                }
            }
            break;

        case EBorzhChatNoMoves:
        //case EBorzhChatFinishExplore:
            m_aPlayersTasks[cRequest.iSpeaker] = EBorzhTaskInvalid;
            m_cBusyPlayers.reset(cRequest.iSpeaker);
            if ( m_bNothingToDo && m_cBusyPlayers.none() )
                m_bNothingToDo = false;
            break;

        case EBotChatBusy:
        case EBorzhChatWait:
        case EBotChatStop:
        case EBotChatNegative:
        case EBotChatNegate:
            if ( IsCollaborativeTask() && m_cCollaborativePlayers.test(cRequest.iSpeaker) )
                UnexpectedChatForCollaborativeTask(cRequest.iSpeaker, true);
            break;

        case EBorzhChatExplore:
        case EBorzhChatExploreNew:
            m_aPlayersTasks[cRequest.iSpeaker] = EBorzhTaskExplore;
            if ( IsUsingPlanner() && m_cCollaborativePlayers.test(cRequest.iSpeaker) )
                UnexpectedChatForCollaborativeTask(cRequest.iSpeaker, true);
            break;

        case EBorzhChatNewArea:
            if ( AREA == 0xFF || AREA == -1 )
            {
                SwitchToSpeakTask(EBotChatError);
                break;
            }
            m_cVisitedAreas.set(AREA);
            m_aPlayersAreas[cRequest.iSpeaker] = AREA;
            if ( IsUsingPlanner() && m_cCollaborativePlayers.test(cRequest.iSpeaker) )
                UnexpectedChatForCollaborativeTask(cRequest.iSpeaker, true); // Other player is investigating area, wait for him.
            break;

        case EBorzhChatChangeArea:
            if ( AREA == 0xFF || AREA == -1 )
            {
                SwitchToSpeakTask(EBotChatError);
                break;
            }
            // TODO: check planner here instead of "done".
            // Check if goal of the plan is reached.
            m_cVisitedAreas.set(AREA);
            m_aPlayersAreas[cRequest.iSpeaker] = AREA;
            CheckGoalReached(cRequest.iSpeaker);
            break;

        case EBorzhChatWallFound:
            m_cSeenWalls.set( m_aPlayersAreas[cRequest.iSpeaker] );
            break;

        case EBorzhChatWeaponFound:
        {
            bool bIsCrossbow = (WEAPON == CModBorzh::iVarValueWeaponCrossbow);
            BASSERT( bIsCrossbow || (WEAPON == CModBorzh::iVarValueWeaponPhyscannon) );
            if ( bIsCrossbow )
                m_cPlayersWithCrossbow.set(cRequest.iSpeaker);
            else
                m_cPlayersWithPhyscannon.set(cRequest.iSpeaker);
            break;
        }

        case EBorzhChatDoorFound:
        case EBorzhChatDoorChange:
        case EBorzhChatDoorNoChange:
            if ( DOOR == 0xFF || DOOR == -1 )
                SwitchToSpeakTask(EBotChatError);
            else
            {
                bool bOpened;
                if ( DOOR_STATUS == EChatVariableValueInvalid )
                    bOpened = m_cOpenedDoors.test(DOOR);
                else
                    bOpened = (DOOR_STATUS == CModBorzh::iVarValueDoorStatusOpened);
                DoorStatusCheck(DOOR, bOpened, false, true);

                if ( (rand() & 3) == 0 )
                    SwitchToSpeakTask(EBorzhChatOk);
            }
            break;

        case EBorzhChatSeeButton:
            if ( BUTTON == 0xFF || BUTTON == -1  )
                SwitchToSpeakTask(EBotChatError);
            else
            {
                m_cSeenButtons.set(BUTTON);
                if ( (rand() & 3) == 0 )
                    SwitchToSpeakTask(EBorzhChatOk);
            }
            break;

        case EBorzhChatButtonCanPush:
        case EBorzhChatButtonCantPush:
        case EBorzhChatButtonWeapon:
        case EBorzhChatButtonNoWeapon:
        case EBorzhChatDoorTry:
            break;

        case EBorzhChatBoxFound:
        case EBorzhChatBoxIDrop:
            if ( BOX == 0xFF || BOX == -1  )
                SwitchToSpeakTask(EBotChatError);
            else
            {
                const good::vector<CBoxInfo>& aBoxes = CModBorzh::GetBoxes();
                good::vector<CBoxInfo>::const_iterator it = good::find(aBoxes, BOX);
                BASSERT( (it != aBoxes.end()) && (m_aPlayersAreas[cRequest.iSpeaker] == it->iArea) );
                m_aBoxes.push_back(*it);
            }
            break;

        case EBorzhChatBoxLost:
        case EBorzhChatBoxITake:
            if ( BOX == 0xFF || BOX == -1  )
                SwitchToSpeakTask(EBotChatError);
            else
            {
                good::vector<CBoxInfo>::iterator it = good::find(m_aBoxes, BOX);
                if ( it != m_aBoxes.end() )
                {
                    BASSERT( it->iArea == m_aPlayersAreas[cRequest.iSpeaker] );
                    m_aBoxes.erase(it);
                }
            }
            break;

        case EBorzhChatHaveGravityGun:
            m_cPlayersWithPhyscannon.set(cRequest.iSpeaker);
            break;

        case EBorzhChatNeedGravityGun:
            m_cPlayersWithPhyscannon.reset(cRequest.iSpeaker);
            break;

        case EBorzhChatButtonTry:
        case EBorzhChatButtonTryGo:
        case EBorzhChatButtonDoor:
            if ( BUTTON == 0xFF || BUTTON == -1  )
                SwitchToSpeakTask(EBotChatError);
            else
                // Other bot has plan to go to button or to go to button&door.
                //m_aPlayersTasks[cRequest.iSpeaker] = EBorzhTaskButtonDoorConfig;
                ReceiveTaskOffer(EBorzhTaskButtonTryDoor, BUTTON, DOOR, cRequest.iSpeaker);
            break;

        case EBorzhChatBoxTry:
            if ( AREA == 0xFF || AREA == -1  )
                SwitchToSpeakTask(EBotChatError);
            else
                ReceiveTaskOffer(EBorzhTaskBringBox, AREA, -1, cRequest.iSpeaker);
            break;

        case EBorzhChatFoundPlan:
            ReceiveTaskOffer(EBorzhTaskGoToGoal, -1, -1, cRequest.iSpeaker);
            break;

        case EBorzhChatButtonToggles:
        case EBorzhChatButtonNoToggles:
            if ( BUTTON == 0xFF || BUTTON == -1  || DOOR == 0xFF || DOOR == -1  )
            {
                SwitchToSpeakTask(EBotChatError);
                break;
            }
            if ( cRequest.iBotChat == EBorzhChatButtonToggles )
            {
                m_cButtonTogglesDoor[BUTTON].set(DOOR);
                m_cButtonNoAffectDoor[BUTTON].reset(DOOR);
            }
            else
            {
                m_cButtonTogglesDoor[BUTTON].reset(DOOR);
                m_cButtonNoAffectDoor[BUTTON].set(DOOR);
            }

            m_cCheckedDoors.set(DOOR);

            /*if ( IsUsingPlanner() && m_cCollaborativePlayers.test(cRequest.iSpeaker) )
                UnexpectedChatForCollaborativeTask(cRequest.iSpeaker, false);
            // No need for this.
            else */if ( (m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor) &&
                      (GET_BUTTON(m_cCurrentBigTask.iArgument) == BUTTON) &&
                      (GET_DOOR(m_cCurrentBigTask.iArgument) == DOOR) )
            {
                CancelTasksInStack();
                BigTaskFinish();
                PushSpeakTask( (rand()&1) ? EBorzhChatOk : EBotChatAffirmative );
            }
            break;

        case EBorzhChatButtonIPush:
        case EBorzhChatButtonIShoot:
            if ( BUTTON == 0xFF || BUTTON == -1  )
            {
                SwitchToSpeakTask(EBotChatError);
                break;
            }
            if ( m_cCurrentBigTask.iTask == EBorzhTaskInvalid )
            {
                // Start new task: try button.
                m_cCurrentBigTask.iTask = EBorzhTaskButtonTryDoor;
                m_cCurrentBigTask.iArgument = 0;
                SET_BUTTON(BUTTON, m_cCurrentBigTask.iArgument);
                SET_DOOR(0xFF, m_cCurrentBigTask.iArgument);
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWait, m_iTimeAfterPushingButton) ); // Wait time after speak, so another bot can press button.
                m_bNothingToDo = false; // Wake up bot, if it has nothing to do.
            }
            //else if ( (m_cCurrentBigTask.iTask != EBorzhTaskButtonDoorConfigHelp) && (m_cCurrentBigTask.iTask != EBorzhTaskButtonDoorConfigHelp) )
            //{
            //	SwitchToSpeakTask(EBorzhChatWait); // Tell player to wait, so bot can finish his business first.
            //	m_cWaitingPlayers.set(cRequest.iSpeaker);
            //}

            ButtonPushed(BUTTON);

            // TODO: check planner here instead of "done".
            if ( IsUsingPlanner() )
            {
                if ( IS_ACCEPTED() && (GetPlanStepPerformer() == cRequest.iSpeaker) ) // TODO: one method.
                {
                    const CPlanner::CPlan* pPlan = CPlanner::GetPlan();
                    CAction cAction = pPlan->at(m_iPlanStep-1);
                    if ( cAction.iArgument != BUTTON || (cAction.iAction != EBotActionPushButton && cAction.iAction != EBotActionShootButton) )
                        CancelCollaborativeTask(); // Someone is not following orders.
                }
                else
                    CancelCollaborativeTask();
            }
            else if ( (m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor) && (GET_BUTTON(m_cCurrentBigTask.iArgument) == BUTTON) )
            {
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWait, m_iTimeAfterPushingButton) ); // Wait time after speak, so another bot can press button.
            }
            break;

        case EBorzhChatButtonYouPush:
        case EBorzhChatButtonYouShoot:
            if ( BUTTON == 0xFF || BUTTON == -1  )
                SwitchToSpeakTask(EBotChatError);
            else if ( cRequest.iDirectedTo == m_iIndex )
            {
                if ( IsCollaborativeTask() && !IS_PUSHED() && (cRequest.iSpeaker == GET_PLAYER(m_cCurrentBigTask.iArgument)) )
                {
                    SET_FOLLOWING_ORDERS();
                    CancelTasksInStack();
                    PushSpeakTask(EBorzhChatDone, MAKE_PLAYER(cRequest.iSpeaker));
                    PushPressButtonTask(BUTTON);
                }
                else
                    ReceiveTaskOffer(EBorzhTaskButtonTryDoor, BUTTON, 0xFF, cRequest.iSpeaker);
            }
            break;

        case EBorzhChatBoxYouTake:
            if ( BOX == 0xFF || BOX == -1  )
                SwitchToSpeakTask(EBotChatError);
            else if ( cRequest.iDirectedTo == m_iIndex )
            {
                if ( IsCollaborativeTask() && !IS_PUSHED() && (cRequest.iSpeaker == GET_PLAYER(m_cCurrentBigTask.iArgument)) )
                {
                    SET_FOLLOWING_ORDERS();
                    CancelTasksInStack();
                    PushSpeakTask(EBorzhChatDone, MAKE_PLAYER(cRequest.iSpeaker));

                    PerformActionCarryBox(BOX);
                }
                else
                    SwitchToSpeakTask(EBotChatBusy, MAKE_PLAYER(cRequest.iSpeaker));
            }
            break;

        case EBorzhChatBoxYouDrop:
            if ( BOX == 0xFF || BOX == -1  )
                SwitchToSpeakTask(EBotChatError);
            else if ( cRequest.iDirectedTo == m_iIndex )
            {
                if ( IsCollaborativeTask() && !IS_PUSHED() && (cRequest.iSpeaker == GET_PLAYER(m_cCurrentBigTask.iArgument)) )
                {
                    SET_FOLLOWING_ORDERS();
                    CancelTasksInStack();
                    PushSpeakTask(EBorzhChatDone, MAKE_PLAYER(cRequest.iSpeaker));

                    PerformActionDropBox(BOX);
                }
                else
                    SwitchToSpeakTask(EBotChatBusy, MAKE_PLAYER(cRequest.iSpeaker));
            }
            break;

        case EBorzhChatAreaGo:
            if ( AREA == 0xFF || AREA == -1  )
                SwitchToSpeakTask(EBotChatError);
            else if ( cRequest.iDirectedTo == m_iIndex )
            {
                if ( IsCollaborativeTask() && !IS_PUSHED() && (cRequest.iSpeaker == GET_PLAYER(m_cCurrentBigTask.iArgument)) )
                {
                    SET_FOLLOWING_ORDERS();
                    CancelTasksInStack();
                    PushSpeakTask(EBorzhChatDone, MAKE_PLAYER(cRequest.iSpeaker));
                    TWaypointId iWaypoint = CModBorzh::GetRandomAreaWaypoint(AREA);
                    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, iWaypoint) );
                }
                else
                    SwitchToSpeakTask(EBotChatBusy, MAKE_PLAYER(cRequest.iSpeaker));
            }
            break;

        case EBorzhChatDoorGo:
            BASSERT(false);
            if ( DOOR == 0xFF || DOOR == -1  )
                SwitchToSpeakTask(EBotChatError);
            else if ( cRequest.iDirectedTo == m_iIndex )
            {
                if ( IsCollaborativeTask() && !IS_PUSHED() && (cRequest.iSpeaker == GET_PLAYER(m_cCurrentBigTask.iArgument)) )
                {
                    SET_FOLLOWING_ORDERS();
                    CancelTasksInStack();
                    TWaypointId iWaypoint = GetDoorWaypoint(DOOR, m_cReachableAreas);
                    if ( iWaypoint == EWaypointIdInvalid )
                    {
                        // TODO: say that can't pass to door waypoint.
                        SwitchToSpeakTask(EBotChatBusy, MAKE_PLAYER(cRequest.iSpeaker));
                    }
                    else
                    {
                        PushSpeakTask(EBorzhChatDone, MAKE_PLAYER(cRequest.iSpeaker));
                        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, iWaypoint) );
                        //SwitchToSpeakTask(EBorzhChatOk, MAKE_PLAYER(cRequest.iSpeaker));
                    }
                }
                else
                    SwitchToSpeakTask(EBotChatBusy, MAKE_PLAYER(cRequest.iSpeaker));
            }
            break;

        case EBorzhChatButtonGo:
            BASSERT(false);
            break;

        case EBorzhChatAreaCantGo:
            if ( IsUsingPlanner() && m_cCollaborativePlayers.test(cRequest.iSpeaker) )
                CancelCollaborativeTask();
            break;

        case EBorzhChatTaskCancel:
            if ( IsCollaborativeTask() && m_cCollaborativePlayers.test(cRequest.iSpeaker) )
                CancelCollaborativeTask();
            break;
    }

    // Wait some time, emulating processing of message.
    Wait(m_iTimeAfterSpeak, true);
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::Think()
{
    if ( !m_bAlive )
    {
        m_cCmd.buttons = rand() & IN_ATTACK; // Force bot to respawn by hitting randomly attack button.
        return;
    }

    if ( !m_bStarted || ( m_bNothingToDo && (m_cCurrentTask.iTask == EBorzhTaskInvalid) && m_cTaskStack.empty() ) )
        return;

    if ( !m_bSaidHello )
    {
        m_bSaidHello = true;
        Start();
    }

    CheckCarryingBox();

    // Check if there are some new tasks.
    if ( m_cCurrentTask.iTask == EBorzhTaskInvalid )
        TaskPop();

    if ( !m_bNewTask && (m_cCurrentTask.iTask == EBorzhTaskInvalid) )
    {
        BASSERT( m_cTaskStack.size() == 0 );
        if ( !CheckBigTask() )
        {
            CheckForNewTasks();
            if ( m_cCurrentBigTask.iTask != EBorzhTaskInvalid )
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWait, 3000) ); // Wait 3 seconds before changing task.
        }
    }

    if ( m_bNewTask || m_bMoveFailure ) // Try again to go to waypoint in case of move failure.
        InitNewTask();

    BASSERT( !m_bNewTask );

    // Update current task.
    switch (m_cCurrentTask.iTask)
    {
        case EBorzhTaskWait:
            if ( CBotrixPlugin::fTime >= m_fEndWaitTime ) // Bot finished waiting.
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
            break;

        case EBorzhTaskLook:
            if ( !m_bNeedAim ) // Bot finished aiming.
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
            break;

        case EBorzhTaskMove:
            if ( !m_bLostBox && !m_bNeedMove ) // Bot finished moving.
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
            /*else if ( m_bStuck && (CBotrixPlugin::fTime >= m_fStuckCheckTime + 5.0f) )
            {
                m_bStuck = false;
                m_fStuckCheckTime = CBotrixPlugin::fTime + 1.0f;
            }*/
            break;

        case EBorzhTaskSpeak:
            m_cCurrentTask.iTask = EBorzhTaskInvalid;
            break;

        case EBorzhTaskWaitButton:
            BASSERT( m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor );
            if ( IS_PUSHED() ) // We already pushed button.
            {
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
                // TODO: EBorzhTaskCheckDoor -> CheckPathsAtCurrentWaypoint();
            }
            break;

        case EBorzhTaskWaitDoor:
            BASSERT( m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor );
            if ( IS_ENDED() ) // Door/button configuration is tested.
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
            break;

        case EBorzhTaskWaitPlanner:
            if ( !CPlanner::IsRunning() )
            {
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
                const CPlanner::CPlan* pPlan = CPlanner::GetPlan();
                // Check if has plan, but discard empty plans.
                if ( pPlan == NULL )
                {
                    BotMessage("%s -> planner finished without plan.", GetName());
                    CancelTasksInStack();
                    //CPlanner::Unlock(this);
                }
                else if ( pPlan->size() == 0 )
                {
                    BotMessage("%s -> planner finished with empty plan, wait for another bot to propose it.", GetName());
                    /*switch ( m_cCurrentBigTask.iTask )
                    {
                    case EBorzhTaskGoToGoal:
                        m_bUsedPlannerForGoal = true;
                        break;
                    case EBorzhTaskBringBox:
                        m_bUsedPlannerForBox = true;
                        break;
                    case EBorzhTaskButtonTryDoor:
                        m_bUsedPlannerForButton = true;
                        break;
                    default:
                        BASSERT(false);
                    }*/
                    BigTaskFinish();
                    m_bNothingToDo = true;
                    if ( m_cWaitingPlayers.any() )
                    {
                        PushSpeakTask(EBorzhChatNoMoves);
                        m_cWaitingPlayers.reset();
                    }
                }
                else
                {
                    BotMessage("%s -> planner finished with a non empty plan.", GetName());
                    m_iPlanStep = 0;
                    OfferCollaborativeTask();
                }
            }
            break;

        case EBorzhTaskWaitPlayer:
            if ( IS_PLAYER_DONE() )
            {
                CLEAR_PLAYER_DONE();
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
            }
            break;

        case EBorzhTaskCarryBox:
            if ( CBotrixPlugin::fTime >= m_fEndWaitTime )
            {
                m_iBox = m_cCurrentTask.iArgument;
                m_bCarryingBox = true;
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
            }
            break;
    }
}


//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::CurrentWaypointJustChanged()
{
    CBot::CurrentWaypointJustChanged();
    // Don't do processing of paths if bot is stucked.
    /*if ( m_bRepeatWaypointAction )
        return;*/

    m_aVisitedWaypoints.set(iCurrentWaypoint);

    CheckPathsAtCurrentWaypoint();
}

//----------------------------------------------------------------------------------------------------------------
bool CBotBorzh::DoWaypointAction()
{
    if ( m_bRepeatWaypointAction )
        return CBot::DoWaypointAction();

    const CWaypoint& cWaypoint = CWaypoints::Get(iCurrentWaypoint);

    // Check if bot enters new area.
    TAreaId iOldArea = m_aPlayersAreas[m_iIndex];
    if ( cWaypoint.iAreaId != iOldArea )
    {
        m_aPlayersAreas[m_iIndex] = cWaypoint.iAreaId;

        // Check if goal of the plan is reached.
        CheckGoalReached(m_iIndex);

        if ( m_cVisitedAreas.test(m_aPlayersAreas[m_iIndex]) )
            SwitchToSpeakTask(EBorzhChatChangeArea, MAKE_AREA(m_aPlayersAreas[m_iIndex]));
        else
            SwitchToSpeakTask(EBorzhChatNewArea, MAKE_AREA(m_aPlayersAreas[m_iIndex]));

        m_cVisitedAreasAfterPushButton.set(m_aPlayersAreas[m_iIndex]);
    }

    // Check if bot saw the button before.
    if ( FLAG_SOME_SET(FWaypointButton | FWaypointSeeButton, cWaypoint.iFlags) )
    {
        TEntityIndex iButton = CWaypoint::GetButton(cWaypoint.iArgument);
        if (iButton > 0)
        {
            --iButton;
            if ( !m_cSeenButtons.test(iButton) ) // Bot sees button for the first time.
            {
                m_cSeenButtons.set(iButton);
                if ( FLAG_SOME_SET(FWaypointButton, cWaypoint.iFlags) )
                    SwitchToSpeakTask( EBorzhChatSeeButton, MAKE_BUTTON(iButton), EEntityTypeButton, iButton );
                else
                {
                    SwitchToSpeakTask( m_cPlayersWithCrossbow.test(m_iIndex) ? EBorzhChatButtonWeapon : EBorzhChatButtonNoWeapon );
                    SwitchToSpeakTask( EBorzhChatSeeButton, MAKE_BUTTON(iButton), EEntityTypeButton, iButton );
                }
            }
        }
        else
            BotMessage("%s -> Error, waypoint %d has invalid button index.", GetName(), iCurrentWaypoint);
    }

#ifdef NEED_TO_RETURN_BOX
    CheckToReturnBox();
#endif

    CheckPathsAtCurrentWaypoint();
    return CBot::DoWaypointAction();
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::ApplyPathFlags()
{
    CBot::ApplyPathFlags();
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::DoPathAction()
{
    CBot::DoPathAction();
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PickItem( const CEntity& cItem, TEntityType iEntityType, TEntityIndex iIndex )
{
    CBot::PickItem( cItem, iEntityType, iIndex );
    if ( iEntityType == EEntityTypeWeapon )
    {
        TEntityIndex iIdx;
        if ( cItem.pItemClass->sClassName == "weapon_crossbow" )
        {
            m_cPlayersWithCrossbow.set(m_iIndex);
            iIdx = CModBorzh::iVarValueWeaponCrossbow;
        }
        else if ( cItem.pItemClass->sClassName == "weapon_physcannon" )
        {
            m_cPlayersWithPhyscannon.set(m_iIndex);
            iIdx = CModBorzh::iVarValueWeaponPhyscannon;
        }
        else
        {
            //BASSERT(false);
            return;
        }
        SwitchToSpeakTask(EBorzhChatWeaponFound, MAKE_WEAPON(iIdx));
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::SetReachableAreas( const good::bitset& cOpenedDoors )
{
    TAreaId iCurrentArea = m_aPlayersAreas[m_iIndex];

    m_cReachableAreas.reset();

    good::vector<TAreaId> cToVisit( CWaypoints::GetAreas().size() );
    cToVisit.push_back( iCurrentArea );

    const good::vector<CEntity>& cDoorEntities = CItems::GetItems(EEntityTypeDoor);
    while ( !cToVisit.empty() )
    {
        int iArea = cToVisit.back();
        cToVisit.pop_back();

        m_cReachableAreas.set(iArea);

        // Check doors.
        const good::vector<TEntityIndex>& cDoors = CModBorzh::GetDoorsForArea(iArea);
        for ( TEntityIndex i = 0; i < cDoors.size(); ++i )
        {
            const CEntity& cDoor = cDoorEntities[ cDoors[i] ];
            if ( m_cSeenDoors.test( cDoors[i] ) && cOpenedDoors.test( cDoors[i] ) ) // Seen and opened door.
            {
                TWaypointId iWaypoint1 = cDoor.iWaypoint;
                TWaypointId iWaypoint2 = (TWaypointId)cDoor.pArguments;
                if ( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2) )
                {
                    TAreaId iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
                    TAreaId iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
                    TAreaId iNewArea = ( iArea1 == iArea ) ? iArea2 : iArea1;
                    if ( !m_cReachableAreas.test(iNewArea) )
                        cToVisit.push_back(iNewArea);
                }
                else
                    BASSERT(false);
            }
        }

        // Check walls.
        const good::vector<CWall>& cWalls = CModBorzh::GetWallsForArea(iArea);
        if ( cWalls.size() > 0 )
        {
            good::vector<CBoxInfo>::iterator itBox = GetBoxInArea(iArea);
            if ( itBox != m_aBoxes.end() ) // We have a box in area, can be used to climb a wall.
            {
                for ( int iWall = 0; iWall < cWalls.size(); ++iWall )
                {
                    const CWall& cWall = cWalls[iWall];

                    TAreaId iNewArea = CWaypoints::Get(cWall.iHigherWaypoint).iAreaId;
                    if ( !m_cReachableAreas.test(iNewArea) )
                        cToVisit.push_back(iNewArea);
                }
            }
        }

        // Check falls.
        const good::vector<CWall>& cFalls = CModBorzh::GetFallsForArea(iArea);
        for ( int iFall = 0; iFall < cFalls.size(); ++iFall )
        {
            const CWall& cWall = cFalls[iFall];
            TAreaId iNewArea = CWaypoints::Get(cWall.iLowerWaypoint).iAreaId;
            if ( !m_cReachableAreas.test(iNewArea) )
                cToVisit.push_back(iNewArea);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
TWaypointId CBotBorzh::GetButtonWaypoint( TEntityIndex iButton, const good::bitset& cReachableAreas, bool bSameArea )
{
    const CEntity& cButton = CItems::GetItems(EEntityTypeButton)[ iButton ];
    TWaypointId iWaypoint = cButton.iWaypoint;
    if ( cButton.iWaypoint == EWaypointIdInvalid )
    {
        if ( m_cPlayersWithCrossbow.test(m_iIndex) )
            iWaypoint = CModBorzh::GetWaypointToShootButton(iButton);
        else
            return EWaypointIdInvalid;
    }
    BASSERT( CWaypoints::IsValid(iWaypoint) );

    TAreaId iArea = CWaypoints::Get(iWaypoint).iAreaId;

    bool bCanReach = bSameArea ? (iArea == m_aPlayersAreas[m_iIndex]) : cReachableAreas.test(iArea);
    return bCanReach ? iWaypoint : EWaypointIdInvalid;
}

//----------------------------------------------------------------------------------------------------------------
TWaypointId CBotBorzh::GetDoorWaypoint( TEntityIndex iDoor, const good::bitset& cReachableAreas )
{
    const good::vector<CEntity>& cDoorEntities = CItems::GetItems(EEntityTypeDoor);
    const CEntity& cDoor = cDoorEntities[ iDoor ];

    TWaypointId iWaypoint1 = cDoor.iWaypoint;
    TWaypointId iWaypoint2 = (TWaypointId)cDoor.pArguments;
    if ( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2) )
    {
        TAreaId iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
        TAreaId iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
        return cReachableAreas.test(iArea1) ? iWaypoint1 : (cReachableAreas.test(iArea2) ? iWaypoint2 : EWaypointIdInvalid);
    }
    else
        BASSERT(false);
    return false;
}

//----------------------------------------------------------------------------------------------------------------
TPlayerIndex CBotBorzh::GetPlanStepPerformer()
{
    BASSERT( IsUsingPlanner() );
    const CPlanner::CPlan* pPlan = CPlanner::GetPlan();
    BASSERT( pPlan );

    int iLastStep = m_iPlanStep - 1;

    if ( 0 <= iLastStep && iLastStep < pPlan->size() )
        return pPlan->at(iLastStep).iExecutioner;
    else
        return EPlayerIndexInvalid;
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::CheckPathsAtCurrentWaypoint()
{
    TAreaId iCurrentArea = CWaypoints::Get(iCurrentWaypoint).iAreaId;

    // Check neighbours of waypoint.
    const CWaypoints::WaypointNode& cNode = CWaypoints::GetNode(iCurrentWaypoint);
    const CWaypoints::WaypointNode::arcs_t& cNeighbours = cNode.neighbours;
    for ( int i=0; i < cNeighbours.size(); ++i)
    {
        const CWaypoints::WaypointArc& cArc = cNeighbours[i];
        const CWaypointPath& cPath = cArc.edge;

        TWaypointId iWaypoint = cArc.target;
        if ( (iWaypoint != iPrevWaypoint) && FLAG_SOME_SET(FPathDoor, cPath.iFlags) ) // Bot is not coming from door oath.
        {
            // Waypoint path is passing through a door.
            TEntityIndex iDoor = cPath.iArgument;
            if ( iDoor > 0 )
            {
                --iDoor;

                bool bOpened = CItems::IsDoorOpened(iDoor);
                bool bNeedToPassThrough = (iWaypoint == m_iAfterNextWaypoint) && m_bNeedMove && m_bUseNavigatorToMove;
                DoorStatusCheck(iDoor, bOpened, bNeedToPassThrough, false);
            }
            else
                BotMessage("%s -> Error, waypoint path from %d to %d has invalid door index.", GetName(), iCurrentWaypoint, iWaypoint);
        }
        else if ( FLAG_SOME_SET(FPathTotem, cPath.iFlags) ) // Waypoint path needs to make a totem.
        {
            if ( !m_cSeenWalls.test(iCurrentArea) )
            {
                m_cSeenWalls.set(iCurrentArea);
                SwitchToSpeakTask(EBorzhChatBoxNeed);
                PushSpeakTask(EBorzhChatWallFound, 0, EEntityTypeTotal, iWaypoint);
            }

            // Check if need to bring box.
            if ( (iWaypoint == m_iAfterNextWaypoint) && m_bNeedMove && m_bUseNavigatorToMove )
            {
                // Check if need to climb a wall to pass to new area.
                CWall cWall(iCurrentWaypoint, iWaypoint);

                if ( !CModBorzh::IsWallClimbable(cWall) ) // We need to climb a wall, but there is no box near.
                {
                    // Search for a box and put it there.
                    good::vector<CBoxInfo>::iterator itBoxInArea = GetBoxInArea(iCurrentArea);
                    if ( itBoxInArea == m_aBoxes.end() ) // No box in current area.
                    {
                        CancelTasksInStack();
                        BigTaskFinish();

                        TAreaId iDestinationArea = CWaypoints::Get(iWaypoint).iAreaId;
                        PushSpeakTask(EBorzhChatAreaCantGo, MAKE_AREA(iDestinationArea));
                    }
                    else // There is a box in area.
                    {
                        SaveCurrentTask();
                        PushDropBoxTask(itBoxInArea->iBox, cWall.iLowerWaypoint);
                        PushGrabBoxTask(itBoxInArea->iBox, itBoxInArea->iWaypoint);
                    }
                }
            }
        }
    }
    CheckIfSeeBox();
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::DoorStatusCheck( TEntityIndex iDoor, bool bOpened, bool bNeedToPassThrough, bool bSpoken )
{
    bool bCheckingDoors = (m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor) && IS_PUSHED(); // We already pushed button.

    if ( !m_cSeenDoors.test(iDoor) ) // Bot sees door for the first time.
    {
        BASSERT( bSpoken || (m_cCurrentBigTask.iTask == EBorzhTaskExplore) ); // Should occur only when exploring new area.
        m_cSeenDoors.set(iDoor);
        m_cOpenedDoors.set(iDoor, bOpened);

        if ( !bSpoken )
        {
            TChatVariableValue iDoorStatus = bOpened ? CModBorzh::iVarValueDoorStatusOpened : CModBorzh::iVarValueDoorStatusClosed;
            SwitchToSpeakTask(EBorzhChatDoorFound, MAKE_DOOR(iDoor) | MAKE_DOOR_STATUS(iDoorStatus), EEntityTypeDoor, iDoor );
        }

        if ( bOpened )
            SetReachableAreas(m_cOpenedDoors);
    }
    else if ( !bOpened && bNeedToPassThrough ) // Bot needs to pass through the door, but door is closed.
        DoorClosedOnTheWay(iDoor, bCheckingDoors, bSpoken);
    else if ( m_cOpenedDoors.test(iDoor) != bOpened ) // Bot belief of opened/closed door is different.
        DoorStatusDifferent(iDoor, bOpened, bCheckingDoors, bSpoken);
    else
        DoorStatusSame(iDoor, bOpened, bCheckingDoors, bSpoken);

    // Cancel going to that door if another player already did it.
    if ( bSpoken && (m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor) && IS_PUSHED() )
    {
        if (GET_DOOR_GOTO(m_cCurrentBigTask.iArgument) == iDoor)
            CancelTasksInStack();
        if (GET_DOOR(m_cCurrentBigTask.iArgument) == iDoor)
        {
            CancelTasksInStack();
            BigTaskFinish();
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::DoorStatusSame( TEntityIndex iDoor, bool bOpened, bool bCheckingDoors, bool bSpoken )
{
    if ( bCheckingDoors && !m_cCheckedDoors.test(iDoor) ) // Checking doors but this one is not checked.
    {
        m_cCheckedDoors.set(iDoor);

        // Button that we are checking doesn't affect iDoor.
        TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
        if ( m_cButtonNoAffectDoor[iButton].test(iDoor) || m_cButtonTogglesDoor[iButton].test(iDoor) )
            return;


        SetButtonTogglesDoor(iButton, iDoor, false);
        if ( !bSpoken )
        {
            SwitchToSpeakTask(EBorzhChatButtonNoToggles, MAKE_BUTTON(iButton) | MAKE_DOOR(iDoor));

            // Say: door $door has not changed.
            TChatVariableValue iDoorStatus = bOpened ? CModBorzh::iVarValueDoorStatusOpened : CModBorzh::iVarValueDoorStatusClosed;
            SwitchToSpeakTask(EBorzhChatDoorNoChange, MAKE_DOOR(iDoor) | MAKE_DOOR_STATUS(iDoorStatus), EEntityTypeDoor, iDoor );
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::DoorStatusDifferent( TEntityIndex iDoor, bool bOpened, bool bCheckingDoors, bool bSpoken )
{
    m_cOpenedDoors.set(iDoor, bOpened);
    if ( bCheckingDoors )
    {
        BASSERT( !m_cCheckedDoors.test(iDoor) ); // Door should not be checked already.
        m_cCheckedDoors.set(iDoor);

        // Button that we are checking opens iDoor.
        TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
        SetButtonTogglesDoor(iButton, iDoor, true);
        if ( !bSpoken )
            SwitchToSpeakTask(EBorzhChatButtonToggles, MAKE_BUTTON(iButton) | MAKE_DOOR(iDoor));

        // If door is opened, don't check areas behind it, for now. Will check after finishing checking doors.
        if ( !bOpened )
        {
            // Bot was thinking that this door was opened. Recalculate reachable areas (closing this door).
            m_cFalseOpenedDoors.reset(iDoor);

            // Recalculate reachable areas using false opened doors.
            SetReachableAreas(m_cFalseOpenedDoors);
        }
    }
    if ( !bSpoken )
    {
        // Say: door $door is now $door_status.
        TChatVariableValue iDoorStatus = bOpened ? CModBorzh::iVarValueDoorStatusOpened : CModBorzh::iVarValueDoorStatusClosed;
        SwitchToSpeakTask(EBorzhChatDoorChange, MAKE_DOOR(iDoor) | MAKE_DOOR_STATUS(iDoorStatus), EEntityTypeDoor, iDoor);
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::DoorClosedOnTheWay( TEntityIndex iDoor, bool bCheckingDoors, bool bSpoken )
{
    BASSERT( !bSpoken );
    BASSERT( m_cOpenedDoors.test(iDoor) ); // Bot should think that this door was opened.
    m_cOpenedDoors.reset(iDoor); // Close the door.

    CancelTasksInStack();
    if ( bCheckingDoors )
    {
        m_cCheckedDoors.set(iDoor);

        TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
        SetButtonTogglesDoor(iButton, iDoor, true);
        PushSpeakTask(EBorzhChatButtonToggles, MAKE_BUTTON(iButton) | MAKE_DOOR(iDoor));

        m_cFalseOpenedDoors.reset(iDoor); // Close the door.
        // Recalculate reachable areas using false opened doors.
        SetReachableAreas(m_cFalseOpenedDoors);
    }
    else
    {
        BigTaskFinish();

        // Say: I can't reach area $area.
        TAreaId iArea = CWaypoints::Get( m_iDestinationWaypoint ).iAreaId;
        PushSpeakTask(EBorzhChatAreaCantGo, MAKE_AREA(iArea));

        // Recalculate reachable areas using opened doors.
        //SetReachableAreas(m_cOpenedDoors);
    }

    // Say: Door $door is closed now.
    PushSpeakTask(EBorzhChatDoorChange, MAKE_DOOR(iDoor) | MAKE_DOOR_STATUS(CModBorzh::iVarValueDoorStatusClosed), EEntityTypeDoor, iDoor);
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::Start()
{
    SetReachableAreas(m_cOpenedDoors);

    if ( !m_cVisitedAreas.test(m_aPlayersAreas[m_iIndex]) )
        PushSpeakTask(EBorzhChatNewArea, MAKE_AREA(m_aPlayersAreas[m_iIndex])); // Say: i am in new area.
    else
        PushSpeakTask(EBorzhChatChangeArea, MAKE_AREA(m_aPlayersAreas[m_iIndex])); // Say: i have changed area.

    // Say hello to some other player.
    for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
    {
        CPlayer* pPlayer = CPlayers::Get(iPlayer);
        if ( pPlayer && (m_iIndex != iPlayer) && (pPlayer->GetTeam() == GetTeam()) )
            PushSpeakTask(EBotChatGreeting, MAKE_PLAYER(iPlayer));
    }
    m_cCurrentTask.iTask = EBorzhTaskInvalid;
}

//----------------------------------------------------------------------------------------------------------------
bool CBotBorzh::CheckBigTask()
{
    bool bHasTask = false;
    BASSERT( (m_cCurrentTask.iTask == EBorzhTaskInvalid) && (m_cTaskStack.size() == 0) );
    switch ( m_cCurrentBigTask.iTask )
    {
        case EBorzhTaskExplore:
        {
            TAreaId iDestinationArea = m_cCurrentBigTask.iArgument;

            const good::vector<TWaypointId>& cWaypoints = CModBorzh::GetWaypointsForArea(iDestinationArea);
            BASSERT( cWaypoints.size() > 0 );

            TWaypointId iWaypoint = EWaypointIdInvalid;
            int iIndex = rand() % cWaypoints.size();

            // Search for some not visited waypoint in this area.
            for ( int i=iIndex; i >= 0; --i)
            {
                int iCurrent = cWaypoints[i];
                if ( (iCurrent != iCurrentWaypoint) && !m_aVisitedWaypoints.test(iCurrent) )
                {
                    iWaypoint = iCurrent;
                    break;
                }
            }
            if ( iWaypoint == EWaypointIdInvalid )
            {
                for ( int i=iIndex+1; i < cWaypoints.size(); ++i)
                {
                    int iCurrent = cWaypoints[i];
                    if ( (iCurrent != iCurrentWaypoint) && !m_aVisitedWaypoints.test(iCurrent) )
                    {
                        iWaypoint = iCurrent;
                        break;
                    }
                }
            }

            BASSERT( iWaypoint != iCurrentWaypoint );
            if ( iWaypoint == EWaypointIdInvalid )
            {
                m_cVisitedAreas.set(iDestinationArea);
                BigTaskFinish();
                PushSpeakTask(EBorzhChatFinishExplore, MAKE_AREA(iDestinationArea));
            }
            else // Start task to move to given waypoint.
            {
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, iWaypoint) );
            }
            bHasTask = true;
            break;
        }

        case EBorzhTaskGoToGoal:
            if ( IsUsingPlanner() )
            {
                if ( IS_ACCEPTED() ) // Already executing plan.
                    bHasTask = PlanStepNext();
                else
                {
                    BotMessage("%s -> No plan for EBorzhTaskGoToGoal.", GetName());
                    BigTaskFinish(); // Plan failed.
                }
            }
            else
            {
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitIndications) );
                CheckGoalReached(m_iIndex);
            }
            break;

        case EBorzhTaskBringBox:
            if ( IsUsingPlanner() )
            {
                if ( IS_ACCEPTED() ) // Already executing plan.
                    bHasTask = PlanStepNext();
                else
                {
                    TAreaId iArea = GET_AREA(m_cCurrentBigTask.iArgument)+1;
                    SET_AREA(iArea, m_cCurrentBigTask.iArgument);
                    bHasTask = CheckBoxForAreas(); // Plan failed, check other box configuration.
                }
            }
            else
            {
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitIndications) );
                CheckGoalReached(m_iIndex);
            }
            break;

        case EBorzhTaskButtonTryDoor:
            if ( IsUsingPlanner() )
            {
                if ( IS_ACCEPTED() ) // Already executing plan.
                    bHasTask = PlanStepNext();
                else
                    bHasTask = CheckButtonDoorConfigurations(); // Plan failed, check other button-door configuration.
            }
            else
            {
                if ( IS_PUSHED() )
                {
                    TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
                    const good::bitset& cButtonTogglesDoor = m_cButtonTogglesDoor[iButton];
                    const good::bitset& cButtonNoAffectDoor = m_cButtonNoAffectDoor[iButton];

                    if ( cButtonTogglesDoor.count() < 2)
                    {
                        const good::vector<CEntity>& aDoors = CItems::GetItems(EEntityTypeDoor);
                        for ( int iDoor = 0; iDoor < aDoors.size(); ++iDoor )
                        {
                            if ( !m_cCheckedDoors.test(iDoor) && !cButtonTogglesDoor.test(iDoor) && !cButtonNoAffectDoor.test(iDoor) )
                            {
                                TWaypointId iWaypoint = GetDoorWaypoint(iDoor, m_cReachableAreas);
                                if ( iWaypoint != EWaypointIdInvalid )
                                {
                                    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, iWaypoint) );
                                    SET_DOOR_GOTO(iDoor, m_cCurrentBigTask.iArgument); // Going to that door.
                                    CheckPathsAtCurrentWaypoint();
                                    return true;
                                }
                            }
                        }
                    }
                    else
                    {
                        BigTaskFinish();
                        bHasTask = false;
                        return false;
                    }

                    TEntityIndex iDoor = GET_DOOR(m_cCurrentBigTask.iArgument);
                    if ( (iDoor == 0xFF) || m_cCheckedDoors.test(iDoor) )
                    {
                        BigTaskFinish();
                        bHasTask = false;
                    }
                    else
                    {
                        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitDoor, iDoor) );
                        bHasTask = true;
                    }
                }
                else
                {
                    // Button is not pushed, push it or wait for commands/button push.
                    TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
                    if ( GetButtonWaypoint(iButton, m_cReachableAreas, true) == EWaypointIdInvalid )
                        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitButton, iButton) );
                    else
                        PushPressButtonTask(iButton);
                    bHasTask = true;
                }

                break;
        }

    }
    return bHasTask;
}

//----------------------------------------------------------------------------------------------------------------
bool CBotBorzh::CheckForNewTasks( TBorzhTask iProposedTask )
{
    BASSERT( (iProposedTask == EBorzhTaskInvalid) || (m_cCurrentBigTask.iTask == EBorzhTaskInvalid) );

    // Check if all bots can pass to goal area. Bot should have been at least in goal area.
    if ( iProposedTask >= EBorzhTaskGoToGoal )
        return true;

    int iGoalArea = CWaypoints::GetAreas().size() - 1;
    if ( (iProposedTask == EBorzhTaskInvalid) && m_cVisitedAreas.test(iGoalArea) && !m_bUsedPlannerForGoal && !CPlanner::IsLocked() )
    {
        m_cCurrentBigTask.iTask = EBorzhTaskGoToGoal;
        m_cCurrentBigTask.iArgument = 0;
        SET_USING_PLANNER();
        SET_PLAYER(m_iIndex, m_cCurrentBigTask.iArgument);

        StartPlannerForGoal();
        m_bUsedPlannerForGoal = true;
        return false;
    }

    // Check if there is some area to investigate.
    if ( iProposedTask >= EBorzhTaskExplore )
        return true;

    // Recalculate reachable areas.
    SetReachableAreas(m_cOpenedDoors);

    const StringVector& aAreas = CWaypoints::GetAreas();
    for ( TAreaId iArea = 0; iArea < aAreas.size(); ++iArea )
    {
        if ( !m_cVisitedAreas.test(iArea) && m_cReachableAreas.test(iArea) )
        {
            BotMessage( "%s -> CheckForNewTasks(): explore new area %s.", GetName(), CWaypoints::GetAreas()[iArea].c_str() );
            m_cCurrentBigTask.iTask = EBorzhTaskExplore;
            m_cCurrentBigTask.iArgument = iArea;
            SwitchToSpeakTask(EBorzhChatExploreNew, MAKE_AREA(m_aPlayersAreas[m_iIndex]));
            return false;
        }
    }

    // Check if there are some area to climb.
    if ( iProposedTask >= EBorzhTaskBringBox )
        return true;

    if ( !m_bUsedPlannerForBox && !CPlanner::IsLocked() )
    {
        m_cCurrentBigTask.iTask = EBorzhTaskBringBox;
        m_cCurrentBigTask.iArgument = 0; // Start with area 0.
        SET_USING_PLANNER();
        SET_PLAYER(m_iIndex, m_cCurrentBigTask.iArgument);
        CPlanner::Lock(this);
        if ( CheckBoxForAreas() )
            return false; // Don't accept proposed task, use EBorzhTaskBringBox.
    }

    // Check if there are some button to push.
    if ( iProposedTask >= EBorzhTaskButtonTryDoor )
        return true;

    const good::vector<CEntity>& aButtons = CItems::GetItems(EEntityTypeButton);
    const good::vector<CEntity>& aDoors = CItems::GetItems(EEntityTypeDoor);

    for ( TEntityIndex iButton = 0; iButton < aButtons.size(); ++iButton )
    {
        if ( m_cSeenButtons.test(iButton) && (GetButtonWaypoint(iButton, m_cReachableAreas) != EWaypointIdInvalid) ) // Can reach button.
        {
            TEntityIndex iDoor = EEntityIndexInvalid;

            int iToggleCount = m_cButtonTogglesDoor[iButton].count();
            int iNoToggleCount = m_cButtonNoAffectDoor[iButton].count();

            bool bTryButton = !m_cPushedButtons.test(iButton);

            // Try button if it opens less than 2 doors it is and there is a door it is not known to toggle.
            if ( bTryButton )
            {
                BotMessage( "%s -> CheckForNewTasks(): investigate button%d.", GetName(), iButton+1 );
            }
            else if ( (iToggleCount < 2) && (iToggleCount+iNoToggleCount < aDoors.size() ) )
            {
                /*
                m_cFalseOpenedDoors.reset();
                m_cFalseOpenedDoors |= m_cOpenedDoors; // Copy opened door to aux.

                // Emulate that button is pushed so some doors change state.
                for ( iDoor = 0; iDoor < aDoors.size(); ++iDoor )
                    if ( m_cButtonTogglesDoor[iButton].test(iDoor) )
                        m_cFalseOpenedDoors.set( iDoor, !m_cFalseOpenedDoors.test(iDoor) );

                m_cAuxReachableAreas.assign( m_cReachableAreas, true, true ); // Copy reachable areas to aux.
                SetReachableAreas(m_cFalseOpenedDoors);
                */

                for ( iDoor = 0; iDoor < aDoors.size(); ++iDoor )
                {
                    if ( !m_cButtonTogglesDoor[iButton].test(iDoor) && !m_cButtonNoAffectDoor[iButton].test(iDoor) )
                    {
                        const CEntity& cDoor = aDoors[iDoor];
                        for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
                        {
                            if ( (iPlayer != m_iIndex) && m_cCollaborativePlayers.test(iPlayer) && IsPlayerInDoorArea(iPlayer, cDoor) )
                            {
                                BotMessage( "%s -> CheckForNewTasks(): check if button%d toggles door%d (%s is there).",
                                            GetName(), iButton+1, iDoor+1, CPlayers::Get(iPlayer)->GetName() );
                                bTryButton = true;
                                break;
                            }
                        }
                        if ( bTryButton )
                            break;
                    }
                }
            }
            if ( bTryButton )
            {
                // We have at least one player to investigate door next to him.
                PushCheckButtonTask(iButton, iDoor);
                return false;
            }
        }
    }

    // Check if there are unknown button-door configuration to check (using planner).
    if ( iProposedTask == EBorzhTaskInvalid )
    {
        if ( !m_bUsedPlannerForButton && !CPlanner::IsLocked() )
        {
            m_cCurrentBigTask.iTask = EBorzhTaskButtonTryDoor;
            m_cCurrentBigTask.iArgument = 0; // Start with button 0 and door 0.
            SET_USING_PLANNER();
            SET_PLAYER(m_iIndex, m_cCurrentBigTask.iArgument);
            CPlanner::Lock(this);
            if ( CheckButtonDoorConfigurations() )
                return false; // Don't accept proposed task, use EBorzhTaskButtonDoorConfig.
        }

        // Restart tasks next time.
        m_bUsedPlannerForGoal = m_bUsedPlannerForBox = m_bUsedPlannerForButton = false;

        // Bot has nothing to do, wait for domain change.
        PushSpeakTask(EBorzhChatNoMoves);
        m_bNothingToDo = true;
        return false;
    }
    else
        return true; // Accept any task, we have nothing to do.
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::InitNewTask()
{
    //BotMessage( "%s -> New task %s, argument 0x%x.", GetName(), CTypeToString::BorzhTaskToString(m_cCurrentTask.iTask).c_str(), m_cCurrentTask.iArgument);

    m_bNewTask = false;

    switch ( m_cCurrentTask.iTask )
    {
        case EBorzhTaskWaitAnswer:
            CheckAcceptedPlayersForCollaborativeTask();
            break;

        case EBorzhTaskWait:
            m_fEndWaitTime = CBotrixPlugin::fTime + m_cCurrentTask.iArgument/1000.0f;
            break;

        case EBorzhTaskLook:
        {
            TEntityType iType = GET_TYPE(m_cCurrentTask.iArgument);
            TEntityIndex iIndex = GET_INDEX(m_cCurrentTask.iArgument);
            if ( iType == EEntityTypeTotal )
            {
                // Look at waypoint but don't look up or down.
                m_vLook = CWaypoints::Get(iIndex).vOrigin;
                m_vLook.z = m_vHead.z;
            }
            else
            {
                const CEntity& cEntity = CItems::GetItems(iType)[iIndex];
                m_vLook = cEntity.CurrentPosition();
            }
            m_bNeedMove = false;
            m_bNeedAim = true;
            m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();
            break;
        }

        case EBorzhTaskMove:
            if ( iCurrentWaypoint == m_cCurrentTask.iArgument )
            {
                m_bNeedMove = m_bUseNavigatorToMove = m_bDestinationChanged = false;
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
                m_bRepeatWaypointAction = false;
                CheckPathsAtCurrentWaypoint();
            }
            else
            {
                m_bNeedMove = m_bUseNavigatorToMove = true;
                if ( m_bMoveFailure || (m_iDestinationWaypoint != m_cCurrentTask.iArgument) )
                {
                    m_bMoveFailure = false;
                    m_iDestinationWaypoint = m_cCurrentTask.iArgument;
                    m_bDestinationChanged = true;
                }
            }
            break;

        case EBorzhTaskSpeak:
            m_bNeedMove = false;
            DoSpeakTask(m_cCurrentTask.iArgument);
            break;

        case EBorzhTaskPushButton:
            BotMessage("%s -> Will push button %d", GetName(), m_cCurrentTask.iArgument+1);
            FLAG_SET(IN_USE, m_cCmd.buttons);
            m_aLastPushedButtons.push_back(m_cCurrentTask.iArgument);
            ButtonPushed(m_cCurrentTask.iArgument);
            Wait(m_iTimeAfterPushingButton); // Wait 2 seconds after pushing button.
            break;

        case EBorzhTaskWeaponSet:
            BASSERT ( m_aWeapons[m_cCurrentTask.iArgument].IsPresent() );
            if ( m_iWeapon != m_cCurrentTask.iArgument )
            {
                BotMessage( "%s -> Will holster %s", GetName(), m_aWeapons[m_cCurrentTask.iArgument].GetName().c_str() );
                SetActiveWeapon( m_cCurrentTask.iArgument );
                Wait( m_aWeapons[m_cCurrentTask.iArgument].GetBaseWeapon()->fHolsterTime*1000 + 500 ); // Wait for holster.
            }
            else
                m_cCurrentTask.iTask = EBorzhTaskInvalid;
            break;

        case EBorzhTaskWeaponZoom:
        case EBorzhTaskWeaponRemoveZoom:
            BotMessage("%s -> Will toggle zoom, zooming: %s", GetName(), m_aWeapons[m_iCrossbow].IsUsingZoom() ? "yes" : "no");
            BASSERT( m_aWeapons[m_iCrossbow].IsUsingZoom() == (m_cCurrentTask.iTask == EBorzhTaskWeaponRemoveZoom) );
            ToggleZoom();
            Wait( m_aWeapons[m_iCrossbow].GetBaseWeapon()->fShotTime[1]*1000 + 500 ); // Wait for zoom.
            break;

        case EBorzhTaskWeaponShoot:
            BotMessage("%s -> Will shoot", GetName());
            Shoot();
            ButtonPushed(m_cCurrentTask.iArgument);
            Wait( m_aWeapons[m_iCrossbow].GetBaseWeapon()->fShotTime[0]*1000 + m_aWeapons[m_iCrossbow].GetBaseWeapon()->fReloadTime[0]*1000 + 500); // Wait for shoot + reload.
            break;

        case EBorzhTaskCarryBox:
            if ( !m_bCarryingBox )
            {
                m_bCarryingBox = true;
                m_bLostBox = false;
                BotMessage("%s -> Will carry box %d", GetName(), m_cCurrentTask.iArgument);
                Shoot(true);
                good::vector<CBoxInfo>::iterator itBox = good::find(m_aBoxes, m_cCurrentTask.iArgument);
                if ( itBox != m_aBoxes.end() ) // No box in current area.
                    m_aBoxes.erase(itBox);
                else
                    BASSERT(false);
            }
            m_fEndWaitTime = CBotrixPlugin::fTime + 1.0f;
            break;

        case EBorzhTaskDropBox:
            DropBox();
            Wait(1000);
            break;

    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PushSpeakTask( TBotChat iChat, int iArgument, TEntityType iType, TEntityIndex iIndex )
{
    // Speak.
    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskSpeak, MAKE_TYPE(iChat) | iArgument) );

    // Look at entity before speak (door, button, box, etc).
    if ( iType != EEntityTypeInvalid )
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskLook, MAKE_TYPE(iType) | MAKE_INDEX(iIndex)) );
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::StartPlannerForGoal()
{
    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlanner) );
    m_cCurrentTask.iTask = EBorzhTaskInvalid;

    BotMessage("%s -> StartPlannerForGoal(): starting planner.", GetName());
    CPlanner::Lock(this);
    CPlanner::Start(this);
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::CheckCarryingBox()
{
    if ( m_bCarryingBox && m_bNeedMove )
    {
        const CEntity& cBox = CItems::GetItems(EEntityTypeObject)[m_iBox];
        Vector vBoxPosition = cBox.CurrentPosition();
        float fDistanceSqr = m_vHead.DistToSqr( vBoxPosition );
        if ( fDistanceSqr > SQR(96.0f) ) // Box is too far, lost.
        {
            BotMessage("%s -> Lost box %d, will try to grab it again.", GetName(), m_iBox);
            m_bCarryingBox = m_bNeedMove = false; // Wait until box stops moving.
            m_bLostBox = true;
            m_vPrevBoxPosition = vBoxPosition;
        }
    }
    else if ( m_bLostBox )
    {
        const CEntity& cBox = CItems::GetItems(EEntityTypeObject)[m_iBox];
        Vector vBoxPosition = cBox.CurrentPosition();

        if ( m_vPrevBoxPosition == vBoxPosition ) // Box is stopped.
        {
            m_bLostBox = false;
            SaveCurrentTask();

            TWaypointId iBoxWaypoint = CWaypoints::GetNearestWaypoint( cBox.CurrentPosition() );
            TAreaId iBoxArea = CWaypoints::Get(iBoxWaypoint).iAreaId;
            if ( m_aPlayersAreas[m_iIndex] != iBoxArea ) // Box is in another area!
            {
                if ( !m_cReachableAreas.test(iBoxArea) )
                {
                    CancelCollaborativeTask();
                    PushSpeakTask(EBorzhChatBoxLost, MAKE_BOX(m_iBox));
                    return;
                }
            }
            PushGrabBoxTask(m_iBox, iBoxWaypoint, false);

            // Add box info, so bot will not speak about this box.
            CBoxInfo cBoxInfo(m_iBox, iBoxWaypoint, iBoxArea);
            BASSERT( good::find(m_aBoxes, cBoxInfo) == m_aBoxes.end() );
            m_aBoxes.push_back( cBoxInfo );
        }
        else
            m_vPrevBoxPosition = vBoxPosition;
    }
}

//----------------------------------------------------------------------------------------------------------------
bool CBotBorzh::CheckBoxForAreas()
{
    BASSERT( m_cCurrentBigTask.iTask == EBorzhTaskBringBox );

    // TODO: increment after doing all this.
    TAreaId iArea = GET_AREA(m_cCurrentBigTask.iArgument)+1;
    const StringVector& aAreas = CWaypoints::GetAreas();
    for ( ; iArea < aAreas.size(); ++iArea )
    {
        const good::vector<CWall>& aWalls = CModBorzh::GetWallsForArea(iArea);
        for ( int iWall = 0; iWall < aWalls.size(); ++iWall )
        {
            const CWall& cWall = aWalls[iWall];
            TAreaId iLowerArea = CWaypoints::Get(cWall.iLowerWaypoint).iAreaId;
            BASSERT( iArea == iLowerArea );
            TAreaId iHigherArea = CWaypoints::Get(cWall.iHigherWaypoint).iAreaId;
            if ( m_cVisitedAreas.test(iLowerArea) && !m_cVisitedAreas.test(iHigherArea) &&
                 (GetBoxInArea(iArea) == m_aBoxes.end()) ) // No box in this area.
            {
                BotMessage("%s -> CheckBoxForAreas(): trying to bring box to area %s.", GetName(), CWaypoints::GetAreas()[iArea].c_str());

                SET_AREA(iArea, m_cCurrentBigTask.iArgument);

                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlanner) );

                CPlanner::Start(this);
                return true;
            }
        }
    }

    BotMessage("%s -> CheckBoxForAreas(): no more box configurations.", GetName());
    BigTaskFinish();
    m_bUsedPlannerForBox = true;
    return false;
}

//----------------------------------------------------------------------------------------------------------------
bool CBotBorzh::CheckButtonDoorConfigurations()
{
    BASSERT( m_cCurrentTask.iTask == EBorzhTaskInvalid );

    TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
    TEntityIndex iDoor = GET_DOOR(m_cCurrentBigTask.iArgument);

    // TODO: increment after doing all this.
    // Increment previous button/door.
    if ( iDoor == 0xFF )
    {
        iButton++;
        iDoor = 0;
    }
    else
        iDoor++;

    const good::vector<CEntity>& aDoors = CItems::GetItems(EEntityTypeDoor);
    const good::vector<CEntity>& aButtons = CItems::GetItems(EEntityTypeButton);

    int iDoorsSize = aDoors.size();
    int iButtonsSize = aButtons.size();

    bool bFinished;
    do {
        bFinished = true;

        // Skip buttons that toggles 2 doors or the ones that toggle some door and not toggle the rest.
        while ( (iButton < iButtonsSize) &&
                ( !m_cSeenButtons.test(iButton) ||
                  (m_cButtonTogglesDoor[iButton].count() > 1) ||
                  (m_cButtonTogglesDoor[iButton].count() + m_cButtonNoAffectDoor[iButton].count() == iDoorsSize) ) )
        {
            iButton++;
            iDoor = 0;
        }
        if ( iButton == iButtonsSize )
            break;

        if ( !m_cPushedButtons.test(iButton) )
        {
            iDoor = EEntityIndexInvalid;
            break;
        }

        // Skip doors that are not discovered yet or button-door configuration is known. Skip tested configurations.
        while ( (iDoor < iDoorsSize) && (!m_cSeenDoors.test(iDoor) || m_cButtonTogglesDoor[iButton].test(iDoor) ||
                m_cButtonNoAffectDoor[iButton].test(iDoor) || m_cTestedToggles[iButton].test(iDoor)) )
            iDoor++;

        if ( iDoor == iDoorsSize )
        {
            iButton++;
            iDoor = 0;
            bFinished = (iButton == iButtonsSize);
        }
    } while ( !bFinished );

    if ( iButton == iButtonsSize ) // All tested.
    {
        BigTaskFinish();
        // Start again next time.
        for ( int i = 0; i < m_cTestedToggles.size(); ++i )
            m_cTestedToggles[i].reset();
        BotMessage("%s -> CheckButtonDoorConfigurations(): no more button-door configs to test. Reset next time.", GetName());
        m_bUsedPlannerForButton = true;
        return false;
    }

    SET_BUTTON(iButton, m_cCurrentBigTask.iArgument);
    SET_DOOR(iDoor, m_cCurrentBigTask.iArgument);

    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlanner) );
    m_cCurrentTask.iTask = EBorzhTaskInvalid;

    if ( iDoor != EEntityIndexInvalid )
        m_cTestedToggles[iButton].set(iDoor);
    BotMessage( "%s -> CheckButtonDoorConfigurations(): button%d - door%d (door0 = only button).", GetName(), iButton+1, iDoor+1 );
    BotMessage("%s -> CheckButtonDoorConfigurations(): starting planner.", GetName());

    CPlanner::Start(this);

    return true;
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::DoSpeakTask( int iArgument )
{
    m_cChat.iBotChat = GET_TYPE(iArgument);
    m_cChat.cMap.clear();
    m_cChat.iDirectedTo = GET_PLAYER(iArgument);

    m_cChat.cMap.push_back( CChatVarValue(CModBorzh::iVarDoor, 0, GET_DOOR(iArgument) ) );
    m_cChat.cMap.push_back( CChatVarValue(CModBorzh::iVarDoorStatus, 0, GET_DOOR_STATUS(iArgument)) );
    m_cChat.cMap.push_back( CChatVarValue(CModBorzh::iVarButton, 0, GET_BUTTON(iArgument) ) );
    m_cChat.cMap.push_back( CChatVarValue(CModBorzh::iVarBox, 0, GET_BOX(iArgument) ) );
    m_cChat.cMap.push_back( CChatVarValue(CModBorzh::iVarWeapon, 0, GET_WEAPON(iArgument)) );
    m_cChat.cMap.push_back( CChatVarValue(CModBorzh::iVarArea, 0, GET_AREA(iArgument)) );

    if ( (m_cChat.iBotChat == EBorzhChatButtonTry) || (m_cChat.iBotChat == EBorzhChatButtonDoor) ||
         (m_cChat.iBotChat == EBorzhChatBoxTry) || (m_cChat.iBotChat == EBorzhChatFoundPlan) )
        SET_ASKED_FOR_HELP();

    Speak(false);
    Wait(m_iTimeAfterSpeak);
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::BigTaskFinish()
{
    BotMessage("%s -> BigTaskFinish(): %s", GetName(), CTypeToString::BorzhTaskToString(m_cCurrentBigTask.iTask).c_str());
    CancelTasksInStack();
    if ( IsUsingPlanner() )
    {
        CPlanner::Stop();
        CPlanner::Unlock(this);
    }
    if ( (m_iWeapon == m_iCrossbow) && m_aWeapons[m_iWeapon].IsUsingZoom() && m_aWeapons[m_iWeapon].CanUse() )
        ToggleZoom();

    if ( m_bCarryingBox )
        DropBox();

    m_cCurrentBigTask.iTask = EBorzhTaskInvalid;
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PushPressButtonTask( TEntityIndex iButton )
{
    TWaypointId iWaypoint = CItems::GetItems(EEntityTypeButton)[iButton].iWaypoint;
    bool bShoot = (iWaypoint == EWaypointIdInvalid);

    // If it is shoot button, check that bot has crossbow.
    BASSERT( !bShoot || m_cPlayersWithCrossbow.test(m_iIndex) );

    // Push/shoot button.
    if ( bShoot )
    {
        // Remove zoom.
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWeaponRemoveZoom) );

        // Shoot.
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWait, 300) );
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWeaponShoot, iButton) );

        // Look at button again for better precision and wait.
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWait, 300) );
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskLook, MAKE_TYPE(EEntityTypeButton) | MAKE_INDEX(iButton)) );

        // Zoom.
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWait, 300) );
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWeaponZoom) );

        // Set crossbow weapon.
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWait, 300) );
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWeaponSet, m_iCrossbow) );
    }
    else
    {
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskPushButton, iButton) );

        // Look at button again for better precision and wait.
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWait, 500) );
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskLook, MAKE_TYPE(EEntityTypeButton) | MAKE_INDEX(iButton)) );
    }

    // Say: I will push/shoot button $button now.
    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskSpeak, MAKE_TYPE(bShoot ? EBorzhChatButtonIShoot : EBorzhChatButtonIPush) | MAKE_BUTTON(iButton)) );

    // Look at button.
    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskLook, MAKE_TYPE(EEntityTypeButton) | MAKE_INDEX(iButton)) );

    // Go to button waypoint.
    if ( bShoot )
        iWaypoint = CModBorzh::GetWaypointToShootButton(iButton);
    BASSERT( iWaypoint != EWaypointIdInvalid );

    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, iWaypoint) );
    m_cCurrentTask.iTask = EBorzhTaskInvalid;
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PushCheckButtonTask( TEntityIndex iButton, TEntityIndex iDoor )
{
    // Mark all doors as unchecked.
    m_cCheckedDoors.reset();

    m_cCurrentBigTask.iTask = EBorzhTaskButtonTryDoor;
    m_cCurrentBigTask.iArgument = 0;
    SET_BUTTON(iButton, m_cCurrentBigTask.iArgument);
    SET_DOOR(iDoor, m_cCurrentBigTask.iArgument);
    SET_PLAYER(m_iIndex, m_cCurrentBigTask.iArgument);

    //BASSERT( m_cCurrentProposedTask.iTask == EBorzhTaskInvalid );
    m_cCurrentProposedTask = m_cCurrentBigTask;

    PushPressButtonTask(iButton);
    OfferCollaborativeTask(iDoor);
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PerformActionCarryBox( TEntityIndex iBox )
{
    TAreaId iCurrentArea = m_aPlayersAreas[m_iIndex];
    const good::vector<CBoxInfo>& aBoxes = CModBorzh::GetBoxes();
    good::vector<CBoxInfo>::const_iterator it = good::find(aBoxes, iBox);
    BASSERT( it != aBoxes.end() );

    if ( good::find(m_aBoxes, *it) == m_aBoxes.end() )
        m_aBoxes.push_back(*it);

    TWaypointId iWaypoint = it->iWaypoint;
    if ( it->iArea != iCurrentArea ) // Box is in another area than we are.
    {
        const good::vector<CWall>& cFalls = CModBorzh::GetFallsForArea(iCurrentArea);
        good::vector<CWall>::const_iterator itFall;
        for ( itFall = cFalls.begin(); itFall != cFalls.end(); ++itFall )
            if ( it->iArea == CWaypoints::Get(itFall->iLowerWaypoint).iAreaId )
                break;

        BASSERT( itFall != cFalls.end() );
        iWaypoint = itFall->iHigherWaypoint;
    }
    PushGrabBoxTask(iBox, iWaypoint);
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PerformActionDropBox( TEntityIndex iBox )
{
    BASSERT( m_bCarryingBox );
    const good::vector<CWall>& cWalls = CModBorzh::GetWallsForArea(m_aPlayersAreas[m_iIndex]);
    TWaypointId iWaypoint = ( cWalls.size() > 0 ) ? cWalls[0].iLowerWaypoint : CModBorzh::GetRandomAreaWaypoint(m_aPlayersAreas[m_iIndex]);

    PushDropBoxTask(m_iBox, iWaypoint);
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PushGrabBoxTask( TEntityIndex iBox, TWaypointId iBoxWaypoint, bool bSpeak )
{
    BASSERT( m_aWeapons[m_iPhyscannon].IsPresent() && iBox != EEntityIndexInvalid );
    BASSERT( m_cCurrentTask.iTask == EBorzhTaskInvalid );
    BotMessage("%s -> PushGrabBoxTask(): box %d, to %d", GetName(), iBox, iBoxWaypoint);

    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskCarryBox, iBox) );

    if (bSpeak)
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskSpeak, MAKE_TYPE(EBorzhChatBoxITake) | MAKE_BOX(iBox)) );

    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskLook, MAKE_TYPE(EEntityTypeObject) | MAKE_INDEX(iBox)) );

    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWeaponSet, m_iPhyscannon) );

    if ( iBoxWaypoint != EWaypointIdInvalid )
        m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, iBoxWaypoint) );
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PushDropBoxTask( TEntityIndex iBox, TWaypointId iWhere )
{
    BASSERT( iBox != EEntityIndexInvalid && iWhere != EWaypointIdInvalid );
    BASSERT( m_cCurrentTask.iTask == EBorzhTaskInvalid );
    BotMessage("%s -> PushDropBoxTask(): box %d, to %d", GetName(), iBox, iWhere);

    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskDropBox, iBox) );

    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskSpeak, MAKE_TYPE(EBorzhChatBoxIDrop) | MAKE_BOX(iBox)) );

    const CWaypoints::WaypointNode& cNode = CWaypoints::GetNode(iWhere);
    const CWaypoints::WaypointNode::arcs_t& cNeighbours = cNode.neighbours;
    for ( int i=0; i < cNeighbours.size(); ++i)
    {
        const CWaypoints::WaypointArc& cArc = cNeighbours[i];
        TWaypointId iWaypoint = cArc.target;
        const CWaypointPath& cPath = cArc.edge;
        if ( FLAG_SOME_SET(FPathTotem, cPath.iFlags) ) // Waypoint path needs to make a totem.
            m_cTaskStack.push_back( CBorzhTask(EBorzhTaskLook, MAKE_TYPE(EEntityTypeTotal) | MAKE_INDEX(iWaypoint)) );
    }

    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, iWhere) );
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::DropBox()
{
    BASSERT( m_bCarryingBox && (m_iWeapon == m_iPhyscannon) );
    BotMessage("%s -> Will drop box %d", GetName(), m_cCurrentTask.iArgument);
    Shoot(true);
    //TWaypointId iBoxWaypoint = CWaypoints::GetNearestWaypoint( CItems::GetItems(EEntityTypeObject)[m_iBox].CurrentPosition() );
    //BASSERT( iBoxWaypoint != EWaypointIdInvalid );
    //m_aBoxes.push_back( CBoxInfo(m_iBox, iBoxWaypoint, CWaypoints::Get(iBoxWaypoint).iAreaId) );
    m_aBoxes.push_back( CBoxInfo(m_iBox, iCurrentWaypoint, CWaypoints::Get(iCurrentWaypoint).iAreaId) );

    m_iBox = EEntityIndexInvalid;
    m_bCarryingBox = false;
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::CheckAcceptedPlayersForCollaborativeTask()
{
    BASSERT( IsCollaborativeTask() );
    if ( m_cAcceptedPlayers.count() == m_cCollaborativePlayers.count() )
    {
        SET_ACCEPTED(); // Start executing plan or press button.
        m_cCurrentTask.iTask = EBorzhTaskInvalid;
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::OfferCollaborativeTask( int iArgument ) // TODO: OfferCollaborativeTask()
{
    BASSERT( IsCollaborativeTask() );

    m_cAcceptedPlayers.reset();
    m_cAcceptedPlayers.set(m_iIndex);

    SaveCurrentTask();

    // Wait for answer after speak.
    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitAnswer, m_iTimeToWaitPlayer) );

    // Speak.
    switch ( m_cCurrentBigTask.iTask )
    {
        case EBorzhTaskButtonTryDoor:
        {
            TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
            TEntityIndex iDoor = GET_DOOR(m_cCurrentBigTask.iArgument);

            if ( iDoor == 0xFF ) // Say: Let's try to find which doors toggles button $button.
                PushSpeakTask(EBorzhChatButtonTry, MAKE_BUTTON(iButton));
            else // Say: Let's try to check if button $button toggles door $door.
                PushSpeakTask(EBorzhChatButtonDoor, MAKE_BUTTON(iButton) | MAKE_DOOR(iDoor));
            break;
        }

        case EBorzhTaskBringBox:
            PushSpeakTask(EBorzhChatBoxTry, MAKE_AREA( GET_AREA(m_cCurrentBigTask.iArgument) ));
            break;

        case EBorzhTaskGoToGoal:
            PushSpeakTask(EBorzhChatFoundPlan); // TODO: say let's do it?
            break;

        default:
            BASSERT(false);
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::ReceiveTaskOffer( TBorzhTask iProposedTask, int iButtonOrArea, int iDoor, TPlayerIndex iSpeaker )
{
    BASSERT( iProposedTask == EBorzhTaskBringBox || iProposedTask == EBorzhTaskButtonTryDoor || iProposedTask == EBorzhTaskGoToGoal );

    if ( (iProposedTask == EBorzhTaskButtonTryDoor) && (iDoor != EEntityIndexInvalid) && (iDoor != 0xFF) )
    {
        // Check if bot knows if button affects door.
        if ( m_cButtonTogglesDoor[iButtonOrArea].test(iDoor) )
        {
            SwitchToSpeakTask( EBorzhChatButtonToggles, MAKE_BUTTON(iButtonOrArea) | MAKE_DOOR(iDoor) );
            return;
        }
        else if ( m_cButtonNoAffectDoor[iButtonOrArea].test(iDoor) )
        {
            SwitchToSpeakTask( EBorzhChatButtonNoToggles, MAKE_BUTTON(iButtonOrArea) | MAKE_DOOR(iDoor) );
            return;
        }
    }

    // Accept task if there is nothing to do or something to do with less priority.
    bool bAcceptTask = false, bNewTask = false;
    if ( m_cCurrentBigTask.iTask == EBorzhTaskInvalid )
    {
        bAcceptTask = CheckForNewTasks(iProposedTask);
        bNewTask = true;
    }
    else if ( m_cCurrentBigTask.iTask < iProposedTask ) // Proposed task has more priority.
        bAcceptTask = true;

    if ( bAcceptTask )
    {
        // Cancel current task.
        if ( IsUsingPlanner() )
        {
            BotMessage("%s -> ReceiveTaskOffer(): cancelling %s, button/area+1/%d - door%d", GetName(), CTypeToString::BorzhTaskToString(m_cCurrentBigTask.iTask).c_str(),
                       GET_BUTTON(m_cCurrentBigTask.iArgument)+1, GET_DOOR(m_cCurrentBigTask.iArgument)+1);
            CPlanner::Stop();
            CPlanner::Unlock(this);
        }
        CancelTasksInStack();
        m_bNothingToDo = false;

        m_cCurrentBigTask.iTask = iProposedTask;
        BotMessage( "%s -> Accepted task %s, button/area+1/%d, door %d. Previous task %s.", GetName(), CTypeToString::BorzhTaskToString(iProposedTask).c_str(),
                    iButtonOrArea+1, iDoor+1, CTypeToString::BorzhTaskToString(m_cCurrentBigTask.iTask).c_str() );

        switch ( iProposedTask )
        {
            case EBorzhTaskButtonTryDoor:
            {
                m_cCurrentBigTask.iArgument = 0;
                SET_BUTTON(iButtonOrArea, m_cCurrentBigTask.iArgument);
                SET_DOOR(iDoor, m_cCurrentBigTask.iArgument);
                SET_PLAYER(iSpeaker, m_cCurrentBigTask.iArgument);

                bool bInButtonArea = GetButtonWaypoint(iButtonOrArea, m_cReachableAreas, true) != EWaypointIdInvalid;
                bool bSomeoneInDoorArea;
                if ( (iDoor & 0xFF) == 0xFF )
                    bSomeoneInDoorArea = true;
                else
                {
                    bSomeoneInDoorArea = false;
                    const CEntity& cDoor = CItems::GetItems(EEntityTypeDoor)[iDoor];
                    TAreaId iDoorArea1 = CWaypoints::Get(cDoor.iWaypoint).iAreaId;
                    TAreaId iDoorArea2 = CWaypoints::Get((TWaypointId)cDoor.pArguments).iAreaId;

                    for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
                        if ( m_cCollaborativePlayers.test(iPlayer) && ( (m_aPlayersAreas[iPlayer] == iDoorArea1) || (m_aPlayersAreas[iPlayer] == iDoorArea2) ) )
                        {
                            bSomeoneInDoorArea = true;
                            break;
                        }
                }
                if ( bInButtonArea && bSomeoneInDoorArea )
                    PushPressButtonTask(iButtonOrArea);
                else
                    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitButton, iButtonOrArea) ); // Wait for button to be pushed or someone to indicate bot what to do.
                break;
            }

            case EBorzhTaskBringBox:
                m_cCurrentBigTask.iArgument = 0;
                SET_AREA(iButtonOrArea, m_cCurrentBigTask.iArgument);
                SET_PLAYER(iSpeaker, m_cCurrentBigTask.iArgument);

                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitIndications) );
                break;

            case EBorzhTaskGoToGoal:
                m_cCurrentBigTask.iArgument = 0;
                SET_PLAYER(iSpeaker, m_cCurrentBigTask.iArgument);

                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitIndications) );
                break;

            default:
                BASSERT(false);
        }

        // Say ok.
        PushSpeakTask( (rand()&1) ? EBorzhChatOk : EBotChatAffirmative );
    }
    else
    {
        BotMessage( "%s -> task %s rejected, current one is %s.", GetName(),
                    CTypeToString::BorzhTaskToString(iProposedTask).c_str(), CTypeToString::BorzhTaskToString(m_cCurrentBigTask.iTask).c_str() );

        if ( bNewTask )
            SwitchToSpeakTask(EBorzhChatBetterIdea, MAKE_PLAYER(iSpeaker));
        else // Say: wait.
        {
            m_cWaitingPlayers.set(iSpeaker);
            SwitchToSpeakTask((rand()&1) ? EBorzhChatWait : EBotChatBusy, MAKE_PLAYER(iSpeaker));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::ButtonPushed( TEntityIndex iButton )
{
    m_cCheckedDoors.reset();
    m_cPushedButtons.set(iButton);

    if ( (m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor) && (GET_BUTTON(m_cCurrentBigTask.iArgument) == iButton) )
    {
        BASSERT( !IS_PUSHED() );
        SET_PUSHED(); // Save that button was pushed.
    }

    m_cVisitedAreasAfterPushButton.reset();
    m_cVisitedAreasAfterPushButton.set(m_aPlayersAreas[m_iIndex]);

    const good::bitset& toggles = m_cButtonTogglesDoor[iButton];
    if ( toggles.any() )
    {
        for ( int iDoor = 0; iDoor < toggles.size(); ++iDoor )
            if ( toggles.test(iDoor) )
                m_cOpenedDoors.set( iDoor, !m_cOpenedDoors.test(iDoor) );
    }

    // Recalculate reachable areas.
    SetReachableAreas(m_cOpenedDoors);
}


//----------------------------------------------------------------------------------------------------------------
bool CBotBorzh::IsPlanPerformed()
{
    BASSERT( IsCollaborativeTask() );
    switch ( m_cCurrentBigTask.iTask )
    {
        case EBorzhTaskButtonTryDoor:
        {
            TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
            TEntityIndex iDoor = GET_DOOR(m_cCurrentBigTask.iArgument);

            TWaypointId iButtonWaypoint = CItems::GetItems(EEntityTypeButton)[iButton].iWaypoint;
            if ( iButtonWaypoint == EWaypointIdInvalid ) // Button is for shoot.
                iButtonWaypoint = CModBorzh::GetWaypointToShootButton(iButton);
            BASSERT( CWaypoints::IsValid(iButtonWaypoint) );

            TAreaId iButtonArea = CWaypoints::Get(iButtonWaypoint).iAreaId;
            TAreaId iDoorArea1 = EAreaIdInvalid;
            TAreaId iDoorArea2 = EAreaIdInvalid;
            if ( iDoor != 0xFF )
            {
                const CEntity& cDoor = CItems::GetItems(EEntityTypeDoor)[iDoor];
                iDoorArea1 = CWaypoints::Get(cDoor.iWaypoint).iAreaId;
                iDoorArea2 = CWaypoints::Get((TWaypointId)cDoor.pArguments).iAreaId;
            }

            TPlayerIndex iPlayerAtButton = EEntityIndexInvalid;
            TPlayerIndex iPlayerAtDoor = EEntityIndexInvalid;

            for ( int i = 0; (i < m_cCollaborativePlayers.size()) && ( (iPlayerAtButton == EEntityIndexInvalid) ||
                             (iDoor != 0xFF) && (iPlayerAtDoor == EEntityIndexInvalid) ); ++i )
            {
                if ( m_cCollaborativePlayers.test(i) )
                {
                    TAreaId iPlayerArea = m_aPlayersAreas[i];
                    if ( iPlayerArea == iButtonArea )
                        iPlayerAtButton = i;
                    else if ( iPlayerArea == iDoorArea1 || iPlayerArea == iDoorArea2 )
                        iPlayerAtDoor = i;
                }
            }
            return (iPlayerAtButton != EEntityIndexInvalid) && ((iDoor == 0xFF) || (iPlayerAtDoor != EEntityIndexInvalid));
        }

        case EBorzhTaskBringBox:
            return GetBoxInArea(GET_AREA(m_cCurrentBigTask.iArgument)) != m_aBoxes.end();

        case EBorzhTaskGoToGoal:
            return false;

        default:
            BASSERT(false);
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------
bool CBotBorzh::PlanStepNext()
{
    BASSERT( !CPlanner::IsRunning() );
    int iStep = m_iPlanStep++;
    const CPlanner::CPlan* pPlan = CPlanner::GetPlan();
    BASSERT( pPlan && iStep <= pPlan->size() );
    if ( iStep == pPlan->size() )
    {
        // All plan steps performed, end big task.
        BigTaskFinish();
        return false;
    }
    else
    {
        const CAction& cAction = pPlan->at(iStep);

        BotMessage( "%s -> step %d, action %s, argument %d, performer %s.", GetName(), iStep, CTypeToString::BotActionToString(cAction.iAction).c_str(),
            cAction.iArgument, CPlayers::Get(cAction.iExecutioner)->GetName() );

        PlanStepExecute(cAction);
        return true;
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PlanStepExecute( const CAction& cAction )
{
    m_cCurrentTask.iTask = EBorzhTaskInvalid;
    switch ( cAction.iAction )
    {
        case EBotActionMove:
        case EBotActionFall:
            if ( cAction.iExecutioner == m_iIndex )
            {
                TWaypointId iWaypoint = CModBorzh::GetRandomAreaWaypoint(cAction.iArgument);
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, iWaypoint) );
                //PushSpeakTask(EBorzhChatAreaIGo, MAKE_AREA(cAction.iArgument));
            }
            else
            {
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlayer, cAction.iExecutioner) );
                PushSpeakTask(EBorzhChatAreaGo, MAKE_AREA(cAction.iArgument) | MAKE_PLAYER(cAction.iExecutioner));
            }
            break;

        case EBotActionPushButton:
        case EBotActionShootButton:
        {
            bool bShoot = (CItems::GetItems(EEntityTypeButton)[cAction.iArgument].iWaypoint == EWaypointIdInvalid);
            BASSERT( bShoot || (cAction.iAction == EBotActionPushButton) );
            if ( cAction.iExecutioner == m_iIndex )
            {
                PushPressButtonTask(cAction.iArgument);
            }
            else
            {
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlayer, cAction.iExecutioner) );
                PushSpeakTask(bShoot ? EBorzhChatButtonYouShoot : EBorzhChatButtonYouPush, MAKE_BUTTON(cAction.iArgument) | MAKE_PLAYER(cAction.iExecutioner));
            }
            break;
        }

        case EBotActionCarryBox:
        case EBotActionCarryBoxFar:
            if ( cAction.iExecutioner == m_iIndex )
            {
                PerformActionCarryBox(cAction.iArgument);
            }
            else
            {
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlayer, cAction.iExecutioner) );
                PushSpeakTask(EBorzhChatBoxYouTake, MAKE_BOX(cAction.iArgument) | MAKE_PLAYER(cAction.iExecutioner));
            }
            break;

        case EBotActionDropBox:
            if ( cAction.iExecutioner == m_iIndex )
            {
                PerformActionDropBox(cAction.iArgument);
            }
            else
            {
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlayer, cAction.iExecutioner) );
                PushSpeakTask(EBorzhChatBoxYouDrop, MAKE_BOX(cAction.iArgument) | MAKE_PLAYER(cAction.iExecutioner));
            }
            break;

        case EBotActionClimbBox:
        {
            const good::vector<CWall>& cWalls = CModBorzh::GetWallsForArea(m_aPlayersAreas[m_iIndex]);
            BASSERT( cWalls.size() > 0 );
            if ( cAction.iExecutioner == m_iIndex )
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, cWalls[0].iHigherWaypoint) );
            else
            {
                TAreaId iArea = CWaypoints::Get(cWalls[0].iHigherWaypoint).iAreaId;
                m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlayer, cAction.iExecutioner) );
                PushSpeakTask(EBorzhChatAreaGo, MAKE_AREA(iArea) | MAKE_PLAYER(cAction.iExecutioner));
            }
            break;
        }

        default:
            BASSERT(false);
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::PlanStepLast()
{
    if ( m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor )
    {
        TEntityIndex iDoor = GET_DOOR(m_cCurrentBigTask.iArgument);
        if ( iDoor == 0xFF )
            return; // One bot is already at area of new button, so it will set new task to check that button.

        // Move one bot to button and another bot to door.
        TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
        const CEntity& cButton = CItems::GetItems(EEntityTypeButton)[iButton];
        bool bShoot = !CWaypoints::IsValid(cButton.iWaypoint);
        TWaypointId iWaypointButton = bShoot ? CModBorzh::GetWaypointToShootButton(iButton) : cButton.iWaypoint;
        BASSERT( CWaypoints::IsValid(iWaypointButton) );
        TAreaId iAreaButton = CWaypoints::Get(iWaypointButton).iAreaId;

        const CEntity& cDoor = CItems::GetItems(EEntityTypeDoor)[iDoor];
        TWaypointId iWaypointDoor1 = cDoor.iWaypoint;
        TWaypointId iWaypointDoor2 = (TWaypointId)cDoor.pArguments;
        BASSERT( CWaypoints::IsValid(iWaypointDoor1) && CWaypoints::IsValid(iWaypointDoor2) );
        TAreaId iAreaDoor1 = CWaypoints::Get(iWaypointDoor1).iAreaId;
        TAreaId iAreaDoor2 = CWaypoints::Get(iWaypointDoor2).iAreaId;

        TPlayerIndex iButtonPlayer = EPlayerIndexInvalid, iDoorPlayer = EPlayerIndexInvalid;

        // Check which bot presses the button and which goes to the door.
        for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size() && (iButtonPlayer == EPlayerIndexInvalid || iDoorPlayer == EPlayerIndexInvalid); ++iPlayer )
        {
            if ( m_cCollaborativePlayers.test(iPlayer) )
            {
                TAreaId iArea = m_aPlayersAreas[iPlayer];
                if ( (iButtonPlayer == EPlayerIndexInvalid) && (iArea == iAreaButton) )
                    iButtonPlayer = iPlayer;
                if ( (iDoorPlayer == EPlayerIndexInvalid) && ( (iArea == iAreaDoor1) || (iArea == iAreaDoor2) ) )
                    iDoorPlayer = iPlayer;
            }
        }

        BASSERT( (iButtonPlayer != EPlayerIndexInvalid) && (iDoorPlayer != EPlayerIndexInvalid) );

        // Press button or say to another player to do it. After this or another player goes to the door.
        if ( iButtonPlayer == m_iIndex )
            PushPressButtonTask(iButton);
        else
        {
            m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlayer, iButtonPlayer) );
            PushSpeakTask( EBorzhChatButtonYouPush, MAKE_BUTTON(iButton) | MAKE_PLAYER(iButtonPlayer) );

            //m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlayer, iButtonPlayer) );
            //PushSpeakTask( EBorzhChatButtonGo, MAKE_BUTTON(iButton) | MAKE_PLAYER(iButtonPlayer) );
        }

        // Go to the door or say to another player to do it.
        if ( iDoorPlayer == m_iIndex )
        {
            // Wait for another player to push the button.
            m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitButton, iButton) );
            // Move to door waypoint.
            TAreaId iArea = m_aPlayersAreas[iDoorPlayer];
            m_cTaskStack.push_back( CBorzhTask(EBorzhTaskMove, (iArea == iAreaDoor1) ? iWaypointDoor1 : iWaypointDoor2) );
        }
        else
        {
            m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitPlayer, iButtonPlayer) );
            PushSpeakTask(EBorzhChatDoorGo, MAKE_DOOR(iDoor) | MAKE_PLAYER(iDoorPlayer) );
        }
        m_cCurrentTask.iTask = EBorzhTaskInvalid;
    }
    else
    {
        BASSERT(EBorzhTaskGoToGoal);
        // GOAL IS REACHED!!!
        BASSERT(false);
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::UnexpectedChatForCollaborativeTask( TPlayerIndex iSpeaker, bool bWaitForThisPlayer )
{
    BASSERT( IsCollaborativeTask() && m_cCollaborativePlayers.test(iSpeaker) );
    if ( IS_ASKED_FOR_HELP() )
    {
        if ( !IS_ACCEPTED() || !IS_USING_PLANNER() || (GetPlanStepPerformer() == iSpeaker) )
        {
            CancelCollaborativeTask( bWaitForThisPlayer ? iSpeaker : EPlayerIndexInvalid );
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::CancelCollaborativeTask( TPlayerIndex iWaitForPlayer )
{
    BASSERT( IsCollaborativeTask() );

    // It is collaborative task, cancel it and wait until player is free.
    CancelTasksInStack();

    // Say task is cancelled.
    if ( IS_ACCEPTED() )
        PushSpeakTask(EBorzhChatTaskCancel);

    BigTaskFinish();

    // Wait for player if this player is busy right now.
    if ( iWaitForPlayer != EPlayerIndexInvalid )
    {
        m_cBusyPlayers.set(iWaitForPlayer);
        m_bNothingToDo = true;
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::CheckGoalReached( TPlayerIndex iSpeaker )
{
    if ( IsCollaborativeTask() && IS_ACCEPTED() && !IS_PUSHED() && m_cCollaborativePlayers.test(iSpeaker) && IsPlanPerformed() )
    {
        BotMessage("%s -> CheckGoalReached(): task %s, goal is reached", GetName(),
                   CTypeToString::BorzhTaskToString(m_cCurrentBigTask.iTask).c_str());
        CancelTasksInStack();
        switch ( m_cCurrentBigTask.iTask )
        {
            case EBorzhTaskButtonTryDoor:
            {
                if ( IS_USING_PLANNER() )
                {
                    CLEAR_USING_PLANNER();
                    CPlanner::Unlock(this);
                }
                else
                {
                    CLEAR_FOLLOWING_ORDERS();
                }
                TEntityIndex iButton = GET_BUTTON(m_cCurrentBigTask.iArgument);
                if ( GetButtonWaypoint(iButton, m_cReachableAreas, true) != EWaypointIdInvalid ) // One of bots should be in button's area.
                    PushPressButtonTask(iButton);
                else
                    m_cTaskStack.push_back( CBorzhTask(EBorzhTaskWaitButton) );
                break;
            }

            case EBorzhTaskBringBox:
                BigTaskFinish();
                break;

            case EBorzhTaskGoToGoal:
                //PushSpeakTask(EBorzhChatFinishMap);
                m_bNothingToDo = true;
                break;

            default:
                BASSERT(false);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::CheckIfSeeBox()
{
    // Check if bot sees a box.
    good::vector<CBoxInfo>::const_iterator itRealBox; // There is a box at current waypoint.
    good::vector<CBoxInfo>::iterator itBeliefBox;     // Bot thinks that there is a box is at current waypoint.

    const good::vector<CBoxInfo>& aBoxes = CModBorzh::GetBoxes();
    for ( itRealBox = aBoxes.begin(); itRealBox != aBoxes.end(); ++itRealBox )
    {
        if ( m_bCarryingBox && (m_iBox == itRealBox->iBox) )
            continue;

        if ( iCurrentWaypoint == itRealBox->iWaypoint )
            break;
    }
    for ( itBeliefBox = m_aBoxes.begin(); itBeliefBox != m_aBoxes.end(); ++itBeliefBox )
    {
        if ( m_bCarryingBox && (m_iBox == itBeliefBox->iBox) )
            continue;
        if ( iCurrentWaypoint == itBeliefBox->iWaypoint )
            break;
    }

    if ( (itRealBox != aBoxes.end()) || (itBeliefBox != m_aBoxes.end()) )
    {
        if ( (itRealBox != aBoxes.end()) && (itBeliefBox == m_aBoxes.end()) ) // There is a box at this position but bot didn't know it.
        {
            good::vector<CBoxInfo>::iterator it = good::find(m_aBoxes, *itRealBox);
            if ( it == m_aBoxes.end() ) // Bot didn't know about this box.
            {
                m_aBoxes.push_back( *itRealBox );

                if ( m_aWeapons[m_iPhyscannon].IsPresent() )
                    SwitchToSpeakTask(EBorzhChatHaveGravityGun);
                else
                    SwitchToSpeakTask(EBorzhChatNeedGravityGun);
                PushSpeakTask(EBorzhChatBoxFound, MAKE_BOX(itRealBox->iBox), EEntityTypeObject, itRealBox->iBox);
            }
            else // Bot did know about this box, but thought that it was somewhere else.
            {
                if ( it->iArea != itRealBox->iArea )
                {
                    SwitchToSpeakTask(EBorzhChatBoxFound, MAKE_BOX(itRealBox->iBox), EEntityTypeObject, itRealBox->iBox);
                    it->iArea = itRealBox->iArea;
                }
                it->iWaypoint = iCurrentWaypoint;
            }
        }
        else if ( (itRealBox == aBoxes.end()) && (itBeliefBox != m_aBoxes.end()) ) // Box changed it's position.
        {
            good::vector<CBoxInfo>::const_iterator it = good::find(aBoxes, *itBeliefBox);
            BASSERT( it != aBoxes.end() );
            if ( it->iArea != itBeliefBox->iArea )
            {
                // Remove box.
                SwitchToSpeakTask(EBorzhChatBoxLost, MAKE_BOX(itBeliefBox->iBox));
                m_aBoxes.erase(itBeliefBox);
            }
            else
                itBeliefBox->iWaypoint = it->iWaypoint;
        }
        else
        {
            BASSERT( itBeliefBox->iBox == itRealBox->iBox );
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBotBorzh::CheckToReturnBox()
{
    if ( m_bCarryingBox || m_bLostBox || IsUsingPlanner() || IS_FOLLOWING_ORDERS() )
        return;

    const good::vector<CWall>& cFalls = CModBorzh::GetFallsForArea( m_aPlayersAreas[m_iIndex] );
    if ( cFalls.size() )
    {
        good::vector<CWall>::const_iterator itDestination;
        for ( itDestination = cFalls.begin(); itDestination != cFalls.end(); ++itDestination )
            if ( iCurrentWaypoint == itDestination->iHigherWaypoint && m_iAfterNextWaypoint == itDestination->iLowerWaypoint )
                break;

        if ( itDestination != cFalls.end() )
        {
            TAreaId iDestinationArea = CWaypoints::Get(m_iAfterNextWaypoint).iAreaId;
            if ( GetBoxInArea(iDestinationArea) == m_aBoxes.end() ) // Where we going has no box to climb back.
            {
                for ( good::vector<CWall>::const_iterator it = cFalls.begin(); it != cFalls.end(); ++it )
                {
                    good::vector<CBoxInfo>::const_iterator itBox = GetBoxInArea( CWaypoints::Get(it->iLowerWaypoint).iAreaId );
                    if ( itBox != m_aBoxes.end() )
                    {
                        BotMessage("%s -> Will bring box %d to be able to go back.", GetName(), itBox->iBox);
                        SaveCurrentTask();
                        PushDropBoxTask(itBox->iBox, m_iAfterNextWaypoint);
                        PushGrabBoxTask(itBox->iBox, it->iHigherWaypoint, true);
                        return;
                    }
                }
            }
        }
    }
}

#endif // BOTRIX_BORZH
