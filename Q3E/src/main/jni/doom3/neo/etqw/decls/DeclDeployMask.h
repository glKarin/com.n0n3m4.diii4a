// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLDEPLOYMASK_H__
#define __DECLDEPLOYMASK_H__

#include "DeployMask.h"

class sdDeclDeployMask : public idDecl {
public:
							sdDeclDeployMask( void );
	virtual					~sdDeclDeployMask( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	static void				CacheFromDict( const idDict& dict );

	const sdDeployMask&		GetMask( void ) const { return deployMask; }

protected:
	sdDeployMask			deployMask;
};

class sdDeployMaskInstance {
public:
							sdDeployMaskInstance( void ) : deployMask ( NULL ) { ; }
	void					Init( const char* declName, const sdBounds2D& bounds );

	bool					IsValid( void ) const { return deployMask != NULL; }

	deployResult_t IsValid( const idBounds& bounds ) const {
		return deployMask->IsValid( bounds, deployMaskData );
	}

	deployResult_t IsValid( int x, int y ) const {
		return deployMask->IsValid( x, y );
	}

	deployResult_t IsValid( const deployMaskExtents_t& extents ) const {
		return deployMask->IsValid( extents );
	}

	void CoordsForBounds( const idBounds& _bounds, deployMaskExtents_t& extents ) const {
		deployMask->CoordsForBounds( _bounds, extents, deployMaskData );
	}

	void GetDimensions( int& x, int& y ) const {
		deployMask->GetDimensions( x, y );
	}

	void WriteTGA( void ) const {
		deployMask->WriteTGA();
	}

	void SetState( int x, int y, bool state ) {
		const_cast< sdDeployMask* >( deployMask )->SetState( x, y, state );
	}

	int GetState( int x, int y ) const {
		return deployMask->GetState( x, y );
	}

	void GetBounds( const deployMaskExtents_t& extents, idBounds& bounds, const sdHeightMapInstance* heightMap ) const {
		deployMask->GetBounds( extents, bounds, heightMap, deployMaskData );
	}

	idVec3 SnapToGrid( const idVec3& point, float snapScale ) const {
		return deployMask->SnapToGrid( point, snapScale, deployMaskData );
	}

	void DebugDraw( void ) const {
		deployMask->DebugDraw( deployMaskData );
	}

private:
	const sdDeployMask*		deployMask;
	sdDeployMaskBounds		deployMaskData;
};

#endif // __DECLDEPLOYMASK_H__
