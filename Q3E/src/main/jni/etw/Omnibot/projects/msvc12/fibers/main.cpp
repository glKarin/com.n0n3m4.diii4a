#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>

//////////////////////////////////////////////////////////////////////////

class Fiber
{
public:	
	friend class FiberManager;

	static void __stdcall FiberWork( void * data )
	{
		Fiber * thisFiber = static_cast<Fiber*>( data );
		// run the fiber

		// free the fiber
		DeleteFiber( thisFiber->fiber );
		thisFiber->fiber = NULL;
	}

	bool IsRunning() const { return fiber != NULL; }

	void Execute() {
		SwitchToFiber( fiber );
	}

	Fiber() : nextFiber( NULL ) {
		fiber = CreateFiber( 0, FiberWork, this );
	}
protected:
	Fiber *			nextFiber;
private:
	void *			fiber;	
};

//////////////////////////////////////////////////////////////////////////

class FiberManager
{
public:
	void Init()
	{
		primaryFiber = ConvertThreadToFiber( NULL );
	}
	void Shutdown()
	{
		ConvertFiberToThread();
	}
	void UpdateFibers()
	{
		Fiber * lastFiber = NULL;
		Fiber * currentFiber = firstFiber;
		while ( currentFiber ) {
			currentFiber->Execute();
			// delete it if it's done
			if ( !currentFiber->IsRunning() ) {
				if ( currentFiber == firstFiber ) {
					firstFiber = currentFiber->nextFiber;
				} else {
					assert( lastFiber );
					lastFiber->nextFiber = currentFiber->nextFiber;
				}
				delete currentFiber;
			}
			currentFiber = currentFiber->nextFiber;
		}
	}
	bool HasActiveFibers() const {
		return firstFiber != NULL;
	}
	/*void wait()
	{
		SwitchToFiber( primaryFiber );
	}
	void println(const char* _msg, ...)
	{
		static char buffer[8192] = {0};
		va_list list;
		va_start(list, _msg);
#ifdef WIN32
		_vsnprintf(buffer, 8192, _msg, list);	
#else
		vsnprintf(buffer, 8192, _msg, list);
#endif
		va_end(list);
		std::cout << buffer << std::endl;
	}*/

	FiberManager() : primaryFiber( NULL ) {
	}
private:
	void * primaryFiber;

	Fiber * firstFiber;
};

//////////////////////////////////////////////////////////////////////////

//class MoveTo : public Fiber
//{
//public:
//	virtual void operator()( FiberManager * sys )
//	{
//		for ( int i = 0; i < 10; ++i ) {
//			sys->println( "moveto: %d %d %d", params[0],params[1]+i,params[2]+i );
//			sys->wait();
//		}
//	}	
//
//	MoveTo( int a = 0, int b = 0, int c = 0 )
//	{
//		params[0] = a;
//		params[1] = b;
//		params[2] = c;
//	}
//private:
//	int params[3];
//};

//////////////////////////////////////////////////////////////////////////

int main(int argc,const char **argv)
{
	FiberManager fiberMan;

	fiberMan.Init();

	/*enum { NumFibers = 8 };
	for ( int i = 0; i < NumFibers; ++i ) {
		fiberMan.AllocFiber( new MoveTo( i, i * 10, i * 1000 ) );
	}*/
	
	while( fiberMan.HasActiveFibers() ) {
		fiberMan.UpdateFibers();
	}

	fiberMan.Shutdown();

	return 0;
}
