// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "TireTread.h"

#include "HardcodedParticleSystem.h"
#include "../../decllib/DeclSurfaceType.h"


/*********************************************************************************************************
sdTireTread
*********************************************************************************************************/

static const float MIN_DISTANCE = 16.f;

class sdTireTread {
	
public:
	static const int MAX_POINTS = 16;
private:

	struct nodeInfo {
		idVec3 p0, p1, m;
		float travel;
		float skid;
		int age;
	};

	int id;
	int age;
	int startPoint;
	int endPoint;
	nodeInfo points[ MAX_POINTS ];

	bool isStrogg;
	bool hasPoint;
	idVec3 point;

public:

	sdTireTread();
	~sdTireTread();

	void Init( void );
	void Think( void );
	void Free( void );

	void Start( int _id, bool _isStrogg );
	void EmitPoint( const idVec3 &pnt, const idVec3 &f, const idVec3 &u, float skid );
	void AddPoint( const idVec3 &pnt, const idVec3 &f, const idVec3 &u, float skid );
	void Stop( void );

	void Draw( srfTriangles_t *surf );

	bool IsId( int _id );
	bool IsFree( void );
};

/*
==============
sdTireTread::sdTireTread
==============
*/
sdTireTread::sdTireTread() {
	startPoint = 0;
	endPoint = 0;
	id = -1;
	hasPoint = false;
}

/*
==============
sdTireTread::~sdTireTread
==============
*/
sdTireTread::~sdTireTread() {
}

/*
==============
sdTireTread::Init
==============
*/
void sdTireTread::Init( void ) {
	startPoint = 0;
	endPoint = 0;
	hasPoint = false;
	isStrogg = false;
}

/*
==============
sdTireTread::Start
==============
*/
void sdTireTread::Start( int _id, bool _isStrogg ) {
	Init();
	id = _id;
	isStrogg = _isStrogg;
}

/*
==============
sdTireTread::Think
==============
*/
void sdTireTread::Think( void ) {
	if ( ( gameLocal.time - age ) > SEC2MS( 60 ) ) {
		Free();
	}
/*	
	int curidx = startPoint % MAX_POINTS;
	if ( (gameLocal.framenum - points[curidx].age) > 100 * 10 ) {
		startPoint++;
		curidx = startPoint % MAX_POINTS;

		if ( startPoint == endPoint ) {
			Free();
			return;
		}
	}*/
}

/*
==============
sdTireTread::Draw
==============
*/
void sdTireTread::Draw( srfTriangles_t *surf ) {

	int lp = endPoint - 1;

	if ( lp <= startPoint ) {
		return;
	}

	float globalAlpha = 1.f;
	int a = ( gameLocal.time - age );
	if ( a > SEC2MS( 50 ) ) {
		globalAlpha = idMath::ClampFloat( 0.f, 1.f, 1.f - ( ( a - SEC2MS( 50 ) ) / (float)SEC2MS( 10 ) ) );
	}

	float ofs = isStrogg ? 0.75f : 0.f;

	idDrawVert *v = &surf->verts[ surf->numVerts ];
	vertIndex_t *indices = &surf->indexes[ surf->numIndexes ];
	int startIdx = surf->numVerts;
	int num = lp - startPoint;
	float anglescale = idMath::PI / num;
	for (int i=startPoint; i<endPoint; i++) {
		int curidx = i % MAX_POINTS;
		byte alpha = (byte)(idMath::ClampFloat( 0.f, 1.f, points[curidx].skid * globalAlpha * idMath::Sin( (i-startPoint) * anglescale ) * 2.f ) * 255);
		v->Clear();
		v->xyz = points[curidx].p0;
		v->SetST( ofs, points[curidx].travel );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		surf->bounds.AddPoint( v->xyz );
		v++;

		v->Clear();
		v->xyz = points[curidx].p1;
		v->SetST( ofs+0.25f, points[curidx].travel );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		surf->bounds.AddPoint( v->xyz );
		v++;
		startIdx += 2;
	}

	int baseIdx = surf->numVerts;
	for (int i=startPoint; i<lp; i++) {

		indices[0] = baseIdx + 0;
		indices[1] = baseIdx + 1;
		indices[2] = baseIdx + 2;

		indices[3] = baseIdx + 2;
		indices[4] = baseIdx + 1;
		indices[5] = baseIdx + 3;

		baseIdx += 2;
		indices += 6;
	}

	surf->numVerts += (endPoint - startPoint) * 2;
	surf->numIndexes += (lp - startPoint) * 6;
}


/*
==============
sdTireTread::Free
==============
*/
void sdTireTread::Free( void ) {
	Init();
	id = -1;
}

/*
==============
sdTireTread::EmitPoint
==============
*/
void sdTireTread::EmitPoint( const idVec3 &pnt, const idVec3 &f, const idVec3 &u, float skid ) {
	int idx = endPoint % MAX_POINTS;

	idVec3 l;
	l = f.Cross( u );
	l.Normalize();

	float scale = isStrogg ? 0.125f : 0.25f;
	float size  = isStrogg ? 18.f : 10.f;

	points[ idx ].skid = skid;
	points[ idx ].m  = pnt;
	points[ idx ].p0 = pnt + l * size;
	points[ idx ].p1 = pnt - l * size;
	if ( endPoint > startPoint ) {
		int previdx = (endPoint + MAX_POINTS - 1) % MAX_POINTS;
		idVec3 d = (points[ idx ].m - points[ previdx ].m);
		points[ idx ].travel = points[ previdx ].travel + d.Length() * scale;
	} else {
		points[ idx ].travel = -8.f;
	}

	points[ idx ].age = gameLocal.time;
	endPoint++;

	age = gameLocal.time;
	int sp = endPoint - MAX_POINTS;
	if ( startPoint < sp ) {
		startPoint = sp;
	}
}

/*
==============
sdTireTread::AddPoint
==============
*/
void sdTireTread::AddPoint( const idVec3 &pnt, const idVec3 &f, const idVec3 &u, float skid ) {
	if ( hasPoint ) {
		idVec3 forward = pnt - point;
		if ( forward.LengthSqr() > Square( MIN_DISTANCE ) ) {
			EmitPoint( point, forward, u, skid );
		}
	}
	point = pnt;
	hasPoint = true;
}
/*
==============
sdTireTread::Stop
==============
*/
void sdTireTread::Stop( void ) {
	int num = endPoint - startPoint;
	if ( num <= 3 ) {
		Free();
	}
}


/*
==============
sdTireTread::IsId
==============
*/
bool sdTireTread::IsId( int _id ) {
	return id == _id;
}

/*
==============
sdTireTread::IsFree
==============
*/
bool sdTireTread::IsFree( void ) {
	return startPoint == endPoint;
}


/*********************************************************************************************************
sdTireTreadManagerLocal
*********************************************************************************************************/

class sdTireTreadManagerLocal : public sdTireTreadManager {
	static const int MAX_TREADS = 32;
	sdTireTread treads[ MAX_TREADS ];

	sdHardcodedParticleSystem *system;

	unsigned short index;

public:
	sdTireTreadManagerLocal() : system( NULL ) {}

	virtual void Init( void );
	virtual void Deinit( void );

	virtual unsigned int  StartSkid( bool isStrogg );
	virtual bool AddSkidPoint( unsigned int handle, const idVec3 & point, const idVec3 &forward, const idVec3 &up, const sdDeclSurfaceType *surface );
	virtual void StopSkid( unsigned int handle );
	
	virtual void Think( void );
};


sdTireTreadManagerLocal tireTreadManagerLocal;
sdTireTreadManager *tireTreadManager = &tireTreadManagerLocal;

/*
==============
sdTireTreadManagerLocal::Init
==============
*/
void sdTireTreadManagerLocal::Init( void ) {
	const sdDeclStringMap* map = gameLocal.declStringMapType[ "tireTreadDef" ];
	if ( !map ) {
		gameLocal.Error( "sdTireTreadManagerLocal::Init stringMap 'tireTreadDef' not found" );
		return;
	}

	index = 1;
	for (int i=0; i<MAX_TREADS; i++) {
		treads[i].Init();
	}
	int maxnumpnts = MAX_TREADS * sdTireTread::MAX_POINTS * 2;
	int maxnumtris = MAX_TREADS * (sdTireTread::MAX_POINTS-1) * 2;

	system = new sdHardcodedParticleSystem;
	system->AddSurfaceDB( declHolder.declMaterialType.LocalFind( map->GetDict().GetString( "material", "_black" ) ), maxnumpnts, maxnumtris * 3 );
	system->GetRenderEntity().axis.Identity();
	system->GetRenderEntity().origin.Zero();
	srfTriangles_t *surf = system->GetTriSurf( 0 );
}


/*
==============
sdTireTreadManagerLocal::Deinit
==============
*/
void sdTireTreadManagerLocal::Deinit( void ) {
	for (int i=0; i<MAX_TREADS; i++) {
		treads[i].Free();
	}
	delete system;
	system = NULL;
}

/*
==============
sdTireTreadManagerLocal::StartSkid
==============
*/
unsigned int  sdTireTreadManagerLocal::StartSkid( bool isStrogg ) {
	for ( int i=0; i<MAX_TREADS; i++ ) {
		if ( treads[i].IsFree() ) {
			int thisindex = index++;
			treads[i].Start( thisindex, isStrogg );
			return (((unsigned int)thisindex) << 16) | (i+1);
		}
	}
	return 0;
}

/*
==============
sdTireTreadManagerLocal::AddSkidPoint
==============
*/
bool sdTireTreadManagerLocal::AddSkidPoint( unsigned int handle, const idVec3 & point, const idVec3 &forward, const idVec3 &up, const sdDeclSurfaceType *surface ) {
	unsigned int idx = (handle & 0xffff)-1;
	unsigned int id = ( handle >> 16 );

	if ( idx < MAX_TREADS ) {
		if ( treads[idx].IsId( id ) ) {
			float skid = 1.f;
			if ( surface != NULL ) {
				skid = surface->GetProperties().GetFloat( "skid" );
			}
			if ( skid > 0.f ) {
				treads[idx].AddPoint( point, forward, up, skid );
				return true;
			} else {
				StopSkid( handle );
			}
		}
	}
	return false;
}

/*
==============
sdTireTreadManagerLocal::StopSkid
==============
*/
void sdTireTreadManagerLocal::StopSkid( unsigned int handle ) {
	unsigned int idx = (handle & 0xffff)-1;
	unsigned int id = (handle >> 16) - 1;

	if ( idx < MAX_TREADS ) {
		if ( treads[idx].IsId( id ) ) {
			treads[idx].Stop();
		}
	}
}

/*
==============
sdTireTreadManagerLocal:::Think
==============
*/
void sdTireTreadManagerLocal::Think( void ) {

	system->SetDoubleBufferedModel();

	srfTriangles_t *surf = system->GetTriSurf( 0 );
	surf->numIndexes = 0;
	surf->numVerts = 0;
	surf->bounds.Clear();

	for ( int i=0; i<MAX_TREADS; i++ ) {
		if ( !treads[i].IsFree() ) {
			treads[i].Think();

			treads[i].Draw( surf );
		}
	}

	if ( surf->numVerts ) {
		surf->bounds.ExpandSelf( 4.f );
		system->GetRenderEntity().hModel->FreeVertexCache();
		system->GetRenderEntity().hModel->SetBounds(surf->bounds);
		system->GetRenderEntity().bounds = surf->bounds;

		system->PresentRenderEntity();
	}
}
