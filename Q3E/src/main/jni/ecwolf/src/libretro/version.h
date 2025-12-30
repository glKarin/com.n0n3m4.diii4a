#ifndef _VERSION_H_
#define _VERSION_H_

const char *GetVersionDescription();
const char *GetVersionHash();
const char *GetVersionTime();
unsigned long long GetSaveVersion();
const char *GetSaveSignature();
const char *GetGameCaption();

#include "versiondefs.h"
#define MAIN_PK3 "ecwolf.pk3"

#define SAVEVERUNDEFINED 99999999999ull
#define MINSAVEVER	1370923175ull
// The following will be used as a less accurate fallback for non-version control builds
#define MINSAVEPRODVER 0x00100201

#endif
