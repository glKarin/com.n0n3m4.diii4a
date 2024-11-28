#ifndef _Q3E_UTILITY_H
#define _Q3E_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

void UnEscapeQuotes( char *arg );
int ParseCommandLine(char *cmdline, char **argv);

#ifdef __cplusplus
};
#endif

#endif // _Q3E_UTILITY_H