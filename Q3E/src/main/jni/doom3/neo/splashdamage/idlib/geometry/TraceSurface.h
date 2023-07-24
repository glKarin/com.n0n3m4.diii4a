// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __TRACESURFACE_H__
#define __TRACESURFACE_H__

/*
===============================================================================

	Traceable surface.

===============================================================================
*/

const int DEFAULT_HASH_AXIS_BINS	= 128;		// must be a power of two
const int MAX_LINKS_PER_BLOCK		= 0x100000;
const int MAX_LINK_BLOCKS			= 0x100;

class sdTraceSurface {
public:
						explicit sdTraceSurface( const idDrawVert* verts, const int numVerts, const vertIndex_t* indexes, const int numIndexes, const int hashBinsPerAxis = DEFAULT_HASH_AXIS_BINS );
						~sdTraceSurface();

	bool				RayIntersection( const idVec3& start, const idVec3& end, idDrawVert& dv ) const;

private:
	struct triLink_t {
		int faceNum;
		int nextLink;
	};

	void				CreateHash();
	void				GetNextHashBin( const idVec3& point, const idVec3& normal, int hashBin[3], int& separatorAxis, float& separatorDist ) const;

	float				TraceToMeshFace( const int faceNum, const idVec3& point, const idVec3 &normal, const float faceDist, const float traceDist, idDrawVert& dv ) const;
	sdTraceSurface&		operator=( const sdTraceSurface& rhs );

public:
	const idDrawVert*	verts;
	const int			numVerts;
	const vertIndex_t*	indexes;
	const int			numIndexes;

	idBounds			_bounds;
	idPlane*			facePlanes;

	// the hash
	float				binSize[ 3 ];
	float				invBinSize[ 3 ];

	int					binsPerAxis;

	int					numLinkBlocks;
	triLink_t*			linkBlocks[ MAX_LINK_BLOCKS ];

	int***				binLinks;
};

#endif /* !__TRACESURFACE_H__ */
