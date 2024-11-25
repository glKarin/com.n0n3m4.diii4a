#ifndef __INTERNALFSM_H__
#define __INTERNALFSM_H__

#define STATE_PROTOTYPE(s) \
	void s##Enter(); \
	void s##Update(); \
	void s##Exit();

#define REGISTER_STATE(T, s) \
	RegState(s, &T::s##Enter, &T::s##Update, &T::s##Exit);

#define S_ENTER(s) s##Enter
#define S_UPDATE(s) s##Update
#define S_EXIT(s) s##Exit

#define STATE_ENTER(T, s) \
	void T::s##Enter()	

#define STATE_UPDATE(T, s) \
	void T::s##Update()

#define STATE_EXIT(T, s) \
	void T::s##Exit()

template <typename T, int MaxStates = 8>
class InternalFSM
{
public:
	typedef void (T::*Func)();

	void RegState(int state, Func Enter, Func Update, Func Exit)
	{
		OBASSERT(state>=0 && state<MaxStates,"State index out of bounds");
		if ( state>=0 && state<MaxStates ) {
			AvailableStates[state].Reset();
			AvailableStates[state].Enter = Enter;
			AvailableStates[state].Update = Update;
			AvailableStates[state].Exit = Exit;
		}
	}

	struct StateBlock
	{
		Func Enter;
		Func Update;
		Func Exit;

		int StateId;

		bool IsValid()
		{
			return Enter || Exit || Update;
		}
		void Reset()
		{
			Enter = 0;
			Update = 0;
			Exit = 0;
			StateId = -1;
		}

		StateBlock()
			: Enter(0)
			, Update(0)
			, Exit(0)
			, StateId(-1)
		{
		}
	};

	void ResetForNewState()
	{
		StateTime = 0.f;
	}

	void SetNextState(int state)
	{
		OBASSERT(state>=0 && state<MaxStates,"State index out of bounds");
		if ( state>=0 && state<MaxStates ) {
			NextState = AvailableStates[state];
			NextState.StateId = state;
		}
		OBASSERT(NextState.IsValid(),"Not a valid state!");
	}

	int GetCurrentStateId()
	{
		return CurrentState.StateId;
	}

	void UpdateFsm(float dt)
	{
		StateTime += dt;

		T * ptr = static_cast<T*>(this);

		// update current state
		if(CurrentState.Update)
			(ptr->*CurrentState.Update)();

		// transition into next state
		if(NextState.IsValid())
		{
			if(CurrentState.Exit)
				(ptr->*CurrentState.Exit)();

			CurrentState = NextState;
			NextState.Reset();

			ResetForNewState();

			if(CurrentState.Enter)
				(ptr->*CurrentState.Enter)();
		}
	}

	InternalFSM()
	{
		for(int i = 0; i < MaxStates; ++i)
		{
			AvailableStates[i].Reset();
		}
	}
	virtual ~InternalFSM() {}
private:
	StateBlock	CurrentState;
	StateBlock	NextState;

	float		StateTime;

	StateBlock	AvailableStates[MaxStates];
};

#endif
