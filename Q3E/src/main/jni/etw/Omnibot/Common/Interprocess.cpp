#include "PrecompCommon.h"
#include <iostream>
#include <fstream>
#include "Omni-Bot_Types.h"
#include "Omni-Bot_BitFlags.h"
#include "Interprocess.h"

Prof_Define(InterProcess);

//////////////////////////////////////////////////////////////////////////

#ifdef INTERPROCESS
#include <boost/interprocess/ipc/message_queue.hpp>
using namespace boost::interprocess;

template <typename MsgType>
class InterProcessMessageQueue
{
public:

	void Send(const MsgType &_message, obuint32 _priority = 0)
	{
		Prof(InterProcessMessageQueue_Send);
		m_MessageQueue.send(&_message, sizeof(MsgType), _priority);
	}

	bool TrySend(const MsgType &_message, obuint32 _priority = 0)
	{
		Prof(InterProcessMessageQueue_TrySend);
		return m_MessageQueue.try_send(&_message, sizeof(MsgType), _priority);
	}

	InterProcessMessageQueue(const char *_name, obuint32 _msgs) :
		m_Name(_name),
		m_MessageQueue(open_or_create, _name, _msgs, sizeof(MsgType))
	{
	}
	~InterProcessMessageQueue()
	{
		message_queue::remove(m_Name.c_str());
	}
private:
	const String	m_Name;
	message_queue	m_MessageQueue;

};

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr< InterProcessMessageQueue<IPC_DebugDrawMsg> > MessageQueuePtr;
#else
typedef boost::shared_ptr< InterProcessMessageQueue<IPC_DebugDrawMsg> > MessageQueuePtr;
#endif
MessageQueuePtr g_MessageQueue;

#else
#include "BotExports.h"

#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>

#ifndef __APPLE__
#include <link.h>

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data)
{
	const char *name=info->dlpi_name;
#ifdef __x86_64__
	if(strstr(name, "/cgame.mp.x86_64.so")){
#else
	if(strstr(name, "/cgame.mp.i386.so")){
#endif
		void *hmod = dlopen(name, RTLD_NOW|RTLD_NOLOAD);
		if(hmod){
			*(void**)data = hmod;
			return 1;
		}
	}
	return 0;
}
#endif
#endif

#endif // INTERPROCESS

IClientInterface *g_ClientFuncs;

//////////////////////////////////////////////////////////////////////////
namespace InterProcess
{
	bool Enabled = true;
	bool Initialized = false;

	//////////////////////////////////////////////////////////////////////////

	void DrawActiveFrame()
	{
		PathPlannerBase *pPathPlanner = IGameManager::GetInstance()->GetNavSystem();
		if(pPathPlanner->IsViewOn())
			pPathPlanner->DrawActiveFrame();
	}

	void Init()
	{
		Prof_Scope(InterProcess);

		if(Initialized || !Enabled) return;

		Vector3f v1(Vector3f::ZERO);

		if(!g_EngineFuncs->DebugLine(v1,v1,COLOR::GREEN,0.f) &&
			!g_EngineFuncs->DebugRadius(v1,0.f,COLOR::GREEN, 0.f))
		{
#ifdef INTERPROCESS
			try
			{
				message_queue::remove("debug_draw_queue");
				g_MessageQueue.reset(new InterProcessMessageQueue<IPC_DebugDrawMsg>("debug_draw_queue", 8192));
				LOG("InterProcess Initialized");
			} catch(interprocess_exception &ex)
			{
				g_MessageQueue.reset();
				LOGERR(ex.what());
			}
#else


#ifdef WIN32
#ifdef _WIN64
			HMODULE hmod = GetModuleHandle("cgame_mp_x64");
#else
			HMODULE hmod = GetModuleHandle("cgame_mp_x86");
#endif
			if(hmod){
				pfnGetClientFunctionsFromDLL pfnGetBotFuncs = (pfnGetClientFunctionsFromDLL)GetProcAddress(hmod, "ExportClientFunctionsFromDLL");
#else
			void *hmod = 0;
#if defined __APPLE__
			hmod = dlopen(va("%s/omnibot/cgame_mac", g_EngineFuncs->GetLogPath()), RTLD_NOW|RTLD_NOLOAD); //ETLegacy
			if(!hmod) hmod = dlopen("omnibot/cgame.mp.x86_64.dylib", RTLD_NOW|RTLD_NOLOAD); //ioRTCW
#else
			dl_iterate_phdr(dl_iterate_callback, &hmod);
#endif
			if(hmod){
				pfnGetClientFunctionsFromDLL pfnGetBotFuncs = (pfnGetClientFunctionsFromDLL)dlsym(hmod, "ExportClientFunctionsFromDLL");
#endif
				if(pfnGetBotFuncs && pfnGetBotFuncs(&g_ClientFuncs, 2)==BOT_ERROR_NONE){
					g_ClientFuncs->DrawActiveFrame = DrawActiveFrame;
					LOG("cgame drawing Initialized");
				}
				else {
					EngineFuncs::ConsoleError("Cannot export drawing functions from cgame module.");
					LOGERR("cgame drawing failed");
				}
#ifndef WIN32
				dlclose(hmod);
#endif
			}
			else {
				//it happens if warmup is zero and there is DrawDebugLine in OnMapLoad
				EngineFuncs::ConsoleError("Cannot find cgame module.");
				LOGERR("cgame module not found");
			}

#endif // INTERPROCESS
		}
		else
		{
			LOG("InterProcess Not Required, interface callbacks defined.");
		}

		Initialized=true;
	}

	void Shutdown()
	{
#ifdef INTERPROCESS
		g_MessageQueue.reset();
#else
		if(g_ClientFuncs)
		{
			g_ClientFuncs->DrawActiveFrame = NULL;
			g_ClientFuncs = 0;
		}
#endif
		Initialized=false;
	}

	void Update()
	{
#ifdef INTERPROCESS
		Prof_Scope(InterProcess);
		{
			Prof(Update);
			if(g_MessageQueue)
			{
				// todo: process incoming messages?
			}
		}
#endif
}

#ifdef INTERPROCESS
	void Enable(bool _en)
	{
		Enabled = _en;
		if(!_en) Shutdown();
		else Init();
	}
#endif

	void DrawLine(const Vector3f &_a, const Vector3f &_b, obColor _color, float _time)
	{
		Prof_Scope(InterProcess);
		{
			Prof(DrawLine);
#ifdef INTERPROCESS
			if(g_MessageQueue)
			{
				IPC_DebugDrawMsg msg;
				msg.m_Debugtype = DRAW_LINE;
				msg.m_Duration = Utils::SecondsToMilliseconds(_time);

				msg.data.m_Line.m_Start.x = _a.x;
				msg.data.m_Line.m_Start.y = _a.y;
				msg.data.m_Line.m_Start.z = _a.z;

				msg.data.m_Line.m_End.x = _b.x;
				msg.data.m_Line.m_End.y = _b.y;
				msg.data.m_Line.m_End.z = _b.z;

				msg.data.m_Line.m_Color = _color.rgba();
				g_MessageQueue->TrySend(msg);
			}
#else
			Init();

			if(g_ClientFuncs)
				g_ClientFuncs->DrawLine(_a, _b, _color.rgba(), Utils::SecondsToMilliseconds(_time));

#endif // INTERPROCESS
		}
	}

	void DrawRadius(const Vector3f &_a, float _radius, obColor _color, float _time)
	{
		Prof_Scope(InterProcess);
		{
			Prof(DrawRadius);
#ifdef INTERPROCESS
			if(g_MessageQueue)
			{
				IPC_DebugDrawMsg msg;
				msg.m_Debugtype = DRAW_RADIUS;
				msg.m_Duration = Utils::SecondsToMilliseconds(_time);

				msg.data.m_Radius.m_Pos.x = _a.x;
				msg.data.m_Radius.m_Pos.y = _a.y;
				msg.data.m_Radius.m_Pos.z = _a.z;

				msg.data.m_Radius.m_Radius = _radius;
				msg.data.m_Radius.m_Color = _color.rgba();
				g_MessageQueue->TrySend(msg);
			}
#else
			Init();

			if(g_ClientFuncs)
				g_ClientFuncs->DrawRadius(_a, _radius, _color.rgba(), Utils::SecondsToMilliseconds(_time));

#endif // INTERPROCESS
		}
	}

	void DrawBounds(const AABB &_aabb, obColor _color, float _time, AABB::Direction _dir)
	{
		Prof_Scope(InterProcess);
		{
			Prof(DrawBounds);
#ifdef INTERPROCESS
			if(g_MessageQueue)
			{
				IPC_DebugDrawMsg msg;
				msg.m_Debugtype = DRAW_BOUNDS;
				msg.m_Duration = Utils::SecondsToMilliseconds(_time);

				msg.data.m_AABB.m_Mins.x = _aabb.m_Mins[0];
				msg.data.m_AABB.m_Mins.y = _aabb.m_Mins[1];
				msg.data.m_AABB.m_Mins.z = _aabb.m_Mins[2];

				msg.data.m_AABB.m_Maxs.x = _aabb.m_Maxs[0];
				msg.data.m_AABB.m_Maxs.y = _aabb.m_Maxs[1];
				msg.data.m_AABB.m_Maxs.z = _aabb.m_Maxs[2];

				msg.data.m_AABB.m_Color = _color.rgba();
				msg.data.m_AABB.m_Sides = _dir;
				g_MessageQueue->TrySend(msg);
			}
#else
			Init();

			if(g_ClientFuncs)
				g_ClientFuncs->DrawAABB(_aabb.m_Mins, _aabb.m_Maxs, _color.rgba(), Utils::SecondsToMilliseconds(_time), _dir);

#endif // INTERPROCESS
		}
	}

	void DrawPolygon(const Vector3List &_vertices, obColor _color, float _time)
	{
		Prof_Scope(InterProcess);
		{
			Prof(DrawPolygon);
#ifdef INTERPROCESS
			if(g_MessageQueue)
			{
				IPC_DebugDrawMsg msg;
				msg.m_Debugtype = DRAW_POLYGON;
				msg.m_Duration = Utils::SecondsToMilliseconds(_time);

				msg.data.m_Polygon.m_NumVerts = (int)_vertices.size();
				if(msg.data.m_Polygon.m_NumVerts>IPC_DebugPolygonMessage::MaxPolyVerts)
					msg.data.m_Polygon.m_NumVerts = IPC_DebugPolygonMessage::MaxPolyVerts;

				for(int v = 0; v < msg.data.m_Polygon.m_NumVerts; ++v)
				{
					msg.data.m_Polygon.m_Verts[v].x = _vertices[v].x;
					msg.data.m_Polygon.m_Verts[v].y = _vertices[v].y;
					msg.data.m_Polygon.m_Verts[v].z = _vertices[v].z;
				}

				msg.data.m_Polygon.m_Color = _color.rgba();
				g_MessageQueue->TrySend(msg);
			}
#else
			Init();

			if(g_ClientFuncs)
				g_ClientFuncs->DrawPolygon((float*)_vertices.data(), (int)_vertices.size(), _color.rgba(), Utils::SecondsToMilliseconds(_time));

#endif // INTERPROCESS
		}
	}

	void DrawText(const Vector3f &_a, const char *_txt, obColor _color, float _time)
	{
		Prof_Scope(InterProcess);
		{
			Prof(DrawText);
#ifdef INTERPROCESS
			if(g_MessageQueue)
			{
				IPC_DebugDrawMsg msg;
				msg.m_Debugtype = DRAW_TEXT;
				msg.m_Duration = Utils::SecondsToMilliseconds(_time);

				msg.data.m_Text.m_Pos.x = _a.x;
				msg.data.m_Text.m_Pos.y = _a.y;
				msg.data.m_Text.m_Pos.z = _a.z;
				msg.data.m_Text.m_Color = _color.rgba();

				Utils::StringCopy(msg.data.m_Text.m_Buffer,_txt,IPC_DebugTextMessage::BufferSize);
				g_MessageQueue->TrySend(msg);
			}
#else
			Init();

			if(g_ClientFuncs)
				g_ClientFuncs->PrintScreenText(_a, Utils::SecondsToMilliseconds(_time), _color.rgba(), _txt);

#endif // INTERPROCESS
		}
	}
}
