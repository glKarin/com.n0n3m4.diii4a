
#ifndef _GMSCHEMALIB_H_
#define _GMSCHEMALIB_H_

#include "gmConfig.h"
#include "gmVariable.h"

class gmMachine;

namespace gmSchema
{
	extern gmType GM_SCHEMA;

	void BindLib(gmMachine * a_machine);

	// helper functions
	enum ElementType 
	{
		EL_NONE,
		EL_TABLEOF,
		EL_ENUM,
		EL_FLOATRANGE,
		EL_INTRANGE,
		EL_NUMRANGE,
		EL_VARTYPE,
	};
	ElementType GetElementType(gmMachine *a_machine, gmVariable &a_schema);

	const gmTableObject *GetEnumOptions(gmMachine *a_machine, gmVariable &a_schema, gmVariable a_obj, gmVariable &a_current);
	bool GetNumRange(gmMachine *a_machine, gmVariable &a_schema, gmVariable a_obj, gmVariable &a_current, float &a_min, float &a_max);
};



#endif // _GMSCHEMALIB_H_
