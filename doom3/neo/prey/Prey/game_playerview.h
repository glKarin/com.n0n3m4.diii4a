#ifndef __PREY_GAME_PLAYERVIEW_H__
#define __PREY_GAME_PLAYERVIEW_H__

class hhPlayerView : public idPlayerView {
public:
						hhPlayerView();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual	void		ClearEffects( void );
	virtual void		DamageImpulse( idVec3 localKickDir, const idDict *damageDef );
	virtual void		RenderPlayerView( idUserInterface *hud );

	void				WeaponFireFeedback( const idDict *weaponDef );			// HUMANHEAD bjk
	idAngles			AngleOffset( float kickSpeed, float kickReturnSpeed );	// HUMANHEAD bjk

	// HUMANHEAD
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	void				MotionBlur(int mbTime, float severity, idVec3 &direction);
	idVec3				ViewOffset() { return viewOffset; }//HUMANHEAD: aob
	void				SetViewOverlayMaterial( const idMaterial* material, int scratchBuffer = false );
	void				SetViewOverlayColor( idVec4 color ); // HUMANHEAD cjr
	void				SetViewOverlayTime( int time, int scratchBuffer = false );
	void				SetLetterBox(bool on);
	void				SetDamageLoc(const idVec3 &damageLoc);
	// HUMANHEAD END

protected:
	virtual void		SingleView(idUserInterface *hud, const renderView_t *view);

	// HUMANHEAD pdm
	void				MotionBlurVision(idUserInterface *hud, const renderView_t *view);
	void				ApplyLetterBox(const renderView_t *view);
	void				SpiritVision( idUserInterface *hud, const renderView_t *view );

protected:
	bool				bLetterBox;				// HUMANHEAD pdm: whether we are in letterbox mode
	float				mbAmplitude;
	int					mbFinishTime;
	int					mbTotalTime;
	idVec3				mbDirection;
	int					kickLastTime;			// HUMANHEAD bjk
	idAngles			kickSpeed;				// HUMANHEAD bjk
	idVec3				viewOffset;				// HUMANHEAD: aob
	const idMaterial *	viewOverlayMaterial;	// HUMANHEAD: aob
	idVec4				viewOverlayColor;		// HUMANHEAD cjr: Used to pass colors/parms to the view overlay
	int					voFinishTime;			// HUMANHEAD cjr: Used to set the time that the view overlay stays on
	int					voTotalTime;			// HUMANHEAD cjr: Used to set the time that the view overlay stays on
	int					voRequiresScratchBuffer;// HUMANHEAD cjr: requires the screen rendered to the scratch buffer
	const idMaterial	*letterboxMaterial;		// HUMANHEAD pdm
	const idMaterial	*dirDmgLeftMaterial;	// HUMANHEAD pdm
	const idMaterial	*dirDmgFrontMaterial;	// HUMANHEAD pdm
	const idMaterial	*spiritMaterial;		// HUMANHEAD cjr
	idVec3				lastDamageLocation;		// HUMANHEAD PDM: saved damage location
	float				hurtValue;				// HUMANHEAD bjk: to smooth the health
};

//HUMANHEAD rww - render demo madness
#if _HH_RENDERDEMO_HACKS
	void RENDER_DEMO_VIEWRENDER(const renderView_t *view, const hhPlayerView *pView = NULL);
	void RENDER_DEMO_VIEWRENDER_END(void);
#else
	#define RENDER_DEMO_VIEWRENDER(a, b)//ignore
	#define RENDER_DEMO_VIEWRENDER_END() //ignore
#endif
//HUMANHEAD END


//------------------------------------------------------
// hhPlayerView::SetViewOverlayMaterial
//
//	HUMANHEAD: aob
//------------------------------------------------------
ID_INLINE void hhPlayerView::SetViewOverlayMaterial( const idMaterial* material, int scratchBuffer ) {
	viewOverlayMaterial = material;

	voTotalTime = -1;
	voFinishTime = gameLocal.time + voTotalTime;
	voRequiresScratchBuffer = scratchBuffer;
}

//------------------------------------------------------
// hhPlayerView::SetViewOverlayColor
//
//	HUMANHEAD: cjr
//------------------------------------------------------
ID_INLINE void hhPlayerView::SetViewOverlayColor( idVec4 color ) {
	viewOverlayColor = color;
}

//------------------------------------------------------
// hhPlayerView::SetViewOverlayTime
//
//	HUMANHEAD: cjr
//
//  Time can be set in SetViewOverlayMaterial
//  or set/updated with this function call
//------------------------------------------------------
ID_INLINE void hhPlayerView::SetViewOverlayTime( int time, int scratchBuffer ) {
	voTotalTime = time;
	voFinishTime = gameLocal.time + voTotalTime;
	voRequiresScratchBuffer = scratchBuffer;
}

#endif /* __PREY_GAME_PLAYERVIEW_H__ */

