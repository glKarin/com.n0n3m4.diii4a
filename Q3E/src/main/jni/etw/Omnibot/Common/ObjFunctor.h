#ifndef __OBJFUNCTOR_H__
#define __OBJFUNCTOR_H__

#include "Functor.h"

// class: ObjFunctor
template<class T>
class ObjFunctor : public Functor
{
public:
	typedef int (T::*funcType)();
    ObjFunctor(T *o, funcType f) 
    { 
        obj = o; 
        m_Function = f;
    }
    int operator ()()
    {
        return (obj->*m_Function)(); 
    }
protected:
	T *obj;
	
	funcType m_Function;
};

#endif
