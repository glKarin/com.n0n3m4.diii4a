// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_EFFECTS_H__
#define __GAME_EFFECTS_H__

//===============================================================
//
//	sdEffect
//
//===============================================================

class sdEffect {
public:
							sdEffect( void );
	virtual					~sdEffect( void );

	void					Init( const char* effectName );
	bool					Start( int startTime );
	void					Update( void );
	void					Stop( bool appendToDieList = false );	// Stops spawning new particles, calling start again will overwrite any particles currently existing and start over
	void					StopDetach( void );						// Stops spawning new particles, calling start agian will start a new effect and let any existing particles die naturally

	renderEffect_t&			GetRenderEffect( void ) { return effect; }
	const renderEffect_t&	GetRenderEffect( void ) const { return effect; }

	idLinkList< sdEffect >&	GetNode( void ) { return effectNode; }
	void					FreeRenderEffect( void ) { Free(); }

	static void				UpdateDeadEffects( void );
	static void 			FreeDeadEffects( void ); 

protected:
	renderEffect_t			effect;
	qhandle_t				effectHandle;
	bool					effectStopped;
	bool					waitingToDie;
	idLinkList< sdEffect >	effectNode;

protected:
	void					Init( void );
	void					Free( void );

private:

	struct DeadEffect {
		DeadEffect( void ) {}
		DeadEffect( const sdEffect &eff ) {
			effect = eff.effect;
			effectHandle = eff.effectHandle;
		}
		renderEffect_t			effect;
		qhandle_t				effectHandle;
	};

	static idList<DeadEffect>	waitingToDieList;
};

/*
================
sdEffect::sdEffect
================
*/
ID_INLINE sdEffect::sdEffect( void ) {
	Init();
}

/*
================
sdEffect::~sdEffect
================
*/
ID_INLINE sdEffect::~sdEffect( void ) {
	Free();
}

#endif /* !__GAME_EFFECTS_H__ */
