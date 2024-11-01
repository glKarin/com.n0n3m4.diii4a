//Commonly included headers
//In order to reduce the clutter in every file that commonly includes
//some headers, such as STL headers and utility headers, we put them
//in here so that those files can just include this and get access to all the common stuff.

#ifndef __COMMON_H__
#define __COMMON_H__

//#include "CodeAnalysis.h"

// Disable some compiler warnings.
#ifdef _MSC_VER
#pragma warning(   error: 4002 )	// Too many actual parameters for macro: promoted to be an error
//#pragma warning( disable: 4097 )	// typedef-name '...' used as synonym for class-name '...'
#pragma warning( disable: 4100 )	// unreferenced formal parameter
//#pragma warning( disable: 4127 )	// conditional expression is constant
//#pragma warning( disable: 4206 )	// nonstandard extension used : translation unit is empty
//#pragma warning( disable: 4505 )	// 'function' : unreferenced local function has been removed
//#pragma warning( disable: 4511 )	// 'class' : copy constructor could not be generated
#pragma warning( disable: 4512 )	// assignment operator could not be generated
//#pragma warning( disable: 4521 )	// 'class' : multiple copy constructors specified
//#pragma warning( disable: 4522 )	// 'class' : multiple assignment operators specified
//#pragma warning( disable: 4530 )	// C++ exception handler used, but unwind semantics are not enabled.
//#pragma warning( disable: 4714 )	// function 'function' marked as __forceinline not inlined
//#pragma warning( disable: 4996 )	// 'function': was declared deprecated
//#pragma warning( disable: 6246 )	// Local declaration of 'class' hides declaration of the same name in outer scope
//#pragma warning( disable: 6322 )	// Empty _except block
//#pragma warning( disable: 6355 )	// _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead
//#pragma warning( disable: 4512 )	// 'class' : assignment operator could not be generated
//#pragma warning( disable: 6384 )	// Dividing sizeof a pointer by another value

// Enable some useful ones that are disabled by default
// http://msdn2.microsoft.com/en-us/library/23k5d385(VS.80).aspx
#pragma warning( default: 4062)		// enumerator 'identifier' in switch of enum 'enumeration' is not handled
#pragma warning( default: 4265)		// class has virtual functions, but destructor is not virtual
#pragma warning( default: 4431)		// missing type specifier - int assumed. Note: C no longer supports default-int

// Disable if these get annoying.
#pragma warning( default: 4710 )	// function '...' not inlined
#pragma warning( default: 4711 )	// function '...' selected for automatic inline expansion
#endif // _MSC_VER

#define _UNUSED(x) ((void)x) // cs: for gcc warnings

// :note
//	Commonly included STL headers.
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>
#include <fstream>
//#include <strstream>
#include <sstream>
#include <limits>
#include <algorithm>
#include <cmath>
#include <bitset>

// typedef: String
//		Simple typedef for String
typedef std::string String;

// typedef: StringStr
//		Simple typedef for String Stream
typedef std::stringstream StringStr;

// typedef: StringVector
//		This type is defined as a vector of strings for various usages
typedef std::vector<String> StringVector;

// typedef: StringList
//		This type is defined as a vector of strings for various usages
typedef std::list<String> StringList;

#ifdef _OPENMP
#include <omp.h>
#endif

namespace stdext
{
    using std::unordered_map;
    using std::hash;
    using std::equal_to;
}

#ifdef WIN32
	//#define ENABLE_REMOTE_DEBUGGER
	//#define ENABLE_DEBUG_WINDOW
	//#define ENABLE_FILE_DOWNLOADER
	//#define ENABLE_REMOTE_DEBUGGING
#endif

#if __cplusplus < 201103L //karin: using C++11 instead of boost
// Boost
#if _MSC_VER >= 1600 // cs: ffs
#include <boost/version.hpp>
#if BOOST_VERSION <= 104400
#define BOOST_NO_RVALUE_REFERENCES 1
#endif // boost lib <= 1_44_0
#endif // vs2010
#endif

#ifdef _WIN32
#pragma warning( push )
// stfu boost
#pragma warning( disable: 4244 )
#pragma warning( disable: 4265 )
#endif //_WIN32

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
#include <filesystem>
#include <memory>
#include <functional>
#include <regex>
#include <algorithm>
#include <vector>
#include <random>

// karin: classes for compat
namespace compat
{
// for boost:dynamic_bitset
template <class T>
class dynamic_bitset : public std::vector<bool>
{
public:
	using std::vector<bool>::vector;

	void set(int pos, bool val)
	{
		operator[](pos) = val;
	}

	bool test(int pos) const
	{
        return operator[](pos) != 0;
	}
};

// for boost:shared_array
template <class T>
class shared_array : public std::shared_ptr<T>
{
public:
	using std::shared_ptr<T>::shared_ptr;
	explicit shared_array(T *ptr)
	: std::shared_ptr<T>(ptr, std::default_delete<T[]>())
	{}

	T & operator[] (ptrdiff_t idx)
	{
		return std::shared_ptr<T>::get()[idx];
	}
};

// for C++98 random_shuffle
template<class RandomIt>
void random_shuffle(RandomIt first, RandomIt last)
{
    std::random_device rd;
    std::default_random_engine g(rd());
    std::shuffle(first, last, g);
}

};
#define BOOST_STATIC_ASSERT static_assert
#else
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/static_assert.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <boost/multi_array.hpp>
#endif // end of __cplusplus >= 201103L

#ifdef ENABLE_FILE_DOWNLOADER
#include <boost/thread.hpp>
#endif

#ifdef _WIN32
#pragma warning( pop )
#endif // _WIN32

#if __cplusplus >= 201103L //karin: using std::filesystem on c++14/c++17. TODO: more __cplusplus macros check
//namespace fs = std::experimental::filesystem; // c++14
namespace fs = std::__fs::filesystem; // Android
//namespace fs = std::filesystem; // c++17
#else
namespace fs = boost::filesystem;
#endif

#ifdef ENABLE_FILE_DOWNLOADER
// typedef: Thread
//		Simpler typedef for boost::thread.
typedef boost::thread Thread;
typedef boost::thread_group ThreadGroup;
typedef boost::mutex Mutex;
typedef boost::try_mutex TryMutex;
#endif



// Wild Magic Math Libraries
#include "WildMagic3Math.h"
#include "WildMagic3Distance.h"
#include "WildMagic3Containment.h"
using namespace Wm3;

#include "Bezier.h"

// General purpose stuff.
#include "Functor.h"

// Custom files
#include "Logger.h"
#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"
#include "CallbackParameters.h"

#include "EngineFuncs.h"
#include "IEngineInterface.h"

#include "prof.h"

// remote debug
#ifdef ENABLE_REMOTE_DEBUGGING
#include "connection.h"
#include "messageTags.h"
#endif

// global: g_EngineFuncs is a bot-wide global so that game functionality
//		can be used from anywhere
extern IEngineInterface *g_EngineFuncs;
extern IClientInterface *g_ClientFuncs;

// typedef: PathList
//		This type is defined as a vector of paths
typedef std::vector<fs::path> DirectoryList;

// typedef: EntityList
//		This type is defined as a vector of GameEntities for various usages
typedef std::vector<GameEntity> EntityList;

// typedef: Vector3List
//		Vector of 3d points.
typedef std::vector<Vector3f> Vector3List;
typedef std::vector<obint32> IndexList;
typedef std::vector<IndexList> PolyIndexList;
typedef std::vector<Plane3f> PlaneList;
typedef std::vector<Segment3f> SegmentList;

#ifdef Prof_ENABLED
typedef ProfileZones<> CustomProfilerZone;
extern CustomProfilerZone gDynamicZones;
#endif

struct PointFacing
{
	Vector3f mPosition;
	Vector3f mFacing;
};
typedef std::vector<PointFacing> PointFacingList;

// typedef: Destination
//		Position, radius pair.
struct Destination
{
	Vector3f	m_Position;
	float		m_Radius;

	Destination() : m_Position(0.f,0.f,0.f), m_Radius(0.f) {}
	Destination(const Vector3f &_pos, float _radius)
		: m_Position(_pos)
		, m_Radius(_radius)
	{
	}
};
typedef std::vector<Destination> DestinationVector;

#if __cplusplus >= 201103L //karin: using std::vector<bool> specialized template instead of boost::dynamic_bitset<T>
typedef compat::dynamic_bitset<obuint32> DynBitSet32;
#else
typedef boost::dynamic_bitset<obuint32> DynBitSet32;
#endif
//typedef boost::dynamic_bitset<obuint64> DynBitSet64;

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
#define REGEX_OPTIONS std::regex::basic|std::regex::icase|std::regex::grep
#else
#define REGEX_OPTIONS boost::regex::basic|boost::regex::icase|boost::regex::grep
#endif

#define OB_DELETE(p)   { if(p) { delete (p); (p)=NULL; } }
#define OB_ARRAY_DELETE(p)   { if(p) { delete [] (p); (p)=NULL; } }
#define OB_MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define OB_MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define NO_OPTIMIZE #pragma optimize("", off)

// namespace: TR_COLOR
//		Predefined colors for use in traces, wrapped in namespace for isolation
namespace COLOR
{
	// const: RED
	//		Constant RGB for the color <RED>
	const obColor RED		= obColor(255, 0, 0);
	// const: GREEN
	//		Constant RGB for the color <GREEN>
	const obColor GREEN		= obColor(0, 255, 0);
	// const: BLUE
	//		Constant RGB for the color <BLUE>
	const obColor BLUE		= obColor(0, 0, 255);
	// const: BLACK
	//		Constant RGB for the color <BLACK>
	const obColor BLACK		= obColor(0, 0, 0);
	// const: WHITE
	//		Constant RGB for the color <WHITE>
	const obColor WHITE		= obColor(255, 255, 255);
	// const: MAGENTA
	//		Constant RGB for the color <MAGENTA>
	const obColor MAGENTA	= obColor(255, 0, 255);
	// const: GREY
	//		Constant RGB for the color <GREY>
	const obColor GREY		= obColor(127, 127, 127);
	// const: LIGHT_GREY
	//		Constant RGB for the color <LIGHT_GREY>
	const obColor LIGHT_GREY= obColor(211, 211, 211);
	// const: ORANGE
	//		Constant RGB for the color <ORANGE>
	const obColor ORANGE	= obColor(255, 127, 0);
	// const: YELLOW
	//		Constant RGB for the color <YELLOW>
	const obColor YELLOW	= obColor(255, 255, 0);
	// const: CYAN
	//		Constant RGB for the color <CYAN>
	const obColor CYAN	= obColor(0, 255, 255);
	// const: PINK
	//		Constant RGB for the color <PINK>
	const obColor PINK	= obColor(255, 20, 147);
	// const: BROWN
	//		Constant RGB for the color <BROWN>
	const obColor BROWN	= obColor(165, 42, 42);
	// const: AQUAMARINE
	//		Constant RGB for the color <AQUAMARINE>
	const obColor AQUAMARINE = obColor(127, 255, 212);
	// const: LAVENDER
	//		Constant RGB for the color <LAVENDER>
	const obColor LAVENDER = obColor(230, 230, 250);
}

enum MoveMode
{
	Run,
	Walk,
	NumMoveModes
};

#define DBG_MSG(debugflag, bot, type, msg) \
	if((debugflag)==0 || (bot)->IsDebugEnabled((debugflag))) { (bot)->OutputDebug((type), (msg)); }

// macro: DEBUG_ONLY
//		A macro that can be used to only allow the contained functionality
//		in debug builds of the library.
#ifdef _DEBUG
#define DEBUG_ONLY(m) m
#else
#define DEBUG_ONLY(m) 0
#endif

#include "Utilities.h"

// cs: FIXME: debug version of OBASSERT doesnt build in linux. doesn't like __VA_ARGS__
#ifdef __linux__
#ifdef	_DEBUG
#define OBASSERT(f, msg, ...) { static bool bShowAssert = true; \
	if(bShowAssert) { \
	bShowAssert = Utils::AssertFunction((bool)((f)!=0), #f, __FILE__, __LINE__, msg, ##__VA_ARGS__); \
		} }
#else	// !DEBUG
#define OBASSERT(f, sz, ...) {}	// cs: for gcc warnings, was (f)
#endif	// _DEBUG
#else // !__linux__
#ifdef	_DEBUG
#define OBASSERT(f, ...) { \
	static bool bShowAssert = true; \
	if(bShowAssert) { \
		bShowAssert = Utils::AssertFunction((bool)((f)!=0), #f, __FILE__, __LINE__, __VA_ARGS__); \
	} }
#else	// !DEBUG
#define OBASSERT(f, sz, ...) (void)(f)
#endif	// !DEBUG

#endif

#ifdef	_DEBUG
#define SOFTASSERTONCE(f, sz, ...) { static bool bShowAssert = true; \
	if(bShowAssert) { \
	bShowAssert = Utils::SoftAssertFunction(Utils::FireOnce, (bool)((f)!=0), #f, __FILE__, __LINE__, __VA_ARGS__); \
	} }
#else	// !DEBUG
#define SOFTASSERTONCE(f, sz, ...)
#endif	// !DEBUG

#define SOFTASSERTALWAYS(f, ...) { static bool bShowAssert = true; \
	if(bShowAssert) { \
	bShowAssert = Utils::SoftAssertFunction(Utils::FireAlways, (bool)((f)!=0), #f, __FILE__, __LINE__, __VA_ARGS__); \
	} }

#define FINDSTATE(var, statename, parent) \
	statename *var = static_cast<statename*>(parent->FindState(#statename)); OBASSERT(var, #statename " Not Found" );

#define FINDSTATEIF(statename, parent, exp) { \
	statename *st = static_cast<statename*>(parent->FindState(#statename));	if(st) st->exp; else OBASSERT(0, #statename " Not Found" ); }

#define FINDSTATE_OPT(var, statename, parent) \
	statename *var = static_cast<statename*>(parent->FindState(#statename));

#define FINDSTATEIF_OPT(statename, parent, exp) { \
	statename *st = static_cast<statename*>(parent->FindState(#statename));	if(st) st->exp; }

#endif
