#include "PrecompCommon.h"
#include "Path.h"

Path::Path() :
	m_CurrentPt		(0),
	m_NumPts		(0)
{
}

void Path::Clear()
{
	m_CurrentPt = 0;
	m_NumPts = 0;
}

Path::PathPoint &Path::AddPt(const Vector3f &_pt, float _radius)
{
	static PathPoint s_pp;

	if(m_NumPts < MAX_PATH_PTS-1)
	{
		m_Pts[m_NumPts].m_Pt = _pt;
		m_Pts[m_NumPts].m_Radius = _radius;
		m_Pts[m_NumPts].m_NavFlags = 0;
		m_Pts[m_NumPts].m_OnPathThrough = 0;

		// If this isn't the first node in the path, update the cached distances.
		if(m_NumPts > 0)
		{
			m_Links[m_NumPts].m_Distance =
				m_Links[m_NumPts-1].m_Distance + (m_Pts[m_NumPts].m_Pt - m_Pts[m_NumPts-1].m_Pt).Length();
		}
		else
		{
			m_Links[0].m_Distance = 0.f;
		}
		return m_Pts[m_NumPts++];
	}

	return s_pp;
}

bool Path::NextPt()
{
	if(m_CurrentPt < m_NumPts-1)
	{
		m_CurrentPt++;
		return true;
	}
	else
	{
		return false;
	}
}

void Path::RemoveLastPt()
{
	if(m_NumPts > 0)
	{
		m_NumPts--;
		if(m_CurrentPt == m_NumPts && m_NumPts > 0) m_CurrentPt--;
	}
}

bool Path::GetCurrentPt(PathPoint &_pt) const 
{
	if(m_NumPts > 0) 
	{ 
		_pt = m_Pts[m_CurrentPt];
		return true;
	}
	return false;
}

bool Path::GetNextPt(PathPoint &_pt) const
{
	if(m_NumPts > 0 && m_CurrentPt+1 < m_NumPts) 
	{ 
		_pt = m_Pts[m_CurrentPt+1];
		return true;
	}
	return false;
}

bool Path::GetPreviousPt(PathPoint &_pt) const 
{
	if(m_NumPts > 0 && m_CurrentPt > 0) 
	{ 
		_pt = m_Pts[m_CurrentPt-1];
		return true;
	}
	return false;
}

bool Path::GetFirstPt(PathPoint &_pt) const 
{
	if(m_NumPts > 0) 
	{ 
		_pt = m_Pts[0];
		return true;
	}
	return false;
}

bool Path::GetLastPt(PathPoint &_pt) const 
{
	if(m_NumPts > 0) 
	{ 
		_pt = m_Pts[m_NumPts-1];
		return true;
	}
	return false;
}

bool Path::GetPt(int _index, PathPoint &_pt) const
{
	if(_index >= m_NumPts || _index > MAX_PATH_PTS-1)
		return GetLastPt(_pt);
	if(_index < 0)
		return GetFirstPt(_pt);
	_pt = m_Pts[_index];
	return true;
}

float Path::GetTotalLength() const
{
	return m_Links[m_NumPts-1].m_Distance;
}

void Path::DebugRender(obColor _color, float _duration)
{
	for(obint32 i = 1; i < m_NumPts; ++i)
	{
		Utils::DrawLine(m_Pts[i-1].m_Pt, m_Pts[i].m_Pt, _color, _duration);
	}
}

Vector3f Path::FindNearestPtOnPath(const Vector3f &_position, Vector3f *_outLookAhead /*= NULL*/, float _lookAheadDistance/* = 0.f*/)
{
	Vector3f vClosestPt = _position;
	if(_outLookAhead)
		*_outLookAhead = _position;

	if(/*m_CurrentPt <= 0 || */m_NumPts == 1)
	{
		vClosestPt = m_Pts[0].m_Pt;
		if(_outLookAhead)
			*_outLookAhead = vClosestPt;
	}
	else
	{
		float fClosestRatioOnSeg = 0.f;
		Vector3f vPtOnSeg;

		float fClosestDist = Utils::FloatMax;
		int iStartPt = 0, iEndPt = 0;
		int iStart = MaxT(m_CurrentPt-1,0);
		int iEnd = MinT(iStart + 2, m_NumPts-1);
		for(int i = iStart; i < iEnd; ++i)
		{
			float fRatioOnSeg;
			float fDist = PointToSegmentDistance(_position, m_Pts[i].m_Pt, m_Pts[i+1].m_Pt, vPtOnSeg, fRatioOnSeg);
			if(fDist < fClosestDist)
			{
				fClosestDist = fDist;
				vClosestPt = vPtOnSeg;
				fClosestRatioOnSeg = fRatioOnSeg;
				iStartPt = i;
				iEndPt = i+1;
			}
		}

		// calculate lookahead point
		if(_outLookAhead && _lookAheadDistance > 0.f && (iStartPt != 0 || iEndPt != 0))
		{
			const float fCurrentDistanceOnPath = m_Links[iStartPt].m_Distance + 
				(m_Links[iEndPt].m_Distance - m_Links[iStartPt].m_Distance) * fClosestRatioOnSeg;

			const float fDesiredDistanceOnPath = fCurrentDistanceOnPath + _lookAheadDistance;
			
			bool bFound = false;
			for(int i = iStartPt; i < m_NumPts-1; ++i)
			{
				if(m_Links[i+1].m_Distance >= fDesiredDistanceOnPath)
				{
					float fDistanceLeft = fDesiredDistanceOnPath - m_Links[i].m_Distance;

					const float fT = fDistanceLeft / (m_Links[i+1].m_Distance - m_Links[i].m_Distance);
					OBASSERT(fT >= 0.f && fT <= 1.f, "Bad Ratio");
					*_outLookAhead = Interpolate(
						m_Pts[i].m_Pt, 
						m_Pts[i+1].m_Pt, 
						ClampT(fT, 0.f, 1.f)); 
					bFound = true;
					break;
				}
			}
			if(!bFound)
				*_outLookAhead = m_Pts[m_NumPts-1].m_Pt; // last point
		}
	}
	return vClosestPt;
}
