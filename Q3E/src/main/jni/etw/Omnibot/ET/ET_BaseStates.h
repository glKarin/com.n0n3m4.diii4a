#ifndef __ETBaseStates_H__
#define __ETBaseStates_H__

#include "StateMachine.h"

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////

	//class PlantMine : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(StringStr &out);
	//	MapGoal *GetMapGoalPtr();
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	PlantMine();
	//private:

	//	Trackers			Tracker;
	//	MapGoalPtr			m_MapGoal;

	//	GameEntity			m_LandMineEntity;
	//	Vector3f			m_LandMinePosition;
	//	Vector3f			m_LandMineVelocity;
	//};

	//////////////////////////////////////////////////////////////////////////

	//class MobileMortar : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:
	//	void GetDebugString(StringStr &out);
	//	MapGoal *GetMapGoalPtr();
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	MobileMortar();
	//private:
	//	enum { MaxMortarAims = 8 };
	//	Vector3f			m_MortarAim[MaxMortarAims];
	//	int					m_CurrentAim;
	//	int					m_NumMortarAims;
	//	int					m_FireDelay;

	//	WeaponLimits		m_Limits;
	//	MapGoalPtr			m_MapGoal;
	//	Trackers			Tracker;

	//	bool CacheGoalInfo(MapGoalPtr mg);
	//};

	//////////////////////////////////////////////////////////////////////////

	//class CallArtillery : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(StringStr &out);
	//	MapGoal *GetMapGoalPtr();
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	CallArtillery();
	//private:
	//	Trackers			Tracker;
	//	MapGoalPtr			m_MapGoal;
	//	MapGoalPtr			m_MapGoalTarget;
	//	GameEntity			m_TargetEntity;
	//	int					m_FireTime;

	//	FilterPtr			m_WatchFilter;

	//	bool				m_Fired;
	//	Seconds				m_MinCampTime;
	//	Seconds				m_MaxCampTime;
	//	int					m_Stance;

	//	int					m_ExpireTime;
	//};

	//////////////////////////////////////////////////////////////////////////

	//class UseCabinet : public StateChild, public FollowPathUser//, public AimerUser
	//{
	//public:
	//	enum CabinetType
	//	{
	//		Health,
	//		Ammo,
	//	};

	//	void GetDebugString(StringStr &out);
	//	MapGoal *GetMapGoalPtr();
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	UseCabinet();
	//private:
	//	Trackers			Tracker;
	//	MapGoalPtr			m_MapGoal;
	//	CabinetType			m_CabinetType;
	//	int					m_AmmoType;
	//	int					m_GetAmmoAmount;
	//	obint32				m_UseTime;

	//	float				m_HealthPriority;
	//	float				m_AmmoPriority;
	//	float				m_Range;

	//	GoalManager::Query	m_Query;
	//};

	//////////////////////////////////////////////////////////////////////////

	//class Flamethrower : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	Flamethrower();
	//private:
	//	Trackers		Tracker;
	//	MapGoalPtr		m_MapGoal;
	//	Vector3f		m_AimPosition;
	//	
	//	TrackTargetZone m_TargetZone;

	//	int				m_MinCampTime;
	//	int				m_MaxCampTime;
	//	int				m_Stance;

	//	int				m_ExpireTime;
	//};

	////////////////////////////////////////////////////////////////////////////

	//class Panzer : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	Panzer();
	//private:
	//	Trackers		Tracker;
	//	MapGoalPtr		m_MapGoal;
	//	Vector3f		m_AimPosition;
	//	
	//	TrackTargetZone m_TargetZone;

	//	int				m_MinCampTime;
	//	int				m_MaxCampTime;
	//	int				m_Stance;

	//	int				m_ExpireTime;
	//};

	//class BuildConstruction : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	BuildConstruction();
	//private:
	//	Trackers		Tracker;

	//	MapGoalPtr		m_MapGoal;
	//	Vector3f		m_ConstructionPos;
	//	bool			m_AdjustedPosition;
	//	bool			m_Crouch;
	//	bool			m_Prone;
	//	bool			m_IgnoreTargets;
	//};

	//////////////////////////////////////////////////////////////////////////

	//class PlantExplosive : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:
	//	typedef ET_SetDynamiteGoal Mg;

	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	PlantExplosive();
	//private:
	//	typedef enum
	//	{
	//		LAY_EXPLOSIVE,
	//		ARM_EXPLOSIVE,
	//		RUNAWAY,
	//		DETONATE_EXPLOSIVE
	//	} GoalState;

	//	Trackers			Tracker;
	//	MapGoalPtr			m_MapGoal;

	//	GoalState			m_GoalState;
	//	Vector3f			m_TargetPosition;
	//	GameEntity			m_ExplosiveEntity;
	//	Vector3f			m_ExplosivePosition;
	//	bool				m_AdjustedPosition;
	//	bool				m_IgnoreTargets;

	//	State::StateStatus _UpdateDynamite();
	//	State::StateStatus _UpdateSatchel();
	//	bool _IsGoalAchievable(MapGoalPtr _g, int _weaponId);
	//};

	//////////////////////////////////////////////////////////////////////////

	//class MountMg42 : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:
	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	MountMg42();
	//private:
	//	MapGoalPtr			m_MapGoal;
	//	Trackers			Tracker;

	//	enum ScanType
	//	{
	//		SCAN_DEFAULT,
	//		SCAN_MIDDLE,
	//		SCAN_LEFT,
	//		SCAN_RIGHT,
	//		SCAN_ZONES,

	//		NUM_SCAN_TYPES
	//	};
	//	int					m_ScanDirection;
	//	int					m_NextScanTime;

	//	TrackTargetZone		m_TargetZone;

	//	Seconds					m_MinCampTime;
	//	Seconds					m_MaxCampTime;
	//	int					m_Stance;

	//	//////////////////////////////////////////////////////////////////////////
	//	Vector3f			m_MG42Position;

	//	Vector3f			m_AimPoint;

	//	Vector3f			m_GunCenterArc;
	//	Vector3f			m_CurrentMountedAngles;
	//	
	//	Vector3f			m_ScanLeft;
	//	Vector3f			m_ScanRight;

	//	float				m_MinHorizontalArc;
	//	float				m_MaxHorizontalArc;
	//	float				m_MinVerticalArc;
	//	float				m_MaxVerticalArc;

	//	int					m_ExpireTime;
	//	int					m_StartTime;

	//	bool				m_GotGunProperties;
	//	//////////////////////////////////////////////////////////////////////////

	//	bool				m_AdjustedPosition;
	//	bool				m_IgnoreTargets;

	//	bool				_GetMG42Properties();
	//};

	//////////////////////////////////////////////////////////////////////////

	//class MobileMg42 : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	MobileMg42();
	//private:
	//	Vector3f			m_AimPosition;
	//	WeaponLimits		m_Limits;
	//	MapGoalPtr			m_MapGoal;
	//	TrackTargetZone		m_TargetZone;
	//	Trackers			Tracker;

	//	Seconds					m_MinCampTime;
	//	Seconds					m_MaxCampTime;

	//	int					m_ExpireTime;
	//};

	//////////////////////////////////////////////////////////////////////////

	//class RepairMg42 : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:
	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	RepairMg42();
	//private:
	//	Vector3f			m_MG42Position;
	//	MapGoalPtr			m_MapGoal;
	//	Trackers			Tracker;

	//	bool				m_IgnoreTargets;
	//};

	//////////////////////////////////////////////////////////////////////////

	//class TakeCheckPoint : public StateChild, public FollowPathUser
	//{
	//public:

	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	TakeCheckPoint();
	//private:
	//	Trackers		Tracker;

	//	MapGoalPtr		m_MapGoal;
	//	Vector3f		m_TargetPosition;
	//};

	//////////////////////////////////////////////////////////////////////////

	//class ReviveTeammate : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:
	//	typedef enum
	//	{
	//		REVIVING,
	//		HEALING,
	//	} GoalState;

	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	ReviveTeammate();
	//private:
	//	GoalState			m_GoalState;

	//	GameTimer			m_CheckReviveTimer;

	//	Trackers			Tracker;
	//	MapGoalPtr			m_MapGoal;
	//	MapGoalList			m_List;
	//	float				m_Range;

	//	bool AreThereNewReviveGoals();
	//};

	//////////////////////////////////////////////////////////////////////////

	//class DefuseDynamite : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(StringStr &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	DefuseDynamite();
	//private:
	//	Vector3f			m_TargetPosition;

	//	Trackers			Tracker;
	//	MapGoalPtr			m_MapGoal;
	//};
};

#endif
