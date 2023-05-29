#ifndef __HH_MISC_H
#define __HH_MISC_H

#define WALLWALK_TRANSITION_TIME	2510		// Time over which to fade
#define WALLWALK_HERM_S1			1.0f		// Slope at start
#define WALLWALK_HERM_S2			-2.0f		// Slope at end

class hhWallWalkable : public idStaticEntity {
	CLASS_PROTOTYPE(hhWallWalkable);

public:
	void						Spawn( void );
	void						Save( idSaveGame *savefile ) const;
	void						Restore( idRestoreGame *savefile );

	//rww - netcode
	virtual void				WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void				ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void				ClientPredictionThink( void );

	void						SetWallWalkable(bool on);
	virtual void				Think();

protected:
	void						Event_Activate(idEntity *activator);

protected:
	hhHermiteInterpolate<float>	alphaOn;		// Degree to which wallwalk is on
	bool						wallwalkOn;
	bool						flicker;
	const idDeclSkin			*onSkin;
	const idDeclSkin			*offSkin;
};

class hhFuncEmitter : public idStaticEntity {
	CLASS_PROTOTYPE( hhFuncEmitter );

public:
	void				Spawn( void );

	virtual void		Hide();
	virtual void		Show();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

protected:
	virtual void		Ticker();

protected:
	void				Event_Activate( idEntity *activator );

protected:
	const idDeclParticle* particle;
	int					particleStartTime;
};


class hhPathEmitter : public hhFuncEmitter {
	CLASS_PROTOTYPE( hhPathEmitter );

public:
	void				Spawn( void );
};


class hhDeathWraithEnergy : public hhPathEmitter {
	CLASS_PROTOTYPE( hhDeathWraithEnergy );

public:
	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetPlayer(hhPlayer *player);
	void					SetDestination(const idVec3 &destination);

protected:
	virtual void			Ticker();
	idVec3					CylindricalToCartesian(float radius, float theta, float z);
	void					CartesianToCylindrical(idVec3 &cartesian, float &radius, float &theta, float &z);

protected:
	hhTCBSpline				spline;
	float					startTime;
	float					duration;
	float					startRadius;
	float					endRadius;
	float					startTheta;
	float					endTheta;
	float					startZ;
	float					endZ;
	idVec3					centerPosition;
	idEntityPtr<hhPlayer>	thePlayer;
};

#endif
