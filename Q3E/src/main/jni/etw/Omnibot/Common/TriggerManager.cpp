#include "PrecompCommon.h"
#include "TriggerManager.h"
#include "ScriptManager.h"
#include "gmTriggerInfo.h"

//////////////////////////////////////////////////////////////////////////
class TriggerShape
{
public:
	struct InTrigger
	{
		GameEntity	m_Entity;
		int			m_TimeStamp;
	};
	enum 
	{
		MaxEntCount=8,
		MaxClassCount=8,
		MaxInTrigger=128,
	};

	void SetNameHash(obuint32 _name) { m_NameHash = _name; }
	obuint32 GetNameHash() const { return m_NameHash; }
	int GetSerial() const { return m_SerialNum; }
	void SetDeleteMe() { m_DeleteMe = true; } // cs: added so script function can 'schedule' a deletion
	bool DeleteMe() const { return m_DeleteMe; }
	bool Expired() const { return DeleteMe() || (m_ExpireTime && IGame::GetTime() >= m_ExpireTime); }

	virtual void UpdatePosition(const Vector3f &pos) = 0;
	virtual bool Test(GameEntity e, const AABB &worldaabb) = 0;
	
	void SetOnEnter(gmGCRoot<gmFunctionObject> &_onenter)
	{
		m_OnEnterFunction = _onenter;
	}
	void SetOnExit(gmGCRoot<gmFunctionObject> &_onexit)
	{
		m_OnExitFunction = _onexit;
	}

	bool TriggerableEntity(const EntityInstance &_ent)
	{
		// check category
		if(m_TriggerOnCategory.AnyFlagSet())
		{
			if((_ent.m_EntityCategory & m_TriggerOnCategory).AnyFlagSet())
				return true;
		}

		// check class
		if(m_TriggerOnClass[0])
		{
			for(int i = 0; i < MaxClassCount; ++i)
			{
				if(m_TriggerOnClass[i]==0)
					break;

				if(m_TriggerOnClass[i] == FilterSensory::ANYPLAYERCLASS)
				{
					if(_ent.m_EntityClass < FilterSensory::ANYPLAYERCLASS)
						return true;
				}

				if(m_TriggerOnClass[i] == _ent.m_EntityClass)
					return true;
			}
		}

		// check specific entities.
		if(m_TriggerOnEntity[0].IsValid())
		{
			for(int i = 0; i < MaxEntCount; ++i)
			{
				if(!m_TriggerOnEntity[i].IsValid())
					break;

				if(m_TriggerOnEntity[i] == _ent.m_Entity)
					return true;
			}
		}
		return false;
	}

	void FireEnterEvent(GameEntity _ent)
	{
		if(m_OnEnterFunction)
		{
			gmMachine *m = ScriptManager::GetInstance()->GetMachine();

			gmCall call;
			if(call.BeginFunction(m,m_OnEnterFunction,m_ThisObject))
			{
				call.AddParamEntity(_ent.AsInt());
				call.End();
				
				int iRet = 0;
				if(call.GetReturnedInt(iRet) && iRet==1)
					m_DeleteMe = true;
			}
		}
	}
	void FireExitEvent(GameEntity _ent)
	{
		if(m_OnExitFunction)
		{
			gmMachine *m = ScriptManager::GetInstance()->GetMachine();

			gmCall call;
			if(call.BeginFunction(m,m_OnExitFunction,m_ThisObject))
			{
				call.AddParamEntity(_ent.AsInt());
				call.End();

				int iRet = 0;
				if(call.GetReturnedInt(iRet) && iRet==1)
					m_DeleteMe = true;
			}
		}
	}

	void FireTrigger(const EntityInstance &_ent)
	{
		if(Expired()) return;

		InTrigger *it = NULL;

		// If it's already in the trigger, just update the timestamp.
		for(int i = 0; i < MaxInTrigger; ++i)
		{
			if(!it && !m_InTrigger[i].m_Entity.IsValid())
				it = &m_InTrigger[i];

			if(m_InTrigger[i].m_Entity == _ent.m_Entity)
			{
				m_InTrigger[i].m_TimeStamp = IGame::GetTime();
				return;
			}
		}

		if(it)
		{
			// otherwise add it to the list of entities in trigger, and fire an enter event.
			it->m_Entity = _ent.m_Entity;
			it->m_TimeStamp = IGame::GetTime();
			FireEnterEvent(_ent.m_Entity);
		}
		else
		{
			OBASSERT(0,"Out of InTrigger slots!");
		}
	}

	void Update()
	{
		// fire exit events for any expired objects.
		for(int i = 0; i < MaxInTrigger; ++i)
		{
			if(m_InTrigger[i].m_Entity.IsValid() &&
				m_InTrigger[i].m_TimeStamp != IGame::GetTime())
			{
				FireExitEvent(m_InTrigger[i].m_Entity);

				m_InTrigger[i].m_Entity.Reset();
				m_InTrigger[i].m_TimeStamp = 0;
			}
		}
	}

	virtual void RenderDebug()
	{
		for(int i = 0; i < MaxInTrigger; ++i)
		{
			if(m_InTrigger[i].m_Entity.IsValid())
			{
				AABB bounds;
				if(EngineFuncs::EntityWorldAABB(m_InTrigger[i].m_Entity,bounds))
				{
					Utils::OutlineAABB(bounds,COLOR::ORANGE,2.f);
					
					Vector3f center;
					bounds.CenterPoint( center );
					String name = Utils::HashToString(GetNameHash());
					Utils::PrintText( center, COLOR::ORANGE, 2.0f, "%s", name.c_str() );
				}
			}
		}
	}

	bool FromTable(gmMachine *_m, gmTableObject *tbl)
	{
		gmVariable vName = tbl->Get(_m,"Name");
		gmVariable vOnEnter = tbl->Get(_m,"OnEnter");
		gmVariable vOnExit = tbl->Get(_m,"OnExit");
		gmVariable vUpdateDelay = tbl->Get(_m,"UpdateDelay");
		m_ThisObject = tbl->Get(_m,"CallbackObject");

		if(vOnEnter.GetFunctionObjectSafe())
			m_OnEnterFunction.Set(vOnEnter.GetFunctionObjectSafe(),_m);
		if(vOnExit.GetFunctionObjectSafe())
			m_OnExitFunction.Set(vOnExit.GetFunctionObjectSafe(),_m);

		// must have at least 1 callback
		if(!m_OnEnterFunction && !m_OnExitFunction)
			return false;

		bool bHasTriggerCondition = false;

		{
			gmVariable vTriggerOnClass = tbl->Get(_m,"TriggerOnClass");
			if(vTriggerOnClass.IsInt())
			{
				bHasTriggerCondition = true;
				m_TriggerOnClass[0] = vTriggerOnClass.GetInt();
			}
			else
			{
				gmTableObject *tbl2 = vTriggerOnClass.GetTableObjectSafe();
				if(tbl2)
				{
					gmTableIterator tIt;
					gmTableNode *pNode = tbl2->GetFirst(tIt);
					while(pNode)
					{
						if(pNode->m_value.IsInt())
						{
							for(int c = 0; c < MaxClassCount; ++c)
							{
								if(m_TriggerOnClass[c]==0)
								{
									bHasTriggerCondition = true;
									m_TriggerOnClass[c] = pNode->m_value.GetInt();
									break;
								}
							}
						}
						pNode = tbl2->GetNext(tIt);
					}
				}
			}
		}
		{
			gmVariable vTriggerOnCategory = tbl->Get(_m,"TriggerOnCategory");
			if(vTriggerOnCategory.IsInt())
			{
				bHasTriggerCondition = true;
				m_TriggerOnCategory.SetFlag(vTriggerOnCategory.GetInt(),true);
			}
			else
			{
				gmTableObject *tbl2 = vTriggerOnCategory.GetTableObjectSafe();
				if(tbl2)
				{
					gmTableIterator tIt;
					gmTableNode *pNode = tbl2->GetFirst(tIt);
					while(pNode)
					{
						if(pNode->m_value.IsInt())
						{
							m_TriggerOnCategory.SetFlag(pNode->m_value.GetInt(),true);
						}
						pNode = tbl2->GetNext(tIt);
					}
				}
			}
		}
		{
			gmVariable vTriggerOnEntity = tbl->Get(_m,"TriggerOnEntity");
			if(vTriggerOnEntity.IsEntity())
			{
				bHasTriggerCondition = true;
				m_TriggerOnEntity[0].FromInt(vTriggerOnEntity.GetEntity());
			}
			else
			{
				gmTableObject *tbl2 = vTriggerOnEntity.GetTableObjectSafe();
				if(tbl2)
				{
					gmTableIterator tIt;
					gmTableNode *pNode = tbl2->GetFirst(tIt);
					while(pNode)
					{
						if(pNode->m_value.IsEntity())
						{
							for(int i = 0; i < MaxClassCount; ++i)
								if(!m_TriggerOnEntity[i].IsValid())
								{
									bHasTriggerCondition = true;
									m_TriggerOnEntity[i].FromInt(pNode->m_value.GetEntity());
								}
						}
						pNode = tbl2->GetNext(tIt);
					}
				}
			}
		}

		if(vName.GetCStringSafe(NULL))
			m_NameHash = Utils::MakeHash32(vName.GetCStringSafe());
		if(vUpdateDelay.IsNumber())
			m_UpdateDelay = Utils::SecondsToMilliseconds(vUpdateDelay.GetFloatSafe());

		return bHasTriggerCondition;
	}

	TriggerShape()
		: m_NameHash(0)
		, m_ExpireTime(0)
		, m_UpdateDelay(0)
		, m_SerialNum(0)
		, m_ThisObject(gmVariable::s_null)
		, m_DeleteMe(false)
	{
		static int sNextSerial = 1;

		m_SerialNum = sNextSerial++;

		for(int i = 0; i < MaxEntCount; ++i)
			m_TriggerOnEntity[i].Reset();
		for(int i = 0; i < MaxClassCount; ++i)
			m_TriggerOnClass[i] = 0;
		for(int i = 0; i < MaxInTrigger; ++i)
		{
			m_InTrigger[i].m_Entity.Reset();
			m_InTrigger[i].m_TimeStamp = 0;
		}
	}
	virtual ~TriggerShape() {}
private:
	obuint32	m_NameHash;
	int			m_ExpireTime;

	int			m_UpdateDelay;

	int			m_SerialNum;
	
	GameEntity	m_TriggerOnEntity[MaxEntCount];
	int			m_TriggerOnClass[MaxClassCount];
	BitFlag32	m_TriggerOnCategory;

	InTrigger	m_InTrigger[MaxInTrigger];

	gmGCRoot<gmFunctionObject>	m_OnEnterFunction;
	gmGCRoot<gmFunctionObject>	m_OnExitFunction;

	gmVariable					m_ThisObject;

	bool		m_DeleteMe;
};
//////////////////////////////////////////////////////////////////////////
class TriggerShapeSphere : public TriggerShape
{
public:
	void UpdatePosition(const Vector3f &pos)
	{
		m_Position = pos;
	}
	bool Test(GameEntity e, const AABB &worldaabb)
	{
		Vector3f center;
		worldaabb.CenterPoint(center);
		return SquaredLength(center,m_Position) <= Mathf::Sqr(m_Radius);
	}
	void RenderDebug()
	{
		TriggerShape::RenderDebug();
		Utils::DrawRadius(m_Position,m_Radius,COLOR::GREEN,2.f);

		String name = Utils::HashToString(GetNameHash());
		Utils::PrintText( m_Position, COLOR::GREEN, 2.0f, "%s", name.c_str() );
	}
	TriggerShapeSphere(const Vector3f &p, float r) 
		: m_Position(p)
		, m_Radius(r)
	{
	}
private:
	Vector3f	m_Position;
	float		m_Radius;
};
//////////////////////////////////////////////////////////////////////////
class TriggerShapeAABB : public TriggerShape
{
public:
	void UpdatePosition(const Vector3f &pos)
	{
		Vector3f center;
		m_Bounds.CenterPoint(center);
		m_Bounds.UnTranslate(center);
		m_Bounds.Translate(pos);
	}
	bool Test(GameEntity e, const AABB &worldaabb)
	{
		return m_Bounds.Intersects(worldaabb);
	}
	void RenderDebug()
	{
		TriggerShape::RenderDebug();
		Utils::OutlineAABB(m_Bounds,COLOR::GREEN,2.f);

		Vector3f center;
		m_Bounds.CenterPoint( center );
		String name = Utils::HashToString(GetNameHash());
		Utils::PrintText( center, COLOR::GREEN, 2.0f, "%s", name.c_str() );
	}
	TriggerShapeAABB(const AABB &aabb)
		: m_Bounds(aabb) 
	{
	}
private:
	AABB	m_Bounds;
};

int TriggerManager::AddTrigger(const Vector3f &_pos, float _radius, gmMachine *_m, gmTableObject *tbl)
{
	ShapePtr trig(new TriggerShapeSphere(_pos,_radius));
	if(trig->FromTable(_m,tbl))
	{
		m_TriggerShapes.push_back(trig);
		return trig->GetSerial();
	}
	return 0;
}

int TriggerManager::AddTrigger(const AABB &_aabb, gmMachine *_m, gmTableObject *tbl)
{
	ShapePtr trig(new TriggerShapeAABB(_aabb));
	if(trig->FromTable(_m,tbl))
	{
		m_TriggerShapes.push_back(trig);
		return trig->GetSerial();
	}
	return 0;
}

void TriggerManager::DeleteTrigger(int _serial)
{
	ShapeList::iterator it = m_TriggerShapes.begin();
	for(; it != m_TriggerShapes.end(); )
	{
		if((*it)->GetSerial() == _serial)
		{
			//it = m_TriggerShapes.erase(it);
			(*it)->SetDeleteMe(); // update handles the delete now
			return;
		}
		++it;
	}
}

void TriggerManager::DeleteTrigger(const String &_name)
{
	const obuint32 uiName = Utils::Hash32(_name.c_str());
	ShapeList::iterator it = m_TriggerShapes.begin();
	for(; it != m_TriggerShapes.end(); )
	{
		if((*it)->GetNameHash() == uiName)
		{
			// cs: this crashes if trigger is active
			//it = m_TriggerShapes.erase(it);
			//continue;

			(*it)->SetDeleteMe(); // update handles the delete now
			//return;
		}
		++it;
	}
}

//////////////////////////////////////////////////////////////////////////

TriggerManager *TriggerManager::m_Instance = 0;

TriggerManager::TriggerManager()
	: m_NextDrawTime(0)
	, m_DebugTriggers(false)
	, m_DrawTriggers(false)
{
	InitCommands();
}

TriggerManager *TriggerManager::GetInstance()
{
	if(!m_Instance)
		m_Instance = new TriggerManager;
	return m_Instance;
}

void TriggerManager::DeleteInstance()
{
	OB_DELETE(m_Instance);
}

void TriggerManager::InitCommands()
{
	SetEx("debugtriggers", "Prints triggers to console", 
		this, &TriggerManager::cmdDebugTriggers);
	SetEx("drawtriggers", "Renders any active trigger zones", 
		this, &TriggerManager::cmdDrawTriggers);

	//////////////////////////////////////////////////////////////////////////
	Options::GetValue("Debug Render","DrawTriggers",m_DrawTriggers);
	Options::GetValue("Debug Render","DebugTriggers",m_DebugTriggers);
	//////////////////////////////////////////////////////////////////////////
}

void TriggerManager::cmdDrawTriggers(const StringVector &_args)
{
	if(_args.size() >= 2)
	{
		if(!m_DrawTriggers && Utils::StringToTrue(_args[1]))
			m_DrawTriggers = true;							
		else if(m_DrawTriggers && Utils::StringToFalse(_args[1]))
			m_DrawTriggers = false;
	}
	else
		m_DrawTriggers = !m_DrawTriggers;

	if(m_DrawTriggers)
		EngineFuncs::ConsoleMessage("Trigger Drawing on.");
	else
		EngineFuncs::ConsoleMessage("Trigger Drawing off.");
}

void TriggerManager::cmdDebugTriggers(const StringVector &_args)
{
	int numArgs = (int)_args.size();
	m_DebugTriggersExpr = ".*";

	if(numArgs >= 2)
	{
		if(!m_DebugTriggers && Utils::StringToTrue(_args[1]))
			m_DebugTriggers = true;
		else if(m_DebugTriggers && Utils::StringToFalse(_args[1]))
			m_DebugTriggers = false;

		if (numArgs >= 3)
			m_DebugTriggersExpr = va("%s",_args[2].c_str());
	}
	else
		m_DebugTriggers = !m_DebugTriggers;

	if(m_DebugTriggers)
		EngineFuncs::ConsoleMessage("Trigger Debug on.");
	else
		EngineFuncs::ConsoleMessage("Trigger Debug off.");
}

void TriggerManager::SetScriptCallback(const String &_name, gmGCRoot<gmFunctionObject> _func)
{
	m_ScriptCallbacks.insert(std::make_pair(_name, _func));
}

void TriggerManager::HandleTrigger(const TriggerInfo &_triggerInfo)
{
	bool bScriptCallback = false;	

	//////////////////////////////////////////////////////////////////////////	
	// Call any script callbacks.
	if(_triggerInfo.m_TagName[0])
	{
		ScriptCallback::iterator it = m_ScriptCallbacks.lower_bound(_triggerInfo.m_TagName), 
			itEnd = m_ScriptCallbacks.upper_bound(_triggerInfo.m_TagName);
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
		DisableGCInScope gcEn(pMachine);

		for( ; it != itEnd; ++it) 
		{
			gmCall call;
			if(call.BeginFunction(pMachine, it->second, gmVariable::s_null, true, gmThread::Highest))
			{
				gmUserObject *pTi = gmTriggerInfo::WrapObject(pMachine, new TriggerInfo(_triggerInfo));
				gmTriggerInfo::SetObjectOwnershipGM(pMachine, pTi);
				call.AddParamUser(pTi);
				call.End();
				bScriptCallback = true;
			}
		}
	}	

	if(m_DebugTriggers)
	{
		if ( _triggerInfo.m_TagName[0] && Utils::RegexMatch(m_DebugTriggersExpr.c_str(), va("%s",_triggerInfo.m_TagName)) )
		{
			StringStr msg;
			msg << "<" << (bScriptCallback ? "+++" : "---") << ">" << (_triggerInfo);
			EngineFuncs::ConsoleMessage(msg.str().c_str());
			LOG(msg.str().c_str());
			
			Utils::OutputDebug(kInfo, msg.str().c_str());
		}
	}
}

bool operator<(const GameEntity& _v1, const GameEntity& _v2)
{
	return _v1.AsInt() < _v2.AsInt();
}

//////////////////////////////////////////////////////////////////////////

void TriggerManager::Update()
{
	Prof(TriggerManager_Update);

	if(m_TriggerShapes.empty()) return;

	//fire Enter events
	AABB bounds;
	BitFlag64 flags;
	IGame::EntityIterator ent;
	while(IGame::IterateEntity(ent))
	{
		bool aabbValid = false, flagValid = false;
		flags.ClearAll();
		for(int x = (int) m_TriggerShapes.size() - 1; x >= 0; --x)
		{
			TriggerShape* shape = m_TriggerShapes[x].get();
			if(shape->TriggerableEntity(ent.GetEnt()))
			{
				if(aabbValid || (aabbValid = EngineFuncs::EntityWorldAABB(ent.GetEnt().m_Entity, bounds))!=false)
				{
					if(shape->Test(ent.GetEnt().m_Entity, bounds))
					{
						if (!(ent.GetEnt().m_EntityClass < FilterSensory::ANYPLAYERCLASS
							&& (flagValid || (flagValid = InterfaceFuncs::GetEntityFlags(ent.GetEnt().m_Entity, flags))!=false)
							&& flags.CheckFlag(ENT_FLAG_DISABLED))) //LIMBO
						{
							shape->FireTrigger(ent.GetEnt());
						}
					}
				}
			}
		}
	}

	//Render, fire Exit events, Delete
	unsigned int x = 0;
	for(; x < m_TriggerShapes.size(); )
	{
		if(m_DrawTriggers && m_NextDrawTime < IGame::GetTime())
			m_TriggerShapes[ x ]->RenderDebug();

		m_TriggerShapes[ x ]->Update();

		if( m_TriggerShapes[ x ]->Expired())
		{
			m_TriggerShapes.erase(m_TriggerShapes.begin()+x);
			continue;
		}

		++x;
	}

	if(m_NextDrawTime < IGame::GetTime())
		m_NextDrawTime = IGame::GetTime() + 2000;
}

//////////////////////////////////////////////////////////////////////////
