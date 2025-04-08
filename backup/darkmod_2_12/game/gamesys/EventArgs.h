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
#ifndef EVENT_ARGS_H_
#define EVENT_ARGS_H_

struct EventArg
{
	char type;
	const char* name;
	const char* desc;
};

/**
 * greebo: An object encapsulating the format specifier for idEventDef, 
 * including the event argument order, their names and description.
 *
 * idEventDef allow for up to 8 arguments, and so are the constructors of this class.
 */
class EventArgs {
private:
	idStaticList<EventArg, 8> args;
public:

	const EventArg &operator[](int index) const;
	int size() const;
	bool empty() const;

	EventArgs();															// 0 args
	
	EventArgs(char argType1, const char* argName1, const char* argDesc1);	// 1 arg

	EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2);	// 2 args

	EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3);	// 3 args

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 4 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4);

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 5 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5);

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 6 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6);	

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 7 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6,
			  char argType7, const char* argName7, const char* argDesc7);

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 8 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6,
			  char argType7, const char* argName7, const char* argDesc7,
			  char argType8, const char* argName8, const char* argDesc8);
};

inline int EventArgs::size() const {
	return args.Num();
}
inline bool EventArgs::empty() const {
	return args.Num() == 0;
}
inline const EventArg &EventArgs::operator[](int index) const {
	return args[index];
}

inline EventArgs::EventArgs()
{
	args.SetNum(0);
}
	
inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1)
{
	args.SetNum(1);
	args[0].type = argType1; args[0].name = argName1; args[0].desc = argDesc1;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2)
{
	args.SetNum(2);
	args[0].type = argType1; args[0].name = argName1; args[0].desc = argDesc1;
	args[1].type = argType2; args[1].name = argName2; args[1].desc = argDesc2;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3)
{
	args.SetNum(3);
	args[0].type = argType1; args[0].name = argName1; args[0].desc = argDesc1;
	args[1].type = argType2; args[1].name = argName2; args[1].desc = argDesc2;
	args[2].type = argType3; args[2].name = argName3; args[2].desc = argDesc3;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4)
{
	args.SetNum(4);
	args[0].type = argType1; args[0].name = argName1; args[0].desc = argDesc1;
	args[1].type = argType2; args[1].name = argName2; args[1].desc = argDesc2;
	args[2].type = argType3; args[2].name = argName3; args[2].desc = argDesc3;
	args[3].type = argType4; args[3].name = argName4; args[3].desc = argDesc4;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 5 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5)
{
	args.SetNum(5);
	args[0].type = argType1; args[0].name = argName1; args[0].desc = argDesc1;
	args[1].type = argType2; args[1].name = argName2; args[1].desc = argDesc2;
	args[2].type = argType3; args[2].name = argName3; args[2].desc = argDesc3;
	args[3].type = argType4; args[3].name = argName4; args[3].desc = argDesc4;
	args[4].type = argType5; args[4].name = argName5; args[4].desc = argDesc5;
}	

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 6 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6)
{
	args.SetNum(6);
	args[0].type = argType1; args[0].name = argName1; args[0].desc = argDesc1;
	args[1].type = argType2; args[1].name = argName2; args[1].desc = argDesc2;
	args[2].type = argType3; args[2].name = argName3; args[2].desc = argDesc3;
	args[3].type = argType4; args[3].name = argName4; args[3].desc = argDesc4;
	args[4].type = argType5; args[4].name = argName5; args[4].desc = argDesc5;
	args[5].type = argType6; args[5].name = argName6; args[5].desc = argDesc6;
}	

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 7 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6,
			  char argType7, const char* argName7, const char* argDesc7)
{
	args.SetNum(7);
	args[0].type = argType1; args[0].name = argName1; args[0].desc = argDesc1;
	args[1].type = argType2; args[1].name = argName2; args[1].desc = argDesc2;
	args[2].type = argType3; args[2].name = argName3; args[2].desc = argDesc3;
	args[3].type = argType4; args[3].name = argName4; args[3].desc = argDesc4;
	args[4].type = argType5; args[4].name = argName5; args[4].desc = argDesc5;
	args[5].type = argType6; args[5].name = argName6; args[5].desc = argDesc6;
	args[6].type = argType7; args[6].name = argName7; args[6].desc = argDesc7;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 8 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6,
			  char argType7, const char* argName7, const char* argDesc7,
			  char argType8, const char* argName8, const char* argDesc8)
{
	args.SetNum(8);
	args[0].type = argType1; args[0].name = argName1; args[0].desc = argDesc1;
	args[1].type = argType2; args[1].name = argName2; args[1].desc = argDesc2;
	args[2].type = argType3; args[2].name = argName3; args[2].desc = argDesc3;
	args[3].type = argType4; args[3].name = argName4; args[3].desc = argDesc4;
	args[4].type = argType5; args[4].name = argName5; args[4].desc = argDesc5;
	args[5].type = argType6; args[5].name = argName6; args[5].desc = argDesc6;
	args[6].type = argType7; args[6].name = argName7; args[6].desc = argDesc7;
	args[7].type = argType8; args[7].name = argName8; args[7].desc = argDesc8;
}

#endif
