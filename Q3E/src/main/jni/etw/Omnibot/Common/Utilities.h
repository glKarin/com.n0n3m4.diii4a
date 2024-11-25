#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdarg.h>

#include "Trajectory.h"

#include <stdlib.h> //#include <malloc.h> 
#define StackAlloc alloca 

class File;
class TargetInfo;

//////////////////////////////////////////////////////////////////////////
namespace Priority
{
	enum ePriority
	{
		Zero,
		Min,
		Idle,   //FollowPath
		VeryLow,//LookAround
		Low,    //goals CAMP, MOUNT, MOUNTMG42, ESCORT, RIDE
		LowMed, //AttackTarget
		Medium, //goals CHECKPOINT, PLANTMINE
		High,   //goals BUILD, PLANT, DEFUSE, SWITCH, GRENADE
		VeryHigh,
		Override,
		NumPriority
	};
	const char *AsString(int n);
}
//////////////////////////////////////////////////////////////////////////

// file: Utils
//		Contains various utilities that may be useful throughout the bot.
namespace Utils
{
	//////////////////////////////////////////////////////////////////////////
	extern float FloatMax;
	//////////////////////////////////////////////////////////////////////////
	bool RegexMatch( const char * exp, const char * str );

	void StringCopy(char *_destination, const char *_source, int _buffersize);

	obint32 StringCompare(const char *_s1, const char *_s2);
	obint32 StringCompareNoCase(const char *_s1, const char *_s2);

	obint32 StringCompare(const String &_s1, const String &_s2);
	obint32 StringCompareNoCase(const String &_s1, const String &_s2);

	String StringToLower(const String &_s1);

	bool IsWhiteSpace(const char _ch);

	const char *FindClassName(obint32 _classId);
	std::string BuildRoleName(obint32 _mask);

	inline float MillisecondsToSeconds(int _ms) { return (float)_ms/1000.f; }
	inline int SecondsToMilliseconds(float _seconds) { return (int)(_seconds*1000.f); }
	inline int HzToMilliseconds(int _hz) { return (int)(1000.f/(float)_hz); }
	inline float HzToSeconds(int _hz) { return MillisecondsToSeconds(HzToMilliseconds(_hz)); }

	inline bool InFieldOfView(const Vector3f &_face1, const Vector3f &_face2, float _fovAngles)
	{
		float fFovInRadians = Mathf::DegToRad(_fovAngles);
		return (_face1.Dot(_face2) >= cosf(fFovInRadians/2.0f) * _face1.Length() * _face2.Length());
	}

	//face1 length must be 1
	inline bool InFieldOfView120(const Vector3f &_face1, const Vector3f &_face2)
	{
		return _face1.Dot(_face2) * 2 >= _face2.Length(); // 2 = 1/cos(120/2)
	}

	inline bool InFieldOfView2d(const Vector3f &_face1, const Vector3f &_face2, float _fovAngles) 
	{ 
		Vector2f v1 = (Vector2f)_face1;
		Vector2f v2 = (Vector2f)_face2;

		float fFovInRadians = Mathf::DegToRad(_fovAngles); 
		return (v1.Dot(v2) >= cosf(fFovInRadians/2.0f) * v1.Length() * v2.Length());
	}

	inline float AngleBetween(const Vector3f &_v1, const Vector3f &_v2)
	{
		return Mathf::ACos(_v1.Dot(_v2));
	}

	inline Plane3f PlaneFromPoint(const Vector3f &_pos, const Vector3f &_normal)
	{
		Plane3f p;
		p.Normal = _normal;
		p.Constant = -(_pos.x * _normal.x + _pos.y * _normal.y + _pos.z * _normal.z);
		return p;
	}

	Segment3f MakeSegment(const Vector3f &_p1, const Vector3f &_p2);
	Vector3f AveragePoint(const Vector3List &_lis);

	float ClosestPointOfApproachTime(const Vector3f& aP1, const Vector3f& aV1, const Vector3f& aP2, const Vector3f& aV2) ;

	// function: Tokenize
	//		Splits a string into tokens using provided seperators. Stores in <StringVector>
	void Tokenize(const String &_s, const String &_separators, StringVector &_tokens);

	const char *VarArgs(CHECK_VALID_BYTES(_buffsize) char *_outbuffer, int _buffsize, CHECK_PRINTF_ARGS const char* _msg, ...);

	void OutputDebugBasic(eMessageType _type, const char* _msg);
	void OutputDebug(eMessageType _type, CHECK_PRINTF_ARGS const char* _msg, ...);

	// function: FindFile
	//		Finds a file in the PATH of the system.
	fs::path FindFile(const fs::path &_file);

	fs::path GetModFolder();
	// function: GetNavFolder
	//		Gets the absolute path to the users bot navigation folder
	fs::path GetNavFolder();
	// function: GetScriptFolder
	//		Gets the absolute path to the users bot script folder
	fs::path GetScriptFolder();
	// function: GetBaseFolder
	//		Gets the absolute path to the users bot folder
	fs::path GetBaseFolder();

	GameEntity GetLocalEntity();
	int GetLocalGameId();

	bool GetLocalPosition(Vector3f &_pos);
	bool GetLocalGroundPosition(Vector3f &_pos, Vector3f *_normal, int _tracemask = TR_MASK_FLOODFILL);
	bool GetLocalGroundPosition(Vector3f &_pos, int _tracemask);
	bool GetLocalEyePosition(Vector3f &_pos);
	bool GetLocalFacing(Vector3f &_face);
	bool GetLocalAABB(AABB &_aabb);
	bool GetLocalAimPoint(Vector3f &_pos, Vector3f *_normal = 0, int _tracemask = TR_MASK_FLOODFILL, int * _contents = 0, int * _surface = 0);
	bool GetNearestNonSolid(Vector3f &_pos, const Vector3f &_start, const Vector3f &_end, int _tracemask = TR_MASK_FLOODFILL);

	obint32 MakeId32(obint8 a, obint8 b, obint8 c, obint8 d);
	obint32 MakeId32(obint16 a, obint16 b);
	obint32 MakeId32(const char *_st);
	void SplitId32(obint32 id, obint16 &id1, obint16 &id2);

	obuint32 MakeHash32(const String &_str, bool _log = true);
	obuint32 Hash32(const char *_name);
	obuint64 Hash64(const char *_name);

	void AddHashedString(const String &_str);
	String HashToString(obuint32 _hash);

	// function: FormatByteString
	//		Formats a byte value into a readable string
	//		with a byte unit postfix, kB, MB, etc...
	String FormatByteString(obuint64 _bytes);

	String FormatEntityString(GameEntity _e);
	String FormatVectorString(const Vector3f &v);
	String FormatMatrixString(const Matrix3f &m);

	/*bool WriteDynamicBitsetToFile(File &_file, boost::dynamic_bitset<obuint64> &_bitset);
	bool ReadDynamicBitsetFromFile(File &_file, boost::dynamic_bitset<obuint64> &_bitset);*/

	void GetAABBBoundary(const AABB &_aabb, Vector3List &_list);
	void OutlineAABB(const AABB &_aabb,  const obColor &_color, float _time, AABB::Direction _dir = AABB::DIR_ALL);
	void OutlineOBB(const Box3f &_obb,  const obColor &_color, float _time, AABB::Direction _dir = AABB::DIR_ALL);
	void DrawLine(const Vector3f &_start, const Vector3f &_end, obColor _color, float _time);
	void DrawArrow(const Vector3f &_start, const Vector3f &_end, obColor _color, float _time);
	void DrawLine(const Vector3List &_list, obColor _color, float _time, float _vertheight = 0.f, obColor _vertcolor = COLOR::MAGENTA, bool _closed = false);
	void DrawLine(const Vector3List &_vertices, const IndexList &_indices, obColor _color, float _time, float _vertheight = 0.f, obColor _vertcolor = COLOR::MAGENTA, bool _closed = false);
	void DrawRadius(const Vector3f &_pos, float _radius, obColor _color, float _time);
	void DrawPolygon(const Vector3List &_vertices, obColor _color, float _time, bool depthTest = true);
	void PrintText(const Vector3f &_pos, obColor _color, float _duration, CHECK_PRINTF_ARGS const char *_msg, ...);
	
	bool ToLocalSpace(GameEntity _ent, const Vector3f &_worldpos, Vector3f &_out);
	bool ToWorldSpace(GameEntity _ent, const Vector3f &_localpos, Vector3f &_out);

	// function: PredictFuturePositionOfTarget
	//		Static helper function for calculating target leading.
	//
	// Parameters:
	//
	//		_tgpos - The <Vector3> position of the target.
	//		_tgvel - The <Vector3> velocity of the target.
	//		_timeahead - How many seconds ahead to calculate.
	//
	// Returns:
	//		Vector3 - A point along the targets current velocity to aim towards.
	Vector3f PredictFuturePositionOfTarget(const Vector3f &_tgpos, const Vector3f &_tgvel, float _timeahead);

	// function: PredictFuturePositionOfTarget
	//		Static helper function for calculating target leading.
	//
	// Parameters:
	//
	//		_mypos - The <Vector3> position of the shooter.
	//		_projectilespeed - The speed of the projectile being fired.
	//		_tgpos - The <Vector3> position of the target.
	//		_tgvel - The <Vector3> velocity of the target.
	//
	// Returns:
	//		Vector3 - A point along the targets current velocity to aim towards.
	Vector3f PredictFuturePositionOfTarget(
		const Vector3f &_mypos, 
		float _projectilespeed,
		const Vector3f &_tgpos, 
		const Vector3f &_tgvel);

	Vector3f PredictFuturePositionOfTarget(
		const Vector3f &_mypos, 
		float _projspeed,
		const TargetInfo &_tg,
		const Vector3f &_extravelocity,
		float _minleaderror = 0.f,
		float _maxleaderror = 0.f);

	bool TestSegmentForOcclusion(const Segment3f &seg);
	bool GetSegmentOverlap(const Segment3f &_seg1, const Segment3f &_seg2, Segment3f &out);

	float ClosestPtOnLine(const Vector3f &p1, const Vector3f &p2, const Vector3f &p, Vector3f &cp);
	bool ClosestPtOnLine(const Vector3List &list, const Vector3f &pos, Vector3f &cp, bool loop);
	float ClosestPtOnLine_Unclamped(const Vector3f& p1, const Vector3f& p2, const Vector3f& p, Vector3f &cp) ;

	Vector3f ChangePitch(const Vector3f &fwd, float _pitchangle);

	bool StringToTrue(const String &_str);
	bool StringToFalse(const String &_str);
	void StringTrimCharacters(String &_out, const String &_trim);

	String GetTeamString(obint32 _team);
	String GetClassString(obint32 _class);

	bool TeamExists(obint32 _team);
	bool ClassExists(obint32 _class);

	void KeyValsToTable(const KeyVals &_kv, gmGCRoot<gmTableObject> _tbl, gmMachine *_machine);

	gmVariable UserDataToGmVar(gmMachine *_machine, const obUserData &bud);

	bool ConvertString(const String &_str, int &_var);
	bool ConvertString(const String &_str, float &_var);

	template <typename T>
	bool ConvertString(const String &_str, T &_var)
	{
		StringStr st;
		st << _str;
		st >> _var;	
		return !st.fail();
	}

	template <typename T>
	bool ConvertString(const T &_var, String &_str)
	{
		StringStr st;
		st << _var;
		_str = st.str();
		return !st.fail();
	}

	String FindOpenPlayerName();

	enum AssertMode
	{
		FireOnce,
		FireAlways,
	};
	bool AssertFunction(bool _bexp, const char* _exp, const char* _file, int _line, CHECK_PRINTF_ARGS const char *_msg, ...);
	bool SoftAssertFunction(AssertMode _mode, bool _bexp, const char* _exp, const char* _file, int _line, CHECK_PRINTF_ARGS const char *_msg, ...);

	template <typename BidirectionalIterator, typename Predicate>
	BidirectionalIterator unstable_remove_if(BidirectionalIterator first, BidirectionalIterator last, Predicate pred) 
	{
		BidirectionalIterator cursor = first;
		while (cursor != last) 
		{
			if (pred(*cursor)) 
			{
				--last;
				if (cursor != last) 
				{
					using std::swap;
					swap(*cursor, *last);
				}
			} 
			else 
			{
				++cursor;
			}
		}
		return last;
	}

	Vector3List CreatePolygon(const Vector3f &_pos, const Vector3f &_normal, float _size = 512.f);
	Vector3List ClipPolygonToPlanes(const Vector3List &_poly, const Plane3f &_plane, bool _clipfront = false);

	float DistancePointToLineSqr(const Vector3f &_point,
		const Vector3f &_pt0,
		const Vector3f &_pt1,
		Vector3f *_linePt = NULL);

	float DistancePointToLine(const Vector3f &_point,
		const Vector3f &_pt0,
		const Vector3f &_pt1,
		Vector3f *_linePt = NULL);

	int intersect2D_Segments(const Segment3f &S1,const Segment3f &S2, Vector3f* I0 = 0, Vector3f* I1 = 0);
}

//////////////////////////////////////////////////////////////////////////
class PropertyMap
{
public:
	typedef std::map<String, String> ValueMap;

	bool AddProperty(const String &_name, const String &_data);
	void DelProperty(const String &_name);
	String GetProperty(const String &_name) const;

	obuint32 GetNumProperties() const { return (obuint32)m_Properties.size(); }

	template <typename T>
	bool AddPropertyT(const String &_name, T &_value)
	{
		String p;
		return (Utils::ConvertString(_value, p) && AddProperty(_name, p));
	}

	template <typename T>
	bool GetProperty(const String &_name, T &_value) const
	{
		String p = GetProperty(_name);
		return (!p.empty() && Utils::ConvertString(p, _value));
	}
	/*template <>
	bool GetProperty(const String &_name, String &_value) const
	{
		_value = GetProperty(_name);
		return !_value.empty();
	}*/
	void GetAsKeyVal(KeyVals &kv);

	const ValueMap &GetProperties() const { return m_Properties; }
private:
	ValueMap m_Properties;
};

//////////////////////////////////////////////////////////////////////////

class va {
public:
	const char * c_str() const { return buffer; }
	operator const char *() const { return buffer; }

	va(CHECK_PRINTF_ARGS const char* msg, ...);
protected:
	enum { BufferSize = 1024 };
	char buffer[BufferSize];
};

//////////////////////////////////////////////////////////////////////////

class filePath {
public:
	const char * c_str() const { return buffer; }
	operator const char *() const { return buffer; }

	String FileName() const;

	filePath();
	filePath( const char* msg, ... );
private:
	enum { BufferSize = 1024 };
	char buffer[BufferSize];

	void FixPath();
};

std::ostream& operator <<(std::ostream& _o, const filePath& _filePath);

//////////////////////////////////////////////////////////////////////////

// Templated helper functions.
template<class Type>
static inline Type MinT(const Type &_val1, const Type &_val2)       
{  
	return (_val1 < _val2) ? _val1 : _val2;
}

template<class Type>
static inline Type MaxT(const Type &_val1, const Type &_val2)       
{  
	return (_val1 > _val2) ? _val1 : _val2;
}

template<class Type>
static inline Type ClampT(const Type &_val, const Type &_low, const Type &_high)       
{  
	return ((_val<_low ? _low : (_val>_high) ? _high : _val));      
}

template<class Type>
static inline Type InRangeT(const Type &_val, const Type &_low, const Type &_high)       
{  
	return ((_val >= _low) && (_val <= _high));
}

template<class Type>
static inline bool CheckBitT(const Type &_val, const Type &_bit)
{  
	return ((_val & _bit) == _bit);
}

template<class Type>
static inline void SetBitT(Type &_val, const Type &_bit)
{  
	(_val |= _bit);
}

template<class Type>
static inline void ClearBitT(Type &_val, const Type &_bit)
{ 
	(_val &= ~_bit);
}

template<class Type>
static inline void SwapT(Type &_1, Type &_2)
{ 
	Type t = _1;
	_1 = _2;
	_2 = t;
}

template<class Type>
inline Type InterpolateT(const Type &_1, const Type &_2, float _t)
{
	return _1 + (_2 - _1) * _t;
}

// ostream helpers
std::ostream& operator <<(std::ostream& _o, const obUserData_t& _bud);
std::ostream& operator <<(std::ostream& _o, const TriggerInfo_t& _ti);

//////////////////////////////////////////////////////////////////////////
class Options
{
public:
	static void Init();
	static void Shutdown();
	static bool LoadConfigFile();
	static void SaveConfigFileIfChanged();

	static const char *GetRawValue(const char *_section, const char *_key);

	static bool GetValue(const char *_section, const char *_key, bool &_out);
	static bool GetValue(const char *_section, const char *_key, int &_out);
	static bool GetValue(const char *_section, const char *_key, float &_out);
	static bool GetValue(const char *_section, const char *_key, String &_out);

	static bool SetValue(const char *_section, const char *_key, bool _val, bool _overwrite = true);
	static bool SetValue(const char *_section, const char *_key, int _val, bool _overwrite = true);
	static bool SetValue(const char *_section, const char *_key, float _val, bool _overwrite = true);
	static bool SetValue(const char *_section, const char *_key, const char *_val, bool _overwrite = true);
	static bool SetValue(const char *_section, const char *_key, const String &_val, bool _overwrite = true);
private:
	static bool LoadConfigFile(const String &_file);
};

//////////////////////////////////////////////////////////////////////////

class LimitChecker
{
public:
	virtual bool IsAllowed(Client *_client) = 0;
	BitFlag128 &Get() { return mFlags; }
	int FromScript(gmThread *a_thread);
	LimitChecker() { }
protected:
	BitFlag128		mFlags;
};

class LimitWeapons : public LimitChecker
{
public:
	bool IsAllowed(Client *_client);
	LimitWeapons() {}
private:
};

//////////////////////////////////////////////////////////////////////////

class ErrorObj
{
public:
	void AddInfo(CHECK_PRINTF_ARGS const char* _msg, ...);
	void AddError(CHECK_PRINTF_ARGS const char* _msg, ...);

	void PrintToConsole();
private:
	StringList	mInfo;
	StringList	mErrors;
};

class StringBuffer
{
public:

	const char *AddUniqueString(const String & _str);
	const char *Find(const String & _str);

	StringBuffer(obuint32 _maxStrings = 64, obuint32 _bufferSize = 1024);
	~StringBuffer();
private:
	obuint32	m_BufferOffset;
	char **		m_Strings;
	char *		m_Buffer;

	const obuint32 m_BufferSize;
	const obuint32 m_MaxStrings;
};

#endif
