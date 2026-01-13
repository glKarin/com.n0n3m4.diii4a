// TODO: from effective c++:
// TODO: declare a private copy constructor and operator = (don't see this anywhere else in the sdk however
// TODO: do i need aasLocal - should use the protocol class aas if possible
#ifndef __BOTAASBUILD_H__
#define __BOTAASBUILD_H__

#include "../ai/AAS_local.h"
/*
===============================================================================

	BotAASBuild.h

	Adds reachability information to the existing aas file.

===============================================================================
*/

//class idAASLocal;

class BotAASBuild
{

public:
    BotAASBuild( void );
    // this is NOT a base class to be inherited from
    ~BotAASBuild( void );
    void						Init( idAASLocal * local );
    // for SysCmd if ever saving aas files - not necessary for now
    //static void				AddReachabilities_f( const idCmdArgs &args );
    // called from InitFromNewMap, no saved maps for now sorry, i don't need em ;)
    void						AddReachabilities( void );
    // try is right ;(
    void						TryToAddLadders( void );
    void						FreeAAS( void );

private:
    idAASLocal *				aas;
    idAASFile *					file;
    idList<aasPortal_t>			portals;
    idList<aasPortal_t>			originalPortals;
    idList<aasIndex_t>			portalIndex;
    idList<aasIndex_t>			originalPortalIndex;
    idList<idReachability *>	reachabilities;

    static const int			travelFlags = TFL_BARRIERJUMP|TFL_CROUCH|TFL_ELEVATOR|TFL_LADDER|TFL_WALKOFFLEDGE|TFL_WALK;

private:
    void						AddElevatorReachabilities( void );
    void						AddTransporterReachabilities( void );
    idReachability *			AllocReachability( void );
    const idBounds &			DefaultBotBounds( void );
    void						CreateReachability( const idVec3 &start, const idVec3 &end, int startArea, int endArea, int travelFlags );
    void						CreatePortal( int areaNum, int cluster, int joinCluster );
};

ID_INLINE BotAASBuild::BotAASBuild( void )
{
    reachabilities.Resize( 128, 128 );
}

ID_INLINE BotAASBuild::~BotAASBuild( void )
{
    // clear out the original so it doesn't try and delete the server list ;)
    originalPortals.list = NULL;
    originalPortals.size = 0;
    originalPortals.num = 0;

    originalPortalIndex.list = NULL;
    originalPortalIndex.size = 0;
    originalPortalIndex.num = 0;

    // if these are not cleared correctly in FreeAAS this will error, dont like this, but hey, whatever
    portals.Clear();
    portalIndex.Clear();

    reachabilities.DeleteContents( true );
}

ID_INLINE const idBounds & BotAASBuild::DefaultBotBounds( void )
{
    return file->GetSettings().boundingBoxes[0];
}
#endif /* !__BOTAASBUILD_H__ */