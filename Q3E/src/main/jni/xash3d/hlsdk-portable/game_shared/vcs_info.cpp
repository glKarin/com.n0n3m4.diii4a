#include "vcs_info.h"

#ifdef XASH_BUILD_COMMIT
const char *g_VCSInfo_Commit = XASH_BUILD_COMMIT;
#else
const char *g_VCSInfo_Commit = "unknown-commit";
#endif

#ifdef XASH_BUILD_BRANCH
const char *g_VCSInfo_Branch = XASH_BUILD_BRANCH;
#else
const char *g_VCSInfo_Branch = "unknown-branch";
#endif
