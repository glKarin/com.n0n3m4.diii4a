// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECL_TEMPLATE_H__
#define __DECL_TEMPLATE_H__

#include "../framework/DeclManager.h"

/*
===============================================================================

	sdDeclTemplate

===============================================================================
*/

class sdDeclTemplate : public idDecl {
public:
	virtual					~sdDeclTemplate() {}

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void ) const;

	static bool				ExpandTemplate(idStr &out, const char *text, int textLength);

private:
	struct parameter_t {
		idStr name;
		idStr defaultValue;
	};
	struct condition_t {
		idStr					name;
		idStr					op;
		idStr					value;
	};
	struct command_t {
		condition_t				cond;
		idStr					append;
	};
	enum section_t {
		SEC_TEXT = 1,
		SEC_COMMANDS = 2,
	};
	struct decl_t {
		section_t type;
		idStr text;
		idList<command_t>		commands;
	};

private:
	bool					ParseParameters(idParser &src);
	bool					ParseTextSource(idParser &src, idStr &out);
	bool					ParseText(idParser &src);
	bool					ParseCommands(idParser &src);
	bool					ParseCommand(idParser &src, command_t &cmd);
	bool					ParseCondition(idParser &src, condition_t &cond);
	void					Expand(idLexer &src, idStr &newDecl) const;
	bool					CheckCondition(const condition_t &cond, const idDict &parms) const;
	void					ExpandParameters(idLexer &src, idDict &newDecl) const;
	void					ExpandText(const idStr &text, idStr &newDecl, const idDict &parms) const;
	void					ExpandCommand(const command_t &cmd, idStr &newDecl, const idDict &parms) const;
	void					ExpandCommands(const idList<command_t> &cmds, idStr &newDecl, const idDict &parms) const;
	void					ExpandDecl(const decl_t &src, idStr &newDecl, const idDict &parms) const;
	static int				ReplacePlaceholder( int start, // include
		int end, // exclude
		idStr replaceStr, idStr &toStr);

private:
    idList<parameter_t>		parameters;
    idList<decl_t>			decls;
};

#endif /* !__DECL_TEMPLATE_H__ */

