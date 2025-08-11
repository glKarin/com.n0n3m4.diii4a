//HUMANEHAD rww - include chunk to disable certain 2005 warnings.
#ifndef _DOTNETWARNINGS_H
#define _DOTNETWARNINGS_H

#ifdef _DOTNET_2005

//HUMANHEAD 2005TODO - see if any of these can/should be fixed
#define _CRT_SECURE_NO_DEPRECATE //get rid of 2005 deprecation warnings

//HUMANHEAD 2005TODO - see if any of these can/should be fixed
#pragma warning(disable : 4237)				//'export' keyword is not yet supported, but reserved for future use
#pragma warning(disable : 4996)				//'function' was declared deprecated

//HH rww SDK - this is all over the place
#pragma warning(disable : 4127)				//conditional expression is constant

#endif //_DOTNET_2005

#endif //_DOTNETWARNINGS_H
//HUMANHEAD END
