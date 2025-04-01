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

#ifndef __TYPEINFOGEN_H__
#define __TYPEINFOGEN_H__

/*
===================================================================================

	Type Info Generator

	- template classes are commented out (different instantiations are not identified)
	- bit fields are commented out (cannot get the address of bit fields)
	- multiple inheritance is not supported (only tracks a single super type)

===================================================================================
*/

class idConstantInfo {
public:
	idStr						name;
	idStr						type;
	idStr						value;
};

class idEnumValueInfo {
public:
	idStr						name;
	int							value;
};

class idEnumTypeInfo {
public:
	idStr						typeName;
	idStr						scope;
	bool						unnamed;
	bool						isTemplate;
	idList<idEnumValueInfo>		values;
};

class idClassVariableInfo {
public:
	idStr						name;
	idStr						type;
	int							bits;
	bool						reference;
};

class idClassTypeInfo {
public:
	idStr						typeName;
	idStr						superType;
	idStr						scope;
	bool						unnamed;
	bool						isTemplate;
	idList<idClassVariableInfo>	variables;
};

class idTypeInfoGen {
public:
								idTypeInfoGen( void );
								~idTypeInfoGen( void );

	void						AddDefine( const char *define );
	void						CreateTypeInfo( const char *path );
	void						WriteTypeInfo( const char *fileName ) const;

private:
	idStrList					defines;

	idList<idConstantInfo *>	constants;
	idList<idEnumTypeInfo *>	enums;
	idList<idClassTypeInfo *>	classes;

	int							numTemplates;
	int							maxInheritance;
	idStr						maxInheritanceClass;

	int							GetInheritance( const char *typeName ) const;
	int							EvaluateIntegerString( const idStr &string );
	float						EvaluateFloatString( const idStr &string );
	idConstantInfo *			FindConstant( const char *name );
	int							GetIntegerConstant( const char *scope, const char *name, idParser &src );
	float						GetFloatConstant( const char *scope, const char *name, idParser &src );
	int							ParseArraySize( const char *scope, idParser &src );
	void						ParseConstantValue( const char *scope, idParser &src, idStr &value );
	idEnumTypeInfo *			ParseEnumType( const char *scope, bool isTemplate, bool typeDef, idParser &src );
	idClassTypeInfo *			ParseClassType( const char *scope, const char *templateArgs, bool isTemplate, bool typeDef, idParser &src );
	void						ParseScope( const char *scope, bool isTemplate, idParser &src, idClassTypeInfo *typeInfo );
};

#endif /* !__TYPEINFOGEN_H__ */
