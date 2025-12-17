#ifdef BOTRIX_TF2

#ifndef __BOTRIX_MOD_TF2_H__
#define __BOTRIX_MOD_TF2_H__


#include "mod.h"


/// Class for Team Fortress 2 mod.
class CModTF2: public IMod
{

public: // Methods.
    /// Constructor. Initializes console commands.
    CModTF2() {}

    //------------------------------------------------------------------------------------------------------------
    // Implementation of IMod inteface.
    //------------------------------------------------------------------------------------------------------------
    /// Process configuration file.
    virtual bool ProcessConfig( const good::ini_file& /*cIni*/ ) { return true; }


    /// Called when map is loaded, after waypoints and items has been loaded.
    virtual void MapLoaded() {}

    /// Called when map is unloaded.
    virtual void MapFinished() {}


    /// Add bot with given name, intelligence, class and other optional parameters.
    virtual CPlayer* AddBot( const char* szName, TBotIntelligence iIntelligence, TTeam iTeam,
                                 TClass iClass, int iParamsCount, const char **aParams );


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


    /// Get chat count.
    virtual int GetChatCount() { return 0; }

    /// Get chat names.
    virtual const good::string* GetChatNames() { return NULL; }

    /// Mod think function.
    virtual void Think() {}


};

#endif // __BOTRIX_MOD_TF2_H__

#endif // BOTRIX_TF2
