#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "EventReciever.h"
#include "Base_Messages.h"
#include "StateMachine.h"
#include "DebugWindow.h"
#include "NameManager.h"

class BotItemSystem;
namespace AiState
{
	class SensoryMemory;
	class TargetingSystem;
	class WeaponSystem;
	class SteeringSystem;
}

class gmThread;
class gmTableObject;
class gmMachine;
class gmUserObject;
class gmFunctionObject;

//////////////////////////////////////////////////////////////////////////

// class: Client
class Client : public EventReciever
{
public:	
	friend class gmBot;

	typedef enum
	{
		PROFILE_NONE,
		PROFILE_CUSTOM,
		PROFILE_CLASS,

		NUM_PROFILE_TYPES
	} ProfileType;

	typedef enum
	{
		FL_DISABLED = 0,		
		FL_SHOOTINGDISABLED,
		FL_SELECTBESTWEAPON_OFF,
		FL_USINGMOUNTEDWEAPON,
		FL_DIRTYEYEPOS,

		// THIS MUST BE LAST
		NUM_INTERNAL_FLAGS
	} InternalFlags;

	// struct: HoldButtons
	//		Tracks time values that buttons should be held for.
	class HoldButtons  
	{
	public:
		enum		{ NumButtons = 64 };
		obuint32	m_StopHoldTime[NumButtons];

		HoldButtons();
	};

	// Function: GetName
	//		Get the current name of this bot
	const char *GetName(bool _clean = false) const;

	// Function: GetTeam
	//		Get the current team this bot is on
	inline int GetTeam() const							{ return m_Team; }

	inline int GetClass() const							{ return m_Class; }

	// Function: GetGameID
	//		Get the current <GameId> of this bot
	inline GameId GetGameID() const						{ return m_GameID; }

	// Function: GetGameEntity
	//		Get the current <GameEntity> of this bot
	inline const GameEntity &GetGameEntity() const		{ return m_GameEntity; }

	inline const Vector3f &GetPosition() const			{ return m_Position; }
	inline const Vector3f &GetFacingVector() const		{ return m_FacingVector; }
	inline const Vector3f &GetUpVector() const			{ return m_UpVector; }
	inline const Vector3f &GetRightVector() const		{ return m_RightVector; }
	inline const Vector3f &GetVelocity() const			{ return m_Velocity; }
	inline const Vector3f &GetMovementVector() const	{ return m_MoveVector; }
	inline const Box3f &GetWorldBounds() const			{ return m_WorldBounds; }
	const Matrix3f &GetOrientation() const				{ return m_Orientation; }
	inline Vector3f GetCenterBounds() const				{ return m_WorldBounds.Center; }

	//////////////////////////////////////////////////////////////////////////

	inline void SetMovementVector(const Vector3f &_vec)	{ m_MoveVector = _vec; }

	inline bool InFieldOfView(const Vector3f &_pos);

	bool HasLineOfSightTo(const Vector3f &_pos, GameEntity _entity = GameEntity(), int customTraceMask = 0);
	static bool HasLineOfSightTo(const Vector3f &_pos1, const Vector3f &_pos2, 
		GameEntity _ent = GameEntity(), int _ignoreent = -1, int customTraceMask = 0);

	bool IsAllied(const GameEntity _ent) const;

	Vector3f GetEyePosition();	

	virtual bool CanGetPowerUp(obint32 _powerup) const;

	inline const BitFlag32 GetMovementCaps() const				{ return m_MovementCaps; }

	inline const BitFlag64 &GetEntityFlags() const				{ return m_EntityFlags; }
	inline const BitFlag64 &GetPowerUpFlags() const				{ return m_EntityPowerUps; }
	inline const BitFlag32 &GetRoleMask() const					{ return m_RoleMask; }
	inline void SetRoleMask(BitFlag32 &_bf)						{ m_RoleMask = _bf; }
	inline bool IsInfiltrator() const { return m_RoleMask.CheckFlag(3); }

	inline bool HasEntityFlag(obint32 _flag) const				{ return m_EntityFlags.CheckFlag(_flag); }
	inline bool HasPowerup(obint32 _flag) const					{ return m_EntityPowerUps.CheckFlag(_flag); }

	inline void SetUserFlag(int _flag, bool _enable);
	inline bool CheckUserFlag(int _flag) const;

	// Function: TurnTowardPosition
	//		Makes the bot turn toward a world space position.
	bool TurnTowardFacing(const Vector3f &_facing);

	// Function: TurnTowardPosition
	//		Makes the bot turn toward a facing.
	bool TurnTowardPosition(const Vector3f &_pos);

	// function: MoveTo
	//		Moves the bot towards the position if the aren't within the _tolerance distance.
	//		returns true if within tolerance, false if not.
	bool MoveTo(const Vector3f &_pos, float _tolerance, MoveMode _m = Run);

	Vector3f ToLocalSpace(const Vector3f &_worldpos);
	Vector3f ToWorldSpace(const Vector3f &_localpos);

	void OutputDebug(MessageType _type, const char * _str);
	
	// Property Accessors

	// todo: put error checking in here, not gmBot
	inline float GetFieldOfView() const						{ return m_FieldOfView; }
	inline void SetFieldOfView(float _fov)					{ m_FieldOfView = _fov; }
	inline float GetMaxViewDistance() const					{ return m_MaxViewDistance; }
	inline void SetMaxViewDistance(float _dist)				{ m_MaxViewDistance = _dist; }
	inline bool IsWithinViewDistance(const Vector3f &_pos);
	inline float GetMaxTurnSpeed() const					{ return m_MaxTurnSpeed; }
	inline void SetMaxTurnSpeed(float _speed)				{ m_MaxTurnSpeed = _speed; }
	inline float GetAimStiffness() const					{ return m_AimStiffness; }
	inline void SetAimStiffness(float _stiffness)			{ m_AimStiffness = _stiffness; }
	inline float GetAimDamping() const						{ return m_AimDamping; }
	inline void SetAimDamping(float _damping)				{ m_AimDamping = _damping; }
	inline float GetAimTolerance() const					{ return m_AimTolerance; }
	inline void SetAimTolerance(float _tolerance)			{ m_AimTolerance = _tolerance; }

	AiState::SensoryMemory		*GetSensoryMemory();
	AiState::SteeringSystem		*GetSteeringSystem();
	AiState::WeaponSystem		*GetWeaponSystem();
	AiState::TargetingSystem	*GetTargetingSystem();

	inline int GetCurrentHealth() const					{ return m_HealthArmor.m_CurrentHealth; }
	inline int GetMaxHealth() const						{ return m_HealthArmor.m_MaxHealth; }
	inline obReal GetHealthPercent() const;
	inline int GetCurrentArmor() const					{ return m_HealthArmor.m_CurrentArmor; }
	inline int GetMaxArmor() const						{ return m_HealthArmor.m_MaxArmor; }
	inline obReal GetArmorPercent() const;
	inline float GetStepHeight() const					{ return m_StepHeight; }
	inline float GetMaxSpeed() const					{ return m_MaxSpeed; }
	
	virtual void Init(int _gameid);
	virtual void Update();
	virtual void UpdateBotInput();
	virtual void Shutdown();

	gmUserObject *GetScriptObject();
	gmVariable GetScriptVariable();

	// Function: ChangeTeam
	//		Tells the bot to change to a different team.
	virtual void ChangeTeam(int _team);

	// Function: ChangeClass
	//		Tells the bot to change to a different class.
	virtual void ChangeClass(int _class);

	void LoadProfile(ProfileType _type);
	void ClearProfile();

	inline void PressButton(int _button)			{ m_ButtonFlags.SetFlag(_button); }
	inline void ReleaseButton(int _button)			{ m_ButtonFlags.ClearFlag(_button); }
	inline bool IsButtonDown(int _button) const		{ return m_ButtonFlags.CheckFlag(_button); }

	void HoldButton(const BitFlag64 &_buttons, int _mstime);
	void ReleaseHeldButton(const BitFlag64 &_buttons);
	void ReleaseAllHeldButtons();

	//AimRequestPtr GetAimRequest(const char *_owner);

	void EnableDebug(const int _flag, bool _enable);
	inline bool IsDebugEnabled(obint32 _flag) const		{ return m_DebugFlags.CheckFlag(_flag); }

	void CheckStuck();
	inline int GetStuckTime() const { return m_StuckTime; }
	inline void ResetStuckTime() { m_StuckTime = 0; }

	virtual NavFlags GetTeamFlag() = 0;
	virtual NavFlags GetTeamFlag(int _team) = 0;

	inline BlackBoard &GetBB()						{ return m_Blackboard; }

	void GameCommand(CHECK_PRINTF_ARGS const char* _msg, ...);

	// These helper functions allow the derived class to perform additional
	// actions as a result of mode specific flags on the waypoint. This allow handling
	// of custom nav flags without subclassing the actual goals.
	virtual void ProcessGotoNode(const Path &_path) {}

	// Game specific variables.
	typedef enum
	{
		JumpGapOffset,
	} GameVar;

	virtual float GetGameVar(GameVar _var) const = 0;
	virtual float GetAvoidRadius(int _class) const = 0;

	virtual void SendVoiceMacro(int _macroId) = 0;
	virtual int HandleVoiceMacroEvent(const MessageHelper &_message) { return 0; }

	virtual bool DoesBotHaveFlag(MapGoalPtr _mapgoal) = 0;
	virtual bool IsFlagGrabbable(MapGoalPtr _mapgoal) { return true; }
	virtual bool IsItemGrabbable(GameEntity _ent) { return false; }

	virtual bool CanBotSnipe() { return false; }
	virtual bool GetSniperWeapon(int &nonscoped, int &scoped) { nonscoped=0; scoped=0; return false; }
	virtual bool GetSkills(gmMachine *machine, gmTableObject *tbl) { _UNUSED(machine); _UNUSED(tbl); return false;}
	virtual int ConvertWeaponIdToMod(int weaponId) { return weaponId; }

	virtual float NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to) { return false; }

	inline int GetProfileType() const					{ return m_ProfileType; }

	State *GetStateRoot() { return m_StateRoot; }

	void InitBehaviorTree();
	bool AddScriptGoal(const String &_name);
	void InitScriptGoals();
	virtual void SetupBehaviorTree() {}
	
	int				m_DesiredTeam;
	int				m_DesiredClass;

	void CheckTeamEvent();
	void CheckClassEvent();

	void PropogateDeletedThreads(const int *_threadIds, int _numThreads);

	bool DistributeUnhandledCommand(const StringVector &_args);

#ifdef ENABLE_REMOTE_DEBUGGING
	virtual void InternalSyncEntity( EntitySnapShot & snapShot, RemoteLib::DataBuffer & db );
#endif

	Client();
	virtual ~Client();

	int		m_SpawnTime;

protected:

	float			m_StepHeight;
	float			m_MaxSpeed;

	AABB			m_StuckBounds;
	int				m_StuckTime;
	bool			m_StuckExpanded;

	State			*m_StateRoot;

	ClientInput		m_ClientInput;

	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);
	void ProcessEventImpl(const MessageHelper &_message, obuint32 _targetState);

private:
	Vector3f		m_Position;
	Vector3f		m_EyePosition;
	Vector3f		m_MoveVector;
	Vector3f		m_Velocity;
	Vector3f		m_FacingVector;
	Vector3f		m_UpVector;
	Vector3f		m_RightVector;
	Box3f			m_WorldBounds;
	Matrix3f		m_Orientation;

	GameEntity		m_MoveEntity;

	BitFlag64		m_ButtonFlags;

	float			m_FieldOfView;
	float			m_MaxViewDistance;

	// Bot Properties
	BitFlag32		m_MovementCaps;
	BitFlag32		m_RoleMask;
	BitFlag64		m_EntityFlags;
	BitFlag64		m_EntityPowerUps;
	BitFlag64		m_InternalFlags;

	Msg_HealthArmor	m_HealthArmor;

	int				m_Team;
	int				m_Class;
	GameId			m_GameID;
	GameEntity		m_GameEntity;

	gmUserObject	*m_ScriptObject;

	// Aiming properties
	float			m_CurrentTurnSpeed;
	float			m_MaxTurnSpeed;
	float			m_AimStiffness;
	float			m_AimDamping;
	float			m_AimTolerance;

	HoldButtons		m_HoldButtons;

	ProfileType		m_ProfileType;

	BlackBoard		m_Blackboard;

	File			m_DebugLog;
	BitFlag32		m_DebugFlags;

	NamePtr			m_NameReference;

	//int				m_SoundSubscriber;
};

inline obReal Client::GetHealthPercent() const
{
	return(m_HealthArmor.m_MaxHealth > 0) ?
		(obReal)m_HealthArmor.m_CurrentHealth / (obReal)m_HealthArmor.m_MaxHealth : (obReal)1.0;
}

inline obReal Client::GetArmorPercent() const
{
	return (m_HealthArmor.m_MaxArmor > 0) ?
		(obReal)m_HealthArmor.m_CurrentArmor / (obReal)m_HealthArmor.m_MaxArmor : (obReal)1.0;
}

inline bool Client::InFieldOfView(const Vector3f &_pos) 
{
	Vector3f toTarget = _pos - GetEyePosition();
	toTarget.Normalize();
	return Utils::InFieldOfView2d(m_FacingVector, toTarget, m_FieldOfView);
}

inline bool Client::IsWithinViewDistance(const Vector3f &_pos)
{
	return ((_pos - m_Position).SquaredLength() <= (m_MaxViewDistance*m_MaxViewDistance));
}

inline void Client::SetUserFlag(int _flag, bool _enable)
{
	if(_enable)
		m_InternalFlags.SetFlag(_flag);
	else
		m_InternalFlags.ClearFlag(_flag);
}

inline bool Client::CheckUserFlag(int _flag) const
{
	return m_InternalFlags.CheckFlag(_flag);
}

#endif
