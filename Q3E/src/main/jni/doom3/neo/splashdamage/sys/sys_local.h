// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __SYS_LOCAL__
#define __SYS_LOCAL__

#include "../framework/KeyInput.h"
#include "sys_input.h"
#include "sys_render.h"

#define SE_KEY_VALUE( key, scanCode ) ( ( (scanCode) & 0x000000FF ) | ( ( (key) << 8 ) & 0x0000FF00 ) )
#define SE_KEY_VALUE2( isDown, isRepeat ) ( \
	( (isDown) ? 0x1 : 0x0 ) | \
	( (isRepeat) ? 0x2 : 0x0 ) \
	)

class sdSysEvent {
public:
								sdSysEvent() :
									ptrLength( 0 ),
									ptr( NULL ) {
								}
								~sdSysEvent( void );

	void						Init( sysEventType_t _type, int _value, int _value2, int _ptrLength, void* _ptr );

	void						FreeData( void );

	bool						IsKeyEvent( void ) const { return type == SE_KEY; }
	bool						IsCharEvent( void ) const { return type == SE_CHAR; }
	bool						IsRealMouseEvent( void ) const { return type == SE_MOUSE; }
	bool						IsMouseEvent( void ) const { return type == SE_MOUSE || type == SE_CONTROLLER_MOUSE; }
	bool						IsControllerMouseEvent( void ) const { return type == SE_CONTROLLER_MOUSE; }
	bool						IsMouseButtonEvent( void ) const { return type == SE_MOUSE_BUTTON; }
	bool						IsConsoleEvent( void ) const { return type == SE_CONSOLE; }
	bool						IsControllerButtonEvent( void ) const { return type == SE_CONTROLLER_BUTTON; }
	bool						IsGuiEvent( void ) const { return type == SE_GUI; }
	bool						IsIMEEvent( void ) const { return type == SE_IME; }

	float						GetXCoord( void ) const { return static_cast< float >( value ); }
	float						GetYCoord( void ) const { return static_cast< float >( value2 ); }

	const char*					GetCommand( void ) const { return reinterpret_cast< const char* >( ptr ); }

	bool						IsKeyDown( void ) const { return ( value2 & 0x1 ) != 0; }
#if defined( MACOS_X ) || defined( __linux__ )
	unsigned int				GetScanCode( void ) const { return static_cast< unsigned int >( value ); }
	keyNum_e					GetKey( void ) const { return static_cast< keyNum_e >( value ); }
#else
	unsigned int				GetScanCode( void ) const { return static_cast< unsigned int >( value & 0xFF ); }
	keyNum_e					GetKey( void ) const { return static_cast< keyNum_e >( ( value & 0xFF00 ) >> 8 ); }
#endif
	wchar_t						GetChar( void ) const { return value2; }

	bool						IsKeyRepeat( void ) const { return ( value2 & 0x2 ) != 0; }

	mouseButton_t				GetMouseButton() const { return static_cast< mouseButton_t >( value ); }

	int							GetGuiAction( void ) const { return value; }

	bool						IsButtonDown( void ) const { return ( value2 & 0x1 ) != 0; }
	int							GetControllerHash( void ) const { return value2 >> 1; }
	int							GetButton( void ) const { return value; }

	// SE_IME
	imeEvent_t					GetIMEEvent() const { return static_cast< imeEvent_t >( value ); }
	const wchar_t*				GetCompositionString() const { return reinterpret_cast< const wchar_t* >( ptr ); }

	idLinkList< sdSysEvent >&	GetNode( void ) { return node; }

	void						Save( idFile* file ) {
									file->WriteInt( type );
									file->WriteInt( value );
									file->WriteInt( value2 );
									file->WriteInt( ptrLength );
									if ( ptrLength ) {
										assert( ptr );
										file->Write( ptr, ptrLength );
									}
								}
	void						Restore( idFile* file ) {
									file->ReadInt( (int&)type );
									file->ReadInt( value );
									file->ReadInt( value2 );
									file->ReadInt( ptrLength );

									FreeData();
									if ( ptrLength ) {
										ptr = Mem_Alloc( ptrLength );
										file->Read( ptr, ptrLength );
									}
								}

	sdSysEvent&					operator=( const sdSysEvent& rhs ) {
									type = rhs.type;
									value = rhs.value;
									value2 = rhs.value2;
									ptrLength = rhs.ptrLength;

									FreeData();
									if ( rhs.ptr ) {
										ptr = Mem_Alloc( ptrLength );
										::memcpy( ptr, rhs.ptr, rhs.ptrLength );
									}

									return *this;
								}

protected:
	sysEventType_t				type;
	int							value;
	int							value2;

	int							ptrLength;			// bytes of data pointed to by evPtr, for journaling
	void*						ptr;				// this must be manually freed if not NULL

	idLinkList< sdSysEvent >	node;
};

/*
==============================================================
sdPerformanceQueryLocal

	Provides some common implementations that may be interesting for all os-es
==============================================================
*/
class sdPerformanceQueryLocal : public sdPerformanceQuery { 
private:
	int capacity;				//Maximum number of performance history values
	int head;					//Index of last value in circular buffer
	float minv;					//Minimum value encountered (even if pushed out of circular buffer)
	float maxv;					//Maximum value encountered
	idList<float> values; 

protected:	
	void Insert( float f )    {
		minv = minv > f ? f : minv;
		maxv = maxv < f ? f : maxv;

		if ( values.Num() < capacity ) {
			values.Append( f );
		} else {
			values[ head ] = f;
		}
		head++;

		//Wrap around
		if ( head == capacity ) {
			head = 0;
		}
	}

public:
	sdPerformanceQueryLocal ( int _capacity = 25 ) : capacity(_capacity), minv(0), maxv(0), head(0) {}
	virtual ~sdPerformanceQueryLocal () {}

	virtual int GetCapacity( void ) const { return capacity; }
	virtual void SetCapacity( int capacity ) { this->capacity = capacity; }
	virtual int GetSize( void ) const { return values.Num(); }
	virtual float GetMin( void ) const { return minv; }
	virtual float GetMax( void ) const { return maxv; }

	/*
	Index 0 is always the most recent one, higher indexes are progressively older
	*/
	virtual float GetSample( int i ) const {
		i = (values.Num()-1) - i;
		if ( values.Num() == capacity ) {
			return values[ (head+i) % capacity ];
		} else {
			return values[i];
		}
	}

	/*
	Call this every frame or whatever, to add a new sample to the front of the list
	Should return false if sampling failed
	*/
	virtual bool Sample( void ) = 0;
};


//===============================================================
//
//	sdControllerManagerLocal
//
//===============================================================

class sdControllerManagerLocal : public sdControllerManager {
public:
											sdControllerManagerLocal();
	virtual									~sdControllerManagerLocal();

	virtual void							Init();
	virtual void							Shutdown();

	sdControllerAPI::controllerApiState_e	GetAPIState( const char* APIName );

	virtual int								GetNumConnectedControllers() const;

private:
	idList< sdControllerAPI* >				controllerAPIs;
};

/*
==============================================================

	idSysLocal

==============================================================
*/

class idSysLocal : public idSys {
public:
							~idSysLocal( void );

	virtual void			Init( void );
	virtual void			PostGameInit( void );
	virtual void			Shutdown( void );

	virtual void			DebugPrintf( const char *fmt, ... );
	virtual void			DebugVPrintf( const char *fmt, va_list arg );

	virtual void			GetCPUInfo( cpuInfo_t& info );
	virtual double			GetClockTicks( void );
	virtual double			ClockTicksPerSecond( void );
	virtual void			Sleep( int msec );
	virtual int				Milliseconds();
	virtual time_t			RealTime( sysTime_t* sysTime );
	const char*				TimeToSystemStr( const sysTime_t& sysTime );
	virtual const char*		TimeAndDateToSystemStr( const sysTime_t& sysTime );
	virtual time_t			TimeDiff( const sysTime_t& from, const sysTime_t& to );
	virtual void			SecondsToTime( const time_t t, sysTime_t& out, bool localTime = false );
	virtual const char *	TimeToStr( const sysTime_t& t );
	virtual const char *	SecondsToStr( const time_t t, bool localTime = false );
	virtual cpuid_t			GetProcessorId( void );
	virtual const char *	GetProcessorString( void );
	virtual const char *	FPU_GetState( void );
	virtual bool			FPU_StackIsEmpty( void );
	virtual void			FPU_SetFTZ( bool enable );
	virtual void			FPU_SetDAZ( bool enable );

	virtual void			FPU_EnableExceptions( int exceptions );

	virtual void			GetCurCallStack( address_t *callStack, const int callStackSize );
	virtual const char *	GetCurCallStackStr( int depth );
	virtual const char *	GetCallStackStr( const address_t *callStack, const int callStackSize );
	virtual const char *	GetFunctionName( const address_t function );
	virtual const char *	GetFunctionSourceFile( const address_t function );
	virtual void			ShutdownSymbols( void );

	virtual void			GetCurrentMemoryStatus( sysMemoryStats_t &stats );
	virtual void			GetExeLaunchMemoryStatus( sysMemoryStats_t &stats );
	virtual void			GetProcessMemoryStatus( sysProcessMemoryStats_t &stats );


	virtual void*			DLL_Load( const char *dllName, bool checkFullPathMatch );
	virtual void *			DLL_GetProcAddress( void* dllHandle, const char *procName );
	virtual void			DLL_Unload( void* dllHandle );
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, int maxLength );

	virtual const char *	EXEPath( void );

	virtual long			File_TimeStamp( FILE* f );
	virtual int				File_Stat( const char* OSPath );

	virtual const sdSysEvent*	GenerateBlankEvent( void );
	virtual const sdSysEvent*	GenerateCharEvent( int ch );
	virtual const sdSysEvent*	GenerateKeyEvent( keyNum_t key, bool down );
	virtual const sdSysEvent*	GenerateMouseButtonEvent( int button, bool down );
	virtual const sdSysEvent*	GenerateMouseMoveEvent( int deltax, int deltay );
	virtual const sdSysEvent*	GenerateGuiEvent( int value );
	virtual const sdSysEvent*	GetEvent( void );
	virtual void				QueEvent( sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
	virtual void				ClearEvents( void );

	virtual void				FreeEvent( const sdSysEvent* event );

	virtual void			OpenURL( const char *url, bool quit );
	virtual void			StartProcess( const char *exeName, bool quit );

	virtual int				MessageBox( const char* title, const char* buffer, messageBoxType_t type );
	virtual void			ProcessOSEvents();

	virtual sdPerformanceQuery* GetPerformanceQuery( sdPerformanceQueryType pqType );
	virtual void			 CollectPerformanceData( void );

	virtual idWStr			GetClipboardData( void );
	virtual void			SetClipboardData( const wchar_t *string );

	virtual void			SetServerInfo( const char* key, const char* value );
	virtual void			FlushServerInfo( void );

	virtual void			InitInput();
	virtual void			ShutdownInput();

	virtual idKeyboard&		Keyboard();
	virtual idMouse&		Mouse();

	virtual sdIME&			IME();

	virtual void			SetSystemLocale();

	virtual void			SetDefaultLocale();

	virtual sdControllerManager&	GetControllerManager() { return controllerManager; }
	virtual sdLogitechLCDSystem*	GetLCDSystem( void );

	virtual const char *	NetAdrToString( const netadr_t& a ) const;
	virtual bool			IsLANAddress( const netadr_t& a ) const;
	virtual bool			StringToNetAdr( const char *s, netadr_t *a, bool doDNSResolve ) const;

	virtual int				GetGUID( unsigned char* guid, const int len ) const;

private:
	idLinkList< sdSysEvent > eventQue;
	idBlockAlloc< sdSysEvent, 128 > eventAllocator;
	
	sdControllerManagerLocal	controllerManager;
};

#endif /* !__SYS_LOCAL__ */
