#ifndef __PIDCONTROLLER_H__
#define __PIDCONTROLLER_H__

class PIDController
{
public:
	PIDController( float kp, float ki, float kd )
		: m_P( kp ), m_I( ki ), m_D( kd )
	{
		m_E0 = 0.f;
		m_E1 = 0.f;
		m_E2 = 0.f;
		m_ControlValue = 0.f;
	}
	

	void Update( float _target, float _current, float _dt )
	{
		if ( _dt <= 0.f )
			return;

		m_E2 = m_E1;
		m_E1 = m_E0;
		m_E0 = _target - _current;
		float const    e01    = m_E0 - m_E1;
		float const    e12    = m_E1 - m_E2;
		float const    p    = m_P * e01;
		float const    i    = m_I * m_E0 * _dt;
		float const    d    = m_D * ( e01 - e12 ) / _dt;
		m_ControlValue += p + i + d;
	}

	float GetP() const { return m_P; }
	float GetI() const { return m_I; }
	float GetD() const { return m_D; }

	void SetP(float _p) { m_P = _p; }
	void SetI(float _i) { m_I = _i; }
	void SetD(float _d) { m_D = _d; }

	void Reset()
	{
		m_E0 = 0.f;
		m_E1 = 0.f;
		m_E2 = 0.f;
		m_ControlValue = 0.f;
	}

	float GetControlValue() const { return m_ControlValue; }
private:
	float    m_ControlValue;
	float    m_P, m_I, m_D;
	float    m_E0, m_E1, m_E2; // errors
};

#endif
