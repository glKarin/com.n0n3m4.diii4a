// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ACTOR_H__
#define __GAME_ACTOR_H__

/*
===============================================================================

	idActor

===============================================================================
*/

#include "AFEntity.h"
#include "IK.h"
#include "structures/TeamManager.h"

extern const idEventDef EV_EnableLegIK;
extern const idEventDef EV_DisableLegIK;
extern const idEventDef AI_SetPrefix;
extern const idEventDef AI_AnimDone;
extern const idEventDef AI_SetBlendFrames;
extern const idEventDef AI_GetBlendFrames;

class idDeclParticle;
class idActor;

class idAnimState {
public:
	bool					idleAnim;
	idStr					state;
	int						animBlendFrames;
	int						lastAnimBlendFrames;		// allows override anims to blend based on the last transition time

public:
							idAnimState();
							~idAnimState();

	void					Init( idActor *owner, idAnimator *_animator, animChannel_t animchannel );
	void					Shutdown( void );
	void					SetState( const char *name, int blendFrames );
	void					StopAnim( int frames );
	void					PlayAnim( int anim );
	void					CycleAnim( int anim );
	void					BecomeIdle( void );
	bool					UpdateState( void );
	bool					Disabled( void ) const;
	void					Enable( int blendFrames );
	void					Disable( void );
	bool					AnimDone( int blendFrames ) const;
	bool					IsIdle( void ) const;
	animFlags_t				GetAnimFlags( void ) const;
	sdProgramThread*	GetThread( void ) const { return thread; }

private:
	idActor *				self;
	idAnimator *			animator;
	sdProgramThread*	thread;
	animChannel_t			channel;
	bool					disabled;
};

class idActor : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( idActor );

	idMat3					viewAxis;			// view axis of the actor
	idVec3					viewAxisOrientator;
	idMat3					viewAxisOrientation;

	enum ePrefixFlags {	APF_WEAPON			= BITT< 0 >::VALUE,
						APF_WEAPON_CLASS	= BITT< 1 >::VALUE,
						APF_STANCE			= BITT< 2 >::VALUE,
						APF_STANCE_ACTION	= BITT< 3 >::VALUE,
						APF_CHANNEL_NAME	= BITT< 4 >::VALUE
	};

	enum ePrefixes {	AP_WEAPON,
						AP_WEAPON_CLASS,
						AP_STANCE,
						AP_STANCE_ACTION,
						AP_CHANNEL_NAME,
						AP_MAX
	};

public:
							idActor( void );
	virtual					~idActor( void );

	static void				ReportCurrentState_f( const idCmdArgs& args );

	void					Spawn( void );
	virtual void			Restart( void );

	void					SetupBody( void );

	virtual void			SetAxis( const idMat3 &axis );
	virtual const idMat3	&GetAxis() { return viewAxis; }
	virtual void			SetPosition( const idVec3 &org, const idMat3 &axis );

	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

							// script state management
	virtual void			ShutdownThreads( void );
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
	virtual sdProgramThread* ConstructScriptObject( void );
	void					UpdateScript( void );
	const sdProgram::sdFunction* GetScriptFunction( const char *funcname );
	void					SetState( const sdProgram::sdFunction* newState );
	void					SetState( const char *statename );

							// vision testing
	bool					CheckFOV( const idVec3 &pos ) const;
	bool					CanSee( idEntity *ent, bool useFOV ) const;
	bool					PointVisible( const idVec3 &point ) const;

							// damage
	void					ClearPain( void );
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );
	virtual void			PlayPain( const char* strength );

							// model/combat model
	void					SetCombatModel( void );
	void					RemoveCombatModel( void );
	virtual void			LinkCombat( void );
	virtual void			UnLinkCombat( void );
	virtual void			EnableCombat( void );
	virtual void			DisableCombat( void );
	virtual bool			UpdateAnimationControllers( void );

							// delta view angles to allow movers to rotate the view of the actor
	const idAngles &		GetDeltaViewAngles( void ) const;
	void					SetDeltaViewAngles( const idAngles &delta );

	virtual bool			OnLadder( void ) const;

	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );

							// animation state control
	int						GetAnim( int channel, const char *name );
	void					UpdateAnimState( void );
	void					SetAnimState( int channel, const char *name, int blendFrames );
	const char *			GetAnimState( int channel ) const;
	bool					InAnimState( int channel, const char *name ) const;
	bool					AnimDone( int channel, int blendFrames ) const;

	virtual void			SetGameTeam( sdTeamInfo* _team ) { team = _team; }
	sdTeamInfo*				GetTeam( void ) { return team; }

	void					SetPrefix( animChannel_t channel, ePrefixes prefix, const char* prefixValue );

private:
	void					AnimationMissing( animChannel_t channel, const char* animation );

protected:
	friend class			idAnimState;

	sdTeamInfo*				team;

	idVec3					eyeOffset;			// offset of eye relative to physics origin

	idAngles				deltaViewAngles;	// delta angles relative to view input angles

	int						painDebounceTime;	// next time the actor can show pain
	int						painDelay;			// time between playing pain sound

	idStrList				damageGroups;		// body damage groups

	// state variables
	const sdProgram::sdFunction*		state;
	const sdProgram::sdFunction*		idealState;

	// joint handles
	jointHandle_t			soundJoint;

	// script variables
	sdProgramThread*		scriptThread;
	idAnimState				torsoAnim;
	idAnimState				legsAnim;

private:
	idStrList				prefixes[ ANIM_NumAnimChannels ];
	sdStringBuilder_Heap	completeAnim;

private:

	void					AssembleAnimName( animChannel_t channel, const char* action, unsigned int prefixes );
	void					SyncAnimChannels( animChannel_t channel, animChannel_t syncToChannel, int blendFrames );
	void					FinishSetup( void );

	void					Event_SetPrefix( animChannel_t channel, ePrefixes prefix, const char *name );
	void					Event_LookAtEntity( idEntity *ent, float duration );
	void					Event_StopAnim( int channel, int frames );
	void					Event_PlayAnim( animChannel_t channel, const char *name );
	void					Event_PlayCycle( animChannel_t channel, const char *name );
	void					Event_IdleAnim( animChannel_t channel, const char *name );
	void					Event_SetAnimFrame( const char *animname, animChannel_t channel, float frame );
	void					Event_OverrideAnim( int channel );
	void					Event_EnableAnim( int channel, int blendFrames );
	void					Event_SetBlendFrames( int channel, int blendFrames );
	void					Event_GetBlendFrames( int channel );
	void					Event_AnimState( int channel, const char *name, int blendFrames );
	void					Event_GetAnimState( int channel );
	void					Event_InAnimState( int channel, const char *name );
	void					Event_AnimDone( int channel, int blendFrames );
	void					Event_HasAnim( int channel, const char* name );
	void					Event_StopSound( int channel );
	void					Event_SetState( const char *name );
	void					Event_GetState( void );
	void					Event_SyncAnim( animChannel_t channel, animChannel_t syncToChannel, int blendFrames );
};

#endif /* !__GAME_ACTOR_H__ */
