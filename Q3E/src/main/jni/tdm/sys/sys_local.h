/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __SYS_LOCAL__
#define __SYS_LOCAL__

/*
==============================================================

	idSysLocal

==============================================================
*/

class idSysLocal : public idSys {
public:
	virtual void			DebugPrintf( const char *fmt, ... ) override id_attribute((format(printf,2,3)));
	virtual void			DebugVPrintf( const char *fmt, va_list arg ) override;

	virtual double			GetClockTicks( void ) override;
	virtual double			ClockTicksPerSecond( void ) override;
	virtual cpuid_t			GetProcessorId( void ) override;
	virtual const char *	GetProcessorString( void ) override;

	virtual void			FPU_SetFTZ( bool enable ) override;
	virtual void			FPU_SetDAZ( bool enable ) override;
	virtual void			FPU_SetExceptions(bool enable) override;

	virtual void			ThreadStartup() override;
	virtual void			ThreadHeartbeat( const char *threadName ) override;

	virtual bool			LockMemory( void *ptr, int bytes ) override;
	virtual bool			UnlockMemory( void *ptr, int bytes ) override;

    virtual uintptr_t		DLL_Load(const char *dllName) override;
    virtual void *			DLL_GetProcAddress(uintptr_t dllHandle, const char *procName) override;
    virtual void			DLL_Unload(uintptr_t dllHandle) override;
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) override;

	virtual sysEvent_t		GenerateMouseButtonEvent( int button, bool down ) override;
	virtual sysEvent_t		GenerateMouseMoveEvent( int deltax, int deltay ) override;

	virtual void			OpenURL( const char *url, bool quit ) override;
	virtual void			StartProcess( const char *exeName, bool quit ) override;
};

#endif /* !__SYS_LOCAL__ */
