#ifndef __TRIGGERMANAGER_H__
#define __TRIGGERMANAGER_H__

#include "CommandReciever.h"
#include "GoalManager.h"

class TriggerShape;
class gmFunctionObject;

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<TriggerShape> ShapePtr;
#else
typedef boost::shared_ptr<TriggerShape> ShapePtr;
#endif
typedef std::vector<ShapePtr> ShapeList;

bool operator<(const GameEntity& _1, const GameEntity& _2);

//////////////////////////////////////////////////////////////////////////

// class: TriggerManager
//		Handles triggers recieved from the game. Calls necessary script
//		or function callbacks for any trigger that has callbacks registered
class TriggerManager : public CommandReciever
{
public:
	// typedef: TriggerSignal
	typedef std::map<GameEntity, MapGoalWPtr> TriggerMap;
    
	// typedef: ScriptCallback
	typedef std::multimap<String, gmGCRoot<gmFunctionObject> > ScriptCallback;

	void SetScriptCallback(const String &_name, gmGCRoot<gmFunctionObject> _func);

	void HandleTrigger(const TriggerInfo &_triggerInfo);

	void Update();

	//////////////////////////////////////////////////////////////////////////
	int AddTrigger(const Vector3f &_pos, float _radius, gmMachine *_m, gmTableObject *tbl);
	int AddTrigger(const AABB &_aabb, gmMachine *_m, gmTableObject *tbl);
	void DeleteTrigger(int _serial);
	void DeleteTrigger(const String &_name);
	//////////////////////////////////////////////////////////////////////////

	static TriggerManager *GetInstance();
	static void DeleteInstance();	
protected:
	ScriptCallback		m_ScriptCallbacks;

	ShapeList			m_TriggerShapes;

	// Commands
	virtual void InitCommands();
	void cmdDebugTriggers(const StringVector &_args);
	void cmdDrawTriggers(const StringVector &_args);

	int		m_NextDrawTime;

	String 	m_DebugTriggersExpr;

	bool	m_DebugTriggers;
	bool	m_DrawTriggers;

	static TriggerManager	*m_Instance;

	TriggerManager();
	~TriggerManager() {}
	TriggerManager &operator=(const TriggerManager&);
};

#endif