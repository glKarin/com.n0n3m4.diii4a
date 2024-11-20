#include "PrecompCommon.h"
#include "BotBaseStates.h"
#include "ScriptManager.h"
#include "gmScriptGoal.h"

//////////////////////////////////////////////////////////////////////////

Prof_Define(ScriptGoal);

namespace AiState
{
	MoveOptions::MoveOptions() 
		: Radius(32.f)
		, ThreadId(GM_INVALID_THREAD)
		, Mode(Run) 
		, NumAvoid(0)
	{
	}

	void MoveOptions::FromTable(gmMachine *a_machine, gmTableObject *a_table)
	{
		Mode = a_table->Get(a_machine,"MoveMode").GetIntSafe(Run)==Walk?Walk:Run;
		a_table->Get(a_machine,"Radius").GetFloat(Radius,Radius);

		gmTableObject *avoidTable = a_table->Get(a_machine,"Avoid").GetTableObjectSafe();
		if(avoidTable)
		{
			gmTableIterator tIt;
			gmTableNode *pNode = avoidTable->GetFirst(tIt);
			while(pNode)
			{
				/*const char *valueString = pNode->m_value.GetCStringSafe();
				valueString;
				if(!Q_stricmp(valueString,"enemies"))
				{
				}
				if(!Q_stricmp(valueString,"allies"))
				{
				}*/

				pNode = avoidTable->GetNext(tIt);
			}

		}
	}

	ScriptGoal::ScriptGoal(const char *_name) 
		: StateChild(_name)
		, FollowPathUser(_name)
		, m_AimVector(Vector3f::ZERO)
		, m_AimWeaponId(0)
		, m_ScriptAimType(Aimer::MoveDirection)
		, m_ScriptPriority(0.0f)
		, m_MinRadius(0.f)
		, m_ParentNameHash(0)
		, m_InsertBeforeHash(0)
		, m_InsertAfterHash(0)
		, m_NextGetPriorityUpdate(0)
		, m_NextGetPriorityDelay(0)
		, m_Finished(false)
		, m_AimSignalled(false)
		, m_AutoReleaseAim(true)
		, m_AutoReleaseWpn(true)
		, m_AutoReleaseTracker(true)
		, m_AutoFinishOnUnavailable(true)
		, m_AutoFinishOnNoProgressSlots(true)
		, m_AutoFinishOnNoUseSlots(true)
		, m_SkipGetPriorityWhenActive(false)
		, m_SkipLastWp(false)
#ifdef Prof_ENABLED
		, m_ProfZone(0)
#endif
	{
		SetScriptGoal(true);
	}

	void ScriptGoal::SetProfilerZone(const String &_name)
	{
#ifdef Prof_ENABLED
		m_ProfZone = gDynamicZones.FindZone(_name.c_str());
		OBASSERT(m_ProfZone,"No Profiler Zone Available!");
#endif
	}

	ScriptGoal::~ScriptGoal()
	{
		if(m_ScriptObject)
		{
			gmScriptGoal::NullifyObject(m_ScriptObject);
			m_ScriptObject = 0; //NULL;
		}
	}

	void ScriptGoal::GetDebugString(StringStr &out)
	{
		if(m_DebugString)
			out << m_DebugString->GetString();
		else if(m_MapGoal)
			out << m_MapGoal->GetName();
	}

	void ScriptGoal::RenderDebug()
	{
		/*if(IsActive())
		{
		Utils::DrawLine(GetClient()->GetEyePosition(),m_MapGoal->GetPosition(),COLOR::CYAN,5.f);
		}*/
	}

	ScriptGoal *ScriptGoal::Clone()
	{
		ScriptGoal *pNewGoal = new ScriptGoal(GetName().c_str());
		*pNewGoal = *this;
		pNewGoal->m_ScriptObject = 0; //NULL; // dont want to copy this

		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
		gmTableObject *pTheirTable = gmScriptGoal::GetUserTable(GetScriptObject(pMachine));
		gmScriptGoal::gmBindUserObject *pObj = 
			gmScriptGoal::GetUserBoundObject(pMachine, pNewGoal->GetScriptObject(pMachine));
		pObj->m_table = pTheirTable->Duplicate(pMachine);

		return pNewGoal;
	}

	void ScriptGoal::SetParentName(const char *_str)
	{
		m_ParentNameHash = Utils::MakeHash32(_str);
	}

	String ScriptGoal::GetParentName() const
	{
		return Utils::HashToString(GetParentNameHash());
	}

	void ScriptGoal::SetInsertBeforeName(const char *_str)
	{
		m_InsertBeforeHash = Utils::MakeHash32(_str);
	}

	String ScriptGoal::GetInsertBeforeName() const
	{
		return Utils::HashToString(GetInsertBeforeHash());
	}

	void ScriptGoal::SetInsertAfterName(const char *_str)
	{
		m_InsertAfterHash = Utils::MakeHash32(_str);
	}

	String ScriptGoal::GetInsertAfterName() const
	{
		return Utils::HashToString(GetInsertAfterHash());
	}

	gmUserObject *ScriptGoal::GetScriptObject(gmMachine *_machine)
	{
		DisableGCInScope gcEn(_machine);

		if(!m_EventTable)
			m_EventTable.Set(_machine->AllocTableObject(), _machine);
		if(!m_CommandTable)
			m_CommandTable.Set(_machine->AllocTableObject(), _machine);
		if(!m_ScriptObject)
			m_ScriptObject.Set(gmScriptGoal::WrapObject(_machine, this), _machine);
		return m_ScriptObject;
	}

	void ScriptGoal::InternalExit() 
	{
		State::InternalExit();
		// always kill goal threads on an exit, we may not actually be active
		// such as if we are running event threads we need to clean up
		KillAllGoalThreads();
	}

	void ScriptGoal::InternalSignal(const gmVariable &_signal)
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
		for(int i = 0; i < NUM_CALLBACKS; ++i)
		{
			if(m_ActiveThread[i])
			{
				pMachine->Signal(_signal, m_ActiveThread[i], GM_INVALID_THREAD);
			}
		}
		for(int i = 0; i < m_NumThreads; ++i)
		{
			if(m_ThreadList[i] != GM_INVALID_THREAD)
			{
				pMachine->Signal(_signal, m_ThreadList[i], GM_INVALID_THREAD);
			}
		}
	}

	void ScriptGoal::KillAllGoalThreads()
	{
		for(int i = 0; i < NUM_CALLBACKS; ++i)
			m_ActiveThread[i].Kill();

		gmMachine * pM = ScriptManager::GetInstance()->GetMachine();
		for(int i = 0; i < m_NumThreads; ++i)
		{
			if(m_ThreadList[i] != GM_INVALID_THREAD)
			{
				pM->KillThread(m_ThreadList[i]);
				m_ThreadList[i] = GM_INVALID_THREAD;
			}
		}
		m_NumThreads = 0;
	}

	void ScriptGoal::RunCallback(FunctionCallback callback, bool whenNotActive)
	{
		if(m_Callbacks[callback])
		{
			if(!whenNotActive || !m_ActiveThread[callback].IsActive())
			{
				gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
				gmCall call;
				if(call.BeginFunction(pMachine, m_Callbacks[callback], gmVariable(GetScriptObject(pMachine))))
				{
					call.End();
					
					if(whenNotActive)
						m_ActiveThread[callback] = call.DidReturnVariable() ? GM_INVALID_THREAD : call.GetThreadId();
				}
			}
		}
	}

	bool ScriptGoal::Goto(const Vector3f &_pos, const MoveOptions &options)
	{
		m_SkipLastWp = false;
		m_MinRadius = options.Radius;

		SetSourceThread(options.ThreadId);
		FINDSTATE(fp, FollowPath, GetRootState());
		if(fp)
			return fp->Goto(this, _pos, options.Radius, options.Mode);
		return false;
	}

	bool ScriptGoal::Goto(const Vector3List &_vectors, const MoveOptions &options)
	{
		m_SkipLastWp = false;
		m_MinRadius = options.Radius;

		SetSourceThread(options.ThreadId);
		FINDSTATE(fp, FollowPath, GetRootState());
		return fp && fp->Goto(this, _vectors, options.Radius, options.Mode);
	}

	bool ScriptGoal::GotoRandom(const MoveOptions &options)
	{
		m_SkipLastWp = false;
		m_MinRadius = options.Radius;

		SetSourceThread(options.ThreadId);
		FINDSTATE(fp, FollowPath, GetRootState());
		if(fp)
		{
			PathPlannerBase *pPathPlanner = IGameManager::GetInstance()->GetNavSystem();
			Vector3f vDestination = pPathPlanner->GetRandomDestination(GetClient(),GetClient()->GetPosition(),GetClient()->GetTeamFlag());
			return fp->Goto(this, vDestination, options.Radius, options.Mode);
		}
		return false;
	}

	void ScriptGoal::Stop()
	{
		FINDSTATEIF(FollowPath, GetRootState(), Stop());
	}

	bool ScriptGoal::RouteTo(MapGoalPtr mg, const MoveOptions &options)
	{
		if(mg)
		{
			m_MapGoalRoute = mg;
			m_MinRadius = options.Radius;

			SetSourceThread(options.ThreadId);
			FINDSTATE(fp, FollowPath, GetRootState());
			if(fp)
				return fp->Goto(this, options.Mode, m_SkipLastWp);
		}
		return false;
	}

	// FollowPathUser functions.
	bool ScriptGoal::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		_skiplastpt = m_SkipLastWp;
		if(m_MapGoalRoute && m_MapGoalRoute->RouteTo(GetClient(), _desination, m_MinRadius))
		{
			_final = false;
		}
		else 
		{
			_final = true;
		}
		return true;
	}

	void ScriptGoal::OnPathSucceeded()
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
		if(GetSourceThread() == GM_INVALID_THREAD)
		{
			if(m_ActiveThread[ON_UPDATE].IsActive())
				pMachine->Signal(gmVariable(PATH_SUCCESS), m_ActiveThread[ON_UPDATE].ThreadId(), GM_INVALID_THREAD);
			// TODO: send it to forked threads also?
		}
		else
		{
			pMachine->Signal(gmVariable(PATH_SUCCESS), GetSourceThread(), GM_INVALID_THREAD);
		}
	}

	void ScriptGoal::OnPathFailed(FollowPathUser::FailType _how)
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
		if(GetSourceThread() == GM_INVALID_THREAD)
		{
			if(m_ActiveThread[ON_UPDATE].IsActive())
				pMachine->Signal(gmVariable(PATH_FAILED), m_ActiveThread[ON_UPDATE].ThreadId(), GM_INVALID_THREAD);
			// TODO: send it to forked threads also?
		}
		else
		{
			pMachine->Signal(gmVariable(PATH_FAILED), GetSourceThread(), GM_INVALID_THREAD);
		}
	}

	// AimerUser functions
	bool ScriptGoal::GetAimPosition(Vector3f &_aimpos)
	{
		if(m_AimWeaponId)
		{
			const MemoryRecord *pRecord = GetClient()->GetTargetingSystem()->GetCurrentTargetRecord();
			WeaponPtr w = GetClient()->GetWeaponSystem()->GetWeapon(m_AimWeaponId);
			if(pRecord != NULL && w)
			{
				_aimpos = w->GetAimPoint(Primary,pRecord->GetEntity(), pRecord->m_TargetInfo);
			}
			else
				return false;
		}

		_aimpos = m_AimVector;
		switch(m_ScriptAimType)
		{
		case Aimer::WorldFacing:
			_aimpos = GetClient()->GetEyePosition() + m_AimVector * 512.f;
			break;
		case Aimer::WorldPosition:
		case Aimer::MoveDirection:
		case Aimer::UserCallback:
		default:
			break;
		}
		return true;
	}

	void ScriptGoal::OnTarget()
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
		if(!m_AimSignalled && m_ActiveThread[ON_UPDATE].IsActive())
		{
			pMachine->Signal(gmVariable(AIM_SUCCESS), m_ActiveThread[ON_UPDATE].ThreadId(), GM_INVALID_THREAD);
			Signal(gmVariable(AIM_SUCCESS));
			m_AimSignalled = true;
		}
	}

	bool ScriptGoal::OnInit(gmMachine *_machine)
	{
		Prof_Scope(ScriptGoal);

#ifdef Prof_ENABLED
		Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif

		{
			Prof(OnInit);

			if(m_Callbacks[ScriptGoal::ON_INIT])
			{
				gmVariable varThis = gmVariable(GetScriptObject(_machine));

				gmCall call;
				if(call.BeginFunction(_machine, m_Callbacks[ScriptGoal::ON_INIT], varThis))
				{
					call.End();

					int iReturnVal = 0;
					if(call.GetReturnedInt(iReturnVal) && iReturnVal == 0)
						return false;
				}
			}
			return true;
		}
	}

	void ScriptGoal::OnSpawn()
	{
		Prof_Scope(ScriptGoal);

#ifdef Prof_ENABLED
		Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif

		{
			Prof(OnSpawn);

			m_NextGetPriorityUpdate = 0;
			SetScriptPriority( 0.0f );
			SetLastPriority( 0.0f );

			KillAllGoalThreads();

			if(m_Callbacks[ON_SPAWN])
			{
				m_ActiveThread[ON_SPAWN].Kill();

				// don't call it unless we can
				if(CanBeSelected() == NoSelectReasonNone)
					RunCallback(ON_SPAWN, true);
			}
		}
	}

	void ScriptGoal::OnException()
	{
		MapGoal* mg = GetMapGoal().get();
		if(mg)
		{
			//disable current MapGoal temporarily
			BlackboardDelay(Mathf::IntervalRandom(20,50), mg->GetSerialNum());
		}
		else
		{
			//disable this ScriptGoal until map restart
			SetEnable(false);
		}
		SetFinished();
	}

	void ScriptGoal::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		Signal(gmVariable(_message.GetMessageId()));
	}

	void ScriptGoal::Signal(const gmVariable &_var)
	{
		InternalSignal(_var);
	}

	bool ScriptGoal::OnPathThrough(const String &_s)
	{
		Prof_Scope(ScriptGoal);

#ifdef Prof_ENABLED
		Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif

		{
			Prof(OnPathThrough);

			if(m_Callbacks[ON_PATH_THROUGH])
			{
				gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
				gmCall call;
				if(call.BeginFunction(pMachine, m_Callbacks[ON_PATH_THROUGH], gmVariable(GetScriptObject(pMachine))))
				{
					call.AddParamString(_s.c_str());
					call.End();

					int iRetVal = 0;
					if(call.DidReturnVariable() && call.GetReturnedInt(iRetVal) && iRetVal)
					{
						SetScriptPriority(1.f);
						SetLastPriority(1.f);
						return true;
					}						
				}
			}
			return false;
		}
	}

	void ScriptGoal::EndPathThrough() 
	{
		SetScriptPriority(0.f);
		SetLastPriority(0.f);
	}

	obReal ScriptGoal::GetPriority()
	{
		Prof_Scope(ScriptGoal);

#ifdef Prof_ENABLED
		Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif

		{
			Prof(GetPriority);

			if(m_SkipGetPriorityWhenActive && IsActive())
			{
				// don't call getpriority while active
			}
			else
			{
				if(m_NextGetPriorityUpdate <= IGame::GetTime())
				{
					DelayGetPriority(m_NextGetPriorityDelay);

					RunCallback(ON_GETPRIORITY, true);
				}
			}
			UpdateEntityInRadius();
		}
		return m_ScriptPriority;
	}

	void ScriptGoal::Enter()
	{
		Prof_Scope(ScriptGoal);

#ifdef Prof_ENABLED
		Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif

		{
			Prof(Enter);

			m_ActiveThread[ON_GETPRIORITY].Kill();

			m_Finished = false;

			RunCallback(ON_ENTER);
		}
	}

	void ScriptGoal::Exit()
	{
		Prof_Scope(ScriptGoal);

#ifdef Prof_ENABLED
		Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif

		{
			Prof(Exit);

			SetScriptPriority( 0.0f );

			RunCallback(ON_EXIT);

			//////////////////////////////////////////////////////////////////////////
			// Automatically release requests on exit.
			if(AutoReleaseAim())
			{
				using namespace AiState;
				FINDSTATE(aim, Aimer, GetClient()->GetStateRoot());
				if(aim)
					aim->ReleaseAimRequest(GetNameHash());
			}
			if(AutoReleaseWeapon())
			{
				using namespace AiState;
				FINDSTATE(weapsys, WeaponSystem, GetClient()->GetStateRoot());
				if(weapsys)
					weapsys->ReleaseWeaponRequest(GetNameHash());
			}
			if(AutoReleaseTracker())
			{
				m_Tracker.Reset();
			}
			//////////////////////////////////////////////////////////////////////////
			ClearFinishCriteria();
			KillAllGoalThreads();

			if(GetParent() && GetParent()->GetNameHash() == 0xd9c27485 /* HighLevel */)
			{
				FINDSTATEIF(FollowPath, GetRootState(), Stop(true));
			}
		}
	}

	State::StateStatus ScriptGoal::Update(float fDt)
	{
		Prof_Scope(ScriptGoal);

#ifdef Prof_ENABLED
		Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif
		{
			Prof(Update);

			if(!m_Finished)
			{
				// check the finish criteria before running the update
				for(int i = 0; i < MaxCriteria; ++i)
				{
					if(m_FinishCriteria[i].m_Criteria!=Criteria::NONE)
					{
						if(m_FinishCriteria[i].Check(GetClient()))
							m_Finished = true;
					}
				}

				// and mapgoal too
				if(m_MapGoal)
				{
					if(m_AutoFinishOnUnavailable)
					{
						if(!m_MapGoal->IsAvailable(GetClient()->GetTeam()))
							return State_Finished;
					}

					if(m_AutoFinishOnNoProgressSlots)
					{
						if(!m_Tracker.InProgress || m_Tracker.InProgress != m_MapGoal)
							if (m_MapGoal->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) == 0)
								return State_Finished;
					}

					if(m_AutoFinishOnNoUseSlots)
					{
						if(!m_Tracker.InUse || m_Tracker.InUse != m_MapGoal)
							if (m_MapGoal->GetSlotsOpen(MapGoal::TRACK_INUSE, GetClient()->GetTeam()) == 0)
								return State_Finished;
					}
				}

				UpdateMapGoalsInRadius();
				
				if(!m_Finished)
					RunCallback(ON_UPDATE, true);
			}
		}
		return m_Finished ? State_Finished : State_Busy;
	}

	bool ScriptGoal::MarkInProgress(MapGoalPtr _p)
	{
		m_Tracker.InProgress.Reset();
		if (!_p || _p->GetSlotsOpen(MapGoal::TRACK_INPROGRESS, GetClient()->GetTeam()) > 0)
		{
			m_Tracker.InProgress.Set(_p, GetClient()->GetTeam());
			return true;
		}
		return false;
	}

	bool ScriptGoal::MarkInUse(MapGoalPtr _p) 
	{
		m_Tracker.InUse.Reset();
		if (!_p || _p->GetSlotsOpen(MapGoal::TRACK_INUSE, GetClient()->GetTeam()) > 0)
		{
			m_Tracker.InUse.Set(_p, GetClient()->GetTeam());
			return true;
		}
		return false;
	}

	void ScriptGoal::SetEnable(bool _enable, const char *_error /*= 0*/)
	{
		if(!_enable && IsActive())
			InternalExit();
		KillAllGoalThreads();
		return State::SetEnable(_enable, _error);
	}

	void ScriptGoal::SetSelectable(bool _selectable)
	{
		if(_selectable == IsSelectable())
			return;

		if(!_selectable && IsActive())
			InternalExit();
		KillAllGoalThreads();
		State::SetSelectable(_selectable);
	}

	bool ScriptGoal::AddScriptAimRequest(Priority::ePriority _prio, Aimer::AimType _type, Vector3f _v)
	{
		m_ScriptAimType = _type;
		m_AimVector = _v;
		FINDSTATE(aim,Aimer,GetRootState());
		if(aim)
		{
			if(_type==Aimer::MoveDirection)
				return aim->AddAimMoveDirRequest(_prio,GetNameHash());
			else 
				return aim->AddAimRequest(_prio,this,GetNameHash());
		}
		return false;
	}

	bool ScriptGoal::AddFinishCriteria(const CheckCriteria &_crit)
	{
		for(int i = 0; i < MaxCriteria; ++i)
		{
			if(m_FinishCriteria[i].m_Criteria == Criteria::NONE)
			{
				m_FinishCriteria[i] = _crit;
				return true;
			}
		}
		return false;
	}

	void ScriptGoal::ClearFinishCriteria(bool _clearpersistent)
	{
		for(int i = 0; i < MaxCriteria; ++i)
		{
			if(!m_FinishCriteria[i].m_Persistent || _clearpersistent)
				m_FinishCriteria[i] = CheckCriteria();
		}
	}

	void ScriptGoal::WatchForEntityCategory(float radius, const BitFlag32 &category, int customTrace)
	{
		m_WatchEntities.m_Radius = radius;
		m_WatchEntities.m_Category = category;
		m_WatchEntities.m_CustomTrace = customTrace;
		for(int i = 0; i < WatchEntity::MaxEntities; ++i)
		{
			m_WatchEntities.m_Entry[i].Reset();
		}

		if(AiState::SensoryMemory::m_pfnAddSensorCategory) 
			AiState::SensoryMemory::m_pfnAddSensorCategory(category);
	}

	void ScriptGoal::UpdateEntityInRadius()
	{
		if(m_WatchEntities.m_Category.AnyFlagSet() && m_WatchEntities.m_Radius > 0.f
			&& (AlwaysRecieveEvents() || IsActive()))
		{
			SensoryMemory *sensory = GetClient()->GetSensoryMemory();

			RecordHandle records[SensoryMemory::NumRecords];
			const int numEnts = sensory->FindEntityByCategoryInRadius(
				m_WatchEntities.m_Radius,
				m_WatchEntities.m_Category,
				records,
				SensoryMemory::NumRecords);
			
			// figure out which ones just entered, and which ones left so we can send the appropriate events
			for(int e = 0; e < numEnts; ++e)
			{
				const MemoryRecord *mr = sensory->GetMemoryRecord(records[e]);

				// does it already exist?
				bool found = false;
				int emptySlot = -1;

				bool hasLOS = true;
				if(m_WatchEntities.m_CustomTrace)
				{
					hasLOS = sensory->HasLineOfSightTo(*mr, m_WatchEntities.m_CustomTrace);
				}

				if(!hasLOS)
					continue;

				for(int i = 0; i < WatchEntity::MaxEntities; ++i)
				{
					if(m_WatchEntities.m_Entry[i].m_Ent == mr->GetEntity())
					{
						m_WatchEntities.m_Entry[i].m_TimeStamp = IGame::GetTime();
						found = true;
						break;
					} 
					else if(!m_WatchEntities.m_Entry[i].m_Ent.IsValid() && emptySlot==-1)
					{
						emptySlot = i;
					}
				}

				if(!found)
				{
					// new entity adding to list
					m_WatchEntities.m_Entry[emptySlot].m_Ent = mr->GetEntity();
					m_WatchEntities.m_Entry[emptySlot].m_TimeStamp = IGame::GetTime();

					Event_EntEnterRadius data = { mr->GetEntity() };
					GetClient()->SendEvent(
						MessageHelper(MESSAGE_ENT_ENTER_RADIUS,&data,sizeof(data)),
						GetNameHash());
				}
			}

			// check for anyone that needs exiting
			for(int i = 0; i < WatchEntity::MaxEntities; ++i)
			{
				if(m_WatchEntities.m_Entry[i].m_Ent.IsValid() && 
					m_WatchEntities.m_Entry[i].m_TimeStamp != IGame::GetTime())
				{
					Event_EntLeaveRadius data = { m_WatchEntities.m_Entry[i].m_Ent };
					GetClient()->SendEvent(
						MessageHelper(MESSAGE_ENT_LEAVE_RADIUS,&data,sizeof(data)),
						GetNameHash());

					m_WatchEntities.m_Entry[i].Reset();
				}
			}
		}
	}

	void ScriptGoal::WatchForMapGoalsInRadius(const GoalManager::Query &qry, const GameEntity & ent, float radius)
	{
		m_MapGoalInRadius.m_Query = qry;
		m_MapGoalInRadius.m_PositionEnt = ent;
		m_MapGoalInRadius.m_Radius = radius;
		m_MapGoalInRadius.m_InRadius.clear();
	}

	void ScriptGoal::ClearWatchForMapGoalsInRadius()
	{
		m_MapGoalInRadius.m_PositionEnt.Reset();
		m_MapGoalInRadius.m_Radius = 0.f;
		m_MapGoalInRadius.m_Query = GoalManager::Query();
		m_MapGoalInRadius.m_InRadius.clear();
	}

	void ScriptGoal::UpdateMapGoalsInRadius()
	{
		if(m_MapGoalInRadius.m_PositionEnt.IsValid())
		{
			Vector3f entPos;
			if(!EngineFuncs::EntityPosition(m_MapGoalInRadius.m_PositionEnt,entPos))
			{
				m_MapGoalInRadius.m_PositionEnt.Reset();
				return;
			}

			gmMachine * pMachine = ScriptManager::GetInstance()->GetMachine();

			m_MapGoalInRadius.m_Query.CheckInRadius(entPos,m_MapGoalInRadius.m_Radius);
			GoalManager::GetInstance()->GetGoals(m_MapGoalInRadius.m_Query);

			MgSet newInRadius;
			MapGoalList::iterator it = m_MapGoalInRadius.m_Query.m_List.begin(),
				itEnt = m_MapGoalInRadius.m_Query.m_List.end();
			for( ; it != itEnt; ++it )
			{
				newInRadius.insert(*it);
			}

			// look for all mapgoals that have left the radius or are still in
			for(MgSet::iterator setIt = m_MapGoalInRadius.m_InRadius.begin();
				setIt != m_MapGoalInRadius.m_InRadius.end();
				)
			{
				if ( newInRadius.find(*setIt) == newInRadius.end() ) {
					MessageHelper hlpr(MESSAGE_MG_LEAVE_RADIUS);
					CallbackParameters cb(hlpr.GetMessageId(),pMachine);
					cb.AddUserObj("MapGoal",(*setIt)->GetScriptObject(pMachine));
					InternalProcessEvent(hlpr,cb);

					MapGoalPtr mg = *setIt;
					++setIt;
					m_MapGoalInRadius.m_InRadius.erase(mg);
				} else {
					// so we can trim it down to only new ones
					newInRadius.erase(*setIt);

					// still in radius
					++setIt;
				}
			}

			// only ones left are new ones!
			for(MgSet::iterator newIt = newInRadius.begin();
				newIt != newInRadius.end();
				++newIt)
			{
				OBASSERT( m_MapGoalInRadius.m_InRadius.find(*newIt)==m_MapGoalInRadius.m_InRadius.end(),"Err");
				m_MapGoalInRadius.m_InRadius.insert(*newIt);

				MessageHelper hlpr(MESSAGE_MG_ENTER_RADIUS);
				CallbackParameters cb(hlpr.GetMessageId(),pMachine);
				cb.AddUserObj("MapGoal",(*newIt)->GetScriptObject(pMachine));
				InternalProcessEvent(hlpr,cb);
			}
		}
	}
}
