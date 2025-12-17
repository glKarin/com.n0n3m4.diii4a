#ifdef BOTRIX_BORZH

#ifndef __BOTRIX_BORZH_H__
#define __BOTRIX_BORZH_H__


#include "chat.h"
#include "mod.h"


/// Class to represent high area that needs a box to climb to.
class CWall
{
public:
    CWall() {}
    CWall( TWaypointId iLower, TWaypointId iHigher ): iLowerWaypoint(iLower), iHigherWaypoint(iHigher) {}

    TWaypointId iLowerWaypoint;
    TWaypointId iHigherWaypoint;
};


/// Class to represent info for a box.
class CBoxInfo
{
public:
    CBoxInfo() {}
    CBoxInfo( TEntityIndex iBox, TWaypointId iWaypoint, TAreaId iArea): iBox(iBox), iWaypoint(iWaypoint), iArea(iArea) {}

    bool operator==( const CBoxInfo& cOther ) const { return iBox == cOther.iBox; }
    bool operator==( TEntityIndex iOtherBox ) const { return iBox == iOtherBox; }

    TEntityIndex iBox;
    TWaypointId iWaypoint;
    TAreaId iArea;
};


/// Class for BorzhMod.
class CModBorzh: public IMod
{
public: // Methods.

    /// Constructor. Initializes events and chat variables.
    CModBorzh();

    /// Load chat configuration file.
    virtual void LoadConfig(const good::string& sModName) {}

    /// Called when map is loaded, after waypoints and items has been loaded. TODO: map unload.
    virtual void MapLoaded();


    /// Get waypoint type count.
    virtual int GetWaypointTypeCount() { return 0; }

    /// Get waypoint type names.
    virtual const good::string* GetWaypointTypeNames() { return NULL; }

    /// Get waypoint type colors.
    virtual const int* GetWaypointTypeColors() { return NULL; }


    /// Get waypoint path count.
    virtual int GetWaypointPathCount() { return 0; }

    /// Get waypoints path names.
    virtual const good::string* GetWaypointPathNames() { return NULL; }

    /// Get waypoints path colors.
    virtual const int* GetWaypointPathColors() { return NULL; }


    /// Get bot's objective count.
    virtual int GetObjectivesCount() { return 0; }

    /// Get bot's objective names.
    virtual const good::string* GetObjectiveNames() { return NULL; }


    /// Get chat count.
    virtual int GetChatCount() { return CHATS_COUNT; }

    /// Get chat names.
    virtual const good::string* GetChatNames() { return NULL; }

    /// Mod think function.
    virtual void Think();

public: // Static methods.

    /// Get random waypoint at given area, away from any door.
    static TWaypointId GetRandomAreaWaypoint( TAreaId iArea );

    /// Get waypoints that are in given area.
    static const good::vector<TWaypointId>& GetWaypointsForArea( TAreaId iArea ) { return m_aAreasWaypoints[iArea]; }

    /// Get doors that are in given area.
    static const good::vector<TEntityIndex>& GetDoorsForArea( TAreaId iArea ) { return m_aAreasDoors[iArea]; }

    /// Get buttons that are in given area.
    static const good::vector<TEntityIndex>& GetButtonsForArea( TAreaId iArea ) { return m_aAreasButtons[iArea]; }

    /// Get all boxes.
    static const good::vector<CBoxInfo>& GetBoxes() { return m_aBoxes; }

    /// Get walls between 2 areas.
    static const CWall* GetWallBetweenAreas( TAreaId iLowerArea, TAreaId iHigherArea );

    /// Get walls for given area.
    static const good::vector<CWall>& GetWallsForArea( TAreaId iArea ) { return m_aWalls[iArea]; }

    /// Get falls for given area.
    static const good::vector<CWall>& GetFallsForArea( TAreaId iArea ) { return m_aFalls[iArea]; }

    /// Get walls for given area.
    static bool IsWallClimbable( const CWall& cWall);

    /// Get waypoint to shoot button.
    static TWaypointId GetWaypointToShootButton( TEntityIndex iButton )
    {
        const good::vector<TWaypointId>& aWaypoints = m_aShootButtonWaypoints[iButton];
        return aWaypoints.at( rand() % aWaypoints.size() );
    }


public: // Members.
    static TChatVariable iVarDoor;
    static TChatVariable iVarDoorStatus;
    static TChatVariable iVarButton;
    static TChatVariable iVarBox;
    static TChatVariable iVarWeapon;
    static TChatVariable iVarArea;
    static TChatVariable iVarPlayer;
    static const int iTotalVars = 7;

    static TChatVariableValue iVarValueDoorStatusOpened;
    static TChatVariableValue iVarValueDoorStatusClosed;

    static TChatVariableValue iVarValueWeaponPhyscannon;
    static TChatVariableValue iVarValueWeaponCrossbow;

protected:
    static const int CHATS_COUNT = 23;
    //static const good::string m_aChats[CHATS_COUNT];

    static good::vector< good::vector<TWaypointId> > m_aAreasWaypoints;       // Waypoints for areas.
    static good::vector< good::vector<TEntityIndex> > m_aAreasDoors;          // Doors for areas.
    static good::vector< good::vector<TEntityIndex> > m_aAreasButtons;        // Buttons for areas.
    static good::vector< good::vector<TWaypointId> > m_aShootButtonWaypoints; // Waypoints to shoot buttons.
    static good::vector< good::vector<CWall> > m_aWalls;                      // Walls for areas.
    static good::vector< good::vector<CWall> > m_aFalls;                      // Falls for areas.
    static good::vector< CBoxInfo > m_aBoxes;                                 // Boxes.

    static int m_iCheckBox;                                                   // Next box to get it waypoint.
    static float m_fCheckBoxTime;                                             // Time to check next box waypoint.

};

#endif // __BOTRIX_BORZH_H__

#endif // BOTRIX_BORZH
