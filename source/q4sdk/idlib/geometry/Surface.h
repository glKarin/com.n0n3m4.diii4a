
#ifndef __SURFACE_H__
#define __SURFACE_H__

/*
===============================================================================

	Surface base class.

	A surface is tesselated to a triangle mesh with each edge shared by
	at most two triangles.

===============================================================================
*/

typedef struct surfaceEdge_s {
	int						verts[2];	// edge vertices always with ( verts[0] < verts[1] )
	int						tris[2];	// edge triangles
} surfaceEdge_t;


class idSurface {
public:
							idSurface( void );
							explicit idSurface( const idSurface &surf );
							explicit idSurface( const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
							~idSurface( void );

	const idDrawVert &		operator[]( const int index ) const;
	idDrawVert &			operator[]( const int index );
	idSurface &				operator+=( const idSurface &surf );

	int						GetNumIndexes( void ) const { return indexes.Num(); }
	const int *				GetIndexes( void ) const { return indexes.Ptr(); }
	int						GetNumVertices( void ) const { return verts.Num(); }
	const idDrawVert *		GetVertices( void ) const { return verts.Ptr(); }
	const int *				GetEdgeIndexes( void ) const { return edgeIndexes.Ptr(); }
	const surfaceEdge_t *	GetEdges( void ) const { return edges.Ptr(); }

	void					Clear( void );
	void					SwapTriangles( idSurface &surf );
	void					TranslateSelf( const idVec3 &translation );
	void					RotateSelf( const idMat3 &rotation );

							// splits the surface into a front and back surface, the surface itself stays unchanged
							// frontOnPlaneEdges and backOnPlaneEdges optionally store the indexes to the edges that lay on the split plane
							// returns a SIDE_?
	int						Split( const idPlane &plane, const float epsilon, idSurface **front, idSurface **back, int *frontOnPlaneEdges = NULL, int *backOnPlaneEdges = NULL ) const;
							// cuts off the part at the back side of the plane, returns true if some part was at the front
							// if there is nothing at the front the number of points is set to zero
	bool					ClipInPlace( const idPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

							// returns true if each triangle can be reached from any other triangle by a traversal
	bool					IsConnected( void ) const;
							// returns true if the surface is closed
	bool					IsClosed( void ) const;
							// returns true if the surface is a convex hull
	bool					IsPolytope( const float epsilon = 0.1f ) const;

	float					PlaneDistance( const idPlane &plane ) const;
	int						PlaneSide( const idPlane &plane, const float epsilon = ON_EPSILON ) const;

							// returns true if the line intersects one of the surface triangles
	bool					LineIntersection( const idVec3 &start, const idVec3 &end, bool backFaceCull = false ) const;
							// intersection point is start + dir * scale
	bool					RayIntersection( const idVec3 &start, const idVec3 &dir, float &scale, bool backFaceCull = false ) const;

protected:
	idList<idDrawVert>		verts;			// vertices
	idList<int>				indexes;		// 3 references to vertices for each triangle
	idList<surfaceEdge_t>	edges;			// edges
	idList<int>				edgeIndexes;	// 3 references to edges for each triangle, may be negative for reversed edge

protected:
	void					GenerateEdgeIndexes( void );
	int						FindEdge( int v1, int v2 ) const;
};

/*
====================
idSurface::idSurface
====================
*/
ID_INLINE idSurface::idSurface( void ) {
}

/*
=================
idSurface::idSurface
=================
*/
ID_INLINE idSurface::idSurface( const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	assert( verts != NULL && indexes != NULL && numVerts > 0 && numIndexes > 0 );
	this->verts.SetNum( numVerts );
	memcpy( this->verts.Ptr(), verts, numVerts * sizeof( verts[0] ) );
	this->indexes.SetNum( numIndexes );
	memcpy( this->indexes.Ptr(), indexes, numIndexes * sizeof( indexes[0] ) );
	GenerateEdgeIndexes();
}

/*
====================
idSurface::idSurface
====================
*/
ID_INLINE idSurface::idSurface( const idSurface &surf ) {
	this->verts = surf.verts;
	this->indexes = surf.indexes;
	this->edges = surf.edges;
	this->edgeIndexes = surf.edgeIndexes;
}

/*
====================
idSurface::~idSurface
====================
*/
ID_INLINE idSurface::~idSurface( void ) {
}

/*
=================
idSurface::operator[]
=================
*/
ID_INLINE const idDrawVert &idSurface::operator[]( const int index ) const {
	return verts[ index ];
};

/*
=================
idSurface::operator[]
=================
*/
ID_INLINE idDrawVert &idSurface::operator[]( const int index ) {
	return verts[ index ];
};

/*
=================
idSurface::operator+=
=================
*/
ID_INLINE idSurface &idSurface::operator+=( const idSurface &surf ) {
	int i, m, n;
	n = verts.Num();
	m = indexes.Num();
	verts.Append( surf.verts );			// merge verts where possible ?
	indexes.Append( surf.indexes );
	for ( i = m; i < indexes.Num(); i++ ) {
		indexes[i] += n;
	}
	GenerateEdgeIndexes();
	return *this;
}

/*
=================
idSurface::Clear
=================
*/
ID_INLINE void idSurface::Clear( void ) {
	verts.Clear();
	indexes.Clear();
	edges.Clear();
	edgeIndexes.Clear();
}

/*
=================
idSurface::SwapTriangles
=================
*/
ID_INLINE void idSurface::SwapTriangles( idSurface &surf ) {
	verts.Swap( surf.verts );
	indexes.Swap( surf.indexes );
	edges.Swap( surf.edges );
	edgeIndexes.Swap( surf.edgeIndexes );
}

/*
=================
idSurface::TranslateSelf
=================
*/
ID_INLINE void idSurface::TranslateSelf( const idVec3 &translation ) {
	for ( int i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz += translation;
	}
}

/*
=================
idSurface::RotateSelf
=================
*/
ID_INLINE void idSurface::RotateSelf( const idMat3 &rotation ) {
	for ( int i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz *= rotation;
		verts[i].normal *= rotation;
		verts[i].tangents[0] *= rotation;
		verts[i].tangents[1] *= rotation;
	}
}

#endif /* !__SURFACE_H__ */
