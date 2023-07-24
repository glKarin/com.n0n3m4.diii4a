// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLHEIGHTMAP_H__
#define __DECLHEIGHTMAP_H__

class sdDeclHeightMap : public idDecl {
public:
							sdDeclHeightMap( void );
	virtual					~sdDeclHeightMap( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	static void				CacheFromDict( const idDict& dict );

	const sdHeightMap&		GetHeightMap( void ) const { return heightMap; }

protected:
	sdHeightMap				heightMap;
};

class sdHeightMapInstance {
public:
							sdHeightMapInstance( void ) : heightMap ( NULL ) { ; }
	void					Init( const char* declName, const idBounds& bounds );
	void					Init( const sdHeightMap *map, const idBounds& bounds );

	bool					IsValid( void ) const { return heightMap != NULL; }

	float GetHeight( const idVec3& pos ) const {
		assert( heightMap != NULL );
		return heightMap->GetHeight( pos, heightMapData );
	}

	float GetInterpolatedHeight( const idVec3& pos ) const {
		assert( heightMap != NULL );
		return heightMap->GetInterpolatedHeight( pos, heightMapData );
	}

	void GetHeight( const idBounds& pos, idVec2& out ) const {
		assert( heightMap != NULL );
		heightMap->GetHeight( pos, out, heightMapData );
	}

	float GetHeight( const idVec3& start, const idVec3& end ) const {
		assert( heightMap != NULL );
		return heightMap->GetHeight( start, end, heightMapData );
	}

	float TracePoint( const idVec3& start, const idVec3& end, idVec3& result, float heightOffset = 0.0f ) const {
		assert( heightMap != NULL );
		return heightMap->TracePoint( start, end, result, heightOffset, heightMapData );
	}


private:
	const sdHeightMap*		heightMap;
	sdHeightMapScaleData	heightMapData;
};

#endif // __DECLHEIGHTMAP_H__
