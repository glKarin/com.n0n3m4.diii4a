// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACESCRIPT_H__
#define __GAME_GUIS_USERINTERFACESCRIPT_H__

class sdUIScript;

class sdUIScriptEvent {
private:
	typedef enum opCode_e {
		EO_NOOP,							// do nothing
		EO_ASSIGN_PROPERTY_EXPRESSION,		// property, property index, expression index
		EO_FUNCTION,						// expression index
		EO_CALL_EVENT,						// event handle
		EO_IF,								// if statement
		EO_BREAK,							// trigger a breakpoint
		EO_RETURN,							// stop execution		
		EO_NUM_OPS,
	} opCode_t;

public:
								sdUIScriptEvent( void );

	static const char*			ReadSection( char breakon, const char* p );
	static void					ReadExpression( idLexer* src, idStr& value, idStrList& list, const char* start = NULL, const char* terminator = ";", bool allowEmpty = false, int initialDepth = 1 );

	void						ParseEvent( idLexer* src, const sdUIEventInfo& info, sdUserInterfaceScope* scope );
	void						ParsePropertyExpression( idLexer* src, sdProperties::sdProperty* property, const char* propertyName, sdUserInterfaceScope* scope, sdUserInterfaceScope* propertyScope );
	static int					GetPropertyField( sdProperties::ePropertyType type, idLexer* src );

	int							EmitOpCode( opCode_t code, int parm1 = USHRT_MAX, int parm2 = USHRT_MAX, int parm3 = USHRT_MAX, sdUserInterfaceScope* assigmentScope = NULL, const char* opName = "" );

	bool						Run( sdUIScript* script, sdUserInterfaceScope* scope, int offset = 0, int num = -1 );

private:
	struct eventOp_t {
		unsigned short			op;
		unsigned short			parm1;
		unsigned short			parm2;
		unsigned short			parm3;
		sdUserInterfaceScope*	assigmentScope;
		//idStr					opName;
	};

	idList< eventOp_t > ops;
};

class sdUIScript {
public:
								sdUIScript( void );
								~sdUIScript( void );

	void						ParseEvent( idLexer* src, const sdUIEventInfo& info, sdUserInterfaceScope* scope );
	bool						RunEventHandle( sdUIEventHandle handle, sdUserInterfaceScope* scope );
	void						Clear( void );

private:
	idList< sdUIScriptEvent* >	events;
};

#endif // __GAME_GUIS_USERINTERFACESCRIPT_H__

