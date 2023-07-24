// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DeployMask.h"
#include "../Game_local.h"
#include "../Player.h"

#include "../../decllib/declTypeHolder.h"









const idEventDef EV_MaskEditSession_Open( "openMask", '\0', DOC_TEXT( "Sets the internal mask handle to that of the given name." ), 1, NULL, "s", "name", "Name of the mask key to use." );
const idEventDef EV_MaskEditSession_UpdateProjection( "updateProjection", '\0', DOC_TEXT( "Updates the projected decals for the specified position." ), 1, "A $decl:material$ must be specified first using $event:setDecalMaterial$ before calling this.", "v", "position", "Location to project the decals around." );
const idEventDef EV_MaskEditSession_SetDecalMaterial( "setDecalMaterial", '\0', DOC_TEXT( "Sets the $decl:material$ that will be used to draw the decals." ),1, NULL, "s", "material", "Name of the $decl:material$ to use." );
const idEventDef EV_MaskEditSession_SetStampSize( "setStampSize", '\0', DOC_TEXT( "Sets how big the square used for modifying the mask will be." ), 1, NULL, "d", "size", "Width of the sides of the square, in mask units." );
const idEventDef EV_MaskEditSession_Stamp( "stamp", 'b', DOC_TEXT( "Applies the given state to the mask around the position specified." ), 3, NULL, "v", "position", "Location to stamp around.", "b", "save", "Whether it should save the mask straight away.", "b", "state", "State to apply to the mask." );
const idEventDef EV_MaskEditSession_SaveAll( "saveAll", '\0', DOC_TEXT( "Saves all modified masks." ), 0, NULL );

/*
===============================================================================

	sdDeployMaskEditSession

===============================================================================
*/

CLASS_DECLARATION( idClass, sdDeployMaskEditSession )
	EVENT( EV_MaskEditSession_UpdateProjection,	sdDeployMaskEditSession::Event_UpdateProjection )
	EVENT( EV_MaskEditSession_Open,				sdDeployMaskEditSession::Event_OpenMask )
	EVENT( EV_MaskEditSession_SetDecalMaterial,	sdDeployMaskEditSession::Event_SetDecalMaterial )
	EVENT( EV_MaskEditSession_SetStampSize,		sdDeployMaskEditSession::Event_SetStampSize )
	EVENT( EV_MaskEditSession_Stamp,			sdDeployMaskEditSession::Event_Stamp )
	EVENT( EV_MaskEditSession_SaveAll,			sdDeployMaskEditSession::Event_SaveAll )
END_CLASS

/*
==============
sdDeployMaskEditSession::sdDeployMaskEditSession
==============
*/
sdDeployMaskEditSession::sdDeployMaskEditSession( void ) {
	maskHandle		= -1;
	decalHandle		= -1;
	scriptObject	= gameLocal.program->AllocScriptObject( this, gameLocal.program->GetDefaultType() );
	decalMaterial	= NULL;
	stampSize		= 1;
}

/*
==============
sdDeployMaskEditSession::~sdDeployMaskEditSession
==============
*/
sdDeployMaskEditSession::~sdDeployMaskEditSession( void ) {
	FreeDecals();
	gameLocal.program->FreeScriptObject( scriptObject );
}

/*
==============
sdDeployMaskEditSession::FreeDecals
==============
*/
void sdDeployMaskEditSession::FreeDecals( void ) {
	if ( decalHandle != -1 ) {
		gameLocal.FreeLoggedDecal( decalHandle );
		decalHandle = -1;
	}
}

/*
==============
sdDeployMaskEditSession::GetMask
==============
*/
sdDeployMaskInstance* sdDeployMaskEditSession::GetMask( const idVec3& position ) {
	const sdPlayZone* playZone = gameLocal.GetPlayZone( position, sdPlayZone::PZF_DEPLOYMENT );
	if ( playZone ) {
		return const_cast< sdDeployMaskInstance* >( playZone->GetMask( maskHandle ) );
	}
	return NULL;
}

/*
==============
sdDeployMaskEditSession::GetHeightMap
==============
*/
const sdHeightMapInstance* sdDeployMaskEditSession::GetHeightMap( const idVec3& position ) {
	const sdPlayZone* playZone = gameLocal.GetPlayZone( position, sdPlayZone::PZF_HEIGHTMAP );
	if ( playZone ) {
		return &playZone->GetHeightMap();
	}
	return NULL;
}

/*
==============
sdDeployMaskEditSession::GetExtents
==============
*/
void sdDeployMaskEditSession::GetExtents( const idVec3& position, const sdDeployMaskInstance& mask, sdDeployMask::extents_t& extents ) {
	idBounds bounds( position );
	mask.CoordsForBounds( bounds, extents );

	// the grid
	int maxX, maxY;
	mask.GetDimensions( maxX, maxY );

	extents.maxx = Min( maxX, extents.maxx + ( stampSize - 1 ) );
	extents.maxy = Min( maxY, extents.maxy + ( stampSize - 1 ) );
}

/*
==============
sdDeployMaskEditSession::SetMaskState
==============
*/
bool sdDeployMaskEditSession::SetMaskState( const idVec3& position, bool save, bool state ) {
	sdDeployMaskInstance* mask = GetMask( position );
	if ( mask == NULL ) {
		return false;
	}

	sdDeployMask::extents_t extents;
	GetExtents( position, *mask, extents );

	bool changed = false;

	for ( int i = extents.minx; i <= extents.maxx; i++ ) {
		for ( int j = extents.miny; j <= extents.maxy; j++ ) {
			if ( ( mask->GetState( i, j ) != 0 ) == state ) {
				continue;
			}
			changed = true;
			mask->SetState( i, j, state );
		}
	}

	if ( changed && save ) {
		mask->WriteTGA();
	}

	return changed;
}

/*
==============
sdDeployMaskEditSession::UpdateProjection
==============
*/
void sdDeployMaskEditSession::UpdateProjection( const idVec3& position ) {
	if ( !decalMaterial ) {
		return;
	}

	if ( decalHandle == -1 ) {
		decalHandle = gameLocal.RegisterLoggedDecal( decalMaterial );
	}
	gameLocal.ResetLoggedDecal( decalHandle );

	gameDecalInfo_t* decalInfo = gameLocal.GetLoggedDecal( decalHandle );

	sdDeployMaskInstance* mask = GetMask( position );
	const sdHeightMapInstance* heightMap = GetHeightMap( position );

	if ( mask != NULL && mask->IsValid() && heightMap != NULL ) {
		sdDeployMask::extents_t extents;
		GetExtents( position, *mask, extents );

		float depth = 512.0f;

		int maxX, maxY;
		mask->GetDimensions( maxX, maxY );

		sdDeployMask::extents_t expandedExtents;

		expandedExtents.minx = Max( 0, extents.minx - 2 );
		expandedExtents.miny = Max( 0, extents.miny - 2 );

		expandedExtents.maxx = Min( maxX, extents.maxx + 2 );
		expandedExtents.maxy = Min( maxY, extents.maxy + 2 );

		idList< const idMaterial* > megaTextureMaterials;
		const idStrList& megaTextureMaterialNames = gameLocal.GetMapInfo().GetMegatextureMaterials();
		for ( int i = 0; i < megaTextureMaterialNames.Num(); i++ ) {
			megaTextureMaterials.Append( declHolder.FindMaterial( megaTextureMaterialNames[ i ] ) );
		}

		idFixedWinding winding;
		
		int spawnID = WORLD_SPAWN_ID;

		for ( int i = expandedExtents.minx; i <= expandedExtents.maxx; i++ ) {
			for ( int j = expandedExtents.miny; j <= expandedExtents.maxy; j++ ) {
				gameDecalInfo_t* info = decalInfo;
				if ( !info ) {
					continue;
				}

				sdDeployMask::extents_t localExtents;
				localExtents.minx = i;
				localExtents.maxx = i;
				localExtents.miny = j;
				localExtents.maxy = j;

				idBounds bounds;
				mask->GetBounds( localExtents, bounds, heightMap );

				idVec3 top = bounds.GetCenter();
				top[ 2 ] = bounds.GetMaxs()[ 2 ];				

				deployResult_t localResult = mask->IsValid( localExtents );

				idVec4 localColor;
				switch ( localResult ) {
					case DR_CLEAR:
						localColor = colorGreen;
						break;
					default:
					case DR_FAILED:
						localColor = colorDkRed;
						break;
				}

				if ( !( ( i >= extents.minx ) && ( i <= extents.maxx ) && ( j >= extents.miny ) && ( j <= extents.maxy ) ) ) {
					localColor.x *= 0.3f;
					localColor.y *= 0.3f;
					localColor.z *= 0.3f;
				}

				winding.Clear();
				winding += idVec5( idVec3( bounds.GetMins()[ 0 ], bounds.GetMins()[ 1 ], bounds.GetMins()[ 2 ] - depth ), idVec2( 0.0f, 0.0f ) );
				winding += idVec5( idVec3( bounds.GetMins()[ 0 ], bounds.GetMaxs()[ 1 ], bounds.GetMins()[ 2 ] - depth ), idVec2( 0.0f, 1.0f ) );
				winding += idVec5( idVec3( bounds.GetMaxs()[ 0 ], bounds.GetMaxs()[ 1 ], bounds.GetMins()[ 2 ] - depth ), idVec2( 1.0f, 1.0f ) );
				winding += idVec5( idVec3( bounds.GetMaxs()[ 0 ], bounds.GetMins()[ 1 ], bounds.GetMins()[ 2 ] - depth ), idVec2( 1.0f, 0.0f ) );

				gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0, 0, 64.f + depth ), true, localColor, info->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );
			}
		}
	}
}

/*
==============
sdDeployMaskEditSession::Event_UpdateProjection
==============
*/
void sdDeployMaskEditSession::Event_UpdateProjection( const idVec3& position ) {
	UpdateProjection( position );
}

/*
==============
sdDeployMaskEditSession::Event_OpenMask
==============
*/
void sdDeployMaskEditSession::Event_OpenMask( const char* maskName ) {
	maskHandle = gameLocal.GetDeploymentMask( maskName );
	if ( maskHandle == -1 ) {
		gameLocal.Warning( "sdDeployMaskEditSession::Event_OpenMask Mask %s not found", maskName );
		return;
	}
}

/*
==============
sdDeployMaskEditSession::Event_SetDecalMaterial
==============
*/
void sdDeployMaskEditSession::Event_SetDecalMaterial( const char* materialName ) {
	decalMaterial = gameLocal.declMaterialType[ materialName ];
}

/*
==============
sdDeployMaskEditSession::Event_SetStampSize
==============
*/
void sdDeployMaskEditSession::Event_SetStampSize( int size ) {
	if ( size < 1 ) {
		return;
	}
	stampSize = size;
}

/*
==============
sdDeployMaskEditSession::Event_Stamp
==============
*/
void sdDeployMaskEditSession::Event_Stamp( const idVec3& position, bool save, bool state ) {
	sdProgram::ReturnInteger( SetMaskState( position, save, state ) );
}

/*
==============
sdDeployMaskEditSession::Event_SaveAll
==============
*/
void sdDeployMaskEditSession::Event_SaveAll( void ) {
	gameLocal.SavePlayZoneMasks();
}
