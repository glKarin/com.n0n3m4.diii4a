#ifndef __GAME_ANIMATEDENTITY_H
#define __GAME_ANIMATEDENTITY_H

extern const idEventDef EV_CheckCycleRotate;
extern const idEventDef EV_CheckThaw;
extern const idEventDef EV_SpawnFxAlongBone;

//HUMANHEAD rww
typedef struct jawFlapInfo_s {
	jointHandle_t	bone;
	idVec3			rMagnitude;
	idVec3			tMagnitude;
	float			rMinThreshold;
	float			tMinThreshold;
} jawFlapInfo_t;
//HUMANHEAD END

class hhAnimatedEntity : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhAnimatedEntity );
	
	void						Spawn( void );
								hhAnimatedEntity();
	virtual						~hhAnimatedEntity();

	void						Think();

	virtual hhAnimator *		GetAnimator( void );
	virtual const hhAnimator *	GetAnimator( void ) const;
	virtual void				FillDebugVars( idDict *args, int page );

	virtual idClipModel*		GetCombatModel() const { return combatModel; }

	virtual bool				GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis );
	virtual bool				GetJointWorldTransform( const char* jointName, idVec3 &offset, idMat3 &axis );
	virtual bool				GetJointWorldTransform( jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis );

	void						JawFlap(hhAnimator *theAnimator);

	void						SpawnFxAlongBonePrefixLocal( const idDict* dict, const char* fxKeyPrefix, const char* bonePrefix, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL );
	virtual void				BroadcastFxInfoAlongBonePrefix( const idDict* args, const char* fxKey, const char* bonePrefix, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL, bool broadcast = true ); //HUMANHEAD rww - added broadcast
	virtual void				BroadcastFxInfoAlongBonePrefixUnique( const idDict* args, const char* fxKey, const char* bonePrefix, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL, bool broadcast = true ); //HUMANHEAD rww - added broadcast
	virtual void				BroadcastFxInfoAlongBone( const char* fxName, const char* boneName, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL, bool broadcast = true ); //HUMANHEAD rww - added broadcast
	virtual void				BroadcastFxInfoAlongBone( bool bNoRemoveWhenUnbound, const char* fxName, const char* boneName, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL, bool broadcast = true ); //HUMANHEAD rww - added broadcast

	// WaitForSilence support
	virtual	bool				StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags = 0, bool broadcast = false, int *length = NULL );
	void						Save( idSaveGame *savefile ) const;
	void						Restore( idRestoreGame *savefile );

protected:
	// WaitForSilence support
	int							waitingThread;			// Thread that's blocking until we tell it we're silent
	int							silentTimeOffset;		// +/- this amount
	int							nextSilentTime;			// Time at which all sounds will be ended
	void						Event_Silent();
	void						Event_SetSilenceCallback(float plusOrMinusSeconds);

	float						lastAmplitude;

	bool						hasFlapInfo; //HUMANHEAD rww
	idList<jawFlapInfo_t>		jawFlapList; //HUMANHEAD rww

	//HUMANHEAD: aob - moved from idActor
	virtual void				UpdateWounds( void );
	virtual hhWoundManagerRenderEntity* CreateWoundManager() const { if (!animator.IsAnimatedModel()) { return idEntity::CreateWoundManager(); } return new hhWoundManagerAnimatedEntity(this); }

protected:
	void						Event_CheckAnimatorCycleRotate( void ) { GetAnimator()->CheckCycleRotate(); }
	void						Event_CheckAnimatorThaw( void ) { GetAnimator()->CheckThaw(); }
	void						Event_SpawnFXAlongBone( idList<idStr>* fxParms );
};

#endif
