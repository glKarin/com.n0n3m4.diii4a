#ifndef __BOTSTEERINGSYSTEM_H__
#define __BOTSTEERINGSYSTEM_H__

class Client;

namespace AiState
{
	// class: SteeringSystem
	//		Handles steering of the bot based on avoidance, seeking,...
	class SteeringSystem : public StateChild
	{
	public:
		
		typedef enum 
		{
			Normal,
			Arrive,
		} MoveType;

		// function: Update
		//		Allows the steering system to process steering effectors
		//		and calculate the <m_vMoveVec>
		void UpdateSteering();

		// function: SetTarget
		//		Sets <m_TargetVector>, which is the goal point the bot 
		//		should be heading for.
		bool SetTarget(const Vector3f &_pos, float _radius = 32.f, MoveMode _movemode = Run, bool _in3d = false);

		const Vector3f &GetTarget() const { return m_TargetVector; }
		float GetTargetRadius() const { return m_TargetRadius; }

		bool InTargetRadius() const;

		// function: GetMoveVector
		//		Returns the <m_vMoveVec> for the direction the bot wants to move
		//		This should be called after Update()
		Vector3f GetMoveVector(MoveType _movetype = Normal);

		// function: SetNoAvoidTime
		//		Set's a time for disabling avoid. Normally goals should just set it for
		//		the time in the current frame. But it may sometimes be useful to set it longer
		void SetNoAvoidTime(int _time);

		// function: SetEnableMovement
		//		Enables/Disables this bots movement.
		inline void SetEnableMovement(bool _b)			{ m_bMoveEnabled = _b; }

		// State Functions
		void Enter();
		StateStatus Update(float fDt);

		void RenderDebug();

		SteeringSystem();
		virtual ~SteeringSystem();
	protected:
		// var: m_TargetVector
		//		The current position we are suppose to be going for
		Vector3f	m_TargetVector;
		float		m_TargetRadius;
		MoveMode	m_MoveMode;

		float		m_DistanceToTarget;

		// var: m_vMoveVec
		//		The latest calculated movement vector
		Vector3f	m_MoveVec;
		// var: m_Obstacles
		//		A list of entities we should try to avoid.
		MemoryRecords	m_Obstacles;
		EntityList		m_NearbyPlayers;
		// var: m_NoAvoidTime
		//		A time to not calculate avoidance. If > IGame::GetTime() avoidance is not processed.
		int			m_NoAvoidTime;

		MoveType	m_MoveType;

		// var: m_NoAvoidTime
		//		A time to not calculate avoidance. If > IGame::GetTime() avoidance is not processed.
		bool		m_bMoveEnabled;

		bool		m_TargetVector3d;

		enum Deceleration{slow = 3, normal = 2, fast = 1};
		float _Arrive(const Vector3f &_targetPos, Deceleration _deceleration);
	};
}
#endif
