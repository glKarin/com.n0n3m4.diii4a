// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLRENDERBINDING_H__
#define __DECLRENDERBINDING_H__

/*
===============================================================================

sdDeclRenderBinding

===============================================================================
*/

class idImage;
class idCinematic;

#include "../renderer/RendererEnums.h"

class sdDeclRenderBinding : public idDecl {
public:

	typedef void (*evaluatorCallback_t)( const sdDeclRenderBinding * );

	enum bindingType_t {
		BT_VECTOR,
		BT_TEXTURE,
		BT_ATTRIB,
	};

									sdDeclRenderBinding() :
										evaluator( NoOpEvaluator ) {
									}

	virtual							~sdDeclRenderBinding() {}

	// Override from idDecl
	virtual const char*				DefaultDefinition( void ) const;
	virtual bool					Parse( const char* text, const int textLength );
	virtual size_t					Size( void ) const { return sizeof( sdDeclRenderBinding ); }
	virtual void					FreeData();

	bindingType_t					GetBindingType() const { return type; }

	void							Set( const float f ) const;
	void							Set( const float f1, const float f2, const float f3, const float f4 ) const;
	void							Set( const float params[4] ) const;
	void							Set( const idVec3& v ) const;
	void							Set( const idVec4& v ) const;
	void							Set( idImage* image ) const;
	void							Set( idCinematic* cinematic ) const;

	float*							GetDefaultVector() const { return const_cast< float* >( defaults.vector ); }
	idImage*						GetDefaultImage() const { return const_cast< idImage* >( defaults.texture.image ); }
	int								GetDefaultAttribIndex() const { return defaults.attrib; }

	float*							GetVector() const { return data.vector; }
	idImage*						GetImage() const { return data.texture.image; }
	int								GetAttribIndex() const { return data.attrib; }

	idVec3&							GetVec3() const { return *(idVec3 *)data.vector; }
	idVec4&							GetVec4() const { return *(idVec4 *)data.vector; }
	textureDepth_t					GetTextureDepth() const { return data.texture.defaultDepth; }
	cubeFiles_t						GetCubeMap() const { return data.texture.defaultCubeMap; }

	int								Infrequent() const { return infrequent; }
	void							SetInfrequentIndex( const int index ) const { infrequent = index; }
	
	// Only for renderer use, the rest of the engine HANDS OFF THE GOODS!
	void							SetEvaluator( evaluatorCallback_t evaluator ) const { this->evaluator = evaluator; }
	void							ClearEvaluator() const { this->evaluator = NoOpEvaluator; }
	void							Evaluate() const;

	virtual void					List( void ) const;


#if defined( _XENON )
	static void						NoOpEvaluator( const sdDeclRenderBinding * ) {}

public:
	mutable evaluatorCallback_t				evaluator;
#endif /* _XENON */

private:
	bool							ParseVector( idParser& src );
	bool							ParseTexture( idParser& src );
	bool							ParseAttrib( idParser& src );

#if !defined( _XENON )
	static void						NoOpEvaluator( const sdDeclRenderBinding * ) {}
#endif /* !_XENON */

private:
	struct textureData_t {
		idImage*		image;
		textureDepth_t	defaultDepth;
		cubeFiles_t		defaultCubeMap;
	};

	union bindingData_t {
		float			vector[4];
		textureData_t	texture;
		int				attrib;
	};

	bindingType_t					type;
	bindingData_t					defaults;
	mutable int						infrequent;
	ALIGN16( mutable bindingData_t	data; )
#if !defined( _XENON )
	mutable evaluatorCallback_t		evaluator;
#endif /* !_XENON */
};

ID_INLINE void sdDeclRenderBinding::Set( const float f ) const {
	data.vector[ 0 ] = data.vector[ 1 ] = data.vector[ 2 ] = data.vector[ 3 ] = f;
}

ID_INLINE void sdDeclRenderBinding::Set( const float f1, const float f2, const float f3, const float f4 ) const {
	data.vector[ 0 ] = f1;
	data.vector[ 1 ] = f2;
	data.vector[ 2 ] = f3;
	data.vector[ 3 ] = f4;
}

ID_INLINE void sdDeclRenderBinding::Set( const float vector[4] ) const {
	data.vector[ 0 ] = vector[ 0 ];
	data.vector[ 1 ] = vector[ 1 ];
	data.vector[ 2 ] = vector[ 2 ];
	data.vector[ 3 ] = vector[ 3 ];
}

ID_INLINE void sdDeclRenderBinding::Set( const idVec3 &v ) const {
	data.vector[ 0 ] = v[ 0 ];
	data.vector[ 1 ] = v[ 1 ];
	data.vector[ 2 ] = v[ 2 ];
	data.vector[ 3 ] = 0.0f;
}

ID_INLINE void sdDeclRenderBinding::Set( const idVec4 &v ) const {
	data.vector[ 0 ] = v[ 0 ];
	data.vector[ 1 ] = v[ 1 ];
	data.vector[ 2 ] = v[ 2 ];
	data.vector[ 3 ] = v[ 3 ];
}

ID_INLINE void sdDeclRenderBinding::Set( idImage* image ) const {
	data.texture.image = image;
}

ID_INLINE void sdDeclRenderBinding::Evaluate() const {
#ifdef _XENON
	if ( evaluator != NoOpEvaluator ) {
		(*evaluator)( this );
	}
#else
	//if ( evaluator ) {
		(*evaluator)( this );
	//}
#endif
}

#endif /* !__DECLRENDERBINDING_H__ */
