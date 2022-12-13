// Copyright (C) 2004 Id Software, Inc.
//
#ifndef __ANIM_H__
#define __ANIM_H__

//
// animation channels
// these can be changed by modmakers and licensees to be whatever they need.
const int ANIM_NumAnimChannels		= 5;
const int ANIM_MaxAnimsPerChannel	= 3;
const int ANIM_MaxSyncedAnims		= 3;

//
// animation channels.  make sure to change script/doom_defs.script if you add any channels, or change their order
//
const int ANIMCHANNEL_ALL			= 0;
const int ANIMCHANNEL_TORSO			= 1;
const int ANIMCHANNEL_LEGS			= 2;
const int ANIMCHANNEL_HEAD			= 3;
const int ANIMCHANNEL_EYELIDS		= 4;

// for converting from 24 frames per second to milliseconds
ID_INLINE int FRAME2MS( int framenum ) {
	return ( framenum * 1000 ) / 24;
}

class idRenderModel;
class idAnimator;
class idAnimBlend;
class function_t;
class idEntity;
class idSaveGame;
class idRestoreGame;
class idEventDef; // HUMANHEAD JRM
class hhAnimator; // HUMANHEAD nla

typedef struct {
	int		cycleCount;	// how many times the anim has wrapped to the begining (0 for clamped anims)
	int		frame1;
	int		frame2;
	float	frontlerp;
	float	backlerp;
} frameBlend_t;

typedef struct {
	int						nameIndex;
	int						parentNum;
	int						animBits;
	int						firstComponent;
} jointAnimInfo_t;

typedef struct {
	jointHandle_t			num;
	jointHandle_t			parentNum;
	int						channel;
} jointInfo_t;

//
// joint modifier modes.  make sure to change script/doom_defs.script if you add any, or change their order.
//
typedef enum {
	JOINTMOD_NONE,				// no modification
	JOINTMOD_LOCAL,				// modifies the joint's position or orientation in joint local space
	JOINTMOD_LOCAL_OVERRIDE,	// sets the joint's position or orientation in joint local space
	JOINTMOD_WORLD,				// modifies joint's position or orientation in model space
	JOINTMOD_WORLD_OVERRIDE		// sets the joint's position or orientation in model space
} jointModTransform_t;

typedef struct {
	jointHandle_t			jointnum;
	idMat3					mat;
	idVec3					pos;
	jointModTransform_t		transform_pos;
	jointModTransform_t		transform_axis;
} jointMod_t;

#define	ANIM_TX				BIT( 0 )
#define	ANIM_TY				BIT( 1 )
#define	ANIM_TZ				BIT( 2 )
#define	ANIM_QX				BIT( 3 )
#define	ANIM_QY				BIT( 4 )
#define	ANIM_QZ				BIT( 5 )

typedef enum {
	FC_SCRIPTFUNCTION,
	FC_SCRIPTFUNCTIONOBJECT,
	FC_EVENTFUNCTION,
	FC_SOUND,
	FC_SOUND_VOICE,
	FC_SOUND_VOICE2,
	FC_SOUND_BODY,
	FC_SOUND_BODY2,
	FC_SOUND_BODY3,
	FC_SOUND_WEAPON,
	FC_SOUND_ITEM,
	FC_SOUND_GLOBAL,
	FC_SOUND_CHATTER,
	FC_SKIN,
	FC_TRIGGER,
	FC_TRIGGER_SMOKE_PARTICLE,
	//HUMANHEAD
	FC_STOPSND,
	FC_STOPSND_VOICE,	
	FC_STOPSND_VOICE2,
	FC_STOPSND_BODY,
	FC_STOPSND_BODY2,
	FC_STOPSND_BODY3,
	FC_STOPSND_WEAPON,
	FC_STOPSND_ITEM,
	FC_EVENT_ARGS,				// nla
	FC_MOOD,					// JRM
	FC_LAUNCHALTMISSILE,		// aob		
	FC_LAUNCHMISSILE_BONEDIR,
	FC_RIGHTFOOTPRINT,
	FC_LEFTFOOTPRINT,
	FC_KICK_OBSTACLE,
	FC_TRIGGER_ANIM_ENT,
	FC_HIDE,
	FC_SETKEY,					// mdc
	//HUMANHEAD END
	FC_MELEE,
	FC_DIRECTDAMAGE,
	FC_BEGINATTACK,
	FC_ENDATTACK,
	FC_MUZZLEFLASH,
	FC_CREATEMISSILE,
	FC_LAUNCHMISSILE,
	FC_FIREMISSILEATTARGET,
	FC_FOOTSTEP,
	FC_LEFTFOOT,
	FC_RIGHTFOOT,
	FC_ENABLE_EYE_FOCUS,
	FC_DISABLE_EYE_FOCUS,
	FC_FX,
	FC_DISABLE_GRAVITY,
	FC_ENABLE_GRAVITY,
	FC_JUMP,
	FC_ENABLE_CLIP,
	FC_DISABLE_CLIP,
	FC_ENABLE_WALK_IK,
	FC_DISABLE_WALK_IK,
	FC_ENABLE_LEG_IK,
	FC_DISABLE_LEG_IK,
	FC_RECORDDEMO,
	FC_AVIGAME
} frameCommandType_t;

typedef struct {
	int						num;
	int						firstCommand;
} frameLookup_t;

typedef struct {
	frameCommandType_t		type;
	idStr					*string;

	//HUMANHEAD: aob - cache for event_arg parms
	idList<idStr>			*parmList;
	//HUMANHEAD END

	union {
		const idSoundShader	*soundShader;
		const function_t	*function;
		const idDeclSkin	*skin;
		int					index;
	};
} frameCommand_t;

typedef struct {
	bool					prevent_idle_override		: 1;
	bool					random_cycle_start			: 1;
	bool					ai_no_turn					: 1;
	bool					anim_turn					: 1;
} animFlags_t;


/*
==============================================================================================

	idModelExport

==============================================================================================
*/

class idModelExport {
private:
	void					Reset( void );
	bool					ParseOptions( idLexer &lex );
	int						ParseExportSection( idParser &parser );

	static bool				CheckMayaInstall( void );
	static void				LoadMayaDll( void );

	bool					ConvertMayaToMD5( void );
	static bool				initialized;

public:
	idStr					commandLine;
	idStr					src;
	idStr					dest;
	bool					force;

							idModelExport();

	static void				Shutdown( void );

	int						ExportDefFile( const char *filename );
	bool					ExportModel( const char *model );
	bool					ExportAnim( const char *anim );
	int						ExportModels( const char *pathname, const char *extension );
};

/*
==============================================================================================

	idMD5Anim

==============================================================================================
*/

class idMD5Anim {
private:
// HUMANHEAD nla - Needed for access in hhBaseAnim
protected:
// HUMANHEAD END
	int						numFrames;
	int						frameRate;
	int						animLength;
	int						numJoints;
	int						numAnimatedComponents;
	idList<idBounds>		bounds;
	idList<jointAnimInfo_t>	jointInfo;
	idList<idJointQuat>		baseFrame;
	idList<float>			componentFrames;
	idStr					name;
	idVec3					totaldelta;
	mutable int				ref_count;

public:
							idMD5Anim();
							~idMD5Anim();

	void					Free( void );
	bool					Reload( void );
	size_t					Allocated( void ) const;
	size_t					Size( void ) const { return sizeof( *this ) + Allocated(); };
	bool					LoadAnim( const char *filename );

	void					IncreaseRefs( void ) const;
	void					DecreaseRefs( void ) const;
	int						NumRefs( void ) const;
	
	void					CheckModelHierarchy( const idRenderModel *model ) const;
	void					GetInterpolatedFrame( frameBlend_t &frame, idJointQuat *joints, const int *index, int numIndexes ) const;
	void					GetSingleFrame( int framenum, idJointQuat *joints, const int *index, int numIndexes ) const;
	virtual // HUMANHEAD nla - virtual for hhBaseAnim
	int						Length( void ) const;
	int						NumFrames( void ) const;
	int						NumJoints( void ) const;
	const idVec3			&TotalMovementDelta( void ) const;
	const char				*Name( void ) const;

	void					GetFrameBlend( int framenum, frameBlend_t &frame ) const;	// frame 1 is first frame
	virtual // HUMANHEAD nla - virtual to be overriden
	void					ConvertTimeToFrame( int time, int cyclecount, frameBlend_t &frame ) const;

	// HUMANHEAD nla - Added to allow partial anims.  Overridden in hhMD5Anim
	virtual void			SetLimits( float start, float end ) const {};
	// HUMANHEAD END

	void					GetOrigin( idVec3 &offset, int currentTime, int cyclecount ) const;
	void					GetOriginRotation( idQuat &rotation, int time, int cyclecount ) const;
	void					GetBounds( idBounds &bounds, int currentTime, int cyclecount ) const;
};

/*
==============================================================================================

	idAnim

==============================================================================================
*/

class idAnim {
protected:		// HUMANHEAD nla - needed to override class
	const class idDeclModelDef	*modelDef;
	const idMD5Anim				*anims[ ANIM_MaxSyncedAnims ];
	int							numAnims;
	idStr						name;
	idStr						realname;
	idList<frameLookup_t>		frameLookup;
	idList<frameCommand_t>		frameCommands;
	animFlags_t					flags;

public:
	// HUMANHEAD nla 
	// To allowed extra frame commands is derived classes.  Returns true if other commands found
	virtual bool			AddFrameCommandExtra( idToken &token, frameCommand_t &fc, idLexer &src, idStr &errorText ) { return( false ); };
	virtual bool			CallFrameCommandsExtra( const frameCommand_t &command, idEntity *ent ) const { return( false ); };	
	// HUMANHEAD END

								idAnim();
								idAnim( const idDeclModelDef *modelDef, const idAnim *anim );
								~idAnim();

	void						SetAnim( const idDeclModelDef *modelDef, const char *sourcename, const char *animname, int num, const idMD5Anim *md5anims[ ANIM_MaxSyncedAnims ] );
	const char					*Name( void ) const;
	const char					*FullName( void ) const;
	const idMD5Anim				*MD5Anim( int num ) const;
	const idDeclModelDef		*ModelDef( void ) const;
	int							Length( void ) const;
	int							NumFrames( void ) const;
	int							NumAnims( void ) const;
	const idVec3				&TotalMovementDelta( void ) const;
	bool						GetOrigin( idVec3 &offset, int animNum, int time, int cyclecount ) const;
	bool						GetOriginRotation( idQuat &rotation, int animNum, int currentTime, int cyclecount ) const;
	bool						GetBounds( idBounds &bounds, int animNum, int time, int cyclecount ) const;
	const char					*AddFrameCommand( const class idDeclModelDef *modelDef, int framenum, idLexer &src, const idDict *def );
	void						CallFrameCommands( idEntity *ent, int from, int to ) const;
	bool						HasFrameCommands( void ) const;

								// returns first frame (zero based) that command occurs.  returns -1 if not found.
	int							FindFrameForFrameCommand( frameCommandType_t framecommand, const frameCommand_t **command ) const;
	void						SetAnimFlags( const animFlags_t &animflags );
	const animFlags_t			&GetAnimFlags( void ) const;
};

/*
==============================================================================================

	idDeclModelDef

==============================================================================================
*/

class idDeclModelDef : public idDecl {
public:
								idDeclModelDef();
								~idDeclModelDef();

	virtual size_t				Size( void ) const;
	virtual const char *		DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );
	virtual void				FreeData( void );

	void						Touch( void ) const;

	const idDeclSkin *			GetDefaultSkin( void ) const;
	const idJointQuat *			GetDefaultPose( void ) const;
	void						SetupJoints( int *numJoints, idJointMat **jointList, idBounds &frameBounds, bool removeOriginOffset ) const;
	idRenderModel *				ModelHandle( void ) const;
	void						GetJointList( const char *jointnames, idList<jointHandle_t> &jointList ) const;
	const jointInfo_t *			FindJoint( const char *name ) const;

	int							NumAnims( void ) const;
	const idAnim *				GetAnim( int index ) const;
	int							GetSpecificAnim( const char *name ) const;
	int							GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;
	const idDeclSkin *			GetSkin( void ) const;
	const char *				GetModelName( void ) const;
	const idList<jointInfo_t> &	Joints( void ) const;
	const int *					JointParents( void ) const;
	int							NumJoints( void ) const;
	const jointInfo_t *			GetJoint( int jointHandle ) const;
	const char *				GetJointName( int jointHandle ) const;
	int							NumJointsOnChannel( int channel ) const;
	const int *					GetChannelJoints( int channel ) const;

	const idVec3 &				GetVisualOffset( void ) const;

	// HUMANHEAD nla
	idDict						channelDict;
	// HUMANHEAD END

private:
	void						CopyDecl( const idDeclModelDef *decl );
	bool						ParseAnim( idLexer &src, int numDefaultAnims );

private:
	idVec3						offset;
	idList<jointInfo_t>			joints;
	idList<int>					jointParents;
	idList<int>					channelJoints[ ANIM_NumAnimChannels ];
	idRenderModel *				modelHandle;
	idList<idAnim *>			anims;
	const idDeclSkin *			skin;
};

/*
==============================================================================================

	idAnimBlend

==============================================================================================
*/

class idAnimBlend {
protected:		// HUMANHEAD nla - Used to access vars in hhAnimBlend
	const class idDeclModelDef	*modelDef;
	mutable		// HUMANHEAD nla - Hack to get around const for calls to UpdateFreezeTime
	int							starttime;
	mutable		// HUMANHEAD nla - Hack to get around const for calls to UpdateFreezeTime
	int							endtime;
	int							timeOffset;
	float						rate;

	int							blendStartTime;
	int							blendDuration;
	float						blendStartValue;
	float						blendEndValue;

	float						animWeights[ ANIM_MaxSyncedAnims ];
	short						cycle;
	short						frame;
	short						animNum;
	bool						allowMove;
	bool						allowFrameCommands;
	// HUMANHEAD nla - All data members need to be in idAnimBlend, due to the pointer math done.  (See idAnimator::GetBounds)
	bool				frozen;		
	int					freezeStart;
	mutable		// HUMANHEAD nla - Hack to get around const for calls to UpdateFreezeTime
	int					freezeCurrent;
	int					freezeEnd;
	int					rotateTime;
	const idEventDef *	rotateEvent;

	friend class				hhAnimator;
	// HUMANHEAD END

	friend class				idAnimator;

	void						Reset( const idDeclModelDef *_modelDef );
	virtual		// HUMANHEAD nla
	void						CallFrameCommands( idEntity *ent, int fromtime, int totime ) const;
	void						SetFrame( const idDeclModelDef *modelDef, int animnum, int frame, int currenttime, int blendtime );
	void						CycleAnim( const idDeclModelDef *modelDef, int animnum, int currenttime, int blendtime );
	void						PlayAnim( const idDeclModelDef *modelDef, int animnum, int currenttime, int blendtime );
	virtual		// HUMANHEAD nla
	bool						BlendAnim( int currentTime, int channel, int numJoints, idJointQuat *blendFrame, float &blendWeight, bool removeOrigin, bool overrideBlend, bool printInfo ) const;
	virtual		// HUMANHEAD nla
	void						BlendOrigin( int currentTime, idVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const;
	virtual		// HUMANHEAD nla
	void						BlendDelta( int fromtime, int totime, idVec3 &blendDelta, float &blendWeight ) const;
	void						BlendDeltaRotation( int fromtime, int totime, idQuat &blendDelta, float &blendWeight ) const;
	virtual		// HUMANHEAD nla
	bool						AddBounds( int currentTime, idBounds &bounds, bool removeOriginOffset ) const;

public:
								idAnimBlend();
	void						Save( idSaveGame *savefile ) const;
	void						Restore( idRestoreGame *savefile, const idDeclModelDef *modelDef );
	const char					*AnimName( void ) const;
	const char					*AnimFullName( void ) const;
	float						GetWeight( int currenttime ) const;
	float						GetFinalWeight( void ) const;
	void						SetWeight( float newweight, int currenttime, int blendtime );
	int							NumSyncedAnims( void ) const;
	bool						SetSyncedAnimWeight( int num, float weight );
	void						Clear( int currentTime, int clearTime );
	bool						IsDone( int currentTime ) const;
	virtual		// HUMANHEAD nla
	bool						FrameHasChanged( int currentTime ) const;
	int							GetCycleCount( void ) const;
	void						SetCycleCount( int count );
	void						SetPlaybackRate( int currentTime, float newRate );
	float						GetPlaybackRate( void ) const;
	void						SetStartTime( int startTime );
	int							GetStartTime( void ) const;
	int							GetEndTime( void ) const;
	virtual		// HUMANHEAD nla
	int							GetFrameNumber( int currenttime ) const;
	virtual		// HUMANHEAD nla
	int							AnimTime( int currenttime ) const;
	int							NumFrames( void ) const;
	int							Length( void ) const;
	int							PlayLength( void ) const;
	void						AllowMovement( bool allow );
	void						AllowFrameCommands( bool allow );
	const idAnim				*Anim( void ) const;
	int							AnimNum( void ) const;
};

// HUMANHEAD nla - Stupid C++ forces me to put this here!  Else I can't use it below in idAnimator due to it not being defined
class hhAnimBlend : public idAnimBlend {
 public:
						hhAnimBlend();

	friend class		hhAnimator;
	friend class		idAnimator;

	// Overridden Methods					
	bool				FrameHasChanged( int currentTime ) const;
	int					AnimTime( int currenttime ) const;
	int					GetFrameNumber( int currenttime ) const;


 protected:	

	// Overridden Methods
	void				CallFrameCommands( idEntity *ent, int fromtime, int totime ) const;
	bool				BlendAnim( int currentTime, int channel, int numJoints, idJointQuat *blendFrame, float &blendWeight, bool removeOrigin, bool overrideBlend, bool printInfo ) const;
	void				BlendOrigin( int currentTime, idVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const;
	void				BlendDelta( int fromtime, int totime, idVec3 &blendDelta, float &blendWeight ) const;
	bool				AddBounds( int currentTime, idBounds &bounds, bool removeOriginOffset ) const;
						
	// Original Methods
	void				UpdateFreezeTime( int currentTime ) const;
	bool				Freeze( int currentTime, idEntity *owner );
	bool				Freeze( int currentTime, idEntity *owner, int durationMS );
	bool				Thaw( int currentTime );
	bool				ThawIfTime( int currentTime );
	bool				IsFrozen() const { return( frozen ); };

};

// HUMANHEAD END

/*
==============================================================================================

	idAFPoseJointMod

==============================================================================================
*/

typedef enum {
	AF_JOINTMOD_AXIS,
	AF_JOINTMOD_ORIGIN,
	AF_JOINTMOD_BOTH
} AFJointModType_t;

class idAFPoseJointMod {
public:
								idAFPoseJointMod( void );

	AFJointModType_t			mod;
	idMat3						axis;
	idVec3						origin;
};

ID_INLINE idAFPoseJointMod::idAFPoseJointMod( void ) {
	mod = AF_JOINTMOD_AXIS;
	axis.Identity();
	origin.Zero();
}

/*
==============================================================================================

	idAnimator

==============================================================================================
*/

class idAnimator {
public:
								idAnimator();
								~idAnimator();

	size_t						Allocated( void ) const;
	size_t						Size( void ) const;

	void						Save( idSaveGame *savefile ) const;					// archives object for save game file
	void						Restore( idRestoreGame *savefile );					// unarchives object from save game file

	void						SetEntity( idEntity *ent );
	idEntity					*GetEntity( void ) const ;
	void						RemoveOriginOffset( bool remove );
	bool						RemoveOrigin( void ) const;

	void						GetJointList( const char *jointnames, idList<jointHandle_t> &jointList ) const;

	int							NumAnims( void ) const;
	const idAnim				*GetAnim( int index ) const;
	int							GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;

	// HUMANHEAD nla - Convenience
	bool						GetJointTransform( const char* name, int currenttime, idVec3 &offset, idMat3 &axis ) { return GetJointTransform( GetJointHandle(name), currenttime, offset, axis ); }
	bool						GetJointLocalTransform( const char* name, int currenttime, idVec3 &offset, idMat3 &axis ) { return GetJointLocalTransform( GetJointHandle(name), currenttime, offset, axis ); }
	// HUMANHEAD END

	void						ServiceAnims( int fromtime, int totime );
	virtual		// HUMANHEAD nla
	bool						IsAnimating( int currentTime ) const;

	void						GetJoints( int *numJoints, idJointMat **jointsPtr );
	int							NumJoints( void ) const;
	jointHandle_t				GetFirstChild( jointHandle_t jointnum ) const;
	jointHandle_t				GetFirstChild( const char *name ) const;

	idRenderModel				*SetModel( const char *modelname );
	idRenderModel				*ModelHandle( void ) const;
	const idDeclModelDef		*ModelDef( void ) const;

	void						ForceUpdate( void );
	void						ClearForceUpdate( void );
	bool						CreateFrame( int animtime, bool force );
	virtual		// HUMANHEAD nla
	bool						FrameHasChanged( int animtime ) const;
	void						GetDelta( int fromtime, int totime, idVec3 &delta ) const;
	bool						GetDeltaRotation( int fromtime, int totime, idMat3 &delta ) const;
	void						GetOrigin( int currentTime, idVec3 &pos ) const;
	bool						GetBounds( int currentTime, idBounds &bounds );

	idAnimBlend					*CurrentAnim( int channelNum );
	void						Clear( int channelNum, int currentTime, int cleartime );
	void						SetFrame( int channelNum, int animnum, int frame, int currenttime, int blendtime );
	void						CycleAnim( int channelNum, int animnum, int currenttime, int blendtime );
	void						PlayAnim( int channelNum, int animnum, int currenttime, int blendTime );

								// copies the current anim from fromChannelNum to channelNum.
								// the copied anim will have frame commands disabled to avoid executing them twice.
	void						SyncAnimChannels( int channelNum, int fromChannelNum, int currenttime, int blendTime );

	void						SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos );
	void						SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const idMat3 &mat );
	void						ClearJoint( jointHandle_t jointnum );
	void						ClearAllJoints( void );

	void						InitAFPose( void );
	void						SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const idMat3 &axis, const idVec3 &origin );
	void						FinishAFPose( int animnum, const idBounds &bounds, const int time );
	void						SetAFPoseBlendWeight( float blendWeight );
	bool						BlendAFPose( idJointQuat *blendFrame ) const;
	void						ClearAFPose( void );

	void						ClearAllAnims( int currentTime, int cleartime );

	jointHandle_t				GetJointHandle( const char *name ) const;
	const char *				GetJointName( jointHandle_t handle ) const;
	int							GetChannelForJoint( jointHandle_t joint ) const;
	bool						GetJointTransform( jointHandle_t jointHandle, int currenttime, idVec3 &offset, idMat3 &axis );
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis );

	const animFlags_t			GetAnimFlags( int animnum ) const;
	int							NumFrames( int animnum ) const;
	int							NumSyncedAnims( int animnum ) const;
	const char					*AnimName( int animnum ) const;
	const char					*AnimFullName( int animnum ) const;
	int							AnimLength( int animnum ) const;
	const idVec3				&TotalMovementDelta( int animnum ) const;

private:
	void						FreeData( void );
	void						PushAnims( int channel, int currentTime, int blendTime );

protected:		// HUMANEAD nla - For access in hhAnimator
	const idDeclModelDef *		modelDef;
	idEntity *					entity;

#ifdef HUMANHEAD
	hhAnimBlend					channels[ ANIM_NumAnimChannels ][ ANIM_MaxAnimsPerChannel ];
#else
	idAnimBlend					channels[ ANIM_NumAnimChannels ][ ANIM_MaxAnimsPerChannel ];
#endif
	idList<jointMod_t *>		jointMods;
	int							numJoints;
	idJointMat *				joints;

	mutable int					lastTransformTime;		// mutable because the value is updated in CreateFrame
	mutable bool				stoppedAnimatingUpdate;
	bool						removeOriginOffset;
	bool						forceUpdate;

	idBounds					frameBounds;

	float						AFPoseBlendWeight;
	idList<int>					AFPoseJoints;
	idList<idAFPoseJointMod>	AFPoseJointMods;
	idList<idJointQuat>			AFPoseJointFrame;
	idBounds					AFPoseBounds;
	int							AFPoseTime;
};

/*
==============================================================================================

	idAnimManager

==============================================================================================
*/

class idAnimManager {
public:
								idAnimManager();
								~idAnimManager();

	static bool					forceExport;

	void						Shutdown( void );
	idMD5Anim *					GetAnim( const char *name );
	void						ReloadAnims( void );
	void						ListAnims( void ) const;
	int							JointIndex( const char *name );
	const char *				JointName( int index ) const;

	void						ClearAnimsInUse( void );
	void						FlushUnusedAnims( void );
	void						PrintMemInfo( MemInfo_t *mi );	// HUMANHEAD pdm

private:
	idHashTable<idMD5Anim *>	animations;
	idStrList					jointnames;
	idHashIndex					jointnamesHash;
};

#endif /* !__ANIM_H__ */
