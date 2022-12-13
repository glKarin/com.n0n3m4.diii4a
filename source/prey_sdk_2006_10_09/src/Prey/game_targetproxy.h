
#ifndef __GAME_TARGETPROXY_H__
#define __GAME_TARGETPROXY_H__

class hhModelProxy : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhModelProxy );

	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	//rww - net code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

	void					SetOriginal(idEntity *other);
	idEntity *				GetOriginal();
	virtual void			ProxyFinished();
	virtual void			Ticker();
	virtual void			UpdateVisualState();
	virtual void			SetOriginAndAxis( idEntity * );
	void					SetOwner(hhPlayer *other);
	hhPlayer *				GetOwner();

protected:
	//rww - these need to be entityptr's, not raw pointers
	idEntityPtr<idEntity>	original;
	idEntityPtr<hhPlayer>	owner;
};


#define BOWVISION_FADEIN_DURATION		1500
#define BOWVISION_FADEOUT_DURATION		1500
#define BOWVISION_UPDATE_FREQUENCY		12*MAX_GENTITIES/BOWVISION_ROVER_STRIDE		// Rate at which to search for proxies
#define BOWVISION_ROVER_STRIDE			25	// Number of entities to check each tick

class hhTargetProxy : public hhModelProxy {
public:
	CLASS_PROTOTYPE( hhTargetProxy );

	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	//rww - net code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	void					StayAlive();
	virtual void			ProxyFinished();
	virtual void			UpdateVisualState();

protected:
	void					Event_FadeOut();

protected:
	idInterpolate<float>	fadeAlpha;
};


#endif
