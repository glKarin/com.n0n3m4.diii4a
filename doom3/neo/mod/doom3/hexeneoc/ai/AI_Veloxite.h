#ifndef __AI_VELOXITE_H__
#define __AI_VELOXITE_H__

typedef enum v_stype {
	v_none = 0,
	v_slope = 1,
	v_wall = 2
};

class idAI_Veloxite : public idAI {

public:
	CLASS_PROTOTYPE( idAI_Veloxite );

	void	Spawn( void );
	void	Think( void );
	void	LinkScriptVariables( void );
	void	Save( idSaveGame *savefile ) const;
	void	Restore( idRestoreGame *savefile );

// ***********************************************************

// Helper Functions

// ***********************************************************
private:
	ID_INLINE bool				onWall( void );
	ID_INLINE bool				wallIsWalkable( idEntity *wall );
	ID_INLINE v_stype			surfaceType( idVec3 normal );


// ***********************************************************

// Class Method Declarations

// ***********************************************************
private:
// surface checks etc
	float				checkSurfaces( void );
	bool				checkSlope( void );
	bool				checkLedge( void );
	bool				checkWall( void );
	void				getOnSurface( const idVec3 &norm, int numt = 0 );
	void				getOffSurface( bool fall );
	void				doSurfaceTransition( void );
	void				postSurfaceTransition( void );
// player chasing / attacks
	bool				checkDropAttack( void );
//	bool				checkParallel( void ); // if enemy is above velox (relative to velox's gravity) and there there is floor to drop on, do it.
// fix oops's
	bool				checkFallingSideways( void ); // if we're going faster than 300, we must be falling, make sure it's truly straight down!
	bool				checkHovering( void ); // if we're AI_ONGROUND but origin is not above a surface, we're hovering over a ledge. do something...

// ***********************************************************

// Variables

// ***********************************************************
private:
// transitions
	bool	doPostTrans;
	bool	doTrans;
	int		curTrans;
	int		numTrans; // in frames
	idVec3	transGrav;
	idVec3	destGrav;
	idVec3	upVec;
// other stuff
	trace_t	trace;
	float	nextWallCheck;
	float	maxGraceTime;
	float	debuglevel;
	idVec3	veloxMins;
	idVec3	veloxMaxs;
	idVec3	traceMins;float next;
	idVec3	traceMaxs;

/* Debug Level:
	1 - useful functions that dont happen every frame
	5 - useful functions that happen every frame
	10 - unuseful functions that happen every frame
	15 - unuseful functions that may happen more than every frame
	20 - things you'll functions never want to see
*/

// ***********************************************************

// Script Stuff

// ***********************************************************
protected:
	idScriptBool			AI_ALIGNING;
	idScriptBool			AI_WALLING;
	idScriptBool			AI_LEDGING;
	idScriptBool			AI_SLOPING;
	idScriptBool			AI_LEAPING;
	idScriptBool			AI_FALLING;
	idScriptBool			AI_DROPATTACK;
	idScriptBool			AI_ONWALL;
private:
	void				Event_doSurfaceChecks( void );
	void				Event_doneLeaping( void );
	void				Event_startLeaping( void );
	void				Event_getVeloxJumpVelocity( float speed, float max_height, float channel, const char *animname );
	void				Event_getOffWall( float fall );
};

#endif // __AI_VELOXITE_H__
