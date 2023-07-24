// Copyright (C) 2007 Id Software, Inc.
//

//----------------------------------------------------------------
// ClientEffect.h
//----------------------------------------------------------------

#ifndef __GAME_CLIENT_EFFECT_H__
#define __GAME_CLIENT_EFFECT_H__

#include "ClientEntity.h"

class rvClientEffect : public sdClientScriptEntity {
public:

	CLASS_PROTOTYPE( rvClientEffect );

	rvClientEffect ( void );
	rvClientEffect ( int effectHandle );
	~rvClientEffect ( void );

	virtual void		Think( void );
	virtual void		ClientUpdateView( void );

	bool				Play( int startTime, bool loop = false, const idVec3& origin = vec3_origin );
	void				Stop( bool destroyParticles = false );
	void				Restart( void );

	void				Attenuate( float attenuation );

	int					GetEffectIndex( void ) const;
	renderEffect_t*		GetRenderEffect( void );

	virtual void		DrawDebugInfo( void ) const;

	void				SetEndOrigin( const idVec3& endOrigin );
	void				SetEndOrigin( jointHandle_t joint );

	const idVec3&		GetEndOrigin( void ) const { return renderEffect.endOrigin; }

	void				SetGravity( const idVec3& gravity );

	void				SetColor( const idVec4& color );
	void				SetMaterialColor( const idVec3& color );
	void				SetBrightness( float brightness );
	void				SetSuppressInViewID( int id );
	void				SetDistanceOffset( float distanceOffset );
	void				SetMaxVisDist( float naxVisDist );
	void				SetRenderBounds( bool renderBounds );

	void				SetViewSuppress( bool vs );

	void				Monitor( idEntity *ent );

	virtual void		FreeEntityDef( void );

	virtual const char*	GetName( void ) const;// { return "rvClientEffect"; }

protected:

	virtual void		UpdateBind( bool skipModelUpdate );

	void				Init( int _effectIndex );
	void				FreeEffectDef( void );

	void				Event_SetEffectEndOrigin( const idVec3& endOrg );
	void				Event_SetEffectLooping( bool looping );
	void				Event_UseRenderBounds( bool rb );
	void				Event_EndEffect( bool destroyParticles );

	renderEffect_t		renderEffect;
	int					effectDefHandle;
	int					startTime;
	int					effectIndex;
	jointHandle_t		endOriginJoint;
	bool				viewSuppress;

	int					monitorSpawnId;
};

ID_INLINE int rvClientEffect::GetEffectIndex ( void ) const {
	return effectIndex;
}

ID_INLINE void rvClientEffect::SetEndOrigin	( const idVec3& endOrigin ) {
	renderEffect.endOrigin = endOrigin;
	renderEffect.hasEndOrigin = !(endOrigin == vec3_origin);
}

ID_INLINE void rvClientEffect::SetEndOrigin	( jointHandle_t joint ) {
	endOriginJoint = joint;
}

ID_INLINE void rvClientEffect::SetGravity( const idVec3& gravity ) {
	renderEffect.gravity = gravity;
}

ID_INLINE void rvClientEffect::SetColor ( const idVec4& color ) {
	renderEffect.shaderParms[SHADERPARM_RED] = color[0];
	renderEffect.shaderParms[SHADERPARM_GREEN] = color[1];
	renderEffect.shaderParms[SHADERPARM_BLUE] = color[2];
	renderEffect.shaderParms[SHADERPARM_ALPHA] = color[3];
}

ID_INLINE void rvClientEffect::SetMaterialColor ( const idVec3& color ) {
	renderEffect.materialColor = color;
}

ID_INLINE void rvClientEffect::SetBrightness ( float brightness ) {
	renderEffect.shaderParms[SHADERPARM_BRIGHTNESS] = brightness;
}

ID_INLINE void rvClientEffect::SetSuppressInViewID ( int id ) {
	renderEffect.suppressSurfaceInViewID = id;
}

ID_INLINE void rvClientEffect::SetDistanceOffset( float distanceOffset ) {
	renderEffect.distanceOffset = distanceOffset;
}

ID_INLINE void rvClientEffect::SetMaxVisDist( float maxVisDist ) {
	renderEffect.maxVisDist = maxVisDist;
}

ID_INLINE void rvClientEffect::SetRenderBounds( bool renderBounds ) {
	renderEffect.useRenderBounds = renderBounds;
}

ID_INLINE void rvClientEffect::SetViewSuppress( bool vs ) {
	viewSuppress = vs;
}



ID_INLINE renderEffect_t* rvClientEffect::GetRenderEffect ( void ) {
	return &renderEffect;
}

//----------------------------------------------------------------
//						rvClientCrawlEffect
//----------------------------------------------------------------

class idAnimatedEntity;

class rvClientCrawlEffect : public rvClientEffect {
public:

	CLASS_PROTOTYPE( rvClientCrawlEffect );

	rvClientCrawlEffect				( void );
	rvClientCrawlEffect				( int _effectIndex , idEntity* ent, int crawlTime, idList<jointHandle_t>* joints = NULL );
	~rvClientCrawlEffect			( void ) {}

	virtual void		Think		( void );

protected:

	idList<jointHandle_t>			crawlJoints;
	int								crawlTime;
	int								nextCrawl;
	int								jointStart;
	int								jointEnd;
	int								crawlDir;
	idEntityPtr<idAnimatedEntity>	crawlEnt;
};

typedef rvClientEntityPtr<rvClientEffect>	rvClientEffectPtr;

#endif // __GAME_CLIENT_EFFECT_H__
