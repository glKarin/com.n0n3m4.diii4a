#include "../../idlib/precompiled.h"
#pragma hdrstop

#ifdef MOD_BOTS // cusTom3 

#include "../Game_local.h"
#include "BotAASBuild.h"

idCVar aas_showElevators( "aas_showElevators", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_NOCHEAT, "show elevator debug stuff" );

/*
===============================================================================

	BotAASBuild.cpp

===============================================================================
*/

/*
============
BotAASBuild::Init
============
*/
void BotAASBuild::Init( idAASLocal * local )
{
    aas = local;
    file = aas->file;
}

/*
============
BotAASBuild::AddReachabilities
============
*/
void BotAASBuild::AddReachabilities( void )
{

    // steal the portals list so i can manipulate it
    originalPortals.list = file->portals.list;
    originalPortals.granularity = file->portals.granularity;
    originalPortals.num = file->portals.num;
    originalPortals.size = file->portals.size;

    portals.Append(file->portals);
    file->portals.list = portals.list;

    originalPortalIndex.list = file->portalIndex.list;
    originalPortalIndex.granularity = file->portalIndex.granularity;
    originalPortalIndex.num = file->portalIndex.num;
    originalPortalIndex.size = file->portalIndex.size;

    portalIndex.Append(file->portalIndex);
    file->portalIndex.list = portalIndex.list;

    AddElevatorReachabilities();
    AddTransporterReachabilities();
    //TryToAddLadders();
}
/*
============
BotAASBuild::TryToAddLadders
============
*/
void BotAASBuild::TryToAddLadders( void )
{

}



/*
============
BotAASBuild::OutputAASReachInfo
============
*/
void OutputAASReachInfo(idAASFile *file)
{
    gameLocal.Printf( "OutputAASReachInfo\n" );

    for ( int i = 0; i < file->GetNumAreas(); i++ )
    {
        gameLocal.Printf( "area %d\n", i );
        for ( idReachability *r = file->GetArea( i ).reach; r; r = r->next )
        {
            gameLocal.Printf( "   reach %d from %d to %d traveltype %d \n", r->number, r->fromAreaNum, r->toAreaNum, r->travelType );
        }
        gameLocal.Printf( "\n" );
        for ( idReachability *r = file->GetArea( i ).rev_reach; r; r = r->rev_next )
        {
            gameLocal.Printf( "   rev_reach %d from %d to %d traveltype %d \n", r->number, r->fromAreaNum, r->toAreaNum, r->travelType );
        }
    }
    gameLocal.Printf( "End OutputAASReachInfo\n" );
}




/*
============
BotAASBuild::AddElevatorReachabilities

 cusTom3	- welcome to the longest, largest, monolithic beast i can ever credit myself to writing
			- the idea is probably bad, the code is probably worse ;)
			- UPDATE: the idea has gotten worse, and the code has gone out of control. refactor or die.
			- lack of portal area's in consistent places = must loop around bottom of plat looking for portal (d3dm1)
			- this wouldn't create a portal up the middle if it needed one - all seemed to have one when necessary
			- look at using PushPointIntoAreaNum, cause it isn't, and maybe it should
			- lol, whatever
============
*/
void BotAASBuild::AddElevatorReachabilities( void )
{
    idAASFile *file;
    file = aas->file;

    idEntity *ent;
    // for each entity in the map
    for ( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
    {
        // that is an elevator
        if ( ent->IsType( idPlat::Type ) )
        {
            idPlat *platform = static_cast<idPlat *>(ent);
            idClipModel *model = platform->GetPhysics()->GetClipModel();
            // TODO: play with how much to expand this
            idBounds bounds = model->GetAbsBounds().Expand( 0 );

            if ( aas_showElevators.GetBool() )
            {
                gameRenderWorld->DebugBounds( colorGreen, bounds, vec3_origin, 200000 );
            }

            // top and bottom of plat (located in center, slightly above)
            idVec3 top, bottom, center;
            center = bounds.GetCenter();
            bottom = center;
            bottom[2] = bounds[1][2] + 2;
            top = center;
            top[2] = bottom[2] + ( platform->GetPosition2()[2] - platform->GetPosition1()[2] );

            // start of possible reach (on bottom), end of possible reach (on top) and thier area and cluster numbers
            idVec3 start, end;
            int topAreaNum, bottomAreaNum;
            int topClusterNum, bottomClusterNum;
            // for last ditch effort to create a reach
            idVec3 bottomNeedPortal, topNeedPortal;
            bool needPortal = false;

            // check for portals going up the plat before processing
            idVec3 firstPortal, lastPortal;
            int firstPortalNum, lastPortalNum;
            firstPortalNum = lastPortalNum = 0;
            bool reachabilityCreated = false;

            start = bottom;

            bottomAreaNum = aas->PointReachableAreaNum( start, DefaultBotBounds(), travelFlags );

            bottomClusterNum = file->areas[bottomAreaNum].cluster;

            // trace from bottom to top getting areas to look for portals
            aasTrace_t trace;
            int areas[10];
            idVec3 points[10];
            trace.maxAreas = 10;
            trace.areas = areas;
            trace.points = points;
            file->Trace( trace, bottom, top );
            // for each area in trace (ignoring start area)
            for ( int q = 1; q < trace.numAreas; q++ )
            {
                aasArea_t area = file->areas[trace.areas[q]];
                // only want portal areas
                if ( area.cluster > 0 ) continue;
                // trace is returning the same area 2 times in a row???
                if ( trace.areas[q] == trace.areas[q-1] ) continue;

                aasPortal_t p = file->portals[-area.cluster];
                // make sure the portal just found is a portal to the bottom cluster (gets set to next bottom)
                // TODO: there is a possibility that an edge point around the bottom
                // is in a different cluster than the bottom center point and a usable portal would be missed
                if ( bottomClusterNum > 0 )
                {
                    if( !( p.clusters[0] == bottomClusterNum || p.clusters[1] == bottomClusterNum ) ) continue;
                }
                // would need an else if < 0 here
                else if ( bottomClusterNum < 0 )   // the bottom is also a portal - need to check that both portals share one cluster
                {
                    aasPortal_t b = file->portals[-bottomClusterNum];
                    if( !( p.clusters[0] == b.clusters[0] || p.clusters[0] == b.clusters[1] || p.clusters[1] == b.clusters[0] || p.clusters[1] == b.clusters[1]) ) continue;
                }
                // if it is 0, the center bottom of plat is out of bounds (d3ctf3 small circle plat)

                if ( !firstPortalNum )   // just save the first portal for later reachability processing
                {
                    firstPortalNum = trace.areas[q];
                    firstPortal = trace.points[q];
                }
                else   // create reaches from portal to portal up the shaft now
                {
                    // found a portal up the elevator shaft to create reaches to
                    aasArea_t bottomArea = file->areas[bottomAreaNum];
                    CreateReachability( start, trace.points[q], bottomAreaNum, trace.areas[q], TFL_ELEVATOR );
                    reachabilityCreated = true;
                } // end else q == 1

                // will be used for top processing
                lastPortal = trace.points[q];
                lastPortalNum = trace.areas[q];

                // reset the bottom area up to the portal just found and keep looking up the shaft from there
                bottomAreaNum = trace.areas[q];
                bottomClusterNum = area.cluster;
                start = trace.points[q];
            } // end for each area

            float x[8], y[8], x_top[8], y_top[8];
            x[0] = bounds[0][0];
            x[1] = center[0];
            x[2] = bounds[1][0];
            x[3] = center[0];
            x[4] = bounds[0][0];
            x[5] = bounds[1][0];
            x[6] = bounds[1][0];
            x[7] = bounds[0][0];

            y[0] = center[1];
            y[1] = bounds[1][1];
            y[2] = center[1];
            y[3] = bounds[0][1];
            y[4] = bounds[1][1];
            y[5] = bounds[1][1];
            y[6] = bounds[0][1];
            y[7] = bounds[0][1];

            // find adjacent areas around the bottom of the plat
            for ( int i = 0; i < 9; i++ )
            {
                if ( i < 8 )    //loops around the outside of the plat
                {
                    start[0] = x[i];
                    start[1] = y[i];
                    start[2] = bottom[2];
                    bottomAreaNum = aas->PointReachableAreaNum( start, DefaultBotBounds(), travelFlags );

                    int k; // TODO: try to eliminate this loop once and see what results
                    for ( k = 0; k < 4; k++ )
                    {
                        if( bottomAreaNum && file->GetArea( bottomAreaNum ).flags & AREA_REACHABLE_WALK )
                        {
                            //gameRenderWorld->DebugCone( colorCyan, start, idVec3( 0, 0, 1 ), 0, 1);
                            break;
                        }
                        start[2] += 2;
                        bottomAreaNum = aas->PointReachableAreaNum( start, DefaultBotBounds(), travelFlags );
                    }
                    // couldn't find a reachable area - no need to process  for this point
                    if ( k >= 4 ) continue;
                    bottomClusterNum = file->areas[bottomAreaNum].cluster;
                }
                else    // check at the middle of the plat (or the portal if there was one)
                {
                    if ( lastPortalNum )
                    {
                        start = lastPortal;
                        bottomAreaNum = lastPortalNum; // TODO: rename
                    }
                    else
                    {
                        start = bottom;
                        bottomAreaNum = aas->PointReachableAreaNum( start, DefaultBotBounds(), travelFlags );
                        if ( !bottomAreaNum ) continue;
                    }
                    bottomClusterNum = file->areas[bottomAreaNum].cluster;
                }

                //look at adjacent areas around the top of the plat make larger steps to outside the plat everytime
                idBounds topBounds = bounds;
                for ( int n = 0; n < 3; n++ )
                {
                    topBounds.ExpandSelf( 2 * n );

                    x_top[0] = topBounds[0][0];
                    x_top[1] = center[0];
                    x_top[2] = topBounds[1][0];
                    x_top[3] = center[0];
                    x_top[4] = topBounds[0][0];
                    x_top[5] = topBounds[1][0];
                    x_top[6] = topBounds[1][0];
                    x_top[7] = topBounds[0][0];
                    y_top[0] = center[1];
                    y_top[1] = topBounds[1][1];
                    y_top[2] = center[1];
                    y_top[3] = topBounds[0][1];
                    y_top[4] = topBounds[1][1];
                    y_top[5] = topBounds[1][1];
                    y_top[6] = topBounds[0][1];
                    y_top[7] = topBounds[0][1];

                    // circle around top plat position looking for areas
                    for ( int j = 0; j < 9; j++ )
                    {
                        // for the last round check for portal
                        if ( j >= 8 )
                        {
                            if ( firstPortalNum )
                            {
                                end = firstPortal;
                                topAreaNum = firstPortalNum;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            end[0] = x_top[j];
                            end[1] = y_top[j];
                            end[2] = top[2];
                            topAreaNum = aas->PointReachableAreaNum( end, DefaultBotBounds(), travelFlags );

                            int l; // trace up a little higher
                            for ( l = 0; l < 8; l++ )
                            {
                                // gameRenderWorld->DebugArrow(colorPurple, start, end, 3, 200000);
                                if ( topAreaNum && file->GetArea( topAreaNum ).flags & AREA_REACHABLE_WALK)
                                {
                                    // found a reachable area near top of plat - trace to see if we can reach it from center
                                    aasTrace_t trace; // TODO: trace with a bounding box (don't know how)???
                                    aas->Trace(trace, top, end);
                                    if ( trace.fraction >= 1 )
                                    {
                                        if ( aas_showElevators.GetBool() )
                                        {
                                            gameRenderWorld->DebugArrow( colorPurple, top, end, 1, 200000 );
                                        }
                                        // TODO: Need to trace down to the floor here and check trace.distance > plat.height (d3dm1)
                                        break;
                                    }
                                    else     // failed trace
                                    {
                                        if ( aas_showElevators.GetBool() )
                                        {
                                            gameRenderWorld->DebugArrow( colorOrange, top, end, 1, 200000 );
                                        }
                                    }
                                }
                                end[2] += 4;
                                topAreaNum = aas->PointReachableAreaNum( end, DefaultBotBounds(), travelFlags );
                            }
                            // couldn't find top area
                            if ( l >= 8 ) continue;
                        }
                        // don't create reachabilities to the same area (worthless)
                        if( bottomAreaNum == topAreaNum ) continue;

                        // area from which we will create a reachability
                        aasArea_t area = file->areas[bottomAreaNum];
                        idReachability *reach;
                        bool create = true;

                        if ( j < 8 )
                        {
                            // if a reachability in the area already points to the area don't create another one
                            for ( reach = area.reach; reach; reach = reach->next )
                            {
                                if ( reach->fromAreaNum == bottomAreaNum && reach->toAreaNum == topAreaNum )
                                {
                                    create = false;
                                    break;
                                }
                            }
                            if ( !create ) continue;
                        }
                        // area to create reachability to
                        aasArea_t dest = file->areas[topAreaNum];

                        // if goal area is portal it needs to be a portal for the bottom cluster
                        if ( dest.cluster < 0 )
                        {
                            if ( bottomClusterNum > 0 )
                            {
                                const aasPortal_t p = file->GetPortal( -dest.cluster );
                                if ( p.clusters[0] != bottomClusterNum && p.clusters[1] != bottomClusterNum )
                                {
                                    continue;
                                }
                            }
                            else   // if they are both portals they need to share a cluster
                            {
                                const aasPortal_t e = file->GetPortal( -dest.cluster );
                                const aasPortal_t s = file->GetPortal( -bottomClusterNum );
                                if (! ( s.clusters[0] == e.clusters[0] || s.clusters[0] == e.clusters[1] || s.clusters[1] == e.clusters[0] || s.clusters[1] == e.clusters[1] ) )
                                {
                                    continue;
                                }
                            }
                        }

                        // if areas are not portals and are in different clusters
                        if ( area.cluster > 0 && dest.cluster > 0 && area.cluster != dest.cluster )
                        {
                            topClusterNum = dest.cluster;
                            create = false;
                            // if any face in the area bounds both clusters make it a portal
                            for ( int j = 0; j < area.numFaces; j++ )
                            {
                                const aasFace_t face = file->GetFace( abs( file->GetFaceIndex( area.firstFace + j ) ) );
                                if ( file->GetArea( face.areas[0]).cluster == bottomClusterNum && file->GetArea( face.areas[1]).cluster== topClusterNum || file->GetArea( face.areas[0]).cluster == topClusterNum && file->GetArea( face.areas[1]).cluster == bottomClusterNum )
                                {
                                    gameLocal.Printf("create a portal here");
                                    create = true;
                                    break;
                                }
                            }
                            // if the bottom area can be made a cluster make it
                            if ( create )
                            {
                                CreatePortal( bottomAreaNum, bottomClusterNum, topClusterNum );
                            }
                            else   // UGLY: check the top area same as we just checked the bottom
                            {
                                // TODO: see if this ever gets used, if not think about getting rid of it for now?
                                // TODO: break out the common code into ConvertAreaToPortal
                                for ( int j = 0; j < dest.numFaces; j++ )
                                {
                                    const aasFace_t face = file->GetFace( abs( file->GetFaceIndex( dest.firstFace + j ) ) );
                                    if ( file->GetArea( face.areas[0]).cluster == bottomClusterNum && file->GetArea( face.areas[1]).cluster== topClusterNum || file->GetArea( face.areas[0]).cluster == topClusterNum && file->GetArea( face.areas[1]).cluster == bottomClusterNum )
                                    {
                                        create = true;
                                        break;
                                    }
                                }
                                if ( create )
                                {
                                    // make the top area a portal
                                    CreatePortal( topAreaNum, topClusterNum, bottomClusterNum);
                                }
                                else   // areas are in different clusters and neither made a portal
                                {
                                    if ( area.rev_reach && dest.reach)
                                    {
                                        bottomNeedPortal = start;
                                        topNeedPortal = end;
                                        needPortal = true;
                                    }
                                    continue;
                                }
                            }
                        }
                        // if got to this point there is a valid to area and from area for a reachability
                        CreateReachability( start, end, bottomAreaNum, topAreaNum, TFL_ELEVATOR );
                        reachabilityCreated = true;
                        //don't go any further to the outside
                        n = 9999;
                    }
                }
            }
            if ( !reachabilityCreated )
            {
                if ( aas_showElevators.GetBool() )
                {
                    // give a little warning visually ;)
                    gameRenderWorld->DebugBounds( colorPink, bounds, vec3_origin, 200000 );
                }
                // could try many different things for a last ditch effort. (d3dm3 plats)
                // for now try and create a portal at top and one reach from bottom (could use list from bottom)
                if ( needPortal )
                {
                    topAreaNum = aas->PointReachableAreaNum( topNeedPortal, DefaultBotBounds(), travelFlags );
                    aasArea_t topArea = file->areas[topAreaNum];
                    topClusterNum = topArea.cluster;
                    assert(topClusterNum > 0);
                    bottomAreaNum = aas->PointReachableAreaNum( bottomNeedPortal, DefaultBotBounds(), travelFlags );
                    bottomClusterNum = file->GetArea( bottomAreaNum ).cluster;
                    assert( bottomClusterNum );
                    // TODO: this wacked out the points way crazy like in some spots
                    //aas->PushPointIntoAreaNum(topAreaNum, topNeedPortal);
                    //aas->PushPointIntoAreaNum(bottomAreaNum, bottomNeedPortal);
                    CreatePortal( topAreaNum, topClusterNum, bottomClusterNum );
                    CreateReachability( bottomNeedPortal, topNeedPortal, bottomAreaNum, topAreaNum, TFL_ELEVATOR );
                }
            }
        }
    } // wow how far in did i go ;)
}


/*
============
BotAASBuild::AddTransporterReachabilities

 cusTom3	- needs some work, but is functional for purpose for now ;)
============
*/
void BotAASBuild::AddTransporterReachabilities( void )
{
    // teleporters - if trigger and destination are in the same area just ignore them???? (not likely but test map had it)

    idEntity *ent;
    // for each entity in the map
    for ( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
    {
        // that is an elevator
        if ( ent->IsType( idTrigger_Multi::Type ) )
        {
            const char *targetName;
            if ( ent->spawnArgs.GetString( "target", "", &targetName ) )
            {
                idEntity *target = gameLocal.FindEntity( targetName );
                if ( target && target->IsType( idPlayerStart::Type ) )
                {
                    int startArea, targetArea, startCluster, targetCluster;
                    bool needsPortal = false;
                    // get the areas the trigger multi bounds is in (just origin for now)
                    startArea = aas->PointReachableAreaNum( ent->GetPhysics()->GetOrigin(), DefaultBotBounds(), travelFlags );
                    // get the areas the info_player_teleport is in (just origin for now) -- expanded bounding box 10 here to get a target area on d3dm1
                    targetArea = aas->PointReachableAreaNum( target->GetPhysics()->GetOrigin(), DefaultBotBounds().Expand( 10 ), travelFlags );
                    // if both are in valid areas (should be)
                    if ( startArea && targetArea )
                    {
                        startCluster = aas->file->GetArea( startArea ).cluster;
                        targetCluster = aas->file->GetArea( targetArea ).cluster;
                        if ( startCluster > 0 )
                        {
                            if ( targetCluster > 0 )
                            {
                                if ( startCluster != targetCluster )
                                {
                                    needsPortal = true;
                                }
                            }
                            else   // target area is a cluster portal
                            {
                                if ( file->GetPortal( -targetCluster ).clusters[0] != startCluster && file->GetPortal( -targetCluster ).clusters[1] != startCluster )
                                {
                                    needsPortal = true;
                                }
                            }
                        }
                        else   // start area is a cluster portal
                        {
                            if ( targetCluster > 0 )
                            {
                                if ( file->GetPortal( -startCluster ).clusters[0] != targetCluster && file->GetPortal( -startCluster ).clusters[1] != targetCluster )
                                {
                                    needsPortal = true;
                                }
                            }
                            else   // both portals
                            {
                                if ( (file->GetPortal( -startCluster ).clusters[0] != file->GetPortal( -targetCluster ).clusters[0]) &&
                                        (file->GetPortal( -startCluster ).clusters[1] != file->GetPortal( -targetCluster ).clusters[0]) &&
                                        (file->GetPortal( -startCluster ).clusters[0] != file->GetPortal( -targetCluster ).clusters[1]) &&
                                        (file->GetPortal( -startCluster ).clusters[1] != file->GetPortal( -targetCluster ).clusters[1]) )
                                {
                                    // need a portal and both are already portals, won't work?
                                    continue;
                                }
                            }
                        }
                    }
                    if ( needsPortal )
                    {
                        CreatePortal( startArea, startCluster, targetCluster );
                    }
                    CreateReachability( ent->GetPhysics()->GetOrigin(), target->GetPhysics()->GetOrigin(), startArea, targetArea, TFL_TELEPORT );
                }
            }
        }
    }

}
/*
============
BotAASBuild::FreeAAS

cusTom3 - this crashes on game/mp/d3dm1???? but not on modified (clipped) version of it
============
*/
void BotAASBuild::FreeAAS()
{
    //OutputAASReachInfo( file );

    // remove the references that point to engine objects - as innefficiently as possible ;)
    for ( int i = 0; i < originalPortals.Num(); i++ )
    {
        // remove the first one from the list, it will move the rest (go backwards at least lazy man)
        file->portals.RemoveIndex( 0 );
    }

    // if file->portals was resized, portals points to a place that has been deleted.
    portals.list = file->portals.list;
    portals.num = file->portals.num;
    portals.size = file->portals.size;
    portals.granularity = file->portals.granularity;

    // reset the aas portals list back to the orignal
    file->portals.list = originalPortals.list;
    file->portals.size = originalPortals.size;
    file->portals.num= originalPortals.num;
    file->portals.granularity = originalPortals.granularity;

    for ( int i = 0; i < originalPortalIndex.Num(); i++ )
    {
        // remove the first one from the list, it will move the rest (go backwards at least lazy man)
        file->portalIndex.RemoveIndex( 0 );
    }

    // if file->portalIndex was resized, portalIndex points to a place that has been deleted.
    portalIndex.list = file->portalIndex.list;
    portalIndex.num = file->portalIndex.num;
    portalIndex.size = file->portalIndex.size;
    portalIndex.granularity = file->portalIndex.granularity;

    // reset the aas portalIndex list back to the orignal
    file->portalIndex.list = originalPortalIndex.list;
    file->portalIndex.size = originalPortalIndex.size;
    file->portalIndex.num= originalPortalIndex.num;
    file->portalIndex.granularity = originalPortalIndex.granularity;

    portals.Clear();
    portalIndex.Clear();

    // for each reachability added unmanipulate aas file
    int numReach = reachabilities.Num();

    for ( int i = 0; i < numReach; i++ )
    {
        idReachability *r = reachabilities[ i ];
        // remove this reach from the list - reaches are added to end of the reach list so 0 represents no other reach in list
        if ( r->number > 0 )
        {
            aas->GetAreaReachability( r->fromAreaNum, r->number - 1 )->next = r->next;
            // removing the reach from the list above f's up the reach numbers - renumber now so if 2 reaches were added the next one through works
            int j = 0;
            for ( idReachability *reach = file->areas[r->fromAreaNum].reach; reach; reach = reach->next, j++ )
            {
                reach->number = j;
            }
        }
        else
        {
            // only one in list - tell area list is now empty
            file->areas[r->fromAreaNum].reach = NULL;
        }

        // rev_reach has no numbers
        if ( r == file->areas[r->toAreaNum].rev_reach )
        {
            // only one in list, tell area
            file->areas[r->toAreaNum].rev_reach = NULL;
        }
        else
        {
            // have to find the reach before me
            for ( idReachability *rev = file->areas[r->toAreaNum].rev_reach; rev; rev = rev->rev_next )
            {
                // if the next reach pointer points to the same thing i do?
                if ( r == rev->rev_next )
                {
                    rev->rev_next = r->rev_next;
                    break;
                }
            }
        }
    }
    // free memory allocated.
    reachabilities.DeleteContents( true );
    file = NULL;
    aas = NULL;

    //OutputAASReachInfo( file );
}

/*
============
BotAASBuild::AllocReachability
============
*/
idReachability * BotAASBuild::AllocReachability( void )
{
    idReachability *r = new idReachability();
    r->areaTravelTimes = NULL;
    r->fromAreaNum = 0;
    r->next = NULL;
    r->number = -1;
    r->rev_next = NULL;
    r->toAreaNum = 0;
    r->travelTime = 1;
    r->travelType = TFL_INVALID;
    reachabilities.Append( r );
    return r;
}









/*
============
BotAASBuild::CreateReachability
============
*/
void BotAASBuild::CreateReachability( const idVec3 &start, const idVec3 &end, int fromAreaNum, int toAreaNum, int travelFlags )
{

    idReachability *r = AllocReachability();
    idReachability *reach;

    // add the reach to the end of the start areas reachability list
    if ( reach = file->areas[fromAreaNum].reach )
    {
        // get to the last reach in the list - he he
        for ( ; reach->next; reach = reach->next ) {}
        reach->next = r;
    }
    else
    {
        // will be only reachability in start area list
        file->areas[fromAreaNum].reach = r;
    }

    r->next = NULL;
    r->start = start;
    r->end = end;
    r->fromAreaNum = fromAreaNum;
    r->toAreaNum = toAreaNum;
    r->travelType = travelFlags;
    r->travelTime = 1; // TODO: distance calculation here
    r->edgeNum = 0; // TODO: elevator height here, portal wouldn't share edge either? make parameter if needed?
    r->areaTravelTimes = NULL;
    if( reach = file->areas[toAreaNum].rev_reach )
    {
        // get to the end of the rev_reach list
        for ( ; reach->rev_next; reach = reach->rev_next ) {}
        reach->rev_next = r;
    }
    else
    {
        // will be only one in list
        file->areas[toAreaNum].rev_reach = r;
    }
    //return r;
}

/*
============
BotAASBuild::CreatePortal

// TODO: either return bool or a reference to the portal so one can check to make sure the portal was created?

============
*/
void BotAASBuild::CreatePortal( int areaNum, int cluster, int joinCluster )
{
    // TODO: could just pass in a reference to the area??
    aasPortal_t portal;
    portal.areaNum = areaNum;
    portal.clusters[0] = cluster;
    portal.clusterAreaNum[0] = aas->ClusterAreaNum( cluster, areaNum );
    portal.clusters[1] = joinCluster;
    portal.clusterAreaNum[1] = file->GetCluster( joinCluster ).numReachableAreas;
    int portalIndex = file->portals.Append( portal );

    // adding an area to the joinCluster, update cluster stats
    file->clusters[joinCluster].numAreas++;
    file->clusters[joinCluster].numPortals++;
    file->clusters[joinCluster].numReachableAreas++;

    // update the files portalIndex
    int insertat = file->clusters[joinCluster].firstPortal;
    file->portalIndex.Insert( portalIndex, insertat );

    // reset the clusters firstPortal member to new locations
    int current = 0;
    for ( int c = 1; c < file->GetNumClusters(); c++ )
    {
        file->clusters[c].firstPortal = current;
        current += file->clusters[c].numPortals;
    }

    // adding a portal to the current cluster, update cluster stats
    file->clusters[cluster].numPortals++;
    insertat = file->clusters[cluster].firstPortal;
    file->portalIndex.Insert( portalIndex, insertat );

    // reset the clusters firstPortal member to new locations (again)
    current = 0;
    for ( int c = 1; c < file->GetNumClusters(); c++ )
    {
        file->clusters[c].firstPortal = current;
        current += file->clusters[c].numPortals;
    }

    // update the area
    file->areas[areaNum].cluster = -portalIndex;
    file->areas[areaNum].contents |= AREACONTENTS_CLUSTERPORTAL;
}



// old stuff below, delete later
void LoadMinimalMapStuff()
{
// idStr mapFileName;
    //unsigned int mapFileCRC;
    //mapFile = new idMapFile();
    //
    //if ( !mapFile->Parse( mapName.c_str())) {
    //	delete mapFile;
    //	mapFile = NULL;
    //	gameLocal.Error( "Couldn't load %s", mapName );
    //}
    //mapFileName = mapFile->GetName();
    //mapFileCRC = mapFile->GetGeometryCRC();
    //
    //// TODO: don't use idMapFile for parsing - what do i need to load for new approach

    //// load the collision map
    //collisionModelManager->LoadMap( mapFile );

    //// TODO: label breaks from where code was taken from here

    //idStr		error;
    //idMapEntity *mapEnt;
    //idDict args;
    //const char	*classname;
    //const char	*spawn;
    //const char  *name;
    //int numEntities = mapFile->GetNumEntities();

    //for ( int i = 1 ; i < numEntities ; i++ ) {
    //	mapEnt = mapFile->GetEntity( i );
    //	args = mapEnt->epairs;

    //	if ( args.GetString( "name", "", &name ) ) {
    //		sprintf( error, " on '%s'", name);
    //	}
    //	gameLocal.CacheDictionaryMedia( &args );
    //	//const idKeyValue *kv;
    //	//kv = args.MatchPrefix( "model" );
    //	//while( kv ) {
    //	//	if ( kv->GetValue().Length() ) {
    //	//	// precache model/animations
    //	//	if ( declManager->FindType( DECL_MODELDEF, kv->GetValue(), false ) == NULL ) {
    //	//		// precache the render model
    //	//		renderModelManager->FindModel( kv->GetValue() );
    //	//		// precache .cm files only
    //	//		collisionModelManager->LoadModel( kv->GetValue(), true );
    //	//	}
    //	//}
    //	//kv = args.MatchPrefix( "model", kv );
    //	//}

    //	args.GetString( "classname", NULL, &classname );
    //	const idDeclEntityDef *def = gameLocal.FindEntityDef( classname, false );
    //	args.SetDefaults( &def->dict );

    //	// check if we should spawn a class object
    //	args.GetString( "spawnclass", NULL, &spawn );
    //	if ( spawn ) {
    //		idTypeInfo	*cls;
    //		idClass		*obj;
    //		cls = idClass::GetClass( spawn );
    //		if ( !cls ) {
    //			gameLocal.Error( "Could not spawn '%s'.  Class '%s' not found %s.", classname, spawn, error.c_str() );
    //		}

    //		obj = cls->CreateInstance();
    //		if ( !obj ) {
    //			gameLocal.Error( "Could not spawn '%s'. Instance could not be created %s.", classname, error.c_str() );
    //		}

    //		obj->CallSpawn();
    //	}
    //}
    //	/*	if ( ent && obj->IsType( idEntity::Type ) ) {
    //			*ent = static_cast<idEntity *>(obj);
    //		}*/

    //	//if ( !InhibitEntitySpawn( args ) ) {
    //		// precache any media specified in the map entity
    //		//CacheDictionaryMedia( &args );

    //		//SpawnEntityDef( args );
    //		//num++;
    //	//} else {
    //	//	inhibit++;
    //	//}
    //mapFileName.SetFileExtension( BOT_AAS );
    //aasFile = AASFileManager->LoadAAS( mapFileName, mapFileCRC );

    //if ( !aasFile ) {
    //	gameLocal.Error( "Couldn't load %s", mapFileName );
    //}
}
void TryToAddLadders_Obselete_1()
{
//// TODO: search the map file entitites for func_plat?
//	for ( int i = 0; i < mapFile->GetNumEntities(); i++ ) {
//		idMapEntity *ent = mapFile->GetEntity( i );
//
//		/*if (!ent) {
//			continue;
//		}*/
//
//		idMapPrimitive *mapPrim;
//		for (int j = 0; j < ent->GetNumPrimitives(); j++ ) {
//			mapPrim = ent->GetPrimitive( j );
//
//			switch( mapPrim->GetType() ) {
//				case idMapPrimitive::TYPE_BRUSH:
//					idMapBrush *brush = static_cast<idMapBrush*>( mapPrim );
//					idMapBrushSide *side;
//					for (int n = 0; n < brush->GetNumSides(); n++ ) {
//						side = brush->GetSide( n );
//						if ( idStr( side->GetMaterial() ).Find( "ladder", false ) > 0 ) { // better check?
//							// yay, found a ladder
//							// TODO: PointReachable with bounds?
//							int area = aasFile->PointAreaNum( side->GetOrigin() );
//							// TODO: should i be checking for negative (portal area)? probably not?
//							aasFile->areas[area].flags |= AREA_LADDER;
//							idPlane plane = side->GetPlane();
//							// TODO: look into face.planeNum
//							// aasFace_t face = FindFaceCorrespondingToPlane(plane);
//
//							// need area number and postion vector for top and bottom of plane to create reachability.
//							// look into face.areas to get area number
//							gameLocal.Printf( "found a ladder in area %d at %s\n", area, side->GetOrigin().ToString());
//							idReachability reach;
//							//reach.start = face.n
//							//aasFile->AddReachability( area, reach);
//						}
//					}
//					break;
//				// TODO: can patches be ladders?
//				/*case idMapPrimitive::TYPE_PATCH:
//					static_cast<idMapPatch*>(mapPrim)->Write( fp, i, origin );
//					break;*/
//			}
//		}
//	}
//
//	// search primitives for ladders? - need to loop through entities i guess
//
//	int faceNum;
//	const aasFace_t *face;
//
//	for ( int i = 0; i < aasFile->GetNumAreas(); i++ ) {
//
//		// PROOF THAT AREA_LADDER NEEDS TO BE SET
//		if ( aasFile->GetArea( i ).flags & AREA_LADDER ) {
//			gameLocal.Printf("area %d has a ladder!", i);
//		}
//
//		for (int j = 0; j < aasFile->GetArea( i ).numFaces; j++ ) {
//			faceNum = aasFile->GetFaceIndex( aasFile->GetArea( i ).firstFace + j );
//			face = &aasFile->GetFace( abs(faceNum) );
//
//			if ( !(face->flags & FACE_LADDER ) ) {
//				continue;
//			}
//			gameLocal.Printf("face %d in area %d flagged as a ladder!", faceNum, i);
//
//
//		//for ( idReachability *reach = file->GetArea( i ).reach; reach; reach = reach->next ) {
//		//	if (reach->travelType & TFL_LADDER || reach->travelType == TFL_LADDER /* cause i am stupid */) {
//		//		// gameLocal.Printf("found ladder reachability");
//		//	}
//		//}
//		}
//	}
//
//	// to make sure i am not too drunk
//	for ( int i = 0; i < aasFile->GetNumFaces(); i++ ) {
//		if ( !(aasFile->GetFace( i ).flags & FACE_LADDER ) ) {
//			continue;
//		}
//			gameLocal.Printf("face %d in area %d flagged as a ladder!", faceNum, i);
//	}
//
//
//
//	gameLocal.Printf("area count for map: %d\n", aasFile->GetNumAreas());
}
void TryToAddElevators_Obselete_1()
{
//idStr cls;
//	// TODO: search the map file entitites for func_plat
//	for ( int i = 0; i < mapFile->GetNumEntities(); i++ ) {
//		idMapEntity *ent = mapFile->GetEntity( i );
//		ent->epairs.GetString( "classname", NULL, cls);
//		// TODO: what about other classnames for elevators
//		if( cls && cls == "func_plat") {
//			idVec3 top = ent->epairs.GetVector("origin");
//			idVec3 height(0, 0, ent->epairs.GetFloat("height"));
//			idVec3 bottom = top - height;
//
//			// TODO: environment sample surrounding areas?
//			// maybe spawn an idPlat and use things like idMover_Binary.pos1 and .pos2
//
//			//gameLocal.Printf("found idPlat top at %s in area %d\n", top.ToString(), aasFile->PointReachableAreaNum(top, bounds, TFL_WALK|TFL_WALKOFFLEDGE|TFL_JUMP, TFL_INVALID));
//			//gameLocal.Printf("found idPlat bottom at %s in area %d\n", bottom.ToString(), aasFile->PointReachableAreaNum(bottom, bounds, TFL_WALK|TFL_WALKOFFLEDGE|TFL_JUMP, TFL_INVALID));
//
//		}
//	}
}

void AddHardCodedReachabilityToAASPlat()
{
//// add the hardcoded reachability
    //aasArea_t *area = &aas->file->areas[3];
    //idReachability *reach = area->reach;
    //idReachability *r = AllocReachability();
    //if ( reach ) {
    //	// get to the last reach in the list - easier way?
    //	for ( ; reach->next; reach = reach->next ) {}
    //	// r->rev_next = reach;
    //	r->number = reach->number + 1;
    //	reach->next = r;
    //}
    //else {
    //	// r->rev_next = NULL;
    //	r->number = 1;
    //	area->reach = r;
    //}
    //r->start = idVec3(152, 102, 0); // should be start
    //r->end = idVec3(88.5, 102, 167);
    //r->fromAreaNum = 3;
    //r->toAreaNum = 8;
    //r->next = NULL;
    //r->travelType = TFL_ELEVATOR;
    //r->travelTime = 1;
    //r->edgeNum = 0;

    //// insert reach as rev reach and link into areas rev reach list
    //area = &aas->file->areas[r->toAreaNum];
    //r->rev_next = area->rev_reach;
    //area->rev_reach = r;
}



/*
============
idAASFindCover::idAASFindCover
============
*/
idAASFindCover::idAASFindCover(const idVec3 &hideFromPos)
{
    int			numPVSAreas;
    idBounds	bounds(hideFromPos - idVec3(16, 16, 0), hideFromPos + idVec3(16, 16, 64));

    // setup PVS
    numPVSAreas = gameLocal.pvs.GetPVSAreas(bounds, PVSAreas, idEntity::MAX_PVS_AREAS);
    hidePVS		= gameLocal.pvs.SetupCurrentPVS(PVSAreas, numPVSAreas);
}

/*
============
idAASFindCover::~idAASFindCover
============
*/
idAASFindCover::~idAASFindCover()
{
    gameLocal.pvs.FreeCurrentPVS(hidePVS);
}

/*
============
idAASFindCover::TestArea
============
*/
bool idAASFindCover::TestArea(const idAAS *aas, int areaNum)
{
    idVec3	areaCenter;
    int		numPVSAreas;
    int		PVSAreas[ idEntity::MAX_PVS_AREAS ];

    areaCenter = aas->AreaCenter(areaNum);
    areaCenter[ 2 ] += 1.0f;

    numPVSAreas = gameLocal.pvs.GetPVSAreas(idBounds(areaCenter).Expand(16.0f), PVSAreas, idEntity::MAX_PVS_AREAS);

    if (!gameLocal.pvs.InCurrentPVS(hidePVS, PVSAreas, numPVSAreas))
    {
        return true;
    }

    return false;
}

/*
============
idAASFindAreaOutOfRange::idAASFindAreaOutOfRange
============
*/
idAASFindAreaOutOfRange::idAASFindAreaOutOfRange(const idVec3 &targetPos, float maxDist)
{
    this->targetPos		= targetPos;
    this->maxDistSqr	= maxDist * maxDist;
}

/*
============
idAASFindAreaOutOfRange::TestArea
============
*/
bool idAASFindAreaOutOfRange::TestArea(const idAAS *aas, int areaNum)
{
    const idVec3 &areaCenter = aas->AreaCenter(areaNum);
    trace_t	trace;
    float dist;

    dist = (targetPos.ToVec2() - areaCenter.ToVec2()).LengthSqr();

    if ((maxDistSqr > 0.0f) && (dist < maxDistSqr))
    {
        return false;
    }

    gameLocal.TracePoint( NULL, trace, targetPos, areaCenter + idVec3(0.0f, 0.0f, 1.0f), MASK_OPAQUE, NULL);

    if (trace.fraction < 1.0f)
    {
        return false;
    }

    return true;
}


/*
============
botAASFindAttackPosition::botAASFindAttackPosition
TinMan: Tweaked from idAI specific, a lot simpler though.
============
*/
botAASFindAttackPosition::botAASFindAttackPosition( const idPlayer *self, const idMat3 &gravityAxis, idEntity *target, const idVec3 &targetPos, const idVec3 &eyeOffset )
{
    int	numPVSAreas;

    this->target		= target;
    this->targetPos		= targetPos;
    this->eyeOffset		= eyeOffset;
    this->self			= self;
    this->gravityAxis	= gravityAxis;

    excludeBounds		= idBounds( idVec3( -64.0, -64.0f, -8.0f ), idVec3( 64.0, 64.0f, 64.0f ) );
    excludeBounds.TranslateSelf( self->GetPhysics()->GetOrigin() );

    // setup PVS
    idBounds bounds( targetPos - idVec3( 16, 16, 0 ), targetPos + idVec3( 16, 16, 64 ) );
    numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, idEntity::MAX_PVS_AREAS );
    targetPVS	= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

/*
============
botAASFindAttackPosition::~botAASFindAttackPosition
============
*/
botAASFindAttackPosition::~botAASFindAttackPosition()
{
    gameLocal.pvs.FreeCurrentPVS( targetPVS );
}

/*
============
botAASFindAttackPosition::TestArea
============
*/
bool botAASFindAttackPosition::TestArea( const idAAS *aas, int areaNum )
{
    idVec3	dir;
    idVec3	local_dir;
    idVec3	fromPos;
    idMat3	axis;
    idVec3	areaCenter;
    int		numPVSAreas;
    int		PVSAreas[ idEntity::MAX_PVS_AREAS ];

    idVec3	targetPos1;
    idVec3	targetPos2;
    trace_t		tr;
    idVec3 toPos;

    areaCenter = aas->AreaCenter( areaNum );
    areaCenter[ 2 ] += 1.0f;

    if ( excludeBounds.ContainsPoint( areaCenter ) )
    {
        // too close to where we already are
        return false;
    }

    numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( areaCenter ).Expand( 16.0f ), PVSAreas, idEntity::MAX_PVS_AREAS );
    if ( !gameLocal.pvs.InCurrentPVS( targetPVS, PVSAreas, numPVSAreas ) )
    {
        return false;
    }


    // calculate the world transform of the launch position
    dir = targetPos - areaCenter;
    gravityAxis.ProjectVector( dir, local_dir );
    local_dir.z = 0.0f;
    local_dir.ToVec2().Normalize();
    axis = local_dir.ToMat3();
    fromPos = areaCenter + eyeOffset * axis;

    if ( target->IsType( idActor::Type ) )
    {
        botAi::GetAIAimTargets( static_cast<idActor *>( target ), target->GetPhysics()->GetOrigin(), targetPos1, targetPos2 );
    }
    else
    {
        targetPos1 = target->GetPhysics()->GetAbsBounds().GetCenter();
        targetPos2 = targetPos1;
    }

    toPos = targetPos1;

    gameLocal.TracePoint( NULL, tr, fromPos, toPos, MASK_SOLID, self );
    if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == target ) )
    {
        return true;
    }

    return false;
}
#endif
