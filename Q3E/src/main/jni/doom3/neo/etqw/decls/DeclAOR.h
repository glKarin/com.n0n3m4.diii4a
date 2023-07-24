// Copyright (C) 2007 Id Software, Inc.
//

#ifndef		__DECLAOR_H__
#define		__DECLAOR_H__

class sdDeclAOR : public idDecl {
public:
								sdDeclAOR( void );
	virtual						~sdDeclAOR( void );

	virtual const char*			DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );

	void						Clear( void );

	float						GetSpreadForDistanceSqr( float distanceSqr ) const;
	int							GetFlagsForDistanceSqr( float distanceSqr ) const;
	void						UpdateCutoffs( void ) const;

	static void					InitCVars( void );
	static void					ShutdownCVars( void );
	static void					OnCutoffChanged( void );

	float						GetPhysicsCutoffSqr( void ) const { return activeCutoffs.physicsCutoffSqr; }

private:

	float		spreadScale;
	float		spreadStart;
	float		spreadDistance;
	float		userCommandCutoffSqr;				//	AOR_INHIBIT_USERCMDS		= BITT< 0 >::VALUE,

	struct clientCutoffDistances_t {
		float		animationCutoffSqr;				//	AOR_INHIBIT_ANIMATION		= BITT< 1 >::VALUE,
		float		ikCutoffSqr;					//	AOR_INHIBIT_IK				= BITT< 2 >::VALUE,
		float		physicsCutoffSqr;				//	AOR_INHIBIT_PHYSICS			= BITT< 3 >::VALUE,

		float		boxDecayClipStartSqr;			//	AOR_BOX_DECAY_CLIP			= BITT< 4 >::VALUE,
		float		pointDecayClipStartSqr;			//	AOR_POINT_DECAY_CLIP		= BITT< 5 >::VALUE,
		float		heightMapDecayClipStartSqr;		//	AOR_HEIGHTMAP_DECAY_CLIP	= BITT< 6 >::VALUE,
		float		decayClipEndSqr;

		float		physicsLOD1StartSqr;			//	AOR_PHYSICS_LOD_1			= BITT< 7 >::VALUE,
		float		physicsLOD2StartSqr;			//	AOR_PHYSICS_LOD_2			= BITT< 8 >::VALUE,
		float		physicsLOD3StartSqr;			//	AOR_PHYSICS_LOD_3			= BITT< 9 >::VALUE,
	};

	clientCutoffDistances_t			baseCutoffs;
	mutable clientCutoffDistances_t	activeCutoffs;
};

#endif	//	__DECLAOR_H__
