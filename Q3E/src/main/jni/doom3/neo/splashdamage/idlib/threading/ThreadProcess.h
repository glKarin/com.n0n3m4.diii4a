// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __THREADPROCESS_H__
#define __THREADPROCESS_H__

class sdThreadProcess {
public:
							sdThreadProcess() :
								terminate( false ) {
							}

	virtual					~sdThreadProcess() {}

	virtual void			Start() { terminate = false; }
	virtual unsigned int	Run( void* parm ) = 0;
	virtual void			Stop() { terminate = true; }

	bool					Terminating() const { return terminate; }

protected:
	bool					terminate;
};

template< class T > class sdThreadProcessFunctor : public sdThreadProcess {
public:
	typedef void( T::*startFunc_t )();
	typedef unsigned int( T::*runFunc_t )( void* parm );
	typedef void( T::*stopFunc_t )();

	void					Init( T* processClass, startFunc_t startFunc, runFunc_t runFunc, stopFunc_t stopFunc ) {
								this->processClass = processClass;
								this->startFunc = startFunc;
								this->runFunc = runFunc;
								this->stopFunc = stopFunc;
							}

	virtual void			Start() {
								sdThreadProcess::Start();
                                (*processClass.*startFunc)();								
							}
	virtual unsigned int	Run( void* parm ) { return (*processClass.*runFunc)( parm ); }
	virtual void			Stop() {
								sdThreadProcess::Stop();
                                (*processClass.*stopFunc)();								
							}

protected:
	T*						processClass;
	startFunc_t				startFunc;
	runFunc_t				runFunc;
	stopFunc_t				stopFunc;

};

#endif /* !__THREADPROCESS_H__ */
