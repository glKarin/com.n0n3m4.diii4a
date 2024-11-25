#include "PrecompCommon.h"
#include "ResponseCurve.h"

ResponseCurve::ResponseCurve()
{
}

ResponseCurve::~ResponseCurve()
{
}

bool ResponseCurve::LoadCurve()
{

	return true;
}

bool ResponseCurve::SaveCurve()
{
	//gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

	//gmTableObject *pCurvesTable = pMachine->AllocTableObject();
	//pMachine->GetGlobals()->Set(pMachine, "Curves", gmVariable(pCurvesTable));

	//gmTableObject *pThisCurve = pMachine->AllocTableObject();
	////pCurvesTable->Set(pMachine, gmVariable(pMachine->AllocStringObject(strCurveName.c_str())), gmVariable(pThisCurve));

	//for(int i = 0; i < m_PointList.size(); ++i)
	//{
	//	gmTableObject *pCoords = pMachine->AllocTableObject();
	//	pCoords->Set(pMachine, "x",  gmVariable(m_PointList[i].x));
	//	pCoords->Set(pMachine, "y",  gmVariable(m_PointList[i].y));
	//	pThisCurve->Set(pMachine, i, gmVariable(pCoords));
	//}

	return true;
}

float ResponseCurve::CalculateValue(float _value)
{
	OBASSERT(!m_PointList.empty(), "No Points in Response Curve!");

	int iNumPoints = (int)m_PointList.size();

	// Check the extents.
	if(_value < m_PointList[0].x)
	{
		return m_PointList[0].y;
	}
	else if(_value > m_PointList[iNumPoints-1].x)
	{
		return m_PointList[iNumPoints-1].y;
	}
	else
	{
		int iStart = 0;
		for(int i = 1; i < iNumPoints; ++i)
		{
			if(_value < m_PointList[i].x)
			{
				float ratio = (_value -  m_PointList[iStart].x) / 
					(m_PointList[i].x - m_PointList[iStart].x) ;
				return (ratio * (m_PointList[i].y - m_PointList[iStart].y));
			}
			++iStart;
		}
	}
	return 0.0f;
}
