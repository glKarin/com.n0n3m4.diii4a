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

#ifndef __STATEQUEUE_H__
#define __STATEQUEUE_H__

#include "../DarkModGlobals.h"

#include <list>
#include <sstream>

#include "Library.h"

namespace ai 
{

/**
 * greebo: Templated queue containing shared ptrs of a certain type (Tasks and States).
 *
 * This is not a priority queue, no sorting happens. It derives from the std::list 
 * class, as this implements most of the methods we need for this queue.
 *
 * The interface is extended by a few convenience methods.
 *
 * The Save/Restore methods work together with the templated Library<> class.
 */
template <class Element>
class Queue :
	public std::list< std::shared_ptr<Element> >
{
	// Parent list type
	typedef std::list<std::shared_ptr<Element> > ListType;
		
	// greebo: Don't define data members in a class deriving from an STL container
	// (std::list destructor is non-virtual)

	// Shortcut typedef
	typedef std::shared_ptr<Element> ElementPtr;

public:
	/**
	* Returns the entire contents of this queue as a string, for debugging purposes.
	* Relies on the Elements having a member method GetName()
	*/
	const std::string DebuggingInfo() const
	{
		std::stringstream debugInfo;
		for (typename ListType::const_iterator i = ListType::begin(); 
			 i != ListType::end(); 
			 ++i)
		{
			debugInfo << (*i)->GetName();
			debugInfo << " \n";
		}
		return debugInfo.str();
	}
	
	/**
	* Save this data structure to a savefile
	*/
	void Save(idSaveGame *savefile) const
	{
        savefile->WriteInt(static_cast<int>(ListType::size()));
		for (typename ListType::const_iterator i = ListType::begin(); 
			 i != ListType::end(); 
			 ++i)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Saving element %s.\r", (*i)->GetName().c_str());
			savefile->WriteString((*i)->GetName().c_str());

			// Write the Element data into the savefile
			(*i)->Save(savefile);
		}
	}

	/**
	* Restore this data structure from a savefile
	*/
	void Restore(idRestoreGame *savefile)
	{
		// Clear the queue before restoring
		ListType::clear();
		
		int elements;
		savefile->ReadInt(elements);
		for (int i = 0; i < elements; i++)
		{
			idStr str;
			savefile->ReadString(str);
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Restoring task %s.\r", str.c_str());

			ElementPtr element = Library<Element>::Instance().CreateInstance(str.c_str());
			assert(element != NULL); // the element must be found

			// Restore the element's state
			element->Restore(savefile);

			// Add the element to the queue
			this->push_back(element);
		}
	}
};

// Convenience typedefs
class State;
typedef Queue<State> StateQueue;

class Task;
typedef Queue<Task> TaskQueue;

} // namespace ai

#endif /* !__STATEQUEUE_H__ */
