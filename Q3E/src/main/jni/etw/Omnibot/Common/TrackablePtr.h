#ifndef __TRACKABLEPTR_H__
#define __TRACKABLEPTR_H__

// class: TrackablePtr
template<class Type, int TrackType>
class TrackablePtr
{
public:
	typedef TrackablePtr<Type, TrackType> TrackPtr;
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
	typedef std::shared_ptr<Type> ShPtr;
	typedef std::weak_ptr<Type> WPtr;
#else
	typedef boost::shared_ptr<Type> ShPtr;
	typedef boost::weak_ptr<Type> WPtr;
#endif

	TrackablePtr() : 
		m_TrackType(TrackType) 
	{
	}	
	TrackablePtr(const TrackPtr &_ptr) : 
		m_TrackType(TrackType)
	{
		*this = _ptr; 
	}
	~TrackablePtr() 
	{ 
		ShPtr shPtr = m_pObject.lock();
		if(shPtr)
		{
			shPtr->DelReference(m_TrackType, m_Team);
		}			
	}

	///////////////////////////
	// Assignement operators //
	///////////////////////////	

	// Assigning a shared pointer
	inline void Set(ShPtr &_obj, int _team)
	{
		// Release the obj if necessary to decrement the reference counter.
		if(!m_pObject.expired())
		{
			ShPtr shPtr = m_pObject.lock();

			// assigning the same thing!
			if(shPtr == _obj)
				return;

			if(shPtr)
			{
				shPtr->DelReference(m_TrackType, m_Team);
			}
		}		
				
		// Assign the new object
		m_pObject = _obj;
		m_Team = _team;
		
		// Addref to increment the new objects reference counter.
		if(!m_pObject.expired())
		{
			ShPtr shPtr = m_pObject.lock();
			if(shPtr)
			{
				shPtr->AddReference(m_TrackType, _team);
			}
		}	
	}
	// Assigning a shared pointer
	inline void Set(WPtr &_obj, int _team)
	{
		// assigning the same thing!
		if(m_pObject == _obj)
			return;

		// Release the obj if necessary to decrement the reference counter.
		ShPtr shPtr = m_pObject.lock();
		if(shPtr)
		{
			shPtr->DelReference(m_TrackType, m_Team);
		}

		// Assign the new object
		m_pObject = _obj;
		m_Team = _team;

		// Addref to increment the new objects reference counter.
		ShPtr shPtr2 = m_pObject.lock();
		if(shPtr2)
		{
			shPtr2->AddReference(m_TrackType, _team);
		}
	}
	// comparison
	inline bool operator==(ShPtr &_obj)
	{
		ShPtr shPtr = m_pObject.lock();
		return shPtr == _obj;
	}
	inline bool operator!=(ShPtr &_obj)
	{
		ShPtr shPtr = m_pObject.lock();
		return shPtr != _obj;
	}
	inline void Reset()
	{
		// Release the obj if necessary to decrement the reference counter.
		ShPtr shPtr = m_pObject.lock();
		if(shPtr)
		{
			shPtr->DelReference(m_TrackType, m_Team);
		}

		// Clear the reference.
		m_pObject.reset();
	}

	///////////////
	// Accessors //
	///////////////
	operator bool() const { return !m_pObject.expired(); }
	// Access as a reference
	/*inline Type& operator*() const
	{
		assert(m_pObject && "Tried to * on a NULL TrackablePtr");
		return *m_pObject;
	}*/
	// Access as a pointer
	/*inline Type* operator->() const
	{
		assert(m_pObject && "Tried to -> on a NULL TrackablePtr");
		return m_pObject;
	}*/

	////////////////////////////////////
	// Conversions to normal pointers //
	////////////////////////////////////
	/*inline operator Type*() const { return m_pObject; }
	inline bool isValid() const { return (m_pObject!=0); }
	inline bool operator!() { return !(m_pObject); }
	inline bool operator==(const TrackablePtr<Type, TrackType> &p) const { return (m_pObject == p.m_pObject); }
	inline bool operator ==(const Type* o) const { return (m_pObject==o); }*/
private:
	WPtr	m_pObject;
	int		m_TrackType;
	int		m_Team;
};

#endif
