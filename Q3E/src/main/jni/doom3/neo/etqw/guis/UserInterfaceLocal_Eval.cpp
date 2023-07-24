// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceLocal.h"
#include "UserInterfaceExpressions.h"
#include "UserInterfaceManagerLocal.h"
#include "UIWindow.h"

#include "../../decllib/declTypeHolder.h"
#include "../../decllib/declLocStr.h"

using namespace sdProperties;

#define ALLOC_EVALUATOR( NAME, TYPE, PARMS, FUNCTION ) uiEvaluators.Append( new sdUIEvaluatorType##TYPE( PARMS, NAME, FUNCTION ) )

/*
============
sdUserInterfaceLocal::InitEvaluators
============
*/
SD_UI_PUSH_CLASS_TAG( sdUserInterfaceLocal )
void sdUserInterfaceLocal::InitEvaluators( void ) {
	SD_UI_PUSH_GROUP_TAG( "Evaluator Functions" )

	SD_UI_FUNC_TAG( compare, "String comparison." )
		SD_UI_FUNC_PARM( string, "str1", "String 1." )
		SD_UI_FUNC_PARM( string, "str2", "String 2." )
		SD_UI_FUNC_RETURN_PARM( float, "True if equal." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "compare", Float, "ss", sdUserInterfaceLocal::Eval_Compare );

	SD_UI_FUNC_TAG( icompare, "Case insensitive string comparison." )
		SD_UI_FUNC_PARM( string, "str1", "String 1." )
		SD_UI_FUNC_PARM( string, "str2", "String 2." )
		SD_UI_FUNC_RETURN_PARM( float, "True if equal." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "icompare", Float, "ss", sdUserInterfaceLocal::Eval_Icompare );

	SD_UI_FUNC_TAG( wcompare, "Wide string comparison." )
		SD_UI_FUNC_PARM( wstring, "str1", "String 1." )
		SD_UI_FUNC_PARM( wstring, "str2", "String 2." )
		SD_UI_FUNC_RETURN_PARM( float, "True if equal." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "wcompare", Float, "ww", sdUserInterfaceLocal::Eval_Wcompare );

	SD_UI_FUNC_TAG( iwcompare, "Case insensitive string comparison. Ignores color codes." )
		SD_UI_FUNC_PARM( wstring, "str1", "String 1." )
		SD_UI_FUNC_PARM( wstring, "str2", "String 2." )
		SD_UI_FUNC_RETURN_PARM( float, "True if equal." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "iwcompare", Float, "ww", sdUserInterfaceLocal::Eval_Iwcompare );

	SD_UI_FUNC_TAG( hcompare, "Compare handles." )
		SD_UI_FUNC_PARM( handle, "handl1", "Handle 1." )
		SD_UI_FUNC_PARM( handle, "handle2", "Handle 2." )
		SD_UI_FUNC_RETURN_PARM( float, "True if equal." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "hcompare", Float, "ii", sdUserInterfaceLocal::Eval_Hcompare );

	SD_UI_FUNC_TAG( toString, "Float to string conversion." )
		SD_UI_FUNC_PARM( float, "value", "Float value." )
		SD_UI_FUNC_PARM( float, "precision", "Number of digits after the decimal point." )
		SD_UI_FUNC_RETURN_PARM( string, "Float value converted to a string." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "toString", String, "ff", sdUserInterfaceLocal::Eval_ToString );

	SD_UI_FUNC_TAG( toWString, "Float to wide string conversion." )
		SD_UI_FUNC_PARM( float, "value", "Float value." )
		SD_UI_FUNC_PARM( float, "precision", "Number of digits after the decimal point." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Float value converted to a wide string." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "toWString", WString, "ff", sdUserInterfaceLocal::Eval_ToWString );

	SD_UI_FUNC_TAG( toFloat, "String value converted to a float." )
		SD_UI_FUNC_PARM( string, "strValue", "String value." )
		SD_UI_FUNC_RETURN_PARM( float, "String value converted to a float." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "toFloat", Float, "s", sdUserInterfaceLocal::Eval_ToFloat );

	SD_UI_FUNC_TAG( wFormat, "Format multiple values into a string." )
		SD_UI_FUNC_PARM( string, "format string", "Format string. %1 is the first string arguent, %2, etc." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "wFormat", WString, "s#", sdUserInterfaceLocal::Eval_FormatWString );

	SD_UI_FUNC_TAG( localizeArgs, "Localize text with additional string arguments." )
		SD_UI_FUNC_PARM( string, "declName", "Text declaration handle." )
		SD_UI_FUNC_PARM( vararg, "...", "Variable number of wide string arguments. This is any additional arguments expected to make the localized text." )
		SD_UI_FUNC_RETURN_PARM( wstring, "The localized text." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "localizeArgs", WString, "s#", sdUserInterfaceLocal::Eval_LocalizeArgs );

	SD_UI_FUNC_TAG( localizeHandle, "Localize text given a text declaration handle." )
		SD_UI_FUNC_PARM( handle, "declHandle", "Declaration index." )
		SD_UI_FUNC_RETURN_PARM( wstring, "The localized text." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "localizeHandle", WString, "i", sdUserInterfaceLocal::Eval_LocalizeHandle );

	SD_UI_FUNC_TAG( localize, "Localize text." )
		SD_UI_FUNC_PARM( string, "declName", "Text declaration name." )
		SD_UI_FUNC_RETURN_PARM( handle, "Declaration handle." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "localize", Int, "s", sdUserInterfaceLocal::Eval_Localize );

	SD_UI_FUNC_TAG( square, "Squares the input value." )
		SD_UI_FUNC_PARM( float, "value", "Float value to square." )
		SD_UI_FUNC_RETURN_PARM( float, "Square of the input value." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "square", Float, "f", sdUserInterfaceLocal::Eval_Square );

	SD_UI_FUNC_TAG( vec2Length, "Length of a 2D vector." )
		SD_UI_FUNC_PARM( vec2, "vector", "2D vector." )
		SD_UI_FUNC_RETURN_PARM( float, "Length of the 2D value." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "vec2Length", Float, "2", sdUserInterfaceLocal::Eval_Vec2Length );

	SD_UI_FUNC_TAG( vec2Normalize, "Normalize a 2D vector." )
		SD_UI_FUNC_PARM( vec2, "vector", "2D vector." )
		SD_UI_FUNC_RETURN_PARM( vec2, "Normalized 2D vector." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "vec2Normalize", Vec2, "2", sdUserInterfaceLocal::Eval_Vec2Normalize );

	SD_UI_FUNC_TAG( abs, "Absolute value of a float." )
		SD_UI_FUNC_PARM( float, "value", "Float value." )
		SD_UI_FUNC_RETURN_PARM( float, "Absolute value." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "abs", Float, "f", sdUserInterfaceLocal::Eval_FAbs );

	SD_UI_FUNC_TAG( min, "Returns the minimum of two values." )
		SD_UI_FUNC_PARM( float, "val1", "Value 1." )
		SD_UI_FUNC_PARM( float, "val2", "Value 2." )
		SD_UI_FUNC_RETURN_PARM( float, "Minimum of the two values." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "min", Float, "ff", sdUserInterfaceLocal::Eval_Min );

	SD_UI_FUNC_TAG( max, "Returns the maximum of two values." )
		SD_UI_FUNC_PARM( float, "val1", "Value 1." )
		SD_UI_FUNC_PARM( float, "val2", "Value 2." )
		SD_UI_FUNC_RETURN_PARM( float, "Maximum of the two values." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "max", Float, "ff", sdUserInterfaceLocal::Eval_Max );

	SD_UI_FUNC_TAG( clamp, "Clamp value between min and max." )
		SD_UI_FUNC_PARM( float, "min", "Minimum value." )
		SD_UI_FUNC_PARM( float, "max", "Maximum value." )
		SD_UI_FUNC_PARM( float, "value", "Value to clamp." )
		SD_UI_FUNC_RETURN_PARM( float, "The value clamped between min and max." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "clamp", Float, "fff", sdUserInterfaceLocal::Eval_Clamp );

	SD_UI_FUNC_TAG( msToHMS, "Converts from milliseconds to hours, minutes, seconds." )
		SD_UI_FUNC_PARM( float, "time", "Time in milliseconds." )
		SD_UI_FUNC_RETURN_PARM( string, "The hours, minutes, seconds in the format H:M:S." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "msToHMS", String, "f", sdUserInterfaceLocal::Eval_MSToHMS );

	SD_UI_FUNC_TAG( sToHMS, "Converts from seconds to hours, minutes, seconds." )
		SD_UI_FUNC_PARM( float, "time", "Time in seconds." )
		SD_UI_FUNC_RETURN_PARM( string, "The hours, minutes, seconds in the format H:M:S." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "sToHMS", String, "f", sdUserInterfaceLocal::Eval_SToHMS );

	SD_UI_FUNC_TAG( toggle, "Returns the opposite boolean value of the input value." )
		SD_UI_FUNC_PARM( float, "value", "Value to toggle." )
		SD_UI_FUNC_RETURN_PARM( float, "True if value is false." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "toggle", Float, "f", sdUserInterfaceLocal::Eval_Toggle );

	SD_UI_FUNC_TAG( floor, "The floor of the input value." )
		SD_UI_FUNC_PARM( float, "value", "Float value." )
		SD_UI_FUNC_RETURN_PARM( float, "Floor of the input value." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "floor", Float, "f", sdUserInterfaceLocal::Eval_Floor );

	SD_UI_FUNC_TAG( ceil, "The ceiling of the input value." )
		SD_UI_FUNC_PARM( float, "value", "Float value." )
		SD_UI_FUNC_RETURN_PARM( float, "Ceiling of the input value." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "ceil", Float, "f", sdUserInterfaceLocal::Eval_Ceil );

	SD_UI_FUNC_TAG( toUpper, "Convert a string to upper case." )
		SD_UI_FUNC_PARM( string, "str", "String to convert to upper case." )
		SD_UI_FUNC_RETURN_PARM( string, "String converted to upper case." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "toUpper", String, "s", sdUserInterfaceLocal::Eval_ToUpper );

	SD_UI_FUNC_TAG( toLower, "Convert a string to lower case." )
		SD_UI_FUNC_PARM( string, "str", "String to convert to lower case." )
		SD_UI_FUNC_RETURN_PARM( string, "String converted to lower case." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "toLower", String, "s", sdUserInterfaceLocal::Eval_ToLower );

	SD_UI_FUNC_TAG( toWStr, "Convert a string to a wide string." )
		SD_UI_FUNC_PARM( string, "str", "String to convert." )
		SD_UI_FUNC_RETURN_PARM( wstring, "String converted to wide string." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "toWStr", WString, "s", sdUserInterfaceLocal::Eval_ToWStr );

	SD_UI_FUNC_TAG( toStr, "Convert a wide string to a string." )
		SD_UI_FUNC_PARM( wstring, "wstr", "Wide string to convert." )
		SD_UI_FUNC_RETURN_PARM( string, "Wide string converted to string." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "toStr", String, "w", sdUserInterfaceLocal::Eval_ToStr );

	SD_UI_FUNC_TAG( isValidHandle, "Check if handle is valid." )
		SD_UI_FUNC_PARM( handle, "_handle", "Handle to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if valid." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "isValidHandle", Float, "i", sdUserInterfaceLocal::Eval_IsValidHandle );

	SD_UI_FUNC_TAG( floatToHandle, "Convert a float to a handle." )
		SD_UI_FUNC_PARM( float, "fHandle", "Float value to convert to a handle." )
		SD_UI_FUNC_RETURN_PARM( handle, "Value converted to a handle." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "floatToHandle", Int, "f", sdUserInterfaceLocal::Eval_FloatToHandle );

	SD_UI_FUNC_TAG( color, "Convert a string to a color." )
		SD_UI_FUNC_PARM( string, "strColor", "String with the format \"R G B A\"." )
		SD_UI_FUNC_RETURN_PARM( color, "The color." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "color", Vec4, "s", sdUserInterfaceLocal::Eval_Color );

	SD_UI_FUNC_TAG( strLen, "Length of the string." )
		SD_UI_FUNC_PARM( string, "str", "String." )
		SD_UI_FUNC_RETURN_PARM( float, "The length of the string." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "strLen", Float, "s", sdUserInterfaceLocal::Eval_StrLen );

	SD_UI_FUNC_TAG( wstrLen, "Length of the wide string." )
		SD_UI_FUNC_PARM( wstring, "str", "Wide string." )
		SD_UI_FUNC_RETURN_PARM( float, "The length of the wide string." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "wstrLen", Float, "w", sdUserInterfaceLocal::Eval_WStrLen );

	SD_UI_FUNC_TAG( sin, "Sine of the input value." )
		SD_UI_FUNC_PARM( float, "val", "Input value." )
		SD_UI_FUNC_RETURN_PARM( float, "Sine of the input value." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "sin", Float, "f", sdUserInterfaceLocal::Eval_Sin );

	SD_UI_FUNC_TAG( cos, "Cosine of the input value." )
		SD_UI_FUNC_PARM( float, "val", "Input value." )
		SD_UI_FUNC_RETURN_PARM( float, "Cosine of the input value." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "cos", Float, "f", sdUserInterfaceLocal::Eval_Cos );

	SD_UI_FUNC_TAG( colorForIndex, "Color for the index where the index is between 0 and 31." )
		SD_UI_FUNC_PARM( float, "index", "Color index." )
		SD_UI_FUNC_RETURN_PARM( vec4, "Color at the index." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "colorForIndex", Vec4, "f", sdUserInterfaceLocal::Eval_ColorForIndex );

	SD_UI_FUNC_TAG( stringToVec4, "Convert a string to a 4D vector." )
		SD_UI_FUNC_PARM( string, "strRect", "String with the format \"X Y Z W\"." )
		SD_UI_FUNC_RETURN_PARM( vec4, "The 4D vector." )
	SD_UI_END_FUNC_TAG
	ALLOC_EVALUATOR( "stringToVec4", Vec4, "s", sdUserInterfaceLocal::Eval_StringToVec4 );

	SD_UI_POP_GROUP_TAG
}
SD_UI_POP_CLASS_TAG

#undef ALLOC_EVALUATOR

/*
============
sdUserInterfaceLocal::Eval_Vec2Normalize
============
*/
idVec2 sdUserInterfaceLocal::Eval_Vec2Normalize( const sdUIEvaluator* evaluator ) {
	idVec2 temp;
	idVec2 retVal = evaluator->GetParm( 0 )->GetVec2Value( temp );
	retVal.Normalize();
	return retVal;
}


/*
============
sdUserInterfaceLocal::Eval_Square
============
*/
float sdUserInterfaceLocal::Eval_Square( const sdUIEvaluator* evaluator ) {
	return Square( evaluator->GetParm( 0 )->GetFloatValue() );
}

/*
============
sdUserInterfaceLocal::Eval_Vec2Length
============
*/
float sdUserInterfaceLocal::Eval_Vec2Length( const sdUIEvaluator* evaluator ) {
	idVec2 temp;
	return evaluator->GetParm( 0 )->GetVec2Value( temp ).Length();
}

/*
============
sdUserInterfaceLocal::Eval_ToFloat
============
*/
float sdUserInterfaceLocal::Eval_ToFloat( const sdUIEvaluator* evaluator ) {
	idStr temp;
	idStr const &parm1 = evaluator->GetParm( 0 )->GetStringValue( temp );
	float output = 0.0f;
	
	int num = sscanf( parm1, "%f", &output );
	if( num != 1 ) {
		gameLocal.Warning( "sdUserInterfaceLocal::Eval_ToFloat: Invalid input string '%s'", parm1.c_str() );
		output = 0.0f;
	}

	return output;
}

/*
============
sdUserInterfaceLocal::Eval_Compare
============
*/
float sdUserInterfaceLocal::Eval_Compare( const sdUIEvaluator* evaluator ) {
	idStr temp1;
	idStr const &parm1 = evaluator->GetParm( 0 )->GetStringValue( temp1 );
	idStr temp2;
	idStr const &parm2 = evaluator->GetParm( 1 )->GetStringValue( temp2 );

	if ( !parm1.Cmp( parm2 ) ) {
		return 1.0f;
	}
	return 0.0f;
}

/*
============
sdUserInterfaceLocal::Eval_Icompare
============
*/
float sdUserInterfaceLocal::Eval_Icompare( const sdUIEvaluator* evaluator ) {
	idStr temp1;
	idStr const &parm1 = evaluator->GetParm( 0 )->GetStringValue( temp1 );
	idStr temp2;
	idStr const &parm2 = evaluator->GetParm( 1 )->GetStringValue( temp2 );

	if ( !parm1.IcmpNoColor( parm2 ) ) {
		return 1.0f;
	}
	return 0.0f;
}

/*
============
sdUserInterfaceLocal::Eval_Wcompare
============
*/
float sdUserInterfaceLocal::Eval_Wcompare( const sdUIEvaluator* evaluator ) {
	idWStr temp1;
	idWStr parm1 = evaluator->GetParm( 0 )->GetWStringValue( temp1 );
	idWStr temp2;
	idWStr parm2 = evaluator->GetParm( 1 )->GetWStringValue( temp2 );

	if ( !parm1.Cmp( parm2.c_str() ) ) {
		return 1.0f;
	}
	return 0.0f;
}

/*
============
sdUserInterfaceLocal::Eval_Iwcompare
============
*/
float sdUserInterfaceLocal::Eval_Iwcompare( const sdUIEvaluator* evaluator ) {
	idWStr temp1;
	idWStr parm1 = evaluator->GetParm( 0 )->GetWStringValue( temp1 );
	idWStr temp2;
	idWStr parm2 = evaluator->GetParm( 1 )->GetWStringValue( temp2 );

	if ( !parm1.IcmpNoColor( parm2.c_str() ) ) {
		return 1.0f;
	}
	return 0.0f;
}

/*
============
sdUserInterfaceLocal::Eval_ToString
============
*/
idStr sdUserInterfaceLocal::Eval_ToString( const sdUIEvaluator* evaluator ) {
	float parm1 = evaluator->GetParm( 0 )->GetFloatValue();
	float parm2 = evaluator->GetParm( 1 )->GetFloatValue();

	// don't allow "-0"
	if ( (int)parm2 == 0 ) {
		parm1 = (int)parm1;
	}

	const char* formatString = va( "%%.%if", (int)parm2 );
	return idStr( va_floatstring( formatString, parm1 ));
}

/*
============
sdUserInterfaceLocal::Eval_ToWString
============
*/
idWStr sdUserInterfaceLocal::Eval_ToWString( const sdUIEvaluator* evaluator ) {
	float parm1 = evaluator->GetParm( 0 )->GetFloatValue();
	float parm2 = evaluator->GetParm( 1 )->GetFloatValue();

	// don't allow "-0"
	if ( (int)parm2 == 0 ) {
		parm1 = (int)parm1;
	}

	const wchar_t* formatString = va( L"%%.%if", (int)parm2 );
	return idWStr( va( formatString, parm1 ) );	// FIXME: no va_floatstring for wide strings
}

/*
============
sdUserInterfaceLocal::Eval_FAbs
============
*/
float sdUserInterfaceLocal::Eval_FAbs( const sdUIEvaluator* evaluator ) {
	float parm1 = evaluator->GetParm( 0 )->GetFloatValue();
	return idMath::Fabs( parm1 );
}

/*
============
sdUserInterfaceLocal::Eval_Min
============
*/
float sdUserInterfaceLocal::Eval_Min( const sdUIEvaluator* evaluator ) {
	float parm1 = evaluator->GetParm( 0 )->GetFloatValue();
	float parm2 = evaluator->GetParm( 1 )->GetFloatValue();
	return Min( parm1, parm2 );
}

/*
============
sdUserInterfaceLocal::Eval_Max
============
*/
float sdUserInterfaceLocal::Eval_Max( const sdUIEvaluator* evaluator ) {
	float parm1 = evaluator->GetParm( 0 )->GetFloatValue();
	float parm2 = evaluator->GetParm( 1 )->GetFloatValue();
	return Max( parm1, parm2 );
}

/*
============
sdUserInterfaceLocal::Eval_Clamp
============
*/
float sdUserInterfaceLocal::Eval_Clamp( const sdUIEvaluator* evaluator ) {
	float parm1 = evaluator->GetParm( 0 )->GetFloatValue();
	float parm2 = evaluator->GetParm( 1 )->GetFloatValue();
	float parm3 = evaluator->GetParm( 2 )->GetFloatValue();
	return idMath::ClampFloat( parm1, parm2, parm3 );
}

/*
============
sdUserInterfaceLocal::Eval_MSToHMS
============
*/
idStr sdUserInterfaceLocal::Eval_MSToHMS( const sdUIEvaluator* evaluator ) {
	double ms = evaluator->GetParm( 0 )->GetFloatValue();

	return idStr( idStr::MS2HMS( ms ) );
}

/*
============
sdUserInterfaceLocal::Eval_SToHMS
============
*/
idStr sdUserInterfaceLocal::Eval_SToHMS( const sdUIEvaluator* evaluator ) {
	double s = evaluator->GetParm( 0 )->GetFloatValue();
	return idStr( idStr::MS2HMS( s * 1000 ) );
}

/*
============
sdUserInterfaceLocal::Eval_Toggle
============
*/
float sdUserInterfaceLocal::Eval_Toggle( const sdUIEvaluator* evaluator ) {
	int s = idMath::Ftoi( evaluator->GetParm( 0 )->GetFloatValue() );
	return !s;
}

/*
============
sdUserInterfaceLocal::Eval_LocalizeArgs
============
*/
idWStr sdUserInterfaceLocal::Eval_LocalizeArgs( const sdUIEvaluator* evaluator ) {
	idWStrList formatStrings;
	formatStrings.SetNum( evaluator->GetNumParms() - 1 );

	idWStr tempw;
	for ( int i = 1; i < evaluator->GetNumParms(); i++ ) {		
		formatStrings[ i - 1 ] = evaluator->GetParm( i )->GetWStringValue( tempw );
	}

	idStr temp;
	return common->LocalizeText( evaluator->GetParm( 0 )->GetStringValue( temp ), formatStrings );
}

/*
============
sdUserInterfaceLocal::Eval_LocalizeHandle
============
*/
idWStr sdUserInterfaceLocal::Eval_LocalizeHandle( const sdUIEvaluator* evaluator ) {
	const sdDeclLocStr* str = declHolder.declLocStrType.LocalFindByIndex( evaluator->GetParm( 0 )->GetIntValue() );
	return idWStr ( str->GetText() );
}

/*
============
sdUserInterfaceLocal::Eval_Localize
============
*/
int sdUserInterfaceLocal::Eval_Localize( const sdUIEvaluator* evaluator ) {
	idStr temp;
	return declHolder.FindLocStr( evaluator->GetParm( 0 )->GetStringValue(temp) )->Index();
}

/*
============
sdUserInterfaceLocal::Eval_Floor
============
*/
float sdUserInterfaceLocal::Eval_Floor( const sdUIEvaluator* evaluator ) {
	return idMath::Floor( evaluator->GetParm( 0 )->GetFloatValue() );
}

/*
============
sdUserInterfaceLocal::Eval_Ceil
============
*/
float sdUserInterfaceLocal::Eval_Ceil( const sdUIEvaluator* evaluator ) {
	return idMath::Ceil( evaluator->GetParm( 0 )->GetFloatValue() );
}

/*
============
sdUserInterfaceLocal::Eval_ToUpper
============
*/
idStr sdUserInterfaceLocal::Eval_ToUpper( const sdUIEvaluator* evaluator ) {
	idStr temp;
	idStr parm1 = evaluator->GetParm( 0 )->GetStringValue( temp );

	parm1.ToUpper();
	return parm1;
}

/*
============
sdUserInterfaceLocal::Eval_ToLower
============
*/
idStr sdUserInterfaceLocal::Eval_ToLower( const sdUIEvaluator* evaluator ) {
	idStr temp;
	idStr parm1 = evaluator->GetParm( 0 )->GetStringValue( temp );

	parm1.ToLower();
	return parm1;
}

/*
============
sdUserInterfaceLocal::Eval_ToWStr
============
*/
idWStr sdUserInterfaceLocal::Eval_ToWStr( const sdUIEvaluator* evaluator ) {
	idStr temp;
	const char* parm1 = evaluator->GetParm( 0 )->GetStringValue( temp ).c_str();

	return va( L"%hs", parm1 );
}

/*
============
sdUserInterfaceLocal::Eval_ToStr
============
*/
idStr sdUserInterfaceLocal::Eval_ToStr( const sdUIEvaluator* evaluator ) {
	idWStr temp;
	idWStr const &parm1 = evaluator->GetParm( 0 )->GetWStringValue( temp );

	return va( "%ls", parm1.c_str() );
}

/*
============
sdUserInterfaceLocal::Eval_IsValidHandle
============
*/
float sdUserInterfaceLocal::Eval_IsValidHandle( const sdUIEvaluator* evaluator ) {
	return evaluator->GetParm( 0 )->GetIntValue() != -1 ? 1.0f : 0.0f;
}

/*
============
sdUserInterfaceLocal::Eval_FloatToHandle
============
*/
int sdUserInterfaceLocal::Eval_FloatToHandle( const sdUIEvaluator* evaluator ) {
	return idMath::FtoiFast( evaluator->GetParm( 0 )->GetFloatValue() );
}


/*
============
sdUserInterfaceLocal::Eval_Hcompare
============
*/
float sdUserInterfaceLocal::Eval_Hcompare( const sdUIEvaluator* evaluator ) {
	return ( evaluator->GetParm( 0 )->GetIntValue() == evaluator->GetParm( 1 )->GetIntValue() ) ? 1.0f : 0.0f;
}

/*
============
sdUserInterfaceLocal::Eval_Color
============
*/
idVec4 sdUserInterfaceLocal::Eval_Color( const sdUIEvaluator* evaluator ) {
	idStr temp;
	return evaluator->GetScope()->GetUI()->GetColor( evaluator->GetParm( 0 )->GetStringValue( temp ) );
}

/*
============
sdUserInterfaceLocal::Eval_StrLen
============
*/
float sdUserInterfaceLocal::Eval_StrLen( const sdUIEvaluator* evaluator ) {
	idStr temp;
	return idStr::Length( evaluator->GetParm( 0 )->GetStringValue( temp ).c_str() );
}


/*
============
sdUserInterfaceLocal::Eval_WStrLen
============
*/
float sdUserInterfaceLocal::Eval_WStrLen( const sdUIEvaluator* evaluator ) {
	idWStr temp;
	return idWStr::Length( evaluator->GetParm( 0 )->GetWStringValue( temp ).c_str() );
}

/*
============
sdUserInterfaceLocal::Eval_Sin
============
*/
float sdUserInterfaceLocal::Eval_Sin( const sdUIEvaluator* evaluator ) {
	return idMath::Sin( evaluator->GetParm( 0 )->GetFloatValue() );
}

/*
============
sdUserInterfaceLocal::Eval_Cos
============
*/
float sdUserInterfaceLocal::Eval_Cos( const sdUIEvaluator* evaluator ) {
	return idMath::Cos( evaluator->GetParm( 0 )->GetFloatValue() );
}

/*
============
sdUserInterfaceLocal::Eval_ColorForIndex
============
*/
idVec4 sdUserInterfaceLocal::Eval_ColorForIndex( const sdUIEvaluator* evaluator ) {
	return idStr::ColorForIndex( idMath::Ftoi( evaluator->GetParm( 0 )->GetFloatValue() ) );
}

/*
============
sdUserInterfaceLocal::Eval_StringToVec4
============
*/
idVec4 sdUserInterfaceLocal::Eval_StringToVec4( const sdUIEvaluator* evaluator ) {
	idStr temp;
	idVec4 const &value = sdTypeFromString< idVec4 >( evaluator->GetParm( 0 )->GetStringValue( temp ) );
	return value;
}


/*
============
sdUserInterfaceLocal::Eval_FormatWString
============
*/
idWStr sdUserInterfaceLocal::Eval_FormatWString( const sdUIEvaluator* evaluator ) {
	idWStr temp;
	
	idStr tempFormat;
	const idStr& formatStr = evaluator->GetParm( 0 )->GetStringValue( tempFormat );

	sdWStringBuilder_Heap builder;
	for( int i = 0; i < formatStr.Length(); i++ ) {
		if( formatStr[ i ] != '%' ) {
			builder += va( L"%hc", formatStr[ i ] );
			continue;
		}
		if( i + 1 < formatStr.Length() ) {
			i++;
			if( formatStr[ i ] == '%' ) {
				builder += L'%';
			} else {				
				int index = sdTypeFromString< int >( &formatStr.c_str()[ i ] );
				if( index > 0 && index < evaluator->GetNumParms() ) {
					const idWStr& value = evaluator->GetParm( index )->GetWStringValue( temp );
					builder.Append( value.c_str() );
				} else {
					builder += L"###";
				}
			}
		}
	}
	builder.ToString( temp );
	return temp;
}
