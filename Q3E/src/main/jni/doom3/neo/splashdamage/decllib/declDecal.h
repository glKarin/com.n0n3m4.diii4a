// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLDECAL_H__
#define __DECLDECAL_H__

#include "../framework/declManager.h"

class sdDeclDecal : public idDecl {
public:
							sdDeclDecal( void );
	virtual					~sdDeclDecal( void ) {}

	// Override from idDecl
	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual size_t			Size( void ) const { return sizeof(sdDeclDecal); }
	static  void			CacheFromDict( const idDict& dict );
	
	// New for this decl
	idVec4					GetStartColor( void ) const { return startColor; }
	idVec4					GetEndColor( void ) const { return endColor; }
	float					GetLifeTime( void ) const { return lifeTime; }
	const idMaterial*		GetMaterial( void ) const { return material; }
	int						GetNumImages( void ) const { return images.Num(); }
	const sdBounds2D&		GetImage( int index ) const { return images[index]; }

	float					GetSpawnSize( void ) const { return minSize + idRandom::StaticRandom().RandomFloat() * sizeDiff; }
private:
	idVec4	startColor;
	idVec4	endColor;
	float	lifeTime;
	float	minSize;
	float	sizeDiff;
	const idMaterial *material;
	idList< sdBounds2D >	images;
};

#endif // __DECLDECAL_H__
