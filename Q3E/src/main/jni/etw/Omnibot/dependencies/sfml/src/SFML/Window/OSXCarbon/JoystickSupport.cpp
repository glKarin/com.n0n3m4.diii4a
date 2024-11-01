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

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/Event.hpp>
#include "JoystickSupport.hpp"
#include <iostream>

namespace sf
{
namespace priv
{


////////////////////////////////////////////////////////////
/// Enumerate all attached joystick/gamepad controllers
////////////////////////////////////////////////////////////
int JoystickSupport::EnumerateDevices(std::vector<JoystickDevice> &devices)
{
	IOReturn ioRes = IOMasterPort(bootstrap_port, &myMasterPort);
	
	if (ioRes != kIOReturnSuccess)
		return 0;

	CFMutableDictionaryRef	hidMatchDictionary	= 0;
	io_iterator_t			hidObjectIterator	= 0;

	hidMatchDictionary = IOServiceMatching(kIOHIDDeviceKey);
	
	ioRes = IOServiceGetMatchingServices( myMasterPort, hidMatchDictionary, &hidObjectIterator );
				
	if ((ioRes != kIOReturnSuccess) || (hidObjectIterator == nil))
	{
		return 0;
	}

	io_object_t 			hidDevice;
	kern_return_t 			result;
	CFMutableDictionaryRef 	properties;
	CFTypeRef 				object;
	long 					usagePage, usage, locationID;

	while ((hidDevice = IOIteratorNext(hidObjectIterator)))
	{
		result = IORegistryEntryCreateCFProperties(hidDevice, &properties, kCFAllocatorDefault, kNilOptions);

		if ((result != KERN_SUCCESS) || (properties == nil))
			continue;
				
		object = CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsagePageKey));
		if (object)
		{
			if (!CFNumberGetValue((CFNumberRef)object, kCFNumberLongType, &usagePage))
				continue;

			object = CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsageKey));
	
			if (object)
			{
				if(!CFNumberGetValue((CFNumberRef)object, kCFNumberLongType, &usage))
					continue;
			}
		}
					
		object = CFDictionaryGetValue(properties, CFSTR(kIOHIDLocationIDKey));
		if (!object)
			locationID = 0;		
		else
		{
			if(!CFNumberGetValue((CFNumberRef)object, kCFNumberLongType, &locationID))
				locationID = 0;
		}
	
	    if (usagePage == kHIDPage_GenericDesktop)
		{
			switch(usage)
			{
				case kHIDUsage_GD_Joystick :
				case kHIDUsage_GD_GamePad :
				{
					JoystickDevice	Joystick;
			
					EnumerateElements(properties, &Joystick);

					if (GetDeviceInterface(&hidDevice, Joystick) == kIOReturnSuccess)
					{
						devices.push_back(Joystick);
						std::cout << "Device with " << Joystick.axis.size() << " axes and " << Joystick.buttons.size() << " buttons was found." << std::endl;
					}
				}
				break;
			}
		}
		
		CFRelease(properties);

		ioRes = IOObjectRelease(hidDevice);

		if(ioRes != kIOReturnSuccess)
			std::cout << "Error releasing device" << std::endl;			
	}
			
	IOObjectRelease(hidObjectIterator);
	
	return 1;
}


////////////////////////////////////////////////////////////
/// Enumerate all elements of a joystick/gamepad controller
////////////////////////////////////////////////////////////
bool JoystickSupport::EnumerateElements(CFDictionaryRef dictionary, JoystickDevice *joystick)
{
	CFTypeRef 	Object;
	CFTypeID 	Type;

	Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementKey));

	if (Object)	
	{
		Type = CFGetTypeID(Object);

		if (Type == CFArrayGetTypeID())
		{
			CFRange Range;
			Range.location = 0;
			Range.length = CFArrayGetCount((CFArrayRef)Object);

			CFArrayApplyFunction((CFArrayRef)Object, Range, (CFArrayApplierFunction)JoystickSupport::ElementArrayCallback, (void*)joystick);
		}
		else if (Type == CFDictionaryGetTypeID())
		{
			ValidateElement((CFDictionaryRef)Object, joystick);
		}
	}
	
	return 1;
}


////////////////////////////////////////////////////////////
/// Enumerate all elements of a joystick/gamepad controller
////////////////////////////////////////////////////////////
void JoystickSupport::ElementArrayCallback(const void *Object, void *Value)
{
	if (CFGetTypeID(Object) != CFDictionaryGetTypeID())
		return;

	ValidateElement((CFDictionaryRef)Object, (JoystickDevice*)Value);
}


////////////////////////////////////////////////////////////
/// Classify the type and parameters of a controller element
////////////////////////////////////////////////////////////
void JoystickSupport::ValidateElement(CFDictionaryRef dictionary, JoystickDevice *joystick)
{	
	CFTypeRef	Object;
	
	Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementTypeKey));
	
	if (Object)
	{
		JoystickElement JoyElement;
	
		CFNumberGetValue((CFNumberRef)Object, kCFNumberLongType, &(JoyElement.elementType));

		if (JoyElement.elementType == kIOHIDElementTypeInput_Misc || JoyElement.elementType == kIOHIDElementTypeInput_Button || JoyElement.elementType == kIOHIDElementTypeInput_Axis)
		{
			Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementCookieKey));	
			CFNumberGetValue((CFNumberRef)Object, kCFNumberLongType, &(JoyElement.cookie));
			Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementUsagePageKey));	
			CFNumberGetValue((CFNumberRef)Object, kCFNumberLongType, &(JoyElement.usagePage));
			Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementUsageKey));	
			CFNumberGetValue((CFNumberRef)Object, kCFNumberLongType, &(JoyElement.usage));
			Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementMinKey));
			CFNumberGetValue((CFNumberRef)Object, kCFNumberLongType, &(JoyElement.min));
			Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementMaxKey));
			CFNumberGetValue((CFNumberRef)Object, kCFNumberLongType, &(JoyElement.max));
			Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementScaledMinKey));
			CFNumberGetValue((CFNumberRef)Object, kCFNumberLongType, &(JoyElement.scaledMin));
			Object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementScaledMaxKey));
			CFNumberGetValue((CFNumberRef)Object, kCFNumberLongType, &(JoyElement.scaledMax));

			switch (JoyElement.usage)
			{
				case kHIDUsage_GD_X :
				case kHIDUsage_GD_Rx :
				case kHIDUsage_GD_Y :
				case kHIDUsage_GD_Ry :
				case kHIDUsage_GD_Z :
				case kHIDUsage_GD_Rz :
				{
					joystick->axis.push_back(JoyElement);
				}
				break;

				default :
				{
					if (JoyElement.usagePage != kHIDPage_Button)
						break;

					joystick->buttons.push_back(JoyElement);
				}
				break;
			}
		}
	}
			
	EnumerateElements(dictionary, joystick); 
}


////////////////////////////////////////////////////////////
/// Store the interface to the joystick/gamepad for future usage
////////////////////////////////////////////////////////////
int JoystickSupport::GetDeviceInterface(io_object_t *hidDevice, JoystickDevice &joystick)
{
	IOCFPlugInInterface 	**plugInInterface;
	HRESULT 				plugInResult;
	SInt32 					score = 0;
	IOReturn 				ioRes;

	if (IOCreatePlugInInterfaceForService(*hidDevice, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score) == kIOReturnSuccess)
	{
		plugInResult = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID), (LPVOID*)&(joystick.hidDeviceInterface));
			
		if( plugInResult == S_OK )
		{						
			(*plugInInterface)->Release(plugInInterface);

			ioRes = (*(joystick.hidDeviceInterface))->open(joystick.hidDeviceInterface, 0);
		}
	}
	
	return ioRes;
}

////////////////////////////////////////////////////////////
/// Release all joystick/gamepad devices
////////////////////////////////////////////////////////////
void JoystickSupport::ReleaseDevices(std::vector<JoystickDevice> &joysticks)
{
	std::vector<JoystickDevice>::iterator end = joysticks.end();
	std::vector<JoystickDevice>::iterator iter = joysticks.begin();
	
	for( ; iter != end; ++iter )
	{
		IOHIDDeviceInterface **interface = (*iter).hidDeviceInterface;
		
		(*interface)->close( interface );
		(*interface)->Release( interface );
	}
			
	joysticks.clear();
			
	if( myMasterPort )
		mach_port_deallocate( mach_task_self(), myMasterPort );	
}


} // namespace priv

} // namespace sf
