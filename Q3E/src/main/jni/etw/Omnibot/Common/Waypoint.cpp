#include "PrecompCommon.h"
#include "Waypoint.h"
#include "NavigationFlags.h"

extern float g_fTopWaypointOffset;
extern float g_fBottomWaypointOffset;
extern float g_fTopPathOffset;
extern float g_fBottomPathOffset;
extern float g_fBlockablePathOffset;
extern float g_fFacingOffset;
extern float g_fWaypointTextOffset;
extern float g_fWaypointTextDuration;

obuint32 Waypoint::m_NextUID = 1;
bool g_DrawLines = false;

Waypoint::Waypoint()
{
	Reset();
}

Waypoint::Waypoint(const Vector3f &_pos, float _radius, const Vector3f &_face)
{
	Reset();
	m_Position = _pos;
	m_Radius = _radius;
	m_Facing = _face;
}

Waypoint::~Waypoint()
{	
}

void Waypoint::Reset()
{
	m_Radius = 100.0f;
	m_NavigationFlags = 0;
	m_ConnectionFlags = 0;
	m_Position = Vector3f::ZERO;
	m_Facing = Vector3f::ZERO;
	m_UID = 0;
	m_HeuristicCost = 0.0f;
	m_GivenCost = 0.0f;
	m_FinalCost = 0.0f;
	m_Parent = 0;
	m_Mark = 0;
	m_GoalIndex = 0;
	m_Locked = false;
	m_PathSerial = 0;

	m_OnPathThrough = 0;
	m_OnPathThroughParam = 0;
}

#ifdef ENABLE_REMOTE_DEBUGGING
void Waypoint::Sync( RemoteLib::DataBuffer & db, bool fullSync ) {
	if ( fullSync ) {
		snapShot.Clear();
	}

	//{
	//	RemoteLib::DataBufferStatic<2048> localBuffer;
	//	localBuffer.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );

	//	WaypointSnapShot newSnapShot = snapShot;

	//	newSnapShot.Sync( "name", GetName().c_str(), localBuffer );
	//	newSnapShot.Sync( "uid", GetUID(), localBuffer );
	//	newSnapShot.Sync( "x", GetPosition().x, localBuffer );
	//	newSnapShot.Sync( "y", GetPosition().y, localBuffer );
	//	newSnapShot.Sync( "z", GetPosition().z, localBuffer );
	//	newSnapShot.Sync( "offsetTop", g_fTopWaypointOffset, localBuffer );
	//	newSnapShot.Sync( "offsetBottom", g_fBottomWaypointOffset, localBuffer );

	//	newSnapShot.Sync( "radius", GetRadius(), localBuffer );
	//	//newSnapShot.Sync( "color", GetRadius(), localBuffer );

	//	if ( m_Entity.IsValid() ) { // optional
	//		newSnapShot.Sync( "entityid", m_Entity.AsInt(), localBuffer );
	//	}

	//	if ( !m_Facing.IsZero() ) { // optional
	//		const float heading = Mathf::RadToDeg( m_Facing.XYHeading() );
	//		newSnapShot.Sync( "yaw", -Mathf::RadToDeg( heading ), localBuffer );
	//	}

	//	const uint32 writeErrors = localBuffer.endWrite();
	//	assert( writeErrors == 0 );

	//	if ( localBuffer.getBytesWritten() > 0 && writeErrors == 0 ) {
	//		db.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
	//		db.startSizeHeader();
	//		db.writeInt32( RemoteLib::ID_qmlComponent );
	//		db.writeInt32( GetUID() );
	//		db.writeSmallString( "waypoint" );
	//		db.append( localBuffer );
	//		db.endSizeHeader();

	//		if ( db.endWrite() == 0 ) {
	//			// mark the stuff we synced as done so we don't keep spamming it
	//			snapShot = newSnapShot;
	//		}
	//	}
	//}

	//////////////////////////////////////////////////////////////////////////

	Waypoint::ConnectionList::iterator it = m_Connections.begin();
	for ( int index = 0; it != m_Connections.end(); ++it, ++index )
	{
		ConnectionInfo & ci = (*it);

		obColor linkColor = COLOR::YELLOW;
		if(IsAnyFlagOn(PathPlannerWaypoint::m_BlockableMask)) {
			if(ci.m_Connection->IsAnyFlagOn(PathPlannerWaypoint::m_BlockableMask)) {
				if ( ci.m_ConnectionFlags & F_LNK_CLOSED ) {
					linkColor = COLOR::RED;
				} else {
					linkColor = COLOR::GREEN;
				}
			}
		}

		if ( fullSync ) {
			ci.snapShot.Clear();
		}

		ConnectionInfo::ConnectionSnapShot newSnapShot = ci.snapShot;

		RemoteLib::DataBufferStatic<2048> localBuffer;
		localBuffer.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
		newSnapShot.Sync( "fromx", GetPosition().x, localBuffer );
		newSnapShot.Sync( "fromy", GetPosition().y, localBuffer );
		newSnapShot.Sync( "fromz", GetPosition().z + g_fTopPathOffset, localBuffer );
		newSnapShot.Sync( "tox", ci.m_Connection->GetPosition().x, localBuffer );
		newSnapShot.Sync( "toy", ci.m_Connection->GetPosition().y, localBuffer );
		newSnapShot.Sync( "toz", ci.m_Connection->GetPosition().z + g_fBottomPathOffset, localBuffer );
		newSnapShot.Sync( "color", linkColor.rgba(), localBuffer );
		
		const uint32 writeErrors = localBuffer.endWrite();
		assert( writeErrors == 0 );

		if ( localBuffer.getBytesWritten() > 0 && writeErrors == 0 ) {
			db.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
			db.startSizeHeader();
			db.writeInt32( RemoteLib::ID_qmlComponent );
			db.writeInt32( GetUID() | ( index<<30 ) );
			db.writeSmallString( "connection" );
			db.append( localBuffer );
			db.endSizeHeader();

			if ( db.endWrite() == 0 ) {
				// mark the stuff we synced as done so we don't keep spamming it
				ci.snapShot = newSnapShot;
			}
		}
	}
}
#endif

Segment3f Waypoint::GetSegment()
{
	return Segment3f(GetPosition().AddZ(g_fBottomWaypointOffset), Vector3f::UNIT_Z, g_fTopWaypointOffset-g_fBottomWaypointOffset);
}

void Waypoint::AssignNewUID()
{
	m_UID = m_NextUID++;
}

const Vector3f &Waypoint::GetFacing() const 
{ 
	return m_Facing; 
}

bool Waypoint::IsConnectedTo(const Waypoint *_wp) const
{
	ConnectionList::const_iterator cIt = m_Connections.begin(), cItEnd = m_Connections.end();
	for(; cIt != cItEnd; ++cIt)
	{
		if((*cIt).m_Connection == _wp)
			return true;
	}
	return false;
}

bool Waypoint::ConnectTo(Waypoint *_wp, obuint32 _flags)
{
	if(_wp && _wp != this && !IsConnectedTo(_wp))
	{
		Waypoint::ConnectionInfo info;
		info.m_Connection = _wp;
		info.m_ConnectionFlags = _flags;
		m_Connections.push_back(info);
		return true;
	}
	return false;
}

void Waypoint::DisconnectFrom(const Waypoint *_wp)
{
	ConnectionList::iterator it = m_Connections.begin(), itEnd = m_Connections.end();
	for(; it != itEnd; )
	{
		if((*it).m_Connection == _wp) {
			m_Connections.erase(it++);
		} else
			++it;
	}
}

void Waypoint::PostLoad()
{
	m_OnPathThrough = 0;
	m_OnPathThroughParam = 0;

	String s = GetPropertyMap().GetProperty("paththrough");
	if(s.size()>1)
	{
		StringVector sv;
		Utils::Tokenize(s," :",sv);
		if(sv.size()>1)
			m_OnPathThroughParam = Utils::MakeHash32(sv[1]); 
		if(!sv.empty())
			m_OnPathThrough = Utils::MakeHash32(sv[0]);
	}
}
