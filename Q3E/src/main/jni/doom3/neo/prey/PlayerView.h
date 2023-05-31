// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_PLAYERVIEW_H__
#define __GAME_PLAYERVIEW_H__

/*
===============================================================================

  Player view.

===============================================================================
*/

// screenBlob_t are for the on-screen damage claw marks, etc
typedef struct {
	const idMaterial *	material;
	float				x, y, w, h;
	float				s1, t1, s2, t2;
	int					finishTime;
	int					startFadeTime;
	float				driftAmount;
} screenBlob_t;

#define	MAX_SCREEN_BLOBS	8

class idPlayerView {
public:
						idPlayerView();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				SetPlayerEntity( class hhPlayer *playerEnt );	//HUMANHEAD bjk

	virtual			// HUMANHEAD
	void				ClearEffects( void );

	virtual			// HUMANHEAD
	void				DamageImpulse( idVec3 localKickDir, const idDict *damageDef );

	void				WeaponFireFeedback( const idDict *weaponDef );

	// HUMANHEAD bjk
	idAngles			AngleOffset( float kickSpeed, float kickReturnSpeed );			// returns the current kick angle

	idMat3				ShakeAxis( void ) const;			// returns the current shake angle

	void				CalculateShake( void );

	// this may involve rendering to a texture and displaying
	// that with a warp model or in double vision mode
	virtual	// HUMANHEAD
	void				RenderPlayerView( idUserInterface *hud );

	void				Fade( idVec4 color, int time );

	void				Flash( idVec4 color, int time );

//	void				AddBloodSpray( float duration );				// HUMANHEAD pdm: not used

	// temp for view testing
//	void				EnableBFGVision( bool b ) { bfgVision = b; };	// HUMANHEAD pdm: not used

	//---------------------
protected:	// HUMANHEAD
	virtual			// HUMANHEAD
	void				SingleView( idUserInterface *hud, const renderView_t *view );
// HUMANHEAD pdm: not used
//	void				DoubleVision( idUserInterface *hud, const renderView_t *view, int offset );
//	void				BerserkVision( idUserInterface *hud, const renderView_t *view );
//	void				InfluenceVision( idUserInterface *hud, const renderView_t *view );
	void				ScreenFade();

	screenBlob_t *		GetScreenBlob();

	screenBlob_t		screenBlobs[MAX_SCREEN_BLOBS];

	int					dvFinishTime;		// double vision will be stopped at this time
	const idMaterial *	dvMaterial;			// material to take the double vision screen shot
	const idMaterial *	scratchMaterial;	// HUMANHEAD bjk
	const idMaterial *	hurtMaterial;		// HUMANHEAD bjk

	int					kickFinishTime;		// view kick will be stopped at this time
	idAngles			kickAngles;	

//	bool				bfgVision;			// HUMANHEAD pdm: not used

//	const idMaterial *	tunnelMaterial;		// health tunnel vision
//	const idMaterial *	armorMaterial;		// armor damage view effect
//	const idMaterial *	berserkMaterial;	// berserk effect
//	const idMaterial *	irGogglesMaterial;	// ir effect
//	const idMaterial *	bloodSprayMaterial; // blood spray
//	const idMaterial *	bfgMaterial;		// when targeted with BFG
	const idMaterial *	lagoMaterial;		// lagometer drawing
	float				lastDamageTime;		// accentuate the tunnel effect for a while

	idVec4				fadeColor;			// fade color
	idVec4				fadeToColor;		// color to fade to
	idVec4				fadeFromColor;		// color to fade from
	float				fadeRate;			// fade rate
	int					fadeTime;			// fade time

	idAngles			shakeAng;			// from the sound sources

	hhPlayer *			player;				//HUMANHEAD bjk
	renderView_t		view;
};

#endif /* !__GAME_PLAYERVIEW_H__ */
