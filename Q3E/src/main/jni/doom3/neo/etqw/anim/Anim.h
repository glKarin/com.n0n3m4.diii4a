// Copyright (C) 2007 Id Software, Inc.
//
#ifndef __ANIM_H__
#define __ANIM_H__

#include "../Common.h"

//
// animation channels
// these can be changed by modmakers and licensees to be whatever they need.
const int ANIM_MaxAnimsPerChannel	= 4;
const int ANIM_MaxSyncedAnims		= 3;

//
// animation channels.  make sure to change script/doom_defs.script if you add any channels, or change their order
//
enum animChannel_t {
	ANIMCHANNEL_ALL			= 0,
	ANIMCHANNEL_TORSO,
	ANIMCHANNEL_LEGS,
	ANIMCHANNEL_HEAD,
	ANIM_NumAnimChannels,
};

// for converting from 24 frames per second to milliseconds
template< typename T >
ID_INLINE T FRAME2MS( T framenum ) {
	return ( framenum * ( T )1000 ) / ( T )24;
}

class idRenderModel;
class idAnimator;
class idAnimBlend;
class idEntity;
class idClass;
class sdAnimFrameCommand;

struct frameBlend_t {
	int		cycleCount;	// how many times the anim has wrapped to the beginning (0 for clamped anims)
	int		frame1;
	int		frame2;
	float	frontlerp;
	float	backlerp;
};

struct jointAnimInfo_t {
	short					nameIndex;
	short					parentNum;
	short					animBits;
	short					firstComponent;
};

struct jointInfo_t {
	jointHandle_t			num;
	jointHandle_t			parentNum;
	animChannel_t			channel;
};

//
// joint modifier modes.  make sure to change script/doom_defs.script if you add any, or change their order.
//
enum jointModTransform_t {
	JOINTMOD_NONE,				// no modification
	JOINTMOD_LOCAL,				// modifies the joint's position or orientation in joint local space
	JOINTMOD_LOCAL_OVERRIDE,	// sets the joint's position or orientation in joint local space
	JOINTMOD_WORLD,				// modifies joint's position or orientation in model space
	JOINTMOD_WORLD_OVERRIDE		// sets the joint's position or orientation in model space
};

struct jointMod_t {
	jointHandle_t			jointnum;
	idMat3					mat;
	idVec3					pos;
	jointModTransform_t		transform_pos;
	jointModTransform_t		transform_axis;
};

#define	ANIM_TX				BIT( 0 )
#define	ANIM_TY				BIT( 1 )
#define	ANIM_TZ				BIT( 2 )
#define	ANIM_QX				BIT( 3 )
#define	ANIM_QY				BIT( 4 )
#define	ANIM_QZ				BIT( 5 )

struct frameLookup_t {
	int						num;
	int						firstCommand;
};

struct animFlags_t {
	bool					prevent_idle_override		: 1;
	bool					random_cycle_start			: 1;
	bool					ai_no_turn					: 1;
	bool					ai_fixed_forward			: 1;
	bool					anim_turn					: 1;
	bool					no_pitch					: 1;
};

/*
==============================================================================================

	idModelExport

==============================================================================================
*/

#if defined( ID_ALLOW_TOOLS )

class idModelExport {
public:
	idStr					commandLine;
	idStr					src;
	idStr					dest;
	bool					force;

							idModelExport();

	static void				Shutdown( void );

	int						ExportDefFile( const char* filename );
	bool					ExportModel( const char* model );
	bool					ExportAnim( const char* anim );
	int						ExportModels( const char* pathname, const char* extension );

private:
	void					Reset( void );
	bool					ParseOptions( idLexer& lex );
	int						ParseExportSection( idParser& parser );

	static bool				CheckMayaInstall( void );
	static void				LoadMayaDll( void );

	bool					ConvertMayaToMD5( void );
	static bool				initialized;
};

#endif /* ID_ALLOW_TOOLS */

/*
==============================================================================================

	idMD5Anim

==============================================================================================
*/

static const int ANIMB_IDENT    = (('A'<<24)+('N'<<16)+('M'<<8)+'B');
static const int ANIMB_VERSION  = 1;


class idMD5Anim {
public:
									idMD5Anim();
									~idMD5Anim();

	void							Free( void );
	bool							Reload( void );
	size_t							Allocated( void ) const;
	size_t							Size( void ) const { return sizeof(* this ) + Allocated(); };
	bool							LoadAnim( const char* filename );

	bool							WriteAnimBinary( const char *filename );
	bool							LoadAnimBinary( const char *filename );

	void							IncreaseRefs( void ) const;
	void							DecreaseRefs( void ) const;
	int								NumRefs( void ) const;
	
	void							CheckModelHierarchy( const idRenderModel* model ) const;
	void							GetInterpolatedFrame( frameBlend_t& frame, idJointQuat* joints, const int* index, int numIndexes ) const;
	void							GetSingleFrame( int framenum, idJointQuat* joints, const int* index, int numIndexes ) const;
	int								Length( void ) const;
	int								NumFrames( void ) const;
	int								NumJoints( void ) const;
	const idVec3 &					TotalMovementDelta( void ) const;
	const char *					Name( void ) const;

	void							GetFrameBlend( int framenum, frameBlend_t& frame ) const;	// frame 1 is first frame
	void							ConvertTimeToFrame( int time, int cyclecount, frameBlend_t& frame ) const;

	void							GetOrigin( idVec3& offset, int currentTime, int cyclecount ) const;
	void							GetOriginRotation( idQuat& rotation, int time, int cyclecount ) const;
	void							GetBounds( idBounds& bounds, int currentTime, int cyclecount ) const;
	int								GetFrameRate() const { return frameRate; }
	void							Resample( void );
	
	bool							IsReduced( void ) const { return reduced; }
private:
	int								numFrames;
	int								frameRate;
	int								animLength;
	int								numJoints;
	int								numAnimatedComponents;
	idList<idBoundsShort>			bounds;
	idList<jointAnimInfo_t>			jointInfo;
	idList<idCompressedJointQuat>	baseFrame;
	idList<short>					componentFrames;
	idStr							name;
	idVec3							totaldelta;
	bool							reduced;
	mutable int						ref_count;
};

/*
==============================================================================================

	idAnim

==============================================================================================
*/

class idDeclModelDef;
class idAnim;

struct animListEntry_t {
	idStr	name;
	idAnim* anim;
	idStr	alias;
};

typedef idList< animListEntry_t > animList_t;

class idAnim {
public:
	enum absoluteFrameCommandType_t {
		FC_START = -1000,
		FC_FINISH = -1001,
		FC_BEGIN = -1002,
		FC_END = -1003
	};
									idAnim( void );
									~idAnim( void );

//									idAnim( const idDeclModelDef* modelDef, const idAnim* anim );

	void							SetAnim( const idDeclModelDef* modelDef, const char* sourcename, const char* animname, int num, const idMD5Anim* md5anims[ ANIM_MaxSyncedAnims ] );
	const char*						Name( void ) const;
	const char*						FullName( void ) const;
	const idMD5Anim*				MD5Anim( int num ) const;
	int								Length( void ) const;
	int								NumFrames( void ) const;
	int								NumAnims( void ) const;
	const idVec3&					TotalMovementDelta( void ) const;
	bool							GetOrigin( idVec3& offset, int animNum, int time, int cyclecount ) const;
	bool							GetOriginRotation( idQuat& rotation, int animNum, int currentTime, int cyclecount ) const;
	bool							GetBounds( idBounds& bounds, int animNum, int time, int cyclecount ) const;
	bool							AddFrameCommand( int framenum, sdAnimFrameCommand* fc );
	bool							AddAbsoluteFrameCommand( absoluteFrameCommandType_t framenum, sdAnimFrameCommand* fc );
	void							CallFrameCommands( idClass* ent, int from, int to ) const;
	void							CallAbsoluteFrameCommands( idClass* ent, absoluteFrameCommandType_t frame ) const;
	bool							HasFrameCommands( void ) const;
	void							ClearFrameCommands( void );

	void							IncRef( void ) { refCount++; }
	void							DecRef( void ) { refCount--; if ( refCount == 0 ) { delete this; } }

	bool							IsBaseModelDef( const idDeclModelDef* modelDef ) const { return modelDef == baseModelDef; }

	static sdAnimFrameCommand*		ReadFrameCommand( idParser &src );

	void							SetAnimFlags( const animFlags_t& animflags );
	const animFlags_t&				GetAnimFlags( void ) const;

private:	
	const class idDeclModelDef*		baseModelDef;

	const idMD5Anim*				anims[ ANIM_MaxSyncedAnims ];
	int								numAnims;
	idStr							name;
	idStr							realname;
	idList< frameLookup_t >			frameLookup;
	idList< sdAnimFrameCommand* >	frameCommands;
	idList< sdAnimFrameCommand* >	startCommands;
	idList< sdAnimFrameCommand* >	finishCommands;
	idList< sdAnimFrameCommand* >	beginCommands;
	idList< sdAnimFrameCommand* >	endCommands;
	animFlags_t						flags;
	int								refCount;
};

/*
==============================================================================================

	idDeclModelDef

==============================================================================================
*/

// Gordon: FIXME: move to gamedecllib
class idDeclModelDef : public idDecl {
public:
								idDeclModelDef();
	virtual						~idDeclModelDef();

	virtual size_t				Size( void ) const;
	virtual const char* 		DefaultDefinition( void ) const;
	virtual bool				Parse( const char* text, const int textLength );
	virtual void				FreeData( void );

	void						Touch( void ) const;

	const idDeclSkin* 			GetDefaultSkin( void ) const;
	const idJointQuat* 			GetDefaultPose( void ) const;
	void						SetupJoints( int* numJoints, idJointMat* *jointList, idBounds& frameBounds, bool removeOriginOffset ) const;
	void						GetJointList( const char* jointnames, idList<jointHandle_t>& jointList ) const;
	const jointInfo_t* 			FindJoint( const char* name ) const;

	int							NumAnims( void ) const;
	const idAnim* 				GetAnim( int index ) const;
	int							GetSpecificAnim( const char* name ) const;
	int							GetAnim( const char* name ) const;
	bool						HasAnim( const char* name ) const;
	const idDeclSkin* 			GetSkin( void ) const;
	const char* 				GetModelName( void ) const;
	const idList< jointInfo_t >&Joints( void ) const;
	const int* 					JointParents( void ) const;
	int							NumJoints( void ) const;
	const jointInfo_t* 			GetJoint( int jointHandle ) const;
	const char* 				GetJointName( int jointHandle ) const;
	int							NumJointsOnChannel( animChannel_t channel ) const { return channelJoints[ channel ].Num(); }
	const int* 					GetChannelJoints( animChannel_t channel ) const { return channelJoints[ channel ].Begin(); }
	int							NumChannels( void ) const { return numChannels; }

	const idVec3& 				GetVisualOffset( void ) const { return offset; }

	idRenderModel*				ModelHandle( void ) const { return modelHandle; }

	static void					CacheFromDict( const idDict& dict );

private:
	void						CopyDecl( const idDeclModelDef* decl );
	bool						ParseAnim( idParser& src, const idList< idStrList >& animGroups );
	bool						ParseAbsoluteFrameCommand( idAnim::absoluteFrameCommandType_t cmd, idParser& src, animList_t& animList );

private:
	typedef sdHashMapGeneric< idStr, idAnim* >						AnimTable;
	typedef sdHashMapGeneric< idStr, idListGranularityOne< int > >	AnimNumTable;

	idVec3									offset;
	idList<jointInfo_t>						joints;
	idList<int>								jointParents;
	idList<int>								channelJoints[ ANIM_NumAnimChannels ];
	idRenderModel* 							modelHandle;
	AnimTable								anims;
	mutable AnimNumTable					animLookup;
	const idDeclSkin* 						skin;
	int										numChannels;
};

/*
==============================================================================================

	idAnimBlend

==============================================================================================
*/

class idAnimBlend {
public:
								idAnimBlend();

	const char *				AnimName( void ) const;
	const char *				AnimFullName( void ) const;
	float						GetWeight( int currenttime ) const;
	float						GetFinalWeight( void ) const;
	void						SetWeight( float newweight, int currenttime, int blendtime );
	int							NumSyncedAnims( void ) const;
	bool						SetSyncedAnimWeight( int num, float weight );
	void						Clear( idClass* ent, int currentTime, int clearTime );
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
	const idAnim *				Anim( void ) const;
	int							AnimNum( void ) const;

	class idAnimBlendNetworkInfo_Minimal {
	public:
		void						MakeDefault( void );

		void						operator=( const idAnimBlend& anim );
		bool						operator==( const idAnimBlendNetworkInfo_Minimal& rhs ) const;
		bool						operator==( const idAnimBlend& rhs ) const;
		void						Write( idAnimBlend& anim ) const;

		void						Read( const idAnimBlendNetworkInfo_Minimal& base, const idBitMsg& msg );
		void						Write( const idAnimBlendNetworkInfo_Minimal& base, idBitMsg& msg ) const;

		void						Read( idFile* file );
		void						Write( idFile* file ) const;

	private:
		int							startTime;
		int							endTime;

		int							blendStartTime;
		int							blendDuration;
		float						blendStartValue;
		float						blendEndValue;

		short						animNum;
	};

	friend class				idAnimBlendNetworkInfo_Minimal;

private:
	const class idDeclModelDef *modelDef;
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
	float						frame;
	short						animNum;
	bool						allowMove;
	bool						allowFrameCommands;

	friend class				idAnimator;

	void						Reset( const idDeclModelDef* _modelDef );
	void						CallFrameCommands( idClass* ent, int fromtime, int totime ) const;
	void						SetFrame( idClass* ent, const idDeclModelDef* modelDef, int animnum, float frame, int currenttime, int blendtime );
	void						CycleAnim( idClass* ent, const idDeclModelDef* modelDef, int animnum, int currenttime, int blendtime );
	void						PlayAnim( idClass* ent, const idDeclModelDef* modelDef, int animnum, int currenttime, int blendtime );
	bool						BlendAnim( int currentTime, animChannel_t channel, int numJoints, idJointQuat* blendFrame, float& blendWeight, bool removeOrigin, bool overrideBlend, bool printInfo ) const;
	void						BlendOrigin( int currentTime, idVec3& blendPos, float& blendWeight, bool removeOriginOffset ) const;
	void						BlendDelta( int fromtime, int totime, idVec3& blendDelta, float& blendWeight ) const;
	void						BlendDeltaRotation( int fromtime, int totime, idQuat& blendDelta, float& blendWeight ) const;
	bool						AddBounds( int currentTime, idBounds& bounds, bool removeOriginOffset, bool ignoreLastFrame = false ) const;
};

/*
==============================================================================================

	idAFPoseJointMod

==============================================================================================
*/

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

	void						SetEntity( idClass* ent );
	idEntity*					GetEntity( void ) const;
	void						RemoveOriginOffset( bool remove );
	bool						RemoveOrigin( void ) const;

	void						GetJointList( const char* jointnames, idList<jointHandle_t>& jointList ) const;

	int							NumAnims( void ) const;
	const idAnim*				GetAnim( int index ) const;
	int							GetAnim( const char* name ) const;
	bool						HasAnim( const char* name ) const;

	void						ServiceAnims( int fromtime, int totime );
	bool						IsAnimating( int currentTime ) const;
	bool						IsAnimatingOnChannel( animChannel_t channelNum, int currentTime ) const;
	bool						IsPlayingAnim( animChannel_t channel, int animNum, int currentTime ) const;
	bool						IsPlayingAnimPrimary( animChannel_t channel, int animNum, int currentTime ) const;
	bool						IsCyclingAnim( animChannel_t channel, int animNum, int currentTime ) const;

	void						GetJoints( int* numJoints, idJointMat* *jointsPtr );
	int							NumJoints( void ) const;
	jointHandle_t				GetFirstChild( jointHandle_t jointnum ) const;
	jointHandle_t				GetFirstChild( const char* name ) const;

	idRenderModel*				SetModel( const char* modelname );
	idRenderModel*				ModelHandle( void ) const;
	const idDeclModelDef*		ModelDef( void ) const;

	void						ForceUpdate( void );
	void						ClearForceUpdate( void );
	bool						CreateFrame( int animtime, bool force );
	bool						FrameHasChanged( int animtime ) const;
	void						GetDelta( int fromtime, int totime, idVec3& delta, int maxChannels = ANIM_MaxAnimsPerChannel ) const;
	bool						GetDeltaRotation( int fromtime, int totime, idMat3& delta ) const;
	void						GetOrigin( int currentTime, idVec3& pos ) const;
	bool						GetBounds( int currentTime, idBounds& bounds, bool force = false );

	// Gets the bounding box in joint space of the specified mesh, this is uncached and will cause reskinning so use sparingly
	bool						GetMeshBounds( jointHandle_t jointnum, int meshHandle, int currentTime, idBounds& bounds, bool useDefaultAnim ); 

	idAnimBlend*				CurrentAnim( animChannel_t channelNum );
	void						Clear( animChannel_t channelNum, int currentTime, int cleartime );
	void						SetFrame( animChannel_t channelNum, int animnum, float frame, int currenttime, int blendtime );
	void						CycleAnim( animChannel_t channelNum, int animnum, int currenttime, int blendtime );
	void						PlayAnim( animChannel_t channelNum, int animnum, int currenttime, int blendTime );

								// copies the current anim from fromChannelNum to channelNum.
								// the copied anim will have frame commands disabled to avoid executing them twice.
	void						SyncAnimChannels( animChannel_t channelNum, animChannel_t fromChannelNum, int currenttime, int blendTime );

	void						SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3& pos );
	void						SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const idMat3& mat );
	void						ClearJoint( jointHandle_t jointnum );
	void						ClearAllJoints( void );

	void						InitAFPose( void );
	void						SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const idMat3& axis, const idVec3& origin );
	void						FinishAFPose( int animnum, const idBounds& bounds, const int time );
	void						SetAFPoseBlendWeight( float blendWeight );
	bool						BlendAFPose( idJointQuat* blendFrame ) const;
	void						ClearAFPose( void );

	void						ClearAllAnims( int currentTime, int cleartime );

	jointHandle_t				GetJointHandle( const char* name ) const;
	const char* 				GetJointName( const jointHandle_t handle ) const;
	animChannel_t				GetChannelForJoint( jointHandle_t joint ) const;
	bool						GetJointTransform( jointHandle_t jointHandle, int currenttime, idVec3& offset, idMat3& axis );
	bool						GetJointTransform( jointHandle_t jointHandle, int currenttime, idVec3& offset );
	bool						GetJointTransform( jointHandle_t jointHandle, int currentTime, idMat3& axis );
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset, idMat3& axis );
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset );
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idMat3& axis );

	jointHandle_t				GetJointParent( jointHandle_t jointHandle ) const;

	const animFlags_t			GetAnimFlags( int animnum ) const;
	int							NumFrames( int animnum ) const;
	int							NumSyncedAnims( int animnum ) const;
	const char*					AnimName( int animnum ) const;
	const char*					AnimFullName( int animnum ) const;
	int							AnimLength( int animnum ) const;
	const idVec3&				TotalMovementDelta( int animnum ) const;

	int							GetLastTransformTime( void ) const { return lastTransformTime; }
	int							GetTransformCount( void ) const { return transformCount; }

	typedef idAnimBlend::idAnimBlendNetworkInfo_Minimal animStates_t[ ANIM_MaxAnimsPerChannel ];

	void						ReadAnimStates( const animStates_t baseStates, animStates_t states, const idBitMsg& msg ) const;

	void						WriteAnimStates( const animStates_t baseStates, animStates_t states, animChannel_t channel, idBitMsg& msg ) const;
	bool						CheckAnimStates( const animStates_t baseStates, animChannel_t channel ) const;
	void						ApplyAnimStates( const animStates_t states, animChannel_t channel );

private:
	void						FreeData( void );
	void						PushAnims( animChannel_t channel, int currentTime, int blendTime );

	void						WriteAnimStates( const animStates_t baseStates, animStates_t states, const idAnimBlend channels[ ANIM_MaxAnimsPerChannel ], idBitMsg& msg ) const;
	bool						CheckAnimStates( const animStates_t baseStates, const idAnimBlend channels[ ANIM_MaxAnimsPerChannel ] ) const;
	void						ApplyAnimStates( const animStates_t states, idAnimBlend channels[ ANIM_MaxAnimsPerChannel ] );

private:
	const idDeclModelDef* 		modelDef;
	idClass* 					entity;

	idAnimBlend					channels[ ANIM_NumAnimChannels ][ ANIM_MaxAnimsPerChannel ];
	idList< jointMod_t* >		jointMods;
	int							numJoints;
	idJointMat* 				joints;

	mutable int					lastTransformTime;		// mutable because the value is updated in CreateFrame
	mutable bool				stoppedAnimatingUpdate;
	mutable int					transformCount;
	bool						removeOriginOffset;
	bool						forceUpdate;

	idBounds					frameBounds;

	float						AFPoseBlendWeight;
	idList<int>					AFPoseJoints;
	idList<idAFPoseJointMod>	AFPoseJointMods;
	int							numAFPoseJointFrame;
	idJointQuat* 				AFPoseJointFrame;
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
	idMD5Anim* 					GetAnim( const char* name );
	void						ReloadAnims( void );
	void						ListAnims( void ) const;
	int							JointIndex( const char* name );
	const char* 				JointName( int index ) const;

	void						ClearAnimsInUse( void );
	void						FlushUnusedAnims( void );

private:
	idHashMap< idMD5Anim* >		animations;
	idStrList					jointnames;
	idHashIndex					jointnamesHash;
};

#endif /* !__ANIM_H__ */
