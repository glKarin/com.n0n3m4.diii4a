#ifndef __AUTOCRIT_H__
#define __AUTOCRIT_H__

#ifdef RV_USE_CRITICALSECTION
// This class eliminates the need to figure out where to call InitializeCriticalSection
// and DeleteCriticalSection.  It makes critical sections into a meaningful class.  More
// importantly, now when they are created as statics, they are already initialized, so
// the AutoCrit doesn't have to worry about checking to see if they've been initialized yet
class CriticalSection
{
public:
	ID_INLINE CriticalSection() 
	{ 
		InitializeCriticalSection(&mCrit); 
	}
	ID_INLINE ~CriticalSection() 
	{ 
		DeleteCriticalSection(&mCrit); 
	}

	ID_INLINE void Enter() 
	{ 
		while(!Try())
		{
			Sleep(0);	// give another thread a chance to run to try to eliminate deadlock
		}
	}
	ID_INLINE void Leave() 
	{ 
		LeaveCriticalSection(&mCrit); 
	}

	ID_INLINE bool Try()
	{
		return TryEnterCriticalSection(&mCrit) != 0;
	}

private:
	CRITICAL_SECTION mCrit;
};
#endif

#ifdef RV_USE_AUTOCRIT
// Enters a critical section unique to the type of object t on construction
// Exits that critical section on destruction.  Effectively synchronizes calls
// to ANY instance of type t
template <typename t>
class AutoCrit
{
public:
	ID_INLINE AutoCrit() 
	{ 
		sCrit.Enter();
	}
	ID_INLINE ~AutoCrit() 
	{ 
		sCrit.Leave();
	}
private:
	static CriticalSection sCrit;
};

template<typename t>
CriticalSection AutoCrit<t>::sCrit;

template <typename t>
class ConditionalAutoCrit
{
public:
	ID_INLINE ConditionalAutoCrit()
	{
		if(sThreading)
		{
			sCrit.Enter();
			mMustLeave = true;
		}
		else
		{
			mMustLeave = false;
		}
	}
	ID_INLINE ~ConditionalAutoCrit()
	{
		if(mMustLeave)
		{
			sCrit.Leave();
		}
	}

	static ID_INLINE SetThreading(bool threading)
	{
		// not threadsafe...must be called BEFORE entering and AFTER leaving a threaded section, not during
		sThreading = threading;
	}

	static ID_INLINE bool IsThreading()
	{
		return sThreading;
	}

private:
	static bool				sThreading;
	static CriticalSection	sCrit;
	bool						mMustLeave;
};

template<typename t>
CriticalSection ConditionalAutoCrit<t>::sCrit;

template<typename t>
bool ConditionalAutoCrit<t>::sThreading=false;

#else
// these compile out completely in release and removes the need
// for ifdefs all over the code
template <typename t>
class AutoCrit
{
public:
	AutoCrit() { }
	~AutoCrit() { }
};

template <typename t>
class ConditionalAutoCrit
{
public:
	ConditionalAutoCrit() { }
	~ConditionalAutoCrit() { }
};
#endif

#ifdef RV_USE_AUTOINSTANCECRIT

class CritInfo
{
public:
	ID_INLINE CritInfo() { refCount = 0; }
	int refCount;
	CriticalSection crit;
};


// Enters a critical section unique to the specific instance of an object of type t on construction
// Exits that critical section on destruction.  This is templated instead of using void * in order
// to reduce search time when entering and leaving.  Effectively synchronizes on a single instance
// of an object.  This would allow containers for example to not be synchronized to other containers
// of the same type.
template<typename t>
class AutoInstanceCrit
{
public:

	ID_INLINE AutoInstanceCrit(t *ptr)
	{
		CritInfo *critPtr = NULL;
		//AutoCrit<AutoInstanceCrit<t> > crit;			// protect access to the map
		critSect.Enter();
		mPtr = ptr;
		int i = sPtrs.FindIndex(mPtr);

		if(i == -1)	// not found
		{
			i = sPtrs.Num();
			sPtrs.Append(mPtr);
			sInfos.Append(CritInfo());
		}
		critPtr = &(sInfos[i]);
		critPtr->refCount++;
		critSect.Leave();

		critPtr->crit.Enter();
	}

	ID_INLINE ~AutoInstanceCrit() 
	{ 
		CritInfo *critPtr = NULL;
		{
			//AutoCrit<AutoInstanceCrit<t> > crit;			// protect access to the map
			critSect.Enter();
			int i = sPtrs.FindIndex(mPtr);
			assert(i != -1);
			critPtr = &(sInfos[i]);
			critPtr->refCount--;								// decrement refcount
			critSect.Leave();
		}
		critPtr->crit.Leave();
		/*
		if(critPtr->refCount == 0)
		{
			sPtrs.RemoveIndex(i);
			sInfos.RemoveIndex(i);
		}*/
	}

private:
	t *mPtr;		// the pointer we're protecting
	static CriticalSection critSect;
	static idList<t *> sPtrs;
	static idList<CritInfo> sInfos;
};
template<typename t>
idList<t *> AutoInstanceCrit<t>::sPtrs;

template<typename t>
idList<CritInfo> AutoInstanceCrit<t>::sInfos;

template<typename t>
CriticalSection AutoInstanceCrit<t>::critSect;

#else
// this compiles out completely in release and removes the need
// for ifdefs all over the code
template <typename t>
class AutoInstanceCrit
{
public:
	ID_INLINE AutoInstanceCrit(t *ptr) { }
};
#endif

#endif
