#include "cbase.h"

// Ugly fix for Source Engine.
#undef MINMAX_H
#undef min
#undef max
#include "bot.h"
#include "clients.h"
#include "item.h"
#include "players.h"
#include "server_plugin.h"
#include "source_engine.h"
#include "type2string.h"
#include "waypoint.h"
//#include <game/server/iplayerinfo.h>
#include <game/shared/props_shared.h>
#include <public/vphysics/friction.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifndef BOTRIX_SOURCE_ENGINE_2006
#define BOTRIX_ENTITY_USE_KEY_VALUE
#endif


extern IVDebugOverlay* pVDebugOverlay;

extern char* szMainBuffer;
extern int iMainBufferSize;

char szValueBuffer[16];

typedef struct
{
    unsigned short iItemType;
    unsigned short iItemIndex;
} fast_edict_index_t;

fast_edict_index_t m_aEdictsIndexes[ MAX_EDICTS ];

//================================================================================================================
//inline int GetEntityFlags( IServerEntity* pServerEntity )
//{
//    // ObjectCaps(), GetMaxHealth(), IsAlive().
//    CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
//    //return pEntity->GetEFlags(); // GetEFlags();
//    int iFlags1 = pEntity->GetEffects();
//    int iFlags2 = pEntity->GetEFlags();
//    int iFlags3 = pEntity->GetFlags();
//    //int iFlags4 = pEntity->GetSolidFlags();
//    int iFlags5 = pEntity->GetSpawnFlags();
//
//#ifdef BOTRIX_ENTITY_USE_KEY_VALUE
//    pEntity->GetKeyValue( "ignite", szValueBuffer, 16 );
//    int flags = atoi( szValueBuffer );
//    return iFlags5;
//#else
//    return pEntity->GetEffects();
//#endif
//}

inline int GetEntityEffects( IServerEntity* pServerEntity )
{
    // ObjectCaps(), GetMaxHealth(), IsAlive().
    CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
#ifdef BOTRIX_ENTITY_USE_KEY_VALUE
    pEntity->GetKeyValue("effects", szValueBuffer, 16);
    int flags = atoi(szValueBuffer);
    return flags;
#else
    return pEntity->GetEffects();
#endif
}

inline int GetEntityHealth( IServerEntity* pServerEntity )
{
    CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
#ifdef BOTRIX_ENTITY_USE_KEY_VALUE
    pEntity->GetKeyValue("health", szValueBuffer, 16);
    int health = atoi(szValueBuffer);
    return health;
#else
    return pEntity->GetHealth();
#endif
}

inline bool IsEntityOnMap( IServerEntity* pServerEntity )
{
     // EF_BONEMERGE is set for weapon, when picked up.
    return FLAG_CLEARED( EF_NODRAW | EF_BONEMERGE, GetEntityEffects(pServerEntity) );
    //return !FLAG_SOME_SET( EF_NODRAW | EF_BONEMERGE, GetEntityEffects(pServerEntity) );
}

inline bool IsEntityTaken( IServerEntity* pServerEntity )
{
     // EF_BONEMERGE is set for weapon, when picked up.
    return FLAG_SOME_SET( EF_BONEMERGE, GetEntityEffects(pServerEntity) );
}

inline bool IsEntityBreakable( IServerEntity* pServerEntity )
{
    return GetEntityHealth(pServerEntity) != 0;
}

//inline bool CanPickupEntity( IServerEntity* pServerEntity )
//{
//    CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
//    if ( pEntity == NULL )
//        return false;
//
//    // Not working:
//    IPhysicsObject *pObject = (IPhysicsObject *)pEntity;
//    if ( pObject )
//        return pObject->IsMoveable();
//
//    // Not working:
//    int iEFlags = pEntity->GetEFlags();
//    int iFlags3 = pEntity->GetFlags();
//    int iFlags4 = pEntity->GetSolidFlags();
//    int iSpawnFlags = pEntity->GetSpawnFlags();
//    return !FLAG_SOME_SET( EFL_NO_PHYSCANNON_INTERACTION, iEFlags ) && ( FLAG_SOME_SET( SF_PHYSPROP_ALWAYS_PICK_UP, iSpawnFlags ) ||
//                                                                         !FLAG_SOME_SET( SF_PHYSPROP_PREVENT_PICKUP, iSpawnFlags ) );
//}

inline string_t GetEntityName( IServerEntity* pServerEntity )
{
    CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
    return pEntity->GetEntityName();
}


//================================================================================================================
bool CItem::IsOnMap() const
{
    return IsEntityOnMap(pEdict->GetIServerEntity());
}

//----------------------------------------------------------------------------------------------------------------
bool CItem::IsBreakable() const
{
    return IsEntityBreakable(pEdict->GetIServerEntity());
}

//----------------------------------------------------------------------------------------------------------------
bool CItem::IsTaken() const
{
    return IsEntityTaken( pEdict->GetIServerEntity() );
}


//================================================================================================================
good::vector<CItem> CItems::m_aItems[EItemTypeKnownTotal];            // Array of items.
good::list<CItemClass> CItems::m_aItemClasses[EItemTypeKnownTotal];   // Array of item classes.
TItemIndex CItems::m_iFreeIndex[EItemTypeKnownTotal];                 // First free weapon index.
int CItems::m_iFreeEntityCount[EItemTypeKnownTotal];                  // Count of unused entities.

good::vector<edict_t*> CItems::m_aOthers(256);                        // Array of other entities.

#ifndef BOTRIX_SOURCE_ENGINE_2006
good::vector<edict_t*> CItems::m_aNewEntities(16);
#endif

good::vector< good::pair<good::string, TItemFlags> > CItems::m_aObjectFlagsForModels(4);
good::vector< TItemIndex > CItems::m_aObjectFlags;

good::bitset CItems::m_aUsedItems(MAX_EDICTS);
int CItems::m_iCurrentEntity;
int CItems::m_iMaxEntityIndex = MAX_EDICTS;
bool CItems::m_bMapLoaded = false;


//----------------------------------------------------------------------------------------------------------------
void CItems::PrintClasses()
{
	for ( TItemType iEntityType = 0; iEntityType < EItemTypeKnownTotal; ++iEntityType )
	{
		BLOG_D( "Item type %s:", CTypeToString::EntityTypeToString( iEntityType ).c_str() );
		const good::list<CItemClass>& aItemClasses = m_aItemClasses[iEntityType];

		for ( good::list<CItemClass>::const_iterator it = aItemClasses.begin(); it != aItemClasses.end(); ++it )
			BLOG_D( "  Item class %s", it->sClassName.c_str() );
	}
}

TItemType CItems::GetItemFromId( TItemId iId, TItemIndex* pIndex )
{
    GoodAssert( 0 <= iId && iId < MAX_EDICTS );
    const fast_edict_index_t& pLookup = m_aEdictsIndexes[ iId ];
    if ( pIndex )
        *pIndex = pLookup.iItemIndex;
    return pLookup.iItemType;
}

TItemIndex CItems::GetNearestItem( TItemType iEntityType, const Vector& vOrigin, const good::vector<CPickedItem>& aSkip, const CItemClass* pClass )
{
    good::vector<CItem>& aItems = m_aItems[iEntityType];

    TItemIndex iResult = -1;
    float fSqrDistResult = 0.0f;

    for ( int i = 0; i < aItems.size(); ++i )
    {
        CItem& cItem = aItems[i];
        if ( (cItem.pEdict == NULL) || cItem.pEdict->IsFree() || !CWaypoint::IsValid(cItem.iWaypoint) ||
             ( pClass && (pClass != cItem.pItemClass) ) ) // If item is already added, we have engine name.
            continue;

        CPickedItem cPickedItem( iEntityType, i );
        if ( find(aSkip.begin(), aSkip.end(), cPickedItem) != aSkip.end() ) // Skip this item.
            continue;

        float fSqrDist = vOrigin.DistToSqr( cItem.CurrentPosition() );
        if ( (iResult == -1) || (fSqrDist < fSqrDistResult) )
        {
            iResult = i;
            fSqrDistResult = fSqrDist;
        }
    }

    return iResult;
}


#ifndef BOTRIX_SOURCE_ENGINE_2006

//----------------------------------------------------------------------------------------------------------------
void CItems::Allocated( edict_t* pEdict )
{
    if ( m_bMapLoaded )
        m_aNewEntities.push_back(pEdict);
}


//----------------------------------------------------------------------------------------------------------------
void CItems::Freed( const edict_t* pEdict )
{
    if ( !m_bMapLoaded )
        return;

    int iIndex = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict);
    GoodAssert( iIndex > 0 ); // Valve should not allow this assert.

    m_aEdictsIndexes[ iIndex ].iItemType = EItemTypeOther;

    // Check only server entities.
    if ( !m_aUsedItems.test(iIndex) )
        return;

    m_aUsedItems.reset(iIndex);
    good::vector<CItem>& aWeapons = m_aItems[EItemTypeWeapon];
    for ( TItemIndex i=0; i < (int)aWeapons.size(); ++i )
        if ( aWeapons[i].pEdict == pEdict )
        {
            aWeapons[i].pEdict = NULL;
            m_iFreeIndex[EItemTypeWeapon] = i;
            return;
        }
    //BASSERT(false); // Only weapons are allocated/deallocated while map is running.
}
#endif // BOTRIX_SOURCE_ENGINE_2006

//----------------------------------------------------------------------------------------------------------------
void CItems::MapUnloaded( bool bClearObjectFlags )
{
    for ( TItemType iEntityType = 0; iEntityType < EItemTypeKnownTotal; ++iEntityType )
    {
        good::list<CItemClass>& aClasses = m_aItemClasses[iEntityType];
        good::vector<CItem>& aItems = m_aItems[iEntityType];

        aItems.clear();
        m_iFreeIndex[iEntityType] = -1;      // Invalidate free entity index.

        for ( good::list<CItemClass>::iterator it = aClasses.begin(); it != aClasses.end(); ++it )
        {
            it->szEngineName = NULL; // Invalidate class name, because it was loaded in previous map.
            aItems.reserve(64);      // At least.
        }
    }

#ifndef BOTRIX_SOURCE_ENGINE_2006
    m_aNewEntities.clear();
#endif
    m_aOthers.clear();
    if ( bClearObjectFlags )
        m_aObjectFlags.clear();
    m_aUsedItems.reset();
    m_bMapLoaded = false;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::MapLoaded(bool bLog)
{
    m_iCurrentEntity = CPlayers::Size()+1;

    // 0 is world, 1..max players are players. Other entities are from indexes above max players.
    m_iMaxEntityIndex = CBotrixPlugin::pEngineServer->GetEntityCount();

    for ( int i = m_iCurrentEntity; i < MAX_EDICTS; ++i )
    {
        fast_edict_index_t t = { EItemTypeOther, 0 };
        m_aEdictsIndexes[ i ] = t;

        edict_t* pEdict = CBotrixPlugin::pEngineServer->PEntityOfEntIndex(i);
        if ( (pEdict == NULL) || pEdict->IsFree() )
            continue;

        // Check items and objects.
		CheckNewEntity(pEdict, bLog);
    }
    m_bMapLoaded = true;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::Update()
{
#ifdef BOTRIX_SOURCE_ENGINE_2006
    // Source engine 2007 uses IServerPluginCallbacks::OnEdictAllocated instead of checking all array of edicts.

    // Update weapons we have in items array.
    good::vector<CItem>& aWeapons = m_aItems[EItemTypeWeapon];
    for ( TItemIndex i = 0; i < aWeapons.size(); ++i )
    {
        CItem& cEntity = aWeapons[i];
        edict_t* pEdict = cEntity.pEdict;
        if ( pEdict == NULL )
        {
            m_iFreeIndex[EItemTypeWeapon] = i;
            continue;
        }

        IServerEntity* pServerEntity = pEdict->GetIServerEntity();
        if ( pEdict->IsFree() || (pServerEntity == NULL) )
        {
            cEntity.pEdict = NULL;
            m_iFreeIndex[EItemTypeWeapon] = i;
            m_aUsedItems.reset( CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict) );
        }
        else if ( IsEntityTaken(pServerEntity) ) // Weapon still belongs to some player.
            FLAG_SET(FTaken, cEntity.iFlags);
        else if ( FLAG_ALL_SET_OR_0(FTaken, cEntity.iFlags) && IsEntityOnMap(pServerEntity) )
        {
            FLAG_CLEAR(FTaken, cEntity.iFlags);
            cEntity.vOrigin = cEntity.CurrentPosition();
            cEntity.iWaypoint = CWaypoints::GetNearestWaypoint( cEntity.vOrigin, NULL, true, CItem::iMaxDistToWaypoint );
        }
    }

    // 0 is world, 1..max players are players. Other entities are from indexes above max players.
    int iCount = CBotrixPlugin::pEngineServer->GetEntityCount();

    // Update weapon entities on map.
    int iTo = m_iCurrentEntity + m_iCheckEntitiesPerFrame;
    if ( iTo > m_iMaxEntityIndex )
        iTo = iCount;

    for ( TItemIndex i = m_iCurrentEntity; i < iTo; ++i )
    {
        if ( m_aUsedItems.test(i) )
            continue;

        edict_t* pEdict = CBotrixPlugin::pEngineServer->PEntityOfEntIndex(i);
        if ( (pEdict == NULL) || pEdict->IsFree() )
            continue;

        // Check only server entities.
        IServerEntity* pServerEntity = pEdict->GetIServerEntity();
        if ( pServerEntity == NULL )
            continue;

        // Check only for new weapons, because new weapon instance is created when weapon is picked up.
        CItemClass* pWeaponClass;
        TItemType iEntityType = GetEntityType(pEdict->GetClassName(), pWeaponClass, EItemTypeWeapon, EItemTypeWeapon+1);

        if ( iEntityType == EItemTypeWeapon )
            AddItem( EItemTypeWeapon, pEdict, pWeaponClass, pServerEntity );
    }
    m_iCurrentEntity = (iTo == iCount) ? CPlayers::Size()+1: iTo;
#else
    for ( int i = 0; i < m_aNewEntities.size(); ++i )
        CheckNewEntity( m_aNewEntities[i] );
    m_aNewEntities.clear();
#endif // BOTRIX_SOURCE_ENGINE_2006
}

//----------------------------------------------------------------------------------------------------------------
good::vector<TItemId>::iterator LocateObjectFlags( good::vector<TItemId>& aFlags, TItemId iObject )
{
    for ( good::vector<TItemId>::iterator it = aFlags.begin(); it != aFlags.end(); it += 2 )
        if ( *it == iObject )
            return it;
    return aFlags.end();
}

bool CItems::GetObjectFlags( TItemId iObject, TItemFlags& iFlags )
{
    good::vector<TItemId>::const_iterator it = LocateObjectFlags( m_aObjectFlags, iObject );
    if ( it == m_aObjectFlags.end() )
        return false;
    iFlags = *( ++it );
    return true;
}

bool CItems::SetObjectFlags( TItemId iObject, TItemFlags iFlags )
{
    GoodAssert( 0 <= iObject && iObject < MAX_EDICTS );
    if ( m_aEdictsIndexes[ iObject ].iItemType != EItemTypeObject )
        return false;
    
    int iIndex = m_aEdictsIndexes[ iObject ].iItemIndex;
    m_aItems[ EItemTypeObject ][ iIndex ].iFlags = iFlags;

    good::vector<TItemId>::iterator it = LocateObjectFlags( m_aObjectFlags, iObject );
    if ( it == m_aObjectFlags.end() )
    {
        m_aObjectFlags.push_back( iObject );
        m_aObjectFlags.push_back( iFlags );
    }
    else
        *(++it) = iFlags;

    return true;
}

//----------------------------------------------------------------------------------------------------------------
TItemType CItems::GetEntityType( const char* szClassName, CItemClass* & pEntityClass, TItemType iFrom, TItemType iTo )
{
    GoodAssert( iTo < EItemTypeAll );
    for ( TItemType iEntityType = iFrom; iEntityType < iTo; ++iEntityType )
    {
        const good::list<CItemClass>& aItemClasses = m_aItemClasses[iEntityType];

        for ( good::list<CItemClass>::iterator it = aItemClasses.begin(); it != aItemClasses.end(); ++it )
        {
            CItemClass& cEntityClass = *it;
			GoodAssert( cEntityClass.sClassName.size() > 0 );

			if ( cEntityClass.szEngineName && (cEntityClass.szEngineName == szClassName) ) // Compare by pointer, faster.
			{
				pEntityClass = &cEntityClass;
				return iEntityType;
			}
			else if ( cEntityClass.sClassName == szClassName ) // Full string content comparison.
			{
				cEntityClass.szEngineName = szClassName; // Save engine string.
				pEntityClass = &cEntityClass;
				return iEntityType;
			}
        }
    }

    return EItemTypeOther;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::CheckNewEntity(edict_t* pEdict, bool bLog)
{
    const char* szClassName = pEdict->GetClassName();
    if ( szClassName == NULL || szClassName[ 0 ] == 0 )
        return;

    int iEntIndex = pEdict->m_EdictIndex;

    if ( bLog )
        BLOG_T( "New entity: %s, index %d.", szClassName ? szClassName : "", iEntIndex );
    m_iMaxEntityIndex = MAX2(iEntIndex, m_iMaxEntityIndex);

    // Check only server entities.
    IServerEntity* pServerEntity = pEdict->GetIServerEntity();
    if ( pServerEntity == NULL )
        return;

    CItemClass* pItemClass;
    TItemType iEntityType = GetEntityType(szClassName, pItemClass, 0, EItemTypeKnownTotal);
    if ( iEntityType == EItemTypeOther )
    {
        fast_edict_index_t t = { EItemTypeOther, (unsigned short)m_aOthers.size() };
        m_aEdictsIndexes[ iEntIndex ] = t;
        m_aOthers.push_back(pEdict);
    }
    else if ( iEntityType == EItemTypeObject )
        AddObject( pEdict, pItemClass, pServerEntity );
    else
    {
        TItemIndex iIndex = AddItem( iEntityType, pEdict, pItemClass, pServerEntity );
        BASSERT(iIndex >= 0, return);
        CItem& cItem = m_aItems[iEntityType][iIndex];

		if (bLog)
			BLOG_D("New item: %s %d (%s), waypoint %d (%s).", CTypeToString::EntityTypeToString(iEntityType).c_str(), iIndex,
			pEdict->GetClassName(), cItem.iWaypoint,
			CWaypoints::IsValid(cItem.iWaypoint)
			? CTypeToString::WaypointFlagsToString(CWaypoints::Get(cItem.iWaypoint).iFlags).c_str()
			: "");

        if (  !m_bMapLoaded && FLAG_CLEARED(FTaken, cItem.iFlags) ) // Item should have waypoint near.
        {
            if ( !CWaypoint::IsValid(cItem.iWaypoint) )
            {
                const Vector& vOrigin = cItem.CurrentPosition();
				if (bLog)
					BLOG_W( "Warning: entity %s %d (%.0f %.0f %.0f) doesn't have waypoint close.", pEdict->GetClassName(), iIndex+1,
						    vOrigin.x, vOrigin.y, vOrigin.z );
                TWaypointId iWaypoint = CWaypoints::GetNearestWaypoint( cItem.vOrigin );
                if ( CWaypoint::IsValid(iWaypoint) && bLog)
                    BLOG_W("  Nearest waypoint %d.", iWaypoint);
            }
            else if ( bLog && iEntityType == EItemTypeDoor && !CWaypoint::IsValid((TWaypointId)((intptr_t)cItem.pArguments)) )
                BLOG_W("Door %d doesn't have 2 waypoints near.", iIndex);
        }

#ifdef BOTRIX_SOURCE_ENGINE_2006
        // Weapon entities are allocated / deallocated when respawned / owner killed.
        if ( iEntityType != EItemTypeWeapon )
#endif
        {
            int iIndex = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict);
            GoodAssert( iIndex > 0 ); // Valve should not allow this assert.
            m_aUsedItems.set(iIndex);
        }
    }

}


//----------------------------------------------------------------------------------------------------------------
TItemIndex CItems::NewEntityIndex( int iEntityType )
{
    good::vector<CItem>& aItems = m_aItems[iEntityType];

    if ( m_bMapLoaded ) // Check if there are free space in items array.
    {
        if ( m_iFreeIndex[iEntityType] != -1 )
        {
            int iIndex = m_iFreeIndex[iEntityType];
            m_iFreeIndex[iEntityType] = -1;
            return iIndex;
        }

        for ( int i = 0; i < aItems.size(); ++i ) // TODO: add free count.
            if ( aItems[ i ].pEdict == NULL )
                return i;
    }

    return aItems.size();
}

//----------------------------------------------------------------------------------------------------------------
void CItems::AutoWaypointPathFlagsForEntity( TItemType iEntityType, TItemIndex iIndex, CItem& cEntity )
{
    TWaypointId iWaypoint = cEntity.iWaypoint;
    if ( (iEntityType == EItemTypeButton) && (iWaypoint != EWaypointIdInvalid) )
    {
        // Set waypoint argument to button.
        CWaypoint& cWaypoint = CWaypoints::Get(iWaypoint);
        FLAG_SET(FWaypointButton, cWaypoint.iFlags);
        CWaypoint::SetButton( iIndex, cWaypoint.iArgument );
    }

    // Check 2nd nearest waypoint for door.
    if ( iEntityType == EItemTypeDoor )
    {
        if ( iWaypoint != EWaypointIdInvalid )
        {
            good::bitset cOmitWaypoints(CWaypoints::Size());
            cOmitWaypoints.set(iWaypoint);
            iWaypoint = CWaypoints::GetNearestWaypoint( cEntity.vOrigin, &cOmitWaypoints, true, CItem::iMaxDistToWaypoint );
        }
        cEntity.pArguments = (void*)(intptr_t)iWaypoint;

        // Set door for paths between these two waypoints.
        if ( iWaypoint != EWaypointIdInvalid )
        {
            CWaypointPath* pPath = CWaypoints::GetPath(iWaypoint, cEntity.iWaypoint); // From -> To.
            if ( pPath )
            {
                FLAG_SET(FPathDoor, pPath->iFlags);
				pPath->SetDoorNumber( iIndex );
            }

            pPath = CWaypoints::GetPath(cEntity.iWaypoint, iWaypoint); // To -> From.
            if ( pPath )
            {
                FLAG_SET(FPathDoor, pPath->iFlags);
				pPath->SetDoorNumber( iIndex );
			}
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
TItemIndex CItems::AddItem( TItemType iEntityType, edict_t* pEdict, CItemClass* pItemClass, IServerEntity* pServerEntity )
{
    GoodAssert( (0 <= iEntityType) && (iEntityType < EItemTypeAll) );

    ICollideable* pCollidable = pServerEntity->GetCollideable();
    BASSERT( pCollidable, return -1 );

    const Vector& vItemOrigin = pCollidable->GetCollisionOrigin();

    float fPickupDistanceSqr = pItemClass->fPickupDistanceSqr;
    if ( fPickupDistanceSqr < 1.0f )
    {
        // Calculate object radius.
        float fMaxsRadiusSqr = pCollidable->OBBMaxs().LengthSqr();
        float fMinsRadiusSqr = pCollidable->OBBMins().LengthSqr();
        fPickupDistanceSqr = FastSqrt( MAX2(fMinsRadiusSqr, fMaxsRadiusSqr) );
        if ( iEntityType < EItemTypeCanPickTotal )
        {
            fPickupDistanceSqr += CMod::iPlayerRadius + CMod::iItemPickUpDistance;
            fPickupDistanceSqr *= fPickupDistanceSqr * 2;
        }
        else
            fPickupDistanceSqr = Sqr( fPickupDistanceSqr + CMod::iPlayerRadius );
        pItemClass->fPickupDistanceSqr = fPickupDistanceSqr;
    }

    int iFlags = pItemClass->iFlags;
    TWaypointId iWaypoint = -1;

    if ( !IsEntityOnMap(pServerEntity) || IsEntityTaken(pServerEntity) )
        FLAG_SET(FTaken, iFlags);
    else
		iWaypoint = CWaypoints::GetNearestWaypoint( vItemOrigin, NULL, true, CItem::iMaxDistToWaypoint );

    TItemIndex iIndex = NewEntityIndex( iEntityType );
    fast_edict_index_t t = { (unsigned short)iEntityType, (unsigned short)iIndex };
    m_aEdictsIndexes[ pEdict->m_EdictIndex ] = t;

    CItem cNewEntity(pEdict, iFlags, fPickupDistanceSqr, pItemClass, vItemOrigin, iWaypoint);
    AutoWaypointPathFlagsForEntity( iEntityType, iIndex, cNewEntity );

    good::vector<CItem>& aItems = m_aItems[ iEntityType ];
    if ( iIndex < aItems.size() )
        aItems[ iIndex ] = cNewEntity;
    else
        aItems.push_back( cNewEntity );

    return iIndex;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::AddObject( edict_t* pEdict, const CItemClass* pObjectClass, IServerEntity* pServerEntity )
{
    // Calculate object radius.
    ICollideable* pCollidable = pServerEntity->GetCollideable();

    // TODO: why not use object class to get distance?
    float fMaxsRadiusSqr = pCollidable->OBBMaxs().LengthSqr();
    float fMinsRadiusSqr = pCollidable->OBBMins().LengthSqr();
    fMaxsRadiusSqr = FastSqrt( MAX2(fMaxsRadiusSqr, fMinsRadiusSqr) );
    fMaxsRadiusSqr += CMod::iPlayerRadius;
    fMaxsRadiusSqr *= fMaxsRadiusSqr;
    
    TItemIndex iIndex = NewEntityIndex( EItemTypeObject );
    fast_edict_index_t t = { EItemTypeObject, (unsigned short)iIndex };
    m_aEdictsIndexes[pEdict->m_EdictIndex] = t;

    TItemFlags iFlags = pObjectClass->iFlags;

    // Get object flags (saved in waypoints).
    bool bHasFlags = GetObjectFlags( pEdict->m_EdictIndex, iFlags );
    if ( !bHasFlags )
    {
        // Get flags from model name.
        if ( m_aObjectFlagsForModels.size() )
        {
            good::string sModel = STRING( pServerEntity->GetModelName() );
            for ( int i = 0; i < m_aObjectFlagsForModels.size(); ++i )
                if ( sModel.find( m_aObjectFlagsForModels[ i ].first ) != good::string::npos )
                {
                    FLAG_SET( m_aObjectFlagsForModels[ i ].second, iFlags );
                    break;
                }
        }
    }

    CItem cObject( pEdict, iFlags, fMaxsRadiusSqr, pObjectClass, pCollidable->GetCollisionOrigin(), -1 );

    good::vector<CItem>& aItems = m_aItems[ EItemTypeObject ];
    if ( iIndex < aItems.size() )
        aItems[ iIndex ] = cObject;
    else
        aItems.push_back( cObject );

}

//----------------------------------------------------------------------------------------------------------------
int CItems::IsDoorClosed( TItemIndex iDoor )
{
    BASSERT( 0 <= iDoor && iDoor < m_aItems[EItemTypeDoor].size(), return false );
	const CItem& cDoor = m_aItems[EItemTypeDoor][iDoor];
	TWaypointId w1 = cDoor.iWaypoint, w2 = (TWaypointId)(intptr_t)cDoor.pArguments;

	if ( !CWaypoints::IsValid( w1 ) || !CWaypoints::IsValid( w2 ) ) // Door should have two waypoints from each side.
		return -1;

    const Vector& v1 = CWaypoints::Get(w1).vOrigin;
    const Vector& v2 = CWaypoints::Get(w2).vOrigin;
    return CUtil::IsRayHitsEntity(cDoor.pEdict, v1, v2) ? 1 : 0;
}

//----------------------------------------------------------------------------------------------------------------
void CItems::Draw( CClient* pClient )
{
    static float fNextDrawTime = 0.0f;
    static unsigned char pvs[MAX_MAP_CLUSTERS/8];

    if ( (pClient->iItemDrawFlags == FItemDontDraw) || (pClient->iItemTypeFlags == 0) || (CBotrixPlugin::fTime < fNextDrawTime) )
        return;

    fNextDrawTime = CBotrixPlugin::fTime + 1.0f;

    int iClusterIndex = CBotrixPlugin::pEngineServer->GetClusterForOrigin( pClient->GetHead() );
    CBotrixPlugin::pEngineServer->GetPVSForCluster( iClusterIndex, sizeof(pvs), pvs );

    Vector vHead = pClient->GetHead();
    CUtil::SetPVSForVector( vHead );
    
    for ( TItemType iEntityType = 0; iEntityType < EItemTypeAll; ++iEntityType )
    {
        if ( !FLAG_SOME_SET(1<<iEntityType, pClient->iItemTypeFlags) ) // Don't draw items of disabled item type.
            continue;

        int iSize = (iEntityType == EItemTypeOther) ? m_aOthers.size() : m_aItems[iEntityType].size();
        for ( int i = 0; i < iSize; ++i )
        {
            edict_t* pEdict = (iEntityType == EItemTypeOther) ? m_aOthers[i] : m_aItems[iEntityType][i].pEdict;

            if ( (pEdict == NULL) || pEdict->IsFree() )
                continue;

            IServerEntity* pServerEntity = pEdict->GetIServerEntity();
            BASSERT( pServerEntity, continue );

            if ( IsEntityTaken(pServerEntity) )
                continue;

            ICollideable* pCollide = pServerEntity->GetCollideable();
            const Vector& vOrigin = pCollide->GetCollisionOrigin();

            if ( CUtil::IsVisiblePVS(vOrigin) && CUtil::IsVisible(vHead, vOrigin, EVisibilityWorld, false ) )
            {
                const CItem* pEntity = (iEntityType == EItemTypeOther) ? NULL : &m_aItems[iEntityType][i];

                if ( FLAG_SOME_SET(FItemDrawStats, pClient->iItemDrawFlags) )
                {
                    int pos = 0;

                    // Draw entity class name name with index.
                    sprintf( szMainBuffer, "%s %d (id %d)", CTypeToString::EntityTypeToString( iEntityType ).c_str(), i, pEdict->m_EdictIndex );

                    CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, szMainBuffer );

                    CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, pEdict->GetClassName() );
					if ( iEntityType == EItemTypeDoor )
					{
						int iClosed = IsDoorClosed( i );
						CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, iClosed == -1 ? "no near waypoints" : iClosed == 0 ? "opened" : "closed" );
					}
					else if ( iEntityType == EItemTypeObject )
                    {
                        sprintf( szMainBuffer, "%s %d", CTypeToString::EntityTypeToString(iEntityType).c_str(), i );
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityOnMap(pServerEntity) ? "alive" : "dead" );
                        //CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityTaken(pServerEntity) ? "taken" : "not taken" ); // Taken not shown.
                        //CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, CanPickupEntity(pServerEntity) ? "moveable" : "static" ); // Taken not shown.
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityBreakable(pServerEntity) ? "breakable" : "non breakable" );
                        const char* sFlags = CTypeToString::EntityClassFlagsToString( pEntity->iFlags, false ).c_str();
                        if ( sFlags[0] != 0 )
                            CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, sFlags);
                    }

                    if ( iEntityType >= EItemTypeObject ) // Draw object model.
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, STRING( pEdict->GetIServerEntity()->GetModelName() ) );
                }

                // Draw box around item.
                if ( FLAG_SOME_SET(FItemDrawBoundBox, pClient->iItemDrawFlags) )
                    CUtil::DrawBox(vOrigin, pCollide->OBBMins(), pCollide->OBBMaxs(), 1.0f, 0xFF, 0xFF, 0xFF, pCollide->GetCollisionAngles());

                if ( FLAG_SOME_SET(FItemDrawWaypoint, pClient->iItemDrawFlags) && (iEntityType < EItemTypeObject) )
                {
                    // Draw nearest waypoint from item, yellow.
                    if (CWaypoint::IsValid(pEntity->iWaypoint) )
                        CUtil::DrawLine(CWaypoints::Get(pEntity->iWaypoint).vOrigin, vOrigin, 1.0f, 0xFF, 0xFF, 0);

                    // Draw second waypoint for door, yellow.
                    if ( (iEntityType == EItemTypeDoor) && CWaypoint::IsValid( (TWaypointId)(intptr_t)pEntity->pArguments ) )
                        CUtil::DrawLine(CWaypoints::Get((TWaypointId)(intptr_t)pEntity->pArguments).vOrigin, vOrigin, 1.0f, 0xFF, 0xFF, 0);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
//void CItems::WaypointDeleted( TWaypointId id )
//{
//    for ( int iEntityType = 0; iEntityType < EItemTypeTotal; ++iEntityType )
//    {
//        good::vector<CItem>& aItems = m_aItems[iEntityType];
//
//        for ( int i = 0; i < aItems.size(); ++i )
//        {
//            CItem& cItem = aItems[i];
//            if ( cItem.iWaypoint == id )
//                cItem.iWaypoint = CWaypoints::GetNearestWaypoint( cItem.vOrigin );
//            else if ( cItem.iWaypoint > id )
//                --cItem.iWaypoint;
//
//            // TODO: update doors.
//            //if ( iEntityType == EItemTypeDoor )
//        }
//    }
//}
