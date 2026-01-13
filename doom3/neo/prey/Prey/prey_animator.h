

#ifndef __PREY_GAME_ANIMATOR_H__
#define __PREY_GAME_ANIMATOR_H__

class hhAnimator : public idAnimator {
 public:
	
						hhAnimator();

	// Overridden methods
	bool				FrameHasChanged( int animtime ) const;
	bool				IsAnimating( int currentTime ) const;

	// Unique methods					
	void				CycleAnim( int channelNum, int anim, int currenttime, int blendtime, const idEventDef *pEvent = NULL );
	void				CheckCycleRotate();
	void				CheckThaw();
	int					CheckTween( int channelNum );	

	bool				IsAnimPlaying( int channelNum, const idAnim *anim );
	//bool				IsAnimPlaying( int channelNum, int animNum ) { return( IsAnimPlaying( channelNum, GetAnim( animNum ) ) ); }
	bool				IsAnimPlaying( int channelNum, const char* anim );
	bool				IsAnimPlaying( const idAnim *anim );
	//bool				IsAnimPlaying( int animNum ) { return( IsAnimPlaying( GetAnim( animNum ) ) ); }
	bool				IsAnimPlaying( const char* anim );
	bool				IsAnimatedModel() const { return modelDef != NULL; }

	int					GetNumAnimVariants( int anim );

	const idAnimBlend	*GetBlendAnim( int channelNum, int index ) const;
	// Backwards compat functions
	idAnimBlend *		FindAnim( int channelNum, const idAnim *anim ); // JRM: removed const - couldn't compile
	const idAnimBlend	*GetBlendAnim( int index ) const { return( GetBlendAnim( ANIMCHANNEL_ALL, index ) ); };
	int					NumBlendAnims( int currentTime );

	void				CopyAnimations( hhAnimator &animator );
	void				CopyPoses( hhAnimator &animator );
	bool				Freeze();
	bool				Thaw();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

 protected:

	int					lastCycleRotate;
};

#endif /* __PREY_GAME_ANIMATOR_H__ */
