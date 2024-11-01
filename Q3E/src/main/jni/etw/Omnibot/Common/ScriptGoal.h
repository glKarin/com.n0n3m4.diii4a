#ifndef __ScriptGoal_H__
#define __ScriptGoal_H__

#include "StateMachine.h"
#include "Path.h"
#include "ScriptManager.h"
#include "TriggerManager.h"
#include "Criteria.h"

class gmScriptGoal;

namespace AiState
{
	struct MoveOptions 
	{
		float		Radius;
		int			ThreadId;
		MoveMode	Mode;

		enum { MaxAvoidPts=32 };
		Vector3f	Avoid[MaxAvoidPts];
		int			NumAvoid;

		void FromTable(gmMachine *a_machine, gmTableObject *a_table);

		MoveOptions();
	};

	class ScriptGoal : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		friend class gmScriptGoal;
		friend class ScriptGoals;

		enum FunctionCallback 
		{
			ON_INIT,
			ON_SPAWN,
			ON_GETPRIORITY,
			ON_ENTER,
			ON_EXIT,
			ON_UPDATE,
			ON_PATH_THROUGH,
			NUM_CALLBACKS
		};

		void GetDebugString(StringStr &out);
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void SetEnable(bool _enable, const char *_error = 0);
		void SetSelectable(bool _selectable);

		bool AddScriptAimRequest(Priority::ePriority _prio, Aimer::AimType _type, Vector3f _v);

		void Signal(const gmVariable &_var);
		void KillAllGoalThreads();

		obReal GetScriptPriority() const { return m_ScriptPriority; }
		void SetScriptPriority(obReal _p) { m_ScriptPriority = _p; }
		void SetFinished() { m_Finished = true; }

		bool OnPathThrough(const String &_s);
		void EndPathThrough();

		gmGCRoot<gmFunctionObject> GetCallback(FunctionCallback cb) { return m_Callbacks[cb]; }
		void SetCallback(FunctionCallback cb, gmGCRoot<gmFunctionObject> f) { m_Callbacks[cb] = f; }
		void RunCallback(FunctionCallback callback, bool whenNotActive = false);

		// functionality
		bool Goto(const Vector3f &_pos, const MoveOptions &options);
		bool Goto(const Vector3List &_vectors, const MoveOptions &options);
		bool GotoRandom(const MoveOptions &options);
		bool RouteTo(MapGoalPtr mg, const MoveOptions &options);
		void Stop();

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);
		void OnPathSucceeded();
		void OnPathFailed(FollowPathUser::FailType _how);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		const Vector3f &GetAimVector() const { return m_AimVector; }
		void SetAimVector(const Vector3f &_v) { m_AimVector = _v; m_AimSignalled = false; }
		obint32 GetAimWeaponId() const { return m_AimWeaponId; }
		void SetAimWeaponId(obint32 id) { m_AimWeaponId = id; }

		void SetGetPriorityDelay(obint32 _delay) { m_NextGetPriorityDelay = _delay; }
		obint32 GetGetPriorityDelay() const { return m_NextGetPriorityDelay; }
		void DelayGetPriority(obint32 _delay) { m_NextGetPriorityUpdate = IGame::GetTime() + _delay; }

		void AutoReleaseAim(bool _b) { m_AutoReleaseAim = _b; }
		bool AutoReleaseAim() { return m_AutoReleaseAim; }

		void AutoReleaseWeapon(bool _b) { m_AutoReleaseWpn = _b; }
		bool AutoReleaseWeapon() { return m_AutoReleaseWpn; }
		
		void AutoReleaseTracker(bool _b) { m_AutoReleaseTracker = _b; }
		bool AutoReleaseTracker() { return m_AutoReleaseTracker; }

		void AutoFinishOnUnAvailable(bool _b) { m_AutoFinishOnUnavailable = _b; }
		bool AutoFinishOnUnAvailable() const { return m_AutoFinishOnUnavailable; }

		void AutoFinishOnNoProgressSlots(bool _b) { m_AutoFinishOnNoProgressSlots = _b; }
		bool AutoFinishOnNoProgressSlots() const { return m_AutoFinishOnNoProgressSlots; }

		void AutoFinishOnNoUseSlots(bool _b) { m_AutoFinishOnNoUseSlots = _b; }
		bool AutoFinishOnNoUseSlots() const { return m_AutoFinishOnNoUseSlots; }
		
		void SkipGetPriorityWhenActive(bool _b) { m_SkipGetPriorityWhenActive = _b; }
		bool SkipGetPriorityWhenActive() const { return m_SkipGetPriorityWhenActive; }
		
		bool AddFinishCriteria(const CheckCriteria &_crit);
		void ClearFinishCriteria(bool _clearpersistent = false);

		void WatchForEntityCategory(float radius, const BitFlag32 &category, int customTrace);
		void UpdateEntityInRadius();

		void WatchForMapGoalsInRadius(const GoalManager::Query &qry, const GameEntity & ent, float radius);
		void ClearWatchForMapGoalsInRadius();
		void UpdateMapGoalsInRadius();

		// Special case callbacks.
		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);
		bool OnInit(gmMachine *_machine);
		void OnSpawn();
		void OnException();

		//ThreadList &GetThreadList() { return m_ThreadList; }

		MapGoalPtr &GetMapGoal() { return m_MapGoal; }
		void SetMapGoal(MapGoalPtr &_mg) { m_MapGoal = _mg; m_Tracker.Reset(); }
		MapGoal *GetMapGoalPtr() { return m_MapGoal.get(); }

		void SetParentName(const char *_str);
		String GetParentName() const;
		obuint32 GetParentNameHash() const { return m_ParentNameHash; }

		void SetInsertBeforeName(const char *_str);
		String GetInsertBeforeName() const;
		obuint32 GetInsertBeforeHash() const { return m_InsertBeforeHash; }

		void SetInsertAfterName(const char *_str);
		String GetInsertAfterName() const;
		obuint32 GetInsertAfterHash() const { return m_InsertAfterHash; }

		gmUserObject *GetScriptObject(gmMachine *_machine);

		//////////////////////////////////////////////////////////////////////////
		int gmfFinished(gmThread *a_thread);
		int gmfGoto(gmThread *a_thread);
		int gmfGotoAsync(gmThread *a_thread);
		int gmfAddAimRequest(gmThread *a_thread);
		int gmfReleaseAimRequest(gmThread *a_thread);
		int gmfAddWeaponRequest(gmThread *a_thread);
		int gmfReleaseWeaponRequest(gmThread *a_thread);
		int gmfUpdateWeaponRequest(gmThread *a_thread);
		int gmfBlockForWeaponChange(gmThread *a_thread);
		int gmfBlockForWeaponFire(gmThread *a_thread);
		int gmfBlockForVoiceMacro(gmThread *a_thread);
		int gmfThreadFork(gmThread *a_thread);
		int gmfSignal(gmThread *a_thread);

		//////////////////////////////////////////////////////////////////////////

		gmGCRoot<gmStringObject> &DebugString() { return m_DebugString; }

		//////////////////////////////////////////////////////////////////////////

		bool MarkInProgress(MapGoalPtr _p);
		bool MarkInUse(MapGoalPtr _p);

		//////////////////////////////////////////////////////////////////////////
		void SetProfilerZone(const String &_name);

		ScriptGoal *Clone();
		ScriptGoal(const char *_name);
		~ScriptGoal();
	protected:
		void InternalSignal(const gmVariable &_signal);
		void InternalExit() ;
	private:
		Vector3f					m_AimVector;
		obint32						m_AimWeaponId;

		Aimer::AimType				m_ScriptAimType;

		obReal						m_ScriptPriority;
		obReal						m_MinRadius;

		gmGCRoot<gmFunctionObject>	m_Callbacks[NUM_CALLBACKS];		
		ThreadScoper				m_ActiveThread[NUM_CALLBACKS];

		gmGCRoot<gmStringObject>	m_DebugString;

		obuint32					m_ParentNameHash;
		obuint32					m_InsertBeforeHash;
		obuint32					m_InsertAfterHash;		

		obint32						m_NextGetPriorityUpdate;
		obint32						m_NextGetPriorityDelay;
		
		struct WatchEntity
		{
			enum { MaxEntities=64 };
			float		m_Radius;
			BitFlag32	m_Category;
			int			m_CustomTrace;
			struct KnownEnt
			{
				GameEntity		m_Ent;
				int				m_TimeStamp;

				KnownEnt() { Reset(); }
				void Reset() { m_Ent.Reset(); m_TimeStamp = 0; }
			} m_Entry[MaxEntities];

			WatchEntity() : m_Radius(0.f), m_CustomTrace(0) {}
		} m_WatchEntities;

		enum { MaxCriteria=8 };
		CheckCriteria				m_FinishCriteria[MaxCriteria];

		typedef std::set<MapGoalPtr> MgSet;
		struct WatchForMapGoal
		{
			GameEntity			m_PositionEnt;
			float				m_Radius;

			MgSet				m_InRadius;
			GoalManager::Query	m_Query;

			WatchForMapGoal() : m_Radius(0.f) {}
		}							m_MapGoalInRadius;

		bool						m_Finished : 1;
		bool						m_AimSignalled : 1;

		bool						m_AutoReleaseAim : 1;
		bool						m_AutoReleaseWpn : 1;
		bool						m_AutoReleaseTracker : 1;
		bool						m_AutoFinishOnUnavailable : 1;
		bool						m_AutoFinishOnNoProgressSlots : 1;
		bool						m_AutoFinishOnNoUseSlots : 1;
		bool						m_SkipGetPriorityWhenActive : 1;

		bool						m_SkipLastWp : 1;

		MapGoalPtr					m_MapGoal;
		MapGoalPtr					m_MapGoalRoute;
		Trackers					m_Tracker;

#ifdef Prof_ENABLED
		Prof_Zone					*m_ProfZone;
#endif

		ScriptGoal();
	};

}

#endif

