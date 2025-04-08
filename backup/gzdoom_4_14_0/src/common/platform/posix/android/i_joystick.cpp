/*
** i_joystick.cpp
**
**---------------------------------------------------------------------------
** Copyright 2005-2016 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "basics.h"
#include "cmdlib.h"

#include "m_joy.h"
#include "keydef.h"

#define DEFAULT_DEADZONE 0.25f;

// Very small deadzone so that floating point magic doesn't happen
#define MIN_DEADZONE 0.000001f

class SDLInputJoystick: public IJoystickConfig
{
public:
	SDLInputJoystick(int DeviceIndex)
	{
	}

	bool IsValid() const
	{
		return false;
	}

	FString GetName()
	{
		return {};
	}
	float GetSensitivity()
	{
		return 1.0f;
	}
	void SetSensitivity(float scale)
	{
	}

	int GetNumAxes()
	{
		return 0;
	}
	float GetAxisDeadZone(int axis)
	{
		return 0.0;
	}
	EJoyAxis GetAxisMap(int axis)
	{
		return JOYAXIS_None;
	}
	const char *GetAxisName(int axis)
	{
		return "";
	}
	float GetAxisScale(int axis)
	{
		return 0.0;
	}

	void SetAxisDeadZone(int axis, float zone)
	{
	}
	void SetAxisMap(int axis, EJoyAxis gameaxis)
	{
	}
	void SetAxisScale(int axis, float scale)
	{
	}

	// Used by the saver to not save properties that are at their defaults.
	bool IsSensitivityDefault()
	{
		return true;
	}
	bool IsAxisDeadZoneDefault(int axis)
	{
		return false;
	}
	bool IsAxisMapDefault(int axis)
	{
		return false;
	}
	bool IsAxisScaleDefault(int axis)
	{
		return false;
	}

	void SetDefaultConfig()
	{
	}

	bool GetEnabled()
	{
		return false;
	}
	
	void SetEnabled(bool enabled)
	{
	}

	FString GetIdentifier()
	{
		return {};
	}

	void AddAxes(float axes[NUM_JOYAXIS])
	{
	}

	void ProcessInput()
	{
	}

	friend class SDLInputJoystickManager;
};

class SDLInputJoystickManager
{
public:
	SDLInputJoystickManager()
	{
	}

	void AddAxes(float axes[NUM_JOYAXIS])
	{
	}
	void GetDevices(TArray<IJoystickConfig *> &sticks)
	{
	}

	void ProcessInput() const
	{
	}
};
static SDLInputJoystickManager *JoystickManager;

void I_StartupJoysticks()
{
}
void I_ShutdownInput()
{
}

void I_GetJoysticks(TArray<IJoystickConfig *> &sticks)
{
}

void I_GetAxes(float axes[NUM_JOYAXIS])
{
}

void I_ProcessJoysticks()
{
}

IJoystickConfig *I_UpdateDeviceList()
{
	return NULL;
}
