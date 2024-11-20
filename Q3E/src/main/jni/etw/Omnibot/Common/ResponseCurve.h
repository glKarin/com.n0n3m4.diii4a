#ifndef __RESPONSECURVE_H__
#define __RESPONSECURVE_H__

// class: ResponseCurve
class ResponseCurve
{
public:

	bool LoadCurve();
	bool SaveCurve();

	float CalculateValue(float _value);

	typedef struct Point_t
	{
		float x, y;
	} Point;

	ResponseCurve();
	virtual ~ResponseCurve();
protected:
	typedef std::vector<Point> PointList;
	PointList		m_PointList;
};

#endif
