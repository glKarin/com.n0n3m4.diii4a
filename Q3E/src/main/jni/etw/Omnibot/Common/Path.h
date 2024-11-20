#ifndef __PATH_H__
#define __PATH_H__

// class: Path
//		This class represents a traversable path through the environment.
class Path
{
public:
	struct PathPoint
	{
		Vector3f			m_Pt;
		float				m_Radius;
		NavFlags			m_NavFlags;

		int					m_NavId;
		obuint32			m_OnPathThrough;
		obuint32			m_OnPathThroughParam;

		PathPoint &Position(const Vector3f &aPos) { m_Pt = aPos; return *this;  }
		PathPoint &Radius(float aRadius) { m_Radius = aRadius; return *this;  }
		PathPoint &Flags(NavFlags aNavFlags) { m_NavFlags = aNavFlags; return *this;  }
		PathPoint &NavId(int aNavId) { m_NavId = aNavId; return *this; }
		PathPoint &OnPathThrough(obuint32 aPathThrough) { m_OnPathThrough = aPathThrough; return *this;  }
		PathPoint &OnPathThroughParam(obuint32 aPathThroughParam) { m_OnPathThroughParam = aPathThroughParam; return *this;  }

		PathPoint()
			: m_Pt(Vector3f::ZERO)
			, m_Radius(0.f)
			, m_NavFlags(0)
			, m_NavId(0)
			, m_OnPathThrough(0)
			, m_OnPathThroughParam(0)
		{
		}
	};

	struct PathLink
	{
		float m_Distance;
		PathLink() : m_Distance(0.f) {}
	};

	void Clear();

	Path::PathPoint &AddPt(const Vector3f &_pt, float _radius);
	bool NextPt();

	void RemoveLastPt();

	Vector3f FindNearestPtOnPath(const Vector3f &_position, Vector3f *_outLookAhead = NULL, float _lookAheadDistance = 0.f);

	bool GetCurrentPt(PathPoint &_pt) const;
	bool GetNextPt(PathPoint &_pt) const;
	bool GetPreviousPt(PathPoint &_pt) const;
	bool GetFirstPt(PathPoint &_pt) const;
	bool GetLastPt(PathPoint &_pt) const;
	bool GetPt(int _index, PathPoint &_pt) const;
	const PathPoint *GetPt(int _index) const { return &m_Pts[_index]; }

	inline int GetNumPts() const { return m_NumPts; }
	inline int GetCurrentPtIndex() const { return m_CurrentPt; }
	inline bool IsEndOfPath() const { return m_CurrentPt >= m_NumPts-1; }

	float GetTotalLength() const;

	void DebugRender(obColor _color = COLOR::GREEN, float _duration = 5.f);

	Path();
private:
	static const int MAX_PATH_PTS = 512;
	PathPoint	m_Pts[MAX_PATH_PTS];
	PathLink	m_Links[MAX_PATH_PTS]; // store total distance at the end [MAX_PATH_PTS-1]

	// state info
	int		m_CurrentPt;
	int		m_NumPts;
};

#endif
