#ifndef __BOTTARGETINGSYSTEM_H__
#define __BOTTARGETINGSYSTEM_H__

#include "FilterSensory.h"

class Client;

namespace AiState
{
	// class: TargetingSystem
	class TargetingSystem : public StateChild
	{
	public:

		// function: HasTarget
		//		true/false whether the bot has a target or not.
		inline bool HasTarget() const { return m_CurrentTarget.IsValid(); }

		// function: GetCurrentTarget
		//		Accessor for the bots current target.
		inline GameEntity GetCurrentTarget() const { return m_CurrentTarget; }

		// function: GetLastTarget
		//		Accessor for the bots last target.
		inline const GameEntity &GetLastTarget() const { return m_LastTarget; }

		void ForceTarget(GameEntity _ent);

		// function: GetCurrentTargetRecord
		//		Accessor for the bots current target data.
		const MemoryRecord *GetCurrentTargetRecord() const;

		// function: SetDefaultTargetingFilter
		//		Set the <FilterSensory> that will be used for targeting
		inline void SetDefaultTargetingFilter(FilterPtr _filter) { m_DefaultFilter = _filter; }

		// function: GetTargetingFilter
		//		Get the current <FilterSensory> being used for targeting
		inline FilterPtr GetTargetingFilter() const { return m_DefaultFilter; }

		// State stuff

		void RenderDebug();

		void Initialize();
		void Exit();
		StateStatus Update(float fDt);

		TargetingSystem();
		virtual ~TargetingSystem();
	protected:
		// ptr: m_DefaultFilter
		//		Pointer to the filter to use for targeting.
		FilterPtr		m_DefaultFilter;

		// var: m_CurrentTarget
		//		The currently targeted entity
		GameEntity		m_CurrentTarget;

		// var: m_LastTarget
		//		The last targeted entity
		GameEntity		m_LastTarget;

		GameEntity		m_ForceTarget;
	};
}
#endif
