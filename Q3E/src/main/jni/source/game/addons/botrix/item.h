#ifndef __BOTRIX_ITEM_H__
#define __BOTRIX_ITEM_H__


#include <good/list.h>
#include <good/bitset.h>
#include <good/vector.h>
#include <good/utility.h>

#include "defines.h"
#include "types.h"

#include "public/edict.h"


class CClient;
class CPlayer;


//****************************************************************************************************************
/// Class that represents class of entities.
//****************************************************************************************************************
class CItemClass
{
public:
    /// Constructor.
    CItemClass(): szEngineName(NULL), iFlags(0), fPickupDistanceSqr(0.0f)  {}

    /// Set class argument.
    void SetArgument( int iArgument ) { SET_2ND_WORD(iArgument, iFlags); }

    /// Get class argument.
    int GetArgument() const { return GET_2ND_WORD(iFlags); }

    /// == operator.
    bool operator==( const CItemClass& cOther ) const
    {
        return ( szEngineName && (szEngineName == cOther.szEngineName) ) ||
               ( sClassName == cOther.sClassName );
    }

    /// == operator with string.
    bool operator==( const good::string& sOther ) const
    {
        return (sClassName == sOther);
    }

    good::string sClassName;            ///< Entity class name (like "prop_physics_multiplayer" or "item_healthkit").
    const char* szEngineName;           ///< Can compare this string with edict_t::GetClassName() only by pointer. Faster.
    TItemFlags iFlags;                  ///< Entity flags and argument (how much health/armor restore, or bullets gives etc.).
    float fPickupDistanceSqr;           ///< Distance to entity to consider it can be picked up.
};



//****************************************************************************************************************
/// Class that represents entity on a map.
//****************************************************************************************************************
class CItem
{
public:
    /// Default constructor for array templates.
    CItem(): pEdict(NULL) {}

    /// Constructor with parameters.
    CItem( edict_t* pEdict, TItemFlags iFlags, float fPickupDistanceSqr, const CItemClass* pItemClass,
             const Vector& vOrigin, TWaypointId iWaypoint ):
        pEdict(pEdict), iFlags(iFlags), fPickupDistanceSqr(fPickupDistanceSqr), iWaypoint(iWaypoint),
        vOrigin(vOrigin), pItemClass(pItemClass), pArguments(NULL) {}

    /// Return true if item was freed, (for example broken, but not respawnable).
    bool IsFree() const { return (pEdict == NULL) || pEdict->IsFree(); }

    /// Return true if item currently is on map (not broken or taken).
    bool IsOnMap() const;

    /// Return true if item is breakable (bot will break it instead of jumping on it or moving it).
    bool IsBreakable() const;

    /// Return true if item is taken (grabbed by player/bot).
    bool IsTaken() const;

    /// Return true if item can be picked up by gravity gun.
    bool CanPickup() const;

    /// Return true if item can explode (bot will never break it and can throw it, if near).
    bool IsExplosive() const { return FLAG_SOME_SET(FObjectExplosive, iFlags); }

    /// Return true if item can't be picked with physcannon.
    bool IsHeavy() const { return FLAG_SOME_SET(FObjectHeavy, iFlags); }

    /// Return current position of the item.
    const Vector& CurrentPosition() const { return pEdict->GetCollideable()->GetCollisionOrigin(); }

    /// == operator.
    bool operator== ( const CItem& other ) const { return pEdict == other.pEdict; }

    /// Maximum distance from item to waypoint, to consider that to grab item you need to go to that waypoint.
	static const int iMaxDistToWaypoint = 100;

    edict_t* pEdict;                    ///< Entity's edict.
    TItemFlags iFlags;                  ///< Entity's flags.
    float fPickupDistanceSqr;           ///< Distance to entity to consider it can be picked up.
    TWaypointId iWaypoint;              ///< Entity's nearest waypoint.
    Vector vOrigin;                     ///< Entity's respawn position on map (bots will be looking there).
    const CItemClass* pItemClass;       ///< Entity's class.
    void* pArguments;                   ///< Entity's arguments. For example for door we have 2 waypoints.
};


//****************************************************************************************************************
/// Class that represents picked item along with pick time.
//****************************************************************************************************************
class CPickedItem
{
public:
    /// Constructor with entity.
    CPickedItem( TItemType iType, TItemIndex iIndex, float fRemoveTime = 0.0f ):
        iType(iType), iIndex(iIndex), fRemoveTime(fRemoveTime) {}

    /// == operator.
    bool operator==( const CPickedItem& other ) const { return (other.iType == iType) && (other.iIndex == iIndex); }

    TItemType iType;    ///< Item's type (health, armor, ammo or weapon).
    TItemIndex iIndex;  ///< Entity's index in array CItems::Get(iType).
    float fRemoveTime;    ///< Time when item should be removed from array of picked items (0 if shouldn't).
};


//****************************************************************************************************************
/// Class that represents set of items on map (health, armor, boxes, chairs, etc.).
//****************************************************************************************************************
class CItems
{

public:

    /// Will print all item classes.
	static void PrintClasses();

    /// Get item from item id (edict's index).
    static TItemType GetItemFromId( TItemId iId, TItemIndex* pIndex );

        /// Get item classes.
	static const good::list<CItemClass>* GetClasses() { return m_aItemClasses; }

    /// Get random item clas for given entity type.
    static const CItemClass* GetRandomItemClass( TItemType iEntityType )
    {
        BASSERT( !m_aItemClasses[iEntityType].empty(), return NULL );
        int iSize = m_aItemClasses[iEntityType].size();
        return iSize ? &good::at(m_aItemClasses[iEntityType], rand() % iSize) : NULL;
    }

    /// Get array of items of needed type.
    static const good::vector<CItem>& GetItems( TItemType iEntityType ) { return m_aItems[iEntityType]; }

    /// Get items class for entity class name.
    static const CItemClass* GetItemClass( TItemType iEntityType, const good::string& sClassName )
    {
        good::list<CItemClass>::const_iterator it = good::find(m_aItemClasses[iEntityType], sClassName);
        return ( it == m_aItemClasses[iEntityType].end() ) ? NULL : &*it;
    }

    /// Get entity type and class given entity name.
    static TItemType GetEntityType( const char* szClassName, CItemClass* & pEntityClass, TItemType iFrom, TItemType iTo );

    /// Get nearest item for a class (for example some item_battery or item_suitcharger for armor), skipping picked items in aSkip array.
    static TItemIndex GetNearestItem( TItemType iEntityType, const Vector& vOrigin, const good::vector<CPickedItem>& aSkip, const CItemClass* pClass = NULL );

    /// Return true if at least one entity of this class exists on current map.
    static bool ExistsOnMap( const CItemClass* pEntityClass ) { return pEntityClass->szEngineName != NULL; }

    /// Add item class (for example item_healthkit for health class).
    static const CItemClass* AddItemClassFor( TItemType iEntityType, CItemClass& cItemClass )
    {
        m_aItemClasses[iEntityType].push_back(cItemClass);
        return &m_aItemClasses[iEntityType].back();
    }

    /// Set object flags for given model.
    static void SetObjectFlagForModel( TItemFlags iItemFlag, const good::string& sModel )
    {
        m_aObjectFlagsForModels.push_back( good::pair<good::string, TItemFlags>(sModel, iItemFlag) );
    }

#ifndef BOTRIX_SOURCE_ENGINE_2006
    /// Called when entity is allocated. Return true if entity is health/armor/weapon/ammo/object.
    static void Allocated( edict_t* pEdict );

    /// Called when entity is freed.
    static void Freed( const edict_t* pEdict );
#endif

    /// Clear all item classes.
    static void Unload()
    {
        MapUnloaded();
        for ( int iType = 0; iType < EItemTypeKnownTotal; ++iType )
            m_aItemClasses[iType].clear();
        m_aObjectFlagsForModels.clear();
    }

    /// Clear all loaded entities.
    static void MapUnloaded( bool bClearObjectFlags = false );

    /// Load entities from current map.
	static void MapLoaded(bool bLog = true);

    /// Update items. Called when player is connected or picks up weapon (new one will be created to respawn later).
    static void Update();


    /// Get all objects flags.
    static const good::vector<TItemId>& GetObjectsFlags() { return m_aObjectFlags; }

    /// Set all objects flags.
    static void SetObjectsFlags( const good::vector<TItemId>& aObjectsFlags ) { m_aObjectFlags = aObjectsFlags; }

    /// Get object flags.
    static bool GetObjectFlags( TItemId iObject, TItemFlags &iFlags );

    /// Set flags for object index.
    static bool SetObjectFlags( TItemId iObject, TItemFlags iFlags );


    /// Check if given door is closed (a ray hits it).
    static int IsDoorClosed( TItemIndex iDoor );

    /// Check if given elevator is on the high floor (a ray hits it).
    static int IsElevatorHigh( TItemIndex iElevator ) { return IsDoorClosed( iElevator ); }

    /// Draw items for a given client.
    static void Draw( CClient* pClient );

protected:
    static void CheckNewEntity( edict_t* pEdict, bool bLog = true );
    static TItemIndex NewEntityIndex( int iEntityType );
    static void AutoWaypointPathFlagsForEntity( TItemType iEntityType, TItemIndex iIndex, CItem& cEntity );
    static TItemIndex AddItem( TItemType iEntityType, edict_t* pEdict, CItemClass* pItemClass, IServerEntity* pServerEntity );
    static void AddObject( edict_t* pEdict, const CItemClass* pObjectClass, IServerEntity* pServerEntity );

    //friend class CWaypoints; // Give access to WaypointDeleted().
    //static void WaypointDeleted( TWaypointId id );

    static good::vector<CItem> m_aItems[EItemTypeKnownTotal];            // Array of items.
    static good::list<CItemClass> m_aItemClasses[EItemTypeKnownTotal];   // List of item classes. Pointer are used so it should not be reallocated.
    static TItemIndex m_iFreeIndex[EItemTypeKnownTotal];                 // First free entity index.
    static int m_iFreeEntityCount[EItemTypeKnownTotal];                  // Free entities count. TODO:

    static good::vector<edict_t*> m_aOthers;                             // Array of other entities.

    static TItemIndex m_iCurrentEntity;                                  // Current entity index to check.
    static int m_iMaxEntityIndex;                                        // Maximum number of known used entity index.
    
    static const int m_iCheckEntitiesPerFrame = 32;

    // This one is to have models specific flags (for example car model with 'heavy' flag, or barrel model with 'explosive' flag).
    static good::vector< good::pair<good::string, TItemFlags> > m_aObjectFlagsForModels;

    static good::vector< TItemIndex > m_aObjectFlags; // Array of (object index, item flags).

    static good::bitset m_aUsedItems; // To know which items are already in m_aItems.

    static bool m_bMapLoaded; // Will be set to true at MapLoaded() and to false at Clear().

#ifndef BOTRIX_SOURCE_ENGINE_2006
    static good::vector<edict_t*> m_aNewEntities; // When Allocated() is called, new entity still has no IServerEntity, check at next frame.
#endif
};


#endif // __BOTRIX_ITEM_H__
