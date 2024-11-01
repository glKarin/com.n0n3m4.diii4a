#include "PrecompCommon.h"
#include "gmConfig.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmHelpers.h"
#include "Wm3Math.h"

// script: MathLibrary
//		Exposes useful math functionality to the scripting system.

//////////////////////////////////////////////////////////////////////////

// function: UnitRandom
//		Generates a random floating point number between 0.0 and 1.0
//
// Parameters:
//
//		None
//
// Returns:
//		float - random number 0.0 - 1.0
static int GM_CDECL gmfUnitRandom(gmThread *a_thread)
{
	a_thread->PushFloat(Mathf::UnitRandom());	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: SymmetricRandom
//		Generates a random floating point number between -1.0 and 1.0
//
// Parameters:
//
//		None
//
// Returns:
//		float - random number -1.0 - 1.0
static int GM_CDECL gmfSymmetricRandom(gmThread *a_thread)
{
	a_thread->PushFloat(Mathf::SymmetricRandom());	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: RandRange
//		deprecated - Use <RandFloat>

//////////////////////////////////////////////////////////////////////////

// function: RandFloat
//		Generates a random floating point number between a user defined range
//
// Parameters:
//
//		float or int - min value
//		float or int - max value
//
// Returns:
//		float - random number between supplied range
static int GM_CDECL gmfRandFloat(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_FLOAT_OR_INT_PARAM(fMin, 0);
	GM_CHECK_FLOAT_OR_INT_PARAM(fMax, 1);
	a_thread->PushFloat(Mathf::IntervalRandom(fMin, fMax));	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: RandInt
//		Generates a random integer number between a user defined range
//
// Parameters:
//
//		float or int - min value
//		float or int - max value
//
// Returns:
//		int - random number between supplied range
static int GM_CDECL gmfRandInt(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(iMin, 0);
	GM_CHECK_INT_PARAM(iMax, 1);
	a_thread->PushInt(Mathf::IntervalRandomInt(iMin, iMax+1));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Sin
//		Gives the Sin for a number
//
// Parameters:
//
//		float or int - value
//
// Returns:
//		float - sin of number
static int GM_CDECL gmfSin(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(fValue, 0);
	a_thread->PushFloat(Mathf::Sin(fValue));	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ASin
//		Gives the ASin for a number
//
// Parameters:
//
//		float or int - value
//
// Returns:
//		float - asin of number
static int GM_CDECL gmfASin(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(fValue, 0);
	a_thread->PushFloat(Mathf::ASin(fValue));	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Cos
//		Gives the Cos for a number
//
// Parameters:
//
//		float or int - value
//
// Returns:
//		float - cos of number
static int GM_CDECL gmfCos(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(fValue, 0);
	a_thread->PushFloat(Mathf::Cos(fValue));	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ACos
//		Gives the ACos for a number
//
// Parameters:
//
//		float or int - value
//
// Returns:
//		float - acos of number
static int GM_CDECL gmfACos(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(fValue, 0);
	a_thread->PushFloat(Mathf::ACos(fValue));	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Tan
//		Gives the Tan for a number
//
// Parameters:
//
//		float or int - value
//
// Returns:
//		float - tan of number
static int GM_CDECL gmfTan(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(fValue, 0);
	a_thread->PushFloat(Mathf::Tan(fValue));	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ATan
//		Gives the ATan for a number
//
// Parameters:
//
//		float or int - value
//
// Returns:
//		float - atan of number
static int GM_CDECL gmfATan(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(fValue, 0);
	a_thread->PushFloat(Mathf::ATan(fValue));	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Clamp
//		Clamps a number between 2 values.
//
// Parameters:
//
//		float or int - value to clamp
//		float or int - min value
//		float or int - max value
//
// Returns:
//		float - clamped value
static int GM_CDECL gmfClamp(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(3);
	if(a_thread->ParamType(0)==GM_INT &&
		a_thread->ParamType(0)==GM_INT &&
		a_thread->ParamType(0)==GM_INT)
	{
		GM_CHECK_INT_PARAM(value, 0);
		GM_CHECK_INT_PARAM(min, 1);
		GM_CHECK_INT_PARAM(max, 2);

		a_thread->PushInt(ClampT<int>(value, min, max));
	}
	else
	{
		GM_CHECK_FLOAT_OR_INT_PARAM(value, 0);
		GM_CHECK_FLOAT_OR_INT_PARAM(min, 1);
		GM_CHECK_FLOAT_OR_INT_PARAM(max, 2);

		a_thread->PushFloat(ClampT<float>(value, min, max));
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: RadToDeg
//		Converts function from radians to degrees
//
// Parameters:
//
//		float or int - value to convert
//
// Returns:
//		float - converted value
static int GM_CDECL gmfRadToDeg(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(radians,0);
	a_thread->PushFloat(Mathf::RadToDeg(radians));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DegToRad
//		Converts function from degrees to radians
//
// Parameters:
//
//		float or int - value to convert
//
// Returns:
//		float - converted value
static int GM_CDECL gmfDegToRad(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(degrees,0);
	a_thread->PushFloat(Mathf::DegToRad(degrees));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Sign
//		Returns a unit sign for a value. -1 if value is negative, 1 if positive
//
// Parameters:
//
//		float or int - value to convert
//
// Returns:
//		float - converted value
static int GM_CDECL gmfSign(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);	

	if(a_thread->ParamType(0) == GM_INT) 
		a_thread->PushInt(Mathf::Sign(a_thread->Param(0).m_value.m_int));
	else if(a_thread->ParamType(0) == GM_FLOAT)
		a_thread->PushFloat(Mathf::Sign(a_thread->Param(0).m_value.m_float));
	else
	{
		GM_EXCEPTION_MSG("Invalid Param type in %s", __FUNCTION__);
		return GM_EXCEPTION;	
	}
	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ToBool
//		Converts a value to a bool
//
// Parameters:
//
//		float, int or string - what to convert to a int
//
// Returns:
//		int - converted value
static int GM_CDECL gmfToBool(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	switch(a_thread->ParamType(0))
	{
	case GM_INT:
	case GM_FLOAT:
		a_thread->PushInt(a_thread->Param(0).GetIntSafe() != 0);
		break;
	case GM_STRING:
		{
			gmStringObject *pString = a_thread->Param(0).GetStringObjectSafe();
			String s = pString->GetString();
			if(Utils::StringToTrue(s))
				a_thread->PushInt(1);
			else if(Utils::StringToFalse(s))
				a_thread->PushInt(0);
			else
				a_thread->PushNull();
			break;
		}
	}	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Abs
//		Absolute value of a number
//
// Parameters:
//
//		float or int - what to get the abs for.
//
// Returns:
//		float or int - the abs value
static int GM_CDECL gmfAbs(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	if(a_thread->ParamType(0) == GM_INT)
	{
		int intValue = a_thread->Param(0).GetInt();
		a_thread->PushInt(abs(intValue));
		return GM_OK;
	}
	else if(a_thread->ParamType(0) == GM_FLOAT)
	{		
		float floatValue = a_thread->Param(0).GetFloat();
		a_thread->PushFloat((float)Mathf::FAbs(floatValue));
		return GM_OK;
	}

	GM_EXCEPTION_MSG("expected float or int param.");
	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

// function: Sqrt
//		Square root of a number
//
// Parameters:
//
//		float or int - what to get the sqrt for.
//
// Returns:
//		float or int - the sqrt value
static int GM_CDECL gmfSqrt(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	if(a_thread->ParamType(0) == GM_INT)
	{
		int intValue = a_thread->Param(0).m_value.m_int;
		a_thread->PushInt((int)Mathf::Sqrt((float)intValue));
		return GM_OK;
	}
	else if(a_thread->ParamType(0) == GM_FLOAT)
	{
		float floatValue = a_thread->Param(0).m_value.m_float;
		a_thread->PushFloat(Mathf::Sqrt(floatValue));
		return GM_OK;
	}

	GM_EXCEPTION_MSG("expected float or int param.");
	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

// function: Min
//		Returns the minimum of 2 numbers
//
// Parameters:
//
//		float or int - 1st number
//		float or int - 2nd number
//
// Returns:
//		float or int - the min of the 2 values
static int GM_CDECL gmfMin(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	if(a_thread->ParamType(0) == GM_INT && a_thread->ParamType(1) == GM_INT)
	{
		a_thread->PushInt(MinT<int>(a_thread->Param(0).GetInt(), a_thread->Param(1).GetInt()));
		return GM_OK;
	}
	if(a_thread->ParamType(0) == GM_FLOAT && a_thread->ParamType(1) == GM_FLOAT)
	{
		a_thread->PushFloat(MinT<float>(a_thread->Param(0).GetFloat(), a_thread->Param(1).GetFloat()));
		return GM_OK;
	}

	GM_EXCEPTION_MSG("expected 2 floats or 2 ints.");
	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

// function: Max
//		Returns the maximum of 2 numbers
//
// Parameters:
//
//		float or int - 1st number
//		float or int - 2nd number
//
// Returns:
//		float or int - the max of the 2 values
static int GM_CDECL gmfMax(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	if(a_thread->ParamType(0) == GM_INT && a_thread->ParamType(1) == GM_INT)
	{
		a_thread->PushInt(MaxT<int>(a_thread->Param(0).GetInt(), a_thread->Param(1).GetInt()));
		return GM_OK;
	}
	if(a_thread->ParamType(0) == GM_FLOAT && a_thread->ParamType(1) == GM_FLOAT)
	{
		a_thread->PushFloat(MaxT<float>(a_thread->Param(0).GetFloat(), a_thread->Param(1).GetFloat()));
		return GM_OK;
	}

	GM_EXCEPTION_MSG("expected 2 floats or 2 ints.");
	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

// function: Floor
//		Returns the floor of a number
//
// Parameters:
//
//		float or int - number
//
// Returns:
//		float or int - the floor of the number
static int GM_CDECL gmfFloor(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	if(a_thread->ParamType(0) == GM_FLOAT)
	{
		float floatValue = a_thread->Param(0).m_value.m_float;
		a_thread->PushFloat(floorf(floatValue));
		return GM_OK;
	}
	else if(a_thread->ParamType(0) == GM_INT) //Do nothing if Int
	{
		int intValue = a_thread->Param(0).m_value.m_int;
		a_thread->PushInt(intValue);
		return GM_OK;
	}

	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

// function: Ceil
//		Returns the ceil of a number
//
// Parameters:
//
//		float or int - number
//
// Returns:
//		float or int - the ceil of the number
static int GM_CDECL gmfCeil(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	if(a_thread->ParamType(0) == GM_FLOAT)
	{
		float floatValue = a_thread->Param(0).m_value.m_float;
		a_thread->PushFloat(ceilf(floatValue));
		return GM_OK;
	}
	else if(a_thread->ParamType(0) == GM_INT) //Do nothing if Int
	{
		int intValue = a_thread->Param(0).m_value.m_int;
		a_thread->PushInt(intValue);
		return GM_OK;
	}

	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

// function: Round
//		Rounds a number to the nearest whole number(non fractional)
//
// Parameters:
//
//		float or int - number
//
// Returns:
//		float or int - the round of the number
static int GM_CDECL gmfRound(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	if(a_thread->ParamType(0) == GM_FLOAT)
	{
		float floatValue = a_thread->Param(0).m_value.m_float;
		a_thread->PushFloat(floorf(floatValue + 0.5f));
		return GM_OK;
	}
	else if(a_thread->ParamType(0) == GM_INT) //Do nothing if Int
	{
		int intValue = a_thread->Param(0).m_value.m_int;
		a_thread->PushInt(intValue);
		return GM_OK;
	}
	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

// function: UnitCircleNormalize
//		Normalizes a value to the range of a unit circle
//
// Parameters:
//
//		float or int - number
//
// Returns:
//		float or int - the normalized number
static int GM_CDECL gmfUnitCircleNormalize(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(num,0);
	a_thread->PushFloat(Mathf::UnitCircleNormalize(num));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

int GM_CDECL gmfToString(gmThread * a_thread)
{
	const gmVariable * var = a_thread->GetThis();

	if(GM_INT == var->m_type)
	{
		char numberAsStringBuffer[64];
		sprintf(numberAsStringBuffer, "%d", var->m_value.m_int); // this won't be > 64 chars
		a_thread->PushNewString(numberAsStringBuffer);
	}
	else if (GM_FLOAT == var->m_type)
	{
		char numberAsStringBuffer[64];

		sprintf(numberAsStringBuffer, "%f", var->m_value.m_float); // this won't be > 64 chars

		a_thread->PushNewString(numberAsStringBuffer);
	}
	else if (GM_STRING == var->m_type)
	{
		a_thread->PushString(var->GetStringObjectSafe());
	}
	else
	{
		return GM_EXCEPTION;
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

static gmFunctionEntry s_mathLib[] = 
{ 
	{"UnitRandom",			gmfUnitRandom},
	{"SymmetricRandom",		gmfSymmetricRandom},
	{"RandRange",			gmfRandFloat},
	{"RandFloat",			gmfRandFloat},
	{"RandInt",				gmfRandInt},
	{"Clamp",				gmfClamp},
	{"Sin",					gmfSin},
	{"Cos",					gmfCos},
	{"Tan",					gmfTan},
	{"ASin",				gmfASin},
	{"ACos",				gmfACos},
	{"ATan",				gmfATan},
	{"RadToDeg",			gmfRadToDeg},
	{"DegToRad",			gmfDegToRad},
	{"Sign",				gmfSign},	
	{"ToBool",				gmfToBool},	
	{"Abs",					gmfAbs},
	{"Sqrt",				gmfSqrt},
	{"Min",					gmfMin},	
	{"Max",					gmfMax},
	{"Floor",				gmfFloor},	
	{"Ceil",				gmfCeil},	
	{"Round",				gmfRound},
	{"UnitCircleNormalize", gmfUnitCircleNormalize},
};

//////////////////////////////////////////////////////////////////////////

static gmFunctionEntry s_numLib[] = 
{ 
	{"String", gmfToString},
};

//////////////////////////////////////////////////////////////////////////

void gmBindMathLibrary(gmMachine * a_machine)
{
	// Register the bot functions.
	a_machine->RegisterLibrary(s_mathLib, sizeof(s_mathLib) / sizeof(s_mathLib[0]));
	a_machine->RegisterTypeLibrary(GM_INT, s_numLib, sizeof(s_numLib) / sizeof(s_numLib[0]));
	a_machine->RegisterTypeLibrary(GM_FLOAT, s_numLib, sizeof(s_numLib) / sizeof(s_numLib[0]));
}

//////////////////////////////////////////////////////////////////////////



