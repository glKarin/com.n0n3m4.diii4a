// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "FootPrints.h"

#include "HardcodedParticleSystem.h"

/*********************************************************************************************************
sdFootPrintManagerLocal
*********************************************************************************************************/

class sdFootPrintManagerLocal : public sdFootPrintManager {
public:

	sdFootPrintManagerLocal() : system( NULL ) {}
	virtual void Init( void );
	virtual void Deinit( void );

	virtual bool AddFootPrint( const idVec3 & point, const idVec3 &forward, const idVec3 &up, bool right );
	
	virtual void Think( void );

	virtual renderEntity_t* GetRenderEntity( void );
	virtual qhandle_t		GetModelHandle( void );

private:
	struct footprint {
		idVec3 p;
		idVec3 f;
		idVec3 l;
		int age;
	};

	static const int MAX_FOOTSTEPS = 128;

	footprint footprints[ MAX_FOOTSTEPS ];

	sdHardcodedParticleSystem *system;
	unsigned short index;

	unsigned int start;
	unsigned int end;
};


sdFootPrintManagerLocal footPrintManagerLocal;
sdFootPrintManager *footPrintManager = &footPrintManagerLocal;

/*
==============
sdFootPrintManagerLocal::Init
==============
*/
void sdFootPrintManagerLocal::Init( void ) {
	const sdDeclStringMap* map = gameLocal.declStringMapType[ "footPrintDef" ];
	if ( !map ) {
		gameLocal.Error( "sdFootPrintManagerLocal::Init stringMap 'footPrintDef' not found" );
		return;
	}
	int maxnumpnts = MAX_FOOTSTEPS * 4;
	int maxnumtris = MAX_FOOTSTEPS * 2;
	system = new sdHardcodedParticleSystem;
	system->AddSurfaceDB( declHolder.declMaterialType.LocalFind( map->GetDict().GetString( "material", "_black" ) ), maxnumpnts, maxnumtris * 3 );
	system->GetRenderEntity().axis.Identity();
	system->GetRenderEntity().origin.Zero();

	start = 0;
	end = 0;
}


/*
==============
sdFootPrintManagerLocal::Deinit
==============
*/
void sdFootPrintManagerLocal::Deinit( void ) {
	delete system;
	system = NULL;
}

/*
==============
sdFootPrintManagerLocal::AddFootPrint
==============
*/
bool sdFootPrintManagerLocal::AddFootPrint( const idVec3 & point, const idVec3 &forward, const idVec3 &up, bool right ) {

	int num = end - start;
	int index = end % MAX_FOOTSTEPS;
	end++;
	if ( num == MAX_FOOTSTEPS ) {
		start++;
	}
	footprint *f = &footprints[ index ];
	f->f = forward;
	f->f.Normalize();
	f->l = up.Cross( f->f );
	f->f = f->l.Cross( up );
	f->p = point;
	f->age = gameLocal.time;
	if ( !right ) {
		f->l = -f->l;
	}
	return false;
}

/*
==============
sdFootPrintManagerLocal::Think
==============
*/
void sdFootPrintManagerLocal::Think( void ) {

	system->SetDoubleBufferedModel();

	while ( start < end ) {
		int index = start % MAX_FOOTSTEPS;
		footprint *f = &footprints[ index ];
		if ( ( gameLocal.time - f->age ) > SEC2MS( 60.f ) ) {
			start++;
		} else {
			break;
		}
	}

	srfTriangles_t *surf = system->GetTriSurf( 0 );
	surf->numIndexes = 0;
	surf->numVerts = 0;
	surf->bounds.Clear();

	for (unsigned int i=start; i<end; i++) {
		int index = i % MAX_FOOTSTEPS;
		footprint *f = &footprints[ index ];

		idDrawVert *v = &surf->verts[ surf->numVerts ];
		vertIndex_t  *idx = &surf->indexes[ surf->numIndexes ];

		byte alpha = 255;
		int a = ( gameLocal.time - f->age );
		if ( a > SEC2MS( 50 ) ) {
			alpha = ( byte )( idMath::ClampFloat( 0.f, 1.f, 1.f - ( ( a - SEC2MS( 50 ) ) / ( float )( SEC2MS( 10 ) ) ) ) * 255 );
		}

		v->Clear();
		v->xyz = f->p + f->f * 8 + f->l * 3;
		v->SetST( 0.f, 0.f );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		v++;
		v->Clear();
		v->xyz = f->p + f->f * 8 - f->l * 3;
		v->SetST( 1.f, 0.f );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		v++;
		v->Clear();
		v->xyz = f->p - f->f * 8 + f->l * 3;
		v->SetST( 0.f, 1.f );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		v++;
		v->Clear();
		v->xyz = f->p - f->f * 8 - f->l * 3;
		v->SetST( 1.f, 1.f );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		v++;

		idx[0] = surf->numVerts + 0;
		idx[1] = surf->numVerts + 1;
		idx[2] = surf->numVerts + 2;
		idx[3] = surf->numVerts + 1;
		idx[4] = surf->numVerts + 3;
		idx[5] = surf->numVerts + 2;

		surf->numIndexes += 6;
		surf->numVerts += 4;

		surf->bounds.AddPoint( f->p );
	}

	if ( surf->numVerts ) {
		surf->bounds.ExpandSelf( 10.f );
		system->GetRenderEntity().hModel->FreeVertexCache();
		system->GetRenderEntity().hModel->SetBounds(surf->bounds);
		system->GetRenderEntity().bounds = surf->bounds;

		system->PresentRenderEntity();
	}
}

/*
==============
sdFootPrintManagerLocal::GetRenderEntity
==============
*/
renderEntity_t* sdFootPrintManagerLocal::GetRenderEntity( void ) {
	return system == NULL ? NULL : &system->GetRenderEntity();
}

/*
==============
sdFootPrintManagerLocal::GetModelHandle
==============
*/
int sdFootPrintManagerLocal::GetModelHandle( void ) {
	return system == NULL ? -1 : system->GetModelHandle();
}
