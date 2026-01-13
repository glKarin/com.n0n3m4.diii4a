#ifndef _Q3E_MISC_H
#define _Q3E_MISC_H

#include <stdio.h>

#include "q3e_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int q3e_argc;
extern char **q3e_argv;

extern FILE *f_stdout;
extern FILE *f_stderr;

void Q3E_RedirectOutput(void);
void Q3E_CloseRedirectOutput(void);

void Q3E_DumpArgs(int argc, char **argv);
void Q3E_FreeArgs(void);

void Q3E_PrintInterface(const Q3E_Interface_t *d3interface);
void Q3E_PrintCallbacks(const Q3E_Callback_t *callback);
void Q3E_PrintInitialContext(const Q3E_InitialContext_t *context);

#ifdef __cplusplus
};
#endif

#endif // _Q3E_MISC_H