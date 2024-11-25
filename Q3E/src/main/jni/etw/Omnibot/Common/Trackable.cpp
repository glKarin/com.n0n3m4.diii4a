#include "PrecompCommon.h"
#include "Trackable.h"

Trackable::Trackable()
{
}

Trackable::~Trackable()
{
}

void Trackable::AddReference(obuint32 _type)
{
	_CheckIndex(_type);
	++m_TrackList[_type];
}

void Trackable::DelReference(obuint32 _type)
{
	_CheckIndex(_type);	
	--m_TrackList[_type];
	OBASSERT(m_TrackList[_type] >= 0, "Counter got below 0!");
}

obuint32 Trackable::GetRefCount(obuint32 _type)
{
	_CheckIndex(_type);	
	return m_TrackList[_type];
}

void Trackable::_CheckIndex(obuint32 _type)
{
	if(_type >= m_TrackList.size())
	{
		m_TrackList.resize(_type+1, 0);
	}
}

bool Trackable::IsReferenced()
{
	for(obuint32 i = 0; i < m_TrackList.size(); ++i)
	{
		if(m_TrackList[i] != 0)
		{
			return true;
		}
	}
	return false;
}
