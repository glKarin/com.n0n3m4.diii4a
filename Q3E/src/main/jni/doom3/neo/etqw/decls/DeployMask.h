// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_DECLS_DEPLOYMASK_H__
#define __GAME_DECLS_DEPLOYMASK_H__

class sdDeployMaskBounds {
public:
	void Init( const sdBounds2D& bounds ) {
		worldBounds	= bounds;
		worldSize	= worldBounds.GetMaxs() - worldBounds.GetMins();
	}

	sdBounds2D						worldBounds;
	idVec2							worldSize;
};

class sdDeployMask {
public:
	typedef deployMaskExtents_t extents_t;

									sdDeployMask( void );
									~sdDeployMask( void );

	bool							Load( const char* _fileName );
	void							Clear( void );

	bool							IsValid( void ) const { return dimensions[ 0 ] > 1 && dimensions[ 1 ] > 1; }

	deployResult_t					IsValid( const idBounds& bounds, const sdDeployMaskBounds& maskBounds ) const;
	deployResult_t					IsValid( int x, int y ) const;
	deployResult_t					IsValid( const extents_t& extents ) const;

	void							CoordsForBounds( const idBounds& _bounds, extents_t& extents, const sdDeployMaskBounds& bounds ) const;
	void							DebugDraw( const sdDeployMaskBounds& bounds ) const;
	void							GetBounds( const sdDeployMask::extents_t& extents, idBounds& bounds, const sdHeightMapInstance* heightMap, const sdDeployMaskBounds& maskBounds ) const;
	idVec3							SnapToGrid( const idVec3& point, float snapScale, const sdDeployMaskBounds& bounds ) const;
	void							ExpandToGrid( idBounds& bounds, const sdDeployMaskBounds& maskBounds ) const;

	void							WriteTGA( void ) const;

	void							SetState( int x, int y, bool state );
	int								GetState( int x, int y ) const;

	float							GetBoxWidth( const sdDeployMaskBounds& bounds ) const { return bounds.worldSize[ 0 ] / dimensions[ 0 ]; }
	float							GetBoxHeight( const sdDeployMaskBounds& bounds ) const { return bounds.worldSize[ 1 ] / dimensions[ 1 ]; }

	void							GetDimensions( int& x, int& y ) const { x = dimensions[ 0 ]; y = dimensions[ 1 ]; }

private:
	idStr							fileName;
	sdBitField_Dynamic				data;
	int								dimensions[ 2 ];
};

#endif // __GAME_DECLS_DEPLOYMASK_H__
