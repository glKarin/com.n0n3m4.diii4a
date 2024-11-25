#include "PrecompCommon.h"
#include "StateMachine.h"
#include "BotTargetingSystem.h"
#include "DebugWindow.h"

State::State(const char * _name, const UpdateDelay &_ur)
	: m_Sibling(0)
	, m_Parent(0)
	, m_FirstChild(0)
	, m_Root(0)
	, m_NumThreads(0)
	, m_Client(0)
	, m_NextUpdate(0)
	, m_LastUpdateTime(0)
	, m_StateTime(0.f)
	, m_StateTimeUser(0.f)
	, m_LastPriority(0.f)
	, m_LastPriorityTime(0)
	, m_UpdateRate(_ur)
	, m_NameHash(0)
	, m_DebugIcon(Ico_Default)
	, m_SyncCrc( 0 )
{
	SetName(_name);
	DebugExpand(true);

	for(int i = 0; i < MaxThreads; ++i)
		m_ThreadList[i] = GM_INVALID_THREAD;
}

State::~State()
{
	while(m_FirstChild)
	{
		State *pSt = m_FirstChild;
		m_FirstChild = pSt->m_Sibling;
		delete pSt;
	}
}

void State::SetName(const char *_name)
{
	m_NameHash = Utils::MakeHash32(_name);
#ifdef _DEBUG
	m_DebugName = _name ? _name : "";
#endif
}

String State::GetName() const
{
	return Utils::HashToString(GetNameHash());
}

void State::AppendState(State *_state)
{
	OBASSERT(_state, "AppendState: No State Given");
	_state->m_Parent = this;
	if(m_FirstChild)
	{
		State *pLastState = m_FirstChild;
		while(pLastState && pLastState->m_Sibling)
			pLastState = pLastState->m_Sibling;
		
		pLastState->m_Sibling = _state;
	}
	else
	{
		m_FirstChild = _state;
	}

	_state->m_Sibling = NULL;	
}

bool State::AppendTo(const char * _name, State *_insertstate)
{
	return AppendTo(Utils::Hash32(_name), _insertstate);
}

bool State::AppendTo(obuint32 _name, State *_insertstate)
{
	if(!_name)
		return false;

	State *pFoundState = FindState(_name);
	if(pFoundState)
	{
		pFoundState->AppendState(_insertstate);
		_insertstate->m_Root = pFoundState->m_Root;
		return true;
	}
	delete _insertstate;
	return false;
}

void State::PrependState(State *_state)
{
	_state->m_Sibling = m_FirstChild;
	m_FirstChild = _state;

	_state->m_Parent = this;
}

bool State::PrependTo(const char * _name, State *_insertstate)
{
	return PrependTo(Utils::Hash32(_name), _insertstate);
}

bool State::PrependTo(obuint32 _name, State *_insertstate)
{
	if(!_name)
		return false;

	State *pFoundState = FindState(_name);
	if(pFoundState)
	{
		pFoundState->PrependState(_insertstate);
		_insertstate->m_Root = pFoundState->m_Root;
		return true;
	}
	delete _insertstate;
	return false;
}

State *State::ReplaceState(const char * _name, State *_insertstate)
{
	State *pReplaceState = FindState(_name);
	if(pReplaceState)
	{
		State *pLastState = NULL;
		for(State *pState = pReplaceState->m_Parent->m_FirstChild; 
			pState; 
			pState = pState->m_Sibling)
		{
			if(pState == pReplaceState)
			{
				if(pState->m_Parent && pState->m_Parent->m_FirstChild == pState)
					pState->m_Parent->m_FirstChild = _insertstate;

				// splice it out
				if(pLastState)
					pLastState->m_Sibling = _insertstate;
				_insertstate->m_Sibling = pState->m_Sibling;
				_insertstate->m_Parent = pState->m_Parent;
				_insertstate->m_Root = pState->m_Root;

				// fix the old one and return it
				pState->m_Parent = 0;
				pState->m_Sibling = 0;

				return pState;
			}
			pLastState = pState;
		}
	}
	return _insertstate;
}

bool State::InsertAfter(const char * _name, State *_insertstate)
{
	return InsertAfter(Utils::Hash32(_name), _insertstate);
}

bool State::InsertAfter(obuint32 _name, State *_insertstate)
{
	if(!_name)
		return false;

	State *pFoundState = FindState(_name);
	if(pFoundState)
	{
		// splice it in
		_insertstate->m_Sibling = pFoundState->m_Sibling;
		_insertstate->m_Parent = pFoundState->m_Parent;
		_insertstate->m_Root = pFoundState->m_Root;
		pFoundState->m_Sibling = _insertstate;
		return true;
	}
	delete _insertstate;
	return false;
}

bool State::InsertBefore(const char * _name, State *_insertstate)
{
	return InsertBefore(Utils::Hash32(_name), _insertstate);
}

bool State::InsertBefore(obuint32 _name, State *_insertstate)
{
	if(!_name)
		return false;

	State *pFoundState = FindState(_name);
	if(pFoundState)
	{
		_insertstate->m_Parent = pFoundState->m_Parent;
		_insertstate->m_Root = pFoundState->m_Root;

		if(pFoundState->m_Parent->m_FirstChild == pFoundState)
		{
			_insertstate->m_Sibling = pFoundState;
			pFoundState->m_Parent->m_FirstChild = _insertstate;
			return true;
		}
		else
		{
			for(State *pS = pFoundState->m_Parent->m_FirstChild; 
				pS; 
				pS = pS->m_Sibling)
			{
				if(pS->m_Sibling == pFoundState)
				{
					pS->m_Sibling = _insertstate;
					_insertstate->m_Sibling = pFoundState;
					return true;
				}
			}
		}
	}
	delete _insertstate;
	return false;
}

State *State::RemoveState(const char * _name)
{
	State *pDeleteState = FindState(_name);
	if(pDeleteState)
	{
		pDeleteState->InternalExit();
			
		State *pLastState = NULL;
		for(State *pState = pDeleteState->m_Parent->m_FirstChild; 
			pState; 
			pState = pState->m_Sibling)
		{
			if(pState == pDeleteState)
			{
				if(pState->m_Parent && pState->m_Parent->m_FirstChild == pState)
					pState->m_Parent->m_FirstChild = pState->m_Sibling;

				if(pLastState)
					pLastState->m_Sibling = pState->m_Sibling;

				// fix the old one and return it
				pState->m_Parent = 0;
				pState->m_Sibling = 0;

				return pState;
			}
			pLastState = pState;
		}
	}
	return 0;
}

void State::InitializeStates()
{
	Initialize();
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		pState->InitializeStates();
}

void State::FixRoot()
{
	// Find the root state
	m_Root = GetParent();
	while(m_Root != NULL && m_Root->m_Parent)
		m_Root = m_Root->m_Parent;
	OBASSERT(!m_Parent || m_Root, "No Root State");

	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		pState->FixRoot();
}

void State::SetClient(Client *_client)
{
	OBASSERT(_client, "No Client!");

	m_Client = _client;

	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		pState->SetClient(_client);
}

State *State::FindState(const char *_name)
{
	return FindState(Utils::Hash32(_name));
}

State *State::FindStateRecurse(obuint32 _namehash)
{
	if(m_NameHash == _namehash)
		return this;

	State *ptr = NULL;
	for(State *pState = m_FirstChild; pState && !ptr; pState = pState->m_Sibling)
		ptr = pState->FindStateRecurse(_namehash);
	return ptr;
}

State *State::FindState(obuint32 _namehash)
{
	return FindStateRecurse(_namehash);
}

void State::RootUpdate()
{
	Prof(RootUpdate);
	if(!IsActive())
		InternalEnter();
	InternalUpdateState();
}

obReal State::InternalGetPriority()
{
	if(m_LastPriorityTime < IGame::GetTime())
	{
		const noSelectReason_t rsn = CanBeSelected();
		SetSelectable( rsn == NoSelectReasonNone );
		m_LastPriority = 
			!IsDisabled() //&&
			/*IsSelectable() && 
			!IsUserDisabled() &&*/
			//(GetCurrentPriority()!=0.f) 
			? GetPriority() : 0.f;
		m_LastPriorityTime = IGame::GetTime();
	}	
	return m_LastPriority;
}

void State::InternalEnter()
{
	OBASSERT(!IsActive(), "Entering Active State!");
	//Utils::OutputDebug(kInfo,"%s: State: %s Enter (%d)\n", GetClient()->GetName(), GetName().c_str(),IGame::GetTime());

	if(m_LimitCallback.m_OnlyWhenActive) m_LimitCallback.m_Result = true;

	m_StateTime = m_StateTimeUser = IGame::GetTimeSecs();
	m_StateFlags.SetFlag(State_Active, true);
	Enter();

	if(m_StateFlags.CheckFlag(State_DebugExpandOnActive))
		DebugExpand(true);
}

void State::InternalExit() 
{
	if(IsActive())
	{
		//Utils::OutputDebug(kInfo,"%s: State: %s Exit (%d)\n", GetClient()->GetName(), GetName().c_str(),IGame::GetTime());

		// exit all child states
		for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
			pState->InternalExit();

		m_StateTime = m_StateTimeUser = 0.f;
		SetLastPriority( 0.0f );
		m_StateFlags.SetFlag(State_Active, false);

		InternalParentExit();
		Exit();	

		if(m_StateFlags.CheckFlag(State_DebugExpandOnActive))
			DebugExpand(false);
	}
}

State::StateStatus State::InternalUpdateState()
{
	if(DebugDrawingEnabled())
		RenderDebug();

	if(m_NextUpdate <= IGame::GetTime())
	{
		const int iMsPassed = IGame::GetTime() - m_LastUpdateTime;
		float fDt = (float)iMsPassed / 1000.f;

		m_NextUpdate = IGame::GetTime() + m_UpdateRate.GetDelayMsec();
		m_LastUpdateTime = IGame::GetTime();

		return UpdateState(fDt);
	}
	return State_Busy;
}

void State::ExitAll()
{
	InternalExit();
}

void State::CheckForCallbacks(const MessageHelper &_message, CallbackParameters &_cb)
{
	if(IsRoot() || IsActive() || AlwaysRecieveEvents())
		InternalProcessEvent(_message, _cb);

	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		pState->CheckForCallbacks(_message, _cb);
}

void State::SignalThreads(const gmVariable &_signal)
{
	if(!IsRoot() && !IsActive() && !m_StateFlags.CheckFlag(State_AlwaysRecieveSignals))
		return;

	Prof(SignalThreads);

	InternalSignal(_signal);
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		pState->SignalThreads(_signal);
}

void State::InternalProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	SignalThreads(gmVariable(_message.GetMessageId()));

	/*switch(_message.GetMessageId())
	{
		HANDLER(SYSTEM_THREAD_CREATED)
		{
			const Event_SystemThreadCreated *m = _message.Get<Event_SystemThreadCreated>();
			break;
		}
		HANDLER(SYSTEM_THREAD_DESTROYED)
		{
			const Event_SystemThreadDestroyed *m = _message.Get<Event_SystemThreadDestroyed>();
			RemoveThreadReference(m->m_ThreadId);
			break;
		}
	}*/

	// Attempt script callbacks.
	const bool eventRelevent = _cb.GetTargetState()==0 || _cb.GetTargetState() == GetNameHash();
	if(m_EventTable && eventRelevent)
	{
		gmVariable callback = m_EventTable->Get(_cb.GetMessageId());
		if(gmFunctionObject *pFunc = callback.GetFunctionObjectSafe())
		{
			OBASSERT(GetScriptObject(_cb.GetMachine()),"No Script Object!");
			gmVariable varThis = gmVariable(GetScriptObject(_cb.GetMachine()));
			int ThreadId = _cb.CallFunction(pFunc, varThis, !_cb.CallImmediate());
			
			// add it to the tracking list for management of its lifetime.
			// don't add thread if AlwaysRecieveEvents=true, because events REVIVED, CHANGETEAM, DEATH would not be executed
			if(ThreadId != GM_INVALID_THREAD && !AlwaysRecieveEvents())
				AddForkThreadId(ThreadId);
		}
	}
	ProcessEvent(_message,_cb);
}

void State::AddForkThreadId(int _threadId)
{
	int freeIndex = -1;
	for(int i = 0; i < m_NumThreads; ++i)
	{
		if(m_ThreadList[i] == GM_INVALID_THREAD)
		{
			if(freeIndex == -1)
				freeIndex = i;
			continue;
		}
		if(m_ThreadList[i] == _threadId)
			return;
	}
	if(freeIndex < 0 && m_NumThreads < MaxThreads)
	{
		freeIndex = m_NumThreads++;
	}
	OBASSERT(freeIndex != -1,"No Free Slots in m_ThreadList, max %d", MaxThreads);
	if(freeIndex != -1)
	{
		m_ThreadList[freeIndex] = _threadId;
	}
}

void State::ClearThreadReference(int index)
{
	m_ThreadList[index] = GM_INVALID_THREAD;
	if(index == m_NumThreads-1)
	{
		do {
			m_NumThreads--;
		} while(m_NumThreads > 0 && m_ThreadList[m_NumThreads-1] == GM_INVALID_THREAD);
	}
}

bool State::DeleteForkThread(int _threadId)
{
	gmMachine * pM = ScriptManager::GetInstance()->GetMachine();
	for(int i = 0; i < m_NumThreads; ++i)
	{
		if(m_ThreadList[i] == _threadId)
		{
			pM->KillThread(_threadId);
			ClearThreadReference(i);
			return true;
		}
	}
	return false;
}

bool State::RemoveThreadReference(const int * _threadId, int _numThreadIds)
{
	bool b = false;
	for(int t = 0; t < _numThreadIds; ++t)
	{
		int id = _threadId[t];
		for(int i = 0; i < m_NumThreads; ++i)
		{
			if(m_ThreadList[i] == id)
			{
				ClearThreadReference(i);
				b = true;
				break;
			}
		}
	}
	return b;
}

void State::PropogateDeletedThreads(const int *_threadIds, int _numThreads)
{
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		pState->PropogateDeletedThreads(_threadIds, _numThreads);

	RemoveThreadReference(_threadIds, _numThreads);
}

void State::DeleteGoalScripts()
{
	State *pNext, *pLastState = NULL;

	for(State *pState = m_FirstChild; pState; pState = pNext)
	{
		pState->DeleteGoalScripts();
		pNext = pState->m_Sibling;

		if(pState->m_ScriptObject)
		{
			InternalParentExit();
			pState->InternalExit();

			if(pLastState) pLastState->m_Sibling = pNext;
			else m_FirstChild = pNext;

			delete pState;
		}
		else
			pLastState = pState;
	}
}

bool State::StateCommand(const StringVector &_args)
{
	bool handled = false;
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		handled |= pState->StateCommand(_args);

	if(m_CommandTable)
	{
		gmMachine * pMachine = ScriptManager::GetInstance()->GetMachine();
		gmVariable varThis = gmVariable::s_null;
		gmUserObject * pScriptObject = GetScriptObject(pMachine);
		if(pScriptObject)
			varThis.SetUser(pScriptObject);

		ScriptCommandExecutor cmdExec(pMachine,m_CommandTable);
		if(cmdExec.Exec(_args,varThis))
			handled |= true;
	}
	return handled;
}

void State::OnSpawn()
{
	SetLastPriority( 0.0f );

	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		if(!pState->IsUserDisabled())
			pState->OnSpawn();
}

void State::SetSelectable(bool _selectable) 
{
	m_StateFlags.SetFlag(State_UnSelectable, !_selectable); 
}

void State::SetEnable(bool _enable, const char *_error) 
{
	if(_error)
	{
		OBASSERT(0, _error);
		LOGERR(_error);
	}
	m_StateFlags.SetFlag(State_UserDisabled, !_enable); 
}

State::noSelectReason_t State::CanBeSelected()
{
	if(m_OnlyClass.AnyFlagSet() && !m_OnlyClass.CheckFlag(GetClient()->GetClass()))
		return NoSelectReason_OnlyClass;
	if(m_OnlyTeam.AnyFlagSet() && !m_OnlyTeam.CheckFlag(GetClient()->GetTeam()))
		return NoSelectReason_OnlyTeam;
	if(m_OnlyPowerUp.AnyFlagSet() && !(m_OnlyPowerUp & GetClient()->GetPowerUpFlags()).AnyFlagSet())
		return NoSelectReason_OnlyPowerup;
	if(m_OnlyNoPowerUp.AnyFlagSet() && (m_OnlyNoPowerUp & GetClient()->GetEntityFlags()).AnyFlagSet())
		return NoSelectReason_OnlyNoPowerup;
	if(m_OnlyEntFlag.AnyFlagSet() && !(m_OnlyEntFlag & GetClient()->GetEntityFlags()).AnyFlagSet())
		return NoSelectReason_OnlyEntFlag;
	if(m_OnlyNoEntFlag.AnyFlagSet() && (m_OnlyNoEntFlag & GetClient()->GetEntityFlags()).AnyFlagSet())
		return NoSelectReason_OnlyNoEntFlag;
	if(m_OnlyRole.AnyFlagSet() && !(m_OnlyRole & GetClient()->GetRoleMask()).AnyFlagSet())
		return NoSelectReason_OnlyRole;
	
	if(m_OnlyWeapon.AnyFlagSet())
	{
		AiState::WeaponSystem *ws = GetClient()->GetWeaponSystem();

		BitFlag128 hasWeapons = (m_OnlyWeapon & ws->GetWeaponMask());
		if(!hasWeapons.AnyFlagSet())
			return NoSelectReason_OnlyWeapon;

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
			return NoSelectReason_OnlyWeaponNoAmmo;
	}
	
	if(m_OnlyTargetClass.AnyFlagSet() || 
		m_OnlyTargetTeam.AnyFlagSet() || 
		m_OnlyTargetPowerUp.AnyFlagSet() || 
		m_OnlyTargetEntFlag.AnyFlagSet() ||
		m_OnlyTargetNoPowerUp.AnyFlagSet() || 
		m_OnlyTargetNoEntFlag.AnyFlagSet() || 
		m_OnlyTargetWeapon.AnyFlagSet())
	{
		AiState::TargetingSystem *ts = GetClient()->GetTargetingSystem();
		if(ts)
		{
			const MemoryRecord *target = ts->GetCurrentTargetRecord();
			const bool NoTargetAllowed = m_OnlyTargetClass.CheckFlag(0);

			if(!target && NoTargetAllowed)
			{
				// noop
			}
			else if(!target)
				return NoSelectReason_OnlyTarget;
			else
			{
				if(m_OnlyTargetClass.AnyFlagSet())
				{
					if(m_OnlyTargetClass.CheckFlag(FilterSensory::ANYPLAYERCLASS) &&
						target->m_TargetInfo.m_EntityClass < FilterSensory::ANYPLAYERCLASS)
					{
						// success
					}
					else if(!m_OnlyTargetClass.CheckFlag(target->m_TargetInfo.m_EntityClass))
						return NoSelectReason_OnlyTargetClass;
				}

				if(m_OnlyTargetTeam.AnyFlagSet())
				{
					if(!m_OnlyTargetTeam.CheckFlag(InterfaceFuncs::GetEntityTeam(target->GetEntity())))
						return NoSelectReason_OnlyTargetTeam;
				}

				// only if target has powerup
				if(m_OnlyTargetPowerUp.AnyFlagSet())
				{
					if((target->m_TargetInfo.m_EntityPowerups & m_OnlyTargetPowerUp) != m_OnlyTargetPowerUp)
					{
						return NoSelectReason_OnlyTargetPowerup;
					}
				}

				// only if target doesn't have powerup
				if(m_OnlyTargetNoPowerUp.AnyFlagSet())
				{
					if((target->m_TargetInfo.m_EntityPowerups & m_OnlyTargetNoPowerUp).AnyFlagSet())
					{
						return NoSelectReason_OnlyTargetNoPowerup;
					}
				}

				// only if target has ent flag
				if(m_OnlyTargetEntFlag.AnyFlagSet())
				{
					if((target->m_TargetInfo.m_EntityFlags & m_OnlyTargetEntFlag) != m_OnlyTargetEntFlag)
					{
						return NoSelectReason_OnlyTargetEntFlag;
					}
				}

				// only if target doesn't have ent flag
				if(m_OnlyTargetNoEntFlag.AnyFlagSet())
				{
					if((target->m_TargetInfo.m_EntityFlags & m_OnlyTargetNoEntFlag).AnyFlagSet())
					{
						return NoSelectReason_OnlyTargetNoEntFlag;
					}
				}

				// only if target has one of these weapons
				if(m_OnlyTargetWeapon.AnyFlagSet())
				{
					if(!m_OnlyTargetWeapon.CheckFlag(target->m_TargetInfo.m_CurrentWeapon))
						return NoSelectReason_OnlyTargetWeapon;
				}
			}
		}
	}
	if(m_LimitCallback.m_LimitTo)
	{
		if(!m_LimitCallback.m_OnlyWhenActive || IsActive())
		{
			if(m_LimitCallback.m_NextCallback <= IGame::GetTime())
			{
				m_LimitCallback.m_NextCallback = IGame::GetTime() + m_LimitCallback.m_Delay;

				gmMachine *pM = ScriptManager::GetInstance()->GetMachine();

				gmCall call;
				if(call.BeginFunction(pM, m_LimitCallback.m_LimitTo, m_LimitCallback.m_This))
				{
					call.End();

					int iCancel = 0;
					if(call.GetReturnedInt(iCancel))
						m_LimitCallback.m_Result = iCancel!=0;
				}
			}

			if(!m_LimitCallback.m_Result)
				return NoSelectReason_LimitCallback;
		}
	}

	return NoSelectReasonNone;
}

void State::LimitTo(const gmVariable &varThis, gmGCRoot<gmFunctionObject> &_fn, int _delay, bool _onlywhenactive)
{
	m_LimitCallback.m_This = varThis;
	m_LimitCallback.m_LimitTo = _fn;
	m_LimitCallback.m_Delay = _delay;
	m_LimitCallback.m_OnlyWhenActive = _onlywhenactive;
	m_LimitCallback.m_NextCallback = 0;
}

void State::ClearLimitTo()
{
	m_LimitCallback = LimitToCallback();
}

void State::BlackboardDelay(float _delayseconds, int _targetId)
{
	enum { NumDelays = 4 };
	BBRecordPtr delays[NumDelays];
	int n = GetClient()->GetBB().GetBBRecords(bbk_DelayGoal,delays,NumDelays);
	for(int i = 0; i < n; ++i)
	{
		if(delays[i]->m_Target == _targetId && 
			delays[i]->m_Owner == GetClient()->GetGameID())
		{
			delays[i]->m_ExpireTime = IGame::GetTime() + Utils::SecondsToMilliseconds(_delayseconds);
			return;
		}
	}

	BBRecordPtr bbp(new bbDelayGoal);
	bbp->m_Owner = GetClient()->GetGameID();
	bbp->m_Target = _targetId;
	bbp->m_ExpireTime = IGame::GetTime() + Utils::SecondsToMilliseconds(_delayseconds);
	bbp->m_DeleteOnExpire = true;
	GetClient()->GetBB().PostBBRecord(bbp);
}

bool State::BlackboardIsDelayed(int _targetId)
{
	return GetClient()->GetBB().GetNumBBRecords(bbk_DelayGoal, _targetId) > 0;
}

//////////////////////////////////////////////////////////////////////////
// Debug

#ifdef ENABLE_DEBUG_WINDOW
void State::RenderDebugWindow(gcn::DrawInfo drawinfo)
{
	if(DontDrawDebugWindow())
		return;

	// Draw the title.
	if(GetParent())
	{
		const char *prefix = "  ";	
		if(m_FirstChild)
			prefix = IsDebugExpanded() ? "- ": "+ ";

		gcn::Color renderColor = IsActive()||m_LastUpdateTime==IGame::GetTime() ? gcn::Color(0,160,0) : gcn::Color(0,0,0);
		if(m_StateFlags.CheckFlag(State_UnSelectable))
			renderColor = gcn::Color(255,255,255);
		if(IsDisabled())
			renderColor = gcn::Color(255,0,0);

		const int fontHeight = drawinfo.m_Widget->getFont()->getHeight();
		int iX = drawinfo.m_Indent * 12;
		int iY = drawinfo.m_Line * fontHeight;

		gcn::Rectangle l(0, iY, drawinfo.m_Widget->getWidth(), fontHeight);

		if(l.isPointInRect(drawinfo.Mouse.X, drawinfo.Mouse.Y))
		{
			renderColor.a = 128;

			if(drawinfo.Mouse.MouseClicked[gcn::MouseCache::Right])
				DebugExpand(!IsDebugExpanded());
			if(drawinfo.Mouse.MouseClicked[gcn::MouseCache::Left])
				ShowStateWindow(GetNameHash());
			if(drawinfo.Mouse.MouseClicked[gcn::MouseCache::Mid])
				DebugDraw(!DebugDrawingEnabled());
		}

		drawinfo.m_Graphics->setColor(renderColor);
		drawinfo.m_Graphics->fillRectangle(l);
		drawinfo.m_Graphics->setColor(drawinfo.m_Widget->getForegroundColor());

		StringStr s;
		GetDebugString(s);

		drawinfo.m_Graphics->drawText((String)va("%s %s (%.2f) - %s:",
			prefix,GetName().c_str(),
			GetLastPriority(),
			s.str().c_str()),iX,iY);

		++drawinfo.m_Line;

		if(IsDebugExpanded())
		{
			for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
				pState->RenderDebugWindow(drawinfo.indent());
		}
		return;
	}	

	if(IsDebugExpanded() || !GetParent())
	{
		for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
			pState->RenderDebugWindow(drawinfo.indent());
	}
}
#endif

#ifdef ENABLE_REMOTE_DEBUGGING
void State::Sync( RemoteLib::DataBuffer & db, bool fullSync, const char * statePath ) {
	String pth = va( "%s/%s", statePath, GetName().c_str() );

	StringStr s;
	GetDebugString(s);
	obColor renderColor = IsActive()||m_LastUpdateTime==IGame::GetTime() ? obColor( 0, 160, 0) : obColor(0,0,0);
	if(m_StateFlags.CheckFlag(State_UnSelectable))
		renderColor = obColor(255,255,255);
	if(IsDisabled())
		renderColor = obColor(255,0,0);

	enum { LocalBufferSize = 256 };
	char localBuffer[ LocalBufferSize ] = {};
	RemoteLib::DataBuffer localDb( localBuffer, LocalBufferSize );
	localDb.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
	localDb.startSizeHeader();
	localDb.writeInt32( RemoteLib::ID_treeNode );
	localDb.writeString( pth.c_str() );
	localDb.writeString( va( "%s (%.2f)", GetName().c_str(), GetLastPriority() ) );
	localDb.writeString( s.str().c_str() );
	localDb.writeInt8( renderColor.r() );
	localDb.writeInt8( renderColor.g() );
	localDb.writeInt8( renderColor.b() );
	localDb.writeInt8( renderColor.a() );
	localDb.endSizeHeader();
	localDb.endWrite();

	const obuint32 crc = FileSystem::CalculateCrc( localBuffer, localDb.getBytesWritten() );
	if ( fullSync || crc != m_SyncCrc ) {
		if ( db.append( localDb ) ) {
			m_SyncCrc = crc;
		} else {
			m_SyncCrc = 0;
		}
	}

	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
		pState->Sync( db, fullSync, pth.c_str() );

}
#endif

//////////////////////////////////////////////////////////////////////////

StateSimultaneous::StateSimultaneous(const char * _name, const UpdateDelay &_ur) 
	: State(_name, _ur)
{
}

obReal StateSimultaneous::GetPriority()
{
	//State *pBestState = NULL;
	float fBestPriority = 0.f;
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
	{
		if(pState->IsUserDisabled())
			continue;

		float fPriority = pState->InternalGetPriority();
		if(fPriority > fBestPriority)
		{
			fBestPriority = fPriority;
			//pBestState = pState;
		}
	}
	return fBestPriority;
}

State::StateStatus StateSimultaneous::UpdateState(float fDt)
{
	OBASSERT(!IsDisabled(), "State Disabled, UpdateState called!");

	State *pLastState = NULL;
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
	{
		bool bWantsActive = !pState->IsDisabled() ? pState->InternalGetPriority() > 0.0 : false;

		// Handle exits
		if(pState->IsActive() && (!bWantsActive || pState->IsDisabled()))
		{
			pState->InternalExit(); // call internal version
			if(!bWantsActive && pState->CheckFlag(State_DeleteOnFinished))
			{
				if(pLastState)
					pLastState = pState->m_Sibling;
				delete pState;
			}
			continue;
		}

		if(!pState->IsActive() && bWantsActive)
			pState->InternalEnter(); // call internal version

		if(pState->IsActive())
		{
			if(pState->InternalUpdateState() == State_Finished)
				pState->InternalExit();
		}
		pLastState = pState;
	}
	Update(fDt);
	return State_Busy;
}

//////////////////////////////////////////////////////////////////////////

StateFirstAvailable::StateFirstAvailable(const char * _name, const UpdateDelay &_ur) 
	: State(_name, _ur)
	, m_CurrentState(0)
{
}

void StateFirstAvailable::GetDebugString(StringStr &out)
{
	if(m_CurrentState)
		m_CurrentState->GetDebugString(out);
}

obReal StateFirstAvailable::GetPriority()
{
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
	{
		if(pState->IsUserDisabled())
			continue;

		float fPriority = pState->InternalGetPriority();
		if(fPriority > 0.f)
			return fPriority;
	}
	return 0.f;
}

void StateFirstAvailable::InternalParentExit()
{
	if(m_CurrentState && m_CurrentState->IsActive())
		m_CurrentState->InternalExit();
	m_CurrentState = 0;
}

State::StateStatus StateFirstAvailable::UpdateState(float fDt)
{
	State *pBestState = NULL;
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
	{
		if(pState->IsUserDisabled())
			continue;

		float fPriority = pState->InternalGetPriority();
		if(fPriority > 0.f)
		{
			pBestState = pState;
			break;
		}
	}

	// Exit active states that are not our best
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
	{
		if(pBestState != pState && pState->IsActive())
		{
			pState->InternalExit();

			if(m_CurrentState == pState)
				m_CurrentState = 0;
		}
	}

	if(pBestState && m_CurrentState != pBestState)
	{
		OBASSERT(!pBestState->IsActive(), "State not active!");
		m_CurrentState = pBestState;
		m_CurrentState->InternalEnter();
	}

	if(m_CurrentState)
	{
		OBASSERT(m_CurrentState->IsActive() || m_CurrentState->IsRoot(), "State not active!");
		if(m_CurrentState->InternalUpdateState() == State_Finished)
		{
			m_CurrentState->InternalExit();
			m_CurrentState = 0;
		}
	}

	Update(fDt);

	return m_CurrentState || InternalGetPriority()>0.f ? State_Busy : State_Finished;
}

//////////////////////////////////////////////////////////////////////////

StatePrioritized::StatePrioritized(const char * _name, const UpdateDelay &_ur) 
	: State(_name, _ur)
	, m_CurrentState(0)
{
}

void StatePrioritized::GetDebugString(StringStr &out)
{
	if(m_CurrentState)
		m_CurrentState->GetDebugString(out);
}

obReal StatePrioritized::GetPriority()
{
	//State *pBestState = NULL;
	float fBestPriority = 0.f;
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
	{
		if(pState->IsUserDisabled())
			continue;

		float fPriority = pState->InternalGetPriority();
		if(fPriority > fBestPriority)
		{
			fBestPriority = fPriority;
			//pBestState = pState;
		}
	}
	return fBestPriority;
}

void StatePrioritized::InternalParentExit()
{
	if(m_CurrentState && m_CurrentState->IsActive())
		m_CurrentState->InternalExit();
	m_CurrentState = 0;
}

State::StateStatus StatePrioritized::UpdateState(float fDt)
{
	State *pBestState = NULL;
	float fBestPriority = 0.f;
	int iBestRand = 0;

#ifdef _DEBUG
	//const int N = 64; // cs: was 32
	//float STATES_PRIO[N] = {};
	//State *STATES_P[N] = {}; int NumPriorities = 0;
	//State *STATES_X[N] = {}; int NumExited = 0;
	//State *OLD_BEST = m_CurrentState; OLD_BEST;
	//const char *BOT_NAME = GetClient() ? GetClient()->GetName() : 0; BOT_NAME;
#endif

	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
	{
		if(pState->IsUserDisabled())
			continue;

		float fPriority = pState->InternalGetPriority();

#ifdef _DEBUG
		//STATES_PRIO[NumPriorities] = fPriority;
		//STATES_P[NumPriorities] = pState;
		//NumPriorities++;
#endif

		if(fPriority >= fBestPriority)
		{
			if(fPriority > fBestPriority)
			{
				fBestPriority = fPriority;
				pBestState = pState;
				iBestRand = 0;
			}
			else if(fPriority > 0)
			{
				int iRand = rand();
				if (iBestRand == 0) iBestRand = rand();
				if (iRand > iBestRand)
				{
					iBestRand = iRand;
					pBestState = pState;
				}
			}
		}
	}

	// if the current state has an equal priority to the 'best', the current
	// state has the edge, to prevent order dependency causing goals to override
	// on equal priorities
	if ( m_CurrentState ) {
		if ( m_CurrentState->GetLastPriority() >= fBestPriority ) {
			pBestState = m_CurrentState;
		}
	}	

	// Exit active states that are not our best
	for(State *pState = m_FirstChild; pState; pState = pState->m_Sibling)
	{
		if(pBestState != pState && pState->IsActive())
		{
#ifdef _DEBUG
			//STATES_X[NumExited++] = pState;
#endif
			pState->InternalExit();
		}
	}

	if(pBestState && m_CurrentState != pBestState)
	{
		OBASSERT(!pBestState->IsActive(), "State not active!");
		m_CurrentState = pBestState;
		m_CurrentState->InternalEnter();
	}

	if(m_CurrentState)
	{
		OBASSERT(m_CurrentState->IsActive() || m_CurrentState->IsRoot(), "State not active!");
		if(m_CurrentState->InternalUpdateState() == State_Finished)
		{
			m_CurrentState->InternalExit();
			m_CurrentState = 0;
		}
	}

	Update(fDt);

	return m_CurrentState || InternalGetPriority()>0.f ? State_Busy : State_Finished;
}

//////////////////////////////////////////////////////////////////////////

StateSequential::StateSequential(const char * _name, const UpdateDelay &_ur) 
	: State(_name, _ur)
	, m_CurrentState(0)
{
}

void StateSequential::GetDebugString(StringStr &out)
{
	if(m_CurrentState)
		m_CurrentState->GetDebugString(out);
}

void StateSequential::Exit()
{
	if(m_CurrentState && m_CurrentState->IsActive())
		m_CurrentState->InternalExit();
	m_CurrentState = 0;
}

State::StateStatus StateSequential::UpdateState(float fDt)
{
	/*if(m_CurrentState)
	{
		if(m_StateList.front()->IsActive())
			return m_StateList.front()->InternalUpdateState();
	}*/
	return State_Busy;
}

//////////////////////////////////////////////////////////////////////////

StateSequentialLooping::StateSequentialLooping(const char * _name, const UpdateDelay &_ur) 
	: State(_name, _ur)
{
}

State::StateStatus StateSequentialLooping::UpdateState(float fDt)
{
	return State_Busy;
}

//////////////////////////////////////////////////////////////////////////

StateProbabilistic::StateProbabilistic(const char * _name, const UpdateDelay &_ur) 
	: State(_name, _ur)
{
}

State::StateStatus StateProbabilistic::UpdateState(float fDt)
{
	return State_Busy;
}

//////////////////////////////////////////////////////////////////////////

StateOneOff::StateOneOff(const char * _name, const UpdateDelay &_ur) 
	: State(_name, _ur)
{
}

State::StateStatus StateOneOff::UpdateState(float fDt)
{
	return State_Busy;
}

//////////////////////////////////////////////////////////////////////////

StateChild::StateChild(const char * _name, const UpdateDelay &_ur) 
	: State(_name, _ur)
{
}

State::StateStatus StateChild::UpdateState(float fDt)
{
	return Update(fDt);
}

//////////////////////////////////////////////////////////////////////////
