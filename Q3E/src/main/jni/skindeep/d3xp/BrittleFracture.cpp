/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "framework/DeclEntityDef.h"
#include "renderer/ModelManager.h"
#include "idlib/LangDict.h"

#include "ai/AI.h"
#include "Fx.h"
#include "Player.h"
#include "Mover.h"

#include "BrittleFracture.h"
#include "bc_glasspiece.h"

CLASS_DECLARATION( idEntity, idBrittleFracture )
	EVENT( EV_Activate,			idBrittleFracture::Event_Activate )
	EVENT( EV_Touch,			idBrittleFracture::Event_Touch )
	EVENT( EV_SpectatorTouch,	idBrittleFracture::Event_SpectatorTouch)
END_CLASS

const int SHARD_ALIVE_TIME	= 1000;
const int SHARD_FADE_START	= 500;

// Beyond the MIN portal fade distance, the portal will start to fade out
// Beyond the MAX portal fade distance, the portal will be opaque and block vis
const int MAX_PORTAL_FADE_DIST = 1024;
const int MIN_PORTAL_FADE_DIST = 768;

// Experimentally-derived value that roughly corresponds to how much 'closer' the player's camera appears to be when zoomed in.
// This is used to ensure that faded-out portals will re-open at an intuitive distance if the player zooms in on them.
const float ZOOM_CORRECTION_FACTOR = 689.0f; 

static const char *brittleFracture_SnapshotName = "_BrittleFracture_Snapshot_";

/*
================
idBrittleFracture::idBrittleFracture
================
*/
idBrittleFracture::idBrittleFracture( void ) {
	material = NULL;
	decalMaterial = NULL;
	crackedMaterial = NULL;
	decalSize = 0.0f;
	maxShardArea = 0.0f;
	maxShatterRadius = 0.0f;
	minShatterRadius = 0.0f;
	linearVelocityScale = 0.0f;
	angularVelocityScale = 0.0f;
	shardMass = 0.0f;
	density = 0.0f;
	friction = 0.0f;
	bouncyness = 0.0f;
	fxFracture.Clear();

	bounds.Clear();
	disableFracture = false;

	lastRenderEntityUpdate = -1;
	changed = false;

	fl.networkSync = true;

#ifdef _D3XP
	isXraySurface = false;
#endif

	//BC
	hasActivatedTargets = false;
	assignedRoom = NULL;

	portal = NULL;
}

/*
================
idBrittleFracture::~idBrittleFracture
================
*/
idBrittleFracture::~idBrittleFracture( void ) {
	int i;

	for ( i = 0; i < shards.Num(); i++ ) {
		shards[i]->decals.DeleteContents( true );
		delete shards[i];
	}

	// make sure the render entity is freed before the model is freed
	FreeModelDef();
	renderModelManager->FreeModel( renderEntity.hModel );
}

/*
================
idBrittleFracture::Save
================
*/
void idBrittleFracture::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( assignedRoom ); // idEntityPtr<idEntity> assignedRoom

	savefile->WriteMaterial( material ); // const idMaterial * material
	savefile->WriteMaterial( decalMaterial ); // const idMaterial * decalMaterial
	savefile->WriteMaterial( crackedMaterial ); // const idMaterial * crackedMaterial
	savefile->WriteFloat( decalSize ); // float decalSize
	savefile->WriteFloat( maxShardArea ); // float maxShardArea
	savefile->WriteFloat( maxShatterRadius ); // float maxShatterRadius
	savefile->WriteFloat( minShatterRadius ); // float minShatterRadius
	savefile->WriteFloat( linearVelocityScale ); // float linearVelocityScale
	savefile->WriteFloat( angularVelocityScale ); // float angularVelocityScale
	savefile->WriteFloat( shardMass ); // float shardMass
	savefile->WriteFloat( density ); // float density
	savefile->WriteFloat( friction ); // float friction
	savefile->WriteFloat( bouncyness ); // float bouncyness
	savefile->WriteString( fxFracture ); // idString fxFracture

	savefile->WriteBool( isXraySurface ); // bool isXraySurface

	savefile->WriteStaticObject( idBrittleFracture::physicsObj ); // idPhysics_StaticMulti physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	savefile->WriteInt( shards.Num() ); // idList<shard_t *> shards
	for (int i = 0; i < shards.Num(); i++ ) {
		savefile->WriteWinding( shards[i]->winding );

		savefile->WriteInt( shards[i]->decals.Num() );
		for (int j = 0; j < shards[i]->decals.Num(); j++ ) {
			savefile->WriteWinding( *shards[i]->decals[j] );
		}

		savefile->WriteInt( shards[i]->neighbours.Num() );
		for (int j = 0; j < shards[i]->neighbours.Num(); j++ ) {
			int index = shards.FindIndex(shards[i]->neighbours[j]);
			assert(index != -1);
			savefile->WriteInt( index );
		}

		savefile->WriteInt( shards[i]->edgeHasNeighbour.Num() );
		for (int j = 0; j < shards[i]->edgeHasNeighbour.Num(); j++ ) {
			savefile->WriteBool( shards[i]->edgeHasNeighbour[j] );
		}

		savefile->WriteInt( shards[i]->droppedTime );
		savefile->WriteInt( shards[i]->islandNum );
		savefile->WriteBool( shards[i]->atEdge );
		savefile->WriteStaticObject( shards[i]->physicsObj );
	}

	savefile->WriteBounds( bounds ); // idBounds bounds
	savefile->WriteBool( disableFracture ); // bool disableFracture
	savefile->WriteInt( lastRenderEntityUpdate ); // mutable int lastRenderEntityUpdate
	savefile->WriteBool( changed ); // mutable bool changed

	savefile->WriteBool( hasActivatedTargets ); // bool hasActivatedTargets
	savefile->WriteDict( &glasspieceDict ); // idDict glasspieceDict

	savefile->WriteInt( portal ); // int portal
	savefile->WriteInt( portalFadeState ); // int portalFadeState
}

/*
================
idBrittleFracture::Restore
================
*/
void idBrittleFracture::Restore( idRestoreGame *savefile ) {
	renderEntity.hModel = renderModelManager->AllocModel();
	renderEntity.hModel->InitEmpty( brittleFracture_SnapshotName );
	renderEntity.callback = idBrittleFracture::ModelCallback;
	renderEntity.noShadow = true;
	renderEntity.noSelfShadow = true;
	renderEntity.noDynamicInteractions = false;

	savefile->ReadObject( assignedRoom ); // idEntityPtr<idEntity> assignedRoom

	savefile->ReadMaterial( material ); // const idMaterial * material
	savefile->ReadMaterial( decalMaterial ); // const idMaterial * decalMaterial
	savefile->ReadMaterial( crackedMaterial ); // const idMaterial * crackedMaterial
	savefile->ReadFloat( decalSize ); // float decalSize
	savefile->ReadFloat( maxShardArea ); // float maxShardArea
	savefile->ReadFloat( maxShatterRadius ); // float maxShatterRadius
	savefile->ReadFloat( minShatterRadius ); // float minShatterRadius
	savefile->ReadFloat( linearVelocityScale ); // float linearVelocityScale
	savefile->ReadFloat( angularVelocityScale ); // float angularVelocityScale
	savefile->ReadFloat( shardMass ); // float shardMass
	savefile->ReadFloat( density ); // float density
	savefile->ReadFloat( friction ); // float friction
	savefile->ReadFloat( bouncyness ); // float bouncyness
	savefile->ReadString( fxFracture ); // idString fxFracture

	savefile->ReadBool( isXraySurface ); // bool isXraySurface

	savefile->ReadStaticObject( physicsObj ); // idPhysics_StaticMulti physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys) {
		RestorePhysics( &physicsObj );
	}

	int num;
	savefile->ReadInt( num ); // idList<shard_t *> shards
	shards.SetNum( num );
	for (int i = 0; i < shards.Num(); i++ ) {
		shards[i] = new shard_t;
	}
	for (int i = 0; i < shards.Num(); i++ ) {
		savefile->ReadWinding( shards[i]->winding );

		savefile->ReadInt( num );
		shards[i]->decals.SetNum( num );
		int j = 0;
		for (int j = 0; j < shards[i]->decals.Num(); j++ ) {
			shards[i]->decals[j] = new idFixedWinding;
			savefile->ReadWinding( *shards[i]->decals[j] );
		}

		savefile->ReadInt( num );
		shards[i]->neighbours.SetNum( num );
		for (int j = 0; j < shards[i]->neighbours.Num(); j++ ) {
			int index;
			savefile->ReadInt( index );
			assert(index != -1);
			shards[i]->neighbours[j] = shards[index];
		}

		savefile->ReadInt( num );
		shards[i]->edgeHasNeighbour.SetNum( num );
		for (j = 0; j < shards[i]->edgeHasNeighbour.Num(); j++ ) {
			savefile->ReadBool( shards[i]->edgeHasNeighbour[j] );
		}

		savefile->ReadInt( shards[i]->droppedTime );
		savefile->ReadInt( shards[i]->islandNum );
		savefile->ReadBool( shards[i]->atEdge );
		savefile->ReadStaticObject( shards[i]->physicsObj );
		if ( shards[i]->droppedTime < 0 ) {
			shards[i]->clipModel = physicsObj.GetClipModel( i );
		} else {
			shards[i]->clipModel = shards[i]->physicsObj.GetClipModel();
		}
	}

	savefile->ReadBounds( bounds ); // idBounds bounds
	savefile->ReadBool( disableFracture ); // bool disableFracture
	savefile->ReadInt( lastRenderEntityUpdate ); // mutable int lastRenderEntityUpdate
	savefile->ReadBool( changed ); // mutable bool changed

	savefile->ReadBool( hasActivatedTargets ); // bool hasActivatedTargets
	savefile->ReadDict( &glasspieceDict ); // idDict glasspieceDict

	savefile->ReadInt( portal ); // int portal
	savefile->ReadInt( portalFadeState ); // int portalFadeState
}

/*
================
idBrittleFracture::Spawn
================
*/
void idBrittleFracture::Spawn( void ) {

	// get shard properties
	decalMaterial = declManager->FindMaterial( spawnArgs.GetString( "mtr_decal" ) );
	crackedMaterial = declManager->FindMaterial( spawnArgs.GetString( "mtr_cracked" ) );
	decalSize = spawnArgs.GetFloat( "decalSize", "40" );
	maxShardArea = spawnArgs.GetFloat( "maxShardArea", "600" );
	maxShardArea = idMath::ClampFloat( 100, 10000, maxShardArea );
	maxShatterRadius = spawnArgs.GetFloat( "maxShatterRadius", "60" );
	minShatterRadius = spawnArgs.GetFloat( "minShatterRadius", "10" );
	linearVelocityScale = spawnArgs.GetFloat( "linearVelocityScale", "0.3" );
	angularVelocityScale = spawnArgs.GetFloat( "angularVelocityScale", "40" );
	fxFracture = spawnArgs.GetString( "fx" );

	// get rigid body properties
	shardMass = spawnArgs.GetFloat( "shardMass", "20" );
	shardMass = idMath::ClampFloat( 0.001f, 1000.0f, shardMass );
	spawnArgs.GetFloat( "density", "0.1", density );
	density = idMath::ClampFloat( 0.001f, 1000.0f, density );
	spawnArgs.GetFloat( "friction", "0.4", friction );
	friction = idMath::ClampFloat( 0.0f, 1.0f, friction );
	spawnArgs.GetFloat( "bouncyness", "0.01", bouncyness );
	bouncyness = idMath::ClampFloat( 0.0f, 1.0f, bouncyness );

	disableFracture = spawnArgs.GetBool( "disableFracture", "0" );
	health = spawnArgs.GetInt( "health", "20" );
	fl.takedamage = true;

	// FIXME: set "bleed" so idProjectile calls AddDamageEffect
	spawnArgs.SetBool( "bleed", 1 );

#ifdef _D3XP
	// check for xray surface
	if ( 1 ) {
		const idRenderModel *model = renderEntity.hModel;

		isXraySurface = false;

		for ( int i = 0; i < model->NumSurfaces(); i++ ) {
			const modelSurface_t *surf = model->Surface( i );

			if ( idStr( surf->shader->GetName() ) == "textures/smf/window_scratch" ) {
				isXraySurface = true;
				break;
			}
		}
	}
#endif

	CreateFractures( renderEntity.hModel );

	FindNeighbours();

	renderEntity.hModel = renderModelManager->AllocModel();
	renderEntity.hModel->InitEmpty( brittleFracture_SnapshotName );
	renderEntity.callback = idBrittleFracture::ModelCallback;
	renderEntity.noShadow = true;
	renderEntity.noSelfShadow = true;
	renderEntity.noDynamicInteractions = false;

	hasActivatedTargets = false;


	const idDeclEntityDef *glasspieceDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_glasspiece"), false);
	if (glasspieceDef)
	{
		const char *debrisClipmodelName;
		idTraceModel trm;

		glasspieceDict = glasspieceDef->dict;

		debrisClipmodelName = glasspieceDef->dict.GetString("clipmodel");

		if (debrisClipmodelName[0])
		{
			//Precache the clipmodel for debris chunks.
			collisionModelManager->TrmFromModel(debrisClipmodelName, trm);
		}
	}
	else
	{
		glasspieceDict.Clear();
	}


	//Precache the shard models.
	gameLocal.FindEntityDef("def_glasspiece", false);

	FindPortalHandle();
	BecomeActive(TH_THINK); // SW: Need to be active so we can handle our portal opening/closing at a distance
}

/*
================
idBrittleFracture::FindPortalHandle
================
*/
void idBrittleFracture::FindPortalHandle()
{
	// SW: Mostly shamelessly stolen from the idVacuumSeparatorEntity code
	idBounds bounds = idBounds(spawnArgs.GetVector("origin")).Expand(spawnArgs.GetInt("radius", "16"));
	portal = gameRenderWorld->FindPortal(bounds);

	if (!portal && 	spawnArgs.GetBool("portalfade", "1"))
	{
		idVec3 origin = this->GetPhysics()->GetOrigin();
		gameLocal.Warning("idBrittleFracture '%s' doesn't contact a portal at %.1f %.1f %.1f", this->GetName(), origin.x, origin.y, origin.z);
	}
}

/*
================
idBrittleFracture::AddShard
================
*/
void idBrittleFracture::AddShard( idClipModel *clipModel, idFixedWinding &w ) {
	shard_t *shard = new shard_t;
	shard->clipModel = clipModel;
	shard->droppedTime = -1;
	shard->winding = w;
	shard->decals.Clear();
	shard->edgeHasNeighbour.AssureSize( w.GetNumPoints(), false );
	shard->neighbours.Clear();
	shard->atEdge = false;
	shards.Append( shard );
}

/*
================
idBrittleFracture::RemoveShard
================
*/
void idBrittleFracture::RemoveShard( int index ) {
	int i;

	delete shards[index];
	shards.RemoveIndex( index );
	physicsObj.RemoveIndex( index );

	for ( i = index; i < shards.Num(); i++ ) {
		shards[i]->clipModel->SetId( i );
	}
}

/*
================
idBrittleFracture::UpdateRenderEntity
================
*/
bool idBrittleFracture::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const {
	int i, j, k, n, msec, numTris, numDecalTris;
	float fade;
	dword packedColor;
	srfTriangles_t *tris, *decalTris;
	modelSurface_t surface;
	idDrawVert *v;
	idPlane plane;
	idMat3 tangents;

	// this may be triggered by a model trace or other non-view related source,
	// to which we should look like an empty model
	if ( !renderView ) {
		return false;
	}

	// don't regenerate it if it is current
	if ( lastRenderEntityUpdate == gameLocal.time || !changed ) {
		return false;
	}

	lastRenderEntityUpdate = gameLocal.time;
	changed = false;

	numTris = 0;
	numDecalTris = 0;
	for ( i = 0; i < shards.Num(); i++ ) {
		n = shards[i]->winding.GetNumPoints();
		if ( n > 2 ) {
			numTris += n - 2;
		}
		for ( k = 0; k < shards[i]->decals.Num(); k++ ) {
			n = shards[i]->decals[k]->GetNumPoints();
			if ( n > 2 ) {
				numDecalTris += n - 2;
			}
		}
	}

	// FIXME: re-use model surfaces
	renderEntity->hModel->InitEmpty( brittleFracture_SnapshotName );

	// allocate triangle surfaces for the fractures and decals
	tris = renderEntity->hModel->AllocSurfaceTriangles( numTris * 3, material->ShouldCreateBackSides() ? numTris * 6 : numTris * 3 );
	decalTris = renderEntity->hModel->AllocSurfaceTriangles( numDecalTris * 3, decalMaterial->ShouldCreateBackSides() ? numDecalTris * 6 : numDecalTris * 3 );

	for ( i = 0; i < shards.Num(); i++ )
	{
		const idVec3 &origin = shards[i]->clipModel->GetOrigin();
		const idMat3 &axis = shards[i]->clipModel->GetAxis();

		fade = 1.0f;
		if ( shards[i]->droppedTime >= 0 )
		{
			msec = gameLocal.time - shards[i]->droppedTime - SHARD_FADE_START;
			if ( msec > 0 )
			{
				fade = 1.0f - (float) msec / ( SHARD_ALIVE_TIME - SHARD_FADE_START );

				if (!shards[i]->hasSpawnedGlasspiece && spawnArgs.GetBool("has_glasspiece", "1"))
				{
					idEntity *glassPiece;

					shards[i]->hasSpawnedGlasspiece = true;

					gameLocal.SpawnEntityDef(glasspieceDict, &glassPiece, false);

					if (glassPiece && glassPiece->IsType(idGlassPiece::Type))
					{
						idGlassPiece *debris = static_cast<idGlassPiece *>(glassPiece);
						debris->Create(shards[i]->physicsObj.GetOrigin() + idVec3(0,0,1), shards[i]->physicsObj.GetAxis(0));

						// SW: Inherit velocity from the shard that spawned us, with a little variance to stop it looking too 'neat'
						debris->GetPhysics()->SetLinearVelocity(shards[i]->physicsObj.GetLinearVelocity() * (0.9 + gameLocal.random.RandomFloat() / 5.0f)); 

						//debris->Create(NULL, shards[i]->physicsObj.GetOrigin(), shards[i]->physicsObj.GetAxis(0));
						//debris->Launch();
					}
				}
			}
		}

		packedColor = PackColor( idVec4( renderEntity->shaderParms[ SHADERPARM_RED ] * fade,
										renderEntity->shaderParms[ SHADERPARM_GREEN ] * fade,
										renderEntity->shaderParms[ SHADERPARM_BLUE ] * fade,
										fade ) );

		
		const idWinding &winding = shards[i]->winding;

		winding.GetPlane( plane );
		tangents = ( plane.Normal() * axis ).ToMat3();

		for ( j = 2; j < winding.GetNumPoints(); j++ ) {

			v = &tris->verts[tris->numVerts++];
			v->Clear();
			v->xyz = origin + winding[0].ToVec3() * axis;
			v->st[0] = winding[0].s;
			v->st[1] = winding[0].t;
			v->normal = tangents[0];
			v->tangents[0] = tangents[1];
			v->tangents[1] = tangents[2];
			v->SetColor( packedColor );

			v = &tris->verts[tris->numVerts++];
			v->Clear();
			v->xyz = origin + winding[j-1].ToVec3() * axis;
			v->st[0] = winding[j-1].s;
			v->st[1] = winding[j-1].t;
			v->normal = tangents[0];
			v->tangents[0] = tangents[1];
			v->tangents[1] = tangents[2];
			v->SetColor( packedColor );

			v = &tris->verts[tris->numVerts++];
			v->Clear();
			v->xyz = origin + winding[j].ToVec3() * axis;
			v->st[0] = winding[j].s;
			v->st[1] = winding[j].t;
			v->normal = tangents[0];
			v->tangents[0] = tangents[1];
			v->tangents[1] = tangents[2];
			v->SetColor( packedColor );

			tris->indexes[tris->numIndexes++] = tris->numVerts - 3;
			tris->indexes[tris->numIndexes++] = tris->numVerts - 2;
			tris->indexes[tris->numIndexes++] = tris->numVerts - 1;

			if ( material->ShouldCreateBackSides() ) {

				tris->indexes[tris->numIndexes++] = tris->numVerts - 2;
				tris->indexes[tris->numIndexes++] = tris->numVerts - 3;
				tris->indexes[tris->numIndexes++] = tris->numVerts - 1;
			}
		}

		for ( k = 0; k < shards[i]->decals.Num(); k++ ) {
			const idWinding &decalWinding = *shards[i]->decals[k];

			for ( j = 2; j < decalWinding.GetNumPoints(); j++ ) {

				v = &decalTris->verts[decalTris->numVerts++];
				v->Clear();
				v->xyz = origin + decalWinding[0].ToVec3() * axis;
				v->st[0] = decalWinding[0].s;
				v->st[1] = decalWinding[0].t;
				v->normal = tangents[0];
				v->tangents[0] = tangents[1];
				v->tangents[1] = tangents[2];
				v->SetColor( packedColor );

				v = &decalTris->verts[decalTris->numVerts++];
				v->Clear();
				v->xyz = origin + decalWinding[j-1].ToVec3() * axis;
				v->st[0] = decalWinding[j-1].s;
				v->st[1] = decalWinding[j-1].t;
				v->normal = tangents[0];
				v->tangents[0] = tangents[1];
				v->tangents[1] = tangents[2];
				v->SetColor( packedColor );

				v = &decalTris->verts[decalTris->numVerts++];
				v->Clear();
				v->xyz = origin + decalWinding[j].ToVec3() * axis;
				v->st[0] = decalWinding[j].s;
				v->st[1] = decalWinding[j].t;
				v->normal = tangents[0];
				v->tangents[0] = tangents[1];
				v->tangents[1] = tangents[2];
				v->SetColor( packedColor );

				decalTris->indexes[decalTris->numIndexes++] = decalTris->numVerts - 3;
				decalTris->indexes[decalTris->numIndexes++] = decalTris->numVerts - 2;
				decalTris->indexes[decalTris->numIndexes++] = decalTris->numVerts - 1;

				if ( decalMaterial->ShouldCreateBackSides() ) {

					decalTris->indexes[decalTris->numIndexes++] = decalTris->numVerts - 2;
					decalTris->indexes[decalTris->numIndexes++] = decalTris->numVerts - 3;
					decalTris->indexes[decalTris->numIndexes++] = decalTris->numVerts - 1;
				}
			}
		}
	}

	tris->tangentsCalculated = true;
	decalTris->tangentsCalculated = true;

	SIMDProcessor->MinMax( tris->bounds[0], tris->bounds[1], tris->verts, tris->numVerts );
	SIMDProcessor->MinMax( decalTris->bounds[0], decalTris->bounds[1], decalTris->verts, decalTris->numVerts );

	memset( &surface, 0, sizeof( surface ) );
	surface.shader = health <= 0 ? crackedMaterial : material;
	surface.id = 0;
	surface.geometry = tris;
	renderEntity->hModel->AddSurface( surface );

	memset( &surface, 0, sizeof( surface ) );
	surface.shader = decalMaterial;
	surface.id = 1;
	surface.geometry = decalTris;
	renderEntity->hModel->AddSurface( surface );

	return true;
}

/*
================
idBrittleFracture::ModelCallback
================
*/
bool idBrittleFracture::ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	const idBrittleFracture *ent;

	ent = static_cast<idBrittleFracture *>(gameLocal.entities[ renderEntity->entityNum ]);
	if ( !ent ) {
		gameLocal.Error( "idBrittleFracture::ModelCallback: callback with NULL game entity" );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
idBrittleFracture::Present
================
*/
void idBrittleFracture::Present() {

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	renderEntity.bounds = bounds;
	renderEntity.origin.Zero();
	renderEntity.axis.Identity();

	// force an update because the bounds/origin/axis may stay the same while the model changes
	renderEntity.forceUpdate = true;

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}

	changed = true;
}

/*
================
idBrittleFracture::Think
================
*/
void idBrittleFracture::Think( void ) {
	int i, startTime, endTime, droppedTime;
	shard_t *shard;
	bool atRest = true, fading = false;

	// remove overdue shards
	for ( i = 0; i < shards.Num(); i++ ) {
		droppedTime = shards[i]->droppedTime;
		if ( droppedTime != -1 ) {
			if ( gameLocal.time - droppedTime > SHARD_ALIVE_TIME )
			{


				RemoveShard( i );
				i--;				
			}

			fading = true;
		}
	}

	// remove the entity when nothing is visible
	if ( !shards.Num() )
	{
		//PostEventMS( &EV_Remove, 0 ); //BC don't delete the window; keep it so that the ingress UI can use it.
		return;
	}

	if ( thinkFlags & TH_PHYSICS ) {

		startTime = gameLocal.previousTime;
		endTime = gameLocal.time;

		// run physics on shards
		for ( i = 0; i < shards.Num(); i++ ) {
			shard = shards[i];

			if ( shard->droppedTime == -1 ) {
				continue;
			}

			shard->physicsObj.Evaluate( endTime - startTime, endTime );

			if ( !shard->physicsObj.IsAtRest() ) {
				atRest = false;
			}
		}

		if ( atRest ) {
			BecomeInactive( TH_PHYSICS );
		} else {
			BecomeActive( TH_PHYSICS );
		}
	}

	if ( !atRest || bounds.IsCleared() ) {
		bounds.Clear();
		for ( i = 0; i < shards.Num(); i++ ) {
			bounds.AddBounds( shards[i]->clipModel->GetAbsBounds() );
		}
	}

	if ( fading ) {
		BecomeActive( TH_UPDATEVISUALS );
	}

	UpdatePortalVisibility();
	RunPhysics();
	Present();
}

// Ensures we only lockSurfaces once per UpdatePortalVisibility call (and only if absolutely necessary)
void CachedQueryLockSurfaces(int* lockSurfaces)
{
	*lockSurfaces = (*lockSurfaces < 0) ? cvarSystem->GetCVarBool("r_lockSurfaces") : *lockSurfaces;
}

/*
================
idBrittleFracture::UpdatePortalVisibility
================
*/
void idBrittleFracture::UpdatePortalVisibility()
{
	if (!spawnArgs.GetBool("portalfade", "1"))
		return;

	// Confirm that yes, we do have a portal, 
	// and it is currently relevant to what the player might be seeing
	if (portal && gameLocal.InPlayerPVS(this) && health > 0)
	{
		int lockSurfaces = -1; // This is a bool, but we don't want to query it unless absolutely necessary. -1 is our 'null' value which tells CachedQueryLockSurfaces() to look it up
		idPlayer* player = gameLocal.GetLocalPlayer();
		idWeapon* weapon = player->weapon.GetEntity();

		int portalState = gameRenderWorld->GetPortalState(portal);
		bool isBlockingView = portalState & PS_BLOCK_VIEW;
		float distanceFromPlayer = (player->firstPersonViewOrigin - this->GetPhysics()->GetOrigin()).LengthFast();
		
		// If we zoom in on a distant window that's faded out, it should adopt the same translucency that we would expect if we had moved closer.
		// This 'zoom distance correction' is a rough estimation of that, based on (questionable) experimentation.
		float currentZoomFactor = 1.0f - ((player->CalcFov(true) - (float)(weapon->GetZoomFov())) / (player->DefaultFov() - (float)(weapon->GetZoomFov()))); // 0.0 to 1.0, represents how 'zoomed in' the view is
		float zoomDistanceCorrection = currentZoomFactor * ZOOM_CORRECTION_FACTOR;
		distanceFromPlayer -= zoomDistanceCorrection;

		// Sanity check since we're subtracting the zoom distance correction above
		if (distanceFromPlayer < 0.0f)
			distanceFromPlayer = 0.0f;

		// We have all the information we need, now check if the portal state needs to change.
		if (isBlockingView && distanceFromPlayer < MAX_PORTAL_FADE_DIST)
		{
			CachedQueryLockSurfaces(&lockSurfaces);
			if (!lockSurfaces)
			{
				// Player has moved close enough that we need to open the portal
				gameLocal.SetPortalState(portal, portalState & ~PS_BLOCK_VIEW);
			}
		}
		if (!isBlockingView && distanceFromPlayer >= MAX_PORTAL_FADE_DIST)
		{
			CachedQueryLockSurfaces(&lockSurfaces);
			if (!lockSurfaces)
			{
				// Player has moved far enough away that we can close the portal
				gameLocal.SetPortalState(portal, portalState | PS_BLOCK_VIEW);
			}
		}
		
		// Figure out if we need to update the visuals.
		// (i.e. are we currently lerping, or have we crossed a boundary?
		int newPortalFadeState;
		if (distanceFromPlayer < MAX_PORTAL_FADE_DIST)
		{
			if (distanceFromPlayer >= MIN_PORTAL_FADE_DIST)
			{
				newPortalFadeState = PORTAL_FADE_FADING;
			}
			else
			{
				newPortalFadeState = PORTAL_FADE_TRANSPARENT;
			}
		}
		else
		{
			newPortalFadeState = PORTAL_FADE_OPAQUE;
		}

		if (newPortalFadeState == PORTAL_FADE_FADING || newPortalFadeState != portalFadeState)
		{
			CachedQueryLockSurfaces(&lockSurfaces);
			if (!lockSurfaces)
			{
				// Now we know we need to update the visuals.
				BecomeActive(TH_UPDATEVISUALS);
				portalFadeState = newPortalFadeState;

				// Calculate how opaque the window should be (clamp if we're above the max distance or below the min distance)
				float fadeLerp = idMath::ClampFloat(0.0f, 1.0f, (distanceFromPlayer - MIN_PORTAL_FADE_DIST) / (MAX_PORTAL_FADE_DIST - MIN_PORTAL_FADE_DIST));

				// Feed the lerp value into the material
				this->GetRenderEntity()->shaderParms[SHADERPARM_ALPHA] = fadeLerp; // We can't use the first three shader params -- they're important for decal rendering
			}
		}
	}
	else
	{
		// Fallback for windows without portals: never fade
		this->GetRenderEntity()->shaderParms[SHADERPARM_ALPHA] = 0;
	}

	Present();
}

/*
================
idBrittleFracture::ApplyImpulse
================
*/
void idBrittleFracture::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) {

	if ( id < 0 /*|| id >= shards.Num()*/ )
	{
		return;
	}

	if (id >= shards.Num())
	{
		id = shards.Num() - 1;
	}

	if ( shards[id]->droppedTime != -1 ) {
		shards[id]->physicsObj.ApplyImpulse( 0, point, impulse );
	} else if ( health <= 0 && !disableFracture ) {
		Shatter( point, impulse, gameLocal.time );

		//BC if hit glass, then activate its targets.
		if (!hasActivatedTargets)
		{
			hasActivatedTargets = true;
			PrintEventMessage(ent);
			ActivateTargets(NULL);
			SetDoorVacuumLock();
			DoSplinePull();			
		}
	}
}

/*
================
idBrittleFracture::AddForce
================
*/
void idBrittleFracture::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force )
{

	//BC make this more lenient to shattering.



	if ( id < 0  /*|| id >= shards.Num() */)
	{
		return;
	}

	if (id >= shards.Num())
	{
		id = shards.Num() - 1;
	}

	if ( shards[id]->droppedTime != -1 )
	{
		shards[id]->physicsObj.AddForce( 0, point, force );
	}
	else if ( health <= 0 && !disableFracture )
	{
		Shatter( point, force , gameLocal.time );

		//BC if hit glass, then activate its targets.
		if (!hasActivatedTargets)
		{
			hasActivatedTargets = true;
			PrintEventMessage(ent);
			ActivateTargets(NULL);
			SetDoorVacuumLock();
			DoSplinePull();
		}
	}
}

/*
================
idBrittleFracture::ProjectDecal
================
*/
void idBrittleFracture::ProjectDecal( const idVec3 &point, const idVec3 &dir, const int time, const char *damageDefName ) {
	int i, j, bits, clipBits;
	float a, c, s;
	idVec2 st[MAX_POINTS_ON_WINDING];
	idVec3 origin;
	idMat3 axis, axistemp;
	idPlane textureAxis[2];

	if ( gameLocal.isServer ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteFloat( point[0] );
		msg.WriteFloat( point[1] );
		msg.WriteFloat( point[2] );
		msg.WriteFloat( dir[0] );
		msg.WriteFloat( dir[1] );
		msg.WriteFloat( dir[2] );
		ServerSendEvent( EVENT_PROJECT_DECAL, &msg, true, -1 );
	}

	if ( time >= gameLocal.time ) {
		// try to get the sound from the damage def


		//BC for some reason this doesn't work?? So, just get the sound from the fracture glass itself.
		//const idDeclEntityDef *damageDef = NULL;
		//const idSoundShader *sndShader = NULL;
		//if ( damageDefName ) {
		//	damageDef = gameLocal.FindEntityDef( damageDefName, false );
		//	if ( damageDef ) {				
		//		sndShader = declManager->FindSound( damageDef->dict.GetString( "snd_glass", "" ) );
		//	}
		//}
		//
		//if ( sndShader ) {
		//	StartSoundShader( sndShader, SND_CHANNEL_ANY, 0, false, NULL );
		//} else {
		//	StartSound( "snd_impact", SND_CHANNEL_ANY, 0, false, NULL );
		//}

		StartSound("snd_glass", SND_CHANNEL_ANY, 0, false, NULL);
	}

	a = gameLocal.random.RandomFloat() * idMath::TWO_PI;
	c = cos( a );
	s = -sin( a );

	axis[2] = -dir;
	axis[2].Normalize();
	axis[2].NormalVectors( axistemp[0], axistemp[1] );
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * s;
	axis[1] = axistemp[ 0 ] * s + axistemp[ 1 ] * -c;

	textureAxis[0] = axis[0] * ( 1.0f / decalSize );
	textureAxis[0][3] = -( point * textureAxis[0].Normal() ) + 0.5f;

	textureAxis[1] = axis[1] * ( 1.0f / decalSize );
	textureAxis[1][3] = -( point * textureAxis[1].Normal() ) + 0.5f;

	for ( i = 0; i < shards.Num(); i++ ) {
		idFixedWinding &winding = shards[i]->winding;
		origin = shards[i]->clipModel->GetOrigin();
		axis = shards[i]->clipModel->GetAxis();
		float d0, d1;

		clipBits = -1;
		for ( j = 0; j < winding.GetNumPoints(); j++ ) {
			idVec3 p = origin + winding[j].ToVec3() * axis;

			st[j].x = d0 = textureAxis[0].Distance( p );
			st[j].y = d1 = textureAxis[1].Distance( p );

			bits = FLOATSIGNBITSET( d0 );
			d0 = 1.0f - d0;
			bits |= FLOATSIGNBITSET( d1 ) << 2;
			d1 = 1.0f - d1;
			bits |= FLOATSIGNBITSET( d0 ) << 1;
			bits |= FLOATSIGNBITSET( d1 ) << 3;

			clipBits &= bits;
		}

		if ( clipBits ) {
			continue;
		}

		idFixedWinding *decal = new idFixedWinding;
		shards[i]->decals.Append( decal );

		decal->SetNumPoints( winding.GetNumPoints() );
		for ( j = 0; j < winding.GetNumPoints(); j++ ) {
			(*decal)[j].ToVec3() = winding[j].ToVec3();
			(*decal)[j].s = st[j].x;
			(*decal)[j].t = st[j].y;
		}
	}

	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idBrittleFracture::DropShard
================
*/
void idBrittleFracture::DropShard( shard_t *shard, const idVec3 &point, const idVec3 &dir, const float impulse, const int time ) {
	int i, j, clipModelId;
	float dist, f;
	idVec3 dir2, origin;
	idMat3 axis;
	shard_t *neighbour;

	// don't display decals on dropped shards
	shard->decals.DeleteContents( true );

	// remove neighbour pointers of neighbours pointing to this shard
	for ( i = 0; i < shard->neighbours.Num(); i++ ) {
		neighbour = shard->neighbours[i];
		for ( j = 0; j < neighbour->neighbours.Num(); j++ ) {
			if ( neighbour->neighbours[j] == shard ) {
				neighbour->neighbours.RemoveIndex( j );
				break;
			}
		}
	}

	// remove neighbour pointers
	shard->neighbours.Clear();

	// remove the clip model from the static physics object
	clipModelId = shard->clipModel->GetId();
	physicsObj.SetClipModel( NULL, 1.0f, clipModelId, false );

	origin = shard->clipModel->GetOrigin();
	axis = shard->clipModel->GetAxis();

	// set the dropped time for fading
	shard->droppedTime = time;
	shard->hasSpawnedGlasspiece = false;

	dir2 = origin - point;
	dist = dir2.Normalize();
	f = dist > maxShatterRadius ? 1.0f : idMath::Sqrt( dist - minShatterRadius ) * ( 1.0f / idMath::Sqrt( maxShatterRadius - minShatterRadius ) );

	// setup the physics
	shard->physicsObj.SetSelf( this );
	shard->physicsObj.SetClipModel( shard->clipModel, density );
	shard->physicsObj.SetMass( shardMass );
	shard->physicsObj.SetOrigin( origin );
	shard->physicsObj.SetAxis( axis );
	shard->physicsObj.SetBouncyness( bouncyness );
	shard->physicsObj.SetFriction( 0.6f, 0.6f, friction );
	
	//shard->physicsObj.SetGravity( gameLocal.GetGravity() );	
	if (gameLocal.GetAirlessAtPoint(origin))
	{
		idVec3 flingDir = (origin - point);
		flingDir.NormalizeFast();
		shard->physicsObj.SetGravity( flingDir * 64);

		if (developer.GetInteger() > 0)
		{
			gameRenderWorld->DebugArrow(idVec4(0, 0, 1, 1), origin, origin + flingDir * 64, 1, 5000);
		}
	}
	else
	{
		shard->physicsObj.SetGravity(gameLocal.GetGravity());
	}
	

	shard->physicsObj.SetContents( CONTENTS_RENDERMODEL );
	shard->physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
	shard->physicsObj.ApplyImpulse( 0, origin, impulse * linearVelocityScale * dir );
	shard->physicsObj.SetAngularVelocity( dir.Cross( dir2 ) * ( f * angularVelocityScale ) );

	shard->clipModel->SetId( clipModelId );

	

	BecomeActive( TH_PHYSICS );
}

/*
================
idBrittleFracture::Shatter
================
*/
void idBrittleFracture::Shatter( const idVec3 &point, const idVec3 &impulse, const int time ) {
	int i;
	idVec3 dir;
	shard_t *shard;
	float m;

	// The portal was partially or fully faded out when the window shattered.
	// This is hopefully not a super common case, but the safest thing to do is to just open the portal and eat the performance impact.
	if (portalFadeState != PORTAL_FADE_TRANSPARENT)
	{
		int portalState = gameRenderWorld->GetPortalState(portal);
		gameLocal.SetPortalState(portal, portalState & ~PS_BLOCK_VIEW);
	}

	if ( gameLocal.isServer )
	{
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteFloat( point[0] );
		msg.WriteFloat( point[1] );
		msg.WriteFloat( point[2] );
		msg.WriteFloat( impulse[0] );
		msg.WriteFloat( impulse[1] );
		msg.WriteFloat( impulse[2] );
		ServerSendEvent( EVENT_SHATTER, &msg, true, -1 );
	}

	if ( time > ( gameLocal.time - SHARD_ALIVE_TIME ) )
	{
		StartSound( "snd_shatter", SND_CHANNEL_ANY, 0, false, NULL );
	}

	if ( !IsBroken() ) {
		Break();
	}

	if ( fxFracture.Length() )
	{
		idEntityFx::StartFx( fxFracture, &point, &GetPhysics()->GetAxis(), this, true );
	}

	dir = impulse;
	m = dir.Normalize();

	

	for ( i = 0; i < shards.Num(); i++ ) {
		shard = shards[i];

		if ( shard->droppedTime != -1 ) {
			continue;
		}

		if ( ( shard->clipModel->GetOrigin() - point ).LengthSqr() > Square( maxShatterRadius ) ) {
			continue;
		}

		//common->Printf("vel %f %f %f\n", dir.x, dir.y, dir.z);
		DropShard( shard, point, dir, m, time );
	}

	DropFloatingIslands( point, impulse, time );

	
}

/*
================
idBrittleFracture::DropFloatingIslands
================
*/
void idBrittleFracture::DropFloatingIslands( const idVec3 &point, const idVec3 &impulse, const int time ) {
	int i, j, numIslands;
	int queueStart, queueEnd;
	shard_t *curShard, *nextShard, **queue;
	bool touchesEdge;
	idVec3 dir;

	dir = impulse;
	dir.Normalize();

	numIslands = 0;
	queue = (shard_t **) _alloca16( shards.Num() * sizeof(shard_t **) );
	for ( i = 0; i < shards.Num(); i++ ) {
		shards[i]->islandNum = 0;
	}

	for ( i = 0; i < shards.Num(); i++ ) {

		if ( shards[i]->droppedTime != -1 ) {
			continue;
		}

		if ( shards[i]->islandNum ) {
			continue;
		}

		queueStart = 0;
		queueEnd = 1;
		queue[0] = shards[i];
		shards[i]->islandNum = numIslands+1;
		touchesEdge = false;

		if ( shards[i]->atEdge ) {
			touchesEdge = true;
		}

		for ( curShard = queue[queueStart]; queueStart < queueEnd; curShard = queue[++queueStart] ) {

			for ( j = 0; j < curShard->neighbours.Num(); j++ ) {

				nextShard = curShard->neighbours[j];

				if ( nextShard->droppedTime != -1 ) {
					continue;
				}

				if ( nextShard->islandNum ) {
					continue;
				}

				queue[queueEnd++] = nextShard;
				nextShard->islandNum = numIslands+1;

				if ( nextShard->atEdge ) {
					touchesEdge = true;
				}
			}
		}
		numIslands++;

		// if the island is not connected to the world at any edges
		if ( !touchesEdge ) {
			for ( j = 0; j < queueEnd; j++ ) {
				DropShard( queue[j], point, dir, 0.0f, time );
			}
		}
	}
}

/*
================
idBrittleFracture::Break
================
*/
void idBrittleFracture::Break( void ) {
	fl.takedamage = false;
	physicsObj.SetContents( CONTENTS_RENDERMODEL | CONTENTS_TRIGGER );
}

/*
================
idBrittleFracture::IsBroken
================
*/
bool idBrittleFracture::IsBroken( void ) const {
	return ( fl.takedamage == false );
}

/*
================
idBrittleFracture::Killed
================
*/
void idBrittleFracture::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( !disableFracture ) {

		if (!hasActivatedTargets)
		{
			hasActivatedTargets = true;
			PrintEventMessage(inflictor);
			ActivateTargets(this);
			SetDoorVacuumLock();
			DoSplinePull();
		}

		Break();
	}
}

/*
================
idBrittleFracture::AddDamageEffect
================
*/
void idBrittleFracture::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) {
	if ( !disableFracture ) {
		ProjectDecal( collision.c.point, collision.c.normal, gameLocal.time, damageDefName );
	}
}

/*
================
idBrittleFracture::Fracture_r
================
*/
void idBrittleFracture::Fracture_r( idFixedWinding &w ) {
	int i, j, bestPlane;
	float a, c, s, dist, bestDist;
	idVec3 origin;
	idPlane windingPlane, splitPlanes[2];
	idMat3 axis, axistemp;
	idFixedWinding back;
	idTraceModel trm;
	idClipModel *clipModel;

	while( 1 ) {
		origin = w.GetCenter();
		w.GetPlane( windingPlane );

		if ( w.GetArea() < maxShardArea ) {
			break;
		}

		// randomly create a split plane
		axis[2] = windingPlane.Normal();
#ifdef _D3XP
		if ( isXraySurface ) {
			a = idMath::TWO_PI / 2.f;
		}
		else {
			a = gameLocal.random.RandomFloat() * idMath::TWO_PI;
		}
#else
		a = gameLocal.random.RandomFloat() * idMath::TWO_PI;
#endif
		c = cos( a );
		s = -sin( a );
		axis[2].NormalVectors( axistemp[0], axistemp[1] );
		axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * s;
		axis[1] = axistemp[ 0 ] * s + axistemp[ 1 ] * -c;

		// get the best split plane
		bestDist = 0.0f;
		bestPlane = 0;
		for ( i = 0; i < 2; i++ ) {
			splitPlanes[i].SetNormal( axis[i] );
			splitPlanes[i].FitThroughPoint( origin );
			for ( j = 0; j < w.GetNumPoints(); j++ ) {
				dist = splitPlanes[i].Distance( w[j].ToVec3() );
				if ( dist > bestDist ) {
					bestDist = dist;
					bestPlane = i;
				}
			}
		}

		// split the winding
		if ( !w.Split( &back, splitPlanes[bestPlane] ) ) {
			break;
		}

		// recursively create shards for the back winding
		Fracture_r( back );
	}

	// translate the winding to it's center
	origin = w.GetCenter();
	for ( j = 0; j < w.GetNumPoints(); j++ ) {
		w[j].ToVec3() -= origin;
	}
	w.RemoveEqualPoints();

	trm.SetupPolygon( w );
	trm.Shrink( CM_CLIP_EPSILON );
	clipModel = new idClipModel( trm );

	physicsObj.SetClipModel( clipModel, 1.0f, shards.Num() );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() + origin, shards.Num() );
	physicsObj.SetAxis( GetPhysics()->GetAxis(), shards.Num() );

	AddShard( clipModel, w );
}

/*
================
idBrittleFracture::CreateFractures
================
*/
void idBrittleFracture::CreateFractures( const idRenderModel *renderModel ) {
	int i, j, k;
	const modelSurface_t *surf;
	const idDrawVert *v;
	idFixedWinding w;

	if ( !renderModel ) {
		return;
	}

	physicsObj.SetSelf( this );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin(), 0 );
	physicsObj.SetAxis( GetPhysics()->GetAxis(), 0 );

#ifdef _D3XP
	if ( isXraySurface ) {
		for ( i = 0; i < 1 /*renderModel->NumSurfaces()*/; i++ ) {
			surf = renderModel->Surface( i );
			material = surf->shader;

			w.Clear();

			int k = 0;
			v = &surf->geometry->verts[k];
			w.AddPoint( v->xyz );
			w[k].s = v->st[0];
			w[k].t = v->st[1];

			k = 1;
			v = &surf->geometry->verts[k];
			w.AddPoint( v->xyz );
			w[k].s = v->st[0];
			w[k].t = v->st[1];

			k = 3;
			v = &surf->geometry->verts[k];
			w.AddPoint( v->xyz );
			w[k].s = v->st[0];
			w[k].t = v->st[1];

			k = 2;
			v = &surf->geometry->verts[k];
			w.AddPoint( v->xyz );
			w[k].s = v->st[0];
			w[k].t = v->st[1];

			Fracture_r( w );
		}

	}
	else {
		for ( i = 0; i < 1 /*renderModel->NumSurfaces()*/; i++ ) {
			surf = renderModel->Surface( i );
			material = surf->shader;

			for ( j = 0; j < surf->geometry->numIndexes; j += 3 ) {
				w.Clear();
				for ( k = 0; k < 3; k++ ) {
					v = &surf->geometry->verts[ surf->geometry->indexes[ j + 2 - k ] ];
					w.AddPoint( v->xyz );
					w[k].s = v->st[0];
					w[k].t = v->st[1];
				}
				Fracture_r( w );
			}
		}
	}
#else
	for ( i = 0; i < 1 /*renderModel->NumSurfaces()*/; i++ ) {
		surf = renderModel->Surface( i );
		material = surf->shader;

		for ( j = 0; j < surf->geometry->numIndexes; j += 3 ) {
			w.Clear();
			for ( k = 0; k < 3; k++ ) {
				v = &surf->geometry->verts[ surf->geometry->indexes[ j + 2 - k ] ];
				w.AddPoint( v->xyz );
				w[k].s = v->st[0];
				w[k].t = v->st[1];
			}
			Fracture_r( w );
		}
	}
#endif

	physicsObj.SetContents( material->GetContentFlags() | CONTENTS_TRANSLUCENT );
	SetPhysics( &physicsObj );
}

/*
================
idBrittleFracture::FindNeighbours
================
*/
void idBrittleFracture::FindNeighbours( void ) {
	int i, j, k, l;
	idVec3 p1, p2, dir;
	idMat3 axis;
	idPlane plane[4];

	for ( i = 0; i < shards.Num(); i++ ) {

		shard_t *shard1 = shards[i];
		const idWinding &w1 = shard1->winding;
		const idVec3 &origin1 = shard1->clipModel->GetOrigin();
		const idMat3 &axis1 = shard1->clipModel->GetAxis();

		for ( k = 0; k < w1.GetNumPoints(); k++ ) {

			p1 = origin1 + w1[k].ToVec3() * axis1;
			p2 = origin1 + w1[(k+1)%w1.GetNumPoints()].ToVec3() * axis1;
			dir = p2 - p1;
			dir.Normalize();
			axis = dir.ToMat3();

			plane[0].SetNormal( dir );
			plane[0].FitThroughPoint( p1 );
			plane[1].SetNormal( -dir );
			plane[1].FitThroughPoint( p2 );
			plane[2].SetNormal( axis[1] );
			plane[2].FitThroughPoint( p1 );
			plane[3].SetNormal( axis[2] );
			plane[3].FitThroughPoint( p1 );

			for ( j = 0; j < shards.Num(); j++ ) {

				if ( i == j ) {
					continue;
				}

				shard_t *shard2 = shards[j];

				for ( l = 0; l < shard1->neighbours.Num(); l++ ) {
					if ( shard1->neighbours[l] == shard2 ) {
						break;
					}
				}
				if ( l < shard1->neighbours.Num() ) {
					continue;
				}

				const idWinding &w2 = shard2->winding;
				const idVec3 &origin2 = shard2->clipModel->GetOrigin();
				const idMat3 &axis2 = shard2->clipModel->GetAxis();

				for ( l = w2.GetNumPoints()-1; l >= 0; l-- ) {
					p1 = origin2 + w2[l].ToVec3() * axis2;
					p2 = origin2 + w2[(l-1+w2.GetNumPoints())%w2.GetNumPoints()].ToVec3() * axis2;
					if ( plane[0].Side( p2, 0.1f ) == SIDE_FRONT && plane[1].Side( p1, 0.1f ) == SIDE_FRONT ) {
						if ( plane[2].Side( p1, 0.1f ) == SIDE_ON && plane[3].Side( p1, 0.1f ) == SIDE_ON ) {
							if ( plane[2].Side( p2, 0.1f ) == SIDE_ON && plane[3].Side( p2, 0.1f ) == SIDE_ON ) {
								shard1->neighbours.Append( shard2 );
								shard1->edgeHasNeighbour[k] = true;
								shard2->neighbours.Append( shard1 );
								shard2->edgeHasNeighbour[(l-1+w2.GetNumPoints())%w2.GetNumPoints()] = true;
								break;
							}
						}
					}
				}
			}
		}

		for ( k = 0; k < w1.GetNumPoints(); k++ ) {
			if ( !shard1->edgeHasNeighbour[k] ) {
				break;
			}
		}
		if ( k < w1.GetNumPoints() ) {
			shard1->atEdge = true;
		} else {
			shard1->atEdge = false;
		}
	}
}

/*
================
idBrittleFracture::Event_Activate
================
*/
void idBrittleFracture::Event_Activate( idEntity *activator ) {
	disableFracture = false;
	if ( health <= 0 ) {
		Break();
	}
}

/*
================
idBrittleFracture::Event_Touch
================
*/
void idBrittleFracture::Event_Touch( idEntity *other, trace_t *trace ) {
	idVec3 point, impulse;

	if ( !IsBroken() ) {
		return;
	}

	if ( trace->c.id < 0 || trace->c.id >= shards.Num() ) {
		return;
	}

	point = shards[trace->c.id]->clipModel->GetOrigin();
	impulse = other->GetPhysics()->GetLinearVelocity() * other->GetPhysics()->GetMass();

	Shatter( point, impulse, gameLocal.time );
}

/*
================
idBrittleFracture::ClientPredictionThink
================
*/
void idBrittleFracture::ClientPredictionThink( void ) {
	// only think forward because the state is not synced through snapshots
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	Think();
}

/*
================
idBrittleFracture::ClientReceiveEvent
================
*/
bool idBrittleFracture::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	idVec3 point, dir;

	switch( event ) {
		case EVENT_PROJECT_DECAL: {
			point[0] = msg.ReadFloat();
			point[1] = msg.ReadFloat();
			point[2] = msg.ReadFloat();
			dir[0] = msg.ReadFloat();
			dir[1] = msg.ReadFloat();
			dir[2] = msg.ReadFloat();
			ProjectDecal( point, dir, time, NULL );
			return true;
		}
		case EVENT_SHATTER: {
			point[0] = msg.ReadFloat();
			point[1] = msg.ReadFloat();
			point[2] = msg.ReadFloat();
			dir[0] = msg.ReadFloat();
			dir[1] = msg.ReadFloat();
			dir[2] = msg.ReadFloat();
			Shatter( point, dir, time );
			return true;
		}
		default:
			break;
	}

	return idEntity::ClientReceiveEvent( event, time, msg );
}

void idBrittleFracture::Event_SpectatorTouch(idEntity *other, trace_t *trace)
{
	idVec3		contact, translate, normal;
	idBounds	bounds;
	idPlayer	*p;

	assert(other && other->IsType(idPlayer::Type) && static_cast<idPlayer *>(other)->spectating);

	p = static_cast<idPlayer *>(other);
	// avoid flicker when stopping right at clip box boundaries
	if (p->lastSpectateTeleport > gameLocal.hudTime - 300) { //BC was 1000
		return;
	}

	//Attempt to find a space for player to teleport to.
	idVec3 glassContact = trace->endpos;

	idVec3 playerMovedir = p->GetPhysics()->GetLinearVelocity().ToAngles().ToForward();
	idVec3 candidateSpot = glassContact + playerMovedir * 48;

	trace_t candidateTr;
	gameLocal.clip.TraceBounds(candidateTr, candidateSpot, candidateSpot + idVec3(0, 0, 1), p->GetPhysics()->GetBounds(), MASK_SOLID, NULL);

	if (candidateTr.fraction < 1.0f)
		return;

	p->SetOrigin(candidateSpot);
	p->lastSpectateTeleport = gameLocal.hudTime;
	gameLocal.GetLocalPlayer()->StartSound("snd_spectate_door", SND_CHANNEL_ANY);
}

void idBrittleFracture::SetRoomAssignment()
{
	idLocationEntity *locationEntity = NULL;

	locationEntity = gameLocal.LocationForPoint(this->GetPhysics()->GetOrigin());
	if (locationEntity)
	{
		if (!idStr::Cmp(locationEntity->GetLocation(), common->GetLanguageDict()->GetString("#str_00000")))
		{
			//window is assigned to outer space. This is no good. We want to know what interior room the window is assigned to.			
			for (int i = 0; i < targets.Num(); i++)
			{
				idEntity *ent = targets[i].GetEntity();
				if (!ent)
					continue;

				//Grab the point behind its vacuumseparator entity.
				if (ent->IsType(idVacuumSeparatorEntity::Type))
				{
					idVec3 forward;
					ent->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
					locationEntity = gameLocal.LocationForPoint(ent->GetPhysics()->GetOrigin() + (forward * -16));
				}
			}
		}
			
		if (locationEntity)
		{
			if (idStr::Cmp(locationEntity->GetLocation(), common->GetLanguageDict()->GetString("#str_00000")))
			{
				//Great, we now know what room this window is assigned to.
				assignedRoom = locationEntity;
				return;
			}
		}
		
		gameLocal.Warning("SetRoomAssignment(): failed to assign room to window '%s'", this->GetName());		
	}
}

void idBrittleFracture::SetDoorVacuumLock()
{
// 	if (vacuumDoorsToClose.Num() <= 0)
// 		return;
// 
// 	for (int i = 0; i < vacuumDoorsToClose.Num(); i++)
// 	{
// 		int entIndex = vacuumDoorsToClose[i];
// 
// 		if (!gameLocal.entities[entIndex]->IsType(idDoor::Type))
// 			continue;
// 
// 		static_cast<idDoor *>(gameLocal.entities[entIndex])->Lock(1);
// 		
// 		//static_cast<idDoor *>(gameLocal.entities[entIndex])->SetUnlockDelay(SUCTION_TIME + 1000);
// 		//gameRenderWorld->DebugArrowSimple(gameLocal.entities[entIndex]->GetPhysics()->GetOrigin());
// 	}
}

void idBrittleFracture::DoSplinePull()
{
	idEntity *vacuumSeparator = GetVacuumSeparator();
	if (vacuumSeparator == NULL)
		return;

	#define SUCTION_MARGIN 64
	idVec3 forward = vacuumSeparator->GetPhysics()->GetAxis().ToAngles().ToForward();
	idVec3 interiorPos = vacuumSeparator->GetPhysics()->GetOrigin() + forward * -64;
	idVec3 exteriorPos = vacuumSeparator->GetPhysics()->GetOrigin() + forward * 64;	

	gameLocal.DoVacuumSuctionActors(interiorPos, exteriorPos, vacuumSeparator->GetPhysics()->GetAxis().ToAngles(), true);
	gameLocal.DoVacuumSuctionItems(interiorPos, exteriorPos);
}

idEntity * idBrittleFracture::GetVacuumSeparator()
{
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity *ent = targets[i].GetEntity();
		if (!ent)
			continue;

		//Grab the point behind its vacuumseparator entity.
		if (ent->IsType(idVacuumSeparatorEntity::Type))
		{
			return ent;
		}
	}

	return NULL;
}

void idBrittleFracture::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (attacker != NULL)
	{
		if (attacker->IsType(idAI::Type) && (!gameLocal.InPlayerPVS(attacker) || !gameLocal.InPlayerConnectedArea(attacker)))
		{
			//if I'm hit by an AI, but the AI is not visible to the player, then ignore the damage. We only want to showcase window explosions if player can see it.
			return;
		}
	}

	idEntity::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);
}

void idBrittleFracture::PrintEventMessage(idEntity *inflictor)
{
	if (inflictor == nullptr)
	{
		gameLocal.AddEventLog("#str_def_gameplay_window_breakgeneric", GetPhysics()->GetOrigin());
		return;
	}

	//BC 5-2-2025: don't ever use the internal entity name. Just use question mark.
	idStr inflictorName = (inflictor->displayName.Length() > 0) ? inflictor->displayName : idStr(common->GetLanguageDict()->GetString("#str_07001"));
	gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_window_break"), inflictorName.c_str()), GetPhysics()->GetOrigin());
}

//BC 2-26-2025: make windows update during spectate, so they do the proper fade in / fade out.
void idBrittleFracture::SpectateUpdate()
{
	UpdatePortalVisibility();
	Present();
}