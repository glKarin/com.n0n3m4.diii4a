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

#ifndef __AI_LIBRARY_H__
#define __AI_LIBRARY_H__

#include <map>
#include <string>
#include <functional>

namespace ai
{

/**
 * greebo: This Singleton Library class maps element names to
 *         static Element::CreateInstance() member functions. All elements
 *         register themselves with this library and provide a 
 *         callback to get an instance of themselves.
 *
 * Why am I doing this? During InitFromSavegame, the Subsystems will have
 * to re-allocate an instance of the elements they were associated with. To avoid
 * implementing a complicated homegrown typesystem, this Library is used.
 *
 * Use the CreateInstance() method to acquire a new instance of a named element.
 */
template <class Element>
class Library
{
public:
	// The shared_ptr type of a registered Element
	typedef std::shared_ptr<Element> ElementPtr;

	// Define the function type to Create an Element Instance
	typedef std::function<ElementPtr()> CreateInstanceFunc;

private:
	// This is the map associating task names with CreateInstance() methods
	// greebo: Use std::string as index, idStr doesn't work!
	typedef std::map<std::string, CreateInstanceFunc> ElementMap;

	ElementMap _elements;

	// Private constructor
	Library() 
	{}

	//noncopyable
	Library(const Library&);
	Library& operator=(const Library&);

public:
	
	/**
	 * greebo: Tries to lookup the element name in the ElementMap
	 *         and instantiates a Element of this type.
	 */
	ElementPtr CreateInstance(const std::string& elementName)
	{
		ElementPtr returnValue; // NULL by default

		// Try to lookup the task in the map
		typename ElementMap::iterator i = _elements.find(elementName);

		if (i != _elements.end())
		{
			CreateInstanceFunc& createInstance(i->second);

			// Invoke the std::function to gather the TaskPtr
			returnValue = createInstance();
		}

		// Check if the task could be found
		if (returnValue == NULL)
		{
			gameLocal.Error("Library: Cannot allocate Element instance for name %s", elementName.c_str());
		}

		return returnValue;
	}

	/**
	 * greebo: Each Task has to register itself here.
	 *         This must happen before any client may call
	 *         CreateTask, so place this call right below 
	 *         the Task declaration.
	 */
	void RegisterElement(const std::string& elementName, const CreateInstanceFunc& func)
	{
		// Insert this task into the map
		_elements.insert(
			typename ElementMap::value_type(elementName, func)
		);
	}

	// Accessor method for the singleton instance
	static Library& Instance()
	{
		static Library _instance; // The actual instance
		return _instance;
	}

// ----------------------------------------------------------------------------

	// Helper functor which registers the given element in its constructor
	class Registrar 
	{
	public:
		Registrar(const std::string& name, const CreateInstanceFunc& func)
		{
			// Pass the call
			Library<Element>::Instance().RegisterElement(name, func);
		}
	};
};

// Shortcut typedefs
class Task;
typedef Library<Task> TaskLibrary;

class State;
typedef Library<State> StateLibrary;

} // namespace ai

#endif /* __AI_LIBRARY_H__ */
