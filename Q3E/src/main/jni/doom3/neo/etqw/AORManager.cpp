// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Player.h"

/*
===============================================================================

	sdAORManagerLocal

===============================================================================
*/

/*
================
sdAORManagerLocal::sdAORManagerLocal
================
*/
sdAORManagerLocal::sdAORManagerLocal( void ) {
	pvsHandle.i = -1;
}

/*
================
sdAORManagerLocal::~sdAORManagerLocal
================
*/
sdAORManagerLocal::~sdAORManagerLocal( void ) {
	Shutdown();
}

/*
================
sdAORManagerLocal::Init
================
*/
void sdAORManagerLocal::Init( void ) {
}

/*
================
sdAORManagerLocal::OnMapLoad
================
*/
void sdAORManagerLocal::OnMapLoad( void ) {
	pvsHandle.i = -1;
}

/*
================
sdAORManagerLocal::Shutdown
================
*/
void sdAORManagerLocal::Shutdown( void ) {
	FreePVS();
}

/*
================
sdAORManagerLocal::FreePVS
================
*/
void sdAORManagerLocal::FreePVS( void ) {
	if ( pvsHandle.i != -1 ) {
		gameLocal.pvs.FreeCurrentPVS( pvsHandle );
		pvsHandle.i = -1;
	}
}

/*
================
sdAORManagerLocal::CheckEntity
================
*/
bool sdAORManagerLocal::CheckEntity( idEntity* ent, int spread ) {
	int marker = gameLocal.GetLastMarker( snapShotPlayerIndex, ent->entityNumber );
	if ( marker == -1 ) {
		return true;
	}

	int timeSinceMark = gameLocal.time - marker;
	if ( timeSinceMark > spread ) {
		return true;
	}

	return false;
}

/*
================
sdAORManagerLocal::Setup
================
*/
void sdAORManagerLocal::Setup( void ) {
}

/*
================
sdAORManagerLocal::SetClient
================
*/
void sdAORManagerLocal::SetClient( idPlayer* client ) {
	if ( client == NULL ) {
		return;
	}

	idVec3 origin;
	idMat3 axis;
	client->GetAORView( origin, axis );

	cachedViewOrg	= origin.ToVec2();
	cachedViewDir	= axis[ 0 ].ToVec2();
	
	FreePVS();
	pvsHandle = gameLocal.pvs.SetupCurrentPVS( client->firstPersonViewOrigin );

	if ( client->GetSniperAOR() ) {
		ringScaleFactor	= 5.f;
		priorityScale	= 1.f / Square( 15.f );
		priorityDot		= idMath::Cos( DEG2RAD( 60.f / 2.f ) );
	} else {
		ringScaleFactor	= 1.f;
		priorityScale	= 0.f;
	}
	ringScale		= Square( ringScaleFactor );
}

/*
================
sdAORManagerLocal::SetPosition
================
*/
void sdAORManagerLocal::SetPosition( const idVec3& position ) {
	cachedViewOrg	= position.ToVec2();

	FreePVS();
	pvsHandle = gameLocal.pvs.SetupCurrentPVS( position );

	ringScaleFactor	= 1.f;
	priorityScale	= 0.f;
	ringScale		= Square( ringScaleFactor );
}

/*
================
sdAORManagerLocal::GetDistSqForPoint
================
*/
float sdAORManagerLocal::GetDistSqForPoint( const idVec3& otherPos ) {
	idVec2 dist = cachedViewOrg - otherPos.ToVec2();
	float lenSquared = dist.LengthSqr() * ringScale;

	if ( priorityScale > 0.f ) {
		dist.NormalizeFast();
		float dot = dist * cachedViewDir;
		if ( dot < priorityDot ) {
			lenSquared *= priorityScale;
		}
	}

	if ( pvsHandle.i != -1 ) {
		if ( !gameLocal.pvs.InCurrentPVS( pvsHandle, otherPos ) ) {
			lenSquared *= Square( net_aorPVSScale.GetFloat() );
		}
	}

	return lenSquared;
}


/*
================
sdAORManagerLocal::UpdateEntityAORFlags
================
*/
void sdAORManagerLocal::UpdateEntityAORFlags( idEntity* ent, const idVec3& otherPos, bool& shouldUpdate ) {
	if ( ent->aorLayout == NULL ) {
		ent->aorFlags = 0;
		ent->aorPacketSpread = 0;
		ent->aorDistanceSqr = 0.0f;
		return;
	}

	ent->aorDistanceSqr = GetDistSqForPoint( otherPos );
	ent->aorPacketSpread = SEC2MS( ent->aorLayout->GetSpreadForDistanceSqr( ent->aorDistanceSqr ) );
	if ( ent->aorPacketSpread < gameLocal.msec ) {
		ent->aorPacketSpread = gameLocal.msec;
	}
	ent->aorFlags = ent->aorLayout->GetFlagsForDistanceSqr( ent->aorDistanceSqr );

	if ( shouldUpdate ) {
		shouldUpdate = CheckEntity( ent, ent->aorPacketSpread );
	}
}

/*
================
sdAORManagerLocal::GetFlagsForPoint
================
*/
int sdAORManagerLocal::GetFlagsForPoint( const sdDeclAOR* aorLayout, const idVec3& otherPos ) {
	if ( aorLayout == NULL ) {
		return 0;
	}

	float distSq = GetDistSqForPoint( otherPos );
	return aorLayout->GetFlagsForDistanceSqr( distSq );
}

/*
// TODO: Reinstate debug drawing for new method
const idVec4* colorTable[ 13 ] = {
	&colorRed,
	&colorGreen,
	&colorBlue,
	&colorMagenta,
	&colorCyan,
	&colorOrange,
	&colorPurple,
	&colorPink,
	&colorBrown,
	&colorLtGrey,
	&colorMdGrey,
	&colorDkGrey,


	&colorYellow,
};
*/

/*
================
sdAORManagerLocal::DebugDraw
================
*/
void sdAORManagerLocal::DebugDraw( int clientNum ) {
// TODO: Reinstate debug drawing for new method
/*	idPlayer* player = gameLocal.GetClient( clientNum );
	if ( !player ) {
		return;
	}

	SetClient( player );

	const idVec3& org = player->GetPhysics()->GetOrigin();
	idVec3 dir( 0.f, 0.f, 1.f );

	if ( priorityScale > 0.f ) {
		idFrustum f;

		idVec3 tempDir( cachedViewDir.x, cachedViewDir.y, 0.f );
		tempDir.NormalizeFast();
		idVec3 tempOrg( cachedViewOrg.x, cachedViewOrg.y, 0.f );

		f.SetAxis( tempDir.ToMat3() );
		f.SetOrigin( tempOrg );
		f.SetSize( 0.f, MAX_WORLD_SIZE, MAX_WORLD_SIZE * idMath::Tan( DEG2RAD( 60.f / 2.f ) ), 0.00001f );
		gameRenderWorld->DebugFrustum( colorWhite, f );
	}*/
}

/*
================
sdAORManagerLocal::DebugDrawEntities
================
*/
void sdAORManagerLocal::DebugDrawEntities( int clientNum ) {
// TODO: Reinstate debug drawing for new method
/*	idPlayer* player = gameLocal.GetClient( clientNum );
	if ( !player ) {
		return;
	}

	idTypeInfo* etype = NULL;
	if ( *net_clientAORFilter.GetString() ) {
		etype = idClass::GetClass( net_clientAORFilter.GetString() );
	}

	SetClient( player );

	idEntity* ent;
	for( ent = gameLocal.networkedEntities.Next(); ent != NULL; ent = ent->networkNode.Next() ) {
		if ( etype && !ent->IsType( *etype ) ) {
			continue;
		}

		idBounds bounds			= ent->GetPhysics()->GetBounds();
		const idVec3& org		= ent->GetPhysics()->GetOrigin();
		const idMat3& axes		= ent->GetPhysics()->GetAxis();

		idStr message;

		gameRenderWorld->DebugBounds( *colorTable[ ent->entityNumber % 13 ], bounds, org, axes );
		
		if ( ent->IsNetSynced() ) {
			message += "Visible";

			if ( ent->snapshotPVSFlags & PVS_VISIBLE ) {
				message += "*";
			}

			message += "\n";

			message += "Broadcast";

			if ( ent->snapshotPVSFlags & PVS_BROADCAST ) {
				message += "*";
			}

			message += "\n";
		} else {
			message += "Not Networked\n";
		}

		float scale = 0.15f;

		idVec3 v = org - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
		v.Normalize();

		if ( ( v * gameLocal.GetLocalPlayer()->firstPersonViewAxis[ 0 ] ) > 0.95f ) {
			scale = 0.75f;
		}

		gameRenderWorld->DrawText( message.c_str(), org, scale, colorWhite, gameLocal.GetLocalPlayer()->firstPersonViewAxis );
	}*/
}

/*
================
sdAORManagerLocal::SetSnapShotPlayer
================
*/
void sdAORManagerLocal::SetSnapShotPlayer( idPlayer* player ) {
	snapShotPlayerIndex = player ? player->entityNumber : -1;
}
