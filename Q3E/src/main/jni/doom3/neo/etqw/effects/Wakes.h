// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_WAKES_H__
#define __GAME_WAKES_H__

#include "HardcodedParticleSystem.h"

struct sdWakeParms {

	static const int MAX_POINTS = 4;

	const idMaterial *centerMat;
	const idMaterial *edgeMat;
	idVec2 centerWidths;
	idVec2 edgeWidths;
	idVec2 centerScales;
	idVec2 edgeScales;
	idVec2 centerTexCoord;
	idVec2 edgeTexCoord;
	int maxVisDist;

	int numPoints;
	idVec3 points[MAX_POINTS];
	idVec3 normals[MAX_POINTS];

	bool noWake;
	void ParseFromDict( const idDict &spawnArgs );
};

struct sdWakeNode {
	int spawnTime;
	bool breakWake;// Start a new subwake from here
	float alpha;
};

class sdWakeLayer {

public:
	static const int MAX_NODES = 64;

private:
	idDrawVert *triangleVerts;
	int	firstVert;	//Index of first vertex in the triangle list where this layer can write to

	int numNodes;
	int firstNode;
	sdWakeNode nodes[MAX_NODES];

	// Tweakables
	float	lifeTime;		//Nodes die after this amount of time
	float	posWidth;		//With of "positive side" of the curve normal
	float	negWidth;		//With of "positive side" of the curve normal
	float	posCurScale;	//Scale curvature by this value to modulate the width the positive side
	float	negCurScale;	//Scale curvature by this value to modulate the width the negative side
	float	texNeg;			//Texture coordinate to use on the negative side
	float	texPos;			//Texture coordinate to use on the positive side

	// These update on the go
	float	texOfs;			// How far have we traveled "in curve parameter space"
	float	negScale;
	float	posScale;
	idVec3	lastOrigin;		// Origin of last "node" call
	idVec3	lastDeriv;		// First order derivative of last node

	int		AddToBack( sdWakeNode &node );
	void	PopFront();
	int		RemapIndex( int index );
	sdWakeNode &GetNode( int index );	

public:
	sdWakeLayer( void );
	void	Init( idDrawVert *triangleVerts, int firstVertex );
	void	AddNode( const idVec3 &origin, const idVec3 &emitLeft, float alpha );
	void	Update( struct srfTriangles_t *triangles );
	void	Break( void );

	void	SetWidths( float negWidth, float posWidth ) { this->posWidth = posWidth; this->negWidth = negWidth; }
	void	SetTexCoords( float texNeg, float texPos ) { this->texNeg = texNeg; this->texPos = texPos; }
	void	SetCurvatureScales( float negCurScale, float posCurScale ) { this->posCurScale = posCurScale; this->negCurScale = negCurScale; }
	int		NumNodes( void ) ;

};

class sdWake : public sdHardcodedParticleSystem {

	static const int MAX_POINTS = 4;
	
	idVec3 minPoint;
	idVec3 maxPoint;
	int nextNodeTime;

	idList< idDrawVert > triangleVerts[2];


//	srfTriangles_t *	GetTriSurf( int index ) { return renderEntity.hModel->Surface( index )->geometry; }
//	int					NumSurfaces( void ) { return renderEntity.hModel->NumSurfaces(); }

	sdWakeLayer layer[MAX_POINTS];
	sdWakeLayer layer3;

	int numPoints;
	idVec3 points[MAX_POINTS];
	idVec3 normals[MAX_POINTS];
	bool stopped;
	int ticket;

	void AddPoint( const idVec3 &point );
	void ClearPoints( void );

public:
    sdWake( void );
	void Init(  const sdWakeParms &params, int ticket );
	void Update( const idVec3 &forward, const idVec3 &origin, const idMat3 &axis );
	void Update( void );
	void Break( void );
	bool HasStopped( void );
	int GetTicket() { return ticket; }
};

#define MAX_WAKES 50

class sdWakeManagerLocal {

	int numWakes;
	int ticket;
	sdWake *wakes;
	
public:
	sdWakeManagerLocal() : wakes(NULL), numWakes(0) {}
	void Init( void );
	void Deinit( void );
	unsigned int AllocateWake( const sdWakeParms &params );
	bool UpdateWake( unsigned int handle, const idVec3 &forward, const idVec3 &origin, const idMat3 &axis );
	void BreakWake( unsigned int wake );
	void Think( void );
};

typedef sdSingleton< sdWakeManagerLocal > sdWakeManager;

#endif //__GAME_WAKES_H__
