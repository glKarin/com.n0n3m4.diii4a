#include "clients.h"
#include "item.h"
#include "server_plugin.h"
#include "type2string.h"
#include "waypoint.h"

#include <good/string_buffer.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
extern char* szMainBuffer;
extern int iMainBufferSize;


//----------------------------------------------------------------------------------------------------------------
// Waypoints file header.
//----------------------------------------------------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)
struct waypoint_header
{
    int          szFileType;
    char         szMapName[64];
    int          iVersion;
    int          iNumWaypoints;
    int          iFlags;
};
#pragma pack(pop)

static const char* WAYPOINT_FILE_HEADER_ID     = "BtxW";   // Botrix's Waypoints.

static const int WAYPOINT_VERSION              = 1;        // Waypoints file version.
static const int WAYPOINT_FILE_FLAG_VISIBILITY = 1<<0;     // Flag for waypoint visibility table.
static const int WAYPOINT_FILE_FLAG_AREAS      = 1<<1;     // Flag for area names.

static const int WAYPOINT_VERSION_FLAGS_SHORT  = 1;        // Flags was short (16bits) instead of int (32bits).

//----------------------------------------------------------------------------------------------------------------
// CWaypoint static members.
//----------------------------------------------------------------------------------------------------------------
int CWaypoint::iWaypointTexture = -1;
int CWaypoint::iDefaultDistance = 144;

int CWaypoint::iUnreachablePathFailuresToDelete = 4;

int CWaypoint::iAnalyzeDistance = 96;
int CWaypoint::iWaypointsMaxCountToAnalyzeMap = 64;

#if defined(DEBUG) || defined(_DEBUG)
float CWaypoint::fAnalyzeWaypointsPerFrame = 1;
#else
float CWaypoint::fAnalyzeWaypointsPerFrame = 0.25f;
#endif

bool CWaypoint::bSaveOnMapChange = false;
bool CWaypoint::bShowAnalyzePotencialWaypoints = false;
bool CWaypoint::bAnalyzeTraceAll = false;


const TWaypointFlags CWaypoint::m_aFlagsForEntityType[ EItemTypeCanPickTotal ] =
{
    FWaypointHealth | FWaypointHealthMachine,
    FWaypointArmor | FWaypointArmorMachine,
    FWaypointWeapon,
    FWaypointAmmo,
};


StringVector CWaypoints::m_aAreas;
CWaypoints::WaypointGraph CWaypoints::m_cGraph;
float CWaypoints::fNextDrawWaypointsTime = 0.0f;
CWaypoints::Bucket CWaypoints::m_cBuckets[CWaypoints::BUCKETS_SIZE_X][CWaypoints::BUCKETS_SIZE_Y][CWaypoints::BUCKETS_SIZE_Z];

good::vector< good::bitset > CWaypoints::m_aVisTable;
bool CWaypoints::bValidVisibilityTable = false;

good::vector<CWaypoints::unreachable_path_t> CWaypoints::m_aUnreachablePaths;

// Sometimes the analyze doesn't add waypoints in small passages, so we try to add waypoints at inter-position between waypoint 
// analyzed neighbours. Waypoint analyzed neighbours are x's. W = analyzed waypoint.
//
// x - x - x
// |   |   |
// x - W - x
// |   |   |
// x - x - x
//
// '-' and '|' on the picture will be evaluated in 'inters' step, when the adjacent waypoints are not set for some reason (like hit wall).
good::vector<CWaypoints::CNeighbour> CWaypoints::m_aWaypointsNeighbours;
good::vector<Vector> CWaypoints::m_aWaypointsToAddOmitInAnalyze[ CWaypoints::EAnalyzeWaypointsTotal ];

CWaypoints::TAnalyzeStep CWaypoints::m_iAnalyzeStep = EAnalyzeStepTotal;
float CWaypoints::m_fAnalyzeWaypointsForNextFrame = 0;
edict_t* CWaypoints::m_pAnalyzer = NULL;
bool CWaypoints::m_bIsAnalyzeStepAddedWaypoints = false;
TWaypointId CWaypoints::m_iCurrentAnalyzeWaypoint = 0;

static const float ANALIZE_HELP_IF_LESS_WAYPOINTS_PER_FRAME = 0.0011;

//----------------------------------------------------------------------------------------------------------------
void CWaypoint::GetColor(unsigned char& r, unsigned char& g, unsigned char& b) const
{
    if ( FLAG_SOME_SET(FWaypointStop, iFlags) )
    {
        r = 0x00; g = 0x00; b = 0xFF;  // Blue effect, stop.
    }
    else if ( FLAG_SOME_SET(FWaypointCamper | FWaypointSniper, iFlags) )
    {
        r = 0x66; g = 0x00; b = 0x00;  // Dark red effect, camper / sniper.
    }
    else if ( FLAG_SOME_SET(FWaypointLadder, iFlags) )
    {
        r = 0xFF; g = 0x00; b = 0x00;  // Red effect, ladder.
    }
    else if ( FLAG_SOME_SET(FWaypointWeapon, iFlags) )
    {
        r = 0xFF; g = 0xFF; b = 0x00;  // Light yellow effect, weapon.
    }
    else if ( FLAG_SOME_SET(FWaypointAmmo, iFlags) )
    {
        r = 0x66; g = 0x66; b = 0x00;  // Dark yellow effect, ammo.
    }
    else if ( FLAG_SOME_SET(FWaypointHealth, iFlags) )
    {
        r = 0xFF; g = 0xFF; b = 0xFF;  // Light white effect, health.
    }
    else if ( FLAG_SOME_SET(FWaypointHealthMachine, iFlags) )
    {
        r = 0x66; g = 0x66; b = 0x66;  // Gray effect, health machine.
    }
    else if ( FLAG_SOME_SET(FWaypointArmor, iFlags) )
    {
        r = 0x00; g = 0xFF; b = 0x00;  // Light green effect, armor.
    }
    else if ( FLAG_SOME_SET(FWaypointArmorMachine, iFlags) )
    {
        r = 0x00; g = 0x66; b = 0x00;  // Dark green effect, armor machine.
    }
    else if ( FLAG_SOME_SET(FWaypointButton | FWaypointSeeButton, iFlags) )
    {
        r = 0x8A; g = 0x2B; b = 0xE2;  // Violet effect, button.
    }
    else
    {
        r = 0x00; g = 0xFF; b = 0xFF;  // Cyan effect, other flags.
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoint::Draw( TWaypointId iWaypointId, TWaypointDrawFlags iDrawType, float fDrawTime ) const
{
    unsigned char r, g, b; // Red, green, blue.
    GetColor(r, g, b);

    Vector vEnd = Vector(vOrigin.x, vOrigin.y, vOrigin.z - CMod::GetVar( EModVarPlayerEye ));

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawBeam, iDrawType) )
        CUtil::DrawBeam(vOrigin, vEnd, WIDTH, fDrawTime, r, g, b);

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawLine, iDrawType) )
        CUtil::DrawLine(vOrigin, vEnd, fDrawTime, r, g, b);

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawBox, iDrawType) )
    {
        Vector vBoxOrigin(vOrigin.x - CMod::GetVar( EModVarPlayerWidth )/2, vOrigin.y - CMod::GetVar( EModVarPlayerWidth )/2, vOrigin.z - CMod::GetVar( EModVarPlayerEye ));
        CUtil::DrawBox(vBoxOrigin, CUtil::vZero, CMod::vPlayerCollisionHull, fDrawTime, r, g, b);
    }

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawText, iDrawType) )
    {
        sprintf(szMainBuffer, "%d", iWaypointId);
        Vector v = vOrigin;
        v.z -= 20.0f;
        int i = 0;
		CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, szMainBuffer );
        if ( iAreaId != 0 )
            CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, CWaypoints::GetAreas()[ iAreaId ].c_str() );
		if ( FLAG_SOME_SET( FWaypointButton | FWaypointSeeButton, iFlags ) )
		{
			sprintf( szMainBuffer, FLAG_SOME_SET( FWaypointSeeButton, iFlags ) ? "see button %d" : "button %d", 
			         CWaypoint::GetButton( iArgument ) );
			CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, szMainBuffer );
			sprintf( szMainBuffer, FLAG_SOME_SET( FWaypointElevator, iFlags ) ? "for elevator %d" : "for door %d",
			         CWaypoint::GetDoor( iArgument ) );
			CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, szMainBuffer );
		}
		else
			CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, CTypeToString::WaypointFlagsToString( iFlags, false ).c_str() );
    }
}


//********************************************************************************************************************
void CWaypoints::Clear()
{
    CPlayers::InvalidatePlayersWaypoints();
    ClearLocations();
    m_cGraph.clear();
    m_aAreas.clear();
    m_aAreas.push_back( "default" ); // New waypoints without area id will be put under this empty area id.
    m_aUnreachablePaths.clear();
    m_aVisTable.clear();
    bValidVisibilityTable = false;
}

//********************************************************************************************************************
bool CWaypoints::Save()
{
    const good::string& sFileName = CUtil::BuildFileName("waypoints", CBotrixPlugin::instance->sMapName, "way");

    FILE *f = CUtil::OpenFile(sFileName.c_str(), "wb");
    if (f == NULL)
        return false;

    waypoint_header header;
    header.iFlags = 0;
    FLAG_SET(WAYPOINT_FILE_FLAG_AREAS, header.iFlags);
    FLAG_SET(WAYPOINT_FILE_FLAG_VISIBILITY, header.iFlags);
    header.iNumWaypoints = m_cGraph.size();
    header.iVersion = WAYPOINT_VERSION;
    header.szFileType = *((int*)&WAYPOINT_FILE_HEADER_ID[0]);
    strncpy(header.szMapName, CBotrixPlugin::instance->sMapName.c_str(), sizeof(header.szMapName));

    // Write header.
    fwrite(&header, sizeof(waypoint_header), 1, f);

    // Write waypoints data.
    for (WaypointNodeIt it = m_cGraph.begin(); it != m_cGraph.end(); ++it)
    {
        fwrite(&it->vertex.vOrigin, sizeof(Vector), 1, f);
        fwrite(&it->vertex.iFlags, sizeof(TWaypointFlags), 1, f);
        fwrite(&it->vertex.iAreaId, sizeof(TAreaId), 1, f);
        fwrite(&it->vertex.iArgument, sizeof(TWaypointArgument), 1, f);
    }

    // Write waypoints neighbours.
    for (WaypointNodeIt it = m_cGraph.begin(); it != m_cGraph.end(); ++it)
    {
        int iNumPaths = it->neighbours.size();
        fwrite(&iNumPaths, sizeof(int), 1, f);

        for ( WaypointArcIt arcIt = it->neighbours.begin(); arcIt != it->neighbours.end(); ++arcIt)
        {
            fwrite(&arcIt->target, sizeof(TWaypointId), 1, f);             // Save waypoint id.
            fwrite(&arcIt->edge.iFlags, sizeof(TPathFlags), 1, f);         // Save path flags.
            fwrite(&arcIt->edge.iArgument, sizeof(TPathArgument), 1, f);   // Save path arguments.
        }
    }

    // Save area names.
    int iAreaNamesSize = m_aAreas.size();
    BASSERT( iAreaNamesSize >= 0, iAreaNamesSize=0 );

    fwrite(&iAreaNamesSize, sizeof(int), 1, f); // Save area names size.

    for ( int i=1; i < iAreaNamesSize; i++ ) // First area name is always empty, for new waypoints.
    {
        int iSize = m_aAreas[i].size();
        fwrite(&iSize, 1, sizeof(int), f); // Save string size.
        fwrite(m_aAreas[i].c_str(), 1, iSize+1, f); // Write string & trailing 0.
    }

    // Save waypoint visibility table.
    BLOG_I( "Saving waypoint visibility table, this may take a while." );
    m_aVisTable.resize( Size() );
    for ( TWaypointId i = 0; i < Size(); ++i )
    {
        good::bitset& cVisibles = m_aVisTable[i];
        cVisibles.resize( Size() );

        Vector vFrom = Get(i).vOrigin;
        CUtil::SetPVSForVector( vFrom );

        for ( TWaypointId j = 0; j < Size(); ++j )
        {
            if ( i < j )
            {
                Vector vTo = Get( j ).vOrigin;
                cVisibles.set( j, CUtil::IsVisiblePVS( vTo ) || CUtil::IsVisible( vFrom, vTo, EVisibilityWorld, false ) );
            }
            else
                cVisibles.set( j, (i == j) || m_aVisTable[j].test(i) );
        }
        fwrite( cVisibles.data(), 1, cVisibles.byte_size(), f );
    }

    // Save items marks.
    BLOG_I( "Saving items marks." );
    GoodAssert( sizeof( TItemIndex ) == sizeof( TItemFlags ) );
    const good::vector<TItemId>& aItems = CItems::GetObjectsFlags();
    int iSize = aItems.size() / 2;
    fwrite( &iSize, 1, sizeof( int ), f );
    for ( int i = 0; i < iSize; ++i )
    {
        TItemIndex iIndex = aItems[ i * 2 ] - CPlayers::Size(); // Items indexes are relatives to player's count.
        fwrite( &iIndex, 1, sizeof( TItemIndex ), f );
        fwrite( &aItems[ i * 2 + 1 ], 1, sizeof( TItemFlags ), f );
    }

    fclose(f);

    bValidVisibilityTable = true;

    return true;
}


//----------------------------------------------------------------------------------------------------------------
bool CWaypoints::Load()
{
    Clear();

    const good::string& sFileName = CUtil::BuildFileName("waypoints", CBotrixPlugin::instance->sMapName, "way");

    FILE *f = CUtil::OpenFile(sFileName, "rb");
    if ( f == NULL )
    {
        BLOG_W( "No waypoints for map %s:", CBotrixPlugin::instance->sMapName.c_str() );
        BLOG_W( "  File '%s' doesn't exists.", sFileName.c_str() );
        return false;
    }

    struct waypoint_header header;
    size_t iRead = fread(&header, 1, sizeof(struct waypoint_header), f);
    BASSERT(iRead == sizeof(struct waypoint_header), Clear();fclose(f);return false);

    if (*((int*)&WAYPOINT_FILE_HEADER_ID[0]) != header.szFileType)
    {
        BLOG_E("Error loading waypoints: invalid file header.");
        fclose(f);
        return false;
    }
    if ( (header.iVersion <= 0) || (header.iVersion > WAYPOINT_VERSION) )
    {
        BLOG_E( "Error loading waypoints, version mismatch:" );
        BLOG_E( "  File version %d, current waypoint version %d.", header.iVersion, WAYPOINT_VERSION );
        fclose(f);
        return false;
    }
    if ( CBotrixPlugin::instance->sMapName != header.szMapName )
    {
        BLOG_W( "Warning loading waypoints, map name mismatch:" );
        BLOG_W( "  File map %s, current map %s.", header.szMapName, CBotrixPlugin::instance->sMapName.c_str() );
    }

    Vector vOrigin;
    TWaypointFlags iFlags = 0;
    int iNumPaths = 0, iArgument = 0;
    TAreaId iAreaId = 0;

    int sizeOfFlags = header.iVersion <= WAYPOINT_VERSION_FLAGS_SHORT ? sizeof( short ) : sizeof( int );

    // Read waypoints information.
    for ( TWaypointId i = 0; i < header.iNumWaypoints; ++i )
    {
        iRead = fread(&vOrigin, 1, sizeof(Vector), f);
        BASSERT(iRead == sizeof(Vector), Clear();fclose(f);return false);

        iRead = fread(&iFlags, 1, sizeOfFlags, f);
        BASSERT(iRead == sizeOfFlags, Clear();fclose(f);return false);

        iRead = fread(&iAreaId, 1, sizeof(TAreaId), f);
        BASSERT(iRead == sizeof(TAreaId), Clear();fclose(f);return false);
        if ( FLAG_CLEARED(WAYPOINT_FILE_FLAG_AREAS, header.iFlags) )
            iAreaId = 0;

        iRead = fread( &iArgument, 1, sizeof( TWaypointArgument ), f );
        BASSERT( iRead == sizeof( TWaypointArgument ), Clear(); fclose( f ); return false );

        Add( vOrigin, iFlags, iArgument, iAreaId );
    }

    // Read waypoints paths.
    TWaypointId iPathTo;
    TPathFlags iPathFlags = 0;
    TPathArgument iPathArgument = 0;

    for ( TWaypointId i = 0; i < header.iNumWaypoints; ++i )
    {
        WaypointGraph::node_it from = m_cGraph.begin() + i;

        iRead = fread(&iNumPaths, 1, sizeof(int), f);
        BASSERT( (iRead == sizeof(int)) && (0 <= iNumPaths) && (iNumPaths < header.iNumWaypoints), Clear();fclose(f);return false );

        m_cGraph[i].neighbours.reserve(iNumPaths);

        for ( int n = 0; n < iNumPaths; n ++ )
        {
            iRead = fread(&iPathTo, 1, sizeof(int), f);
            BASSERT( (iRead == sizeof(int)) && (0 <= iPathTo) && (iPathTo < header.iNumWaypoints), Clear();fclose(f);return false );

            iRead = fread(&iPathFlags, 1, sizeOfFlags, f);
            BASSERT( iRead == sizeOfFlags, Clear();fclose(f);return false );

            iRead = fread(&iPathArgument, 1, sizeOfFlags, f);
            BASSERT( iRead == sizeOfFlags, Clear();fclose(f);return false );

            WaypointGraph::node_it to = m_cGraph.begin() + iPathTo;
            m_cGraph.add_arc( from, to, CWaypointPath(from->vertex.vOrigin.DistTo(to->vertex.vOrigin), iPathFlags, iPathArgument) );
        }
    }

    int iAreaNamesSize = 0;
    if ( FLAG_SOME_SET(WAYPOINT_FILE_FLAG_AREAS, header.iFlags) )
    {
        // Read area names.
        iRead = fread(&iAreaNamesSize, 1, sizeof(int), f);
        BASSERT( iRead == sizeof(int), Clear();fclose(f);return false);
        //BASSERT( (0 <= iAreaNamesSize) && (iAreaNamesSize <= header.iNumWaypoints), Clear();fclose(f);return false );

        m_aAreas.reserve(iAreaNamesSize);
        m_aAreas.push_back("default"); // New waypoints without area id will be put under this empty area id.

        for ( int i=1; i < iAreaNamesSize; i++ )
        {
            int iStrSize;
            iRead = fread(&iStrSize, 1, sizeof(int), f);
            BASSERT(iRead == sizeof(int), Clear();fclose(f);return false);

            BASSERT(0 < iStrSize && iStrSize < iMainBufferSize, Clear(); return false);
            if ( iStrSize > 0 )
            {
                iRead = fread(szMainBuffer, 1, iStrSize+1, f); // Read also trailing 0.
                BASSERT(iRead == iStrSize+1, Clear();fclose(f);return false);

                good::string sArea(szMainBuffer, true, true, iStrSize);
                m_aAreas.push_back(sArea);
            }
        }
    }
    else
        m_aAreas.push_back("default"); // New waypoints without area id will be put under this empty area id.

    // Check for areas names.
    iAreaNamesSize = m_aAreas.size();
    for ( TWaypointId i = 0; i < header.iNumWaypoints; ++i )
    {
        if ( m_cGraph[i].vertex.iAreaId >= iAreaNamesSize )
        {
            BreakDebugger();
            m_cGraph[i].vertex.iAreaId = 0;
        }
    }

    bValidVisibilityTable = header.iFlags & WAYPOINT_FILE_FLAG_VISIBILITY;
    if ( bValidVisibilityTable )
    {
        m_aVisTable.resize( header.iNumWaypoints );

        for ( TWaypointId i = 0; i < Size(); ++i )
        {
            m_aVisTable[i].resize( header.iNumWaypoints );
            int iByteSize = m_aVisTable[i].byte_size();
            iRead = fread( m_aVisTable[i].data(), 1, iByteSize, f );
            if ( iRead != iByteSize )
            {
                BLOG_E( "Invalid waypoints visibility table." );
                Clear();
                fclose(f);
                return false;
            }
        }
        BLOG_I( "Waypoints visibility table loaded." );
    }
    else
        BLOG_W( "No waypoints visibility table in file." );

    // Load items marks.
    int iItemsCount = 0;
    if ( fread( &iItemsCount, 1, sizeof( int ), f ) == sizeof( int ) )
    {
        good::vector<TItemId> aItems;
        aItems.resize( iItemsCount * 2 );
        iRead = fread( &aItems[0], 1, iItemsCount * 2 * sizeof( TItemIndex ), f );
        BASSERT( iRead == iItemsCount * 2 * (int)sizeof( TItemIndex ), fclose( f ); return true; );

        for ( int i = 0; i < iItemsCount; ++i )
        {
            aItems[ i * 2 ] += CPlayers::Size();
            BLOG_D( "Object id %d: %s.", aItems[ i * 2 ], CTypeToString::EntityClassFlagsToString( aItems[ i * 2 + 1 ] ).c_str() );
        }
        CItems::SetObjectsFlags( aItems );

        BLOG_I( "Loaded %d object marks.", iItemsCount );
    }

    fclose(f);

    return true;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetRandomNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible )
{
    const WaypointNode::arcs_t& aNeighbours = GetNode(iWaypoint).neighbours;
    if ( !aNeighbours.size() )
        return EWaypointIdInvalid;

    TWaypointId iResult = rand() % aNeighbours.size();
    if ( bValidVisibilityTable && CWaypoint::IsValid(iTo) )
    {
        for ( int i = 0; i < aNeighbours.size(); ++i )
        {
            TWaypointId iNeighbour = aNeighbours[iResult].target;
            if ( m_aVisTable[iNeighbour].test(iTo) == bVisible )
                return iNeighbour;
            if ( ++iResult == aNeighbours.size() )
                iResult = 0;
        }
    }
    return aNeighbours[iResult].target;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetNearestNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible )
{
    GoodAssert( bValidVisibilityTable );
    const WaypointNode::arcs_t& aNeighbours = GetNode(iWaypoint).neighbours;
    if ( !aNeighbours.size() )
        return EWaypointIdInvalid;

    Vector vTo = Get(iTo).vOrigin;

    TWaypointId iResult = aNeighbours[0].target;
    float fMinDist = Get(iResult).vOrigin.DistToSqr(vTo);
    bool bResultVisibleOk = (m_aVisTable[iResult].test(iTo) == bVisible);

    for ( int i = 1; i < aNeighbours.size(); ++i )
    {
        TWaypointId iNeighbour = aNeighbours[i].target;
        bool bVisibleNeighbourOk = (m_aVisTable[iNeighbour].test(iTo) == bVisible);
        if ( bResultVisibleOk && !bVisibleNeighbourOk )
            continue;

        Vector& vOrigin = Get(iNeighbour).vOrigin;
        float fDist = vOrigin.DistToSqr(vTo);
        if ( fDist < fMinDist )
        {
            bVisible = bVisibleNeighbourOk;
            fMinDist = fDist;
            iResult = iNeighbour;
        }
    }
    return iResult;
}

//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetFarestNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible )
{
    GoodAssert( bValidVisibilityTable );
    const WaypointNode::arcs_t& aNeighbours = GetNode(iWaypoint).neighbours;
    if ( !aNeighbours.size() )
        return EWaypointIdInvalid;

    Vector vTo = Get(iTo).vOrigin;

    TWaypointId iResult = aNeighbours[0].target;
    float fMaxDist = Get(iResult).vOrigin.DistToSqr(vTo);
    bool bResultVisibleOk = (m_aVisTable[iResult].test(iTo) == bVisible);

    for ( int i = 1; i < aNeighbours.size(); ++i )
    {
        TWaypointId iNeighbour = aNeighbours[i].target;
        bool bVisibleNeighbourOk = (m_aVisTable[iNeighbour].test(iTo) == bVisible);
        if ( bResultVisibleOk && !bVisibleNeighbourOk )
            continue;

        Vector& vOrigin = Get(iNeighbour).vOrigin;
        float fDist = vOrigin.DistToSqr(vTo);
        if ( fDist > fMaxDist )
        {
            bVisible = bVisibleNeighbourOk;
            fMaxDist = fDist;
            iResult = iNeighbour;
        }
    }
    return iResult;
}


//----------------------------------------------------------------------------------------------------------------
CWaypointPath* CWaypoints::GetPath(TWaypointId iFrom, TWaypointId iTo)
{
    WaypointGraph::node_t& from = m_cGraph[iFrom];
    for (WaypointGraph::arc_it it = from.neighbours.begin(); it != from.neighbours.end(); ++it)
        if (it->target == iTo)
            return &(it->edge);
    return NULL;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::Add( const Vector& vOrigin, TWaypointFlags iFlags, int iArgument, int iAreaId )
{
    CWaypoint w(vOrigin, iFlags, iArgument, iAreaId);

    // lol, this is not working because m_cGraph.begin() is called first. wtf?
    // TWaypointId id = m_cGraph.add_node(w) - m_cGraph.begin();
    CWaypoints::WaypointNodeIt it( m_cGraph.add_node(w) );
    TWaypointId id = it - m_cGraph.begin();

    it->neighbours.reserve( 8 );

    AddLocation(id, vOrigin);
    bValidVisibilityTable = false;
    return id;
}

  
//----------------------------------------------------------------------------------------------------------------
void CWaypoints::Remove( TWaypointId id, bool bResetPlayers )
{
    if ( bResetPlayers )
        CPlayers::InvalidatePlayersWaypoints();

    DecrementLocationIds(id);
    m_cGraph.delete_node( m_cGraph.begin() + id );
    bValidVisibilityTable = false;
}


//----------------------------------------------------------------------------------------------------------------
bool CWaypoints::AddPath( TWaypointId iFrom, TWaypointId iTo, float fDistance, TPathFlags iFlags )
{
    if ( !CWaypoint::IsValid( iFrom ) || !CWaypoint::IsValid( iTo ) || ( iFrom == iTo ) || HasPath( iFrom, iTo ) )
        return false; // Can happen with commands.

    WaypointGraph::node_it from = m_cGraph.begin() + iFrom;
    WaypointGraph::node_it to = m_cGraph.begin() + iTo;

    if ( fDistance <= 0.0f)
        fDistance = from->vertex.vOrigin.DistTo(to->vertex.vOrigin);

    CWaypointPath cPath( fDistance, iFlags );
    if ( FLAG_SOME_SET( FPathJump | FPathBreak, iFlags ) )
    {
        // Jump / break after half seconds, and maintain (duck/shoot) during 1 second.
        cPath.SetActionTime( 5 );
        cPath.SetActionDuration( 10 );
    }
    m_cGraph.add_arc( from, to, cPath );
    return true;
}


//----------------------------------------------------------------------------------------------------------------
bool CWaypoints::RemovePath( TWaypointId iFrom, TWaypointId iTo )
{
    if ( !CWaypoint::IsValid(iFrom) || !CWaypoint::IsValid(iTo) || (iFrom == iTo) || !HasPath(iFrom, iTo) )
        return false;

    m_cGraph.delete_arc( m_cGraph.begin() + iFrom, m_cGraph.begin() + iTo );
    return true;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::CreatePathsWithAutoFlags( TWaypointId iWaypoint1, TWaypointId iWaypoint2, bool bIsCrouched, int iMaxDistance, bool bShowHelp )
{
    BASSERT( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2), return );

    WaypointNode& w1 = m_cGraph[iWaypoint1];
    WaypointNode& w2 = m_cGraph[iWaypoint2];

    if ( FLAG_SOME_SET( FWaypointLadder, w1.vertex.iFlags ) || FLAG_SOME_SET( FWaypointLadder, w2.vertex.iFlags ) )
        return;

    float fDist = w1.vertex.vOrigin.DistTo( w2.vertex.vOrigin );

    Vector v1 = w1.vertex.vOrigin, v2 = w2.vertex.vOrigin;
    bool bCrouch = bIsCrouched;
    TReach iReach = CUtil::GetReachableInfoFromTo( v1, v2, bCrouch, 0, Sqr( iMaxDistance ), bShowHelp );
    if (iReach != EReachNotReachable)
    {
        TPathFlags iFlags = (iReach == EReachNeedJump) ? FPathJump :
            (iReach == EReachFallDamage) ? FPathDamage : FPathNone;
        if ( bIsCrouched )
            FLAG_SET(FPathCrouch, iFlags);

        AddPath(iWaypoint1, iWaypoint2, fDist, iFlags);
    }

    iReach = CUtil::GetReachableInfoFromTo( v2, v1, bCrouch, 0, Sqr( iMaxDistance ), bShowHelp );
    if (iReach != EReachNotReachable)
    {
        TPathFlags iFlags = (iReach == EReachNeedJump) ? FPathJump :
                                    (iReach == EReachFallDamage) ? FPathDamage :
                                     FPathNone;
        if ( bIsCrouched )
            FLAG_SET(FPathCrouch, iFlags);

        AddPath(iWaypoint2, iWaypoint1, fDist, iFlags);
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::CreateAutoPaths( TWaypointId id, bool bIsCrouched, float fMaxDistance, bool bShowHelp )
{
    WaypointNode& w = m_cGraph[id];
    Vector vOrigin = w.vertex.vOrigin;

    int minX, minY, minZ, maxX, maxY, maxZ;
    int x = GetBucketX(vOrigin.x);
    int y = GetBucketY(vOrigin.y);
    int z = GetBucketZ(vOrigin.z);
    GetBuckets(x, y, z, minX, minY, minZ, maxX, maxY, maxZ);

    for (x = minX; x <= maxX; ++x)
        for (y = minY; y <= maxY; ++y)
            for (z = minZ; z <= maxZ; ++z)
            {
                Bucket& bucket = m_cBuckets[x][y][z];
                for ( Bucket::iterator it = bucket.begin(); it != bucket.end(); ++it )
                {
                    if ( *it == id || FLAG_SOME_SET( FPathLadder, Get( *it ).iFlags ) || HasPath( *it, id ) || HasPath( id, *it ) )
                        continue;

                    CreatePathsWithAutoFlags( id, *it, bIsCrouched, fMaxDistance, bShowHelp );
                }
            }
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::DecrementLocationIds( TWaypointId id )
{
    BASSERT( CWaypoint::IsValid(id), return );

    // Shift waypoints indexes, all waypoints with index > id.
    for (int x=0; x<BUCKETS_SIZE_X; ++x)
        for (int y=0; y<BUCKETS_SIZE_Y; ++y)
            for (int z=0; z<BUCKETS_SIZE_Z; ++z)
            {
                Bucket& bucket = m_cBuckets[x][y][z];
                for (Bucket::iterator it=bucket.begin(); it != bucket.end(); ++it)
                    if (*it > id)
                        --(*it);
            }

    // Remove waypoint id from bucket.
    Vector vOrigin = m_cGraph[id].vertex.vOrigin;
    Bucket& bucket = m_cBuckets[GetBucketX(vOrigin.x)][GetBucketY(vOrigin.y)][GetBucketZ(vOrigin.z)];
    bucket.erase(find(bucket, id));
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetNearestWaypoint(const Vector& vOrigin, const good::bitset* aOmit,
                                           bool bNeedVisible, float fMaxDistance, TWaypointFlags iFlags)
{
	TWaypointId result = EWaypointIdInvalid;

    float sqDist = SQR(fMaxDistance);
    float sqMinDistance = sqDist;

    int minX, minY, minZ, maxX, maxY, maxZ;
    int x = GetBucketX(vOrigin.x);
    int y = GetBucketY(vOrigin.y);
    int z = GetBucketZ(vOrigin.z);

    GetBuckets(x, y, z, minX, minY, minZ, maxX, maxY, maxZ);

    if ( bNeedVisible )
        CUtil::SetPVSForVector( vOrigin );

    for (x = minX; x <= maxX; ++x)
        for (y = minY; y <= maxY; ++y)
            for (z = minZ; z <= maxZ; ++z)
            {
                Bucket& bucket = m_cBuckets[x][y][z];
                for (Bucket::iterator it=bucket.begin(); it != bucket.end(); ++it)
                {
                    TWaypointId iWaypoint = *it;
                    if ( aOmit && aOmit->test(iWaypoint) )
                        continue;

                    WaypointNode& node = m_cGraph[iWaypoint];
                    if ( FLAG_SOME_SET_OR_0(iFlags, node.vertex.iFlags) )
                    {
                        float distTo = vOrigin.DistToSqr(node.vertex.vOrigin);
                        if ( (distTo <= sqDist) && (distTo < sqMinDistance) )
                        {
                            if ( !bNeedVisible || ( CUtil::IsVisiblePVS( node.vertex.vOrigin ) &&
                                                    CUtil::IsVisible( vOrigin, node.vertex.vOrigin, EVisibilityWorld, false ) ) )
                            {
                                result = iWaypoint;
                                sqMinDistance = distTo;
                            }
                        }
                    }
                }
            }

    return result;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::GetNearestWaypoints( good::vector<TWaypointId>& aResult, const Vector& vOrigin, bool bNeedVisible, float fMaxDistance )
{
    float fMaxDistSqr = SQR( fMaxDistance );

    int minX, minY, minZ, maxX, maxY, maxZ;
    int x = GetBucketX( vOrigin.x );
    int y = GetBucketY( vOrigin.y );
    int z = GetBucketZ( vOrigin.z );

    GetBuckets( x, y, z, minX, minY, minZ, maxX, maxY, maxZ );

    if ( bNeedVisible )
        CUtil::SetPVSForVector( vOrigin );
    
    for ( x = minX; x <= maxX; ++x )
        for ( y = minY; y <= maxY; ++y )
            for ( z = minZ; z <= maxZ; ++z )
            {
                Bucket& bucket = m_cBuckets[ x ][ y ][ z ];
                for ( Bucket::iterator it = bucket.begin(); it != bucket.end(); ++it )
                {
                    TWaypointId iWaypoint = *it;
                    const WaypointNode& node = m_cGraph[ iWaypoint ];

                    if ( fabs( vOrigin.z - node.vertex.vOrigin.z ) <= fMaxDistance )
                    {
                        float fDistToSqr = vOrigin.AsVector2D().DistToSqr( node.vertex.vOrigin.AsVector2D() );
                        if ( fDistToSqr <= fMaxDistSqr &&
                            ( !bNeedVisible || ( CUtil::IsVisiblePVS( node.vertex.vOrigin ) && 
                                                 CUtil::IsVisible( vOrigin, node.vertex.vOrigin, EVisibilityWorld, false ) ) ) )
                             aResult.push_back( iWaypoint );
                    }
                }
            }
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetAnyWaypoint(TWaypointFlags iFlags)
{
    if ( CWaypoints::Size() == 0 )
        return EWaypointIdInvalid;

    TWaypointId id = rand() % CWaypoints::Size();
    for ( TWaypointId i = id; i >= 0; --i )
        if ( FLAG_SOME_SET_OR_0(iFlags, CWaypoints::Get(i).iFlags) )
            return i;
    for ( TWaypointId i = id+1; i < CWaypoints::Size(); ++i )
        if ( FLAG_SOME_SET_OR_0(iFlags, CWaypoints::Get(i).iFlags) )
            return i;
    return EWaypointIdInvalid;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetAimedWaypoint( const Vector& vOrigin, const QAngle& ang )
{
    int x = GetBucketX(vOrigin.x);
    int y = GetBucketY(vOrigin.y);
    int z = GetBucketZ(vOrigin.z);

    // Draw only waypoints from nearest buckets.
    int minX, minY, minZ, maxX, maxY, maxZ;
    GetBuckets(x, y, z, minX, minY, minZ, maxX, maxY, maxZ);

    // Get visible clusters from player's position.
    CUtil::SetPVSForVector(vOrigin);

    TWaypointId iResult = EWaypointIdInvalid;
    float fLowestAngDiff = 180 + 90; // Set to max angle difference.

    for (x = minX; x <= maxX; ++x)
        for (y = minY; y <= maxY; ++y)
            for (z = minZ; z <= maxZ; ++z)
            {
                Bucket& bucket = m_cBuckets[x][y][z];
                for (Bucket::iterator it=bucket.begin(); it != bucket.end(); ++it)
                {
                    WaypointNode& node = m_cGraph[*it];

                    // Check if waypoint is in pvs from player's position.
                    if ( CUtil::IsVisiblePVS(node.vertex.vOrigin) && CUtil::IsVisible(vOrigin, node.vertex.vOrigin, EVisibilityWorld, false ) )
                    {
                        Vector vRelative(node.vertex.vOrigin);
                        vRelative.z -= CMod::GetVar( EModVarPlayerEye ) / 2; // Consider to look at center of waypoint.
                        vRelative -= vOrigin;

                        QAngle angDiff;
                        VectorAngles( vRelative, angDiff );
                        CUtil::DeNormalizeAngle(angDiff.y);
                        CUtil::GetAngleDifference(ang, angDiff, angDiff);
                        float fAngDiff = fabs(angDiff.x) + fabs(angDiff.y);
                        if ( fAngDiff < fLowestAngDiff )
                        {
                            fLowestAngDiff = fAngDiff;
                            iResult = *it;
                        }
                    }
                }
            }

    return iResult;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::Draw( CClient* pClient )
{
    if ( CBotrixPlugin::fTime < fNextDrawWaypointsTime )
        return;

    float fDrawTime = CWaypoint::DRAW_INTERVAL + (2.0f / CBotrixPlugin::iFPS); // Add two frames to not flick.
    fNextDrawWaypointsTime = CBotrixPlugin::fTime + CWaypoint::DRAW_INTERVAL;

    if ( pClient->iWaypointDrawFlags != FWaypointDrawNone )
    {
        float fPlayerEye = CMod::GetVar( EModVarPlayerEye );

        Vector vOrigin;
        vOrigin = pClient->GetHead();
        int x = GetBucketX(vOrigin.x);
        int y = GetBucketY(vOrigin.y);
        int z = GetBucketZ(vOrigin.z);

        // Draw only waypoints from nearest buckets.
        int minX, minY, minZ, maxX, maxY, maxZ;
        GetBuckets(x, y, z, minX, minY, minZ, maxX, maxY, maxZ);

        // Get visible clusters from player's position.
        CUtil::SetPVSForVector(vOrigin);

        for (x = minX; x <= maxX; ++x)
            for (y = minY; y <= maxY; ++y)
                for (z = minZ; z <= maxZ; ++z)
                {
                    Bucket& bucket = m_cBuckets[x][y][z];
                    for (Bucket::iterator it=bucket.begin(); it != bucket.end(); ++it)
                    {
                        WaypointNode& node = m_cGraph[*it];

                        // Check if waypoint is in pvs from player's position.
                        if ( CUtil::IsVisiblePVS( node.vertex.vOrigin ) && CUtil::IsVisible( vOrigin, node.vertex.vOrigin, EVisibilityWorld ) )
                            node.vertex.Draw( *it, pClient->iWaypointDrawFlags, fDrawTime );
                    }
                }

        for ( TAnalyzeWaypoints j = 0; j < EAnalyzeWaypointsTotal; ++j )
        {
            good::vector<Vector>& aPositions = m_aWaypointsToAddOmitInAnalyze[j];
            unsigned char r[ 3 ] = { 0x00, 0xFF, 0x00 }, g[ 3 ] = { 0xFF, 0x00, 0x00 }, b[ 3 ] = { 0x00, 0x00, 0xFF };
            for ( int i = 0; i < aPositions.size(); ++i )
            {
                Vector vOrigin = aPositions[ i ]; vOrigin.x += 0.3f; vOrigin.y += 0.3f;
                Vector vEnd = vOrigin; vEnd.x += 0.3f; vEnd.y += 0.3f; vEnd.z -= fPlayerEye;
                if ( CUtil::IsVisiblePVS( aPositions[ i ] ) && CUtil::IsVisible( vOrigin, aPositions[ i ], EVisibilityWorld ) )
                    CUtil::DrawLine( vOrigin, vEnd, fDrawTime, r[ j ], g[ j ], b[ j ] );
            }
        }
    }

    if ( pClient->iPathDrawFlags != FPathDrawNone )
    {
        // Draw nearest waypoint paths.
        if ( CWaypoint::IsValid( pClient->iCurrentWaypoint ) )
        {
            if ( pClient->iPathDrawFlags != FPathDrawNone )
                DrawWaypointPaths( pClient->iCurrentWaypoint, pClient->iPathDrawFlags );
            CWaypoint& w = CWaypoints::Get( pClient->iCurrentWaypoint );
            CUtil::DrawText(w.vOrigin, 0, fDrawTime, 0xFF, 0xFF, 0xFF, "Current");
        }

        if ( CWaypoint::IsValid(pClient->iDestinationWaypoint) )
        {
            CWaypoint& w = CWaypoints::Get( pClient->iDestinationWaypoint );
            Vector v(w.vOrigin);
            v.z -= 10.0f;
            CUtil::DrawText(v, 0, fDrawTime, 0xFF, 0xFF, 0xFF, "Destination");
        }
    }

    if ( bValidVisibilityTable && (pClient->iVisiblesDrawFlags != FPathDrawNone) &&
         CWaypoint::IsValid(pClient->iCurrentWaypoint) )
        DrawVisiblePaths( pClient->iCurrentWaypoint, pClient->iVisiblesDrawFlags );
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::MarkUnreachablePath( TWaypointId iWaypointFrom, TWaypointId iWaypointTo )
{
    if ( CWaypoint::iUnreachablePathFailuresToDelete <= 0 )
        return;

    GoodAssert( IsValid( iWaypointFrom ) && IsValid( iWaypointTo ) && HasPath( iWaypointFrom, iWaypointTo ) );

    Vector vFrom = Get( iWaypointFrom ).vOrigin;
    Vector vTo = Get( iWaypointTo ).vOrigin;

    for ( int i = 0; i < m_aUnreachablePaths.size(); ++i )
    {
        unreachable_path_t &cPath = m_aUnreachablePaths[ i ];
        if ( cPath.vFrom == vFrom && vTo == vTo )
        {
            if ( ++cPath.iFailedCount >= CWaypoint::iUnreachablePathFailuresToDelete )
            {
                BLOG_I( "Removing unreachable path from %d to %d.", iWaypointFrom, iWaypointTo );
                RemovePath( iWaypointFrom, iWaypointTo );
                m_aUnreachablePaths.erase( i );
            }
            return;
        }
    }
    
    // Add new unreachable path.
    unreachable_path_t cPath;
    cPath.iFailedCount = 1;
    cPath.vFrom = vFrom;
    cPath.vTo = vTo;
    m_aUnreachablePaths.push_back( cPath );
}

//----------------------------------------------------------------------------------------------------------------
void CWaypoints::AddLadderDismounts( ICollideable *pLadder, float fPlayerWidth, float fPlayerEye, TWaypointId iBottom, TWaypointId iTop )
{
    float fMaxHeight = CMod::GetVar( EModVarPlayerJumpHeightCrouched );
    float fPlayerHalfWidth = fPlayerWidth / 2.0f;
    QAngle angles = pLadder->GetCollisionAngles();

    Vector vZ( 0, 0, 1 );
    Vector vDirection; AngleVectors( angles, &vDirection ); vDirection.z = 0; vDirection.NormalizeInPlace();
    Vector vPerpendicular = vDirection.Cross( vZ );

    Vector vDirections[ 4 ];
    vDirections[ 0 ] = vDirection * fPlayerWidth;
    vDirections[ 1 ] = -vDirection * fPlayerWidth;
    vDirections[ 2 ] = vPerpendicular * fPlayerWidth;
    vDirections[ 3 ] = -vPerpendicular * fPlayerWidth;

    TWaypointId iWaypoints[ 2 ] = { iBottom, iTop };
    CTraceFilterWithFlags& iTracer = CUtil::GetTraceFilter( EVisibilityWaypoints );

    for ( int i = 0; i < 2; ++i )
    {
        Vector vPos = Get( iWaypoints[ i ] ).vOrigin;
        Vector vPosGround = vPos; vPosGround.z -= fPlayerEye;

        bool bFound = false;
        for ( int j = 0; j < 4; ++j )
        {
            Vector vNew = vPos + vDirections[j];
            Vector vGround = CUtil::GetGroundVec( vNew ); // Not hull ground.
            vGround.z += fPlayerEye;

            if ( vPos.z - vGround.z > fMaxHeight ) // Too high.
                continue;

            if ( !CUtil::IsVisible( vPos, vGround, EVisibilityWaypoints, false ) )
                continue;

            vGround = CUtil::GetHullGroundVec( vNew );
            CUtil::TraceHull( vPosGround, vGround, CMod::vPlayerCollisionHullMins, CMod::vPlayerCollisionHullMaxs, iTracer.iTraceFlags, &iTracer );

            if ( CUtil::IsTraceHitSomething() )
                continue;

            vGround.z += fPlayerEye;

            TWaypointId iDismount = CWaypoints::GetNearestWaypoint( vGround, NULL, false, fPlayerHalfWidth );
            if ( iDismount == EWaypointIdInvalid )
            {
                iDismount = Add( vGround );
                BULOG_D( m_pAnalyzer, "  added waypoint %d (%s ladder dismount) at (%.0f, %.0f, %.0f)", iDismount,
                         i == 0 ? "bottom" : "top", vPos.x, vPos.y, vPos.z );
            }

            AddPath( iWaypoints[ i ], iDismount, 0.0f, FPathLadder | FPathJump );
            AddPath( iDismount, iWaypoints[ i ], 0.0f, FPathLadder );

            bFound = true;
        }

        if ( !bFound )
            BULOG_W( m_pAnalyzer, "Ladder waypoint %d, couldn't create dismount waypoints.", iWaypoints[i] );
    }
}
//----------------------------------------------------------------------------------------------------------------
void CWaypoints::Analyze( edict_t* pClient, bool bShowLines )
{
    m_pAnalyzer = pClient;
    CWaypoint::bShowAnalyzePotencialWaypoints = bShowLines;
    
    m_iAnalyzeStep = EAnalyzeStepNeighbours;
    m_iCurrentAnalyzeWaypoint = 0;
    m_fAnalyzeWaypointsForNextFrame = 0;

    BULOG_W( pClient, "Started to analyze waypoints." );
    static TItemType aItemTypes[] = { EItemTypePlayerSpawn, EItemTypeHealth, EItemTypeArmor, EItemTypeWeapon, EItemTypeAmmo };

    float fPlayerHeight = CMod::GetVar( EModVarPlayerHeight );
    float fPlayerEye = CMod::GetVar( EModVarPlayerEye );
    float fPlayerWidth = CMod::GetVar( EModVarPlayerWidth );
    float fPlayerHalfWidth = fPlayerWidth / 2.0f;

    float fAnalyzeDistance = CWaypoint::iAnalyzeDistance;
    float fAnalyzeDistanceExtra = fAnalyzeDistance * 1.9f; // To include diagonal, almost but not 2 (Pythagoras).

    BULOG_I( pClient, "Adding waypoints at spawn / items / ladder positions." );
    for ( int iType = 0; iType < (int)( sizeof( aItemTypes ) / sizeof( aItemTypes[ 0 ] ) ); ++iType )
    {
        TItemType iItemType = aItemTypes[ iType ];

        const good::vector<CItem>& items = CItems::GetItems( iItemType );
        for ( TItemIndex i = 0; i < items.size(); ++i )
        {
            if ( items[ i ].IsFree() || items[ i ].IsTaken() )
                continue;
            
            bool bUse = FLAG_SOME_SET( FItemUse, items[ i ].pItemClass->iFlags );
            TWaypointFlags iFlags = CWaypoint::GetFlagsFor(iItemType);
            int iArgument = 0;
            Vector vOrigin, vPos;
            
            if ( bUse )
            {
                FLAG_CLEAR( FWaypointHealth | FWaypointArmor, iFlags ); // Leave only chargers, not items.
                
                ICollideable *pCollideable = items[ i ].pEdict->GetCollideable();
                Vector vMins, vMaxs; pCollideable->WorldSpaceSurroundingBounds( &vMins, &vMaxs );
                Vector vMid = ( vMins + vMaxs ) / 2.0f;
                vOrigin = ( vMaxs - vMins ) / 2.0f;
                float fItemHalfWidth = MAX2( vOrigin.x, vOrigin.y );
                
                QAngle angles = pCollideable->GetCollisionAngles();

                Vector vDirection; AngleVectors( angles, &vDirection );
                vDirection *= fItemHalfWidth + fPlayerHalfWidth + 5.0f;

                vPos = vMid + vDirection;
                vPos = CUtil::GetHullGroundVec( vPos );
                vPos.z += fPlayerEye;

                if ( fabs( vPos.z - vMid.z ) > fPlayerHeight ) // Too high or need to fall to grab, probably needs to grab with gravity gun.
                    continue;

                VectorAngles(vMid - vPos, angles);
                CUtil::DeNormalizeAngle( angles.x ); CUtil::DeNormalizeAngle( angles.y );

                CWaypoint::SetFirstAngle( angles.x, angles.y, iArgument );
            }
            else
            {
                FLAG_CLEAR( FWaypointHealthMachine | FWaypointArmorMachine, iFlags ); // Leave only items, not chargers.

                vOrigin = items[ i ].CurrentPosition();
                vPos = vOrigin;
                vPos.z += fPlayerEye;

                Vector vGround = CUtil::GetHullGroundVec( vPos );
                vGround.z += fPlayerEye;

                if ( fabs( vPos.z - vGround.z ) > fPlayerHeight ) // Too high or need to fall to grab, probably needs to grab with gravity gun.
                    continue;

                vPos = vGround;
            }

            TWaypointId iWaypoint = CWaypoints::GetNearestWaypoint( vPos, NULL, false, fPlayerHalfWidth );
            if ( iWaypoint == EWaypointIdInvalid )
            {
                iWaypoint = Add( vPos, iFlags, iArgument );
                BULOG_D( m_pAnalyzer, "  added waypoint %d (%s %d at (%.0f, %.0f, %.0f)) at (%.0f, %.0f, %.0f)", iWaypoint, items[ i ].pItemClass->sClassName.c_str(), i,
                         vOrigin.x, vOrigin.y, vOrigin.z, vPos.x, vPos.y, vPos.z );
            }

            CreateAutoPaths( iWaypoint, false, fAnalyzeDistanceExtra, false );
        }
    }

    // Add ladder waypoints.
    const good::vector<CItem>& items = CItems::GetItems( EItemTypeLadder );
    for ( TItemIndex i = 0; i < items.size(); ++i )
    {
        if ( items[ i ].IsFree() )
            continue;

        ICollideable *pCollideable = items[ i ].pEdict->GetCollideable();
        Vector vMins, vMaxs; pCollideable->WorldSpaceSurroundingBounds( &vMins, &vMaxs );
        
        vMins.x = ( vMins.x + vMaxs.x ) / 2.0f;
        vMins.y = ( vMins.y + vMaxs.y ) / 2.0f;

        vMaxs.x = vMins.x; vMaxs.y = vMins.y;

        vMins.z += fPlayerEye;
        vMins = CUtil::GetHullGroundVec( vMins );
        vMins.z += fPlayerEye + 2.0f;

        vMaxs.z += -fPlayerHeight + fPlayerEye - 2.0f;

        TWaypointId w1 = CWaypoints::GetNearestWaypoint( vMins, NULL, false, fPlayerHalfWidth );
        if ( w1 == EWaypointIdInvalid )
        {
            w1 = Add( vMins, FWaypointLadder );
            BULOG_D( m_pAnalyzer, "  added waypoint %d (bottom ladder %d) at (%.0f, %.0f, %.0f)", w1, i, vMins.x, vMins.y, vMins.z );
        }
        else
            FLAG_SET( FWaypointLadder, Get( w1 ).iFlags );
        
        TWaypointId w2 = CWaypoints::GetNearestWaypoint( vMaxs, NULL, false, fPlayerHalfWidth );
        if ( w2 == EWaypointIdInvalid )
        {
            w2 = Add( vMaxs, FWaypointLadder );
            BULOG_D( m_pAnalyzer, "  added waypoint %d (top ladder %d) at (%.0f, %.0f, %.0f)", w2, i, vMaxs.x, vMaxs.y, vMaxs.z );
        }
        else
            FLAG_SET( FWaypointLadder, Get( w2 ).iFlags );

        float fDist = vMaxs.DistTo( vMins );

        if ( HasPath( w1, w2 ) )
            FLAG_SET( FPathLadder, GetPath( w1, w2 )->iFlags );
        else
            AddPath( w1, w2, fDist, FPathLadder );
        
        if ( HasPath( w2, w1 ) )
            FLAG_SET( FPathLadder, GetPath( w2, w1 )->iFlags );
        else
            AddPath( w2, w1, fDist, FPathLadder );
        
        CreateAutoPaths( w1, false, fAnalyzeDistanceExtra, false );
        CreateAutoPaths( w2, false, fAnalyzeDistanceExtra, false );

        AddLadderDismounts( pCollideable, fPlayerWidth, fPlayerEye, w1, w2 );
    }

    BULOG_I( pClient, "Adding waypoints at added positions ('botrix waypoint analyze add')." );
    good::vector<Vector>& aAdded = m_aWaypointsToAddOmitInAnalyze[ EAnalyzeWaypointsAdd ];
    for ( int i = 0; i < aAdded.size(); ++i )
    {
        Vector& vPos = aAdded[ i ];
        
        TWaypointId iWaypoint = CWaypoints::GetNearestWaypoint( vPos, NULL, false, fPlayerHalfWidth );
        if ( iWaypoint == EWaypointIdInvalid )
        {
            iWaypoint = Add( vPos );
            BULOG_D( m_pAnalyzer, "  added waypoint %d at (%.0f, %.0f, %.0f)", iWaypoint, vPos.x, vPos.y, vPos.z );
        }
        CreateAutoPaths( iWaypoint, false, fAnalyzeDistanceExtra, false );
    }

    if ( Size() == 0 )
    {
        StopAnalyzing();
        BULOG_W( pClient, "No waypoints to analyze (no player spawn entities on the map?)." );
        BULOG_W( pClient, "Please add some waypoints manually and run the command again." );
        BULOG_W( pClient, "Stopped analyzing waypoints." );
        return;
    }

    BULOG_I( m_pAnalyzer, "Adding new waypoints." );
    m_aWaypointsNeighbours.resize( 2048 );
}

void CWaypoints::StopAnalyzing()
{
    m_aWaypointsNeighbours = good::vector<CNeighbour>();
    m_iAnalyzeStep = EAnalyzeStepTotal;
    m_pAnalyzer = NULL;
}

void CWaypoints::AnalyzeStep()
{
    GoodAssert( m_iAnalyzeStep < EAnalyzeStepTotal );
    static int iOldSize;
    
    // Search for analyzer in players, else remove him. Can't rely on CPlayers::GetIndex() because the server is reusing edicts.
    if ( m_pAnalyzer )
    {
        bool bFound = false;
        for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
        {
            if ( CPlayers::Get( iPlayer )->GetEdict() == m_pAnalyzer )
            {
                bFound = true;
                break;
            }
        }

        if ( !bFound )
            m_pAnalyzer = NULL;
    }

    // Check more waypoints in inters step, as less traces are required.
    float fToAnalyze = m_fAnalyzeWaypointsForNextFrame;
    fToAnalyze += CWaypoint::fAnalyzeWaypointsPerFrame * ( 1 + m_iAnalyzeStep * 4 );
    int iToAnalyze = (int)fToAnalyze;
    m_fAnalyzeWaypointsForNextFrame = fToAnalyze - iToAnalyze;

    if ( m_iAnalyzeStep < EAnalyzeStepDeleteOrphans )
    {
        float fPlayerEye = CMod::GetVar( EModVarPlayerEye );
        float fHalfPlayerWidth = CMod::GetVar( EModVarPlayerWidth ) / 2.0f;

        float fAnalyzeDistance = CWaypoint::iAnalyzeDistance;
        float fAnalyzeDistanceExtra = fAnalyzeDistance * 1.9f; // To include diagonal, almost but not 2 (Pythagoras).
        float fAnalyzeDistanceExtraSqr = Sqr( fAnalyzeDistanceExtra * 1.9f );

        for ( int i = 0; i < iToAnalyze && m_iCurrentAnalyzeWaypoint < Size(); ++i, ++m_iCurrentAnalyzeWaypoint )
        {
            TWaypointId iWaypoint = m_iCurrentAnalyzeWaypoint;
            CWaypoint &cWaypoint = Get( iWaypoint );

            if ( FLAG_SOME_SET( FWaypointLadder, cWaypoint.iFlags ) )
                continue; // Don't analyze ladder waypoints here, this will be done in the last step.

            Vector vPos = cWaypoint.vOrigin;
            if ( good::find( m_aWaypointsToAddOmitInAnalyze[EAnalyzeWaypointsOmit], vPos ) != m_aWaypointsToAddOmitInAnalyze[ EAnalyzeWaypointsOmit ].end() )
                continue;

            CNeighbour neighbours = m_aWaypointsNeighbours[ iWaypoint ];
            for ( int x = -1; x <= 1; ++x )
            {
                for ( int y = -1; y <= 1; ++y )
                {
                    if ( ( x == 0 && y == 0 ) || neighbours.a[ x + 1 ][ y + 1 ] )
                        continue;

                    Vector vNew = vPos;

                    if ( m_iAnalyzeStep == EAnalyzeStepNeighbours )
                    {
                        // First check if there is a waypoint near final position.
                        vNew.x += fAnalyzeDistance * x;
                        vNew.y += fAnalyzeDistance * y;

                        if ( AnalyzeWaypoint( iWaypoint, vPos, vNew, fPlayerEye, fHalfPlayerWidth,
                                              fAnalyzeDistance, fAnalyzeDistanceExtra, fAnalyzeDistanceExtraSqr ) )
                            neighbours.a[ x + 1 ][ y + 1 ] = true; // Position is already occupied or new waypoint is added.
                    }
                    else // m_iAnalyzeStep == EAnalyzeStepInters
                    {
                        if ( x == 1 && y == 1 ) // Omit (1, 1), as there are no adjacent points up / right to it.
                            continue;

                        if ( m_aWaypointsNeighbours[ iWaypoint ].a[ x + 1 ][ y + 1 ] ) // Convert [-1..1] to [0..2].
                            continue; // Don't use neighbours here, as it can be updated.

                        int incX = x + 1; // Adjacent point on X.
                        if ( incX <= 1 && !( incX == 0 && y == 0 ) && // Omit (0, 0) and (2, y).
                             !m_aWaypointsNeighbours[ iWaypoint ].a[ incX + 1 ][ y + 1 ] )
                        {
                            vNew.x += fAnalyzeDistance * ( x + incX / 2.0f ); // Will be -1/2 or 1/2.
                            vNew.y += fAnalyzeDistance * y;

                           if ( AnalyzeWaypoint( iWaypoint, vPos, vNew, fPlayerEye, fHalfPlayerWidth,
                                                 fAnalyzeDistance, fAnalyzeDistanceExtra, fAnalyzeDistanceExtraSqr ) )
                                neighbours.a[ x + 1 ][ y + 1 ] = true;
                        }

                        int incY = y + 1; // Adjacent point on Y.
                        if ( incY <= 1 && !( x == 0 && incY == 0 ) && // Omit (0, 0) and (x, 2).
                             !m_aWaypointsNeighbours[ iWaypoint ].a[ x + 1 ][ incY + 1 ] )
                        {
                            vNew = vPos;
                            vNew.x += fAnalyzeDistance * x;
                            vNew.y += fAnalyzeDistance * ( y + incY / 2.0f ); // Will be -1/2 or 1/2.

                            if ( AnalyzeWaypoint( iWaypoint, vPos, vNew, fPlayerEye, fHalfPlayerWidth,
                                                  fAnalyzeDistance, fAnalyzeDistanceExtra, fAnalyzeDistanceExtraSqr ) )
                                neighbours.a[ x + 1 ][ y + 1 ] = true;
                        }
                    }
                }
            } // for x.. y..
            m_aWaypointsNeighbours[ iWaypoint ] = neighbours;
        }
    }
    else
    {
        // Remove waypoints without paths.
        good::vector<Vector> &aToOmit = m_aWaypointsToAddOmitInAnalyze[ EAnalyzeWaypointsOmit ];
        TWaypointId i = m_iCurrentAnalyzeWaypoint;
        for ( ; iToAnalyze > 0 && i >= 0; --i, --iToAnalyze )
            if ( m_cGraph[ i ].neighbours.size() == 0 ||
                 good::find( aToOmit, m_cGraph[ i ].vertex.vOrigin ) != aToOmit.end() )
            {
                BULOG_D( m_pAnalyzer, "  removing waypoint %d.", i );
                Remove( i-- );
                break; // Don't remove more that 1 waypoint per frame, will hang up for couple of seconds.
            }
        m_iCurrentAnalyzeWaypoint = i;
    }

    if ( m_iCurrentAnalyzeWaypoint < 0 || m_iCurrentAnalyzeWaypoint >= Size() ) // Got out of bounds.
    {
        switch ( m_iAnalyzeStep )
        {
            case EAnalyzeStepNeighbours:
                BULOG_I( m_pAnalyzer, "Checking for missing spots." );
                m_iCurrentAnalyzeWaypoint = 0;
                m_bIsAnalyzeStepAddedWaypoints = false;
                ++m_iAnalyzeStep;
                break;

            case EAnalyzeStepInters:
                if ( m_bIsAnalyzeStepAddedWaypoints )
                {
                    --m_iAnalyzeStep;
                    BULOG_I( m_pAnalyzer, "Checking new waypoints from missing spots." );
                    m_iCurrentAnalyzeWaypoint = 0;
                }
                else
                {
                    ++m_iAnalyzeStep;
                    iOldSize = Size(); // Save size before erasing waypoints.
                    BULOG_I( m_pAnalyzer, "Erasing orphans / omitted waypoints." );
                    m_iCurrentAnalyzeWaypoint = Size() - 1;
                }
                break;

            case EAnalyzeStepDeleteOrphans:
                BULOG_I( m_pAnalyzer, "Removed %d orphan / omitted waypoints.", iOldSize - Size() );

                StopAnalyzing(); // Stop analyzing, no more waypoints.
                BULOG_I( m_pAnalyzer, "Total waypoints: %d.", Size() );
                BULOG_W( m_pAnalyzer, "Stopped analyzing waypoints." );
                break;

            default:
                GoodAssert( false );
        }
    }
}

bool CWaypoints::AnalyzeWaypoint( TWaypointId iWaypoint, Vector& vPos, Vector& vNew, float fPlayerEye, float fHalfPlayerWidth,
                                  float fAnalyzeDistance, float fAnalyzeDistanceExtra, float fAnalyzeDistanceExtraSqr )
{
    static good::vector<TWaypointId> aNearWaypoints( 16 );

    if ( ( CBotrixPlugin::pEngineTrace->GetPointContents( vNew ) & MASK_SOLID_BRUSHONLY ) != 0 )
        return false; // Ignore, if inside some solid brush.

    float fJumpHeight = CMod::GetVar( EModVarPlayerJumpHeightCrouched );
    float fHalfPlayerWidthSqr = SQR( fHalfPlayerWidth );
    Vector vGround = CUtil::GetHullGroundVec( vNew );

    // Check if on a border.
    Vector vHullGround = vGround, vFineGround = CUtil::GetGroundVec( vNew );
    if ( vHullGround.z - vFineGround.z > fJumpHeight )
    {
        // On a border (line trace hits lower ground), try to get off from there.
        static float directions[ 4 ][ 3 ] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 } };

        for ( int i = 0; i < 4; ++i )
        {
            Vector vDirection = Vector( directions[ i ][ 0 ], directions[ i ][ 1 ], directions[ i ][ 2 ] );
            vDirection *= fHalfPlayerWidth;

            Vector vDisplaced = vNew + vDirection;
            vHullGround = CUtil::GetHullGroundVec( vDisplaced );
            if ( fabs( vHullGround.z - vGround.z ) < CMod::GetVar( EModVarPlayerObstacleToJump ) ) // Small difference.
            {
                vFineGround = CUtil::GetGroundVec( vDisplaced );
                if ( fabs( vHullGround.z - vFineGround.z ) < CMod::GetVar( EModVarPlayerObstacleToJump ) ) // Small difference.
                {
                    vGround = vHullGround;
                    break;
                }
            }
        }
    }

    vGround.z += fPlayerEye;

    if ( CWaypoint::bShowAnalyzePotencialWaypoints )
    {
        Vector v = vGround; v.z -= fPlayerEye;
        CUtil::DrawLine( vGround, v, 10, 255, 255, 255 );
    }

    aNearWaypoints.clear();
    CWaypoints::GetNearestWaypoints( aNearWaypoints, vGround, true, fAnalyzeDistance / 1.41f );

    bool bSkip = false;

    const good::vector<Vector>& aPositions = m_aWaypointsToAddOmitInAnalyze[ EAnalyzeWaypointsDebug ];
    bool showHelp = ( CWaypoint::fAnalyzeWaypointsPerFrame < ANALIZE_HELP_IF_LESS_WAYPOINTS_PER_FRAME ||
                      good::find( aPositions, vNew ) != aPositions.end() ||
                      good::find( aPositions, vPos ) != aPositions.end() );

    for ( int w = 0; !bSkip && w < aNearWaypoints.size(); ++w )
    {
        int iNear = aNearWaypoints[ w ];
        if ( iNear != iWaypoint && !HasPath( iWaypoint, iNear ) && !HasPath( iNear, iWaypoint ) )
            CreatePathsWithAutoFlags( iWaypoint, iNear, false, fAnalyzeDistanceExtra, showHelp  );
        
        // If has path, set bSkip to true.
        bSkip |= HasPath( iWaypoint, iNear ) != NULL;

        // If path is not adding somehow, but the waypoint is really close (half player's width or closer).
        bSkip |= Get( iNear ).vOrigin.AsVector2D().DistToSqr( vGround.AsVector2D() ) <= fHalfPlayerWidthSqr;
    }

    if ( bSkip )
        return true; // No waypoint is added, but nearby paths are created.

    bool bCrouch = false;
    TReach reach = CUtil::GetReachableInfoFromTo( vPos, vGround, bCrouch, 0.0f, fAnalyzeDistanceExtraSqr, showHelp );
    if ( reach != EReachFallDamage && reach != EReachNotReachable )
    {
        TWaypointId iNew = Add( vGround );
        BULOG_D( m_pAnalyzer, "  added waypoint %d (from %d) at (%.0f, %.0f, %.0f)", iNew, iWaypoint, vGround.x, vGround.y, vGround.z );

        m_bIsAnalyzeStepAddedWaypoints = true;

        TPathFlags iFlags = bCrouch ? FPathCrouch : FPathNone;
        AddPath( iWaypoint, iNew, 0, iFlags | ( reach == EReachNeedJump ? FPathJump : FPathNone ) );

        bool bDestCrouch = false;
        reach = CUtil::GetReachableInfoFromTo( vGround, vPos, bDestCrouch, 0, fAnalyzeDistanceExtraSqr, showHelp );
        if ( reach != EReachFallDamage && reach != EReachNotReachable )
        {
            iFlags = bDestCrouch ? FPathCrouch : FPathNone;
            AddPath( iNew, iWaypoint, 0, iFlags | ( reach == EReachNeedJump ? FPathJump : FPathNone ) );
        }

        CreateAutoPaths( iNew, bCrouch, fAnalyzeDistanceExtra, showHelp );

        m_aWaypointsNeighbours.resize( Size() );
        return true; // New waypoint is added.
    }

    return false; // No waypoint is added.
}

//----------------------------------------------------------------------------------------------------------------
void CWaypoints::GetPathColor( TPathFlags iFlags, unsigned char& r, unsigned char& g, unsigned char& b )
{
    if (FLAG_SOME_SET(FPathDemo, iFlags) )
    {
        r = 0xFF; g = 0x00; b = 0xFF; // Magenta effect, demo.
    }
    else if (FLAG_SOME_SET(FPathBreak, iFlags) )
    {
        r = 0xFF; g = 0x00; b = 0x00; // Red effect, break.
    }
    else if (FLAG_SOME_SET(FPathSprint, iFlags) )
    {
        r = 0xFF; g = 0xFF; b = 0x00; // Yellow effect, sprint.
    }
    else if (FLAG_SOME_SET(FPathStop, iFlags) )
    {
        r = 0x66; g = 0x66; b = 0x00; // Dark yellow effect, stop.
    }
    else if ( FLAG_SOME_SET(FPathLadder, iFlags) )
    {
        r = 0xFF; g = 0x80; b = 0x00; // Orange effect, ladder.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathJump | FPathCrouch, iFlags) )
    {
        r = 0x00; g = 0x00; b = 0x66; // Dark blue effect, jump + crouch.
    }
    else if (FLAG_SOME_SET(FPathJump, iFlags) )
    {
        r = 0x00; g = 0x00; b = 0xFF; // Light blue effect, jump.
    }
    else if (FLAG_SOME_SET(FPathCrouch, iFlags) )
    {
        r = 0x00; g = 0xFF; b = 0x00; // Green effect, crouch.
    }
    else if ( FLAG_SOME_SET(FPathDoor | FPathElevator, iFlags) )
    {
        r = 0x8A; g = 0x2B; b = 0xE2;  // Violet effect, door.
    }
    else if (FLAG_SOME_SET(FPathTotem, iFlags) )
    {
        r = 0x96; g = 0x48; b = 0x00;  // Brown effect, totem.
    }
    else
    {
        r = 0x00; g = 0x7F; b = 0x7F; // Cyan effect, other flags.
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::DrawWaypointPaths( TWaypointId id, TPathDrawFlags iPathDrawFlags )
{
    BASSERT( iPathDrawFlags != FPathDrawNone, return );

    WaypointNode& w = m_cGraph[id];

    unsigned char r, g, b;
    Vector diff(0, 0, -CMod::GetVar( EModVarPlayerEye )/4);
    float fDrawTime = CWaypoint::DRAW_INTERVAL + (2.0f / CBotrixPlugin::iFPS); // Add two frames to not flick.

    for (WaypointArcIt it = w.neighbours.begin(); it != w.neighbours.end(); ++it)
    {
        WaypointNode& n = m_cGraph[it->target];
        GetPathColor(it->edge.iFlags, r, g, b);

        if ( FLAG_SOME_SET(FPathDrawBeam, iPathDrawFlags) )
            CUtil::DrawBeam(w.vertex.vOrigin + diff, n.vertex.vOrigin + diff, CWaypoint::PATH_WIDTH, fDrawTime, r, g, b);
        if ( FLAG_SOME_SET(FPathDrawLine, iPathDrawFlags) )
            CUtil::DrawLine(w.vertex.vOrigin + diff, n.vertex.vOrigin + diff, fDrawTime, r, g, b);
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::DrawVisiblePaths( TWaypointId id, TPathDrawFlags iPathDrawFlags )
{
    GoodAssert( bValidVisibilityTable && (iPathDrawFlags != FPathDrawNone) );

    Vector vOrigin( Get(id).vOrigin );
    Vector diff(0, 0, -CMod::GetVar( EModVarPlayerEye )/2);

    const unsigned char r = 0xFF, g = 0xFF, b = 0xFF;
    float fDrawTime = CWaypoint::DRAW_INTERVAL + (2.0f / CBotrixPlugin::iFPS); // Add two frames to not flick.

    for ( int i = 0; i < Size(); ++i )
        if ( (i != id) && m_aVisTable[id].test(i) )
        {
            const CWaypoint& cWaypoint = Get(i);
            if ( FLAG_SOME_SET(FPathDrawBeam, iPathDrawFlags) )
                CUtil::DrawBeam(vOrigin + diff, cWaypoint.vOrigin + diff, CWaypoint::PATH_WIDTH, fDrawTime, r, g, b);
            if ( FLAG_SOME_SET(FPathDrawLine, iPathDrawFlags) )
                CUtil::DrawLine(vOrigin + diff, cWaypoint.vOrigin + diff, fDrawTime, r, g, b);
        }
}
