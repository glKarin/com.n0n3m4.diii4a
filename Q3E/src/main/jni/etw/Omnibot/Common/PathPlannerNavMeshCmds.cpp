#include "PrecompCommon.h"
#include <limits>

#include "PathPlannerNavMesh.h"
#include "ScriptManager.h"
#include "IGameManager.h"
#include "Waypoint.h"
#include "IGame.h"
#include "Client.h"

using namespace std;

const float SNAP_RADIUS = 8.f;

//////////////////////////////////////////////////////////////////////////

void PathPlannerNavMesh::InitCommands()
{
	PathPlannerBase::InitCommands();

	SetEx("nav_save", "Save current navigation to disk", 
		this, &PathPlannerNavMesh::cmdNavSave);
	SetEx("nav_load", "Load last saved navigation from disk", 
		this, &PathPlannerNavMesh::cmdNavLoad);
	SetEx("nav_view", "Turn on/off navmesh visibility.", 
		this, &PathPlannerNavMesh::cmdNavView);
	SetEx("nav_viewconnections", "Turn on/off navmesh connection visibility.", 
		this, &PathPlannerNavMesh::cmdNavViewConnections);
	
	//////////////////////////////////////////////////////////////////////////
	SetEx("nav_enablestep", "Enable step by step generation process.", 
		this, &PathPlannerNavMesh::cmdNavEnableStep);
	SetEx("nav_step", "Step to the next nav process.", 
		this, &PathPlannerNavMesh::cmdNavStep);
	//////////////////////////////////////////////////////////////////////////
	SetEx("nav_autofeature", "Automatically waypoints jump pads, teleporters, player spawns.", 
		this, &PathPlannerNavMesh::cmdAutoBuildFeatures);

	//////////////////////////////////////////////////////////////////////////
	SetEx("nav_startpoly", "Starts a sector polygon.", 
		this, &PathPlannerNavMesh::cmdStartPoly);
	SetEx("nav_undopoly", "Starts a sector polygon.", 
		this, &PathPlannerNavMesh::cmdUndoPoly);

	/*SetEx("nav_loadobj", "Loads navmesh from obj file.", 
		this, &PathPlannerNavMesh::cmdLoadObj);
	SetEx("nav_loadmap", "Loads navmesh from map file.", 
		this, &PathPlannerNavMesh::cmdLoadMap);*/

	SetEx("nav_pp", "Creates a plane from a point and normal.", 
		this, &PathPlannerNavMesh::cmdCreatePlanePoly);
	SetEx("nav_sp", "Creates a plane from a point and normal.", 
		this, &PathPlannerNavMesh::cmdCreateSlicePoly);
	SetEx("nav_ss", "Slices working sector with the sector being aimed at.", 
		this, &PathPlannerNavMesh::cmdCreateSliceSector);
	
	SetEx("nav_commit", "Creates a plane from a point and normal.", 
		this, &PathPlannerNavMesh::cmdCommitPoly);
	SetEx("nav_editsector", "Begins editing a sector so further slicing may be done.", 
		this, &PathPlannerNavMesh::cmdEditSector);	
	SetEx("nav_splitsector", "Splits a sector into 2 sectors along a plane.", 
		this, &PathPlannerNavMesh::cmdSplitSector);	
	SetEx("nav_groundsector", "Grounds all the vertices in the sector.", 
		this, &PathPlannerNavMesh::cmdGroundSector);		

	SetEx("nav_next", "Steps the current tool to the next operation.", 
		this, &PathPlannerNavMesh::cmdNext);
	
	SetEx("nav_delsector", "Deletes a sector by its id number.", 
		this, &PathPlannerNavMesh::cmdDeleteSector);

	SetEx("nav_mirrorsectors", "Mirrors all sectors across a specified axis.", 
		this, &PathPlannerNavMesh::cmdMirrorSectors);

	SetEx("sector_setproperty", "Sets a property of the current sector.", 
		this, &PathPlannerNavMesh::cmdSectorSetProperty);

	SetEx("sector_createconnections", "Sets a property of the current sector.", 
		this, &PathPlannerNavMesh::cmdSectorCreateConnections);

	SetEx("nav_setmapcenter", "Sets the map center.", 
		this, &PathPlannerNavMesh::cmdSetMapCenter);

	REGISTER_STATE(PathPlannerNavMesh,NoOp);
	REGISTER_STATE(PathPlannerNavMesh,PlaceSector);
	REGISTER_STATE(PathPlannerNavMesh,SliceSector);
	REGISTER_STATE(PathPlannerNavMesh,SliceSectorWithSector);	
	REGISTER_STATE(PathPlannerNavMesh,EditSector);
	REGISTER_STATE(PathPlannerNavMesh,SplitSector);	
	REGISTER_STATE(PathPlannerNavMesh,TraceSector);	
	REGISTER_STATE(PathPlannerNavMesh,GroundSector);		
	REGISTER_STATE(PathPlannerNavMesh,CommitSector);
	REGISTER_STATE(PathPlannerNavMesh,MirrorSectors);
	REGISTER_STATE(PathPlannerNavMesh,PlaceBorder);

	SetNextState(NoOp);
}

void PathPlannerNavMesh::cmdNavSave(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(Save(g_EngineFuncs->GetMapName()))
	{
		EngineFuncs::ConsoleMessage("Saved Nav.");
	}
	else
		EngineFuncs::ConsoleError("ERROR Saving Nav.");
}

void PathPlannerNavMesh::cmdNavLoad(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(Load(g_EngineFuncs->GetMapName()))
	{
		EngineFuncs::ConsoleMessage("Loaded Nav.");
	} 
	else
		EngineFuncs::ConsoleError("ERROR Loading Nav.");
}

void PathPlannerNavMesh::cmdNavView(const StringVector &_args)
{
	const char *strUsage[] = 
	{ 
		"nav_view enable[bool]",
		"> enable: Enable nav rendering. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);
	ScriptManager::GetInstance()->ExecuteStringLogged(
		(String)va("Nav.EnableView( %s );", 
		bEnable ? "true" : "false"));
}

void PathPlannerNavMesh::cmdNavViewConnections(const StringVector &_args)
{
	const char *strUsage[] = 
	{ 
		"nav_viewconnections enable[bool]",
		"> enable: Enable nav connection rendering. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);
	ScriptManager::GetInstance()->ExecuteStringLogged(
		(String)va("Nav.EnableViewConnection( %s );", 
		bEnable ? "true" : "false"));
}

void PathPlannerNavMesh::cmdNavEnableStep(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const char *strUsage[] = 
	{ 
		"nav_enablestep enable[bool]",
		"> enable: Enable step by step nav generation. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);
	ScriptManager::GetInstance()->ExecuteStringLogged(
		(String)va("Nav.EnableStep( %s );", 
		bEnable ? "true" : "false"));
}

void PathPlannerNavMesh::cmdNavStep(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;
	ScriptManager::GetInstance()->ExecuteStringLogged("Nav.Step();");
}

//////////////////////////////////////////////////////////////////////////

extern float g_fBottomWaypointOffset;

void PathPlannerNavMesh::cmdAutoBuildFeatures(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const int iMaxFeatures = 1024;
	AutoNavFeature features[iMaxFeatures];
	int iNumFeatures = g_EngineFuncs->GetAutoNavFeatures(features, iMaxFeatures);
	for(int i = 0; i < iNumFeatures; ++i)
	{
		const float fTime = 30.f;

		Vector3f vPos(features[i].m_Position);		
		Vector3f vFace(features[i].m_Facing);
		Vector3f vTarget(features[i].m_TargetPosition);

		// Adjust for waypoint offsets
		if(!features[i].m_Bounds.IsZero())
		{
			features[i].m_Bounds.CenterBottom(vPos);
			vPos.z -= g_fBottomWaypointOffset;
		}
		if(!features[i].m_TargetBounds.IsZero())
		{
			features[i].m_TargetBounds.CenterBottom(vTarget);
			vTarget.z -= g_fBottomWaypointOffset;
		}

		//AddFloodStart(vPos);
		//if(vPos != vTarget)
		//{
		//	AddFloodStart(vTarget);
		//	//pFeature->ConnectTo(pTarget);
		//}

		//////////////////////////////////////////////////////////////////////////
		Utils::DrawLine(vPos, vPos+Vector3f::UNIT_Z * 32.f, COLOR::GREEN, fTime);
		if(vPos != vTarget)
		{
			Utils::DrawLine(vPos, vTarget, COLOR::MAGENTA, fTime);
			Utils::DrawLine(vTarget, vTarget+Vector3f::UNIT_Z * 32.f, COLOR::RED, fTime);
		}
		Utils::OutlineAABB(features[i].m_Bounds, COLOR::GREEN, fTime);
		//////////////////////////////////////////////////////////////////////////
	}
	EngineFuncs::ConsoleMessage(va("Found %d nav features.", iNumFeatures));
}

Vector3f PathPlannerNavMesh::_SectorVertWithin(const Vector3f &_pos1, const Vector3f &_pos2, float _range, bool *_snapped)
{
	Vector3f r = _pos2;

	const float fRangeSq = Mathf::Sqr(_range);
	float fClosest = Utils::FloatMax;

	*_snapped = false;
	for(obuint32 s = 0; s < m_NavSectors.size(); ++s)
	{
		const NavSector &sec = m_NavSectors[s];

		for(obuint32 v = 0; v < sec.m_Boundary.size(); ++v)
		{
			float fLenSq = Utils::DistancePointToLineSqr(sec.m_Boundary[v],_pos1,_pos2);
			if(fLenSq < fRangeSq && fLenSq < fClosest)
			{
				r = sec.m_Boundary[v];
				fClosest = fLenSq;

				if(_snapped)
					*_snapped = true;
			}
		}
	}
	return r;
}

void PathPlannerNavMesh::cmdStartPoly(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	m_ToolCancelled = false;
	SetNextState(TraceSector);
}

void PathPlannerNavMesh::cmdUndoPoly(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(!m_WorkingManualSector.empty())
		m_WorkingManualSector.pop_back();

	if(m_WorkingManualSector.empty() && GetCurrentStateId()==TraceSector)
		SetNextState(NoOp);
}

//void PathPlannerNavMesh::cmdLoadObj(const StringVector &_args)
//{
//	String s = "J:/CVS/decompilers/" + String(g_EngineFuncs->GetMapName()) + ".obj";
//	String line;
//	char trash;
//
//	m_PolyIndexList.clear();
//
//	std::fstream f;
//	f.open(s.c_str(), std::ios_base::in);
//	if(f.is_open())
//	{
//		while(!f.eof())
//		{
//			std::getline(f, line);
//			if(line[0] == '#')
//				continue;
//			if(line.empty())
//				continue;
//
//			if(line.compare(0,1,"v",0,1) == 0)
//			{
//				Vector3f v;
//
//				StringStr st;
//				st << line;
//				st >> trash >> v;
//
//				m_Vertices.push_back(v);
//				continue;
//			}
//
//			if(line.compare(0,2,"vt",0,2) == 0)
//			{
//				// textures
//				continue;
//			}
//
//			if(line.compare(0,2,"vn",0,2) == 0)
//			{
//				// normals
//				continue;
//			}
//
//			if(line.compare(0,1,"f",0,1) == 0)
//			{
//				StringList tokens;
//				String buffer;
//				StringStr st;
//				st << line;
//				while(st >> buffer)
//					tokens.push_back(buffer);
//
//				int vindex, tindex, nindex;
//				IndexList il;
//
//				StringList::iterator it = tokens.begin(), itEnd = tokens.end();
//				for(; it != itEnd; ++it)
//				{
//					const String &token = (*it);
//					if(token == "f")
//						continue;
//
//					StringStr st;
//					st << token;
//
//					obuint32 c = std::count(token.begin(), token.end(), '/');
//					switch(c)
//					{
//					case 0:
//						st >> vindex;
//						il.push_back(vindex-1);
//						break;
//					case 1:
//						st >> vindex >> trash >> tindex;
//						il.push_back(vindex-1);
//						break;
//					case 3:
//						st >> vindex >> trash >> tindex >> trash >> nindex;
//						il.push_back(vindex-1);
//						break;
//					}
//				}
//
//				// Skip all with a bad normal.
//				obuint32 c = 0;
//				Vector3f vNormal = Vector3f::ZERO;
//				while(vNormal == Vector3f::ZERO)
//				{
//					Vector3f v1 = m_Vertices[il[c+1]] - m_Vertices[il[c]];
//					Vector3f v2 = m_Vertices[il[c+2]] - m_Vertices[il[c]];
//					vNormal = v1.UnitCross(v2);
//					++c;
//				}
//
//				if(Utils::AngleBetween(vNormal, Vector3f::UNIT_Z) > Mathf::DegToRad(45.f))
//					continue;
//
//				m_PolyIndexList.push_back(il);
//				continue;
//			}
//		}
//		f.close();
//	}
//}

//void PathPlannerNavMesh::cmdLoadMap(const StringVector &_args)
//{
//	//String s = "J:/CVS/decompilers/" + String(g_EngineFuncs->GetMapName()) + ".map";
//	String s = "J:/CVS/decompilers/q4ctf1.map";
//	
//	String line, tmp;
//	char trash;
//	int versionnum = 3;
//	//int bracket = 0;
//
//	m_NavSectors.clear();
//	ReleaseCollision();
//
//	enum MapMode
//	{
//		MAP_UNKNOWN,
//		MAP_PRIMITIVE,
//	};
//
//	MapMode m = MAP_UNKNOWN;
//
//	obuint32 iNumBrushes = 0;
//
//	std::fstream f;
//	f.open(s.c_str(), std::ios_base::in);
//	if(f.is_open())
//	{
//		while(!f.eof())
//		{
//			std::getline(f, line);
//			if(line[0] == '/' && line[1] == '/')
//				continue;
//			if(line.empty())
//				continue;
//
//			if(line.compare(0,7,"Version",0,7) == 0)
//			{
//				StringStr st;
//				st << line;
//				st >> tmp >> versionnum;
//				continue;
//			}
//
//			if(line[0]=='{' || line[0]=='}')
//				continue;
//
//			int c = 0;
//			while(line[c] == ' ') 
//				++c;
//
//			if(line.compare(0,11,"\"classname\"",0,11) == 0)
//			{
//				continue;
//			}
//
//			/*if(line[c]=='{')
//				bracket++;
//			if(line[c]=='}')
//				bracket--;*/
//
//			if(c != 0)
//				line.erase(0,c);
//
//			if(line.compare(0,8,"brushDef",0,8) == 0)
//			{
//				m = MAP_PRIMITIVE;
//				continue;
//			}
//
//			if(line[0]=='{')
//			{
//				switch(m)
//				{
//				case MAP_PRIMITIVE:
//					{
//						++iNumBrushes;
//
//						PlaneList planes;
//
//						while(line[0]!='}')
//						{
//							std::getline(f, line);
//
//							Plane3f pl;
//
//							StringStr st;
//							st << line;
//							st >> trash >> pl >> trash;
//							if(st.good())
//								planes.push_back(pl);
//						}
//						
//						// contruct polys from the list
//						obuint32 wp = 0;
//						bool bDidSomething = true;
//						while(bDidSomething)
//						{
//							m_CurrentSector.clear();
//							bDidSomething = false;
//
//							for(; wp < planes.size(); ++wp)
//							{
//								if(Utils::AngleBetween(planes[wp].Normal, Vector3f::UNIT_Z) > Mathf::DegToRad(45.f))
//									continue;
//
//								// build a poly from this walkface
//								bDidSomething = true;
//								m_CurrentSector = Utils::CreatePolygon(planes[wp].Normal*planes[wp].Constant, planes[wp].Normal);
//								break;
//							}
//
//							// Clip the polygon
//							if(!m_CurrentSector.empty())
//							{
//								for(obuint32 p = 0; p < planes.size(); ++p)
//								{
//									if(wp==p)
//										continue;
//
//									if(planes[p].Normal == planes[wp].Normal ||
//										planes[p].Normal == -planes[wp].Normal)
//										continue;
//
//									m_CurrentSector = Utils::ClipPolygonToPlanes(m_CurrentSector, planes[p], true);
//
//									if(m_CurrentSector.empty())
//										break;
//								}
//							}
//
//							// Commit the polygon
//							if(m_CurrentSector.size() > 2)
//							{
//								NavSector ns;
//								ns.m_Boundary = m_CurrentSector;
//								m_NavSectors.push_back(ns);
//
//								m_CurrentSector.clear();
//							}
//
//							++wp;
//						}
//						break;
//					}
//				case MAP_UNKNOWN:
//					continue;
//				}
//			}
//		}
//		f.close();
//	}
//
//	InitCollision();
//}

void PathPlannerNavMesh::cmdCreatePlanePoly(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(GetCurrentStateId()!=PlaceSector)
	{
		SetNextState(PlaceSector);
	}
	else
	{
		m_ToolCancelled = false;
		SetNextState(NoOp);
	}
}

void PathPlannerNavMesh::cmdCreateSlicePoly(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(GetCurrentStateId()!=SliceSector)
	{
		SetNextState(SliceSector);
	}
	else
	{
		m_ToolCancelled = false;
		SetNextState(NoOp);
	}
}

void PathPlannerNavMesh::cmdCreateSliceSector(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(GetCurrentStateId()!=SliceSectorWithSector)
	{
		SetNextState(SliceSectorWithSector);
	}
	else
	{
		m_ToolCancelled = false;
		SetNextState(NoOp);
	}
}

void PathPlannerNavMesh::cmdEditSector(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	m_ToolCancelled = false;
	SetNextState(EditSector);
}

void PathPlannerNavMesh::cmdSplitSector(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(GetCurrentStateId()!=SplitSector)
	{
		SetNextState(SplitSector);
	}
	else
	{
		m_ToolCancelled = false;
		SetNextState(NoOp);
	}
}

void PathPlannerNavMesh::cmdGroundSector(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	m_ToolCancelled = false;
	SetNextState(GroundSector);
}

void PathPlannerNavMesh::cmdCommitPoly(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	m_ToolCancelled = false;
	SetNextState(CommitSector);
}

void PathPlannerNavMesh::cmdDeleteSector(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vFacing, vPos;
	if(!Utils::GetLocalEyePosition(vPos) || !Utils::GetLocalFacing(vFacing))
	{
		EngineFuncs::ConsoleError("can't get facing or eye position");
		return;
	}

	PathPlannerNavMesh::NavCollision nc = FindCollision(vPos, vFacing*2048.f);
	if(!nc.DidHit() || nc.HitAttrib().Fields.Mirrored)
	{
		EngineFuncs::ConsoleError("can't find sector, aim at a sector and try again.");
		return;
	}

	m_NavSectors.erase(m_NavSectors.begin() + nc.HitAttrib().Fields.SectorId);
	InitCollision();
}

void PathPlannerNavMesh::cmdMirrorSectors(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() < 2)
	{
		EngineFuncs::ConsoleError("axis not specified, valid axis are x,y,z,o");
		return;
	}

	bool bNeg = false, bAll = false;
	if(_args[1].find("all") != std::string::npos)
		bAll = true;
	if(_args[1].find('-') != std::string::npos)
		bNeg = true;

	NavSector::eMirror mir = NavSector::MirrorNone;

	if(_args[1].find('x') != std::string::npos)
		mir = bNeg ? NavSector::MirrorNX : NavSector::MirrorX;
	if(_args[1].find('y') != std::string::npos)
		mir = bNeg ? NavSector::MirrorNY : NavSector::MirrorY;
	if(_args[1].find('z') != std::string::npos)
		mir = bNeg ? NavSector::MirrorNZ : NavSector::MirrorZ;
	if(_args[1].find('o') != std::string::npos)
		mir = NavSector::MirrorNone;

	if(bAll)
	{
		for(obuint32 s = 0; s < m_NavSectors.size(); ++s)
		{
			m_NavSectors[s].m_Mirror = mir;
		}
	}
	else
	{
		Vector3f vFacing, vPos;
		if(!Utils::GetLocalEyePosition(vPos) || !Utils::GetLocalFacing(vFacing))
		{
			EngineFuncs::ConsoleError("can't get facing or eye position");
			return;
		}

		NavCollision nc = FindCollision(vPos, vPos + vFacing * 2048.f);
		if(!nc.DidHit() || nc.HitAttrib().Fields.Mirrored)
		{
			EngineFuncs::ConsoleError("no sector at cursor");
			SetNextState(NoOp);
			return;
		}
		NavSector *pNavSector = &m_NavSectors[nc.HitAttrib().Fields.SectorId];
		if(!pNavSector)
		{
			EngineFuncs::ConsoleError("can't find sector, aim at a sector and try again.");
			return;
		}
		pNavSector->m_Mirror = mir;
	}

	InitCollision();
}

void PathPlannerNavMesh::cmdSectorCreateConnections(const StringVector &_args)
{
	typedef std::vector<SegmentList> SectorSegments;
	SectorSegments segs;

	//////////////////////////////////////////////////////////////////////////

	for(obuint32 i = 0; i < m_ActiveNavSectors.size(); ++i)
	{
		const NavSector &ns = m_ActiveNavSectors[i];
		segs.push_back(ns.GetEdgeSegments());
	}

	// look for edges that could be connected
	obuint32 LastPortalSector = (obuint32)-1;

	for(obuint32 i = 0; i < segs.size(); ++i)
	{
		for(obuint32 j = 0; j < segs.size(); ++j)
		{
			if(i==j)
				continue;

			// look for matching segments
			for(obuint32 s1 = 0; s1 < segs[i].size(); ++s1)
			{
				for(obuint32 s2 = 0; s2 < segs[j].size(); ++s2)
				{
					const Segment3f &sec1 = segs[i][s1];
					const Segment3f &sec2 = segs[j][s2];

					Segment3f overlap;
					if(Utils::GetSegmentOverlap(sec1,sec2,overlap))
					{
						if(!Utils::TestSegmentForOcclusion(overlap))
						{
							Utils::DrawLine(overlap.GetPosEnd(),overlap.Origin+Vector3f(0,0,32),COLOR::MAGENTA,10.f);
							Utils::DrawLine(overlap.GetNegEnd(),overlap.Origin+Vector3f(0,0,32),COLOR::MAGENTA,10.f);
							//Utils::DrawLine(sec2.GetPosEnd()+Vector3f(0,0,32),sec1.GetNegEnd()+Vector3f(0,0,32),COLOR::MAGENTA,10.f);

							NavPortal navPortal;
							navPortal.m_Segment = overlap;
							navPortal.m_DestPortal = s2; // todo
							navPortal.m_DestSector = j;
							m_NavPortals.push_back(navPortal);

							//////////////////////////////////////////////////////////////////////////
							// Update the sector so it knows about its portals
							if(LastPortalSector==i)
							{
								m_ActiveNavSectors[i].m_NumPortals++;
							}
							else
							{
								m_ActiveNavSectors[i].m_StartPortal = (int)m_NavPortals.size()-1;
								m_ActiveNavSectors[i].m_NumPortals = 1;
							}
							//////////////////////////////////////////////////////////////////////////

							LastPortalSector = i;
						}
					}
				}
			}
		}
	}
}

void PathPlannerNavMesh::cmdSetMapCenter(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vMapCenter;

	bool bError = false;
	switch(_args.size())
	{
	case 2:
		{
			if(_args[1]=="<position>" && Utils::GetLocalPosition(vMapCenter))
			{

			}
			else
			{
				bError = true;
			}
			break;
		}
	case 4:
		{
			if(Utils::ConvertString(_args[1],vMapCenter.x) &&
				Utils::ConvertString(_args[2],vMapCenter.y) &&
				Utils::ConvertString(_args[3],vMapCenter.z))
			{
			}
			else
			{
				bError = true;
			}
			break;
		}
	}
	
	if(bError)
	{
		EngineFuncs::ConsoleError("Invalid Parameters");
		return;
	}

	m_MapCenter = vMapCenter;
	InitCollision();
}


void PathPlannerNavMesh::cmdSectorSetProperty(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() < 3)
	{
		EngineFuncs::ConsoleError("sector_setproperty name value");
		return;
	}

	String propName = _args[1];
	String propValue = _args[2];

	Vector3f vFacing, vPos;
	if(!Utils::GetLocalEyePosition(vPos) || !Utils::GetLocalFacing(vFacing))
	{
		EngineFuncs::ConsoleError("can't get facing or eye position");
		return;
	}

	PathPlannerNavMesh::NavCollision nc = FindCollision(vPos, vFacing*2048.f);
	if(!nc.DidHit() || nc.HitAttrib().Fields.Mirrored)
	{
		EngineFuncs::ConsoleError("can't find sector, aim at a sector and try again.");
		return;
	}

	// TODO:
}

void PathPlannerNavMesh::_BenchmarkPathFinder(const StringVector &_args)
{
	EngineFuncs::ConsoleMessage("-= NavMesh PathFind Benchmark =-");

	double dTimeTaken = 0.0f;
	obint32 iNumSectors = (obint32)m_ActiveNavSectors.size();
	obint32 iNumPaths = iNumSectors * iNumSectors;
	
	Timer tme;
	tme.Reset();
	for(obint32 w1 = 0; w1 < iNumSectors; ++w1)
	{
		for(obint32 w2 = 0; w2 < iNumSectors; ++w2)
		{
			const NavSector &pS1 = m_ActiveNavSectors[w1];
			const NavSector &pS2 = m_ActiveNavSectors[w2];
			
			PlanPathToGoal(NULL,
				pS1.m_Middle+Vector3f(0,0,NavigationMeshOptions::CharacterHeight),
				pS2.m_Middle+Vector3f(0,0,NavigationMeshOptions::CharacterHeight),
				0);
		}
	}
	dTimeTaken = tme.GetElapsedSeconds();

	EngineFuncs::ConsoleMessage(va("generated %d paths in %f seconds: %f paths/sec", 
		iNumPaths, dTimeTaken, dTimeTaken != 0.0f ? (float)iNumPaths / dTimeTaken : 0.0f));
}

void PathPlannerNavMesh::_BenchmarkGetNavPoint(const StringVector &_args)
{
	obuint32 iNumIterations = 1;
	if(_args.size() > 1)
	{
		iNumIterations = atoi(_args[1].c_str());
		if(iNumIterations <= 0)
			iNumIterations = 1;
	}

	EngineFuncs::ConsoleMessage("-= NavMesh GetNavPoint Benchmark  =-");

	/*double dTimeTaken = 0.0f;
	obuint32 iNumWaypoints = m_ActiveNavSectors.size();
	Timer tme;

	obuint32 iHits = 0, iMisses = 0;
	tme.Reset();
	for(obuint32 i = 0; i < iNumIterations; ++i)
	{
		for(obuint32 w1 = 0; w1 < iNumWaypoints; ++w1)
		{
			NavSector *pSector = m_ActiveNavSectors[w1];

			Waypoint *pClosest = _GetClosestWaypoint(pWaypoint->GetPosition(), (NavFlags)0, true);
			if(pClosest)
				++iHits;
			else
				++iMisses;
		}
	}

	dTimeTaken = tme.GetElapsedSeconds();

	EngineFuncs::ConsoleMessage("_GetClosest() %d calls, %d hits, %d misses : avg %f per second", 
		iNumWaypoints * iNumIterations, 
		iHits, 
		iMisses, 
		dTimeTaken != 0.0f ? ((float)(iNumWaypoints * iNumIterations) / dTimeTaken) : 0.0f);	*/
}

void PathPlannerNavMesh::cmdNext(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, NoOp)
{
}

STATE_UPDATE(PathPlannerNavMesh, NoOp)
{
}

STATE_EXIT(PathPlannerNavMesh, NoOp)
{
}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, PlaceSector)
{
	Vector3f vPos, vNormal;
	if(!Utils::GetLocalAimPoint(vPos, &vNormal))
		return;

	m_WorkingSectorStart = vPos;
	m_WorkingSectorPlane = Utils::PlaneFromPoint(vPos, vNormal);
}

STATE_UPDATE(PathPlannerNavMesh, PlaceSector)
{
	Vector3f vPos, vNormal;
	if(!Utils::GetLocalAimPoint(vPos, &vNormal))
		return;

	m_WorkingSectorPlane = Utils::PlaneFromPoint(vPos, m_WorkingSectorPlane.Normal);

	Vector3List poly = Utils::CreatePolygon(
		vPos, 
		m_WorkingSectorPlane.Normal,
		32768.f);

	Utils::DrawPolygon(poly, COLOR::GREEN.fade(100), 0.1f, false);
}

STATE_EXIT(PathPlannerNavMesh, PlaceSector)
{
	if(m_ToolCancelled)
	{
		m_WorkingSector.clear();
		return;
	}

	m_WorkingSector = Utils::CreatePolygon(
		m_WorkingSectorPlane.Normal*-m_WorkingSectorPlane.Constant, 
		m_WorkingSectorPlane.Normal,
		32768.f);
}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, SliceSector)
{
	if(m_WorkingSector.empty())
	{
		EngineFuncs::ConsoleError("No Active Sector");
		SetNextState(NoOp);
	}

	Vector3f vPos, vNormal;
	if(!Utils::GetLocalAimPoint(vPos, &vNormal))
		return;

	m_WorkingSlicePlane = Utils::PlaneFromPoint(vPos, vNormal);
}

STATE_UPDATE(PathPlannerNavMesh, SliceSector)
{
	Vector3f vEye, vPos, vNormal;
	if(!Utils::GetLocalAimPoint(vPos, &vNormal) ||
		!Utils::GetLocalEyePosition(vEye))
		return;

	bool bSnapped = false;
	vPos = _SectorVertWithin(vEye, vPos, SNAP_RADIUS, &bSnapped);
	if(bSnapped)
		m_CursorColor = COLOR::ORANGE;

	m_WorkingSlicePlane = Utils::PlaneFromPoint(vPos, m_WorkingSlicePlane.Normal);

	Vector3List slicePoly = Utils::CreatePolygon(
		vPos, 
		m_WorkingSlicePlane.Normal,
		1024.f);

	Utils::DrawPolygon(slicePoly, COLOR::RED.fade(100), IGame::GetDeltaTimeSecs()*2.f, false);
}

STATE_EXIT(PathPlannerNavMesh, SliceSector)
{
	if(m_ToolCancelled)
		return;

	bool bClipFront = false;
	if(!bClipFront && m_WorkingSlicePlane.WhichSide(m_WorkingSectorStart) < 0)
		bClipFront = true;

	m_WorkingSector = Utils::ClipPolygonToPlanes(m_WorkingSector, m_WorkingSlicePlane, bClipFront);
}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, SliceSectorWithSector)
{
	if(m_WorkingSector.empty())
	{
		EngineFuncs::ConsoleError("No Active Sector");
		SetNextState(NoOp);
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	Vector3f vFacing, vPos;
	if(!Utils::GetLocalEyePosition(vPos) || !Utils::GetLocalFacing(vFacing))
	{
		EngineFuncs::ConsoleError("can't get facing or eye position");
		SetNextState(NoOp);
		return;
	}

	NavCollision nc = FindCollision(vPos, vPos + vFacing * 2048.f);
	if(!nc.DidHit())
	{
		EngineFuncs::ConsoleError("no sector at cursor");
		SetNextState(NoOp);
		return;
	}
	//////////////////////////////////////////////////////////////////////////

	m_WorkingSlicePlane = Utils::PlaneFromPoint(nc.HitPosition(),nc.HitNormal());

	bool bClipFront = false;
	if(!bClipFront && m_WorkingSlicePlane.WhichSide(m_WorkingSectorStart) < 0)
		bClipFront = true;

	m_WorkingSector = Utils::ClipPolygonToPlanes(m_WorkingSector, m_WorkingSlicePlane, bClipFront);

	SetNextState(NoOp);
}

STATE_UPDATE(PathPlannerNavMesh, SliceSectorWithSector)
{
}

STATE_EXIT(PathPlannerNavMesh, SliceSectorWithSector)
{

}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, EditSector)
{	
}

STATE_UPDATE(PathPlannerNavMesh, EditSector)
{
	Vector3f vLocalPos, vLocalAim;

	if(!Utils::GetLocalEyePosition(vLocalPos) ||
		!Utils::GetLocalFacing(vLocalAim))
	{
		SetNextState(NoOp);
		return;
	}

	NavCollision nc = FindCollision(vLocalPos, vLocalPos + vLocalAim * 1024.f);
	if(!nc.DidHit() && !nc.HitAttrib().Fields.Mirrored)
	{
		EngineFuncs::ConsoleError("No Nav Sector Found");
		SetNextState(NoOp);
		return;
	}

	m_WorkingSectorStart = nc.HitPosition();
	m_WorkingSector = m_NavSectors[nc.HitAttrib().Fields.SectorId].m_Boundary;
	m_NavSectors.erase(m_NavSectors.begin() + nc.HitAttrib().Fields.SectorId);

	InitCollision();

	SetNextState(NoOp);
}

STATE_EXIT(PathPlannerNavMesh, EditSector)
{
}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, SplitSector)
{
	if(m_WorkingSector.empty())
	{
		EngineFuncs::ConsoleError("No Active Sector");
		SetNextState(NoOp);
	}

	Vector3f vPos, vNormal;
	if(!Utils::GetLocalAimPoint(vPos, &vNormal))
		return;

	m_WorkingSlicePlane = Utils::PlaneFromPoint(vPos, vNormal);
}

STATE_UPDATE(PathPlannerNavMesh, SplitSector)
{
	Vector3f vPos, vNormal;
	if(!Utils::GetLocalAimPoint(vPos, &vNormal))
		return;

	m_WorkingSlicePlane = Utils::PlaneFromPoint(vPos, m_WorkingSlicePlane.Normal);

	Vector3List slicePoly = Utils::CreatePolygon(
		vPos, 
		m_WorkingSlicePlane.Normal,
		1024.f);

	Utils::DrawPolygon(slicePoly, COLOR::RED.fade(100), IGame::GetDeltaTimeSecs()*2.f, false);
}

STATE_EXIT(PathPlannerNavMesh, SplitSector)
{
	if(m_ToolCancelled)
		return;

	Vector3List s = m_WorkingSector;
	Vector3List s1 = Utils::ClipPolygonToPlanes(s, m_WorkingSlicePlane, true);
	Vector3List s2 = Utils::ClipPolygonToPlanes(s, m_WorkingSlicePlane, false);

	m_WorkingSector.resize(0);

	// add the 2 new sectors
	NavSector ns;
	if(!s1.empty())
	{
		ns.m_Boundary = s1;
		m_NavSectors.push_back(ns);
	}
	if(!s2.empty())
	{
		ns.m_Boundary = s2;
		m_NavSectors.push_back(ns);
	}
	
	InitCollision();
}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, TraceSector) 
{
	Vector3f vEye, vAimPos;
	Utils::GetLocalEyePosition(vEye);
	Utils::GetLocalAimPoint(vAimPos);

	// see if this closes the loop.
	if(m_WorkingManualSector.size()>2)
	{
		if(Length(vAimPos,m_WorkingManualSector[0]) <= SNAP_RADIUS)
		{
			m_WorkingManualSector.pop_back(); // remove the last pt
			vAimPos = m_WorkingManualSector[0];
			m_WorkingSector = m_WorkingManualSector;
			m_WorkingManualSector.resize(0);
			SetNextState(NoOp);
			return;
		}
	}	
	
	vAimPos = _SectorVertWithin(vEye, vAimPos,SNAP_RADIUS);

	if(m_WorkingManualSector.empty())
	{
		m_WorkingManualSector.push_back(vAimPos);
		m_WorkingManualSector.push_back(vAimPos);
	}
	else
	{
		m_WorkingManualSector.push_back(vAimPos);
	}
}

STATE_UPDATE(PathPlannerNavMesh, TraceSector)
{
	if(m_WorkingManualSector.empty())
		return;

	Vector3f vEye, vAimPos;
	Utils::GetLocalEyePosition(vEye);
	Utils::GetLocalAimPoint(vAimPos);

	bool bSnapped = false;

	// see if we can snap to the start.
	if(Length(vAimPos,m_WorkingManualSector[0]) <= SNAP_RADIUS)
	{
		m_CursorColor = COLOR::ORANGE;
		vAimPos = m_WorkingManualSector[0];
	}
	else
		vAimPos = _SectorVertWithin(vEye, vAimPos,SNAP_RADIUS,&bSnapped);

	if(bSnapped)
		m_CursorColor = COLOR::ORANGE;

	// update the last point
	m_WorkingManualSector[m_WorkingManualSector.size()-1] = vAimPos;

	Utils::DrawLine(m_WorkingManualSector, COLOR::GREEN, IGame::GetDeltaTimeSecs());
}

STATE_EXIT(PathPlannerNavMesh, TraceSector)
{
	if(m_ToolCancelled)
	{
		m_WorkingManualSector.resize(0);
		return;
	}

}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, GroundSector) 
{
}

STATE_UPDATE(PathPlannerNavMesh, GroundSector)
{
	Vector3f vLocalPos, vLocalAim;

	if(!Utils::GetLocalEyePosition(vLocalPos) ||
		!Utils::GetLocalFacing(vLocalAim))
	{
		SetNextState(NoOp);
		return;
	}

	NavCollision nc = FindCollision(vLocalPos, vLocalPos + vLocalAim * 1024.f);
	if(!nc.DidHit() && !nc.HitAttrib().Fields.Mirrored)
	{
		EngineFuncs::ConsoleError("No Nav Sector Found");
		SetNextState(NoOp);
		return;
	}

	NavSector &ns = m_NavSectors[nc.HitAttrib().Fields.SectorId];

	obTraceResult tr;

	/*for(obuint32 i = 0; i < ns.m_Boundary.size(); ++i)
	{
		const obuint32 x1 = i;
		const obuint32 x2 = i-1 < 0 ? ns.m_Boundary.size()-1 : i-1;

		Vector3f vMidPt = (ns.m_Boundary[x1] + ns.m_Boundary[x2]) * 0.5f;
		EngineFuncs::TraceLine(tr,
			vMidPt+Vector3f(0.f,0.f,1.f),
			vMidPt+Vector3f(0.f,0.f,-1024.f),
			NULL,
			TR_MASK_FLOODFILL,
			-1,
			False);

		ns.m_Boundary[x1].z = tr.m_Endpos[2];
		ns.m_Boundary[x2].z = tr.m_Endpos[2];
	}*/

	for(obuint32 i = 0; i < ns.m_Boundary.size(); ++i)
	{
		Vector3f vPt = ns.m_Boundary[i];
		EngineFuncs::TraceLine(tr,
			vPt+Vector3f(0.f,0.f,1.f),
			vPt+Vector3f(0.f,0.f,-1024.f),
			NULL,
			TR_MASK_FLOODFILL,
			-1,
			False);

		ns.m_Boundary[i].z = tr.m_Endpos[2];
	}

	InitCollision();

	SetNextState(NoOp);
}

STATE_EXIT(PathPlannerNavMesh, GroundSector)
{
}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, CommitSector) 
{
}

STATE_UPDATE(PathPlannerNavMesh, CommitSector)
{
	bool bInitCollision = false;

	//////////////////////////////////////////////////////////////////////////

	if(m_WorkingSector.size() > 2)
	{
		NavSector ns;
		ns.m_Boundary = m_WorkingSector;
		m_NavSectors.push_back(ns);
		bInitCollision = true;
	}
	m_WorkingSector.resize(0);

	//////////////////////////////////////////////////////////////////////////

	if(m_WorkingManualSector.size() > 2)
	{
		NavSector ns;
		ns.m_Boundary = m_WorkingManualSector;
		m_NavSectors.push_back(ns);
		bInitCollision = true;
	}
	m_WorkingManualSector.resize(0);

	if(bInitCollision)
	{
		InitCollision();
	}
}

STATE_EXIT(PathPlannerNavMesh, CommitSector)
{
}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, PlaceBorder)
{
}

STATE_UPDATE(PathPlannerNavMesh, PlaceBorder)
{
}

STATE_EXIT(PathPlannerNavMesh, PlaceBorder)
{
}

//////////////////////////////////////////////////////////////////////////

STATE_ENTER(PathPlannerNavMesh, MirrorSectors)
{
}

STATE_UPDATE(PathPlannerNavMesh, MirrorSectors)
{
}

STATE_EXIT(PathPlannerNavMesh, MirrorSectors)
{
}

