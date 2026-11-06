// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

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
	JOINTMOD_WORLD_OVERRIDE,	// sets the joint's position or orientation in model space
	JOINTMOD_COLLAPSE
} jointModTransform_t;

typedef struct {
	jointHandle_t			jointnum;
	idMat3					mat;
	idVec3					pos;
	jointModTransform_t		transform_pos;
	jointModTransform_t		transform_axis;

// RAVEN BEGIN
// bdube: added more features to programmer controlled joints
	idInterpolateAccelDecelLinear<idAngles>	angularVelocity;
	int										lastTime;
	
	jointHandle_t			collapseJoint;
// RAVEN END

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
// RAVEN BEGIN
// abahr: event call with parms
	FC_EVENTFUNCTION_ARGS,
// RAVEN END
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
	FC_DIRECTDAMAGE,
	FC_MUZZLEFLASH,
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
	FC_AVIGAME,
	FC_GUIEVENT,
	
	FC_AI_ENABLE_PAIN,
	FC_AI_DISABLE_PAIN,
	FC_AI_ENABLE_DAMAGE,
	FC_AI_DISABLE_DAMAGE,
	FC_AI_LOCKENEMYORIGIN,	
	FC_AI_ATTACK,
	FC_AI_ATTACK_MELEE,
	
	FC_AI_SPEAK,
	FC_AI_SPEAK_RANDOM,
// MCG: for attachment managing
	FC_ACT_ATTACH_HIDE,
	FC_ACT_ATTACH_SHOW,

	FC_ENABLE_BLINKING,
	FC_DISABLE_BLINKING,
	FC_ENABLE_AUTOBLINK,
	FC_DISABLE_AUTOBLINK,

	FC_COUNT

} frameCommandType_t;

// RAVEN BEGIN
// rjohnson: new camera frame commands
extern struct frameCommandInfo_t
{
	const char*		name;
	bool			modview;

} frameCommandInfo[FC_COUNT];
// RAVEN END

typedef struct {
	int						num;
	int						firstCommand;
} frameLookup_t;

typedef struct {
	frameCommandType_t		type;

	idStr*					string;

// RAVEN BEGIN
// bdube: added joint
	idStr*					joint;
	idStr*					joint2;
// abahr:
	idList<idStr>*			parmList;
// RAVEN END

	union {
		const idSoundShader	*soundShader;
		const function_t	*function;
		const idDeclSkin	*skin;
		int					index;
// RAVEN BEGIN
// bdube: effects
		const idDecl		*effect;
		idStr*				projectile;
		idStr*				melee;
// abahr:
		const class idEventDef*	event;
// RAVEN END
	};
} frameCommand_t;

typedef struct {
	bool					prevent_idle_override		: 1;
	bool					random_cycle_start			: 1;
	bool					ai_no_turn					: 1;
	bool					ai_no_look					: 1;
	bool					ai_look_head_only			: 1;	
	bool					anim_turn					: 1;
	bool					sync_cycle					: 1;	
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
	void					GetInterpolatedFrame( const frameBlend_t &frame, idJointQuat *joints, const int *index, int numIndexes ) const;
	void					GetSingleFrame( int framenum, idJointQuat *joints, const int *index, int numIndexes ) const;
	int						Length( void ) const;
	int						NumFrames( void ) const;
	int						NumJoints( void ) const;
	const idVec3			&TotalMovementDelta( void ) const;
	const char				*Name( void ) const;

	void					ConvertTimeToFrame( int time, int cyclecount, frameBlend_t &frame ) const;
// RAVEN BEGIN
// jscott: for modview
	int						ConvertFrameToTime( frameBlend_t &frame ) const;
// RAVEN END

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
private:
	const class idDeclModelDef	*modelDef;
	const idMD5Anim				*anims[ ANIM_MaxSyncedAnims ];
	int							numAnims;
	idStr						name;
	idStr						realname;
	idList<frameLookup_t>		frameLookup;
	idList<frameCommand_t>		frameCommands;
	animFlags_t					flags;

// RAVEN BEGIN
// bdube: added anim speed
	float						rate;
// RAVEN END

public:
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
// RAVEN BEGIN
// bdube: frame command function that takes a list of frames
	const char					*AddFrameCommand( const class idDeclModelDef *modelDef, const idList<int>& frames, idLexer &src, const idDict *def );
// RAVEN END
	void						CallFrameCommands( idEntity *ent, int from, int to ) const;
	bool						HasFrameCommands( void ) const;

								// returns first frame (zero based) that command occurs.  returns -1 if not found.
	int							FindFrameForFrameCommand( frameCommandType_t framecommand, const frameCommand_t **command ) const;
	void						SetAnimFlags( const animFlags_t &animflags );
	const animFlags_t			&GetAnimFlags( void ) const;

// RAVEN BEGIN
// bdube: added
	void						CallFrameCommandSound ( const frameCommand_t& command, idEntity* ent, const s_channelType channel ) const;
	float						GetPlaybackRate ( void ) const;
	void						SetPlaybackRate ( float rate );
// jsinger: to support binary serialization/deserialization of idAnims
#ifdef RV_BINARYDECLS
								idAnim( idDeclModelDef const *def, SerialInputStream &stream );
	void						Write( SerialOutputStream &stream ) const;
#endif
// RAVEN END
};

// RAVEN BEGIN
// bdube: added configurable playback rate
ID_INLINE float idAnim::GetPlaybackRate ( void ) const {
	return rate;
}

ID_INLINE void idAnim::SetPlaybackRate ( float _rate ) {
	rate = _rate;
}

// RAVEN END

/*
==============================================================================================

	idDeclModelDef

==============================================================================================
*/

// RAVEN BEGIN
// jsinger; allow support for serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclModelDef : public idDecl, public Serializable<'DMD '> {
public:
	virtual void				Write( SerialOutputStream &stream ) const;
	virtual void				AddReferences() const;
								idDeclModelDef( SerialInputStream &stream );
private:
	int							mNumChannels;
#else
class idDeclModelDef : public idDecl {
#endif
// RAVEN END
public:
								idDeclModelDef();
								~idDeclModelDef();

	virtual size_t				Size( void ) const;
	virtual const char *		DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength, bool noCaching );
	virtual void				FreeData( void );

// RAVEN BEGIN
// jscott: to prevent a recursive crash
	virtual	bool				RebuildTextSource( void ) { return( false ); }
// scork: for detailed error-reporting
	virtual bool				Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END

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

ID_INLINE const idAnim *idDeclModelDef::GetAnim( int index ) const {
	if ( ( index < 1 ) || ( index > anims.Num() ) ) {
		return NULL;
	}
	return anims[ index - 1 ];
}

/*
==============================================================================================

	idAnimBlend

==============================================================================================
*/

class idAnimBlend {
private:
	const class idDeclModelDef	*modelDef;
	int							starttime;
	int							endtime;
	int							timeOffset;
	float						rate;

	int							blendStartTime;
	int							blendDuration;
	float						blendStartValue;
	float						blendEndValue;

	float						animWeights[ ANIM_MaxSyncedAnims ];
	short						cycle;
	short						animNum;
	bool						allowMove;
	bool						allowFrameCommands;
	bool						useFrameBlend;

	frameBlend_t				frameBlend;

	friend class				idAnimator;

	void						Reset( const idDeclModelDef *_modelDef );
	void						CallFrameCommands( idEntity *ent, int fromtime, int totime ) const;
// RAVEN BEGIN
// twhitaker & jscott: create new SetFrame that allows interpolation between arbitrary frames
	void						SetFrame( const idDeclModelDef *modelDef, int animnum, const frameBlend_t & frameBlend );
// jshepard: added rate parameter so we can speed up/slow down animations.
	void						CycleAnim( const idDeclModelDef *modelDef, int animnum, int currenttime, int blendtime, float rate );
	void						PlayAnim( const idDeclModelDef *modelDef, int animnum, int currenttime, int blendtime, float rate );
// RAVEN END
	bool						BlendAnim( int currentTime, int channel, int numJoints, idJointQuat *blendFrame, float &blendWeight, bool removeOrigin, bool overrideBlend, bool printInfo ) const;
	void						BlendOrigin( int currentTime, idVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const;
	void						BlendDelta( int fromtime, int totime, idVec3 &blendDelta, float &blendWeight ) const;
	void						BlendDeltaRotation( int fromtime, int totime, idQuat &blendDelta, float &blendWeight ) const;
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
	bool						FrameHasChanged( int currentTime ) const;
	int							GetCycleCount( void ) const;
	void						SetCycleCount( int count );
	void						SetPlaybackRate( int currentTime, float newRate );
	float						GetPlaybackRate( void ) const;
	void						SetStartTime( int startTime );
	int							GetStartTime( void ) const;
	int							GetEndTime( void ) const;
	int							GetFrameNumber( int currenttime ) const;
	int							AnimTime( int currenttime ) const;
	int							NumFrames( void ) const;
	int							Length( void ) const;
	int							PlayLength( void ) const;
	void						AllowMovement( bool allow );
	void						AllowFrameCommands( bool allow );
	const idAnim				*Anim( void ) const;
	int							AnimNum( void ) const;
};

ID_INLINE const idAnim *idAnimBlend::Anim( void ) const {
	if ( !modelDef ) {
		return NULL;
	}
	return modelDef->GetAnim( animNum );
}

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

class idAnimator{
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

	void						ServiceAnims( int fromtime, int totime );

// RAVEN BEGIN
// rjohnson: added flag to ignore AF when checking for animation
	bool						IsAnimating	( int currentTime, bool IgnoreAF = false ) const;
	bool						IsBlending	( int channelNum, int currentTime ) const;
// RAVEN END

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
	bool						FrameHasChanged( int animtime ) const;
	void						GetDelta( int fromtime, int totime, idVec3 &delta ) const;
	bool						GetDeltaRotation( int fromtime, int totime, idMat3 &delta ) const;
	void						GetOrigin( int currentTime, idVec3 &pos ) const;
	bool						GetBounds( int currentTime, idBounds &bounds );

	idAnimBlend					*CurrentAnim( int channelNum );
	void						Clear( int channelNum, int currentTime, int cleartime );

// twhitaker & jscott: create new SetFrame that allows interpolation between arbitrary frames
	void						SetFrame( int channelNum, int animnum, const frameBlend_t & frameBlend );
	void						CycleAnim( int channelNum, int animnum, int currenttime, int blendtime );
	void						PlayAnim( int channelNum, int animnum, int currenttime, int blendTime );

								// copies the current anim from fromChannelNum to channelNum.
								// the copied anim will have frame commands disabled to avoid executing them twice.
	void						SyncAnimChannels( int channelNum, int fromChannelNum, int currenttime, int blendTime );

	void						SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos );
	void						SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const idMat3 &mat );
	void						GetJointAxis( jointHandle_t jointnum, idMat3 &mat );
	void						CollapseJoint ( jointHandle_t jointnum, jointHandle_t collapseTo );
	void						CollapseJoints ( const char* jointnames, jointHandle_t collapseJoint );
	void						ClearJoint( jointHandle_t jointnum );
	void						ClearAllJoints( void );

// RAVEN BEGIN
// bdube: more joint control functions
	void						AimJointAt ( jointHandle_t jointnum, const idVec3& pos, const int blendtime );
	void						SetJointAngularVelocity ( jointHandle_t jointnum, const idAngles& vel, const int currentTime, const int blendTime );
	idAngles					GetJointAngularVelocity ( jointHandle_t jointnum, const int currentTime );
	void						ClearJointAngularVelocity ( jointHandle_t jointnum );

// jshepard: rate of playback change
	void						SetPlaybackRate(float multiplier);
// abahr:
	void						SetPlaybackRate( const char* animName, float rate );
	void						SetPlaybackRate( int animHandle, float rate );
// RAVEN END

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
// RAVEN BEGIN
// rjohnson: more output for animators
	const char					*AnimMD5Name( int animnum, int index ) const;
// RAVEN END
	int							AnimLength( int animnum ) const;
	const idVec3				&TotalMovementDelta( int animnum ) const;
	
// RAVEN BEGIN
// nrausch: get the nearest joint to a segment - ignores joints behind the origin
// you can pass it a null jointList in order to test against all joints ( use NumJoints() for the count )
	jointHandle_t				GetNearestJoint( const idVec3 &start, const idVec3 &end, int time, jointHandle_t *jointList, int cnt ); 
//MCG
	jointMod_t *				FindExistingJointMod( jointHandle_t jointnum, int *index );
// RAVEN END

private:
// RAVEN BEGIN
// bdube: added methods
	jointMod_t *				FindJointMod ( jointHandle_t jointnum );
// RAVEN END

	void						FreeData( void );
	void						PushAnims( int channel, int currentTime, int blendTime );

private:
	const idDeclModelDef *		modelDef;
	idEntity *					entity;

	idAnimBlend					channels[ ANIM_NumAnimChannels ][ ANIM_MaxAnimsPerChannel ];
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
	idJointQuat *				AFPoseJointFrame;
	int							AFPoseJointFrameSize;
	idBounds					AFPoseBounds;
	int							AFPoseTime;

// RAVEN BEGIN
// jshepard: multiplier for the animation rate for all anims under this animator
	float						rateMultiplier;
// RAVEN END
};

ID_INLINE void idAnimator::SetPlaybackRate ( float _rate ) {
	rateMultiplier = _rate;
}

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

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	void						BeginLevelLoad( void );
	void						EndLevelLoad( void );
#endif
// RAVEN END
	void						Shutdown( void );
	idMD5Anim *					GetAnim( const char *name );
	void						ReloadAnims( void );
	void						ListAnims( void ) const;
	void						PrintMemInfo( MemInfo *mi );
	int							JointIndex( const char *name );
	const char *				JointName( int index ) const;

	void						ClearAnimsInUse( void );
	void						FlushUnusedAnims( void );

private:
	idHashTable<idMD5Anim *>	animations;
	idStrList					jointnames;
	idHashIndex					jointnamesHash;
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	bool						insideLevelLoad;
#endif
// RAVEN END
};

#endif /* !__ANIM_H__ */

// RAVEN END
