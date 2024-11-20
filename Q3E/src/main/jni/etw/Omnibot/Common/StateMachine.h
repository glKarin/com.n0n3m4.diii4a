#ifndef __STATE_H__
#define __STATE_H__

#include "IGame.h"
#include "DebugWindow.h"

class Client;
class MessageHelper;
class CallbackParameters;

namespace gcn
{
	class Widget;
}

class UpdateDelay
{
public:

	obReal GetDelay() const { return m_UpdateDelay; }
	int GetDelayMsec() const { return Utils::SecondsToMilliseconds(m_UpdateDelay); }
	obReal GetRate() const { return m_UpdateDelay / 1000.f; }

	UpdateDelay(obReal _delay = 0) : m_UpdateDelay(_delay) {}
private:
	obReal m_UpdateDelay;
};

// class: State
//		Hierarchial FSM, modelled after the Halo 2 system, detailed here
//		http://www.gamasutra.com/gdc2005/features/20050311/isla_01.shtml
class State
{
public:
	friend class StateSimultaneous;
	friend class StatePrioritized;
	friend class StateSequential;
	friend class StateFirstAvailable;
	friend class gmScriptGoal;

	enum DebugIcon
	{
		Ico_Default,
		Ico_Warning,
		Ico_Error
	};

	enum StateStatus
	{
		State_Busy,
		State_Finished,
		//State_Failed,
	};

	enum StateFlags
	{
		State_InActive,				// Sate currently not running.
		State_Active,				// State currently running.
		State_WantsActive,			// State wants to run.
		State_DeleteOnFinished,		// State should be deleted when it exits.
		State_UserDisabled,			// State has been disabled by the user.
		State_UnSelectable,			// State not available for activation(class filter, etc).

		State_DebugDraw,
		State_DebugDontRender,		// Don't show this state on the debug render window.
		State_DebugExpanded,		// State is expanded on debug menu.
		State_DebugExpandOnActive,	// State should auto expand on debug menu when active.

		State_AlwaysRecieveEvents, // State should recieve event callbacks even when not currently active
		State_AlwaysRecieveSignals, // State should recieve event callbacks even when not currently active

		State_DontAutoAdd,
		State_ScriptGoal,			// This state is a script goal.

		State_StartUser = 24,
	};

	void AppendState(CHECK_PARAM_VALID State *_state);
	bool AppendTo(const char * _name, State *_insertstate);
	bool AppendTo(obuint32 _name, State *_insertstate);
	void PrependState(State *_state);
	bool PrependTo(const char * _name, State *_insertstate);
	bool PrependTo(obuint32 _name, State *_insertstate);
	State *ReplaceState(const char * _name, State *_insertstate);
	bool InsertAfter(const char * _name, State *_insertstate);
	bool InsertAfter(obuint32 _name, State *_insertstate);
	bool InsertBefore(const char * _name, State *_insertstate);
	bool InsertBefore(obuint32 _name, State *_insertstate);
	State *RemoveState(const char * _name);
	void DeleteGoalScripts();

	void SetClient(Client *_client);
	void SetName(const char *_name);

	State *FindState(const char *_name);
	State *FindState(obuint32 _namehash);

	virtual obReal GetPriority() { return (obReal)1.0; }
	virtual bool OnPathThrough(const String &_s) { return false; }
	virtual void EndPathThrough() { }
	inline obReal GetLastPriority() const { return m_LastPriority; }
	inline void SetLastPriority(obReal _p) { m_LastPriority = _p; }
	
	State *GetParent() const { return m_Parent; }
	State *GetRootState() const { return m_Root; }
	State *GetFirstChild() const { return m_FirstChild; }
	State *GetSibling() const { return m_Sibling; }

	virtual State *GetActiveState() const { return NULL; }
	
	void RootUpdate();

	virtual void Initialize() {}
	virtual void Enter() {}
	virtual void Exit() {}
	virtual StateStatus Update(float fDt) { return State_Busy; };
	virtual void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb) {}

	void ExitAll();

	enum noSelectReason_t {
		NoSelectReasonNone,

		NoSelectReason_OnlyClass,
		NoSelectReason_OnlyTeam,
		NoSelectReason_OnlyPowerup,
		NoSelectReason_OnlyNoPowerup,
		NoSelectReason_OnlyEntFlag,
		NoSelectReason_OnlyNoEntFlag,
		NoSelectReason_OnlyRole,
		NoSelectReason_OnlyWeapon,
		NoSelectReason_OnlyWeaponNoAmmo,
		NoSelectReason_OnlyTarget,
		NoSelectReason_OnlyTargetClass,
		NoSelectReason_OnlyTargetTeam,
		NoSelectReason_OnlyTargetWeapon,
		NoSelectReason_OnlyTargetPowerup,
		NoSelectReason_OnlyTargetNoPowerup,
		NoSelectReason_OnlyTargetEntFlag,
		NoSelectReason_OnlyTargetNoEntFlag,
		NoSelectReason_LimitCallback,
	};
	noSelectReason_t CanBeSelected();

	virtual StateStatus UpdateState(float fDt) = 0;

	inline bool IsActive() const { return m_StateFlags.CheckFlag(State_Active); }
	inline bool IsUserDisabled() const { return m_StateFlags.CheckFlag(State_UserDisabled); }
	inline bool IsSelectable() const { return !m_StateFlags.CheckFlag(State_UnSelectable); }
	inline bool IsDisabled() const { return IsUserDisabled() || !IsSelectable(); }
	inline bool IsScriptGoal() const { return m_StateFlags.CheckFlag(State_ScriptGoal); }
	inline bool IsAutoAdd() const { return !m_StateFlags.CheckFlag(State_DontAutoAdd); }
	
	inline void SetScriptGoal(bool _b) { m_StateFlags.SetFlag(State_ScriptGoal, _b); }
	inline void SetAutoAdd(bool _b) { m_StateFlags.SetFlag(State_DontAutoAdd, !_b); }
	inline void SetUserDisabled(bool _b) { m_StateFlags.SetFlag(State_UserDisabled, _b); }

	inline bool CheckFlag(obint32 _flag) { return m_StateFlags.CheckFlag(_flag); }
	inline void SetFlag(obint32 _flag) { m_StateFlags.SetFlag(_flag); }

	virtual void SetSelectable(bool _selectable);
	virtual void SetEnable(bool _enable, const char *_error = 0);

	DebugIcon GetDebugIcon() const { return m_DebugIcon; }

	void InitializeStates();

	inline bool DebugDrawingEnabled() const { return m_StateFlags.CheckFlag(State_DebugDraw); }
	inline void DebugDraw(bool _draw) { m_StateFlags.SetFlag(State_DebugDraw, _draw); }

	inline bool IsDebugExpanded() { return m_StateFlags.CheckFlag(State_DebugExpanded); }
	inline void DebugExpand(bool b) { m_StateFlags.SetFlag(State_DebugExpanded, b); }

	inline void ToggleDebugDraw() { DebugDraw(!DebugDrawingEnabled()); }

	inline bool DontDrawDebugWindow() const { return m_StateFlags.CheckFlag(State_DebugDontRender); }

	inline obReal GetStateTime() const { return m_StateTime != 0.f ? IGame::GetTimeSecs()-m_StateTime : 0.f; }
	inline obReal GetUserStateTime() const { return m_StateTimeUser != 0.f ? IGame::GetTimeSecs()-m_StateTimeUser : 0.f; }
	inline void ResetStateTimeUser() { m_StateTimeUser = IGame::GetTimeSecs(); }

	void SetAlwaysRecieveEvents(bool _b) { m_StateFlags.SetFlag(State::State_AlwaysRecieveEvents, _b); }
	bool AlwaysRecieveEvents() const { return m_StateFlags.CheckFlag(State::State_AlwaysRecieveEvents); }

	bool IsRoot() { return !m_Parent && !m_Root; }
	void FixRoot();

	String GetName() const;
	inline obuint32 GetNameHash() const { return m_NameHash; }

	Client *GetClient() const { return m_Client; }

	// Filters
	BitFlag32 &LimitToClass() { return m_OnlyClass; }
	BitFlag32 &LimitToTeam() { return m_OnlyTeam; }
	BitFlag32 &LimitToRole() { return m_OnlyRole; }
	BitFlag64 &LimitToPowerup() { return m_OnlyPowerUp; }
	BitFlag64 &LimitToEntFlag() { return m_OnlyEntFlag; }
	
	BitFlag64 &LimitToNoEntFlag() { return m_OnlyNoEntFlag; }
	BitFlag64 &LimitToNoPowerup() { return m_OnlyNoPowerUp; }
	
	BitFlag128 &LimitToWeapon() { return m_OnlyWeapon; }	

	BitFlag32 &LimitToTargetClass() { return m_OnlyTargetClass; }
	BitFlag32 &LimitToTargetTeam() { return m_OnlyTargetTeam; }
	BitFlag64 &LimitToTargetPowerup() { return m_OnlyTargetPowerUp; }
	BitFlag64 &LimitToTargetNoPowerup() { return m_OnlyTargetNoPowerUp; }
	BitFlag64 &LimitToTargetEntFlag() { return m_OnlyTargetEntFlag; }
	BitFlag64 &LimitToTargetNoEntFlag() { return m_OnlyTargetNoEntFlag; }
	BitFlag32 &LimitToTargetWeapon() { return m_OnlyTargetWeapon; }

	void LimitTo(const gmVariable &varThis, gmGCRoot<gmFunctionObject> &_fn, int _delay, bool _onlywhenactive);
	void ClearLimitTo();

	void BlackboardDelay(float _delayseconds, int _targetId);
	bool BlackboardIsDelayed(int _targetId);

	void CheckForCallbacks(const MessageHelper &_message, CallbackParameters &_cb);
	void SignalThreads(const gmVariable &_signal);

	void AddForkThreadId(int _threadId);
	bool DeleteForkThread(int _threadId);
	bool RemoveThreadReference(const int * _threadId, int _numThreadIds);

	void PropogateDeletedThreads(const int *_threadIds, int _numThreads);

	bool StateCommand(const StringVector &_args);

	virtual gmUserObject *GetScriptObject(gmMachine *_machine) { return NULL; }
	
	// Special case callbacks.
	virtual void OnSpawn();

	// Debug
	virtual void RenderDebug() {}
	virtual void GetDebugString(StringStr &out) {}
	virtual MapGoal *GetMapGoalPtr() { return NULL; }

#ifdef ENABLE_DEBUG_WINDOW
	virtual void RenderDebugWindow(gcn::DrawInfo drawinfo);
#endif

#ifdef ENABLE_REMOTE_DEBUGGING
	virtual void Sync( RemoteLib::DataBuffer & db, bool fullSync, const char * statePath );
#endif

	//////////////////////////////////////////////////////////////////////////
	struct LimitToCallback
	{
		gmGCRoot<gmFunctionObject>	m_LimitTo;
		int							m_NextCallback;
		int							m_Delay;
		gmVariable					m_This;
		bool						m_OnlyWhenActive;
		bool						m_Result;

		LimitToCallback() 
			: m_LimitTo(0)
			, m_NextCallback(0)
			, m_Delay(0)
			, m_This(gmVariable::s_null)
			, m_OnlyWhenActive(false)
			, m_Result(false)
		{}
	};
	int gmfLimitToClass(gmThread *a_thread);
	int gmfLimitToTeam(gmThread *a_thread);
	int gmfLimitToPowerUp(gmThread *a_thread);
	int gmfLimitToEntityFlag(gmThread *a_thread);
	int gmfLimitToWeapon(gmThread *a_thread);

	int gmfLimitToNoTarget(gmThread *a_thread);
	int gmfLimitToTargetClass(gmThread *a_thread);
	int gmfLimitToTargetTeam(gmThread *a_thread);
	int gmfLimitToTargetPowerUp(gmThread *a_thread);
	int gmfLimitToTargetEntityFlag(gmThread *a_thread);
	//////////////////////////////////////////////////////////////////////////

	State(const char * _name, const UpdateDelay &_ur = UpdateDelay(0));
	virtual ~State();
protected:
	
	obReal InternalGetPriority();
	void InternalEnter();
	virtual void InternalExit();
	virtual void InternalParentExit() {}
	StateStatus InternalUpdateState();

	void InternalProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);
	virtual void InternalSignal(const gmVariable &_signal) {}

	State *FindStateRecurse(obuint32 _namehash);

	BitFlag32		m_StateFlags;

	// limitations
	BitFlag32		m_OnlyClass;
	BitFlag32		m_OnlyTeam;
	BitFlag32		m_OnlyRole;
	BitFlag64		m_OnlyPowerUp;
	BitFlag64		m_OnlyNoPowerUp;
	BitFlag64		m_OnlyEntFlag;
	BitFlag64		m_OnlyNoEntFlag;
	BitFlag128		m_OnlyWeapon;

	BitFlag32		m_OnlyTargetClass;
	BitFlag32		m_OnlyTargetTeam;
	BitFlag32		m_OnlyTargetWeapon;
	BitFlag64		m_OnlyTargetPowerUp;
	BitFlag64		m_OnlyTargetEntFlag;

	BitFlag64		m_OnlyTargetNoPowerUp;
	BitFlag64		m_OnlyTargetNoEntFlag;

	LimitToCallback	m_LimitCallback;

	State			*m_Sibling;
	State			*m_Parent;
	State			*m_FirstChild;
	State			*m_Root;

	enum { MaxThreads = 128 };
	int				m_NumThreads;
	int				m_ThreadList[MaxThreads];
	
	gmGCRoot<gmTableObject>		m_EventTable;
	gmGCRoot<gmTableObject>		m_CommandTable;
	gmGCRoot<gmUserObject>		m_ScriptObject;
private:
	void ClearThreadReference(int index);

	Client			*m_Client;
	
	obint32			m_NextUpdate;
	obint32			m_LastUpdateTime;
	obReal			m_StateTime;
	obReal			m_StateTimeUser;
	obReal			m_LastPriority;
	obint32			m_LastPriorityTime;
	UpdateDelay		m_UpdateRate;

	obuint32		m_NameHash;

	DebugIcon		m_DebugIcon;

	obuint32		m_SyncCrc;

#ifdef _DEBUG
	String			m_DebugName;
#endif

	State();
};

//////////////////////////////////////////////////////////////////////////

// class: StateSimultaneous
//		Runs all non zero priority states at the same time.
class StateSimultaneous : public State
{
public:

	obReal GetPriority();
	StateStatus UpdateState(float fDt);

	StateSimultaneous(const char * _name, const UpdateDelay &_ur = UpdateDelay());
protected:
private:
};

//////////////////////////////////////////////////////////////////////////

// class: StateFirstAvailable
//		List of states. First one that can run does, but it can
//		be interrupted by higher priority states.
class StateFirstAvailable : public State
{
public:

	void GetDebugString(StringStr &out);

	obReal GetPriority();
	void InternalParentExit();
	StateStatus UpdateState(float fDt);

	StateFirstAvailable(const char * _name, const UpdateDelay &_ur = UpdateDelay());
protected:
private:
	State	*m_CurrentState;
};

//////////////////////////////////////////////////////////////////////////

// class: StatePrioritized
//		Prioritized List of states. Highest priority runs, but it can
//		be interrupted by higher priority states.
class StatePrioritized : public State
{
public:

	void GetDebugString(StringStr &out);

	obReal GetPriority();
	void InternalParentExit();
	StateStatus UpdateState(float fDt);

	virtual State *GetActiveState() const { return m_CurrentState; }

	StatePrioritized(const char * _name, const UpdateDelay &_ur = UpdateDelay());
protected:
private:
	State	*m_CurrentState;
};

//////////////////////////////////////////////////////////////////////////

// class: StateSequential
//		Run each child in order, skipping any that have priotity 0. 
//		When all children finish running, the parent state is finished.
//		Fail if any children fail.
class StateSequential : public State
{
public:

	void GetDebugString(StringStr &out);

	void Exit();
	StateStatus UpdateState(float fDt);

	virtual State *GetActiveState() const { return m_CurrentState; }

	StateSequential(const char * _name, const UpdateDelay &_ur = UpdateDelay());
protected:
private:
	State	*m_CurrentState;
};

//////////////////////////////////////////////////////////////////////////

// class: StateSequentialLooping
//		Run each child in order, skipping any that have priotity 0. 
//		When all children finish running, it starts over.
class StateSequentialLooping : public State
{
public:

	StateStatus UpdateState(float fDt);

	//virtual State *GetActiveState() const { return m_CurrentState; }

	StateSequentialLooping(const char * _name, const UpdateDelay &_ur = UpdateDelay());
protected:
private:
};

//////////////////////////////////////////////////////////////////////////

// class: StateProbabilistic
//		Choose a random child state to run.
class StateProbabilistic : public State
{
public:

	StateStatus UpdateState(float fDt);

	//virtual State *GetActiveState() const { return m_CurrentState; }

	StateProbabilistic(const char * _name, const UpdateDelay &_ur = UpdateDelay());
protected:
private:
};

//////////////////////////////////////////////////////////////////////////

// class: StateOneOff
//		Pick a random child state to run, but never repeat the same one twice.
class StateOneOff : public State
{
public:

	StateStatus UpdateState(float fDt);

	//virtual State *GetActiveState() const { return m_CurrentState; }

	StateOneOff(const char * _name, const UpdateDelay &_ur = UpdateDelay(0));
protected:
private:
};

//////////////////////////////////////////////////////////////////////////

// class: StateChild
//		Pick a random child state to run, but never repeat the same one twice.
class StateChild : public State
{
public:

	StateStatus UpdateState(float fDt);

	//virtual State *GetActiveState() const { return m_CurrentState; }

	StateChild(const char * _name, const UpdateDelay &_ur = UpdateDelay(0));
protected:
private:
};

//////////////////////////////////////////////////////////////////////////

#endif
