#include "PrecompCommon.h"
#include "Trajectory.h"

namespace Trajectory
{
	int Calculate( const Vector3f &_start, const Vector3f &_end, float _speed, float _gravity, AimTrajectory _bal[2] ) 
	{
		int n, i;
		float x, y, a, b, c, d, sqrtd, inva, p[2];

		x = ( Vector2f(_end) - Vector2f(_start) ).Length();
		y = _end[2] - _start[2];

		a = 4.0f * y * y + 4.0f * x * x;
		b = -4.0f * _speed * _speed - 4.0f * y * _gravity;
		c = _gravity * _gravity;

		d = b * b - 4.0f * a * c;
		if ( d <= 0.0f || a == 0.0f ) {
			return 0;
		}
		sqrtd = Mathf::Sqrt( d );
		inva = 0.5f / a;
		p[0] = ( - b + sqrtd ) * inva;
		p[1] = ( - b - sqrtd ) * inva;
		n = 0;
		for ( i = 0; i < 2; i++ ) 
		{
			if ( p[i] <= 0.0f )
				continue;

			d = Mathf::Sqrt( p[i] );
			_bal[n].m_Angle = Mathf::ATan2( 0.5f * ( 2.0f * y * p[i] - _gravity ) / d, d * x );
			_bal[n].m_Time = x / ( Mathf::Cos( _bal[n].m_Angle ) * _speed );
			//bal[n].angle = Mathf::AngleNormalize180( Mathf::RadToDeg( bal[n].angle ) );

			// Calculate the Aim vector for this result.
			Vector3f vTargetPos = _end;
			Vector3f vAimDir = vTargetPos - _start;
			vTargetPos.z = _start.z + Mathf::Tan(_bal[n].m_Angle) * Vector2f(vAimDir).Length();
			_bal[n].m_AimVector = vTargetPos - _start;
			_bal[n].m_AimVector.Normalize();

			n++;
		}

		return n;
	}

	float MaxHeightForTrajectory( const Vector3f &start, float zVel, float gravity ) 
	{
		float t = zVel / gravity;
		return start.z - 0.5f * gravity * ( t * t );
	}

	float HeightForTrajectory( const Vector3f &start, float zVel, float gravity, float t ) 
	{
		return start.z - 0.5f * gravity * ( t * t );
	}

	//////////////////////////////////////////////////////////////////////////

	TrajectorySim::TrajectorySim()
		: m_Position(0.f,0.f,0.f)
		, m_Velocity(0.f,0.f,0.f)
		, m_Interval(0.05f)
		, m_PhysInterval(0.05f)
		, m_Duration(5.f)
		, m_BounceLoss(0.f)
		, m_GravityMultiplier(1.f)
		, m_TraceBounce(true)
		, m_StopAtHit(false)
		, m_StopPos(0.f,0.f,0.f)
	{
	}

	int TrajectorySim::FromTable(gmThread *a_thread, gmTableObject *a_table)
	{
		gmVariable v0 = a_table->Get(a_thread->GetMachine(),"Position");
		if(!v0.GetVector(m_Position)) 
		{
			GM_EXCEPTION_MSG("expected Position field in table");
			return GM_EXCEPTION;
		}
		gmVariable v1 = a_table->Get(a_thread->GetMachine(),"Velocity");
		if(!v1.GetVector(m_Velocity)) 
		{
			GM_EXCEPTION_MSG("expected Velocity field in table");
			return GM_EXCEPTION;
		}
		gmVariable vInterval = a_table->Get(a_thread->GetMachine(),"Interval");
		if(!vInterval.IsNull() && !vInterval.GetFloatSafe(m_Interval,m_Interval))
		{
			GM_EXCEPTION_MSG("expected Interval field as float or int");
			return GM_EXCEPTION;
		}
		gmVariable vPhysInterval = a_table->Get(a_thread->GetMachine(),"PhysInterval");
		if(!vPhysInterval.IsNull() && !vPhysInterval.GetFloatSafe(m_PhysInterval,m_PhysInterval))
		{
			GM_EXCEPTION_MSG("expected PhysInterval field as float or int");
			return GM_EXCEPTION;
		}

		gmVariable vDuration = a_table->Get(a_thread->GetMachine(),"Duration");
		if(!vDuration.IsNull() && !vDuration.GetFloatSafe(m_Duration,m_Duration))
		{
			GM_EXCEPTION_MSG("expected Duration field as float or int");
			return GM_EXCEPTION;
		}

		gmVariable vBounceLoss = a_table->Get(a_thread->GetMachine(),"BounceLoss");
		if(!vBounceLoss.IsNull() && !vBounceLoss.GetFloatSafe(m_BounceLoss,m_BounceLoss))
		{
			GM_EXCEPTION_MSG("expected BounceLoss field as float or int");
			return GM_EXCEPTION;
		}

		gmVariable vGravMult = a_table->Get(a_thread->GetMachine(),"GravityMultiplier");
		if(!vGravMult.IsNull() && !vGravMult.GetFloatSafe(m_GravityMultiplier,m_GravityMultiplier))
		{
			GM_EXCEPTION_MSG("expected GravityMultiplier field as float or int");
			return GM_EXCEPTION;
		}

		gmVariable vStopAtHit = a_table->Get(a_thread->GetMachine(),"StopAtHit");
		if(!vStopAtHit.IsNull() && !vStopAtHit.GetBoolSafe(m_StopAtHit,m_StopAtHit))
		{
			GM_EXCEPTION_MSG("expected StopAtHit field as true/false");
			return GM_EXCEPTION;
		}

		gmVariable vTraceBounce = a_table->Get(a_thread->GetMachine(),"TraceBounce");
		if(!vTraceBounce.IsNull() && !vTraceBounce.GetBoolSafe(m_TraceBounce,m_TraceBounce))
		{
			GM_EXCEPTION_MSG("expected TraceBounce field as true/false");
			return GM_EXCEPTION;
		}

		return GM_OK;
	}

	void TrajectorySim::Render(obColor _col, float _time)
	{
		float interval = Mathf::Max(m_PhysInterval,0.05f);

		obTraceResult tr;

		Vector3f vel = m_Velocity;
		Vector3f pos = m_Position;
		Vector3f vlast = m_Position;
		float tlast = 0;

		for(float t = interval; t <= m_Duration;)
		{
			bool Hit = false;
			if(m_TraceBounce && 
				EngineFuncs::TraceLine(tr,vlast,pos,0,TR_MASK_SOLID,Utils::GetLocalGameId(),False) && 
				tr.m_Fraction < 1.f)
			{				
				Hit = true;

				//Vector3f ray = pos - vlast;
				//const float length = ray.Normalize();

				// reflect the velocity
				vel = vel.Reflect(tr.m_Normal);
				vel *= m_BounceLoss;

				// adjust the position with the rest of the bounce
				float whatsleftT = (1.f - tr.m_Fraction);
				pos = Vector3f(tr.m_Endpos) + (vel * whatsleftT) * interval;					

				//t += interval * (interval * tr.m_Fraction);
			}
			else
				pos += (vel * interval); // move the entire interval if it didnt hit anything
		
			t += interval;			

			
			vel.z += (IGame::GetGravity() * m_GravityMultiplier) * interval;

			//////////////////////////////////////////////////////////////////////////

			if(t-tlast > m_Interval)
			{
				Utils::DrawLine(vlast,pos,_col,_time);
				vlast = pos;
				tlast = t;
			}

			m_StopPos = pos;

			if(m_StopAtHit && Hit)
				break;
		}
	}
}
