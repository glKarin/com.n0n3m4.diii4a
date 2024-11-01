////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ETQWBaseStates_H__
#define __ETQWBaseStates_H__

#include "StateMachine.h"

namespace AiState
{
	class BuildConstruction : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		BuildConstruction();
	private:
		Trackers		Tracker;

		MapGoalPtr		m_MapGoal;
		Vector3f		m_ConstructionPos;
		bool			m_AdjustedPosition;
	};

	//////////////////////////////////////////////////////////////////////////

	class PlantExplosive : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		PlantExplosive();
	private:
		typedef enum
		{
			LAY_EXPLOSIVE,
			ARM_EXPLOSIVE,
			RUNAWAY,
			DETONATE_EXPLOSIVE
		} GoalState;

		Trackers			Tracker;
		MapGoalPtr			m_MapGoal;

		GoalState			m_GoalState;
		Vector3f			m_TargetPosition;
		GameEntity			m_ExplosiveEntity;
		Vector3f			m_ExplosivePosition;
		bool				m_AdjustedPosition : 1;

		State::StateStatus _UpdateDynamite();
		State::StateStatus _UpdateSatchel();
	};

	//////////////////////////////////////////////////////////////////////////

	class MountMg42 : public StateChild, public FollowPathUser, public AimerUser
	{
	public:

		void GetDebugString(StringStr &out);
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		MountMg42();
	private:
		MapGoalPtr			m_MapGoal;
		Trackers			Tracker;

		enum ScanType
		{
			SCAN_DEFAULT,
			SCAN_MIDDLE,
			SCAN_LEFT,
			SCAN_RIGHT,
			SCAN_ZONES,

			NUM_SCAN_TYPES
		};
		int					m_ScanDirection;
		int					m_NextScanTime;

		TrackTargetZone		m_TargetZone;

		//////////////////////////////////////////////////////////////////////////
		Vector3f			m_MG42Position;

		Vector3f			m_AimPoint;

		Vector3f			m_GunCenterArc;
		Vector3f			m_CurrentMountedAngles;
		
		Vector3f			m_ScanLeft;
		Vector3f			m_ScanRight;

		float				m_MinHorizontalArc;
		float				m_MaxHorizontalArc;
		float				m_MinVerticalArc;
		float				m_MaxVerticalArc;

		bool				m_GotGunProperties;
		//////////////////////////////////////////////////////////////////////////

		bool				m_AdjustedPosition;

		bool				_GetMG42Properties();
	};

	//////////////////////////////////////////////////////////////////////////

	class TakeCheckPoint : public StateChild, public FollowPathUser
	{
	public:
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		TakeCheckPoint();
	private:
		Trackers		Tracker;

		MapGoalPtr		m_MapGoal;
		Vector3f		m_TargetPosition;
	};

	//////////////////////////////////////////////////////////////////////////

	class PlantMine : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		PlantMine();
	private:

		Trackers			Tracker;
		MapGoalPtr			m_MapGoal;

		Vector3f			m_TargetPosition;
		GameEntity			m_LandMineEntity;
		Vector3f			m_LandMinePosition;
		Vector3f			m_LandMineVelocity;
	};

	//////////////////////////////////////////////////////////////////////////

	class MobileMg42 : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		MobileMg42();
	private:

		Trackers			Tracker;
		MapGoalPtr			m_MapGoal;

	};

	//////////////////////////////////////////////////////////////////////////

	class ReviveTeammate : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		typedef enum
		{
			REVIVING,
			HEALING,
		} GoalState;

		void GetDebugString(StringStr &out);
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		ReviveTeammate();
	private:
		GoalState			m_GoalState;

		Trackers			Tracker;
		MapGoalPtr			m_MapGoal;
	};

	//////////////////////////////////////////////////////////////////////////

	class DefuseDynamite : public StateChild, public FollowPathUser, public AimerUser
	{
	public:

		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		DefuseDynamite();
	private:
		Vector3f			m_TargetPosition;

		Trackers			Tracker;
		MapGoalPtr			m_MapGoal;
	};
};

#endif
