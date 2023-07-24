// Copyright (C) 2007 Id Software, Inc.
//

#ifndef _BSE_INTERFACE_H_INC_
#define _BSE_INTERFACE_H_INC_

class idCVar;
class rvDeclEffect;

const char* const BSE_EFFECT_EXTENSION = "effect";

enum
{
	VIEWEFFECT_SHAKE = 0,
	VIEWEFFECT_TUNNEL
};

typedef enum {
	EC_IGNORE = 0,
	EC_IMPACT,
	EC_IMPACT_PARTICLES,

	EC_MAX,
} effectCategory_t;

extern	idCVar				bse_enabled;
extern	idCVar				bse_render;
extern	idCVar				bse_debug;
extern	idCVar				bse_showBounds;
extern	idCVar				bse_physics;
extern	idCVar				bse_debris;
extern	idCVar				bse_singleEffect;
extern	idCVar				bse_maxParticles;
extern	idCVar				bse_detailLevel;
extern	idCVar				bse_simple;

// Interface to the effects system

class rvBSEManager
{
public:
	virtual						~rvBSEManager( void ) {}	

	virtual	bool				Init( void ) = 0;
	virtual	bool				Shutdown( void ) = 0;

	virtual	bool				PlayEffect( class rvRenderEffectLocal *def, float time ) = 0;
	virtual	bool				ServiceEffect( class rvRenderEffectLocal *def, float time, bool &forcePush ) = 0;
	virtual	void				StopEffect( rvRenderEffectLocal *def ) = 0;
	virtual	void				RestartEffect( rvRenderEffectLocal *def ) = 0;
	virtual	void				FreeEffect( rvRenderEffectLocal *def ) = 0;
	virtual	float				EffectDuration( const rvRenderEffectLocal *def ) = 0;

	virtual	void				BeginLevelLoad( void ) = 0;
	virtual	void				EndLevelLoad( void ) = 0;

	virtual	void				StartFrame( void ) = 0;
	virtual	void				EndFrame( void ) = 0;
	virtual bool				Filtered( const char *name, effectCategory_t category ) = 0;

	virtual void				UpdateRateTimes( void ) = 0;
	virtual bool				CanPlayRateLimited( effectCategory_t category ) = 0;

	virtual int					AddTraceModel( idTraceModel* model ) = 0;
	virtual idTraceModel*		GetTraceModel( int index ) = 0;
	virtual void				FreeTraceModel( int index ) = 0;

	virtual const idVec3&		GetCubeNormals( int index ) = 0;

	virtual void				SetShakeParms( float time, float scale ) = 0;
	virtual void				SetTunnelParms( float time, float scale ) = 0;

	virtual const idMat3&		GetModelToBSE() = 0;

	virtual bool				IsTimeLocked() const = 0;
	virtual float				GetLockedTime() const = 0;

	virtual void				MakeEditable( class rvParticleTemplate *particle ) = 0;

	virtual void				CopySegment( class rvSegmentTemplate *dest, class rvSegmentTemplate *src ) = 0;
};

extern	rvBSEManager			*bse;

#endif // _BSE_INTERFACE_H_INC_
