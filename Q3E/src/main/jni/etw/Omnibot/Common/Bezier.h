#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "Wm3Vector2.h"
#include "Wm3Vector3.h"

namespace Wm3
{

// class: Bezier2
//		2d bezier curve class
template <class Real>
class Bezier2
{
public:
	Bezier2() {}
	Bezier2(const Vector2<Real>& _1, const Vector2<Real>& _2, const Vector2<Real>& _3, const Vector2<Real>& _4) : 
		m_P1(_1),
		m_P2(_2),
		m_P3(_3),
		m_P4(_4)
	{
	}

	const Vector2<Real> &GetStartPosition() const  { return m_P1; }
	const Vector2<Real> &GetEndPosition() const    { return m_P4; }
	const Vector2<Real> &GetStartControl() const   { return m_P2; }
	const Vector2<Real> &GetEndControl() const     { return m_P3; }

	void TranslatePoints(const Vector2f &_vec)
	{
		m_P1 += _vec;
		m_P2 += _vec;
		m_P3 += _vec;
		m_P4 += _vec;
	}

	Vector2<Real> GetPoint(float _t) const
	{
		Real fA = (1.0-_t)*(1.0-_t)*(1.0-_t);
		Real fB = 3*(1.0-_t)*(1.0-_t)*_t;
		Real fC = 3*(1.0-_t)*_t*_t;
		Real fD = _t*_t*_t;
		return Vector2<Real>(fA*m_P1.x + fB*m_P2.x + fC*m_P3.x + fD*m_P4.x,
			fA*m_P1.y + fB*m_P2.y + fC*m_P3.y + fD*m_P4.y);
	}

private:
	Vector2<Real> m_P1, m_P2, m_P3, m_P4;
};

// class: Bezier3
//		3d bezier curve class
template <class Real>
class Bezier3
{
public:
	Bezier3() {}
	Bezier3(const Vector3<Real>& _1, const Vector3<Real>& _2, const Vector3<Real>& _3, const Vector3<Real>& _4) : 
		m_P1(_1),
		m_P2(_2),
		m_P3(_3),
		m_P4(_4)
	{
	}

	const Vector3<Real> &GetStartPosition() const    { return m_P1; }
	const Vector3<Real> &GetEndPosition() const    { return m_P4; }
	const Vector3<Real> &GetStartControl() const    { return m_P2; }
	const Vector3<Real> &GetEndControl() const        { return m_P3; }

	void TranslatePoints(const Vector3f &_vec)
	{
		m_P1 += _vec;
		m_P2 += _vec;
		m_P3 += _vec;
		m_P4 += _vec;
	}

	Vector3<Real> GetPoint(float _t) const
	{
		Real fA = (1.0-_t)*(1.0-_t)*(1.0-_t);
		Real fB = 3*(1.0-_t)*(1.0-_t)*_t;
		Real fC = 3*(1.0-_t)*_t*_t;
		Real fD = _t*_t*_t;
		return Vector3<Real>(fA*m_P1.x  + fB*m_P2.x + fC*m_P3.x + fD*m_P4.x,
			fA*m_P1.y + fB*m_P2.y + fC*m_P3.y + fD*m_P4.y,
			fA*m_P1.z + fB*m_P2.z + fC*m_P3.z + fD*m_P4.z);
	}

private:
	Vector3<Real> m_P1, m_P2, m_P3, m_P4;
};

}

typedef Wm3::Bezier2<float> Bezier2f;
typedef Wm3::Bezier2<double> Bezier2d;

typedef Wm3::Bezier3<float> Bezier3f;
typedef Wm3::Bezier3<double> Bezier3d;


#endif
