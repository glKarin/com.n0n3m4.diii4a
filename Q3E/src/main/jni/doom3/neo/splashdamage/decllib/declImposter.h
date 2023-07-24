// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLIMPOSTER_H__
#define __DECLIMPOSTER_H__

class idFile_Memory;
class idLexer;

class sdImposterSubImage {
public:

	sdImposterSubImage () {
		rectMins = vec2_origin;
		rectMaxs = idVec2( 1.0f, 1.0f );
	}

	const idVec2 &GetMins( void ) const {
		return rectMins;
	}

	void SetMins( const idVec2 &mins ) {
		rectMins = mins;
	}

	const idVec2 &GetMaxs( void ) const {
		return rectMaxs;
	}

	void SetMaxs( const idVec2 &maxs ) {
		rectMaxs = maxs;
	}

	const idVec2 &GetTexCoord( int index ) const {
		return texCoords[index];
	}

	void SetTexCoord(  int index, const idVec2 &texC ) {
		texCoords[index] = texC;
	}

	void Write( idFile_Memory &f );
	bool Read( idParser &src );

private:
	idVec2 texCoords[4];	// Texture coords for the four corners of the billboard (these may be rotated for rotated textures,...)
	idVec2 rectMins;		// We "clip" the billboard rectangle to the texture coords so we don't sample from adjacent billboard images
	idVec2 rectMaxs;		// this is the actual used part of the rectangle, this is in 0-1 space instead of texture space
};


class sdDeclImposter : public idDecl {
public:
	struct imposterInfo_t {
		idList< sdImposterSubImage >	images;
		const idMaterial*				material;
		idVec3							origin;
		float							scalex;
		float							scaley;
		float							screenScale;
		int								tileSize;
		int								numAngles;
	};

public:
			sdDeclImposter( void );
	virtual ~sdDeclImposter( void ) {}

	// Override from idDecl
	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual size_t			Size( void ) const { return sizeof(sdDeclImposter); }

	static void				CacheFromDict( const idDict& dict );

	// New
	const sdImposterSubImage &	GetSubImage( int index ) const { return info.images[ index ]; }
	sdImposterSubImage &		GetSubImage( int index ) { return info.images[ index ]; }
	const idMaterial *			GetMaterial( void ) const { return info.material; }
	const idVec3 &				GetOrigin( void ) const { return info.origin; }
	float						GetScaleX( void ) const { return info.scalex; }
	float						GetScaleY( void ) const { return info.scaley; }
	float						GetScreenScale( void ) const { return info.screenScale; }
	int							GetNumAngles( void ) const { return info.numAngles; }

	const imposterInfo_t&		GetInfo( void ) const { return info; }

	void						SetMaterial( const idMaterial *material ) { info.material = material; }
	void						SetOrigin( idVec3 &origin ) { info.origin = origin; }
	void						SetScale( float scalex, float scaley ) { info.scalex = scalex; info.scaley = scaley; }
	void						SetScreenScale( float scale ) { info.screenScale = scale; }
	void						ClearSubImages( void ) { info.images.Clear(); }
	void						AddSubImage( const sdImposterSubImage &image ) { info.images.Append( image ); }
	void						SetNumAngles( int num ) { info.numAngles = num; };

	bool						Save( void );
	void						RebuildTextSource( void );

private:
	imposterInfo_t				info;
};

class sdDeclImposterGenerator : public idDecl {
public:

	sdDeclImposterGenerator( void );
	virtual ~sdDeclImposterGenerator( void ) {}

	// Override from idDecl
	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual size_t			Size( void ) const { return sizeof(sdDeclImposterGenerator); }

	// New
	const char *			GetSourceModelName( void ) const { return sourceModel.c_str(); }
	const char *			GetOutputTextureName( void ) const { return outputTexture.c_str(); }
	bool					UseVertexColors( void ) const { return vertexColor; }
	int						GetNumAngles( void ) const { return numAngles; }
	int						GetTileSizeX( void ) const { return tileSize[0]; }
	int						GetTileSizeY( void ) const { return tileSize[1]; }
	bool					GetNoBump( void ) const { return noBump; }
	float					GetStartAngle( void ) const { return startAngle; }
	float					GetScreenScale( void ) const { return screenScale; }

private:
	idStr		sourceModel;
	idStr		outputTexture;
	bool		vertexColor;
	int			numAngles;
	int			tileSize[2];
	bool		noBump;
	float		startAngle;
	float		screenScale;
};

#endif // __DECLIMPOSTER_H__
