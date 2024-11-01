/**********************************************************
GameMonkey C++ Class Binding Template

Version 0.9.5a

A C++ class template is designed to ease the process of binding
C++ classes to the Game Monkey scripting environment.

This template uses the "Curiously Recursive Template Pattern",
which allows static polymorphism in C++ and provides the ability
for the user to override any of the functions exposed by the template.

By using this method, users can provide their own custom constructors,
destructors and property access methods.


*********************************************************

Version History:

18/04/2006
- Fixed operators assuming native type was [0] - DrEvil
- Fixed some calls to _allocObject improperly setting the objects to native - DrEvil

30/01/2006 Version 0.9.5a OJW
- Bug fix in SetObjectOwnershipNative and SetObjectOwnershipGM methods

09/12/2005 Version 0.9.5 OJW
- Updated various Op functions not to Assert if not defined
- Added new CreateObject method to create a local object that isn't assigned to the global table
- Added original Constructor() and allowed gmBind to select which one to use based on param count
- Added new CreateGmUserObject method to allow you to retreive the gmUserObject back in preference to the native ptr
- Added new Get/SetProperty functions to work with the gmUserObject holding the gmBindUserObject ptr
- Added new Get/SetProperty functions to allow getting/setting of gmUserObject types
- Added SetGlobalObject() to set a gmBind variable in the Global Table
- Added SetObjectOwnershipNative / SetObjectOwnershipGM to reduce confusion about who owns the object
- Added RenameGlobalObject helper
- Added gmBindBase baseclass for C++ objects; allows you to automatically 'null' global vars when object is destructed in C++

24/05/2005 Version 0.9.4e (Updated by DrEvil)
- Added ~16 more operator overload capability - DrEvil
- Added overloadable AsString callback - DrEvil
- Added SetObject helper - DrEvil
- Added PushObject helper - DrEvil
- Added GetThisTable helper - DrEvil
- Added GetPropertyTable & SetPropertyTable helper - DrEvil
- Added 2 GetUserBoundObject overloads - DrEvil
- Added GetTypeName accessor, primarily for the AsString callback - DrEvil

24/05/2005 Version 0.9.4d
- Added a variable parameter constructor as requested by Toolmaker

20/04/2005 Version 0.9.4c
- Added GetPropertyFunction, SetPropertyFunction methods to allow a user to get/set
function properties
- Added GetUserBoundObject to return the gmBindUserObject for an object (USE WITH CAUTION)

18/04/2005 Version 0.9.4b
- Minor changes to fix bugs under VC++ 2003 (Thanks, ToolMaker)

08/04/2005 Version 0.9.4a
- Major bugfix; the gmTrace function wasn't implemented and the persistent table objects
caused a huge crash eventually. This was solved by removing the persistant
objects and implementing the gmTrace function. For completeness I should also
implement the gmMark function for ref-counted GC
- Added GetParamNative to get the native user object from a gmBind object passed
as a gmUserObject in a threadcall
- Used gmMemFixed allocators for allocation of gmBindUserObjects
- Autoproperties have been added. A major new feature to minimise the binding code
you need to write to bind integral types

13/03/2005 Version 0.9.3
- Add WrapObject function, allows the use of native objects as a parameter to
a function

03/03/2005 Version 0.9.2
- Fixed compatibility with GCC 3.4.x compilers

26/01/2005 Version 0.9.1
- Fixed bug which was reported on the GM forums (m_gmTypeLib[])
- Fixed bug in constructor where script created objects weren't attached
- Made note in file that GetObject will only retrieve GLOBAL objects

18/11/2004 Version 0.9.0
- Changed the way the user object is stored
- Added the ability to supply user operators

18/11/2004 Version 0.8.3
- Fixed a typedef to allow compilation under GCC and declared typename on some implicit typedefs
to remove warnings under GCC
- Added initialisation flag to prevent double-init
- Documented public interfaces and provided a simple example

17/11/2004 Version 0.8.2
- Fixed a very ugly bug in gmBind that would lead to random deletion of memory

14/11/2004 Version 0.8.0
- Added Get/Set property functions to allow the user to retrieve properties of global objects
- Perhaps look to extend this into other scopes if needed

14/11/2004 Version 0.7.0
- Added the ability to extend a user-ported object script-side. Each gm user class now
contains a property table as well as it's basic functionality
- Added getThisObject call for obtaining the native user object pointer in bound function calls
- Added flag to stop the GC from culling 'ported' objects

02/11/2004 Version 0.6.1
- Changed name to gmBind, all macros and references to gmObject have been removed

14/10/2004 Version 0.6.0
- Fixing up problems with property registration

13/10/2004 Version 0.5.1
- Bug report on destructor function when using multiple script threads, believe will be solved when porting native object instances

25/09/2004 Version 0.5.0 PR
- 0.5PR sent to GameMonkey authors for previewing/feedback


*********************************************************

gmBind Public interface documentation:


Main API functions
------------------


void Initialise( gmMachine *a_machine, bool a_extensible = true );

a_machine - The GameMonkey Script machine to use
a_extensible - Is the object extensible in script?

Used to initialise this type within a GameMonkey gmMachine. If the type is
flagged as 'extensible' the user is able to extend the type within GMScript
by adding their own methods and properties. A non-extensible type will perform
no action if the user tries to extend the type in script. The default is that all
gmBind types are extensible.



gmType GetType();

Retrieves the gmType that was created when the type was initialised. This function
is inlined for performance.



obj *CreateObject( gmMachine * a_machine, const char *a_name );

a_machine - The GameMonkey Script machine to use
a_name - The name of the object variable created in a_machine

Used to create an instance of this type within the GM Environment. This version
will create a named global object within the machine. Will return a pointer to the
created native type. Be warned that gmBind is the owner of this object and so it
should not be deleted without calling OwnObject(); At present there is no way of
supplying parameters to CreateObject, although this feature is planned for a later
version.


bool DestroyObject( gmMachine * a_machine, const char *a_name );

a_machine - The GameMonkey Script machine to use
a_name - The name of the object variable created in a_machine

Will release a named global object of this type from the gmMachine. The object
will be freed for garbage collection, meaning that it will be destroyed on the next
GC sweep. You should no longer consider references to this object as being valid
and unless you own the pointer to the native object, that will be destroyed too.


gmUserObject *WrapObject( gmMachine *a_machine, T_NATIVE *a_object );

a_machine - The GameMonkey Script machine to use
a_object - The native object to wrap

WrapObject will wrap a native object pointer in a gmBind type. The returned
object is a gmUserObject, which can then be used in function calls and utilities
such as gmCall. The gmUserObject being returned may be susceptible to Garbage 
Collection and therefore it's the native program's responsibility to make it 
persistent. Note that if this object is garbage collected, the native pointer it
referrs to will remain unaffected.



obj *GetObject( gmMachine * a_machine, const char *a_name );

a_machine - The GameMonkey Script machine to use
a_name - The name of the object variable to use

GetObject will retreive a native pointer to a **globally** created object of this 
type from a_machine. If the specified variable doesn't exist or if it is of the 
wrong type then NULL will be returned.



bool OwnObject( gmMachine * a_machine, const char *a_name, bool a_flag );

a_machine - The GameMonkey Script machine to use
a_name - The name of the global gmMachine object to use
a_flag - The ownership setting you wish to pass (true = host owns object)

OwnObject will change the ownership status of an object created by or registered
with gmBind. An object created with CreateObject() is owned by gmBind so shouldn't
be freed within your program. Use OwnObject with a flag setting of 'true' to 
force gmBind to release the pointer to you. This is useful in allowing GM scripts
to create game objects that should persist after the script has ended.



bool IsOwned( gmMachine * a_machine, const char *a_name, bool &a_flag );

a_machine - The GameMonkey Script machine to use
a_name - The name of the global gmMachine object to use
a_flag - The boolean flag to set

Will return the gmBind ownership flag of an object. Will return false if the
object doesn't exist.



bool IsExtensible();

Will return whether the type is user extensible or not



bool GetProperty( gmMachine * a_machine, const char *a_name, const char *a_property, int &a_value );
bool GetProperty( gmMachine * a_machine, const char *a_name, const char *a_property, float &a_value );
bool GetProperty( gmMachine * a_machine, const char *a_name, const char *a_property, char *a_value );

a_machine - The GameMonkey Script machine to use
a_name - The name of the global gmMachine object to use
a_property - The name of the property to retreive
a_value - Reference to the value to fill with the property data

The GetProperty set of functions will retreive the value of an object's
extended property. Like GetObject, the object must exist globally in the machine
context. If the object or property doesn't exist or the value is of an incorrect 
type the call will return false, otherwise the reference value is set and true 
is returned. When using the char* version the string returned should be copied
immediately in order to prevent garbage collection; preferably GC for the gmMachine
should be disabled until you have copied the string.



bool SetProperty( gmMachine *a_machine, const char *a_name, const char *a_property, float a_value );
bool SetProperty( gmMachine *a_machine, const char *a_name, const char *a_property, int a_value );
bool SetProperty( gmMachine *a_machine, const char *a_name, const char *a_property, char *a_value );

a_machine - The GameMonkey Script machine to use
a_name - The name of the global gmMachine object to use
a_property - The name of the property to retreive
a_value - Value to assign to the property

The SetProperty functions work in the same way as GetProperty, except the
property is assigned instead of being retrieved.





Proxy API Functions & Macros
----------------------------


Declaration:    None

GMBIND_INIT_TYPE( a_class, a_name );

a_class - The gmBind derived class to use for this proxy
a_name - The name of the type to declare to GameMonkey

This macro is used within the implementation of a proxy class. gmBind classes
will NOT compile without this declaration.



Declaration:    GMBIND_DECLARE_FUNCTIONS( );

GMBIND_FUNCTION_MAP_BEGIN( a_class );
// Function declarations
GMBIND_FUNCTION_MAP_END()

a_class - The gmBind derived class to use for this proxy

This macro pair forms the function map to be used for this type. If
GMBIND_DECLARE_FUNCTIONS() is not declared within the class definition the 
map will not compile. Only use a function map if your exported type
has native-bound functions.


GMBIND_FUNCTION( a_name, a_gmdeclFunction )

a_name - The name of the function to export to GameMonkey
a_gmdeclFunction - The GM function callback to use

Functions must be declared within the GMBIND_FUNCTION_MAP pair. The GM
function callback used MUST be of the type:

_cdecl int function( gmThread * a_thread );

Please note that when declaring functions within the map, no semi colons or commas
are needed.



Declaration:    GMBIND_DECLARE_PROPERTIES( );

GMBIND_PROPERTY_MAP_BEGIN( a_class );
// Property declarations
GMBIND_PROPERTY_MAP_END();

a_class - The gmBind derived class to use for this proxy

This macro pair forms the property map to be used for this type. If
GMBIND_DECLARE_PROPERTIES() is not declared within the class definition the 
map will not compile. Only use a property map if your exported type
has native-bound properties.


GMBIND_PROPERTY( a_name, a_getter, a_setter )

a_name - The name of the property to export to GameMonkey
a_getter - The property 'get' function to call
a_setter - The property 'set' function to call

Properties must be declared within the GMBIND_PROPERTY_MAP pair. When specifying
properties you can supply a 'get' and/or a 'set' method for this property. If
a property should be considered readonly/writeonly the get/set method can be NULL
respectively. The get/set function declaration prototypes are:

]

a_native is a pointer to the native object associated with the gmBind type.
a_thread is the current calling thread context
a_operands is the operands associated with the call

Generally, with a 'Get' call, a_operands[0] is the value to be populated. When
assigned, this value is what gets passed back to GM. With a 'Set' call, a_operands[1]
is the first value passed to the property to be assigned.



Declaration:	GMBIND_DECLARE_OPERATORS( );

GMBIND_OPERATOR_MAP_BEGIN( a_class );
// Operator declarations
GMBIND_OPERATOR_MAP_END();

a_class - The gmBind derived class to use for this proxy

This macro pair forms the operator map to be used for this type. If
GMBIND_DECLARE_OPERATORS() is not declared within the class definition the 
map will not compile. Only use a operator map if your exported type requires
the user to hook operator calls

GMBIND_OPERATOR_ADD( a_func )
GMBIND_OPERATOR_SUB( a_func )
GMBIND_OPERATOR_NEG( a_func )
GMBIND_OPERATOR_MUL( a_func )
GMBIND_OPERATOR_DIV( a_func )

a_getter - The operator function to call

GMBIND_OPERATOR_INDEX( a_getter, a_setter )

a_getter - The index 'get' function to call
a_setter - The index 'set' function to call

Operators must be declared within the GMBIND_OPERATOR_MAP pair. All operators except
GMBIND_OPERATOR_INDEX follow the same declaration convention, index operations are declared
as a get/set call in one macro call. Any operator function callback can be set to NULL, although
this is the default value. The operator function callback is prototyped as:

_cdecl bool operatorFunc( obj *a_native, gmThread * a_thread, gmVariable * a_operands );

a_native is a pointer to the native object associated with the gmBind type.
a_thread is the current calling thread context
a_operands is the operands associated with the call




Public overloads:


_cdecl r_native *Constructor();

r_native - The native object returned by the constructor

The Constructor method is used within your gmBind derived proxy class. It
is used to create an instance of the native associated with the gmBind type. If
this constructor is not overloaded, the default gmBind implementation will create
an instance of your object using it's default constructor. Therefore, this method
MUST be overloaded if your object has no default constructor or you require extra
control over how your object types are constructed. At present these is no way of
passing parameters to the constructor but this feature is planned for a future release
of gmBind.


_cdecl void Destructor(obj *a_native);

a_native - The native object ready to be destructed. The destructor method 
is called after your object has been garbage collected within the gmMachine and
only if gmBind owns the pointer to the native object. If the pointer is not owned, 
it is the responsibility of the host program to delete the object! The default gmBind
destructor will call 'delete' on the native object - this method should be overloaded
if you require control of how your objects are destroyed.



a_native *GetThisObject( gmThread * a_thread );

a_native - Pointer to the native object returned.


GetThisObject should be used exclusivley within GM function callbacks on
gmBind proxy objects. This function replaces the gmThread::ThisUser type functions
that are normally used. The reason this function exists is because the gmUserObject
pointer normally stored within an object actually refers to the gmBind internal
structure that is responsible for managing the properties and the native pointer
to your object. GetThisObject refers to this structure and retrieves this pointer
for you. It is not advisable to alter the gmBind internal pointer in any way
and failure to use this method could result in a crash.




A simple usage example
----------------------

This example will create a simple gmBind type that is bound to a native class.
The native class 'bomb' is an object used within the host application. For all
intents this class could represent a game object such as a ship or an alien, etc.
The bomb class contains one method and a member variable to demonstrate how gmBind
works.

// The native class

class bomb
{
public:

bomb()
{
isExploded = 0;
}

// Explode the bomb - the bomb can only be exploded once before it needs
// to be reset
void explode()
{
if (!isExploded)
{
std::cout << "BOOM!" << std::endl;
isExploded = 1;
} 
else
{
std::cout << "Already exploded, dude" << std::endl;
}
}
// Attribute flag to state whether the bomb has been exploded or not
int isExploded;
};

// The gmBind proxy class

// The class needs to be derived from gmBind using the native class and the
// proxy class as template parameters

// gmBomb.h

#include "gmBind.h"     // Must include gmBind!

class gmBomb	:	public gmBind< bomb, gmBomb >
{
public:
// Indicate that this proxy class exports functions
GMBIND_DECLARE_FUNCTIONS( );
// This proxy also exports properties
GMBIND_DECLARE_PROPERTIES( );

// No Constructor or Destructor are required

// No gmbind Constructor() or Destructor() methods are needed as this
// is a simple class which needs no special constructor arguments

// Declare a method to handle the explosion callback
static int gmExplode(gmThread * a_thread);

// Declare a get/set property callback to handle the 'exploded' property
static bool getExploded( bomb *a_native, gmThread * a_thread, gmVariable * a_operands );
static bool setExploded( bomb *a_native, gmThread * a_thread, gmVariable * a_operands );
};


//	gmBomb.cpp

#include "gmBomb.h"

// Initialise the static data for this type and declare it's gmType name as 'bomb'
GMBIND_INIT_TYPE( gmBomb, "bomb" );

// Implement the function map for this type
GMBIND_FUNCTION_MAP_BEGIN( gmBomb )
// Declare a gm function called "explode" using gmBomb::gmExplode as it's
// native callback
//
GMBIND_FUNCTION( "explode", gmExplode )
//
GMBIND_FUNCTION_MAP_END()

// Implement the property map for this type
GMBIND_PROPERTY_MAP_BEGIN( gmBomb )
// Declare a property "isExploded" which uses gmBomb::getExploded / gmBomb::setExploded
// as it's access callbacks. Omitting setExploded will make the property read-only
//
GMBIND_PROPERTY( "isExploded", getExploded, setExploded )
//
GMBIND_PROPERTY_MAP_END();

int gmBomb::gmExplode(gmThread * a_thread)
{
// Obtain a pointer to the native object associated with the gmBind type
// instance - you MUST use GetThisObject instead of the usual GameMonkey
// gmThread::ThisUser functions
bomb* native = gmBomb::GetThisObject( a_thread );
// Call the native object's explode method!
native->explode();
// return OK, all is well
return GM_OK; 
}

bool gmBomb::getExploded( bomb *a_native, gmThread * a_thread, gmVariable * a_operands )
{
// Set the return of the 'get' method to 0 or 1, depending on whether the
// bomb's native instance has been blown
//
// GM Example:
//
// b = bomb();
// print( b.isExploded );
//
a_operands[0].m_value.m_int = (a_native->isExploded ? 1 : 0);
// Return true, the property get was successful
return true;
}

bool gmBomb::setExploded( bomb *a_native, gmThread * a_thread, gmVariable * a_operands )
{
// Set the native bomb's exploded member as 1 or 0, depending on the status
// of the parameter passed in script
//
// GM Example:
//
// b = bomb();
// b.isExploded = 1;
//
a_native->isExploded = (a_operands[1].m_value.m_int ? 1 : 0);
// Return true, the property set was successful
return true;
}


// main.cpp

#include <iostream>

#include "gmMachine.h"    // Technically not needed as it is included implicitly through gmBind.h

#include "bomb.h"
#include "gmBomb.h"

int main()
{
// Create a GameMonkey machine
gmMachine   *gm = new gmMachine();

// Initialise the 'bomb' type within this gmMachine
gmBomb::Initialise( gm );

// Create a global bomb object called 'boomer'
gmBomb::CreateObject( gm, "boomer" );    
// This object can now be used within your scripts
//
// GM Example
//
// boomer.explode();        // should go boom!
// boomer.explode();        // already exploded :(
// boomer.isExploded = 0;   // Reset the exploded status
// boomer.explode();        // should go boom again!

// Let's have some fun and claim boomer from the script
bomb *myBomb = 0;
// Retrieve native object pointer belonging to boomer
myBomb = gmBomb::GetObject( gm, "boomer" );
// Reclaim ownership to the pointer - gmBind will no longer delete it
gmBomb::OwnObject( gm, "boomer", true );

// myBomb can now persist beyond the script and gmMachine
// But all this power is scary, give it back to the script
gmBomb::OwnObject( gm, "boomer", false );
// We no longer own the pointer to myBomb, so it could be GC'd and deleted

// Let's create an extended property on boomer for fun
// Create a string property called "MyName" and assign it a value
gmBomb::SetProperty( gm, "boomer", "MyName", "BOOMER!" );

// Execute a script to display boomer's name
gm->ExecuteString( "print( `Hello my name is ` + boomer.MyName );" );

// > Hello my name is BOOMER!

// Execute another script to change boomer's name
gm->ExecuteString( "boomer.MyName = `Freddy`;" );

// Execute a script to display boomer's name again
gm->ExecuteString( "print( `Hello my name is ` + boomer.MyName );" );

// > Hello my name is Freddy

// Let's retreive boomer's name ourselves
const char *name = 0;
gmBomb::GetProperty( gm, "boomer", "MyName", name );

// Say his name from the program
std::cout << "boomer's name is " << name << std::endl;

delete gm;

// Too much fun, bye bye. Don't worry about boomer, he'll be GC'd
// and his native pointer deleted    
return 0;  
}; 



*********************************************************

License Agreement:

GameMonkey C++ Class Binding Template

Copyright (c) 2004 Oliver Wilkinson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated 
documentation files (the "Software"), to deal in the 
Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the 
Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice 
shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************/

#ifndef __GAMEMONKEY_BINDING_TEMPLATE_GMBIND_H__
#define __GAMEMONKEY_BINDING_TEMPLATE_GMBIND_H__

#include <stddef.h>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "gmMachine.h"
#include "gmThread.h"

#define GMBIND_VERSION "0.9.5"

#undef GetObject //Argh Windows defines this in WINGDI.H

template <class T>
class gmBindHashMap
{
public:
	typedef std::map< int, T >	tMap;
	typedef std::pair< int, T >	tMapPair;

	typedef typename tMap::iterator		tMapIterator;

	tMapIterator begin()
	{
		return m_map.begin();
	}

	tMapIterator end()
	{
		return m_map.end();
	}

	void insert( const char *key, T data)
	{
		int hash = _hashFunc( key );
		m_map.insert( tMapPair( hash, data ) );
	}

	void remove( const char *key )
	{
		int hash = _hashFunc( key );
		typename tMap::iterator it = m_map.find( hash );
		if ( it == m_map.end() )
			return;
		m_map.erase( it );
	}

	void clear()
	{
		m_map.clear();
	}

	size_t size()
	{
		return m_map.size();
	}

	T find( const char *key )
	{
		int hash = _hashFunc( key );
		typename tMap::iterator it = m_map.find( hash );
		if ( it == m_map.end() )
			return T();
		return it->second;
	}	

private:
	tMap		m_map;

	int	_hashFunc(const char *text)
	{
		// http://www.flipcode.com/cgi-bin/msg.cgi?showThread=Tip-HashString&forum=totd&id=-1
		unsigned int iHash = 5381; 
		int c; int index = 0;
		while( text[index] != '\0' ) 
		{ 
			c = text[index] ; ++index; 
			iHash = ((iHash << 5) + iHash) + c; 
		} 
		return( iHash );
	}

};








///////////////////////////////////////////////////////////////////////////////
// Class:	 gmBind
//
// Templated class to allow easy binding of C++ classes to GameMonkey. Takes
// the native type and the proxy api type as template parameters
//
template <class T_NATIVE, class T_API>
class gmBind
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Type:	T_OBJ
	//
	// Typedef to allow easy access to this template type
	//
	typedef gmBind< T_NATIVE, T_API >						T_OBJ;
	typedef T_NATIVE                						T_NATIVETYPE;
	//
	////////////////////////////////////////////////////////////////////////
	// Type:	gmBindUserObject
	//
	// Holds the user object pointer and the virtual property table for extensions
	//
	struct gmBindUserObject
	{
		gmTableObject	*m_table;		// Extended properties
		T_NATIVE		*m_object;			// The native object
		bool			m_native;			// Is this native?
	};
	//
	//

	//////////////////////////////////////////////////////
	// Parameterised contructor
	// Is called instead of the default if parameters are passed by GM	
	static T_NATIVE *Constructor( gmThread *a_thread )
	{
		return new T_NATIVE();
	}

	//////////////////////////////////////////////////////
	// AsString callback
	// Called whenever the scripting system wishes to create a string
	// for an instance of the object
	static void AsString(gmUserObject *a_object, char *a_buffer, int a_bufferLen)
	{
		a_buffer[0] = 0;
		T_NATIVE * nativeObj = GetNative( a_object );
		if(nativeObj)
		{
			T_API::AsStringCallback(nativeObj, a_buffer, a_bufferLen);
		}
	}
	static void AsStringCallback(T_NATIVE * a_object, char * a_buffer, int a_bufferLen)
	{
		_gmsnprintf(a_buffer, a_bufferLen, "%p", a_object);
	}
	static void DebugInfo(gmUserObject *a_object, gmMachine *a_machine, gmChildInfoCallback a_infoCallback)
	{
		gmTableObject *pTbl = GetUserTable(a_object);
		if(pTbl)
		{
			const int buffSize = 256;
			char buffVar[buffSize];
			char buffVal[buffSize];

			gmTableIterator tIt;
			gmTableNode *pNode = pTbl->GetFirst(tIt);
			while(pNode)
			{
				const char *pVar = pNode->m_key.AsString(a_machine, buffVar, buffSize);
				const char *pVal = pNode->m_value.AsString(a_machine, buffVal, buffSize);
				a_infoCallback(
					pVar, 
					pVal,
					a_machine->GetTypeName(pNode->m_value.m_type),
					pNode->m_value.IsReference() ? pNode->m_value.m_value.m_ref : 0);
				pNode = pTbl->GetNext(tIt);
			}
		}
	}

	////////////////////////////////////////////////////
	// Method:		Destructor
	//
	// Public user destructor for bound object
	//
	// Default implementation will delete the object created in
	// the user-specified constructor. Override to prevent this
	// from happening
	//
	// Parameters:
	//
	// obj - The native object being destroyed
	//
	// History:
	//		13/10/2004 - Bug report received about multiple scripts deleting an object twice
	//
	static void Destructor( T_NATIVE *a_native )
	{
		delete( a_native );
	}

	//////////////////////////////////////////////////////
	// Method:		Initialise
	//
	// Initialise function, to be called ONCE to initialise the class in the GM machine
	// Handles registration of types and sets the get/set dot operators to allows 
	// the user to hook callbacks
	//
	// Parameters:
	// 
	// a_machine - The gmMachine instance that the class is registered to
	//
	static void Initialise( gmMachine *a_machine, bool a_extensible = true )
	{
		// m_gmType is static, but a_machine is not static
		// gmMachine is deleted at the end of each match and new gmMachine is created when next match starts
		// static data are cleared and initialized on Windows when DLL is loaded
		// static data are never cleared on Linux because dlclose does not work in current version of gcc
#ifdef WIN32
		GM_ASSERT(!GetType());
#endif

		// Registery library
		a_machine->RegisterLibrary( T_API::m_gmTypeLib, 1 );
		// Create the new user type
		m_gmType = a_machine->CreateUserType( m_gmTypeName );
		
		m_extensible = a_extensible;

		//
		// register as a type
		// This will call the registerFunctions override function to register
		// each function in turn with the machine. Mimics the behaviour of gmMachine::RegisterTypeLibrary
		T_API::registerFunctions( a_machine );

		// Register destructor
		a_machine->RegisterUserCallbacks(m_gmType, gmfTrace, gmfDestruct, T_API::AsString, T_API::DebugInfo );

		memset( &m_operatorFunctions, 0, sizeof(m_operatorFunctions) );

		// Set up user property handlers, calling the user's API implementation
		T_API::registerProperties();
		// Register the operator overload callbacks
		T_API::registerOperators();

		//
		// Register the property callbacks
		//
		a_machine->RegisterTypeOperator( m_gmType, O_GETDOT, NULL, gmOpGetDot);
		a_machine->RegisterTypeOperator( m_gmType, O_SETDOT, NULL, gmOpSetDot);

		//
		// Index operator callback
		//
		if(m_operatorFunctions.opGetIndex)
			a_machine->RegisterTypeOperator( m_gmType, O_GETIND, NULL, gmOpGetInd);
		if(m_operatorFunctions.opSetIndex)
			a_machine->RegisterTypeOperator( m_gmType, O_SETIND, NULL, gmOpSetInd);
		//

		//
		// Register operator callbacks for ones that are defined
		if(m_operatorFunctions.opAdd)
			a_machine->RegisterTypeOperator( m_gmType, O_ADD, NULL, gmOpAdd);
		if(m_operatorFunctions.opSub)
			a_machine->RegisterTypeOperator( m_gmType, O_SUB, NULL, gmOpSub);
		if(m_operatorFunctions.opMul)
			a_machine->RegisterTypeOperator( m_gmType, O_MUL, NULL, gmOpMul);
		if(m_operatorFunctions.opDiv)
			a_machine->RegisterTypeOperator( m_gmType, O_DIV, NULL, gmOpDiv);
		if(m_operatorFunctions.opNeg)
			a_machine->RegisterTypeOperator( m_gmType, O_NEG, NULL, gmOpNeg);
		if(m_operatorFunctions.opRem)
			a_machine->RegisterTypeOperator( m_gmType, O_REM, NULL, gmOpRem);
		if(m_operatorFunctions.opBitOr)
			a_machine->RegisterTypeOperator( m_gmType, O_BIT_OR, NULL, gmOpBitOr);
		if(m_operatorFunctions.opBitXOr)
			a_machine->RegisterTypeOperator( m_gmType, O_BIT_XOR, NULL, gmOpBitXOr);
		if(m_operatorFunctions.opBitAnd)
			a_machine->RegisterTypeOperator( m_gmType, O_BIT_AND, NULL, gmOpBitAnd);
		if(m_operatorFunctions.opBitShiftLeft)
			a_machine->RegisterTypeOperator( m_gmType, O_BIT_SHIFTLEFT, NULL, gmOpBitShiftL);
		if(m_operatorFunctions.opBitShiftRight)
			a_machine->RegisterTypeOperator( m_gmType, O_BIT_SHIFTRIGHT, NULL, gmOpBitShiftR);
		if(m_operatorFunctions.opBitInv)
			a_machine->RegisterTypeOperator( m_gmType, O_BIT_INV, NULL, gmOpBitInv);
		if(m_operatorFunctions.opLessThan)
			a_machine->RegisterTypeOperator( m_gmType, O_LT, NULL, gmOpLT);
		if(m_operatorFunctions.opGreaterThan)
			a_machine->RegisterTypeOperator( m_gmType, O_GT, NULL, gmOpGT);
		if(m_operatorFunctions.opLessThanOrEqual)
			a_machine->RegisterTypeOperator( m_gmType, O_LTE, NULL, gmOpLTE);
		if(m_operatorFunctions.opGreaterThanOrEqual)
			a_machine->RegisterTypeOperator( m_gmType, O_GTE, NULL, gmOpGTE);
		if(m_operatorFunctions.opIsEqual)
			a_machine->RegisterTypeOperator( m_gmType, O_EQ, NULL, gmOpIsEq);
		if(m_operatorFunctions.opIsNotEqual)
			a_machine->RegisterTypeOperator( m_gmType, O_NEQ, NULL, gmOpIsNotEq);
		if(m_operatorFunctions.opPos)
			a_machine->RegisterTypeOperator( m_gmType, O_POS, NULL, gmOpPos);
		if(m_operatorFunctions.opNot)
			a_machine->RegisterTypeOperator( m_gmType, O_NOT, NULL, gmOpNot);
	}


	////////////////////////////////////////////////////////
	// Method:		IsExtensible
	//
	// Returns whether the object is extensible script-side or not
	//
	static bool IsExtensible() { return m_extensible; }

	////////////////////////////////////////////////////////
	// Method:		CreateObject
	//
	// Creates a GM object based on this class and assigns it to the global table
	//
	//static T_NATIVE *CreateObject( gmMachine * a_machine, const char *a_name )
	//{
	//	// Return NULL immediately if the type hasn't been initialised
	//	if ( GetType() == GM_NULL )
	//		return 0;   

	//	a_machine->AdjustKnownMemoryUsed( sizeof( gmBindUserObject ) );
	//	// Create a new machine variable
	//	gmTableObject *table = a_machine->GetGlobals();
	//	// Native object to bind
	//	T_NATIVE *p = T_API::Constructor( );
	//	// Create holding object
	//	gmBindUserObject *gmbUser = _allocObject( a_machine, p, true );
	//	//a_machine->GetGC()->MakeObjectPersistant( gmbUser->m_table );

	//	gmVariable var;
	//	var.SetUser( a_machine->AllocUserObject( gmbUser, GetType() ) );

	//	// Set in machine global table
	//	table->Set( a_machine, a_name, var );

	//	return p;
	//}

	
	
	
	static void SetGlobalObject( gmMachine * a_machine, const char *a_name, gmUserObject *a_object )
	{
		gmTableObject *table = a_machine->GetGlobals();
		table->Set( a_machine, a_name, gmVariable( GetType(), (gmptr)a_object ) );
	}
	
	static bool RenameGlobalObject( gmMachine * a_machine, const char *a_name, const char *a_new_name )
	{
		gmTableObject *table = a_machine->GetGlobals();
		gmVariable old( table->Get( a_machine, a_name ) );
		
		if (old.m_type == GM_NULL)
            return false;
		
		table->Set( a_machine, a_new_name, gmVariable::s_null );
		table->Set( a_machine, a_name, old );
		return true;
	}
	
	////////////////////////////////////////////////////////
	// Method:		CreateObject
	//
	// Creates a GM object based on this class
	//
	static T_NATIVE *CreateObject( gmMachine * a_machine, bool a_gm_owned = true )
	{
		// Return NULL immediately if the type hasn't been initialised
		if ( GetType() == GM_NULL )
			return 0;   

		a_machine->AdjustKnownMemoryUsed( sizeof( gmBindUserObject ) );

		// Native object to bind
		T_NATIVE *p = T_API::Constructor( );
		// Create holding object
		gmBindUserObject *gmbUser = _allocObject( a_machine, p, !a_gm_owned );

		return p;
	}
	
	////////////////////////////////////////////////////////
	// Method:      CreateGmUserObject
	//
	// Creates a gmUserObject that holds the gmBindUserObject.
	// Useful for allocating objects for use in GM
	//
	//
	static gmUserObject *CreateGmUserObject( gmMachine * a_machine, bool a_gm_owned = true )
	{
		// Return NULL immediately if the type hasn't been initialised
		if ( GetType() == GM_NULL )
			return 0;   

		a_machine->AdjustKnownMemoryUsed( sizeof( gmBindUserObject ) );

		// Native object to bind
		T_NATIVE *p = T_API::Constructor( 0 );
		
		// Create holding object
		gmBindUserObject *gmbUser = _allocObject( a_machine, p, !a_gm_owned );
        
		return a_machine->AllocUserObject( gmbUser, GetType() );
	}

	////////////////////////////////////////////////////////
	// Method:		PushObject
	//
	// Creates a GM object based on this class and pushes onto thread
	//
	static void PushObject( gmThread *a_thread, const T_NATIVE &_obj )
	{
		// Return NULL immediately if the type hasn't been initialised
		if ( GetType() == GM_NULL )
			return;   

		gmMachine *pMachine = a_thread->GetMachine();
		pMachine->AdjustKnownMemoryUsed( sizeof( gmBindUserObject ) );

		// Native object to bind
		T_NATIVE *p = T_API::Constructor( 0 );
		// Copy the passed in parameter.
		*p = _obj;

		// Create holding object
		gmBindUserObject *gmbUser = _allocObject( pMachine, p, false );
		//a_machine->GetGC()->MakeObjectPersistant( gmbUser->m_table );

		gmVariable var;
		var.SetUser( pMachine->AllocUserObject( gmbUser, GetType() ) );

		// Set in machine global table
		a_thread->Push(var);
	}

	////////////////////////////////////////////////////////
	// Method:		SetObject
	//
	// Sets a gmVariable with the value of an object
	//
	static void SetObject( gmMachine *a_machine, gmVariable &_var, const T_NATIVE &_obj )
	{
		// Return NULL immediately if the type hasn't been initialised
		if ( GetType() == GM_NULL )
			return;   

		a_machine->AdjustKnownMemoryUsed( sizeof( gmBindUserObject ) );

		// Native object to bind
		T_NATIVE *p = T_API::Constructor( 0 );

		// Copy the passed in parameter.
		*p = _obj;

		// Create holding object
		gmBindUserObject *gmbUser = _allocObject( a_machine, p, false );

		_var.SetUser( a_machine->AllocUserObject( gmbUser, GetType() ) );
	}

	////////////////////////////////////////////////////////
	// Method:		WrapObject
	//
	// Will wrap a native object up as a gmUserObject for use in function
	// calls and utilities such as gmBind
	//
	static gmUserObject *WrapObject( gmMachine *a_machine, T_NATIVE *a_object )
	{
		if ( GetType() == GM_NULL )
			return 0;

		a_machine->AdjustKnownMemoryUsed( sizeof( gmBindUserObject ) );

		// Create holding object
		gmBindUserObject *gmbUser = _allocObject( a_machine, a_object, true );

		gmUserObject *usr = a_machine->AllocUserObject( gmbUser, GetType() );      
		return usr;  
	}

	////////////////////////////////////////////////////////
	// Method:		DestroyObject
	//
	// Destroys an object of this type created globally in script
	// or via CreateObject
	//
	static bool DestroyObject( gmMachine * a_machine, const char *a_name )
	{
		// Return NULL immediately if the type hasn't been initialised
		if ( GetType() == GM_NULL )
			return false;

		gmTableObject *table = a_machine->GetGlobals();
		gmVariable	var = table->Get(a_machine, a_name);
		if (var.m_type != GetType())
			return false;

		var.Nullify();
		// Set in machine global table
		table->Set( a_machine, a_name, var );
		return true;
	}


	///////////////////////////////////////////////////////
	// Method:		GetThisObject
	//
	// Used in thread calls to return the user object 
	//
	//
	static GM_INLINE T_NATIVE *GetThisObject( gmThread * a_thread )
	{          
		gmBindUserObject	*gmbUser = (gmBindUserObject *)a_thread->ThisUser_NoChecks();
		return gmbUser->m_object;
	}

	static GM_INLINE T_NATIVE *GetThisObjectSafe( gmThread * a_thread )
	{          
		gmBindUserObject	*gmbUser = (gmBindUserObject *)a_thread->ThisUserCheckType(GetType());
		return gmbUser ? gmbUser->m_object : 0;
	}

	///////////////////////////////////////////////////////
	// Method:		GetThisTable
	//
	// Used in thread calls to return the user object 
	//
	//
	static GM_INLINE gmTableObject *GetThisTable( gmThread * a_thread )
	{          
		gmBindUserObject *gmbUser = static_cast<gmBindUserObject*>(a_thread->ThisUser_NoChecks());
		return gmbUser->m_table;
	}

	///////////////////////////////////////////////////////
	static GM_INLINE T_NATIVE *GetNative( gmUserObject *a_user )
	{
		GM_ASSERT(a_user && a_user->GetType() == GetType());
		if(a_user != NULL && a_user->GetType() == GetType())
		{
			gmBindUserObject *u = static_cast<gmBindUserObject*>(a_user->m_user);
			return (T_NATIVE *)u->m_object;
		}
		return NULL;
	}

	////////////////////////////////////////////////////////
	// Method:		GetObject
	//
	// Gets a named object of this type
	//
	// Returns null if not found
	//
	static T_NATIVE *GetObject( gmMachine * a_machine, const char *a_name )
	{        
		gmTableObject *table = a_machine->GetGlobals();
		gmVariable	var = table->Get(a_machine, a_name);
		if (var.m_type != GetType())
			return 0;
		// Look it up in our table
		
		gmUserObject *user = var.GetUserObjectSafe();
		gmBindUserObject *gmbUser = static_cast<gmBindUserObject*>(user->m_user);
		return (T_NATIVE *)gmbUser->m_object;
	}

	////////////////////////////////////////////////////////
	// Method:		OwnObject
	//
	// Take ownership of the specified object. If an object is owned then this object's
	// native type won't be free()'d upon GC
	//
	// Returns false if flag not changed
	//
	static bool GM_INLINE OwnObject( gmMachine * a_machine, const char *a_name, bool a_flag )
	{
		gmTableObject *table = a_machine->GetGlobals();
		gmVariable	var = table->Get(a_machine, a_name);
		if (var.m_type != GetType())
			return false;
		// Look it up in our table
		gmUserObject *user = var.GetUserObjectSafe();
		gmBindUserObject *gmbUser =  static_cast<gmBindUserObject*>(user->m_user);

		// If the object is already flagged as desired type, return false
		if (gmbUser->m_native == a_flag)
			return false;
		/// Otherwise flag it
		gmbUser->m_native = a_flag;
		return true;
	}

    static bool GM_INLINE SetObjectOwnershipNative( gmMachine * a_machine, const char *a_name )
    {
    	return OwnObject(a_machine, a_name, true);
    }
    
    static bool GM_INLINE SetObjectOwnershipGM( gmMachine * a_machine, const char *a_name )
    {
    	return OwnObject(a_machine, a_name, false);
    }
    

    static bool GM_INLINE OwnObject( gmMachine * a_machine, gmUserObject *a_object, bool a_flag )
	{
		if (a_object->GetType() != GetType())
			return false;
		// Look it up in our table
		gmBindUserObject *gmbUser = static_cast<gmBindUserObject*>(a_object->m_user);

		// If the object is already flagged as desired type, return false
		if (gmbUser->m_native == a_flag)
			return false;
		/// Otherwise flag it
		gmbUser->m_native = a_flag;
		return true;
	}

    static bool GM_INLINE SetObjectOwnershipNative( gmMachine * a_machine, gmUserObject *a_object )
    {
    	return OwnObject(a_machine, a_object, true);
    }
    
    static bool GM_INLINE SetObjectOwnershipGM( gmMachine * a_machine, gmUserObject *a_object )
    {
    	return OwnObject(a_machine, a_object, false);
    }

	////////////////////////////////////////////////////////
	// Method:		IsOwned
	//
	// Returns the gmBind ownership of the object
	//
	static bool IsOwned( gmMachine * a_machine, const char *a_name, bool &a_flag )
	{
		gmTableObject *table = a_machine->GetGlobals();
		gmVariable	var = table->Get(a_machine, a_name);
		if (var.m_type != GetType())
			return false;
		// Look it up in our table
		gmUserObject *user = var.GetUserObjectSafe();
		gmBindUserObject *gmbUser = static_cast<gmBindUserObject*>(user->m_user);
		// If native = false then the object is owned
		a_flag = (gmbUser->m_native);
		return true;
	}    

	static bool IsOwned( gmMachine * a_machine, gmUserObject *a_object, bool &a_flag )
	{
		if (a_object->GetType() != GetType())
			return false;

		gmBindUserObject *gmbUser = static_cast<gmBindUserObject*>(a_object->m_user);
		// If native = false then the object is owned
		a_flag = (gmbUser->m_native);
		return true;
	} 

	////////////////////////////////////////////////////////
	// Method:		GetProperty
	//
	// Gets a property of a gm object and fills the passed value
	//
	// Comes in three versions for now; int, float and const char*
	//
	// Returns false if there was an error
	//
	static bool GetProperty( gmMachine * a_machine, const char *a_name, const char *a_property, int &a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_name, a_property );
		if (var.m_type != GM_INT)
			return false;

		a_value = var.m_value.m_int;
		return true;
	}

	static bool GetProperty( gmMachine * a_machine, const char *a_name, const char *a_property, float &a_value )
	{
		// Float variables can be stored in the gmMachine as a float OR an int

		gmVariable var = gmGetObjectProperty( a_machine, a_name, a_property );
		if (var.m_type == GM_FLOAT)
		{
			a_value = var.m_value.m_float;
			return true;
		}
		if (var.m_type == GM_INT)
		{
			a_value = var.m_value.m_int;
			return true;
		}

		return false;
	}

	static bool GetProperty( gmMachine * a_machine, const char *a_name, const char *a_property, const char *&a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_name, a_property );
		if (var.m_type != GM_STRING)
			return false;

		gmStringObject *str = var.GetStringObjectSafe();
		a_value = str->GetString();
		return true;
	}

	static bool GetPropertyFunction( gmMachine * a_machine, const char *a_name, const char *a_property, gmFunctionObject *&a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_name, a_property );
		if (var.m_type != GM_FUNCTION)
			return false;

		gmFunctionObject *f = var.GetFunctionObjectSafe();
		a_value = f;
		return true;	
	}

	static bool GetPropertyTable( gmMachine * a_machine, const char *a_name, const char *a_property, gmTableObject *&a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_name, a_property );
		if (var.m_type != GM_TABLE)
			return false;

		gmTableObject *t = var.GetTableObjectSafe();
		a_value = t;
		return true;	
	}

	static bool GetPropertyUser( gmMachine * a_machine, const char *a_name, const char *a_property, gmUserObject *&a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_name, a_property );
		if (var.m_type != GM_USER)
			return false;

		gmUserObject *t = var.GetUserObjectSafe();
		a_value = t;
		return true;	
	}
	
	
	///////////////////////////////////////////////
	// Method:      GetProperty
	//
	// These methods work on a gmUserObject holding a pointer to a gmUserBoundObject
	//
	
	static bool GetProperty( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, int &a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_object, a_property );
		if (var.m_type != GM_INT)
			return false;
		a_value = var.m_value.m_int;
		return true;
	}
	
	static bool GetProperty( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, float &a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_object, a_property );
		if (var.m_type != GM_FLOAT)
			return false;
		a_value = var.m_value.m_float;
		return true;
	}
	
	static bool GetProperty( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, const char *&a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_object, a_property );
		if (var.m_type != GM_STRING)
			return false;
		gmStringObject *str = var.GetStringObjectSafe();
		a_value = str->GetString();
		return true;
	}
	
	
	static bool GetPropertyFunction( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, gmFunctionObject *&a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_object, a_property );
		if (var.m_type != GM_FUNCTION)
			return false;

		gmFunctionObject *f = var.GetFunctionObjectSafe();
		a_value = f;
		return true;	
	}

	static bool GetPropertyTable( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, gmTableObject *&a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_object, a_property );
		if (var.m_type != GM_TABLE)
			return false;

		gmTableObject *t = var.GetTableObjectSafe();
		a_value = t;
		return true;	
	}
	
	static bool GetPropertyUser( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, gmUserObject *&a_value )
	{
		gmVariable var = gmGetObjectProperty( a_machine, a_object, a_property );
		if (var.m_type != GM_USER)
			return false;

		gmUserObject *t = var.GetUserObjectSafe();
		a_value = t;
		return true;	
	}
	

	////////////////////////////////////////////////////////
	// Method:		SetProperty
	//
	// Sets a property of a gm object with the user-specified value
	//
	// Comes in three versions for now; int, float and const char*
	//
	// Returns false if there was an error
	//
	static bool SetProperty( gmMachine *a_machine, const char *a_name, const char *a_property, float a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_name );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar;
		userVar.SetFloat( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;
	}

	static bool SetProperty( gmMachine *a_machine, const char *a_name, const char *a_property, int a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_name );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar;
		userVar.SetInt( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;
	}

	static bool SetProperty( gmMachine *a_machine, const char *a_name, const char *a_property, const char *a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_name );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar( a_machine->AllocStringObject( a_value ) );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;
	}

	static bool SetPropertyFunction( gmMachine * a_machine, const char *a_name, const char *a_property, gmFunctionObject *a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_name );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;	
	}

	static bool SetPropertyTable( gmMachine * a_machine, const char *a_name, const char *a_property, gmTableObject *a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_name );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;	
	}

	static bool SetPropertyUser( gmMachine * a_machine, const char *a_name, const char *a_property, gmUserObject *a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_name );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;	
	}
	
	
    ///////////////////////////////////////////////
	// Method:      SetProperty
	//
	// These methods work on a gmUserObject holding a pointer to a gmUserBoundObject
	//
	
	static bool SetProperty( gmMachine *a_machine, gmUserObject *a_object, const char *a_property, float a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_object );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar;
		userVar.SetFloat( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;
	}

    static bool SetProperty( gmMachine *a_machine, gmUserObject *a_object, const char *a_property, int a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_object );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar;
		userVar.SetInt( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;
	}
	
	static bool SetProperty( gmMachine *a_machine, gmUserObject *a_object, const char *a_property, const char *a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_object );
		if (!gmbUser)
			return false;

		// Find the property
        gmVariable userVar( a_machine->AllocStringObject( a_value ) );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;
	}
	
    static bool SetPropertyFunction( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, gmFunctionObject *a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_object );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;	
	}

	static bool SetPropertyTable( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, gmTableObject *a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_object );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;	
	}
	
	static bool SetPropertyUser( gmMachine * a_machine, gmUserObject *a_object, const char *a_property, gmUserObject *a_value )
	{
		gmBindUserObject *gmbUser = gmGetUserBoundObject( a_machine, a_object );
		if (!gmbUser)
			return false;

		// Find the property
		gmVariable userVar( a_value );
		gmbUser->m_table->Set( a_machine, a_property, userVar );
		return true;	
	}

	/////////////////////////////////////////////////////////
	// Method:      GetUserBoundObject
	//
	// Returns the gmBindUserObject object by name
	//
	// WARNING: USE WITH CARE - Do not free or delete this object, table or user pointer
	//
	// Made public in 0.9.4c
	//	
	static gmBindUserObject *GetUserBoundObject( gmMachine *a_machine, const char *objName )
	{
		return gmGetUserBoundObject( a_machine, objName );
	}
	static gmBindUserObject *GetUserBoundObject( gmMachine *a_machine, gmUserObject *_userObj )
	{
		if (_userObj->GetType() != GetType())
			return 0;

		gmBindUserObject *gmbUser = (gmBindUserObject *)_userObj->m_user;
		return gmbUser;
	}
	
	static gmBindUserObject *GetUserBoundObject( gmMachine *a_machine, gmVariable &_var )
	{
		gmUserObject *user = _var.GetUserObjectSafe(GetType());
		if(!user)
			return 0;

		gmBindUserObject *gmbUser = static_cast<gmBindUserObject*>(user->m_user);
		return gmbUser;
	}

	/////////////////////////////////////////////////////////
	// Method:      GetUserTable
	//
	// Returns the gmTableObject of the gmBind object instance
	//
	// WARNING: USE WITH CARE - Do not free or delete this table
	//	
	static gmTableObject *GetUserTable( gmUserObject *_userObj )
	{
		if (_userObj->GetType() != GetType())
			return 0;

		gmBindUserObject *gmbUser = (gmBindUserObject *)_userObj->m_user;
		return gmbUser->m_table;
	}

	/////////////////////////////////////////////////////////
	// Method:      NullifyObject
	//
	// Nullifies the object pointed to by this object.
	//	
	static void NullifyObject( gmUserObject *_userObj )
	{
		GM_ASSERT(_userObj->GetType() == GetType());
		if (_userObj->GetType() == GetType())
		{
			gmBindUserObject *gmbUser = (gmBindUserObject *)_userObj->m_user;
			GM_ASSERT(gmbUser->m_native == true); 
			gmbUser->m_object = NULL;
		}
	}


	/////////////////////////////////////////////////////////
	// Method:		GetType
	//
	// Returns the gmType of the bound object
	//
	static GM_INLINE gmType GetType()
	{
		return T_API::m_gmType;
	}

	/////////////////////////////////////////////////////////
	// Method:		GetTypeName
	//
	// Returns the type name of the bound object
	//
	static GM_INLINE const char *GetTypeName()
	{
		return T_API::m_gmTypeName;
	}

	//////////////////////////////////////////////////////////////////////////
	// Type:	gmBindPropertyFP 
	//
	// Typedef for the function pointer to the property callbacks
	typedef bool ( *gmBindPropertyFP )( T_NATIVE *p, gmThread * a_thread, gmVariable * a_operands );
	//
	//////////////////////////////////////////////////////////////////////////
	// Type:	gmBindOperatorFP 
	//
	// Typedef for the function pointer to the property callbacks
	typedef bool ( *gmBindOperatorFP )( gmThread * a_thread, gmVariable * a_operands );
	//
	///////////////////////////////////////////////////////////////////////////
	// Type:	gmBindPropertyFunctionPair
	//
	// Used to hold onto the function pointers for the user-specified get/set callbacks
	//
	struct gmBindPropertyFunctionPair
	{
		gmBindPropertyFunctionPair() : getter(0), setter(0), isAutoProp(false), propType(GM_NULL), propOffset(0) { }
		gmBindPropertyFunctionPair(gmBindPropertyFP g, gmBindPropertyFP s) : getter(g), setter(s), isAutoProp(false), propType(GM_NULL), propOffset(0) { }
		gmBindPropertyFunctionPair(gmBindPropertyFP g, gmBindPropertyFP s, int a_propType, size_t a_propOffset) : getter(g), setter(s), isAutoProp(true), propType(a_propType), propOffset(a_propOffset) { }
		gmBindPropertyFP getter;
		gmBindPropertyFP setter;
		// Autoprop stuff
		bool isAutoProp;
		int propType;
		size_t propOffset;
	};
	//
	///////////////////////////////////////////////////////////////////////
	// Type: T_FPMAP
	//
	// Typedef for the function pointer map and associated pair
	//
	typedef gmBindHashMap< gmBindPropertyFunctionPair >									T_FPMAP;
	//

	struct gmBindOperatorMap
	{
		// O_ADD,              // op1, op2                  (tos is a_operands + 2)
		gmBindOperatorFP	opAdd;
		// O_SUB,              // op1, op2
		gmBindOperatorFP	opSub;
		// O_MUL,              // op1, op2
		gmBindOperatorFP	opMul;
		// O_DIV,              // op1, op2
		gmBindOperatorFP	opDiv;
		// O_NEG,              // op1
		gmBindOperatorFP	opNeg;			
		// O_REM,              // op1, op2
		gmBindOperatorFP	opRem;
		// O_BIT_OR,           // op1, op2
		gmBindOperatorFP	opBitOr;
		// O_BIT_XOR,          // op1, op2
		gmBindOperatorFP	opBitXOr;
		// O_BIT_AND,          // op1, op2
		gmBindOperatorFP	opBitAnd;
		// O_BIT_SHIFTLEFT,    // op1, op2 (shift)
		gmBindOperatorFP	opBitShiftLeft;
		// O_BIT_SHIFTRIGHT,   // op1, op2 (shift)
		gmBindOperatorFP	opBitShiftRight;
		// O_BIT_INV,          // op1
		gmBindOperatorFP	opBitInv;
		// O_LT,               // op1, op2
		gmBindOperatorFP	opLessThan;
		// O_GT,               // op1, op2
		gmBindOperatorFP	opGreaterThan;
		// O_LTE,              // op1, op2
		gmBindOperatorFP	opLessThanOrEqual;
		// O_GTE,              // op1, op2
		gmBindOperatorFP	opGreaterThanOrEqual;
		// O_EQ,               // op1, op2
		gmBindOperatorFP	opIsEqual;
		// O_NEQ,              // op1, op2
		gmBindOperatorFP	opIsNotEqual;
		// O_POS,              // op1
		gmBindOperatorFP	opPos;
		// O_NOT,              // op1
		gmBindOperatorFP	opNot;
		// O_GETIND,           // object, index 
		gmBindOperatorFP	opGetIndex;	
		// O_SETIND,           // object, index, value
		gmBindOperatorFP	opSetIndex;
	};

	//
	//

protected:
	//
	////////////////////////////////////////////////
	// Variable:	m_gmType
	//
	// Holds the gmType created
	static gmType		m_gmType;
	//////////////////////////////////////////////
	// Variable:	m_gmTypeLib
	//
	// Holds the typelibrary that is popluated
	// by the macro call GMBIND_INIT_TYPE
	//
	static gmFunctionEntry m_gmTypeLib[1];
	//	
	//////////////////////////////////////////////
	// Variable:	m_propertyFunctions
	//
	// Holds the function pointer map
	//
	static T_FPMAP	m_propertyFunctions;

	static gmBindOperatorMap m_operatorFunctions;

	static bool			m_extensible;

	////////////////////////////////////////////////////////
	// Method: registerProperty
	//
	// Function to register each property in the map. Called from
	// the overridden version of registerProperties as implemented
	// by the GMBIND_PROPERTY_MAP_BEGIN macro
	//
public:
	static void registerProperty(const char *name, gmBindPropertyFP getFunc, gmBindPropertyFP setFunc)
	{
		// Create a new map pair
		m_propertyFunctions.insert( name, gmBindPropertyFunctionPair( getFunc, setFunc ) );
	}
protected:

	////////////////////////////////////////////////////////
	// Method: registerAutoProperty
	//
	// Function to register each automatic property in the map. Called from
	// the overridden version of registerProperties as implemented
	// by the GMBIND_PROPERTY_MAP_BEGIN macro
	//
	enum { AUTO_PROP_READONLY = 1, };
	static void registerAutoProperty(const char *name, int a_propType, size_t a_propOffset, int a_propFlags)
	{
		gmBindPropertyFP getFunc = 0;
		gmBindPropertyFP setFunc = 0;

		switch ( a_propType )
		{
		case GM_FLOAT:
			getFunc = _autoprop_getFloat;
			if(!(a_propFlags & AUTO_PROP_READONLY))
				setFunc = _autoprop_setFloat;
			break;
		case GM_INT:
			getFunc = _autoprop_getInt;
			if(!(a_propFlags & AUTO_PROP_READONLY))
				setFunc = _autoprop_setInt;
			break;
		case GM_USER:
			getFunc = _autoprop_getUser;
			if(!(a_propFlags & AUTO_PROP_READONLY))
				setFunc = _autoprop_setUser;
			break;
		};

		m_propertyFunctions.insert( name, gmBindPropertyFunctionPair( getFunc, setFunc, a_propType, a_propOffset ) );
	}

	///////////////////////////////////////////////////
	// Method:	registerFunctions
	//
	// Default implementation of the function called when
	// the type is initialised. Does nothing and is overridden
	// through the use of the GMBIND_DECLARE_FUNCTIONS macro
	static void registerFunctions( gmMachine *a_machine )
	{
	}

	/////////////////////////////////////////////////
	// Method:	gmGetProperty
	//
	// Handles the call for OpGetDot property getting. The first thing it does is
	// look up the relevant entry in the registered functions table. If an entry is found
	// it will call the relevant user-specified function. If no entry is found or the
	// function pointer was NULL (allowing for readonly properties) the function
	// will call getProperty(). If the user provides no override to getProperty the function
	// will use the default template class function (fail). This allows custom get property
	// methods as how GameMonkey usually provides them
	//
	static bool gmGetProperty(gmBindUserObject *user, const char *prop, gmThread * a_thread, gmVariable * a_operands )
	{
		// Grab user's data pointer
		T_NATIVE *p = (T_NATIVE *)user->m_object;

		if(!p)
			return false;

		gmBindPropertyFunctionPair f = m_propertyFunctions.find( prop );
		// Call the function pointer
		gmBindPropertyFP func = f.getter;
		if (func == 0)
			return T_API::_getProperty(user, prop, a_thread, a_operands);
		// See if this is an auto property, if not return the regular function
		if (f.isAutoProp)
		{
			// Is an autoprop, set up the member pointer
			char *ptr = (char *)p;
			ptr += f.propOffset;
			// Access the auto prop member
			return func( (T_NATIVE*)ptr, a_thread, a_operands );
		}
		// Otherwise just call the user-defined method
		return func( p, a_thread, a_operands );
	}

	/////////////////////////////////////////////////
	// Method:	gmSetProperty
	//
	// Handles the call for OpSetDot property setting. The first thing it does is
	// look up the relevant entry in the registered functions table. If an entry is found
	// it will call the relevant user-specified function. If no entry is found or the
	// function pointer was NULL (allowing for writeonly properties) the function
	// will call setProperty(). If the user provides no override to setProperty the function
	// will use the default template class function (fail). This allows custom set property
	// methods as how GameMonkey usually provides them
	//
	static bool gmSetProperty(gmBindUserObject *user, const char *prop, gmThread * a_thread, gmVariable * a_operands )
	{
		// Grab user's data pointer
		T_NATIVE *p = (T_NATIVE *)user->m_object;

		gmBindPropertyFunctionPair f = m_propertyFunctions.find( prop );
		// Call the function pointer
		gmBindPropertyFP func = f.setter;
		if (func == 0)
			return T_API::_setProperty(user, prop, a_thread, a_operands);
		// See if this is an auto property, if not return the regualr function
		if (f.isAutoProp)
		{
			// Is an autoprop, set up the member pointer
			char *ptr = (char *)p;
			ptr += f.propOffset;
			// Access the auto prop member
			return func( (T_NATIVE*)ptr, a_thread, a_operands );
		}
		// Otherwise just call the user-defined method
		return func( p, a_thread, a_operands );
	}

	////////////////////////////////
	// Method:		gmGetObjectProperty
	// 
	// Will retrieve a property (as a gmVariable) of a global user-bound object
	//
	static GM_INLINE gmVariable gmGetObjectProperty( gmMachine * a_machine, const char *objName, const char *propName )
	{
		gmTableObject *table = a_machine->GetGlobals();
		gmVariable	var = table->Get(a_machine, objName);
		if (var.m_type != GetType())
		{
			return gmVariable::s_null;
		}
		// Look it up in our table
		gmUserObject *user = (gmUserObject *)var.m_value.m_ref;
		gmBindUserObject *gmbUser = (gmBindUserObject *)user->m_user;
		// Find the property
		return gmbUser->m_table->Get( a_machine, propName );
	}

	static GM_INLINE gmVariable gmGetObjectProperty( gmMachine * a_machine, gmUserObject *a_object, const char *propName )
	{
		// Look it up in our table
		gmBindUserObject *gmbUser = (gmBindUserObject *)a_object->m_user;
		// Find the property
		return gmbUser->m_table->Get( a_machine, propName );
	}

	////////////////////////////////
	// Method:		gmGetUserBoundObject
	// 
	// Will look up the user bound object in the global machine table
	//
	static GM_INLINE gmBindUserObject *gmGetUserBoundObject( gmMachine *a_machine, const char *objName )
	{
		gmTableObject *table = a_machine->GetGlobals();
		// Get out names user object
		gmVariable	var = table->Get(a_machine, objName);
		if (var.m_type != GetType())
			return 0;
		// Look it up in our table
		gmUserObject *user = (gmUserObject *)var.m_value.m_ref;
		gmBindUserObject *gmbUser = (gmBindUserObject *)user->m_user;
		return gmbUser;
	}

	static GM_INLINE gmBindUserObject *gmGetUserBoundObject( gmMachine *a_machine, gmUserObject *a_object )
    {
		if (a_object->GetType() != GetType())
			return 0;
		// Look it up in our table
		gmBindUserObject *gmbUser = static_cast<gmBindUserObject*>(a_object->m_user);
		return gmbUser;
	}

	///////////////////////////////////////////////////////
	// Method:		getProperty
	//
	// Public user get property handler for the DotGet op
	// User can override this to handle the properties as they normally would
	// in GameMonkey, however the property registration interface is the preferred
	// method of handling properties
	//
	// Default implementation fails
	//
	// Parameters:
	//
	// p - A pointer to an instance of the native class that is used by the script
	// prop - The name of the property that is being called
	// a_thread - The gmThread making the call
	// a_operands - The operands of the get method
	//
	static GM_INLINE bool _getProperty(gmBindUserObject *user, const char *prop, gmThread * a_thread, gmVariable * a_operands )
	{
		// If not extensible, just return fail
		if (!m_extensible)
			return false;

		gmMachine *a_machine = a_thread->GetMachine();
		gmVariable var = user->m_table->Get( a_machine, prop );

		if (var.m_type == GM_NULL)
			return false;

		a_operands[0] = var;
		return true;
	}
	///////////////////////////////////////////////////////
	// Method:		setProperty
	//
	// Public user get property handler for the DotSet op
	// User can override this to handle the properties as they normally would
	// in GameMonkey, however the property registration interface is the preferred
	// method of handling properties
	//
	// Default implementation fails
	//
	// Parameters:
	//
	// p - A pointer to an instance of the native class that is used by the script
	// prop - The name of the property that is being called
	// a_thread - The gmThread making the call
	// a_operands - The operands of the set method
	//
	static GM_INLINE bool _setProperty(gmBindUserObject *user, const char *prop, gmThread * a_thread, gmVariable * a_operands )
	{
		// If not extensible, don't create any properies
		if (!m_extensible)
			return false;

		// If the property doesn't exist on the object (user defined)
		// Create a new one
		gmMachine *a_machine = a_thread->GetMachine();
		user->m_table->Set(a_machine, prop, a_operands[1]);
		return true;
	}

	static bool _autoprop_setInt( T_NATIVE *a_native, gmThread * a_thread, gmVariable * a_operands )
	{
		(*(int *)a_native) = a_operands[1].m_value.m_int;
		return true;
	}

	static bool _autoprop_getInt( T_NATIVE *a_native, gmThread * a_thread, gmVariable * a_operands )
	{
		a_operands[0].SetInt( (*(int *)a_native) );
		return true;
	}

	static bool _autoprop_setUser( T_NATIVE *a_native, gmThread * a_thread, gmVariable * a_operands )
	{
		a_native = (T_NATIVE*)a_operands[1].m_value.m_ref;
		return true;
	}

	static bool _autoprop_getUser( T_NATIVE *a_native, gmThread * a_thread, gmVariable * a_operands )
	{
		a_operands[0].SetUser( (gmUserObject*)a_native );
		return true;
	}
	
	static bool _autoprop_getFloat( T_NATIVE *a_native, gmThread * a_thread, gmVariable * a_operands )
	{
		a_operands[0].SetFloat( (*(float *)a_native) );
		return true;
	}

	static bool _autoprop_setFloat( T_NATIVE *a_native, gmThread * a_thread, gmVariable * a_operands )
	{
		switch( a_operands[1].m_type )
		{
		case GM_FLOAT:
			(*(float *)a_native) = a_operands[1].m_value.m_float;
			return true;
		case GM_INT:
			(*(float *)a_native) = (float)a_operands[1].m_value.m_int;
			return true;
		};

		return false;
	}

	///////////////////////////////////////////////////////
	// Method:		registerProperties
	//
	// Default register properties method that is called when the type
	// is initialised. This is overridden when the user specifies the
	// GMBIND_DECLARE_PROPERTIES macro
	//
public:
	static void registerProperties()
	{
		return;
	}

	static void registerOperators()
	{
		return;
	}

	static void RegisterAutoProperty(const char *name, int a_propType, size_t a_propOffset, int a_propFlags)
	{
		registerAutoProperty(name, a_propType, a_propOffset, a_propFlags);
	}

private:

	static gmBindUserObject* _allocObject( gmMachine *a_machine, T_NATIVE *a_object, bool a_native )
	{
		DisableGCInScope gcEn(a_machine);

		gmBindUserObject *gmb = (gmBindUserObject*)m_gmUserObjects.Alloc();
		gmb->m_table = a_machine->AllocTableObject();
		gmb->m_native = a_native;
		gmb->m_object = a_object;
		a_machine->AdjustKnownMemoryUsed( sizeof( gmBindUserObject ) );
		a_machine->AdjustKnownMemoryUsed( sizeof( T_NATIVE ) );
		return gmb;
	}


	//////////////////////////////////////////////////////
	// Method:		gmfConstructor
	//
	// The constructor called by GameMonkey when a new object
	// is created. The first action this will do is call the
	// user-defined constructor function (default constructor returns
	// a fail and produces a GM exception).
	//
	// After the constructor is called, the object is stored in the
	// GM thread and the memory used by the object is added
	//
	static int GM_CDECL gmfConstructor(gmThread * a_thread)
	{
		// Create native object via optional constructor
		T_NATIVE *native = T_API::Constructor( a_thread );

		if (!native)
		{
			// Return with an exception if failed to allocate
			return GM_EXCEPTION;
		}

		gmMachine *a_machine = a_thread->GetMachine();

		gmBindUserObject *gmbUser = _allocObject( a_machine, native, false );//new gmBindUserObject;

		// Store in thread
		a_thread->PushNewUser( gmbUser, GetType());
		//
		return GM_OK;
	};

	///////////////////////////////////////////////////////
	// Method:		gmfDestruct
	//
	// Destructor called by GameMonkey when an object is destroyed or
	// garbage collected. This method will free the object from the gmMachine
	// and call the appropriate user-destructor. The default destructor here will
	// automatically delete the native object but there may be times where an override
	// is needed
	//
	static void GM_CDECL gmfDestruct(gmMachine * a_machine, gmUserObject* a_object)
	{
		GM_ASSERT(a_object->m_userType == m_gmType);
		gmfFree(a_machine);

		gmBindUserObject *gmbUser = (gmBindUserObject *)a_object->m_user;
		if ( gmbUser->m_native == false )
		{
			// Not a native object, we own it - kill
			T_NATIVE *p = (T_NATIVE *)gmbUser->m_object;
			T_API::Destructor( p );
			gmbUser->m_object = NULL;
		}

		// Wipe data to ensure noone misuses it ;)
		gmbUser->m_table = 0;
		gmbUser->m_object = 0;

		// Finally, delete this user object holder
		m_gmUserObjects.Free( gmbUser );
	}


	///////////////////////////////////////////////////////
	// Method:		gmfTrace
	//
	// Function called by the Garbage Collector. Must trace the table and the 
	// user object as in addition to this object
	//
	static bool gmfTrace( gmMachine * a_machine, gmUserObject* a_object, gmGarbageCollector* a_gc, const int a_workLeftToGo, int& a_workDone )
	{
		// Make sure this is the correct type
		GM_ASSERT(a_object->m_userType == m_gmType);

		// Need to trace the user object
		gmBindUserObject *gmb = (gmBindUserObject *)a_object->m_user;

		// Trace the regular user object - 0.9.4b - Shouldn't trace the user's native pointer
		//gmObject* object = GM_MOBJECT(a_machine, gmb->m_object );
		//a_gc->GetNextObject( object );

		// trace the table for the properties
		a_gc->GetNextObject( gmb->m_table );

		// finally mark 'me' as done
		a_workDone += 2;
		return true;
	}

	// Free memory for one object
	static GM_INLINE void gmfFree(gmMachine* a_machine)
	{
		a_machine->AdjustKnownMemoryUsed(-(int)sizeof(T_NATIVE));
		a_machine->AdjustKnownMemoryUsed(-(int)sizeof(gmBindUserObject));
	}

	/////////////////////////////////////////////////
	// Method:		gmOpGetDot
	//
	// Handles the OpGetDot GameMonkey operator. This will delegate the
	// call out to the appropriate user get function
	//
	static int GM_CDECL gmOpGetDot(gmThread * a_thread, gmVariable * a_operands)
	{
		// Ensure the operation is being performed on our type
		GM_ASSERT(a_operands[0].m_type == m_gmType);
		gmBindUserObject *gmbUser = GetUserBoundObject(a_thread->GetMachine(),a_operands[0]);
		// ensure the property being 'got' is defined as a string
		GM_ASSERT(a_operands[1].IsString());
		gmStringObject* stringObj = a_operands[1].GetStringObjectSafe();
		const char* cstr = stringObj->GetString();
		// Call function to delegate the get call
		if ( !gmbUser->m_object || !gmGetProperty( gmbUser, cstr, a_thread, a_operands ) )
		{
			a_operands[0].Nullify();
			return GM_OK;
		}
		return GM_OK;
	}
	/////////////////////////////////////////////////
	// Method:		gmOpSetDot
	//
	// Handles the OpSetDot GameMonkey operator. This will delegate the
	// call out to the appropriate user set function
	//
	static int GM_CDECL gmOpSetDot(gmThread * a_thread, gmVariable * a_operands)
	{
		// ensure the type being set is our type
		GM_ASSERT(a_operands[0].m_type == m_gmType);		
		gmBindUserObject *gmbUser = GetUserBoundObject(a_thread->GetMachine(),a_operands[0]);
		// Ensure the value set is a string (eg: property)
		GM_ASSERT(a_operands[2].IsString());
		gmStringObject* stringObj = a_operands[2].GetStringObjectSafe();
		const char* cstr = stringObj->GetString();
		// Call the set property handler to delegate the property setter
		if ( !gmbUser->m_object || !gmSetProperty( gmbUser, cstr, a_thread, a_operands ) )
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
		return GM_OK;
	}

	static int GM_CDECL gmOpGetInd(gmThread * a_thread, gmVariable * a_operands)
	{
		if(m_operatorFunctions.opGetIndex)
		{
			m_operatorFunctions.opGetIndex( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpSetInd(gmThread * a_thread, gmVariable * a_operands)
	{
		if(m_operatorFunctions.opSetIndex)
		{
			m_operatorFunctions.opSetIndex( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpNeg(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opNeg )
		{
            m_operatorFunctions.opNeg( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpMul(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opMul)
		{
			m_operatorFunctions.opMul( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpDiv(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opDiv)
		{
			m_operatorFunctions.opDiv( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpSub(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opSub)
		{
            m_operatorFunctions.opSub( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpAdd(gmThread *a_thread, gmVariable *a_operands)
	{
		// Special case for string values. Append a string representation of us.
		//if(a_operands[0].m_type == GM_STRING)
		//{
		//	// TODO: concat the pOp and the buffer string
		//	gmStringObject *pOp = reinterpret_cast<gmStringObject*>(a_operands[0].m_value.m_ref);
		//	
		//	gmMachine *pMachine = GM_THREAD_ARG->GetMachine();
		//	const int iAsStringBuffSize = 256;
		//	char bufAsString[iAsStringBuffSize] = {0};
		//	T_API::AsString(reinterpret_cast<gmUserObject*>(a_operands[1].m_value.m_ref), bufAsString, iAsStringBuffSize);

		//	// Build the final string
		//	const int iBufferSize = 1024;
		//	char buffer[iBufferSize] = {0};
		//	_gmsnprintf(buffer, iBufferSize, "%s%s", pOp ? pOp->GetString() : "", bufAsString);

		//	gmStringObject *pMyString = pMachine->AllocStringObject(buffer);
		//	a_operands[0].SetString(pMyString);
		//	return;
		//}

		if (m_operatorFunctions.opAdd)
		{
            m_operatorFunctions.opAdd( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpRem(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opRem)
		{
            m_operatorFunctions.opRem( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpBitOr(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opBitOr)
		{
           m_operatorFunctions.opBitOr( a_thread, a_operands );
		   return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpBitXOr(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opBitXOr)
		{
            m_operatorFunctions.opBitXOr( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpBitAnd(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opBitAnd)
		{
            m_operatorFunctions.opBitAnd( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpBitShiftL(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opBitShiftLeft)
		{
            m_operatorFunctions.opBitShiftLeft( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpBitShiftR(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opBitShiftRight)
		{
            m_operatorFunctions.opBitShiftRight( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpBitInv(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opBitInv)
		{
            m_operatorFunctions.opBitInv( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpLT(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opLessThan)
		{
            m_operatorFunctions.opLessThan( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpGT(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opGreaterThan)
		{
			m_operatorFunctions.opGreaterThan( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}		
	}

	static int GM_CDECL gmOpLTE(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opLessThanOrEqual)
		{
            m_operatorFunctions.opLessThanOrEqual( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpGTE(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opGreaterThanOrEqual)
		{
			m_operatorFunctions.opGreaterThanOrEqual( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpIsEq(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opIsEqual)
		{
            m_operatorFunctions.opIsEqual( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpIsNotEq(gmThread * a_thread, gmVariable * a_operands)
	{		
		if (m_operatorFunctions.opIsNotEqual)
		{
            m_operatorFunctions.opIsNotEqual( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpPos(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opPos)
		{
            m_operatorFunctions.opPos( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	static int GM_CDECL gmOpNot(gmThread * a_thread, gmVariable * a_operands)
	{
		if (m_operatorFunctions.opNot)
		{
            m_operatorFunctions.opNot( a_thread, a_operands );
			return GM_OK;
		}
		else
		{
			a_operands[0].Nullify();
			return GM_EXCEPTION;
		}
	}

	/////////////////////////////////////////////////////
	// Variable:	m_gmTypeName
	//
	// Holds onto the typename that is registered with GM
	//
	static const char *m_gmTypeName;

	static gmMemFixed m_gmUserObjects;

};  // END gmBind


/////////////////////////////////////////////
// Template static property section
//
// Sets the default type to 0
template <class T_NATIVE, class T_API>
gmType gmBind<T_NATIVE, T_API>::m_gmType = 0;

template <class T_NATIVE, class T_API>
bool gmBind<T_NATIVE, T_API>::m_extensible = true;

template < typename T_NATIVE, typename T_API >
typename gmBind< T_NATIVE, T_API >::T_FPMAP	gmBind< T_NATIVE, T_API >::m_propertyFunctions;

template < typename T_NATIVE, typename T_API >
typename gmBind< T_NATIVE, T_API >::gmBindOperatorMap gmBind< T_NATIVE, T_API >::m_operatorFunctions;

template < typename T_NATIVE, typename T_API >
gmMemFixed gmBind< T_NATIVE, T_API >::m_gmUserObjects( 8+2*sizeof(void*), 1024 );


//////////////////////////////////////////////
// Macro section
//
// Method:	GMBIND_INIT_TYPE
//
// Sets the name of the user's registered type and initialises the gmTypeLib
// to call the relevant user/default constructor as specified in the template
//
// Parameters:
//
// api - The name of the Proxy API class
// name - Name of the type to expose to GameMonkey
//
#define GMBIND_INIT_TYPE(api, name)			\
	template<>     \
	const char * api::T_OBJ::m_gmTypeName = name;    \
	\
	template<> \
	gmFunctionEntry api::T_OBJ::m_gmTypeLib[1] = {   \
{ api::T_OBJ::m_gmTypeName, api::gmfConstructor } };	

//////////////////////////////////////////////////
// Method:	GMBIND_DECLARE_FUNCTIONS
//
// Used to declare that the user has specified a function map
// Overrides the default empty method. Should be used in the
// public section of the proxy API class
//
#define GMBIND_DECLARE_FUNCTIONS()		\
	static void registerFunctions( gmMachine *a_machine )	
///////////////////////////////////////////////////
// Method:	GMBIND_FUNCTION_MAP_BEGIN
//
// Macro to implement the registerFunctions() method of the template
// class. This section basically constructs a function that is called 
// when the class is declared to GameMonkey
//
// Parameters:
//
// api - The name of the Proxy API class
//
#define GMBIND_FUNCTION_MAP_BEGIN(api)								\
	void api::registerFunctions( gmMachine	*a_machine )			 \
{
//
///////////////////////////////////////////////////
// Method:	GMBIND_FUNCTION
//
// Declares a function to the gmMachine. Takes the function name to expose
// and a regular gmFunctionObject callback function pointer
//
// Parameters:
//
// name - Name of the function to declare to GameMonkey
// func - The GameMonkey function callback to use
//
// Function specification:
//
// int functionCallback(gmThread * a_thread)
//
#define GMBIND_FUNCTION( name, func )			\
	a_machine->RegisterTypeVariable( GetType(), name, gmVariable(GM_FUNCTION, (gmptr) a_machine->AllocFunctionObject( func ) ));
//
///////////////////////////////////////////////////
// Method:	GMBIND_FUNCTION_MAP_END
//
// Closes the function map macro
//
#define GMBIND_FUNCTION_MAP_END()				\
};


/////////////////////////////////////////////////////
// Method:	GMBIND_DECLARE_PROPERTIES
//
// Used to declare that the user has a property map.
// Should be used in the public section of the proxy API
// class. Overrides the default implementation
//
#define GMBIND_DECLARE_PROPERTIES()			\
	static void registerProperties()
//
///////////////////////////////////////////////////////
// Method:	GMBIND_PROPERTY_MAP_BEGIN
//
// Implements an override function of the registerProperties()
// template method that is called from the class initialiser
//
// Parameters:
//
// api - The name of the Proxy API class
//
#define GMBIND_PROPERTY_MAP_BEGIN(api)		\
	void api::registerProperties()				\
{										
//
//////////////////////////////////////////////////////
// Method:	GMBIND_PROPERTY
//
// Used within the property map to register each property in turn
//
// Parameters:
//
// name - The name of the property to expose
// getFunc - The function pointer of the user-specified get property to call
// setFunc - The function pointer of the user-specified set property call
//
// Callback specification:
//
// bool propertyCallbackFunction( NativeUserType *p, gmThread * a_thread, gmVariable * a_operands )
//
#define GMBIND_PROPERTY( name, getFunc, setFunc )		\
	registerProperty( name, getFunc, setFunc );
//		
#define GMBIND_AUTOPROPERTY( name, propType, propNative, propFlags )		\
	registerAutoProperty( name, propType, offsetof( T_NATIVETYPE, propNative ), propFlags );
//
///////////////////////////////////////////////////////////
// Method:	GMBIND_PROPERTY_MAP_END
//
// Used to close the property map
//
#define GMBIND_PROPERTY_MAP_END()				\
};


/////////////////////////////////////////////////////
// Method:	GMBIND_DECLARE_OPERATORS
//
// Used to declare that the user has an operator map.
// Should be used in the public section of the proxy API
// class. Overrides the default implementation
//
#define GMBIND_DECLARE_OPERATORS()			\
	static void registerOperators();
//
///////////////////////////////////////////////////////
// Method:	GMBIND_OPERATOR_MAP_BEGIN
//
// Implements an override function of the registerOperators()
// template method that is called from the class initialiser
//
// Parameters:
//
// api - The name of the Proxy API class
//
#define GMBIND_OPERATOR_MAP_BEGIN(api)		\
	void api::registerOperators()				\
{
//
//////////////////////////////////////////////////////
// Method:	GMBIND_OPERATOR_XXX
//
// Used within the operator map to register a specific operaor
//
// Parameters:
//
// operatorFun - The function pointer of the user-specified operator call
//
// Callback specification:
//
// bool operatorCallbackFunction( NativeUserType *a_native, gmThread * a_thread, gmVariable * a_operands )
//

// O_ADD operator
//
#define GMBIND_OPERATOR_ADD( operatorFunc ) \
	m_operatorFunctions.opAdd = operatorFunc;
//
// O_SUB operator
//
#define GMBIND_OPERATOR_SUB( operatorFunc ) \
	m_operatorFunctions.opSub = operatorFunc;
//
// O_NEG operator
//
#define GMBIND_OPERATOR_NEG( operatorFunc ) \
	m_operatorFunctions.opNeg = operatorFunc;
//
// O_MUL operator
//
#define GMBIND_OPERATOR_MUL( operatorFunc ) \
	m_operatorFunctions.opMul = operatorFunc;
//
// O_DIV operator
//
#define GMBIND_OPERATOR_DIV( operatorFunc ) \
	m_operatorFunctions.opDiv = operatorFunc;
//
// O_REM operator
//
#define GMBIND_OPERATOR_REM( operatorFunc ) \
	m_operatorFunctions.opRem = operatorFunc;
//
// O_BIT_OR operator
//
#define GMBIND_OPERATOR_BIT_OR( operatorFunc ) \
	m_operatorFunctions.opBitOr = operatorFunc;
//
// O_BIT_XOR,          // op1, op2
//
#define GMBIND_OPERATOR_BIT_XOR( operatorFunc ) \
	m_operatorFunctions.opBitXOr = operatorFunc;
//
// O_BIT_AND,          // op1, op2
//
#define GMBIND_OPERATOR_BIT_AND( operatorFunc ) \
	m_operatorFunctions.opBitAnd = operatorFunc;
//
// O_BIT_SHIFTLEFT,    // op1, op2 (shift)
//
#define GMBIND_OPERATOR_BIT_SHIFTLEFT( operatorFunc ) \
	m_operatorFunctions.opBitShiftLeft = operatorFunc;
//
// O_BIT_SHIFTRIGHT,   // op1, op2 (shift)
//
#define GMBIND_OPERATOR_BIT_SHIFTRIGHT( operatorFunc ) \
	m_operatorFunctions.opBitShiftRight = operatorFunc;
//
// O_BIT_INV,          // op1
//
#define GMBIND_OPERATOR_BIT_INV( operatorFunc ) \
	m_operatorFunctions.opBitInv = operatorFunc;
//
// O_LT,               // op1, op2
//
#define GMBIND_OPERATOR_LT( operatorFunc ) \
	m_operatorFunctions.opLessThan = operatorFunc;
//
// O_GT,               // op1, op2
//
#define GMBIND_OPERATOR_GT( operatorFunc ) \
	m_operatorFunctions.opGreaterThan = operatorFunc;
//
// O_LTE,              // op1, op2
//
#define GMBIND_OPERATOR_LTE( operatorFunc ) \
	m_operatorFunctions.opLessThanOrEqual = operatorFunc;
//
// O_GTE,              // op1, op2
//
#define GMBIND_OPERATOR_GTE( operatorFunc ) \
	m_operatorFunctions.opGreaterThanOrEqual = operatorFunc;
//
// O_EQ,               // op1, op2
//
#define GMBIND_OPERATOR_EQ( operatorFunc ) \
	m_operatorFunctions.opIsEqual = operatorFunc;
//
// O_NEQ,              // op1, op2
//
#define GMBIND_OPERATOR_NEQ( operatorFunc ) \
	m_operatorFunctions.opIsNotEqual = operatorFunc;
//
// O_POS,              // op1
//
#define GMBIND_OPERATOR_POS( operatorFunc ) \
	m_operatorFunctions.opPos = operatorFunc;
//
// O_NOT,              // op1
//
#define GMBIND_OPERATOR_NOT( operatorFunc ) \
	m_operatorFunctions.opNot = operatorFunc;

//
// Index operators - Get and Set are combined in one macro
//
#define GMBIND_OPERATOR_INDEX( getFunc, setFunc )		\
	m_operatorFunctions.opGetIndex = getFunc;		\
	m_operatorFunctions.opSetIndex = setFunc;
//
///////////////////////////////////////////////////////////
// Method:	GMBIND_OPERATOR_MAP_END
//
// Used to close the operator map
//
#define GMBIND_OPERATOR_MAP_END()				\
};


/// A baseclass for C++ classes to inherit from to automatically tell gmBind to
/// unhook them from GM
template <class T_API>
class gmBindBase
{
public:
    gmBindBase() : gmb_UserObject(0), gmb_Machine(0) { }
    virtual ~gmBindBase()
    {
        gmbReleaseObject();
    }

    void gmbInitObject( gmMachine *a_machine, gmUserObject *a_user, const char *a_global_name )
    {
    	gmbSetMachine(a_machine); gmbSetUserObject( a_user ); gmbSetGlobalName( a_global_name );
    }
    
    void gmbReleaseObject()
    {
    	if (gmb_GlobalName.length() > 0 && gmb_Machine != 0)
        {
            T_API::DestroyObject( gmb_Machine, gmb_GlobalName.c_str() );
        }
    }

    void gmbSetUserObject( gmUserObject *a_user ) { gmb_UserObject = a_user; }
    gmUserObject *gmb_GetUserObject( ) const { return gmb_UserObject; }
    void gmbSetMachine( gmMachine *a_machine ) { gmb_Machine = a_machine; }
    
    void gmbSetGlobalName( const char *a_name ) { gmb_GlobalName = a_name; }
    const char *gmbGetGlobalName() const { return gmb_GlobalName.c_str(); }
    
protected:

    gmUserObject *gmb_UserObject;
    gmMachine    *gmb_Machine;
	std::string   gmb_GlobalName;
};

#define GM_GMBIND_PARAM(OBJECT, TYPE, VAR, PARAM, DEFAULT) OBJECT VAR = (DEFAULT); \
	if(GM_THREAD_ARG->ParamType((PARAM)) == (TYPE::GetType())) { VAR = (OBJECT) TYPE::GetNative(reinterpret_cast<gmUserObject*>(GM_THREAD_ARG->Param(PARAM).m_value.m_ref)); }

#define GM_CHECK_GMBIND_PARAM(OBJECT, TYPE, VAR, PARAM) \
	if(GM_THREAD_ARG->ParamType((PARAM)) != (TYPE::GetType())) \
	{ char buffer[256]; GM_EXCEPTION_MSG("expecting param %d as user type %s, got %s", (PARAM), (TYPE::GetTypeName()), GM_THREAD_ARG->Param((PARAM)).AsStringWithType(GM_THREAD_ARG->GetMachine(), buffer, 256)); return GM_EXCEPTION; } \
	OBJECT VAR = (OBJECT)TYPE::GetNative(reinterpret_cast<gmUserObject*>(GM_THREAD_ARG->Param(PARAM).m_value.m_ref));

#endif   // __GAMEMONKEY_BINDING_TEMPLATE_GMBIND_H__
