#ifndef __AUTOPTR_H__
#define __AUTOPTR_H__

// this class is NOT safe for array new's.  It will not properly call
// the destructor for each element and you will silently leak memory.
// it does work for classes requiring no destructor however(base types)
template<typename type>
class idAutoPtr
{
public:
	explicit idAutoPtr(type *ptr = 0)
		: mPtr(ptr)
		{
		}

	~idAutoPtr()
	{
		delete mPtr;
	}

	type &operator*() const
		{
		return *mPtr;
		}

	type *operator->() const
	{
		return &**this;
	}

	type *get() const
	{
		return mPtr;
	}

	type *release()
	{
		type *ptr = mPtr;
		mPtr = NULL;
		return ptr;
		}

	void reset(type *ptr = NULL)
	{
		if (ptr != mPtr)
			delete mPtr;
		mPtr = ptr;
	}

	operator type*()
	{
		return get();
	}

private:
	// disallow copies
	idAutoPtr<type> &operator=(idAutoPtr<type>& ptr);
	idAutoPtr(idAutoPtr<type>& ptr);

	type *mPtr;
};

#endif

