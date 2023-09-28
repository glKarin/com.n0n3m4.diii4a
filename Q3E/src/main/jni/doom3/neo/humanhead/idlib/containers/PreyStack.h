#ifndef _HH_STACK_
#define _HH_STACK_

/*
================================================================================
 hhStack:
 List based stack
================================================================================
*/

template< class type >
class hhStack : public idList<type> {
public:
							hhStack(int newgranularity=16);
							~hhStack<type>();
	type					Top(void);
	type					Pop(void);
	void					Push(type &object);
	bool					Empty(void);
	void					Clear(void);
};

template< class type >
hhStack<type>::hhStack(int newgranularity) {
	this->SetGranularity(newgranularity);
}

template< class type >
hhStack<type>::~hhStack(void) {
}

template< class type >
ID_INLINE type hhStack<type>::Top(void) {
	assert(this->Num() > 0);

	return this->list[this->Num()-1];
}

template< class type >
ID_INLINE type hhStack<type>::Pop(void) {

	assert(this->Num() > 0);

	type obj = Top();
	this->SetNum(this->Num()-1, false);
	return obj;
}

template< class type >
ID_INLINE void hhStack<type>::Push(type &object) {
	this->Append(object);
}

template< class type >
ID_INLINE bool hhStack<type>::Empty(void) {
	return this->Num() == 0;
}

template< class type >
ID_INLINE void hhStack<type>::Clear(void) {
	idList<type>::Clear();
}

#endif
