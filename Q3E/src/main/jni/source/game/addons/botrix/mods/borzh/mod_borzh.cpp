#ifdef BOTRIX_BORZH

#include "chat.h"
#include "console_commands.h"
#include "event.h"
#include "mods/borzh/mod_borzh.h"
#include "players.h"
#include "source_engine.h"
#include "waypoint.h"


extern void GeneratePddl( bool bFromBotBelief );

//----------------------------------------------------------------------------------------------------------------
class CGeneratePddlCommand: public CConsoleCommand
{
public:
    CGeneratePddlCommand()
    {
        m_sCommand = "generate-pddl";
        m_sHelp = "use this command to generate PDDL for current map";
        m_iAccessLevel = FCommandAccessNone;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv )
    {
        if ( pClient == NULL )
            return ECommandError;

        if ( argc > 0 )
        {
            BULOG_E(pClient->GetEdict(), "Error, invalid parameters.");
            return ECommandError;
        }

        GeneratePddl(false);
        return ECommandPerformed;
    }
};

//----------------------------------------------------------------------------------------------------------------
TChatVariable CModBorzh::iVarDoor;
TChatVariable CModBorzh::iVarDoorStatus;
TChatVariable CModBorzh::iVarButton;
TChatVariable CModBorzh::iVarBox;
TChatVariable CModBorzh::iVarWeapon;
TChatVariable CModBorzh::iVarArea;
TChatVariable CModBorzh::iVarPlayer;

TChatVariableValue CModBorzh::iVarValueDoorStatusOpened;
TChatVariableValue CModBorzh::iVarValueDoorStatusClosed;

TChatVariableValue CModBorzh::iVarValueWeaponPhyscannon;
TChatVariableValue CModBorzh::iVarValueWeaponCrossbow;

good::vector< good::vector<TWaypointId> > CModBorzh::m_aAreasWaypoints;       // Waypoints for areas.
good::vector< good::vector<TEntityIndex>  >CModBorzh::m_aAreasDoors;          // Doors for areas.
good::vector< good::vector<TEntityIndex> > CModBorzh::m_aAreasButtons;        // Buttons for areas.
good::vector< good::vector<TWaypointId> > CModBorzh::m_aShootButtonWaypoints; // Waypoints to shoot buttons.

good::vector< good::vector<CWall> > CModBorzh::m_aWalls;                      // Walls for areas.
good::vector< good::vector<CWall> > CModBorzh::m_aFalls;                      // Falls for areas.
good::vector< CBoxInfo > CModBorzh::m_aBoxes(8);                              // Boxes.

int CModBorzh::m_iCheckBox;                                                   // Next box to get it waypoint.
float CModBorzh::m_fCheckBoxTime;                                             // Time to check next box waypoint.


//----------------------------------------------------------------------------------------------------------------
CModBorzh::CModBorzh()
{
    CMainCommand::instance.get()->Add( new CGeneratePddlCommand() );

    //CMod::AddEvent(new CPlayerActivateEvent()); // No need for this.
    CMod::AddEvent(new CPlayerTeamEvent());
    CMod::AddEvent(new CPlayerSpawnEvent());

    CMod::AddEvent(new CPlayerHurtEvent());
    CMod::AddEvent(new CPlayerDeathEvent());

    CMod::AddEvent(new CPlayerChatEvent());

    iVarDoor = CChat::AddVariable("$door");
    iVarDoorStatus = CChat::AddVariable("$door_status");
    iVarButton = CChat::AddVariable("$button");
    iVarBox = CChat::AddVariable("$box");
    iVarWeapon = CChat::AddVariable("$weapon");
    iVarArea = CChat::AddVariable("$area");
    iVarPlayer = CChat::AddVariable("$player"); // TODO: move away.

}


//----------------------------------------------------------------------------------------------------------------
void CModBorzh::MapLoaded()
{
    CChat::CleanVariableValues();

    // Add possible chat variable values for doors, buttons and weapons.
    iVarValueDoorStatusOpened = CChat::AddVariableValue(iVarDoorStatus, "opened");
    iVarValueDoorStatusClosed = CChat::AddVariableValue(iVarDoorStatus, "closed");

    iVarValueWeaponPhyscannon = CChat::AddVariableValue(iVarWeapon, "physcannon");
    iVarValueWeaponCrossbow = CChat::AddVariableValue(iVarWeapon, "crossbow");

    static char szInt[16];

    // Add possible doors values.
    const good::vector<CEntity>& aDoors = CItems::GetItems(EEntityTypeDoor);
    for ( TEntityIndex i=0; i < aDoors.size(); ++i )
    {
        sprintf(szInt, "%d", i+1);
        CChat::AddVariableValue( iVarDoor, good::string(szInt).duplicate() );
    }

    // Add possible buttons values.
    const good::vector<CEntity>& aButtons = CItems::GetItems(EEntityTypeButton);
    for ( TEntityIndex i=0; i < aButtons.size(); ++i )
    {
        sprintf(szInt, "%d", i+1);
        CChat::AddVariableValue( iVarButton, good::string(szInt).duplicate() );
    }

    // Add possible areas values.
    const StringVector& aAreas = CWaypoints::GetAreas();
    for ( TAreaId i=0; i < aAreas.size(); ++i )
        CChat::AddVariableValue( iVarArea, aAreas[i].duplicate() );

    // Add empty player names.
    for ( int i=0; i < CPlayers::Size(); ++i )
        CChat::AddVariableValue( iVarPlayer, "" );

    // Waypoints for areas.
    m_aAreasWaypoints.clear();
    if ( aAreas.size() > 0 )
    {
        m_aAreasWaypoints.resize( aAreas.size() );
        for ( TWaypointId iWaypoint = 0; iWaypoint < CWaypoints::Size(); ++iWaypoint )
            m_aAreasWaypoints[ CWaypoints::Get(iWaypoint).iAreaId ].push_back( iWaypoint );
    }

    // Doors for areas.
    m_aAreasDoors.clear();
    m_aAreasDoors.resize( aAreas.size() );
    for ( TEntityIndex iDoor = 0; iDoor < aDoors.size(); ++iDoor )
    {
        const CEntity& cDoor = aDoors[iDoor];
        TWaypointId iDoorWaypoint1 = cDoor.iWaypoint;
        TWaypointId iDoorWaypoint2 = (TWaypointId)cDoor.pArguments;
        if ( iDoorWaypoint1 != EWaypointIdInvalid )
            m_aAreasDoors[ CWaypoints::Get(iDoorWaypoint1).iAreaId ].push_back( iDoor );
        if ( iDoorWaypoint2 != EWaypointIdInvalid )
            m_aAreasDoors[ CWaypoints::Get(iDoorWaypoint2).iAreaId ].push_back( iDoor );
    }

    m_aAreasButtons.clear();
    m_aShootButtonWaypoints.clear();
    m_aWalls.clear();
    m_aFalls.clear();
    m_aBoxes.clear();

    if ( aAreas.size() > 0 )
    {
        // Buttons for areas.
        m_aAreasButtons.resize( aAreas.size() );
        for ( TEntityIndex iButton = 0; iButton < aButtons.size(); ++iButton )
        {
            const CEntity& cButton = aButtons[iButton];
            if ( cButton.iWaypoint != EWaypointIdInvalid )
                m_aAreasButtons[ CWaypoints::Get(cButton.iWaypoint).iAreaId ].push_back( iButton );
        }

        // Shoot buttons waypoints, walls.
        m_aShootButtonWaypoints.resize( aButtons.size() );
        m_aWalls.resize( aAreas.size() );
        m_aFalls.resize( aAreas.size() );
        for ( TWaypointId iWaypoint = 0; iWaypoint < CWaypoints::Size(); ++iWaypoint )
        {
            const CWaypoints::WaypointNode& cNode = CWaypoints::GetNode(iWaypoint);
            if ( FLAG_SOME_SET(FWaypointSeeButton, cNode.vertex.iFlags) )
                m_aShootButtonWaypoints[ CWaypoint::GetButton(cNode.vertex.iArgument) - 1 ].push_back(iWaypoint);
            for ( int i=0; i < cNode.neighbours.size(); ++i )
                if ( FLAG_SOME_SET(FPathTotem, cNode.neighbours[i].edge.iFlags) )
                {
                    TWaypointId iHigherWaypoint = cNode.neighbours[i].target;
                    m_aWalls[cNode.vertex.iAreaId].push_back( CWall(iWaypoint, iHigherWaypoint) );
                    m_aFalls[CWaypoints::Get(iHigherWaypoint).iAreaId].push_back( CWall(iWaypoint, iHigherWaypoint) );
                }
        }

        // Boxes.
        const good::vector<CEntity>& aObjects = CItems::GetItems(EEntityTypeObject);
        for ( TEntityIndex iObject = 0; iObject < aObjects.size(); ++iObject )
        {
            const CEntity& cObject = aObjects[iObject];
            if ( FLAG_SOME_SET(FObjectBox, cObject.iFlags) )
            {
                TWaypointId iBoxWaypoint = CWaypoints::GetNearestWaypoint( cObject.CurrentPosition() );
                m_aBoxes.push_back( CBoxInfo(iObject, iBoxWaypoint, (iBoxWaypoint == EWaypointIdInvalid) ? EAreaIdInvalid : CWaypoints::Get(iBoxWaypoint).iAreaId) );

                // Add box value.
                sprintf(szInt, "%d", iObject+1);
                CChat::AddVariableValue( iVarBox, good::string(szInt).duplicate() );
            }
        }
    }

    m_iCheckBox = 0;
    m_fCheckBoxTime = CBotrixPlugin::fTime + 1.0f;
}

//----------------------------------------------------------------------------------------------------------------
void CModBorzh::Think()
{
    if ( (m_aBoxes.size() > 0) && (CBotrixPlugin::fTime >= m_fCheckBoxTime) )
    {
        CBoxInfo& cBoxInfo = m_aBoxes[m_iCheckBox];
        const CEntity& cBox = CItems::GetItems(EEntityTypeObject)[ cBoxInfo.iBox ];
        if ( !cBox.IsFree() && cBox.IsOnMap() )
        {
            cBoxInfo.iWaypoint = CWaypoints::GetNearestWaypoint( cBox.CurrentPosition() );
            if ( cBoxInfo.iWaypoint == EWaypointIdInvalid )
                cBoxInfo.iArea = EAreaIdInvalid;
            else
                cBoxInfo.iArea = CWaypoints::Get(cBoxInfo.iWaypoint).iAreaId;
        }
        else
        {
            cBoxInfo.iWaypoint = EWaypointIdInvalid;
            cBoxInfo.iArea = EAreaIdInvalid;
        }
        m_iCheckBox = (m_iCheckBox+1) % m_aBoxes.size();
        m_fCheckBoxTime = CBotrixPlugin::fTime + 0.2f; // 5 boxes per second.
    }
}

//----------------------------------------------------------------------------------------------------------------
TWaypointId CModBorzh::GetRandomAreaWaypoint( TAreaId iArea )
{
    const good::vector<TWaypointId>& aWaypoints = CModBorzh::GetWaypointsForArea(iArea);

    // Get random waypoint until it is not near door.
    int iWaypoint;
    bool bDone;
    do {
        bDone = true;
        iWaypoint = aWaypoints[rand() % aWaypoints.size()];
        const CWaypoints::WaypointNode& cNode = CWaypoints::GetNode(iWaypoint);
        for ( int i = 0; i < cNode.neighbours.size(); ++i )
            if ( FLAG_SOME_SET(FPathDoor, cNode.neighbours[i].edge.iFlags) )
            {
                bDone = false;
                break;
            }

    } while ( !bDone );
    return iWaypoint;
}

//----------------------------------------------------------------------------------------------------------------
/*TEntityIndex CModBorzh::GetBoxForAreas( const good::bitset& aPossibleAreas, TWaypointId* iBoxWaypoint )
{
    for ( int iBox = 0; iBox < m_aBoxes.size(); ++iBox )
    {
        const CEntity* pBox = &m_aBoxes[iBox];
        if ( !pBox->IsFree() && pBox->IsOnMap() )
        {
            TWaypointId iWaypoint = CWaypoints::GetNearestWaypoint( pBox->CurrentPosition() );
            if ( (iWaypoint != EWaypointIdInvalid) && aPossibleAreas.test( CWaypoints::Get(iWaypoint).iAreaId ) )
            {
                if ( iBoxWaypoint )
                    *iBoxWaypoint = iWaypoint;
                return iBox;
            }
        }
    }

    return EEntityIndexInvalid;
}*/

//----------------------------------------------------------------------------------------------------------------
const CWall* CModBorzh::GetWallBetweenAreas( TAreaId iLowerArea, TAreaId iHigherArea )
{
    const good::vector<CWall>& aWalls = m_aWalls[iLowerArea];
    for ( int iWall = 0; iWall < aWalls.size(); ++iWall )
        if ( iHigherArea == CWaypoints::Get(aWalls[iWall].iHigherWaypoint).iAreaId ) // We are heading to high area, need a box.
            return &aWalls[iWall];
    return NULL;
}

//----------------------------------------------------------------------------------------------------------------
bool CModBorzh::IsWallClimbable( const CWall& cWall )
{
    Vector w1 = CWaypoints::Get(cWall.iLowerWaypoint).vOrigin;
    w1.z -= CMod::iPlayerEyeLevel;
    Vector w2 = CWaypoints::Get(cWall.iHigherWaypoint).vOrigin;
    w2.z -= CMod::iPlayerEyeLevel;

    float sqrDist = (w1 - w2).Length2DSqr();

    for ( int iBox = 0; iBox < m_aBoxes.size(); ++iBox )
    {
        const CBoxInfo& cBoxInfo = m_aBoxes[iBox];
        const CEntity& cBox = CItems::GetItems(EEntityTypeObject)[ cBoxInfo.iBox ];
        if ( !cBox.IsFree() && cBox.IsOnMap() )
        {
            const Vector& vBox = cBox.CurrentPosition();
            if ( w1.z <= vBox.z && vBox.z <= w2.z )
            {
                float sqrDistToW1 = (vBox - w1).Length2DSqr();
                if ( sqrDistToW1 > sqrDist )
                    continue;
                float sqrDistToW2 = (vBox - w2).Length2DSqr();
                if ( sqrDistToW2 > sqrDist )
                    continue;
                return true;
            }
        }
    }
    return false;
}

#endif // BOTRIX_BORZH
