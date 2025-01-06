/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop

#include "TypeInfoGen.h"

#define TYPE_INFO_GEN_VERSION		"1.1"

/*
================
idTypeInfoGen::idTypeInfoGen
================
*/
idTypeInfoGen::idTypeInfoGen( void )
{
}

/*
================
idTypeInfoGen::~idTypeInfoGen
================
*/
idTypeInfoGen::~idTypeInfoGen( void )
{
	constants.DeleteContents( true );
	enums.DeleteContents( true );
	classes.DeleteContents( true );
}

/*
================
idTypeInfoGen::GetInheritance
================
*/
int idTypeInfoGen::GetInheritance( const char* typeName ) const
{
	int i;

	for( i = 0; i < classes.Num(); i++ )
	{
		if( classes[i]->typeName.Cmp( typeName ) == 0 )
		{
			if( classes[i]->superType[0] != '\0' )
			{
				return 1 + GetInheritance( classes[i]->superType );
			}
			break;
		}
	}
	return 0;
}

/*
================
idTypeInfoGen::EvaluateIntegerString
================
*/
int idTypeInfoGen::EvaluateIntegerString( const idStr& string )
{
	idParser src;
	idStr evalString;

	if( string.Find( "::" ) != -1 )
	{
		return 0;
	}
	evalString = "$evalint(" + string + ")";
	src.LoadMemory( evalString, evalString.Length(), "eval integer" );
	return src.ParseInt();
}

/*
================
idTypeInfoGen::EvaluateFloatString
================
*/
float idTypeInfoGen::EvaluateFloatString( const idStr& string )
{
	idParser src;
	idStr evalString;

	if( string.Find( "::" ) != -1 )
	{
		return 0.0f;
	}
	evalString = "$evalfloat(" + string + ")";
	src.LoadMemory( evalString, evalString.Length(), "eval float" );
	return src.ParseFloat();
}

/*
================
idTypeInfoGen::FindConstant
================
*/
idConstantInfo* idTypeInfoGen::FindConstant( const char* name )
{
	int i;

	for( i = 0; i < constants.Num(); i++ )
	{
		if( constants[i]->name.Cmp( name ) == 0 )
		{
			return constants[i];
		}
	}
	return NULL;
}

/*
================
idTypeInfoGen::GetIntegerConstant
================
*/
int idTypeInfoGen::GetIntegerConstant( const char* scope, const char* name, idParser& src )
{
	idConstantInfo* constant = FindConstant( idStr( scope ) + name );
	if( constant == NULL )
	{
		constant = FindConstant( name );
	}
	if( constant )
	{
		return EvaluateIntegerString( constant->value );
	}
	src.Warning( "unknown value '%s' in constant expression", name );
	return 0;
}

/*
================
idTypeInfoGen::GetFloatConstant
================
*/
float idTypeInfoGen::GetFloatConstant( const char* scope, const char* name, idParser& src )
{
	idConstantInfo* constant = FindConstant( idStr( scope ) + name );
	if( constant == NULL )
	{
		constant = FindConstant( name );
	}
	if( constant )
	{
		return EvaluateFloatString( constant->value );
	}
	src.Warning( "unknown value '%s' in constant expression", name );
	return 0;
}

/*
================
idTypeInfoGen::ParseArraySize
================
*/
int idTypeInfoGen::ParseArraySize( const char* scope, idParser& src )
{
	idToken token;
	idStr sizeString, constantString;
	int size, totalSize;

	if( !src.CheckTokenString( "[" ) )
	{
		return 0;
	}

	totalSize = 1;
	sizeString = "";
	while( src.ReadToken( &token ) )
	{
		if( token == "]" )
		{
			if( sizeString.Length() )
			{
				size = EvaluateIntegerString( sizeString );
				if( size )
				{
					totalSize *= size;
				}
				sizeString = "";
			}
			if( !src.CheckTokenString( "[" ) )
			{
				break;
			}
		}
		else if( token.type == TT_NAME )
		{
			constantString = token;
			while( src.CheckTokenString( "::" ) )
			{
				src.ExpectTokenType( TT_NAME, 0, &token );
				constantString += "::" + token;
			}
			sizeString += va( "%d", GetIntegerConstant( scope, constantString, src ) );
		}
		else
		{
			sizeString += token;
		}
	}

	return totalSize;
}

/*
================
idTypeInfoGen::ParseConstantValue
================
*/
void idTypeInfoGen::ParseConstantValue( const char* scope, idParser& src, idStr& value )
{
	idToken token;
	idStr constantString;

	int indent = 0;
	while( src.ReadToken( &token ) )
	{
		if( token == "(" )
		{
			indent++;
		}
		else if( token == ")" )
		{
			indent--;
		}
		else if( indent == 0 && ( token == ";" || token == "," || token == "}" ) )
		{
			src.UnreadToken( &token );
			break;
		}
		else if( token.type == TT_NAME )
		{
			constantString = token;
			while( src.CheckTokenString( "::" ) )
			{
				src.ExpectTokenType( TT_NAME, 0, &token );
				constantString += "::" + token;
			}
			value += va( "%d", GetIntegerConstant( scope, constantString, src ) );
			continue;
		}
		value += token;
	}
}

/*
================
idTypeInfoGen::ParseEnumType
================
*/
idEnumTypeInfo* idTypeInfoGen::ParseEnumType( const char* scope, bool isTemplate, bool typeDef, idParser& src )
{
	int value;
	idToken token;
	idEnumTypeInfo* typeInfo;
	idEnumValueInfo enumValue;
	idStr valueString;

	typeInfo = new idEnumTypeInfo;
	typeInfo->scope = scope;
	typeInfo->isTemplate = isTemplate;

	if( src.CheckTokenType( TT_NAME, 0, &token ) )
	{
		typeInfo->typeName = token;
		typeInfo->unnamed = false;
	}
	else
	{
		sprintf( typeInfo->typeName, "enum_%d", enums.Num() );
		typeInfo->unnamed = true;
	}

	if( !src.CheckTokenString( "{" ) )
	{
		src.UnreadToken( &token );
		delete typeInfo;
		return NULL;
	}

	value = -1;
	while( src.ExpectTokenType( TT_NAME, 0, &token ) )
	{

		enumValue.name = token;

		if( src.CheckTokenString( "=" ) )
		{
			idStr valueString;
			ParseConstantValue( scope, src, valueString );
			if( valueString.Length() )
			{
				value = EvaluateIntegerString( valueString );
			}
		}
		else
		{
			value++;
		}

		enumValue.value = value;
		typeInfo->values.Append( enumValue );

		// add a constant for the enum value
		idConstantInfo* constantInfo = new idConstantInfo;
		constantInfo->name = scope + enumValue.name;
		constantInfo->type = "int";
		constantInfo->value = va( "%d", value );
		constants.Append( constantInfo );

		src.CheckTokenString( "," );

		if( src.CheckTokenString( "}" ) )
		{
			break;
		}
	}

	if( typeDef )
	{
		if( src.CheckTokenType( TT_NAME, 0, &token ) )
		{
			typeInfo->typeName = token;
			typeInfo->unnamed = false;
		}
		src.ExpectTokenString( ";" );
	}

	//common->Printf( "enum %s%s\n", typeInfo->scope.c_str(), typeInfo->typeName.c_str() );

	return typeInfo;
}

/*
================
idTypeInfoGen::ParseClassType
================
*/
idClassTypeInfo* idTypeInfoGen::ParseClassType( const char* scope, const char* templateArgs, bool isTemplate, bool typeDef, idParser& src )
{
	idToken token;
	idClassTypeInfo* typeInfo;

	typeInfo = new idClassTypeInfo;
	typeInfo->scope = scope;
	typeInfo->isTemplate = isTemplate;

	if( src.CheckTokenType( TT_NAME, 0, &token ) )
	{
		typeInfo->typeName = token + templateArgs;
		typeInfo->unnamed = false;
	}
	else
	{
		sprintf( typeInfo->typeName, "class_%d%s", classes.Num(), templateArgs );
		typeInfo->unnamed = true;
	}

	if( src.CheckTokenString( ":" ) )
	{
		if( !src.ExpectTokenType( TT_NAME, 0, &token ) )
		{
			delete typeInfo;
			return NULL;
		}
		while(	token == "public" ||
				token == "protected" ||
				token == "private" )
		{

			if( !src.ExpectTokenType( TT_NAME, 0, &token ) )
			{
				delete typeInfo;
				return NULL;
			}

			typeInfo->superType = token;

			// read template arguments
			if( src.CheckTokenString( "<" ) )
			{

				int indent = 1;
				typeInfo->superType += "< ";
				while( src.ReadToken( &token ) )
				{
					if( token == "<" )
					{
						indent++;
					}
					else if( token == ">" )
					{
						indent--;
						if( indent == 0 )
						{
							break;
						}
					}
					typeInfo->superType += token + " ";
				}
				typeInfo->superType += ">";
			}

			// check for multiple inheritance
			if( !src.CheckTokenString( "," ) )
			{
				break;
			}

			if( !src.ExpectTokenType( TT_NAME, 0, &token ) )
			{
				delete typeInfo;
				return NULL;
			}

			src.Warning( "multiple inheritance not supported for '%s%s'", typeInfo->scope.c_str(), typeInfo->typeName.c_str() );
		}
	}

	if( !src.CheckTokenString( "{" ) )
	{
		src.UnreadToken( &token );
		delete typeInfo;
		return NULL;
	}

	ParseScope( typeInfo->scope + typeInfo->typeName + "::", typeInfo->isTemplate, src, typeInfo );

	if( typeDef )
	{
		if( src.CheckTokenType( TT_NAME, 0, &token ) )
		{
			typeInfo->typeName = token + templateArgs;
			typeInfo->unnamed = false;
		}
		src.ExpectTokenString( ";" );
	}

	//common->Printf( "class %s%s : %s\n", typeInfo->scope.c_str(), typeInfo->typeName.c_str(), typeInfo->superType.c_str() );

	return typeInfo;
}

/*
================
idTypeInfoGen::ParseScope
================
*/
void idTypeInfoGen::ParseScope( const char* scope, bool isTemplate, idParser& src, idClassTypeInfo* typeInfo )
{
	int indent;
	idToken token;
	idClassTypeInfo* classInfo;
	idEnumTypeInfo* enumInfo;
	idStr varType;
	bool isConst = false;
	bool isStatic = false;
	bool classValid = false;
	bool forceIgnore = false;

	indent = 1;
	while( indent )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}

		if( token == "Invoke" )
		{
			classValid = true;
		}
		if( token == "{" )
		{

			do
			{
				if( token == "{" )
				{
					indent++;
				}
				else if( token == "}" )
				{
					indent--;
				}
				varType += token + " ";
			}
			while( indent > 1 && src.ReadToken( &token ) );

		}
		else if (token == "TYPEINFO_IGNORE") {
			forceIgnore = true;
		}
		else if (token == "TYPEINFO_CLASS_PROTOTYPE")
		{
			typeInfo->isTypedClass = true;
		}
		else if( token == "}" )
		{

			assert( indent == 1 );
			indent--;

		}
		else if( token == "<" )
		{

			do
			{
				if( token == "<" )
				{
					indent++;
				}
				else if( token == ">" )
				{
					indent--;
				}
				varType += token + " ";
			}
			while( indent > 1 && src.ReadToken( &token ) );

		}
		else if( token == ";" )
		{

			varType = "";
			isConst = false;
			isStatic = false;

		}
		else if( token == "public" || token == "protected" || token == "private" )
		{

			//if ( !src.ExpectTokenString( ":" ) ) {
			//	break;
			//}
			varType = "";
			isConst = false;
			isStatic = false;

		}
		else if( token == "friend" )
		{

			// skip friend classes/methods
			while( src.ReadToken( &token ) )
			{
				if( token == "{" )
				{
					indent++;
				}
				else if( token == "}" )
				{
					indent--;
					if( indent == 1 )
					{
						break;
					}
				}
				else if( token == ";" && indent == 1 )
				{
					break;
				}
			}

			varType = "";
			isConst = false;
			isStatic = false;

		}
		else if( token == "template" )
		{

			varType = "";

			if( src.CheckTokenString( "<" ) )
			{

				int indent = 1;
				varType += "< ";
				while( src.ReadToken( &token ) )
				{
					if( token == "<" )
					{
						indent++;
					}
					else if( token == ">" )
					{
						indent--;
						if( indent == 0 )
						{
							break;
						}
					}
					varType += token + " ";
				}
				varType += ">";
			}

			if( src.CheckTokenString( "class" ) )
			{

				// parse template class
				classInfo = ParseClassType( scope, varType, true, false, src );
				if( classInfo )
				{
					classes.Append( classInfo );
				}

			}
			else
			{

				// skip template methods
				while( src.ReadToken( &token ) )
				{
					if( token == "{" )
					{
						indent++;
					}
					else if( token == "}" )
					{
						indent--;
						if( indent == 1 )
						{
							break;
						}
					}
					else if( token == ";" && indent == 1 )
					{
						break;
					}
				}
			}

			varType = "";
			isConst = false;
			isStatic = false;

		}
		else if( token == "namespace" )
		{

			// parse namespace
			classInfo = ParseClassType( scope, "", isTemplate, false, src );
			delete classInfo;

		}
		else if( token == "class" )
		{

			// parse class
			classInfo = ParseClassType( scope, "", isTemplate, false, src );
			if( classInfo )
			{
				classes.Append( classInfo );
			}

		}
		else if( token == "struct" )
		{

			// parse struct
			classInfo = ParseClassType( scope, "", isTemplate, false, src );
			if( classInfo )
			{
				classes.Append( classInfo );
				varType = classInfo->scope + classInfo->typeName;
			}

		}
		else if( token == "union" )
		{

			// parse union
			classInfo = ParseClassType( scope, "", isTemplate, false, src );
			if( classInfo )
			{
				classes.Append( classInfo );
			}

		}
		else if( token == "enum" )
		{

			// parse enum
			enumInfo = ParseEnumType( scope, isTemplate, false, src );
			if( enumInfo )
			{
				enums.Append( enumInfo );
				varType = enumInfo->scope + enumInfo->typeName;
			}

		}
		else if( token == "typedef" )
		{

			if( token == "class" )
			{

				// parse typedef class
				classInfo = ParseClassType( scope, "", isTemplate, true, src );
				if( classInfo )
				{
					classes.Append( classInfo );
				}

			}
			else if( src.CheckTokenString( "struct" ) )
			{

				// parse typedef struct
				classInfo = ParseClassType( scope, "", isTemplate, true, src );
				if( classInfo )
				{
					classes.Append( classInfo );
				}

			}
			else if( src.CheckTokenString( "union" ) )
			{

				// parse typedef union
				classInfo = ParseClassType( scope, "", isTemplate, true, src );
				if( classInfo )
				{
					classes.Append( classInfo );
				}

			}
			else if( src.CheckTokenString( "enum" ) )
			{

				// parse typedef enum
				enumInfo = ParseEnumType( scope, isTemplate, true, src );
				if( enumInfo )
				{
					enums.Append( enumInfo );
				}

			}
			else
			{

				// skip other typedefs
				while( src.ReadToken( &token ) )
				{
					if( token == "{" )
					{
						indent++;
					}
					else if( token == "}" )
					{
						indent--;
					}
					else if( token == ";" && indent == 1 )
					{
						break;
					}
				}
			}

			varType = "";
			isConst = false;
			isStatic = false;

		}

		else if( token == "const" )
		{

			varType += token + " ";
			isConst = true;

		}
		else if( token == "static" )
		{

			varType += token + " ";
			isStatic = true;

		}
		else if( token.type == TT_NAME )
		{

			assert( indent == 1 );

			// if this is a class operator
			if( token == "operator" )
			{
				while( src.ReadToken( &token ) )
				{
					if( token == "(" )
					{
						src.UnreadToken( &token );
						break;
					}
				}
			}

			// if this is a class method
			if( src.CheckTokenString( "(" ) )
			{

				rvmClassFunctionInfo function = { };
				function.returnType = varType;
				function.isConst = isConst;
				function.isStatic = isStatic;
				function.name = token;

				if( function.returnType[0] == ':' )
				{
					function.returnType = function.returnType.c_str() + 1;
				}

				if( classValid == false )
				{
					function.isValidFunction = false;
				}
				else
				{
					function.isValidFunction = true;
				}
				indent++;

				//rvmClassFunctionParamInfo paramInfo = { };
				//idStr lastToken;
				while( indent > 1 && src.ReadToken( &token ) )
				{
					if( token == "(" )
					{
						indent++;
					}
					else if( token == ")" )
					{
						indent--;
					}
					else if( token == "void" )
					{

					}
					else if( token == "stateParms_t" )
					{
						function.paramStateInput = "stateParms_t";
						src.ReadToken( &token ); // *
						src.ReadToken( &token ); // variable name
					}
					else
					{
						function.isValidFunction = false;
					}
				}

				if( src.CheckTokenString( "(" ) )
				{
					indent++;
					while( indent > 1 && src.ReadToken( &token ) )
					{
						if( token == "(" )
						{
							indent++;
						}
						else if( token == ")" )
						{
							indent--;
						}
					}
				}

				if( src.CheckTokenString( "const" ) )
				{
				}

				if( src.CheckTokenString( "=" ) )
				{

					src.ExpectTokenString( "0" );

				}
				else if( src.CheckTokenString( "{" ) )
				{
					indent++;
					while( indent > 1 && src.ReadToken( &token ) )
					{
						if( token == "{" )
						{
							indent++;
						}
						else if( token == "}" )
						{
							indent--;
						}
					}
				}

				if( classValid  && !forceIgnore)
				{
					typeInfo->functions.Append( function );
				}

				varType = "";
				forceIgnore = false;
				isConst = false;
				isStatic = false;

			}
			else if( ( isStatic || isConst ) && src.CheckTokenString( "=" ) )
			{

				// constant
				idConstantInfo* constantInfo = new idConstantInfo;
				constantInfo->name = scope + token;
				constantInfo->type = varType;
				constantInfo->type.StripTrailing( ' ' );
				ParseConstantValue( scope, src, constantInfo->value );
				constants.Append( constantInfo );

			}
			else if( isStatic )
			{

				// static class variable
				varType += token + " ";

			}
			else
			{

				// check for class variables
				while( 1 )
				{

					int arraySize = ParseArraySize( scope, src );

					if( arraySize )
					{
						idClassVariableInfo var;

						var.name = token;
						var.type = varType;
						var.type.StripTrailing( ' ' );
						var.type += va( "[%d]", arraySize );
						var.bits = 0;
						typeInfo->variables.Append( var );
						if( !src.CheckTokenString( "," ) )
						{
							varType = "";
							isConst = false;
							isStatic = false;
							break;
						}
						varType.StripTrailing( "* " );

					}
					else
					{

						int bits = 0;

						if( src.CheckTokenString( ":" ) )
						{
							idToken bitSize;
							src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &bitSize );
							bits = bitSize.GetIntValue();
						}

						if( src.CheckTokenString( "," ) )
						{
							idClassVariableInfo var;

							var.name = token;
							var.type = varType;
							var.type.StripTrailing( ' ' );
							var.bits = bits;
							typeInfo->variables.Append( var );
							varType.StripTrailing( "* " );

						}
						else if( src.CheckTokenString( ";" ) )
						{
							idClassVariableInfo var;

							var.name = token;
							var.type = varType;
							var.type.StripTrailing( ' ' );
							var.bits = bits;
							typeInfo->variables.Append( var );
							varType = "";
							isConst = false;
							isStatic = false;
							break;

						}
						else
						{

							varType += token + " ";
							break;
						}
					}

					while( src.CheckTokenString( "*" ) )
					{
						varType += "* ";
					}

					if( !src.ExpectTokenType( TT_NAME, 0, &token ) )
					{
						break;
					}
				}
			}
		}
		else
		{
			varType += token + " ";
		}
	}
}

/*
================
idTypeInfoGen::AddDefine
================
*/
void idTypeInfoGen::AddDefine( const char* define )
{
	defines.Append( define );
}

/*
================
idTypeInfoGen::CreateTypeInfo
================
*/
void idTypeInfoGen::CreateTypeInfo( const char* path )
{
	int i, j, inheritance;
	idStr fileName;
	idFileList* files;
	idParser src;

	common->Printf( "Type Info Generator v" TYPE_INFO_GEN_VERSION " (c) 2004 id Software\n" );
	common->Printf( "%s\n", path );

	{

		fileName = "dukegame/Gamelib/game_precompiled.cpp";

		common->Printf( "processing '%s' for type info...\n", fileName.c_str() );

		if( !src.LoadFile( fileName, true ) )
		{
			common->Warning( "couldn't load %s", fileName.c_str() );
			return;
		}

		src.SetFlags( LEXFL_NOBASEINCLUDES );

		for( j = 0; j < defines.Num(); j++ )
		{
			src.AddDefine( defines[j] );
		}

		idClassTypeInfo* typeInfo = new idClassTypeInfo;
		ParseScope( "", false, src, typeInfo );
		delete typeInfo;

		src.FreeSource();
	}

	numTemplates = 0;
	for( i = 0; i < classes.Num(); i++ )
	{
		if( classes[i]->isTemplate )
		{
			numTemplates++;
		}
	}

	maxInheritance = 0;
	maxInheritanceClass = "";
	for( i = 0; i < classes.Num(); i++ )
	{
		inheritance = GetInheritance( classes[i]->typeName );
		if( inheritance > maxInheritance )
		{
			maxInheritance = inheritance;
			maxInheritanceClass = classes[i]->typeName;
		}
	}

	common->Printf( "%d constants\n", constants.Num() );
	common->Printf( "%d enums\n", enums.Num() );
	common->Printf( "%d classes/structs/unions\n", classes.Num() );
	common->Printf( "%d templates\n", numTemplates );
	common->Printf( "%d max inheritance level for '%s'\n", maxInheritance, maxInheritanceClass.c_str() );
}

/*
================
CleanName
================
*/
void CleanName( idStr& name )
{
	name.Replace( "::", "_" );
	name.Replace( " , ", "_" );
	name.Replace( "< ", "_" );
	name.Replace( " >", "_" );
	name.Replace( " ", "_" );
}

/*
================
idTypeInfoGen::WriteTypeInfo
================
*/
void idTypeInfoGen::WriteTypeInfo( const char* fileName ) const
{
	int i, j;
	idStr path, define;
	idFile* file, *filecpp, *savecpp;
	char wd[256];
	getcwd(wd,256);

	path = fileSystem->RelativePathToOSPath( va( "%s.cpp", fileName ) );
	path = idStr(wd) + path;
	filecpp = fileSystem->OpenExplicitFileWrite( path );
	if( !filecpp )
	{
		common->Warning( "couldn't open %s", path.c_str() );
		return;
	}

	path = fileSystem->RelativePathToOSPath(va("%s_savegame.cpp", fileName));
	path = idStr(wd) + path;
	savecpp = fileSystem->OpenExplicitFileWrite(path);
	if (!savecpp)
	{
		common->Warning("couldn't open %s", path.c_str());
		return;
	}

	path = fileSystem->RelativePathToOSPath( va( "%s.h", fileName ) );
	path = idStr(wd) + path;
	file = fileSystem->OpenExplicitFileWrite( path );
	if( !file )
	{
		common->Warning( "couldn't open %s", path.c_str() );
		return;
	}

	filecpp->WriteFloatString( "// This file has been autogenerated by the Type Info Generator\n\n" );
	filecpp->WriteFloatString( "#pragma warning(disable : 4189)\n" );
	filecpp->WriteFloatString( "#pragma warning(disable : 4505)\n" );
	filecpp->WriteFloatString( "#include \"../../idlib/precompiled.h\"\n" );
	filecpp->WriteFloatString( "#include \"../Gamelib/Game_local.h\"\n" );

	savecpp->WriteFloatString("// This file has been autogenerated by the Type Info Generator\n\n");
	savecpp->WriteFloatString("#pragma warning(disable : 4189)\n");
	savecpp->WriteFloatString("#pragma warning(disable : 4505)\n");
	savecpp->WriteFloatString("#include \"../../idlib/precompiled.h\"\n");
	savecpp->WriteFloatString("#include \"../Gamelib/Game_local.h\"\n");

	common->Printf( "writing %s...\n", path.c_str() );

	path.ExtractFileName( define );
	define.Replace( ".", "_" );
	define.ToUpper();

	file->WriteFloatString(
		"\n"
		"#ifndef __%s__\n"
		"#define __%s__\n"
		"\n"
		"/*\n"
		"===================================================================================\n"
		"\n"
		"\tThis file has been generated with the Type Info Generator v" TYPE_INFO_GEN_VERSION " (c) 2004 id Software\n"
		"\n"
		"\t%d constants\n"
		"\t%d enums\n"
		"\t%d classes/structs/unions\n"
		"\t%d templates\n"
		"\t%d max inheritance level for '%s'\n"
		"\n"
		"===================================================================================\n"
		"*/\n"
		"\n", define.c_str(), define.c_str(), constants.Num(), enums.Num(), classes.Num(),
		numTemplates, maxInheritance, maxInheritanceClass.c_str() );

	file->WriteFloatString(
		"typedef struct {\n"
		"\t"	"const char * name;\n"
		"\t"	"const char * type;\n"
		"\t"	"const char * value;\n"
		"} constantInfo_t;\n"
		"\n"
		"typedef struct {\n"
		"\t"	"const char * name;\n"
		"\t"	"int value;\n"
		"} enumValueInfo_t;\n"
		"\n"
		"typedef struct {\n"
		"\t"	"const char * typeName;\n"
		"\t"	"const enumValueInfo_t * values;\n"
		"} enumTypeInfo_t;\n"
		"\n"
		"typedef struct {\n"
		"\t"	"const char * type;\n"
		"\t"	"const char * name;\n"
		"\t"	"intptr_t offset;\n"
		"\t"	"int size;\n"
		"} classVariableInfo_t;\n"
		"\n"
		"typedef struct {\n"
		"\t"	"const char * typeName;\n"
		"\t"	"const char * superType;\n"
		"\t"	"int size;\n"
		"\t"	"const classVariableInfo_t * variables;\n"
		"} classTypeInfo_t;\n"
		"\n" );

	// constants
	file->WriteFloatString( "static constantInfo_t constantInfo[] = {\n" );

	for( i = 0; i < constants.Num(); i++ )
	{
		idConstantInfo* info = constants[i];
		file->WriteFloatString( "\t{ \"%s\", \"%s\", \"%s\" },\n", info->type.c_str(), info->name.c_str(), info->value.c_str() );
	}

	file->WriteFloatString( "\t{ NULL, NULL, NULL }\n" );
	file->WriteFloatString( "};\n\n" );

	// enum values
	for( i = 0; i < enums.Num(); i++ )
	{
		idEnumTypeInfo* info = enums[i];

		idStr typeInfoName = info->scope + info->typeName;
		CleanName( typeInfoName );

		file->WriteFloatString( "static enumValueInfo_t %s_typeInfo[] = {\n", typeInfoName.c_str() );

		for( j = 0; j < info->values.Num(); j++ )
		{
			if( info->isTemplate )
			{
				file->WriteFloatString( "//" );
			}
			file->WriteFloatString( "\t{ \"%s\", %d },\n", info->values[j].name.c_str(), info->values[j].value );
		}

		file->WriteFloatString( "\t{ NULL, 0 }\n" );
		file->WriteFloatString( "};\n\n" );
	}

	// enums
	file->WriteFloatString( "static enumTypeInfo_t enumTypeInfo[] = {\n" );

	for( i = 0; i < enums.Num(); i++ )
	{
		idEnumTypeInfo* info = enums[i];

		idStr typeName = info->scope + info->typeName;
		idStr typeInfoName = typeName;
		CleanName( typeInfoName );

		if( info->isTemplate )
		{
			file->WriteFloatString( "//" );
		}
		file->WriteFloatString( "\t{ \"%s\", %s_typeInfo },\n", typeName.c_str(), typeInfoName.c_str() );
	}

	file->WriteFloatString( "\t{ NULL, NULL }\n" );
	file->WriteFloatString( "};\n\n" );

	// class variables
	for( i = 0; i < classes.Num(); i++ )
	{
		idClassTypeInfo* info = classes[i];

		idStr typeName = info->scope + info->typeName;
		idStr typeInfoName = typeName;
		CleanName( typeInfoName );

		if( typeInfoName == "class_25_class_25" )  // were the hell is this coming from?
		{
			continue;
		}

		if( strstr( typeName.c_str(), "<" ) )
		{
			continue;
		}

		if( info->functions.Num() > 0 )
		{
			filecpp->WriteFloatString( "intptr_t %s::Invoke(const char *functionName, void *param1) {\n", typeInfoName.c_str(), typeName.c_str() );
			//file->WriteFloatString("\tTypeInfoVariableArgs();\n");
			filecpp->WriteFloatString( "\tint functionNameHash = idStr::Hash(functionName);\n" );
			for( j = 0; j < info->functions.Num(); j++ )
			{
				//const char* varType = info->functions[j].returnType.c_str();
				const char* varName = info->functions[j].name.c_str();

				if( info->functions[j].name[0] == '(' )
				{
					continue;
				}

				if( info->functions[j].name[0] == ':' )
				{
					continue;
				}

				if( !info->functions[j].isValidFunction )
				{
					continue;
				}

				idStr tempName = info->functions[j].name;
				tempName.Replace( "_s", "_t" );

				// constructors/deconstrctors
				if( typeName == info->functions[j].name )
				{
					continue;
				}

				if( strstr( varName, "idMenuDataSource" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "idMenuScreen" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "doomLeaderboard_t" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "optionData_t" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "rididBodyIState_s" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "simulatedProjectile_t" ) ) // HACK!
				{
					continue;
				}

				if( typeName == tempName )
				{
					continue;
				}

				if( info->functions[j].isStatic )
				{
					continue;
				}

				int hash = idStr::Hash( varName );
				filecpp->WriteFloatString( "\tif(functionNameHash == %d) { // %s\n", hash, varName );

				idStr functionCall = varName;
				if( info->functions[j].paramStateInput != "" )
				{
					functionCall += va( "((%s *)param1)", info->functions[j].paramStateInput.c_str() );
				}
				else
				{
					functionCall += "()";
				}

				if( !strstr( info->functions[j].returnType.c_str(), "*" ) && info->functions[j].returnType != "stateResult_t " && info->functions[j].returnType != "int " && info->functions[j].returnType != "short " && info->functions[j].returnType != "bool " )
				{
					filecpp->WriteFloatString( "\t\t%s;\n", functionCall.c_str() );
					filecpp->WriteFloatString( "\t\treturn 0;\n", varName );
				}
				else
				{
					filecpp->WriteFloatString( "\t\treturn (intptr_t)%s;\n", functionCall.c_str() );
				}
				filecpp->WriteFloatString( "\t};\n" );
			}

			if( typeInfoName == "idClass" )
			{
				filecpp->WriteFloatString( "\treturn 0;\n\n" );
			}
			else
			{
#ifdef WIN32
				filecpp->WriteFloatString( "\treturn __super::Invoke(functionName, param1);\n\n" );
#else
				filecpp->WriteFloatString( "\treturn %s::Invoke(functionName, param1);\n\n", info->superType.c_str() );
#endif
			}
			filecpp->WriteFloatString( "};\n\n" );
		}

		if( info->functions.Num() > 0 )
		{
			filecpp->WriteFloatString( "bool %s::HasNativeFunction(const char *functionName) {\n", typeInfoName.c_str(), typeName.c_str() );
			//file->WriteFloatString("\tTypeInfoVariableArgs();\n");
			filecpp->WriteFloatString( "\tint functionNameHash = idStr::Hash(functionName);\n" );
			for( j = 0; j < info->functions.Num(); j++ )
			{
				//const char* varType = info->functions[j].returnType.c_str();
				const char* varName = info->functions[j].name.c_str();

				if( info->functions[j].name[0] == '(' )
				{
					continue;
				}

				if( info->functions[j].name[0] == ':' )
				{
					continue;
				}

				if( !info->functions[j].isValidFunction )
				{
					continue;
				}

				idStr tempName = info->functions[j].name;
				tempName.Replace( "_s", "_t" );

				// constructors/deconstrctors
				if( typeName == info->functions[j].name )
				{
					continue;
				}

				if( strstr( varName, "idMenuDataSource" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "idMenuScreen" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "doomLeaderboard_t" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "optionData_t" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "rididBodyIState_s" ) ) // HACK!
				{
					continue;
				}

				if( strstr( varName, "simulatedProjectile_t" ) ) // HACK!
				{
					continue;
				}

				if( typeName == tempName )
				{
					continue;
				}

				if( info->functions[j].isStatic )
				{
					continue;
				}

				int hash = idStr::Hash( varName );
				filecpp->WriteFloatString( "\tif(functionNameHash == %d) { // %s\n", hash, varName );
				filecpp->WriteFloatString( "\t\treturn true;\n" );
				filecpp->WriteFloatString( "\t};\n" );
			}

			if( typeInfoName == "idClass" )
			{
				filecpp->WriteFloatString( "\treturn false;\n\n" );
			}
			else
			{
#ifdef WIN32
				filecpp->WriteFloatString( "\treturn __super::HasNativeFunction(functionName);\n\n" );
#else
				filecpp->WriteFloatString( "\treturn %s::HasNativeFunction(functionName);\n\n", info->superType.c_str() );
#endif
			}
			filecpp->WriteFloatString( "};\n\n" );
		}

		if (info->isTypedClass)
		{
			bool isSkippedClass = typeInfoName == "idThread" || typeInfoName == "idProgram";
			if (!isSkippedClass)
			{
				savecpp->WriteFloatString("void %s::Save(idSaveGame* savefile) const {\n", typeInfoName.c_str());
#ifdef WIN32
				savecpp->WriteFloatString("\t__super::Save(savefile);\n");
#else
				savecpp->WriteFloatString("\t%s::Save(savefile);\n", info->superType.c_str());
#endif
				savecpp->WriteFloatString("}\n\n");

				savecpp->WriteFloatString("void %s::Restore(idRestoreGame* savefile) {\n", typeInfoName.c_str());
#ifdef WIN32
				savecpp->WriteFloatString("\t__super::Restore(savefile);\n");
#else
				savecpp->WriteFloatString("\t%s::Restore(savefile);\n", info->superType.c_str());
#endif
				savecpp->WriteFloatString("}\n\n");
			}
		}

		file->WriteFloatString( "static classVariableInfo_t %s_typeInfo[] = {\n", typeInfoName.c_str() );

		for( j = 0; j < info->variables.Num(); j++ )
		{
			const char* varName = info->variables[j].name.c_str();
			const char* varType = info->variables[j].type.c_str();

			if( info->variables[j].name == "override" )
			{
				continue;
			}

			if( info->unnamed || info->isTemplate || info->variables[j].bits != 0 )
			{
				file->WriteFloatString( "//" );
			}
			file->WriteFloatString( "\t{ \"%s\", \"%s\", (intptr_t)(&((%s *)0)->%s), sizeof( ((%s *)0)->%s ) },\n",
									varType, varName, typeName.c_str(), varName, typeName.c_str(), varName );
		}

		file->WriteFloatString( "\t{ NULL, 0 }\n" );
		file->WriteFloatString( "};\n\n" );
	}

	// classes
	file->WriteFloatString( "static classTypeInfo_t classTypeInfo[] = {\n" );

	for( i = 0; i < classes.Num(); i++ )
	{
		idClassTypeInfo* info = classes[i];

		idStr typeName = info->scope + info->typeName;
		idStr typeInfoName = typeName;
		CleanName( typeInfoName );

		if( info->unnamed || info->isTemplate )
		{
			file->WriteFloatString( "//" );
		}
		file->WriteFloatString( "\t{ \"%s\", \"%s\", sizeof(%s), %s_typeInfo },\n",
								typeName.c_str(), info->superType.c_str(), typeName.c_str(), typeInfoName.c_str() );
	}

	file->WriteFloatString( "\t{ NULL, NULL, 0, NULL }\n" );
	file->WriteFloatString( "};\n\n" );

	file->WriteFloatString( "#endif /* !__%s__ */\n", define.c_str() );

	fileSystem->CloseFile( file );
	fileSystem->CloseFile(savecpp);
}
