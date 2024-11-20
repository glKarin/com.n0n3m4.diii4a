#include "PrecompCommon.h"

#include "Interprocess.h"
#include "IGame.h"
#include "IGameManager.h"
#include "KeyValueIni.h"

#if defined WIN32
#define PATHDELIMITER ";"
#elif defined __linux__ || ((defined __MACH__) && (defined __APPLE__))
#define PATHDELIMITER ":"
#endif

typedef std::map<obuint32, obuint32> HashIndexMap;
static HashIndexMap		g_HashIndexMap;
static String			g_StringRepository;

//////////////////////////////////////////////////////////////////////////
// turn radius
// turnradius = fWheelBase / (2.f * PblMath::Sin(PblMath::DegToRad(fWheelAngle/2.f)));
//////////////////////////////////////////////////////////////////////////

namespace Priority
{
	const char *AsString(int n)
	{
		static const char *str[] =
		{
			"Zero",
			"Min",
			"Idle",
			"VeryLow",
			"Low",
			"LowMed",
			"Medium",
			"High",
			"VeryHigh",
			"Override",
		};
		BOOST_STATIC_ASSERT(sizeof(str) / sizeof(str[0]) == Priority::NumPriority);
		if(n >= 0 && n < Priority::NumPriority)
			return str[n];
		return "";
	}
}

//////////////////////////////////////////////////////////////////////////

namespace Utils
{
	bool RegexMatch( const char * exp, const char * str )
	{
		for(; ; exp++, str++)
		{
			char e = *exp, s = *str;
			if(!e) return !s;
			if(e == s) continue;
			if(e >= 'A' && e <= 'Z')
			{
				if(e + ('a' - 'A') == s) continue; //case insensitive
			}
			else if(e >= 'a' && e <= 'z')
			{
				if(e - ('a' - 'A') == s) continue;
			}
			else if(e != '_' && !(e >= '0' && e <= '9'))
			{
				if(e == '*' || e == '\\' && exp[1] == '{') exp--, str--; //repeat last char
				break;
			}
			e = exp[1];
			if(e != '*' && e != '\\') return false;
			break;
		}

		if(exp[0]=='.' && exp[1]=='*' && exp[2]=='\0') return true;

		try
		{
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
			std::regex expression( exp, REGEX_OPTIONS );
			return std::regex_match( str, expression );
#else
			boost::regex expression( exp, REGEX_OPTIONS );
			return boost::regex_match( str, expression );
#endif
		}
		catch(const std::exception&e)
		{
			_UNUSED(e);
			OBASSERT(0, e.what());
		}
		return false;
	}

	float FloatMax = std::numeric_limits<float>::max();

	void StringCopy(char *_destination, const char *_source, int _buffersize)
	{
		strncpy(_destination, _source, _buffersize);
	}

	obint32 StringCompare(const char *_s1, const char *_s2)
	{
		return strcmp(_s1, _s2);
	}

	obint32 StringCompareNoCase(const char *_s1, const char *_s2)
	{
#ifdef __GNUC__
		return strcasecmp(_s1, _s2);
#else
		return stricmp(_s1, _s2);
#endif
	}

	obint32 StringCompare(const String &_s1, const String &_s2)
	{
		return StringCompare(_s1.c_str(), _s2.c_str());
	}
	obint32 StringCompareNoCase(const String &_s1, const String &_s2)
	{
		return StringCompareNoCase(_s1.c_str(), _s2.c_str());
	}

	String StringToLower(const String &_s1)
	{
		String s = _s1;
		std::transform(s.begin(), s.end(), s.begin(), toLower());
		return s;
	}

	obint32 MakeId32(obint16 a, obint16 b)
	{
		return (((a)<<16) | (b));
	}

	void SplitId32(obint32 id, obint16 &id1, obint16 &id2)
	{
		id1 = (obint16)(id>>16);
		id2 = (obint16)id & 0xFF;
	}

	obint32 MakeId32(obint8 a, obint8 b, obint8 c, obint8 d )
	{
		return (((d)<<24) | ((c)<<16) | ((b)<<8) | (a));
	}

	obint32 MakeId32(const char *_st)
	{
		return MakeId32(_st[0], _st[1], _st[2], _st[3]);
	}

	obuint32 MakeHash32(const String &_str, bool _log /*= true*/)
	{
		if(_str.empty())
			return 0;

		if(_log)
			AddHashedString(_str);
		return Hash32(_str.c_str());
	}

	String FormatEntityString(GameEntity _e)
	{
		return String(va("%d:%d", _e.GetIndex(), _e.GetSerial()));
	}
	String FormatVectorString(const Vector3f &v)
	{
		return String(va("(%.3f, %.3f, %.3f)",v.x,v.y,v.z));
	}

	String FormatMatrixString(const Matrix3f &m)
	{
		return String(va("(%.3f, %.3f, %.3f) (%.3f, %.3f, %.3f) (%.3f, %.3f, %.3f)",
			m.GetColumn(0).x,m.GetColumn(0).y,m.GetColumn(0).z,
			m.GetColumn(1).x,m.GetColumn(1).y,m.GetColumn(1).z,
			m.GetColumn(2).x,m.GetColumn(2).y,m.GetColumn(2).z));
	}

	/*
	* FNV-1a hash 32 bit http://www.isthe.com/chongo/tech/comp/fnv/
	*/
	obuint32 Hash32(const char *_name)
	{
		const obuint32 FNV_32_PRIME = ((obuint32)0x01000193);
		const obuint32 FNV1A_32_INIT = ((obuint32)0x811c9dc5);

		const char *s = _name;
		obuint32 hval = FNV1A_32_INIT;
		while (*s)
		{
			char c = (char)tolower(*s++);
			hval ^= (obuint32)c;
			hval *= FNV_32_PRIME;
		}
		return hval;
	}
	/*
	* FNV-1a hash 64 bit http://www.isthe.com/chongo/tech/comp/fnv/
	*/
	obuint64 Hash64(const char *_name)
	{
		const obuint64 FNV_64_PRIME = ((obuint64)0x100000001b3ULL);
		const obuint64 FNV1A_64_INIT = ((obuint64)0x84222325 << 32 | (obuint64)0x1b3);

		const char *s = _name;
		obuint64 hval = FNV1A_64_INIT;
		while (*s)
		{
			char c = (char)tolower(*s++);
			hval ^= (obuint64)c;
			hval *= FNV_64_PRIME;
		}
		return hval;
	}

	void AddHashedString(const String &_str)
	{
		obuint32 hash = Hash32(_str.c_str());
		HashIndexMap::iterator it = g_HashIndexMap.find(hash);
		if(it == g_HashIndexMap.end())
		{
			obuint32 iOffset = (obuint32)g_StringRepository.size();
			g_StringRepository.append(_str.c_str(), (int)_str.size()+1);
			g_HashIndexMap.insert(std::make_pair(hash, iOffset));
		}
	}

	String HashToString(obuint32 _hash)
	{
		HashIndexMap::iterator it = g_HashIndexMap.find(_hash);
		if(it != g_HashIndexMap.end())
		{
			if(it->second < g_StringRepository.size())
			{
				return &g_StringRepository[it->second];
			}
		}
		return String(va("%x", _hash));
	}

	bool IsWhiteSpace(const char _ch)
	{
		const obuint8 cr = 0x0D;
		const obuint8 lf = 0x0A;
		return
			_ch == cr ||
			_ch == lf ||
			_ch == '\t' ||
			_ch == '\n' ||
			_ch == ' ';
	}

	void Tokenize(const String &_s, const String &_separators, StringVector &_tokens)
	{
		size_t pos_start, pos_end;

		pos_start = _s.find_first_not_of(_separators);
		if (pos_start == String::npos)
			return;

		pos_end = _s.find_first_of(_separators, pos_start);
		if (pos_end == String::npos)
		{
			_tokens.push_back(_s);
			return;
		}
		_tokens.push_back(_s.substr(pos_start, pos_end - pos_start));

		while ((pos_start = _s.find_first_not_of (_separators, pos_end)) != String::npos)
		{
			pos_end = _s.find_first_of(_separators, pos_start);
			if (pos_end == String::npos) {
				_tokens.push_back(_s.substr (pos_start));
				return;
			}
			_tokens.push_back(_s.substr(pos_start, pos_end - pos_start));
		}
	}

	const char *VarArgs(char *_outbuffer, int _buffsize, const char* _msg, ...)
	{
		va_list list;
		va_start(list, _msg);
#ifdef WIN32
		_vsnprintf(_outbuffer, _buffsize, _msg, list);
#else
		vsnprintf(_outbuffer, _buffsize, _msg, list);
#endif
		va_end(list);
		return _outbuffer;
	}

	void OutputDebugBasic(eMessageType _type, const char* _msg)
	{
#ifdef WIN32
		OutputDebugString(_msg);
		OutputDebugString("\n");
#endif
#ifdef ENABLE_DEBUG_WINDOW
		DW.Log.AddLine(_type,_msg);
#endif
	}
	void OutputDebug(eMessageType _type, const char* _msg, ...)
	{
		enum { BufferSize=2048 };
		char buffer[BufferSize] = {0};
		va_list list;
		va_start(list, _msg);
#ifdef WIN32
		_vsnprintf(buffer, BufferSize, _msg, list);
#else
		vsnprintf(buffer, BufferSize, _msg, list);
#endif
		va_end(list);
		OutputDebugBasic(_type,buffer);
	}

	fs::path FindFile(const fs::path &_file)
	{
		try
		{
			// Look for JUST the file in the current folder first.
			if(fs::exists(_file.filename()))
				return _file.filename();

			// Look for the file using the full provided path, if it differs from just the filename
			if((_file.string() != _file.filename()) && fs::exists(_file))
				return _file;

			// Look in the system path for the file.
			StringVector pathList;
			const char* pPathVariable = getenv("OMNIBOTFOLDER");
			if(pPathVariable)
				Utils::Tokenize(pPathVariable, PATHDELIMITER, pathList);
			pPathVariable = getenv("PATH");
			if(pPathVariable)
				Utils::Tokenize(pPathVariable, PATHDELIMITER, pathList);
				StringVector::const_iterator it = pathList.begin();
				for( ; it != pathList.end(); ++it)
				{
					try
					{
						// search for the just the file or the whole path
						fs::path checkPath = fs::path(*it) / fs::path(_file.filename());
						if(fs::exists(checkPath) && !fs::is_directory(checkPath))
							return checkPath;

						if (_file.string() != _file.filename())
						{
							checkPath = fs::path(*it) / fs::path(_file);
							if(fs::exists(checkPath) && !fs::is_directory(checkPath))
								return checkPath;
						}
					}
					catch(const std::exception & ex)
					{
						const char *pErr = ex.what();
						LOG("Filesystem Exception: " << pErr);
					}
				}
			}
		catch(const std::exception & ex)
		{
			const char *pErr = ex.what();
			LOG("Filesystem Exception: " << pErr);
		}

		// Not found, give back an empty path.
		return fs::path();
	}

	fs::path GetModFolder()
	{
		fs::path navFolder = GetBaseFolder();

		if(IGame *pGame = IGameManager::GetInstance()->GetGame())
		{
			// Append the script subfolder
			navFolder /= fs::path(pGame->GetModSubFolder());
			return navFolder;
		}

		return fs::path();
	}

	fs::path GetNavFolder()
	{
		fs::path navFolder = GetBaseFolder();

		if(IGame *pGame = IGameManager::GetInstance()->GetGame())
		{
			// Append the script subfolder
			navFolder /= fs::path(pGame->GetNavSubfolder());
			return navFolder;
		}

		return fs::path();
	}

	fs::path GetScriptFolder()
	{
		fs::path scriptFolder = GetBaseFolder();

		if(IGame *pGame = IGameManager::GetInstance()->GetGame())
		{
			// Append the script subfolder
			scriptFolder /= fs::path(pGame->GetScriptSubfolder());
			return scriptFolder;
		}

		return fs::path();
	}

	fs::path GetBaseFolder()
	{
		fs::path basePath;

		// First try to get a path from the game.
		const char *pPathOverride = g_EngineFuncs->GetBotPath();
		try
		{
			fs::path pathOverride(pPathOverride);
			if(fs::exists(pathOverride) && !fs::is_directory(pathOverride))
			{
				basePath = fs::path(pPathOverride);
				basePath = basePath.parent_path();
			}

			if(basePath.empty())
			{
				basePath = Utils::FindFile(pathOverride.filename());
				basePath = basePath.parent_path();
			}
		}
		catch(const std::exception & ex)
		{
			EngineFuncs::ConsoleError(ex.what());
			LOG("Bad Override Path: " << ex.what());
		}
		return basePath;
	}

	int GetLocalGameId()
	{
		return g_EngineFuncs->IDFromEntity(GetLocalEntity());
	}

	GameEntity GetLocalEntity()
	{
		return g_EngineFuncs->GetLocalGameEntity();
	}

	bool GetLocalPosition(Vector3f &_pos)
	{
		return EngineFuncs::EntityPosition(GetLocalEntity(), _pos);
	}

	bool GetLocalGroundPosition(Vector3f &_pos, int _tracemask)
	{
		return GetLocalGroundPosition(_pos, NULL, _tracemask);
	}

	bool GetLocalGroundPosition(Vector3f &_pos, Vector3f *_normal, int _tracemask /*= TR_MASK_FLOODFILL*/)
	{
		obTraceResult tr;
		Vector3f vPos;
		if(GetLocalEyePosition(vPos))
		{
			EngineFuncs::TraceLine(tr, vPos, vPos.AddZ(-4096),
				NULL, _tracemask, GetLocalGameId(), False);

			if(tr.m_Fraction < 1.f)
			{
				_pos = tr.m_Endpos;
				if(_normal)
					*_normal = tr.m_Normal;
				return true;
			}
		}
		return false;
	}

	bool GetLocalEyePosition(Vector3f &_pos)
	{
		return EngineFuncs::EntityEyePosition(GetLocalEntity(), _pos);
	}

	bool GetLocalFacing(Vector3f &_face)
	{
		return EngineFuncs::EntityOrientation(GetLocalEntity(), _face, NULL, NULL);
	}

	bool GetLocalAABB(AABB &_aabb)
	{
		return EngineFuncs::EntityWorldAABB(GetLocalEntity(), _aabb);
	}

	bool GetLocalAimPoint(Vector3f &_pos, Vector3f *_normal, int _tracemask /*= TR_MASK_FLOODFILL*/, int * _contents, int * _surface)
	{
		if(_contents)
			*_contents = 0;
		if(_surface)
			*_surface = 0;

		obTraceResult tr;
		Vector3f vNewStart, vPos, vFace;
		if(GetLocalEyePosition(vPos) &&
			GetLocalFacing(vFace) &&
			GetNearestNonSolid(vNewStart, vPos, vPos + vFace * 4096.f, _tracemask))
		{
			EngineFuncs::TraceLine(tr, vNewStart, vNewStart + vFace * 4096.f,
				NULL, _tracemask, GetLocalGameId(), False);

			if(tr.m_Fraction < 1.f)
			{
				_pos = tr.m_Endpos;
				if(_normal)
					*_normal = tr.m_Normal;
				if(_contents)
					*_contents = tr.m_Contents;
				if(_surface)
					*_surface = tr.m_Surface;
				return true;
			}
		}
		return false;
	}

	bool GetNearestNonSolid(Vector3f &_pos, const Vector3f &_start, const Vector3f &_end, int _tracemask /*= TR_MASK_FLOODFILL*/)
	{
		obTraceResult tr;
		Vector3f vStart = _start;
		Vector3f vEnd = _end;
		Vector3f vDir = vEnd - vStart;
		float fLength = vDir.Normalize();

		const float fStepSize = 32.f;
		while(fLength > 0.f)
		{
			EngineFuncs::TraceLine(tr, vStart, vEnd,
				NULL, _tracemask, GetLocalGameId(), False);

			if(!tr.m_StartSolid)
			{
				_pos = vStart;
				return true;
			}

			vStart += vDir * fStepSize;
			fLength -= fStepSize;
		}
		return false;
	}

	String FormatByteString(obuint64 _bytes)
	{
		const char * byteUnits[] =
		{
			" bytes",
			" KB",
			" MB",
			" GB",
			" TB"
		};

		obuint64 iNumUnits = sizeof(byteUnits) / sizeof(byteUnits[0]);
		int iUnit = 0;
		for(int i = 1; i < iNumUnits; ++i)
		{
			if(_bytes / pow(1024.0, i) >= 1)
				iUnit = i;
		}

		OBASSERT(iUnit >= 0 && iUnit < iNumUnits, "Out of Bounds!");

		std::stringstream str;
		str << (iUnit > 0 ? (_bytes / pow(1024.0, iUnit)) : _bytes) << byteUnits[iUnit];

		return str.str();
	}

	void DrawLine(const Vector3f &_start, const Vector3f &_end, obColor _color, float _time)
	{
		if(!g_EngineFuncs->DebugLine(_start, _end, _color, _time))
			InterProcess::DrawLine(_start, _end, _color, _time);
	}

	void DrawArrow(const Vector3f &_start, const Vector3f &_end, obColor _color, float _time)
	{
		if(!g_EngineFuncs->DebugArrow(_start, _end, _color, _time))
			InterProcess::DrawLine(_start, _end, _color, _time);
	}

	void DrawLine(const Vector3List &_list, obColor _color, float _time, float _vertheight, obColor _vertcolor, bool _closed)
	{
		if(_list.size()>1)
		{
			if(_vertheight > 0.f)
				DrawLine(_list[0], _list[0].AddZ(_vertheight), _vertcolor, _time);
			for(obuint32 i = 1; i < _list.size(); ++i)
			{
				Utils::DrawLine(_list[i-1], _list[i], _color, _time);

				if(_vertheight > 0.f)
					DrawLine(_list[i], _list[i].AddZ(_vertheight), _vertcolor, _time);
			}
			if(_closed)
				DrawLine(_list.back(), _list.front(), _color, _time);
		}
	}

	void DrawLine(const Vector3List &_vertices, const IndexList &_indices, obColor _color, float _time, float _vertheight, obColor _vertcolor, bool _closed)
	{
		if(_indices.size()>1)
		{
			if(_vertheight > 0.f)
				DrawLine(_vertices[_indices[0]], _vertices[_indices[0]].AddZ(_vertheight), _vertcolor, _time);
			for(obuint32 i = 1; i < _indices.size(); ++i)
			{
				Utils::DrawLine(_vertices[_indices[i-1]], _vertices[_indices[i]], _color, _time);

				if(_vertheight > 0.f)
					DrawLine(_vertices[_indices[i]], _vertices[_indices[i]].AddZ(_vertheight), _vertcolor, _time);
			}
			if(_closed)
				DrawLine(_vertices[_indices.front()], _vertices[_indices.back()], _color, _time);
		}
	}

	void DrawRadius(const Vector3f &_pos, float _radius, obColor _color, float _time)
	{
		if(!g_EngineFuncs->DebugRadius(_pos, _radius, _color, _time))
			InterProcess::DrawRadius(_pos, _radius, _color, _time);
	}

	void DrawPolygon(const Vector3List &_vertices, obColor _color, float _time, bool depthTest)
	{
		if(_vertices.empty())
			return;

		int flags = 0;

		if(depthTest)
			flags|=IEngineInterface::DR_NODEPTHTEST;

		if(!g_EngineFuncs->DebugPolygon((obVec3*)&_vertices[0], (int)_vertices.size(), _color, _time, flags))
			InterProcess::DrawPolygon(_vertices, _color, _time);
	}

	void GetAABBBoundary(const AABB &_aabb, Vector3List &_list)
	{
		Vector3f vVertex[8] = { Vector3f::ZERO };

		vVertex[0] = Vector3f(_aabb.m_Mins[0], _aabb.m_Mins[1], _aabb.m_Mins[2]);
		vVertex[1] = Vector3f(_aabb.m_Maxs[0], _aabb.m_Mins[1], _aabb.m_Mins[2]);
		vVertex[2] = Vector3f(_aabb.m_Maxs[0], _aabb.m_Maxs[1], _aabb.m_Mins[2]);
		vVertex[3] = Vector3f(_aabb.m_Mins[0], _aabb.m_Maxs[1], _aabb.m_Mins[2]);

		vVertex[4] = Vector3f(_aabb.m_Mins[0], _aabb.m_Mins[1], _aabb.m_Maxs[2]);
		vVertex[5] = Vector3f(_aabb.m_Maxs[0], _aabb.m_Mins[1], _aabb.m_Maxs[2]);
		vVertex[6] = Vector3f(_aabb.m_Maxs[0], _aabb.m_Maxs[1], _aabb.m_Maxs[2]);
		vVertex[7] = Vector3f(_aabb.m_Mins[0], _aabb.m_Maxs[1], _aabb.m_Maxs[2]);

		_list.push_back(vVertex[0]);
		_list.push_back(vVertex[1]);
		_list.push_back(vVertex[2]);
		_list.push_back(vVertex[3]);
	}

	void OutlineAABB(const AABB &_aabb, const obColor &_color, float _time, AABB::Direction _dir/* = AABB::DIR_ALL*/)
	{
		if(g_EngineFuncs->DebugBox(Vector3f::ZERO,Vector3f::ZERO,COLOR::WHITE,0.f))
		{
			if(_dir == AABB::DIR_ALL)
			{
				g_EngineFuncs->DebugBox(_aabb.m_Mins,_aabb.m_Maxs,_color,_time);
				return;
			}

			Vector3f vVertex[8] = { Vector3f::ZERO };

			vVertex[0] = Vector3f(_aabb.m_Mins[0], _aabb.m_Mins[1], _aabb.m_Mins[2]);
			vVertex[1] = Vector3f(_aabb.m_Maxs[0], _aabb.m_Mins[1], _aabb.m_Mins[2]);
			vVertex[2] = Vector3f(_aabb.m_Maxs[0], _aabb.m_Maxs[1], _aabb.m_Mins[2]);
			vVertex[3] = Vector3f(_aabb.m_Mins[0], _aabb.m_Maxs[1], _aabb.m_Mins[2]);

			vVertex[4] = Vector3f(_aabb.m_Mins[0], _aabb.m_Mins[1], _aabb.m_Maxs[2]);
			vVertex[5] = Vector3f(_aabb.m_Maxs[0], _aabb.m_Mins[1], _aabb.m_Maxs[2]);
			vVertex[6] = Vector3f(_aabb.m_Maxs[0], _aabb.m_Maxs[1], _aabb.m_Maxs[2]);
			vVertex[7] = Vector3f(_aabb.m_Mins[0], _aabb.m_Maxs[1], _aabb.m_Maxs[2]);

			// Top
			if(_dir == AABB::DIR_TOP || _dir == AABB::DIR_ALL)
			{
				Utils::DrawLine(vVertex[4], vVertex[5], _color, _time);
				Utils::DrawLine(vVertex[5], vVertex[6], _color, _time);
				Utils::DrawLine(vVertex[6], vVertex[7], _color, _time);
				Utils::DrawLine(vVertex[7], vVertex[4], _color, _time);
			}

			// Bottom
			if(_dir == AABB::DIR_BOTTOM || _dir == AABB::DIR_ALL)
			{
				Utils::DrawLine(vVertex[0], vVertex[1], _color, _time);
				Utils::DrawLine(vVertex[1], vVertex[2], _color, _time);
				Utils::DrawLine(vVertex[2], vVertex[3], _color, _time);
				Utils::DrawLine(vVertex[3], vVertex[0], _color, _time);
			}

			// Sides
			if(_dir == AABB::DIR_ALL)
			{
				Utils::DrawLine(vVertex[4], vVertex[0], _color, _time);
				Utils::DrawLine(vVertex[5], vVertex[1], _color, _time);
				Utils::DrawLine(vVertex[6], vVertex[2], _color, _time);
				Utils::DrawLine(vVertex[7], vVertex[3], _color, _time);
			}
		}
		else
			InterProcess::DrawBounds(_aabb, _color, _time, _dir);
	}

	void OutlineOBB(const Box3f &_obb, const obColor &_color, float _time, AABB::Direction _dir)
	{
		Vector3f vertices[8];
		_obb.ComputeVertices(vertices);

		// bottom
		if(_dir == AABB::DIR_BOTTOM || _dir == AABB::DIR_ALL)
		{
			Utils::DrawLine(vertices[0], vertices[1], _color, _time);
			Utils::DrawLine(vertices[1], vertices[2], _color, _time);
			Utils::DrawLine(vertices[2], vertices[3], _color, _time);
			Utils::DrawLine(vertices[3], vertices[0], _color, _time);
		}

		// top
		if(_dir == AABB::DIR_TOP || _dir == AABB::DIR_ALL)
		{
			Utils::DrawLine(vertices[4], vertices[5], _color, _time);
			Utils::DrawLine(vertices[5], vertices[6], _color, _time);
			Utils::DrawLine(vertices[6], vertices[7], _color, _time);
			Utils::DrawLine(vertices[7], vertices[4], _color, _time);
		}

		//verts
		if(_dir == AABB::DIR_ALL)
		{
			Utils::DrawLine(vertices[0], vertices[4], _color, _time);
			Utils::DrawLine(vertices[1], vertices[5], _color, _time);
			Utils::DrawLine(vertices[2], vertices[6], _color, _time);
			Utils::DrawLine(vertices[3], vertices[7], _color, _time);
		}
	}

	void PrintText(const Vector3f &_pos, obColor _color, float _duration, const char *_msg, ...)
	{
		const int iBufferSize = 2048;
		char buffer[iBufferSize] = {0};
		va_list list;
		va_start(list, _msg);
#ifdef WIN32
		_vsnprintf(buffer, iBufferSize, _msg, list);
#else
		vsnprintf(buffer, iBufferSize, _msg, list);
#endif
		va_end(list);

		if(!g_EngineFuncs->PrintScreenText(_pos, _duration, _color, buffer))
			InterProcess::DrawText(_pos, buffer, _color, _duration);
	}

	const char *FindClassName(obint32 _classId)
	{
		return IGameManager::GetInstance()->GetGame()->FindClassName(_classId);
	}

	obint32 GetRoleMask(const String &_name)
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
		gmTableObject *pTbl = pMachine->GetGlobals()->Get(pMachine,"Role").GetTableObjectSafe();
		if(pTbl)
		{
			gmTableIterator it;
			gmTableNode *pNode = pTbl->GetFirst(it);
			while(pNode)
			{
				const char *pName = pNode->m_key.GetCStringSafe(0);
				if(pName && pNode->m_value.IsInt() && !StringCompareNoCase(_name,pName))
				{
					return pNode->m_value.GetInt();
				}
				pTbl->GetNext(it);
			}
		}
		return 0;
	}
	std::string BuildRoleName(obint32 _mask)
	{
		if(_mask==0)
			return "None";

		bool AllRoles = true;
		std::string ret;

		int EnumSize = 0;
		const IntEnum *Enum = 0;
		IGameManager::GetInstance()->GetGame()->GetRoleEnumeration(Enum,EnumSize);

		for(int r = 0; r < EnumSize; ++r)
		{
			if((1<<Enum[r].m_Value) & _mask)
			{
				ret += Enum[r].m_Key;
				ret += " ";
			}
			else
			{
				AllRoles = false;
			}
		}

		if(AllRoles)
			return "All Roles";
		return ret;
	}

	bool ToLocalSpace(GameEntity _ent, const Vector3f &_worldpos, Vector3f &_out)
	{
		Vector3f vPos, vForward, vRight, vUp;
		if(EngineFuncs::EntityPosition(_ent, vPos) &&
			EngineFuncs::EntityOrientation(_ent, vForward, vRight, vUp))
		{
			Matrix3f mTransform(vRight, vForward, vUp, true);
			mTransform.Inverse();
			_out = (_worldpos - vPos) * mTransform;
			return true;
		}
		return false;
	}

	bool ToWorldSpace(GameEntity _ent, const Vector3f &_localpos, Vector3f &_out)
	{
		Vector3f vPos, vForward, vRight, vUp;
		if(EngineFuncs::EntityPosition(_ent, vPos) &&
			EngineFuncs::EntityOrientation(_ent, vForward, vRight, vUp))
		{
			Matrix3f mTransform(vRight, vForward, vUp, true);
			//Vector3f vOffset = mTransform.Inverse() * _localpos;

			_out = vPos + (mTransform * _localpos);
			return true;
		}

		return false;
	}

	Vector3f PredictFuturePositionOfTarget(const Vector3f &_tgpos, const Vector3f &_tgvel, float _timeahead)
	{
		return _tgpos + (_tgvel * _timeahead);
	}

	Vector3f PredictFuturePositionOfTarget(
		const Vector3f &_mypos,
		float _projspeed,
		const Vector3f &_tgpos,
		const Vector3f &_tgvel)
	{
		//if the target is ahead and facing the agent shoot at its current pos
		Vector3f vToEnemy = _tgpos - _mypos;

		//the lookahead time is proportional to the distance between the enemy
		//and the pursuer; and is inversely proportional to the sum of the
		//agent's velocities
		float fLookAheadTime = vToEnemy.Length() / (_projspeed + _tgvel.Length());

		//return the predicted future position of the enemy
		return _tgpos + (_tgvel * fLookAheadTime);
	}

	Vector3f PredictFuturePositionOfTarget(
		const Vector3f &_mypos,
		float _projspeed,
		const TargetInfo &_tg,
		const Vector3f &_extravelocity,
		float _minleaderror,
		float _maxleaderror)
	{
		//if the target is ahead and facing the agent shoot at its current pos
		Vector3f vToEnemy = _tg.m_LastPosition - _mypos;

		//the lookahead time is proportional to the distance between the enemy
		//and the pursuer; and is inversely proportional to the sum of the
		//agent's velocities
		float fLookAheadTime = vToEnemy.Length() / (_projspeed + _tg.m_LastVelocity.Length());

		if(_minleaderror != _maxleaderror)
			fLookAheadTime += Mathf::IntervalRandom(_minleaderror,_maxleaderror);
		else
			fLookAheadTime += _minleaderror;

		//return the predicted future position of the enemy
		Vector3f vPredictedPos = _tg.m_LastPosition + ((_tg.m_LastVelocity-_extravelocity) * fLookAheadTime);

		// Compensate for airborne target?
		//if(!_tg.m_EntityFlags.CheckFlag(ENT_FLAG_ONGROUND))
		//{
		//	//vPredictedPos.z -= _tg.m_LastVelocity.z + IGame::GetGravity() * fLookAheadTime;
		//	vPredictedPos.z = Trajectory::HeightForTrajectory(
		//		vPredictedPos,
		//		_tg.m_LastVelocity.z,
		//		IGame::GetGravity(),
		//		fLookAheadTime);
		//}
		return vPredictedPos;
	}

	Segment3f MakeSegment(const Vector3f &_p1, const Vector3f &_p2)
	{
		Vector3f t = _p2 - _p1;
		float fExtent = t.Normalize() * 0.5f;
		return Segment3f (_p1 + t * fExtent, t, fExtent);
	}

	Vector3f AveragePoint(const Vector3List &_list)
	{
		Vector3f vAvg = Vector3f::ZERO;

		if(!_list.empty())
		{
			for(obuint32 v = 0; v < _list.size(); ++v)
			{
				vAvg += _list[v];
			}

			vAvg /= (float)_list.size();
		}
		return vAvg;
	}

	float ClosestPointOfApproachTime(const Vector3f& aP1, const Vector3f& aV1, const Vector3f& aP2, const Vector3f& aV2)
	{
		Vector3f dv = aV1 - aV2;
		float dv2 = dv.Dot(dv);
		if(dv2 < Mathf::EPSILON)
			return 0.0;
		return -(aP1 - aP2).Dot(dv) / dv2;
	}

	void StringTrimCharacters(String &_out, const String &_trim)
	{
		obuint32 i;
		for(obuint32 t = 0; t < _trim.size(); ++t)
		{
			while((i = (obuint32)_out.find(_trim[t])) != _out.npos)
				_out.erase(i, 1);
		}
	}

	bool StringToTrue(const String &_str)
	{
		return (_str == "1" || _str == "on" || _str == "true");
	}

	bool StringToFalse(const String &_str)
	{
		return (_str == "0" || _str == "off" || _str == "false");
	}

	bool AssertFunction(bool _bexp, const char* _exp, const char* _file, int _line, const char *_msg, ...)
	{
		if(!_bexp)
		{
			enum { BufferSize=2048 };
			char buffer[BufferSize] = {0};
			va_list list;
			va_start(list, _msg);
#ifdef WIN32
			_vsnprintf(buffer, BufferSize, _msg, list);
#else
			vsnprintf(buffer, BufferSize, _msg, list);
#endif
			va_end(list);

#ifdef WIN32
			char strBigBuffer[BufferSize] = {};
			sprintf(strBigBuffer, "Assertion: %s\n%s\n%s : %d\nAbort to break\nRetry to continue\nIgnore to ignore this assert",
				_exp, buffer, _file, _line);
			int iRes = MessageBox(NULL, strBigBuffer, "Omni-bot: Assertion Failed", MB_ABORTRETRYIGNORE | MB_ICONWARNING);
			if(iRes == IDABORT)
				DebugBreak();
			return iRes != IDIGNORE;
#else
			snprintf(buffer, BufferSize, "%s(%d): ", _file, _line);
			size_t len = strlen(buffer);
			vsnprintf(buffer+len, BufferSize-len, _msg, list);
			EngineFuncs::ConsoleError(buffer);
			abort();
			return true;
#endif
		}
		return true;
	}

	bool SoftAssertFunction(AssertMode _mode, bool _bexp, const char* _exp, const char* _file, int _line, const char *_msg, ...)
	{
		if(!_bexp)
		{
			enum { BufferSize=2048 };
			char buffer[BufferSize] = {0};
			va_list list;
			va_start(list, _msg);
#ifdef WIN32
			_vsnprintf(buffer, BufferSize, _msg, list);
#else
			vsnprintf(buffer, BufferSize, _msg, list);
#endif
			va_end(list);

			char strBigBuffer[BufferSize] = {};
			sprintf(strBigBuffer, "--------------------\nAssertion: %s\n%s\n%s : %d\n--------------------\n",
				_exp, buffer, _file, _line);
			/*int iRes = MessageBox(NULL, strBigBuffer, "Omni-bot: Assertion Failed", MB_ABORTRETRYIGNORE | MB_ICONWARNING);
			if(iRes == IDABORT)
			DebugBreak();*/
#ifdef _WIN32
			OutputDebugString(strBigBuffer);
			return _mode != FireOnce;
#else
			std::cout << strBigBuffer;
			return false;
#endif
		}
		return true;
	}

	String FindOpenPlayerName()
	{
		// FIX THIS! CHECK WITH ENGINE
		static int nextIndex = 0;
		return String(va("OmniBot[%i]", nextIndex++));
	}

	bool TestSegmentForOcclusion(const Segment3f &seg)
	{
		AABB b(Vector3f(-32,-32,0), Vector3f(32,32,64));

		obTraceResult tr;
		EngineFuncs::TraceLine(tr,seg.Origin,seg.Origin,&b,TR_MASK_FLOODFILL,-1,False);
		if(tr.m_Fraction < 0.f)
			return true;

		return false;
	}

	bool GetSegmentOverlap(const Segment3f &_seg1, const Segment3f &_seg2, Segment3f &out)
	{
		static float DotThreshold = -0.98f;
		//static float DistanceThreshold = 8.f;

		// TODO: move these out.
		static float MinOverlapWidth = 10.f;
		static float MaxStepHeight = 20.f;
		static float MaxDropHeight = 32.f;

		static float MaxHorizontalDist = 32.f;

		// make sure the direction is proper
		const float fDot = _seg1.Direction.Dot(_seg2.Direction);
		if(fDot > DotThreshold)
			return false;

		// make sure the lines 'overlap' on the same line
		Vector3f cp;
		DistancePointToLine(_seg1.Origin,_seg2.GetNegEnd(),_seg2.GetPosEnd(),&cp);

		const float f2dDist = Length2d(cp,_seg1.Origin);

		const float fStepDist = (cp.z - _seg1.Origin.z);
		if(f2dDist > MaxHorizontalDist)
			return false;
		if(fStepDist > MaxStepHeight)
			return false;
		if(fStepDist < -MaxDropHeight)
			return false;

		// make sure they overlap in segment space
		const float Dist = Length(_seg1.Origin, _seg2.Origin);
		if(Dist > (_seg1.Extent + _seg2.Extent))
			return false;

		Vector3f vMin = _seg1.Origin,vMax = _seg2.Origin;

		float fT = 0.f;
		//////////////////////////////////////////////////////////////////////////
		fT = ClosestPtOnLine_Unclamped(_seg1.GetNegEnd(),_seg1.GetPosEnd(),_seg2.GetNegEnd(),cp);
		if(fT > 1.f)
		{
			vMax = _seg1.GetPosEnd();
		}
		else if(fT >= 0.f)
		{
			vMax = _seg2.GetNegEnd();
		}
		else
			OBASSERT(0,"Unexpected");

		fT = ClosestPtOnLine_Unclamped(_seg1.GetNegEnd(),_seg1.GetPosEnd(),_seg2.GetPosEnd(),cp);
		if(fT < 0.f)
		{
			vMin = _seg1.GetNegEnd();
		}
		else if(fT <= 1.f)
		{
			vMin = _seg2.GetPosEnd();
		}
		else
			OBASSERT(0,"Unexpected");
		//////////////////////////////////////////////////////////////////////////
		/*fT = ClosestPtOnLine_Unclamped(_seg2.GetPosEnd(),_seg2.GetNegEnd(),_seg1.GetPosEnd(),cp);
		if(fT > 1.f)
		{
			vMax = _seg1.GetPosEnd();
		}
		else if(fT > 0.f)
		{
			vMax = _seg2.GetNegEnd();
		}
		else
			OBASSERT(0,"Unexpected");

		fT = ClosestPtOnLine_Unclamped(_seg2.GetPosEnd(),_seg2.GetNegEnd(),_seg1.GetPosEnd(),cp);
		if(fT < 0.f)
		{
			vMin = _seg2.GetNegEnd();
		}
		else if(fT < 1.f)
		{
			vMin = _seg1.GetPosEnd();
		}
		else
			OBASSERT(0,"Unexpected");*/
		//////////////////////////////////////////////////////////////////////////

		out = MakeSegment(vMin,vMax);

		if(out.Extent * 2.f < MinOverlapWidth)
			return false;

		/*Vector3f cp;

		const fT1p = Utils::ClosestPtOnLine(
			_seg2.GetPosEnd(),
			_seg2.GetNegEnd(),
			_seg1.GetPosEnd(),
			cp);
		const fT1e = Utils::ClosestPtOnLine(
			_seg2.GetPosEnd(),
			_seg2.GetNegEnd(),
			_seg1.GetNegEnd(),
			cp);
		const fT2p = Utils::ClosestPtOnLine(
			_seg1.GetPosEnd(),
			_seg1.GetNegEnd(),
			_seg2.GetPosEnd(),
			cp);
		const fT2e = Utils::ClosestPtOnLine(
			_seg1.GetPosEnd(),
			_seg1.GetNegEnd(),
			_seg2.GetNegEnd(),
			cp);*/


		return true;
	}

	float ClosestPtOnLine(const Vector3f& p1, const Vector3f& p2, const Vector3f& p, Vector3f &cp)
	{
		Vector3f norm = p2 - p1;
		float len = norm.Normalize();
		float t = norm.Dot(p - p1);
		if(t <= 0.f || len <= Mathf::EPSILON)
		{
			t = 0.f;
			cp = p1;
		}
		else if(t >= len)
		{
			t = 1.f;
			cp = p2;
		}
		else
		{
			cp = p1 + norm * t;
			t /= len;
		}
		return t;
	}

	float ClosestPtOnLine_Unclamped(const Vector3f& p1, const Vector3f& p2, const Vector3f& p, Vector3f &cp)
	{
		Vector3f norm = p2 - p1;
		float len = norm.Normalize();
		float t = norm.Dot(p - p1);

		cp = p1 + norm * t;
		t /= len;
		return t;
	}

	Vector3f ChangePitch(const Vector3f &fwd, float _pitchangle)
	{
		float fHeading = fwd.XYHeading();
		Vector3f vNewFacing;
		vNewFacing.FromSpherical(fHeading, Mathf::DegToRad(_pitchangle), 1.f);
		return vNewFacing;
	}

	bool ClosestPtOnLine(const Vector3List &list, const Vector3f &pos, Vector3f &cp, bool loop)
	{
		if(list.size() < 2)
			return false;

		Vector3f vClosestPt, vPtOnLine;
		float fClosestDist = Utils::FloatMax;
		for(obuint32 v = 0; v < list.size()-1; ++v)
		{
			Utils::ClosestPtOnLine(list[v], list[v+1],pos,vPtOnLine);

			float fDist = SquaredLength(pos,vPtOnLine);
			if(fDist < fClosestDist)
			{
				fClosestDist = fDist;
				vClosestPt = vPtOnLine;
			}
		}

		if(loop)
		{
			Utils::ClosestPtOnLine(list.front(), list.back(),pos,vPtOnLine);
			float fDist = SquaredLength(pos,vPtOnLine);
			if(fDist < fClosestDist)
			{
				fClosestDist = fDist;
				vClosestPt = vPtOnLine;
			}
		}

		cp =  vClosestPt;
		return true;
	}


	Vector3List CreatePolygon(const Vector3f &_pos, const Vector3f &_normal, float _size)
	{
		Vector3List vl;

		Quaternionf q;
		q.FromAxisAngle(_normal, Mathf::DegToRad(90.f));

		Vector3f v = _normal.Perpendicular();
		v *= _size;

		vl.push_back(_pos+v);
		for(int i = 0; i < 3; ++i)
		{
			v = q.Rotate(v);
			vl.push_back(_pos+v);
		}
		return vl;
	}

	Vector3List ClipPolygonToPlanes(const Vector3List &_poly, const Plane3f &_plane, bool _clipfront)
	{
		if(_poly.size() < 2)
			return _poly;

		Vector3List newlist;
		Vector3f s = _poly[ _poly.size() - 1];
		for(obuint32 v = 0; v < _poly.size(); ++v)
		{
			Vector3f p = _poly[v];

			if(_clipfront)
			{
				if(_plane.WhichSide(p) < 0)
				{
					if(_plane.WhichSide(s) < 0)
					{
						newlist.push_back(p);
					}
					else
					{
						IntrSegment3Plane3f intr(Utils::MakeSegment(s,p), _plane);
						intr.Find();

						Vector3f vIntr = intr.GetSegment().Origin + intr.GetSegmentT()*intr.GetSegment().Direction;
						newlist.push_back(vIntr);
						newlist.push_back(p);
					}
				}
				else if(_plane.WhichSide(s) < 0)
				{
					IntrSegment3Plane3f intr(Utils::MakeSegment(s,p), _plane);
					intr.Find();

					Vector3f vIntr = intr.GetSegment().Origin + intr.GetSegmentT()*intr.GetSegment().Direction;
					newlist.push_back(vIntr);
				}
				s = p;
			}
			else
			{
				if(_plane.WhichSide(p) > 0)
				{
					if(_plane.WhichSide(s) > 0)
					{
						newlist.push_back(p);
					}
					else
					{
						IntrSegment3Plane3f intr(Utils::MakeSegment(s,p), _plane);
						intr.Find();

						Vector3f vIntr = intr.GetSegment().Origin + intr.GetSegmentT()*intr.GetSegment().Direction;
						newlist.push_back(vIntr);
						newlist.push_back(p);
					}
				}
				else if(_plane.WhichSide(s) > 0)
				{
					IntrSegment3Plane3f intr(Utils::MakeSegment(s,p), _plane);
					intr.Find();

					Vector3f vIntr = intr.GetSegment().Origin + intr.GetSegmentT()*intr.GetSegment().Direction;
					newlist.push_back(vIntr);
				}
				s = p;
			}
		}
		return newlist;
	}

	float DistancePointToLineSqr(const Vector3f &_point,
		const Vector3f &_pt0,
		const Vector3f &_pt1,
		Vector3f *_linePt)
	{
		Vector3f v = _point - _pt0;
		Vector3f s = _pt1 - _pt0;
		float lenSq = s.SquaredLength();
		float dot = v.Dot(s) / lenSq;
		Vector3f disp = s * dot;
		if (_linePt)
			*_linePt = _pt0 + disp;
		v -= disp;
		return v.SquaredLength();
	}

	float DistancePointToLine(const Vector3f &_point,
		const Vector3f &_pt0,
		const Vector3f &_pt1,
		Vector3f *_linePt)
	{
		return Mathf::Sqrt( DistancePointToLineSqr(_point, _pt0, _pt1, _linePt) );
	}

	//////////////////////////////////////////////////////////////////////////
	// inSegment(): determine if a point is inside a segment
	//    Input:  a point P, and a collinear segment S
	//    Return: 1 = P is inside S
	//            0 = P is not inside S
	int inSegment(Vector3f P, Segment3f _S)
	{
		if (_S.GetNegEnd().x != _S.GetPosEnd().x) {    // S is not vertical
			if (_S.GetNegEnd().x <= P.x && P.x <= _S.GetPosEnd().x)
				return 1;
			if (_S.GetNegEnd().x >= P.x && P.x >= _S.GetPosEnd().x)
				return 1;
		}
		else {    // S is vertical, so test y coordinate
			if (_S.GetNegEnd().y <= P.y && P.y <= _S.GetPosEnd().y)
				return 1;
			if (_S.GetNegEnd().y >= P.y && P.y >= _S.GetPosEnd().y)
				return 1;
		}
		return 0;
	}

	inline float dot(const Vector3f &u,const Vector3f &v)
	{
		return ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z);
	}
	inline float perp(const Vector3f &u,const Vector3f &v)
	{
		return ((u).x * (v).y - (u).y * (v).x);
	}
	// intersect2D_2Segments(): the intersection of 2 finite 2D segments
	//    Input:  two finite segments S1 and S2
	//    Output: *I0 = intersect point (when it exists)
	//            *I1 = endpoint of intersect segment [I0,I1] (when it exists)
	//    Return: 0=disjoint (no intersect)
	//            1=intersect in unique point I0
	//            2=overlap in segment from I0 to I1
	int intersect2D_Segments(const Segment3f &S1,const Segment3f &S2, Vector3f* I0, Vector3f* I1 )
	{
		Vector3f    u = S1.GetPosEnd() - S1.GetNegEnd();
		Vector3f    v = S2.GetPosEnd() - S2.GetNegEnd();
		Vector3f    w = S1.GetNegEnd() - S2.GetNegEnd();
		float		D = perp(u,v);

		// test if they are parallel (includes either being a point)
		if (fabs(D) < Mathf::EPSILON)
		{          // S1 and S2 are parallel
			if (perp(u,w) != 0 || perp(v,w) != 0)
			{
				return 0;                   // they are NOT collinear
			}
			// they are collinear or degenerate
			// check if they are degenerate points
			float du = dot(u,u);
			float dv = dot(v,v);
			if (du==0 && dv==0)
			{           // both segments are points
				if (S1.GetNegEnd() != S2.GetNegEnd())         // they are distinct points
					return 0;
				*I0 = S1.GetNegEnd();                // they are the same point
				return 1;
			}
			if (du==0)
			{                    // S1 is a single point
				if (inSegment(S1.GetNegEnd(), S2) == 0)  // but is not in S2
					return 0;
				*I0 = S1.GetNegEnd();
				return 1;
			}
			if (dv==0)
			{                    // S2 a single point
				if (inSegment(S2.GetNegEnd(), S1) == 0)  // but is not in S1
					return 0;
				*I0 = S2.GetNegEnd();
				return 1;
			}
			// they are collinear segments - get overlap (or not)
			float t0, t1;                   // endpoints of S1 in eqn for S2
			Vector3f w2 = S1.GetPosEnd() - S2.GetNegEnd();
			if (v.x != 0)
			{
				t0 = w.x / v.x;
				t1 = w2.x / v.x;
			}
			else
			{
				t0 = w.y / v.y;
				t1 = w2.y / v.y;
			}
			if (t0 > t1)
			{                  // must have t0 smaller than t1
				float t=t0; t0=t1; t1=t;    // swap if not
			}
			if (t0 > 1 || t1 < 0)
			{
				return 0;     // NO overlap
			}
			t0 = t0<0? 0 : t0;              // clip to min 0
			t1 = t1>1? 1 : t1;              // clip to max 1
			if (t0 == t1)
			{                 // intersect is a point
				*I0 = S2.GetNegEnd() + t0 * v;
				return 1;
			}

			// they overlap in a valid subsegment
			*I0 = S2.GetNegEnd() + t0 * v;
			*I1 = S2.GetNegEnd() + t1 * v;
			return 2;
		}

		// the segments are skew and may intersect in a point
		// get the intersect parameter for S1
		float     sI = perp(v,w) / D;

		// no intersect with S1
		if (sI < 0 || sI > 1)
			return 0;

		// get the intersect parameter for S2
		float     tI = perp(u,w) / D;

		// no intersect with S2
		if (tI < 0.f)
		{
			*I0 = S2.GetNegEnd();
			return 1;
		}
		if (tI > 1.f)
		{
			*I0 = S2.GetPosEnd();
			return 1;
		}

		*I0 = S1.GetNegEnd() + sI * u;               // compute S1 intersect point
		return 1;
	}


	gmVariable UserDataToGmVar(gmMachine *_machine, const obUserData &bud)
	{
		DisableGCInScope gcEn(_machine);

		switch(bud.DataType)
		{
		case obUserData::dtNone:
			return gmVariable::s_null;
		case obUserData::dtVector:
			return gmVariable(bud.GetVector()[0], bud.GetVector()[1], bud.GetVector()[2]);
		case obUserData::dtString:
			return gmVariable(_machine->AllocStringObject(bud.GetString() ? bud.GetString() : ""));
		case obUserData::dtInt:
			return gmVariable(bud.GetInt());
			break;
		case obUserData::dtFloat:
			return gmVariable(bud.GetFloat());
			break;
		case obUserData::dtEntity:
			{
				gmVariable v;
				v.SetEntity(bud.GetEntity().AsInt());
				return v;
			}
		case obUserData::dt3_4byteFlags:
			{
				gmTableObject *pTbl = _machine->AllocTableObject();
				for(int i = 0; i < 3; ++i)
					pTbl->Set(_machine, i, gmVariable(bud.Get4ByteFlags()[i]));
				return gmVariable(pTbl);
			}
		case obUserData::dt3_Strings:
			{
				gmTableObject *pTbl = _machine->AllocTableObject();
				for(int i = 0; i < 3; ++i)
					if(bud.GetStrings(i))
						pTbl->Set(_machine, i, gmVariable(_machine->AllocStringObject(bud.GetStrings(i))));
				return gmVariable(pTbl);
			}
		case obUserData::dt6_2byteFlags:
			{
				gmTableObject *pTbl = _machine->AllocTableObject();
				for(int i = 0; i < 6; ++i)
					pTbl->Set(_machine, i, gmVariable(bud.Get2ByteFlags()[i]));
				return gmVariable(pTbl);
			}
		case obUserData::dt12_1byteFlags:
			{
				gmTableObject *pTbl = _machine->AllocTableObject();
				for(int i = 0; i < 12; ++i)
					pTbl->Set(_machine, i, gmVariable(bud.Get1ByteFlags()[i]));
				return gmVariable(pTbl);
			}
		};
		return gmVariable::s_null;
	}
}

//////////////////////////////////////////////////////////////////////////

va::va(const char* msg, ...)
{
	va_list list;
	va_start(list, msg);
#ifdef WIN32
	_vsnprintf(buffer, BufferSize, msg, list);
#else
	vsnprintf(buffer, BufferSize, msg, list);
#endif
	va_end(list);
}

//////////////////////////////////////////////////////////////////////////

filePath::filePath()
{
	buffer[ 0 ] = 0;
}

filePath::filePath(const char* msg, ...)
{
	va_list list;
	va_start(list, msg);
#ifdef WIN32
	_vsnprintf(buffer, BufferSize, msg, list);
#else
	vsnprintf(buffer, BufferSize, msg, list);
#endif
	va_end(list);
	FixPath();
}

String filePath::FileName() const
{
	const char * fileName = buffer;
	const char *pC = buffer;
	while(*pC != '\0')
	{
		if(*pC == '\\' || *pC == '/')
			fileName = pC+1;
		++pC;
	}
	return fileName;
}

void filePath::FixPath()
{
	// unixify the path slashes
	char *pC = buffer;
	while(*pC != '\0')
	{
		if(*pC == '\\')
			*pC = '/';
		++pC;
	}

	// trim any trailing slash
	while(*pC == '/' && pC > buffer)
	{
		*pC = '\0';
		--pC;
	}
}

std::ostream& operator <<(std::ostream& _o, const filePath& _filePath) {
	_o << _filePath.c_str();
	return _o;
}

//////////////////////////////////////////////////////////////////////////

bool PropertyMap::AddProperty(const String &_name, const String &_data)
{
	if(_name.empty())
	{
		LOGERR("Invalid Waypoint Property Name or Data");
		return false;
	}

	// remove the old, case insensitive version
	ValueMap::iterator iter = m_Properties.begin();
	for(; iter != m_Properties.end(); ++iter)
	{
		if(!Utils::StringCompareNoCase(iter->first,_name))
		{
			m_Properties.erase(iter);
			break;
		}
	}

	m_Properties.insert(std::make_pair(_name, _data));
	return true;
}

void PropertyMap::DelProperty(const String &_name)
{
	ValueMap::iterator iter = m_Properties.find(_name);
	if(iter != m_Properties.end())
		m_Properties.erase(iter);
}

String PropertyMap::GetProperty(const String &_name) const
{
	ValueMap::const_iterator iter = m_Properties.find(_name);
	return (iter != m_Properties.end()) ? iter->second : String();
}

void PropertyMap::GetAsKeyVal(KeyVals &kv)
{
	PropertyMap::ValueMap::const_iterator cIt = m_Properties.begin();
	for(;cIt != m_Properties.end(); ++cIt)
	{
		kv.SetString(cIt->first.c_str(),cIt->second.c_str());
	}
}

std::ostream& operator <<(std::ostream& _o, const obUserData_t& _bud)
{
	_o << "UserData(";
	switch(_bud.DataType)
	{
	case obUserData_t::dtNone:
		_o << "dtNone";
		break;
	case obUserData_t::dtVector:
		_o << "dtVector, " <<
			_bud.udata.m_Vector[0] <<  ", " <<
			_bud.udata.m_Vector[1] <<  ", " <<
			_bud.udata.m_Vector[2];
		break;
	case obUserData_t::dtString:
		if(_bud.udata.m_String) _o << "dtString, " << _bud.udata.m_String;
		break;
	case obUserData_t::dtInt:
		_o << "dtInt, " << _bud.udata.m_Int;
		break;
	case obUserData_t::dtFloat:
		_o << "dtFloat, " << _bud.udata.m_Float;
		break;
	case obUserData_t::dtEntity:
		_o << "dtEntity, " << _bud.udata.m_Entity;
		break;
	case obUserData_t::dt3_4byteFlags:
		_o << "dt3_4byteFlags, " <<
			_bud.udata.m_4ByteFlags[0] << ", " <<
			_bud.udata.m_4ByteFlags[1] << ", " <<
			_bud.udata.m_4ByteFlags[2];
		break;
	case obUserData_t::dt3_Strings:
		_o << "dt3_Strings";
		if(_bud.udata.m_CharPtrs[0]) _o << ", " << _bud.udata.m_CharPtrs[0];
		if(_bud.udata.m_CharPtrs[1]) _o << ", " <<  _bud.udata.m_CharPtrs[1];
		if(_bud.udata.m_CharPtrs[2]) _o << ", " << _bud.udata.m_CharPtrs[2];
		break;
	case obUserData_t::dt6_2byteFlags:
		_o << "dt6_2byteFlags, " <<
			_bud.udata.m_2ByteFlags[0] << ", " <<
			_bud.udata.m_2ByteFlags[1] << ", " <<
			_bud.udata.m_2ByteFlags[2] << ", " <<
			_bud.udata.m_2ByteFlags[3] << ", " <<
			_bud.udata.m_2ByteFlags[4] << ", " <<
			_bud.udata.m_2ByteFlags[5];
		break;
	case obUserData_t::dt12_1byteFlags:
		_o << "dt12_1byteFlags, " <<
			(int)_bud.udata.m_1ByteFlags[0] << ", " <<
			(int)_bud.udata.m_1ByteFlags[1] << ", " <<
			(int)_bud.udata.m_1ByteFlags[2] << ", " <<
			(int)_bud.udata.m_1ByteFlags[3] << ", " <<
			(int)_bud.udata.m_1ByteFlags[4] << ", " <<
			(int)_bud.udata.m_1ByteFlags[5] << ", " <<
			(int)_bud.udata.m_1ByteFlags[6] << ", " <<
			(int)_bud.udata.m_1ByteFlags[7] << ", " <<
			(int)_bud.udata.m_1ByteFlags[8] << ", " <<
			(int)_bud.udata.m_1ByteFlags[9] << ", " <<
			(int)_bud.udata.m_1ByteFlags[10] << ", " <<
			(int)_bud.udata.m_1ByteFlags[11];
		break;
	}
	_o << ")";
	return _o;
}

std::ostream& operator <<(std::ostream& _o, const TriggerInfo_t& _ti)
{
	_o << "Trigger:";
	if(*_ti.m_TagName) _o << " TagName: " << _ti.m_TagName;
	if(*_ti.m_Action) _o << " Action: " << _ti.m_Action;
	if(_ti.m_Entity.IsValid())
		_o << " Entity: (" << _ti.m_Entity.GetIndex() << ":" << _ti.m_Entity.GetSerial() << ")";
	else
		_o << " Entity: (null)";
	if(_ti.m_Entity.IsValid())
		_o << " Activator: (" << _ti.m_Activator.GetIndex() << ":" << _ti.m_Activator.GetSerial() << ")";
	else
		_o << " Activator: (null)";

	return _o;
}

//////////////////////////////////////////////////////////////////////////

static KeyValueIni *FileOptions = 0;
static bool	OptionsChanged = false;
static bool OptionsInHomePath;

void Options::Init()
{
}

void Options::Shutdown()
{
	if(FileOptions)
	{
		releaseKeyValueIni(FileOptions);
		FileOptions = 0;
	}
}

bool Options::LoadConfigFile(const String &_file)
{
	obuint32 NumSections = 0;

	File f;
	if(f.OpenForRead(_file.c_str(),File::Text))
	{
		String contents;
		const obuint64 FileSize = f.ReadWholeFile(contents);
		if(FileSize)
		{
			if(FileOptions)
			{
				releaseKeyValueIni(FileOptions);
				FileOptions = 0;
			}

			FileOptions = loadKeyValueIni(contents.c_str(),(unsigned int)contents.size(),NumSections);
		}
		return true;
	}
	return false;
}

bool Options::LoadConfigFile()
{
	if(LoadConfigFile("homepath/omni-bot.cfg"))
	{
		OptionsInHomePath = true;
		return true;
	}
	OptionsInHomePath = false;
	return LoadConfigFile("user/omni-bot.cfg") || LoadConfigFile("config/omni-bot.cfg");
}

void Options::SaveConfigFileIfChanged()
{
	if(!OptionsChanged || !FileOptions) return;
	OptionsChanged = false;
	File f;
	/*
		GetLogPath() should return "fs_homepath" cvar
		Windows ETL: C:\Users\username\Documents\ETLegacy
		Windows ET 2.6: C:\Program Files (x86)\Wolfenstein - Enemy Territory
		Windows ioRTCW: C:\Users\username\Documents\RTCW
		Linux ETL: /home/username/.etlegacy
		Linux ET 2.6: /home/username/.etwolf
		Linux RTCW: /home/username/.wolf
		Mac ETL: /Users/username/Library/Application Support/etlegacy
		Mac ioRTCW: /Users/username/Library/Application Support/RTCW
	*/

	//If the config has been loaded from fs_homepath, save it to fs_homepath
	if(OptionsInHomePath && FileSystem::SetWriteDirectory(g_EngineFuncs->GetLogPath())
		&& !f.OpenForWrite("omni-bot.cfg", File::Text))
		FileSystem::SetWriteDirectory(Utils::GetModFolder());
	//Otherwise save it to omni-bot installation folder
	if(!f.IsOpen() && !f.OpenForWrite("user/omni-bot.cfg", File::Text)
		//If it failed (access denied), save it to fs_homepath
		&& !OptionsInHomePath && FileSystem::SetWriteDirectory(g_EngineFuncs->GetLogPath()))
	{
		OptionsInHomePath = true;
		f.OpenForWrite("omni-bot.cfg", File::Text);
	}
	if(f.IsOpen())
	{
		obuint32 fileLength = 0;
		void *fileData = saveKeyValueIniMem(FileOptions, fileLength);
		if(fileData)
		{
			f.Write(fileData, fileLength);
			releaseIniMem(fileData);
		}
		f.Close();
	}
	//restore previous write directory
	if(OptionsInHomePath) FileSystem::SetWriteDirectory(Utils::GetModFolder());
}

const char *Options::GetRawValue(const char *_section, const char *_key)
{
	if(FileOptions)
	{
		obuint32 KeyCount = 0, LineNo = 0;
		const KeyValueSection *Section = locateSection(FileOptions,_section,KeyCount,LineNo);
		if(Section)
		{
			return locateValue(Section,_key,LineNo);
		}
	}
	return 0;
}

bool Options::GetValue(const char *_section, const char *_key, bool &_out)
{
	const char *Value = GetRawValue(_section,_key);
	if(Value)
	{
		if(Utils::StringToTrue(Value))
			_out = true;
		else if(Utils::StringToFalse(Value))
			_out = false;
		else
			return false;
		return true;
	}
	return false;
}

bool Options::GetValue(const char *_section, const char *_key, int &_out)
{
	const char *Value = GetRawValue(_section,_key);
	if(Value && Utils::ConvertString(String(Value),_out))
	{
		return true;
	}
	return false;
}

bool Options::GetValue(const char *_section, const char *_key, float &_out)
{
	const char *Value = GetRawValue(_section,_key);
	if(Value && Utils::ConvertString(String(Value),_out))
	{
		return true;
	}
	return false;
}

bool Options::GetValue(const char *_section, const char *_key, String &_out)
{
	const char *Value = GetRawValue(_section,_key);
	if(Value)
	{
		_out = Value;
		return true;
	}
	return false;
}

bool Options::SetValue(const char *_section, const char *_key, bool _val, bool _overwrite)
{
	String sVal = _val?"true":"false";
	return SetValue(_section,_key,sVal,_overwrite);
}

bool Options::SetValue(const char *_section, const char *_key, int _val, bool _overwrite)
{
	String s;
	if(Utils::ConvertString(_val,s))
		return SetValue(_section,_key,s,_overwrite);
	return false;
}

bool Options::SetValue(const char *_section, const char *_key, float _val, bool _overwrite)
{
	String s;
	if(Utils::ConvertString(_val,s))
		return SetValue(_section,_key,s,_overwrite);
	return false;
}

bool Options::SetValue(const char *_section, const char *_key, const char *_val, bool _overwrite)
{
	return SetValue(_section,_key,String(_val),_overwrite);
}

bool Options::SetValue(const char *_section, const char *_key, const String &_val, bool _overwrite)
{
	if(!FileOptions)
		FileOptions = createKeyValueIni();

	if(FileOptions)
	{
		KeyValueSection *Section = createKeyValueSection(FileOptions,_section,false);

		obuint32 LineNo = 0;
		if(!_overwrite && locateValue(Section,_key,LineNo))
			return false;

		bool bGood = addKeyValue(Section,_key,_val.c_str());

		OptionsChanged = true;
		return bGood;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

bool Utils::TeamExists(obint32 _team)
{
	gmMachine *pM = ScriptManager::GetInstance()->GetMachine();
	gmTableObject *pTeams = pM->GetGlobals()->Get(pM,"TEAM").GetTableObjectSafe();
	gmTableIterator tit;
	gmTableNode *pNode = pTeams->GetFirst(tit);
	while(pNode)
	{
		if(pNode->m_value.GetInt()==_team)
			return true;
		pNode = pTeams->GetNext(tit);
	}
	return false;
}

bool Utils::ClassExists(obint32 _class)
{
	if(_class > 0 && _class <= FilterSensory::ANYPLAYERCLASS)
		return true;
	return false;
}

String Utils::GetTeamString(obint32 _team)
{
	gmMachine *pM = ScriptManager::GetInstance()->GetMachine();
	gmTableObject *pTeams = pM->GetGlobals()->Get(pM,"TEAM").GetTableObjectSafe();

	String sOut;

	bool bAllTeams = true;
	bool bNoTeams = true;

	// append all effecting teams.
	gmTableIterator tit;
	gmTableNode *pNode = pTeams->GetFirst(tit);
	while(pNode)
	{
		if(pNode->m_value.GetInt()!=OB_TEAM_SPECTATOR)
		{
			if(_team & (1<<pNode->m_value.GetInt()))
			{
				bNoTeams = false;
				sOut += pNode->m_key.GetCStringSafe("!!!");
				sOut += " ";
			}
			else
				bAllTeams = false;
		}
		pNode = pTeams->GetNext(tit);
	}

	if(bAllTeams)
		sOut = "All Teams";
	if(bNoTeams)
		sOut = "None";

	return sOut;
}

String Utils::GetClassString(obint32 _class)
{
	IGame *pGame = IGameManager::GetInstance()->GetGame();

	String sOut;
	bool bAllClasses = true;

	// append all effecting classes
	for(int c = 1; c < FilterSensory::ANYPLAYERCLASS; ++c)
	{
		if(_class & (1<<c))
		{
			const char *classname = pGame->FindClassName(c);
			sOut += (classname?classname:"!!!");
			sOut += " ";
		}
		else
			bAllClasses = false;
	}

	if(bAllClasses)
		sOut = "All Classes";

	return sOut;
}

void Utils::KeyValsToTable(const KeyVals &_kv, gmGCRoot<gmTableObject> _tbl, gmMachine *_machine)
{
	for(int i = 0; i < KeyVals::MaxArgs; ++i)
	{
		const char *Key = 0;
		obUserData Val;
		_kv.GetKV(i,Key,Val);
		if(Key)
		{
			_tbl->Set(_machine,
				gmVariable(_machine->AllocPermanantStringObject(Key)),
				UserDataToGmVar(_machine,Val)
				);
		}
	}

}

bool Utils::ConvertString(const String &_str, int &_var)
{
	errno = 0;
	char *endPtr;
	const char *startPtr = _str.c_str();
	int i = (int)strtol(startPtr, &endPtr, 10);
	if (endPtr != startPtr && !*endPtr && errno != ERANGE)
	{
		_var = i;
		return true;
	}
	return false;
}

bool Utils::ConvertString(const String &_str, float &_var)
{
	errno=0;
	char *endPtr;
	const char *startPtr = _str.c_str();
	double d = strtod(startPtr, &endPtr);
	if (endPtr!=startPtr && !*endPtr && errno!=ERANGE && abs(d)<=GM_MAX_FLOAT32)
	{
		_var = (float)d;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

int LimitChecker::FromScript(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	Get().ClearAll();
	for(int i = 0; i < a_thread->GetNumParams(); ++i)
	{
		gmTableObject *Tbl = a_thread->Param(i).GetTableObjectSafe();
		if(Tbl)
		{
			gmTableIterator tIt;
			gmTableNode *pNode = Tbl->GetFirst(tIt);
			while(pNode)
			{
				if(pNode->m_key.IsInt())
					Get().SetFlag(pNode->m_key.GetInt());
				pNode = Tbl->GetNext(tIt);
			}
		}
		else
		{
			GM_CHECK_INT_PARAM(id, i);
			Get().SetFlag(id);
		}
	}
	return GM_OK;
}

bool LimitWeapons::IsAllowed(Client *_client)
{
	if(mFlags.AnyFlagSet())
	{
		AiState::WeaponSystem *ws = _client->GetWeaponSystem();

		BitFlag128 hasWeapons = (mFlags & ws->GetWeaponMask());
		if(!hasWeapons.AnyFlagSet())
			return false;

		bool bOutOfAmmo = true;
		for(int i = 0; i < 128; ++i)
		{
			if(hasWeapons.CheckFlag(i))
			{
				WeaponPtr w = ws->GetWeapon(i);
				if(w)
				{
					w->UpdateAmmo();

					if(w->OutOfAmmo()==InvalidFireMode)
					{
						bOutOfAmmo = false;
						break;
					}
				}
			}
		}
		if(bOutOfAmmo)
			return false;
	}
	return true;
}

void ErrorObj::AddInfo(const char* _msg, ...)
{
	char buffer[8192] = {0};
	va_list list;
	va_start(list, _msg);
#ifdef WIN32
	_vsnprintf(buffer, 8192, _msg, list);
#else
	vsnprintf(buffer, 8192, _msg, list);
#endif
	va_end(list);

	mInfo.push_back(buffer);
}

void ErrorObj::AddError(const char* _msg, ...)
{
	char buffer[8192] = {0};
	va_list list;
	va_start(list, _msg);
#ifdef WIN32
	_vsnprintf(buffer, 8192, _msg, list);
#else
	vsnprintf(buffer, 8192, _msg, list);
#endif
	va_end(list);

	mErrors.push_back(buffer);
}

void ErrorObj::PrintToConsole()
{
	for(StringList::iterator it = mInfo.begin();
		it != mInfo.end();
		++it)
	{
		EngineFuncs::ConsoleMessage((*it).c_str());
	}

	for(StringList::iterator it = mErrors.begin();
		it != mErrors.end();
		++it)
	{
		EngineFuncs::ConsoleError((*it).c_str());
	}
}

StringBuffer::StringBuffer(obuint32 _maxStrings, obuint32 _bufferSize)
	: m_BufferSize(_bufferSize)
	, m_MaxStrings(_maxStrings)
{
	m_Buffer = new char[m_BufferSize];
	m_Strings = new char*[m_MaxStrings];

	memset(m_Strings,0,sizeof(char*)*m_MaxStrings);
	memset(m_Buffer,0,sizeof(char)*m_BufferSize);
	m_BufferOffset = 0;
}

StringBuffer::~StringBuffer()
{
	delete [] m_Strings;
	delete [] m_Buffer;
}

const char *StringBuffer::AddUniqueString(const String & _str)
{
	const char * exists = Find(_str);
	if(exists)
		return exists;

	if(m_BufferOffset + _str.length() + 1 >= m_BufferSize)
		return NULL;

	for(obuint32 s = 0; s < m_MaxStrings; ++s)
	{
		if(!m_Strings[s])
		{
			m_Strings[s] = &m_Buffer[m_BufferOffset];
			Utils::StringCopy(&m_Buffer[m_BufferOffset],_str.c_str(),(int)_str.length()+1);
			m_BufferOffset += (obuint32)_str.length()+1;
			return m_Strings[s];
		}
	}
	return NULL;
}

const char *StringBuffer::Find(const String & _str)
{
	for(obuint32 s = 0; s < m_MaxStrings; ++s)
	{
		if(m_Strings[s] && _str == m_Strings[s])
			return m_Strings[s];
	}
	return 0;
}
