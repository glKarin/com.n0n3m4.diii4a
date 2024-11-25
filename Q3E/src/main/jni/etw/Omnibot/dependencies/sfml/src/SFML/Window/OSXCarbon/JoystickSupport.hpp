////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007 Brad Leffler (brad.leffler@gmail.com) and Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

#ifndef SFML_JOYSTICKSUPPORTOSX_HPP
#define SFML_JOYSTICKSUPPORTOSX_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/OSXCarbon/Joystick.hpp>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <mach/mach_port.h>
#include <string>
#include <vector>


namespace sf
{
namespace priv
{

////////////////////////////////////////////////////////////
/// MacOS X implementation of JoystickSupport 
/// Give access to joystick related OS-specific functions
////////////////////////////////////////////////////////////
class JoystickSupport
{

public :
    ////////////////////////////////////////////////////////////
    /// Get all attached Joystick/Gamepad devices
    ///
    /// \return Error state of device retrieval
    ///
    ////////////////////////////////////////////////////////////
    int EnumerateDevices(std::vector<JoystickDevice> &devices);

    ////////////////////////////////////////////////////////////
    /// Enumerate all elements of a joystick/gamepad
    ///
    /// \return Error state of element enumeration
    ///
    ////////////////////////////////////////////////////////////
    static bool EnumerateElements(CFDictionaryRef dictionary, JoystickDevice *joystick);
	
    ////////////////////////////////////////////////////////////
    /// Callback for joystick elements
    ////////////////////////////////////////////////////////////
    static void ElementArrayCallback(const void *Object, void *Value);
	
    ////////////////////////////////////////////////////////////
    /// Determine the type and viability of a joystick element
    ////////////////////////////////////////////////////////////
    static void ValidateElement(CFDictionaryRef dictionary, JoystickDevice *joystick);
	
    ////////////////////////////////////////////////////////////
    /// Get an interface to the joystick
    ///
    /// \return Error state of device interface
    ///
    ////////////////////////////////////////////////////////////
	int GetDeviceInterface(io_object_t *hidDevice, JoystickDevice &joystick);
		
    ////////////////////////////////////////////////////////////
    /// Release all joysticks
    ////////////////////////////////////////////////////////////
	void ReleaseDevices(std::vector<JoystickDevice> &joysticks);

private:

	mach_port_t		myMasterPort;
};

} // namespace priv

} // namespace sf

#endif // SFML_JOYSTICKSUPPORTOSX_HPP
