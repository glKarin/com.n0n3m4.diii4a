#ifndef CODE_ANALYSIS_MACROS
#define CODE_ANALYSIS_MACROS

#ifdef CODE_ANALYSIS
#include <CodeAnalysis\SourceAnnotations.h>
#define CHECK_PRINTF_ARGS			[SA_FormatString(Style="printf")]
#define CHECK_PARAM_VALID			[vc_attributes::Pre(Valid=vc_attributes::Yes)]
#define CHECK_VALID_BYTES(parm)		[vc_attributes::Pre(ValidBytes=#parm)]
#else
#define CHECK_PRINTF_ARGS
#define CHECK_PARAM_VALID
#define CHECK_VALID_BYTES(parm)
#endif

#endif
