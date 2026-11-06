#ifndef __GAME_VIEWBODY_H__
#define __GAME_VIEWBODY_H__

/**
 * Show player's torso and lower body model at first-person view

cvar harm_ui_showViewBody, default 0 = hide view body

// player_viewbody def example:
entityDef player_viewbody { // must setup in player entity with property 'player_viewbody'
    "spawnclass"				"idViewBody"
    "body_model"                "player_model_torso_and_lower_body" // body's md5 model: animations's name same as player model: string
    "body_offset"               "-15 0 0" // extras model offset: vector <forward right up>, default = 0 0 0
	"body_allChannel"			"0" // play animation with all channels, else only play with legs channel: bool, default = 1
	"body_usePlayerModel"		"0" // use player model and not use 'body_model': bool, default = 0
	"body_weaponDepthHack"		"0" // override view weapon: bool, default = 0
	"body_modelDepthHack"		"0.0" // near clip: float, default = 0
	"body_hidesurfaces" "shader_head,shader_toast" // hide surface names, separate by ',': string
	// "hidesurface1" "shader_arm" // hide surface name by entity property: string, hidesurfaceXXX
	// "hidesurface5" "shader_neck"
	// "hidesurface0" "shader_hand"
	// "anim run_forward" "walk_forward" // override model animation name: string, "anim <model animation name>" "<replace animation name>"
}

// model example:
model player_model_torso_and_lower_body {
    offset (0 0 0)
    model models/md5/player_model_torso_and_lower_body.md5mesh

// loop anims:
    idle models/md5/player_model_torso_and_lower_body/idle.md5anim
    run_forward models/md5/player_model_torso_and_lower_body/run_forward.md5anim
    run_backwards models/md5/player_model_torso_and_lower_body/run_backwards.md5anim
    run_strafe_left models/md5/player_model_torso_and_lower_body/run_strafe_left.md5anim
    run_strafe_right models/md5/player_model_torso_and_lower_body/run_strafe_right.md5anim
    walk_forward models/md5/player_model_torso_and_lower_body/walk_forward.md5anim
    walk_backwards models/md5/player_model_torso_and_lower_body/walk_backwards.md5anim
    walk_left models/md5/player_model_torso_and_lower_body/walk_left.md5anim
    walk_right models/md5/player_model_torso_and_lower_body/walk_right.md5anim
    crouch models/md5/player_model_torso_and_lower_body/crouch.md5anim
    crouch_walk models/md5/player_model_torso_and_lower_body/crouch_walk.md5anim
    crouch_walk_backward models/md5/player_model_torso_and_lower_body/crouch_walk_backward.md5anim
    fall models/md5/player_model_torso_and_lower_body/fall.md5anim

// single anims:
    crouch_down models/md5/player_model_torso_and_lower_body/crouch_down.md5anim
    crouch_up models/md5/player_model_torso_and_lower_body/crouch_up.md5anim
    run_jump models/md5/player_model_torso_and_lower_body/run_jump.md5anim
    jump models/md5/player_model_torso_and_lower_body/jump.md5anim
    hard_land models/md5/player_model_torso_and_lower_body/hard_land.md5anim
    soft_land models/md5/player_model_torso_and_lower_body/soft_land.md5anim
}
 */

class idPlayer;
class idViewBody : public idAnimatedEntity {
public:

    CLASS_PROTOTYPE( idViewBody );

    idViewBody( void );
    virtual					~idViewBody( void );

    // Init
    void					Spawn						( void );

    // save games
    void					Save						( idSaveGame *savefile ) const;					// archives object for save game file
    void					Restore						( idRestoreGame *savefile );					// unarchives object from save game file


    // Weapon definition management
    void					Clear						( void );

    virtual void			SetViewModel				( const char *modelname );

    // State control/player interface
    void					Think						( void );

    // Visual presentation
    void					PresentBody				    ( bool showViewModel );

    // Networking
    virtual void			WriteToSnapshot				( idBitMsgDelta &msg ) const {};
    virtual void			ReadFromSnapshot			( const idBitMsgDelta &msg ) {};
    virtual bool			ClientReceiveEvent			( int event, int time, const idBitMsg &msg );
    virtual void			ClientPredictionThink		( void );
    virtual bool			ClientStale					( void );

    virtual void			ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis );
    virtual void			UpdateModelTransform		( void );

    virtual void			GetPosition					( idVec3& origin, idMat3& axis ) const;
    void				    Init						( idPlayer* _owner, bool setstate );
    // Debugging
    virtual void			GetDebugInfo				( debugInfoProc_t proc, void* userData );
    idPlayer *              GetOwner                    ( void );
	void					UpdateBody				    (void);
	void					EnterCinematic				(void);
	void					ExitCinematic				(void);

protected:
    void				PlayAnim					( int channel, const char *animname, int blendFrames );
    void				PlayCycle					( int channel, const char *animname, int blendFrames );
    bool				AnimDone					( int channel, int blendFrames );
    void				SetState					( const char *statename, int blendFrames );
    void				PostState					( const char *statename, int blendFrames );
    void				ExecuteState				( const char *statename );
	int					GetBodyAnim					( const char *animname );

    stateResult_t			State_Wait_Alive				( const stateParms_t& parms );

    stateResult_t			State_Legs_Idle					( const stateParms_t& parms );
    stateResult_t			State_Legs_Run_Forward			( const stateParms_t& parms );
    stateResult_t			State_Legs_Run_Backward			( const stateParms_t& parms );
    stateResult_t			State_Legs_Run_Left				( const stateParms_t& parms );
    stateResult_t			State_Legs_Run_Right			( const stateParms_t& parms );
    stateResult_t			State_Legs_Walk_Forward			( const stateParms_t& parms );
    stateResult_t			State_Legs_Walk_Backward		( const stateParms_t& parms );
    stateResult_t			State_Legs_Walk_Left			( const stateParms_t& parms );
    stateResult_t			State_Legs_Walk_Right			( const stateParms_t& parms );
    stateResult_t			State_Legs_Crouch				( const stateParms_t& parms );
    stateResult_t			State_Legs_Uncrouch				( const stateParms_t& parms );
    stateResult_t			State_Legs_Crouch_Idle			( const stateParms_t& parms );
    stateResult_t			State_Legs_Crouch_Forward		( const stateParms_t& parms );
    stateResult_t			State_Legs_Crouch_Backward		( const stateParms_t& parms );
    stateResult_t			State_Legs_Jump					( const stateParms_t& parms );
    stateResult_t			State_Legs_Fall					( const stateParms_t& parms );
    stateResult_t			State_Legs_Land					( const stateParms_t& parms );
    stateResult_t			State_Legs_Dead					( const stateParms_t& parms );

private:
	int SplitSurfaces(const char *surfaces, idStrList &ret) const;

private:
    idPlayer *                      owner;
    rvStateThread					stateThread;
    int								animDoneTime[ANIM_NumAnimChannels];
    idVec3                          bodyOffset;
	idStrList						hideSurfaces;
	bool							allChannel;
	bool							usePlayerModel;
	int								animChannel;
	bool							weaponDepthHack;
	float							modelDepthHack;

    friend class idPlayer;

    CLASS_STATES_PROTOTYPE ( idViewBody );
};

ID_INLINE idPlayer * idViewBody::GetOwner( void )
{
    return owner;
}
#endif
