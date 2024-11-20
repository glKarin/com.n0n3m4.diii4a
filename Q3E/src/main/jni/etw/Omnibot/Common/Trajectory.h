#ifndef __TRAJECTORY_H__
#define __TRAJECTORY_H__

// namespace: Trajectory
//		This class simulates a trajectory of a projectile
namespace Trajectory
{
	struct AimTrajectory
	{
		Vector3f			m_AimVector;	// vector to aim at to accomplish this trajectory.
		float				m_Angle;		// angle in degrees in the range [-180, 180]
		float				m_Time;			// time it takes before the projectile arrives
	};

	int Calculate( const Vector3f &start, const Vector3f &end, float speed, float gravity, AimTrajectory bal[2] );
	float MaxHeightForTrajectory( const Vector3f &start, float zVel, float gravity );
	float HeightForTrajectory( const Vector3f &start, float zVel, float gravity, float t );

	struct TrajectorySim
	{
		Vector3f		m_Position;
		Vector3f		m_Velocity;

		float			m_Interval;
		float			m_PhysInterval;
		float			m_Duration;

		float			m_BounceLoss;
		float			m_GravityMultiplier;

		bool			m_TraceBounce;
		bool			m_StopAtHit;

		//////////////////////////////////////////////////////////////////////////
		Vector3f		m_StopPos;

		int FromTable(gmThread *a_thread, gmTableObject *a_table);

		void Render(obColor _color, float _duration);

		TrajectorySim();
	};
}

#endif
